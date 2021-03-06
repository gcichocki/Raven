#pragma warning (disable:4786)
#include <windows.h>
#include <time.h>
#include "constants.h"
#include "misc/utils.h"
#include "Time/PrecisionTimer.h"
#include "Resource.h"
#include "misc/windowutils.h"
#include "misc/Cgdi.h"
#include "debug/DebugConsole.h"
#include "Raven_UserOptions.h"
#include "Raven_Game.h"
#include "Raven_WeaponSystem.h"
#include "navigation/Raven_PathPlanner.h"
#include "armory/Raven_Weapon.h"
#include "lua/Raven_Scriptor.h"
#include <iostream>
#include <fstream>
#include "NeuralNetwork\Net.h"
#include "NeuralNetwork\Neuron.h"

using namespace std;

bool once = false;
vector<unsigned> topology;
vector<double> inputVals;
LPARAM positionMouse;


//need to include this for the toolbar stuff
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")



//--------------------------------- Globals ------------------------------
//------------------------------------------------------------------------

char* g_szApplicationName = "Raven";
char* g_szWindowClassName = "MyWindowClass";


Raven_Game* g_pRaven;

//numero arme courante
static int armeCourante = 0; //type_blaster
//active or not the collection of data
static bool getDataFromHuman = false;
//active or not the neuralnetwork.
//When activate, a human can move a character, but let the neuralk shoot for him
static bool NeuronIsActivated = false;

//permit to add an information lign in the data file
bool firstTimeGetData = true;
//human bot shoot
static bool humanBotShot = false;


//Matcher arme courante au type d'arme
int WeaponType(int nbRoulette) {
	int res = 0;
	switch (nbRoulette) {
	case 0: {
		res = type_blaster;
	}
			break;
	case 1: {
		res = type_shotgun;
	}
			break;
	case 2: {
		res = type_rocket_launcher;
	}
			break;
	case 3: {
		res = type_rail_gun;
	}
	case 4:
		res = type_grenade;
		break;
	}
	return res;
}

void choiceWeaponScrollUp(int& armeCourante, int nbArme) {
	if (armeCourante >= nbArme) {
		armeCourante = 0;
	}
	else {
		armeCourante++;
	}

	int i = 0; //permet de savoir si on a fait le tour des armes possibles
	while (g_pRaven->PossessedBot()->isWeaponChanged() != true && i != nbArme) { //permet d'eviter de faire trop de molette pour passer de l'arme 1 � 3 si le bot n'a pas l'arme 2
		g_pRaven->ChangeWeaponOfPossessedBot(WeaponType(armeCourante));
		if (armeCourante == nbArme) {
			armeCourante = 0;
		}
		else {
			armeCourante++;
		}
		i++;
	}
	if (armeCourante == 0) {
		armeCourante = nbArme;
	}
	else {
		armeCourante--;
	}
}

void choiceWeaponScrollDown(int& armeCourante, int nbArme) {
	if (armeCourante <= 0) {
		armeCourante = nbArme;
	}
	else {
		armeCourante--;
	}

	int i = 0; //permet de savoir si on a fait le tour des armes possibles
	while (g_pRaven->PossessedBot()->isWeaponChanged() != true && i != nbArme) { //permet d'eviter de faire trop de molette pour passer de l'arme 1 � 3 si le bot n'a pas l'arme 2
		g_pRaven->ChangeWeaponOfPossessedBot(WeaponType(armeCourante));
		if (armeCourante == 0) {
			armeCourante = nbArme;
		}
		else {
			armeCourante--;
		}
		i++;
	}
	if (armeCourante == nbArme) {
		armeCourante = 0;
	}
	else {
		armeCourante++;
	}
}

void getFrameInfo(vector<double> *inputVals){
	
	inputVals->clear();
		int lifeHumanBot = 100;
		int lifeOtherBot = 100;
		int otherBotVisible = 0; //true if 1, 0 else
		int isHumanBotShot = 0; //true if 1, 0 else
		double distance = 0;
		int ammunition = 20; //The blaster ammo is 0 because we can shoot all the time.

		double posBotX;
		double posBotY;

		double posOtherBotX;
		double posOtherBotY;

		//get the life of the other bot and the human bot
		std::list<Raven_Bot*> bots = g_pRaven->GetAllBots();
		std::list<Raven_Bot*>::const_iterator curBot = bots.begin();
		for (curBot; curBot != bots.end(); ++curBot) {
			if (*curBot == g_pRaven->PossessedBot()) {
				lifeHumanBot = (*curBot)->Health();
				posBotX = (*curBot)->Pos().x;
				posBotY = (*curBot)->Pos().y;
			}
			else {
				lifeOtherBot = (*curBot)->Health();
				//get the distance between the 2 bots
				distance = g_pRaven->PossessedBot()->isPathPlanner()->GetCostToNode((*curBot)->isPathPlanner()->GetClosestNodeToPosition((*curBot)->Pos()));
				posOtherBotX = (*curBot)->Pos().x;
				posOtherBotY = (*curBot)->Pos().y;

			}
		}
		//knows if the other bot is visible or not
		std::vector<Raven_Bot*> botsVisible = g_pRaven->GetAllBotsInFOV(g_pRaven->PossessedBot());
		if (botsVisible.empty()) otherBotVisible = 0;
		else otherBotVisible = 1;

		//ammunition quantity
		if (armeCourante != 0) { //si l'arme n'est pas un blaster (sinon il en affiche 0 car on peut tirer a l'infini car le nb de ammo n'est pas pris en compte
			ammunition = g_pRaven->PossessedBot()->GetWeaponSys()->GetCurrentWeapon()->NumRoundsRemaining();
		}

		//get if the human bot shoot
		if (humanBotShot) isHumanBotShot = 1;
		else 0;

		//get the direction where the bot is facing
		Vector2D humanBotDirectionFacing = g_pRaven->PossessedBot()->Facing();
		int xFacing, yFacing;
		xFacing = 100 * humanBotDirectionFacing.x;
		humanBotDirectionFacing.x = (double)xFacing / 100;
		yFacing = 100 * humanBotDirectionFacing.y;
		humanBotDirectionFacing.y = (double)yFacing / 100;

		inputVals->push_back(lifeHumanBot);
		inputVals->push_back(lifeOtherBot);
		inputVals->push_back(otherBotVisible);
		inputVals->push_back(distance);
		inputVals->push_back(humanBotDirectionFacing.x);
		inputVals->push_back(humanBotDirectionFacing.y);
		inputVals->push_back(posBotX);
		inputVals->push_back(posBotY);
		inputVals->push_back(posOtherBotX);
		inputVals->push_back(posOtherBotY);
		//inputVals->push_back(WeaponType(armeCourante));
		inputVals->push_back(ammunition);

		
		humanBotShot = false;
}



/*Get data for the neural network, only work with 2 bots on the map*/
void getDataFromHumanBot() {
	ofstream fichier("dataHumanBot.txt", ios::out | ios::app);
	if (fichier)
	{
		if (firstTimeGetData) {
			fichier << "HealthHumanBot HealthOtherBot OtherBotVisible Distance HumanBotDirectionFacingX HumanBotDirectionFacingY HumanPosX HumanPosY BotPosX BotPosY Weapon Ammo HumanBotShot" << endl;
			firstTimeGetData = false;
		}
		int lifeHumanBot = 100;
		int lifeOtherBot = 100; 
		int otherBotVisible = 0; //true if 1, 0 else
		int isHumanBotShot = 0; //true if 1, 0 else
		double distance = 0;
		int ammunition = 20; //The blaster ammo is 0 because we can shoot all the time.

		double posBotX;
		double posBotY;

		double posOtherBotX;
		double posOtherBotY;

	//get the life of the other bot and the human bot
		std::list<Raven_Bot*> bots = g_pRaven->GetAllBots();
		std::list<Raven_Bot*>::const_iterator curBot = bots.begin();
		for (curBot; curBot != bots.end(); ++curBot) {
			if (*curBot == g_pRaven->PossessedBot()) {
				lifeHumanBot = (*curBot)->Health();
				posBotX = (*curBot)->Pos().x;
				posBotY = (*curBot)->Pos().y;
			}
			else {
				lifeOtherBot = (*curBot)->Health();
				//get the distance between the 2 bots
				distance = g_pRaven->PossessedBot()->isPathPlanner()->GetCostToNode((*curBot)->isPathPlanner()->GetClosestNodeToPosition((*curBot)->Pos()));
				posOtherBotX = (*curBot)->Pos().x;
				posOtherBotY = (*curBot)->Pos().y;

			}
		}
	//knows if the other bot is visible or not
		std::vector<Raven_Bot*> botsVisible = g_pRaven->GetAllBotsInFOV(g_pRaven->PossessedBot());
		if (botsVisible.empty()) otherBotVisible = 0;
		else otherBotVisible = 1;

	//ammunition quantity
		if (armeCourante != 0) { //si l'arme n'est pas un blaster (sinon il en affiche 0 car on peut tirer a l'infini car le nb de ammo n'est pas pris en compte
			ammunition = g_pRaven->PossessedBot()->GetWeaponSys()->GetCurrentWeapon()->NumRoundsRemaining();
		} 

	//get if the human bot shoot
		if (humanBotShot) isHumanBotShot = 1;
		else 0;

	//get the direction where the bot is facing
		Vector2D humanBotDirectionFacing = g_pRaven->PossessedBot()->Facing();
		int xFacing, yFacing;
		xFacing = 100 * humanBotDirectionFacing.x;
		humanBotDirectionFacing.x = (double)xFacing / 100;
		yFacing = 100 * humanBotDirectionFacing.y;
		humanBotDirectionFacing.y = (double)yFacing / 100;
		
		fichier << lifeHumanBot << " " << lifeOtherBot << " " << otherBotVisible << " " << distance << " " << humanBotDirectionFacing << " " << posBotX << " " << posBotY << " " << posOtherBotX << " " << posOtherBotY << WeaponType(armeCourante) << " " << ammunition << " " << isHumanBotShot  << endl;
		humanBotShot = false;
	}

	else cerr << "Can't open the file !" << endl;
}


//---------------------------- WindowProc ---------------------------------
//	
//	This is the callback function which handles all the windows messages
//-------------------------------------------------------------------------
LRESULT CALLBACK WindowProc (HWND   hwnd,

                             UINT   msg,
                             WPARAM wParam,
                             LPARAM lParam)
{

	//nombre arme differente possible au total
	int nbArme = 5;
 
   //these hold the dimensions of the client window area
	 static int cxClient, cyClient; 

	 //used to create the back buffer
   static HDC		hdcBackBuffer;
   static HBITMAP	hBitmap;
   static HBITMAP	hOldBitmap;

      //to grab filenames
   static TCHAR   szFileName[MAX_PATH],
                  szTitleName[MAX_PATH];


    switch (msg)
    {
	
		//A WM_CREATE msg is sent when your application window is first
		//created
    case WM_CREATE:
      {
         //to get get the size of the client window first we need  to create
         //a RECT and then ask Windows to fill in our RECT structure with
         //the client window size. Then we assign to cxClient and cyClient 
         //accordingly
			   RECT rect;

			   GetClientRect(hwnd, &rect);

			   cxClient = rect.right;
			   cyClient = rect.bottom;

         //seed random number generator
         srand((unsigned) time(NULL));

         
         //---------------create a surface to render to(backbuffer)

         //create a memory device context
         hdcBackBuffer = CreateCompatibleDC(NULL);

         //get the DC for the front buffer
         HDC hdc = GetDC(hwnd);

         hBitmap = CreateCompatibleBitmap(hdc,
                                          cxClient,
                                          cyClient);

			  
         //select the bitmap into the memory device context
			   hOldBitmap = (HBITMAP)SelectObject(hdcBackBuffer, hBitmap);

         //don't forget to release the DC
         ReleaseDC(hwnd, hdc);  
              
         //create the game
         g_pRaven = new Raven_Game();

        //make sure the menu items are ticked/unticked accordingly
        CheckMenuItemAppropriately(hwnd, IDM_NAVIGATION_SHOW_NAVGRAPH, UserOptions->m_bShowGraph);
        CheckMenuItemAppropriately(hwnd, IDM_NAVIGATION_SHOW_PATH, UserOptions->m_bShowPathOfSelectedBot);
        CheckMenuItemAppropriately(hwnd, IDM_BOTS_SHOW_IDS, UserOptions->m_bShowBotIDs);
        CheckMenuItemAppropriately(hwnd, IDM_NAVIGATION_SMOOTH_PATHS_QUICK, UserOptions->m_bSmoothPathsQuick);
        CheckMenuItemAppropriately(hwnd, IDM_NAVIGATION_SMOOTH_PATHS_PRECISE, UserOptions->m_bSmoothPathsPrecise);
        CheckMenuItemAppropriately(hwnd, IDM_BOTS_SHOW_HEALTH, UserOptions->m_bShowBotHealth);
        CheckMenuItemAppropriately(hwnd, IDM_BOTS_SHOW_TARGET, UserOptions->m_bShowTargetOfSelectedBot);
        CheckMenuItemAppropriately(hwnd, IDM_BOTS_SHOW_FOV, UserOptions->m_bOnlyShowBotsInTargetsFOV);
        CheckMenuItemAppropriately(hwnd, IDM_BOTS_SHOW_SCORES, UserOptions->m_bShowScore);
        CheckMenuItemAppropriately(hwnd, IDM_BOTS_SHOW_GOAL_Q, UserOptions->m_bShowGoalsOfSelectedBot);
        CheckMenuItemAppropriately(hwnd, IDM_NAVIGATION_SHOW_INDICES, UserOptions->m_bShowNodeIndices);
        CheckMenuItemAppropriately(hwnd, IDM_BOTS_SHOW_SENSED, UserOptions->m_bShowOpponentsSensedBySelectedBot);

      }

      break;

    case WM_KEYUP:
      {
        switch(wParam)
        {
         case VK_ESCAPE:
          {
            SendMessage(hwnd, WM_DESTROY, NULL, NULL);
          }
          
          break;

         case 'P':

           g_pRaven->TogglePause();

           break;

         case '1':
			if (!g_pRaven->isThereAHuman()) {
				 g_pRaven->ChangeWeaponOfPossessedBot(type_blaster);
			}

           break;

         case '2':
			 if (!g_pRaven->isThereAHuman()) {
				 g_pRaven->ChangeWeaponOfPossessedBot(type_shotgun);
			 }

           break;
           
         case '3':
			 if (!g_pRaven->isThereAHuman()) {
				 g_pRaven->ChangeWeaponOfPossessedBot(type_rocket_launcher);
			 }

           break;

         case '4':
			 if (!g_pRaven->isThereAHuman()) {
				 g_pRaven->ChangeWeaponOfPossessedBot(type_rail_gun);
			 }

           break;

		 case '5':
			 if (!g_pRaven->isThereAHuman())
				 g_pRaven->ChangeWeaponOfPossessedBot(type_grenade);
		   break;

         case 'X':
			 if (!g_pRaven->isThereAHuman()) {
				 g_pRaven->ExorciseAnyPossessedBot();
			 }
           break;

		 case 'T':
			 g_pRaven->AddTeammates(1);
			 break;
		
		 case 'Y':
			 g_pRaven->RemoveTeammate();
			 break;

		 case 'L':
			 g_pRaven->AddOrRemoveLeader();
			 break;

		 case 'F':
			 g_pRaven->AddFollowers(1);
			 break;

		 case 'G':
			 g_pRaven->RemoveFollower();

         case VK_UP:

           g_pRaven->AddBots(1); break;

         case VK_DOWN:

           g_pRaven->RemoveBot(); break;
           

        }
      }

      break;


    case WM_LBUTTONDOWN: //tire vers l'endroit ou se trouve le curseur de la souris
    {
		 g_pRaven->ClickLeftMouseButton(MAKEPOINTS(lParam));
		 humanBotShot = true;
    }
    
    break;

   case WM_RBUTTONDOWN: //se deplace a l'endroit ou se trouve le curseur de la souris
   {
	   if (!g_pRaven->isThereAHuman()) {
		   g_pRaven->ClickRightMouseButton(MAKEPOINTS(lParam));
		}
    }
    break;

   case WM_KEYDOWN:
   {
	   
	   break;
   }

   case WM_MOUSEWHEEL: //change les armes avec la molette de la souris
   {	if (g_pRaven->PossessedBot()) {
			g_pRaven->PossessedBot()->SetWeaponChanged(false);
			if ((short)HIWORD(wParam)/120 > 0) { //si la roulette est montee
				choiceWeaponScrollUp(armeCourante,nbArme);
			} else if ((short)HIWORD(wParam)/120 < 0) { //si la roulette est descendue
				choiceWeaponScrollDown(armeCourante, nbArme);
			}
		}  
   }
   break;

    case WM_COMMAND:
    {

     switch(wParam)
      {
      

      case IDM_GAME_LOAD:
          
          FileOpenDlg(hwnd, szFileName, szTitleName, "Raven map file (*.map)", "map");

          debug_con << "Filename: " << szTitleName << "";

          if (strlen(szTitleName) > 0)
          {
            g_pRaven->LoadMap(szTitleName);
          }

          break;

      case IDM_GAME_ADDBOT:

          g_pRaven->AddBots(1);
          
          break;

      case IDM_GAME_REMOVEBOT:
          
          g_pRaven->RemoveBot();

          break;

      case IDM_GAME_PAUSE:
          
          g_pRaven->TogglePause();

          break;



      case IDM_NAVIGATION_SHOW_NAVGRAPH:

        UserOptions->m_bShowGraph = !UserOptions->m_bShowGraph;

        CheckMenuItemAppropriately(hwnd, IDM_NAVIGATION_SHOW_NAVGRAPH, UserOptions->m_bShowGraph);

        break;
        
      case IDM_NAVIGATION_SHOW_PATH:

        UserOptions->m_bShowPathOfSelectedBot = !UserOptions->m_bShowPathOfSelectedBot;

        CheckMenuItemAppropriately(hwnd, IDM_NAVIGATION_SHOW_PATH, UserOptions->m_bShowPathOfSelectedBot);

        break;

      case IDM_NAVIGATION_SHOW_INDICES:

        UserOptions->m_bShowNodeIndices = !UserOptions->m_bShowNodeIndices;

        CheckMenuItemAppropriately(hwnd, IDM_NAVIGATION_SHOW_INDICES, UserOptions->m_bShowNodeIndices);

        break;

      case IDM_NAVIGATION_SMOOTH_PATHS_QUICK:

        UserOptions->m_bSmoothPathsQuick = !UserOptions->m_bSmoothPathsQuick;
        UserOptions->m_bSmoothPathsPrecise = false;
        CheckMenuItemAppropriately(hwnd, IDM_NAVIGATION_SMOOTH_PATHS_PRECISE, UserOptions->m_bSmoothPathsPrecise);
        CheckMenuItemAppropriately(hwnd, IDM_NAVIGATION_SMOOTH_PATHS_QUICK, UserOptions->m_bSmoothPathsQuick);

        break;

      case IDM_NAVIGATION_SMOOTH_PATHS_PRECISE:

        UserOptions->m_bSmoothPathsPrecise = !UserOptions->m_bSmoothPathsPrecise;
        UserOptions->m_bSmoothPathsQuick = false;
        CheckMenuItemAppropriately(hwnd, IDM_NAVIGATION_SMOOTH_PATHS_QUICK, UserOptions->m_bSmoothPathsQuick);
        CheckMenuItemAppropriately(hwnd, IDM_NAVIGATION_SMOOTH_PATHS_PRECISE, UserOptions->m_bSmoothPathsPrecise);

        break;

      case IDM_BOTS_SHOW_IDS:

        UserOptions->m_bShowBotIDs = !UserOptions->m_bShowBotIDs;

        CheckMenuItemAppropriately(hwnd, IDM_BOTS_SHOW_IDS, UserOptions->m_bShowBotIDs);

        break;

      case IDM_BOTS_SHOW_HEALTH:

        UserOptions->m_bShowBotHealth = !UserOptions->m_bShowBotHealth;
        
        CheckMenuItemAppropriately(hwnd, IDM_BOTS_SHOW_HEALTH, UserOptions->m_bShowBotHealth);

        break;

      case IDM_BOTS_SHOW_TARGET:

        UserOptions->m_bShowTargetOfSelectedBot = !UserOptions->m_bShowTargetOfSelectedBot;
        
        CheckMenuItemAppropriately(hwnd, IDM_BOTS_SHOW_TARGET, UserOptions->m_bShowTargetOfSelectedBot);

        break;

      case IDM_BOTS_SHOW_SENSED:

        UserOptions->m_bShowOpponentsSensedBySelectedBot = !UserOptions->m_bShowOpponentsSensedBySelectedBot;
        
        CheckMenuItemAppropriately(hwnd, IDM_BOTS_SHOW_SENSED, UserOptions->m_bShowOpponentsSensedBySelectedBot);

        break;


      case IDM_BOTS_SHOW_FOV:

        UserOptions->m_bOnlyShowBotsInTargetsFOV = !UserOptions->m_bOnlyShowBotsInTargetsFOV;
        
        CheckMenuItemAppropriately(hwnd, IDM_BOTS_SHOW_FOV, UserOptions->m_bOnlyShowBotsInTargetsFOV);

        break;

      case IDM_BOTS_SHOW_SCORES:

        UserOptions->m_bShowScore = !UserOptions->m_bShowScore;
        
        CheckMenuItemAppropriately(hwnd, IDM_BOTS_SHOW_SCORES, UserOptions->m_bShowScore);

        break;

      case IDM_BOTS_SHOW_GOAL_Q:

        UserOptions->m_bShowGoalsOfSelectedBot = !UserOptions->m_bShowGoalsOfSelectedBot;
        
        CheckMenuItemAppropriately(hwnd, IDM_BOTS_SHOW_GOAL_Q, UserOptions->m_bShowGoalsOfSelectedBot);

        break;

      }//end switch
    }

    
    case WM_PAINT:
      {
		if (g_pRaven->isThereAHuman()) {
			Vector2D position = Vector2D(0, 0);
			if (GetAsyncKeyState('Z')) {
				position += Vector2D(0, -3);
			}
			if (GetAsyncKeyState('Q')) {
				position += Vector2D(-3, 0);
			}
			if (GetAsyncKeyState('S')) {
				position += Vector2D(0, 3);
			}
			if (GetAsyncKeyState('D')) {
				position += Vector2D(3, 0);
			}
			if (position != Vector2D(0, 0)) {
				g_pRaven->ChangePositionHumanBot(position);
			}
		}
		      
         PAINTSTRUCT ps;
          
         BeginPaint (hwnd, &ps);

        //fill our backbuffer with white
         BitBlt(hdcBackBuffer,
                0,
                0,
                cxClient,
                cyClient,
                NULL,
                NULL,
                NULL,
                WHITENESS);
          
         
         gdi->StartDrawing(hdcBackBuffer);

         g_pRaven->Render();

         gdi->StopDrawing(hdcBackBuffer);


         //now blit backbuffer to front
			   BitBlt(ps.hdc, 0, 0, cxClient, cyClient, hdcBackBuffer, 0, 0, SRCCOPY); 
          
         EndPaint (hwnd, &ps);

      }

      break;

    //has the user resized the client area?
		case WM_SIZE:
		  {
        //if so we need to update our variables so that any drawing
        //we do using cxClient and cyClient is scaled accordingly
			  cxClient = LOWORD(lParam);
			  cyClient = HIWORD(lParam);

        //now to resize the backbuffer accordingly. First select
        //the old bitmap back into the DC
			  SelectObject(hdcBackBuffer, hOldBitmap);

        //don't forget to do this or you will get resource leaks
        DeleteObject(hBitmap); 

			  //get the DC for the application
        HDC hdc = GetDC(hwnd);

			  //create another bitmap of the same size and mode
        //as the application
        hBitmap = CreateCompatibleBitmap(hdc,
											  cxClient,
											  cyClient);

			  ReleaseDC(hwnd, hdc);
			  
			  //select the new bitmap into the DC
        SelectObject(hdcBackBuffer, hBitmap);

      }

      break;
          
		 case WM_DESTROY:
			 {

         //clean up our backbuffer objects
         SelectObject(hdcBackBuffer, hOldBitmap);

         DeleteDC(hdcBackBuffer);
         DeleteObject(hBitmap); 
         

         // kill the application, this sends a WM_QUIT message  
				 PostQuitMessage (0);
			 }

       break;

     }//end switch
	 	 

     //this is where all the messages not specifically handled by our 
		 //winproc are sent to be processed
		 return DefWindowProc (hwnd, msg, wParam, lParam);
}


//-------------------------------- WinMain -------------------------------
//
//	The entry point of the windows program
//------------------------------------------------------------------------
int WINAPI WinMain (HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR     szCmdLine, 
                    int       iCmdShow)
{
  MSG msg;
  //handle to our window
	HWND						hWnd;

 //the window class structure
	WNDCLASSEX     winclass;

  // first fill in the window class stucture
	winclass.cbSize        = sizeof(WNDCLASSEX);
	winclass.style         = CS_HREDRAW | CS_VREDRAW;
  winclass.lpfnWndProc   = WindowProc;
  winclass.cbClsExtra    = 0;
  winclass.cbWndExtra    = 0;
  winclass.hInstance     = hInstance;
  winclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
  winclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
  winclass.hbrBackground = NULL;
  winclass.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU1);
  winclass.lpszClassName = g_szWindowClassName;
	winclass.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

  //register the window class
  if (!RegisterClassEx(&winclass))
  {
		MessageBox(NULL, "Registration Failed!", "Error", 0);

	  //exit the application
		return 0;
  }
		

  try
  {  		 
		 //create the window and assign its ID to hwnd    
     hWnd = CreateWindowEx (NULL,                 // extended style
                            g_szWindowClassName,  // window class name
                            g_szApplicationName,  // window caption
                            WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION | WS_SYSMENU,  // window style
                            GetSystemMetrics(SM_CXSCREEN)/2 - WindowWidth/2,
                            GetSystemMetrics(SM_CYSCREEN)/2 - WindowHeight/2,                    
                            WindowWidth,     // initial x size
                            WindowHeight,    // initial y size
                            NULL,                 // parent window handle
                            NULL,                 // window menu handle
                            hInstance,            // program instance handle
                            NULL);                // creation parameters

     //make sure the window creation has gone OK
     if(!hWnd)
     {
       MessageBox(NULL, "CreateWindowEx Failed!", "Error!", 0);
     }

     
    //make the window visible
    ShowWindow (hWnd, iCmdShow);
    UpdateWindow (hWnd);
   
    //create a timer
    PrecisionTimer timer(FrameRate);

    //start the timer
    timer.Start();

    //enter the message loop
    bool bDone = false;

	//initialize the time when we peak up information avec data
	int currentTime = 0;
	int timeMax = 200;

	while (!bDone)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				// Stop loop if it's a quit message
				bDone = true;
			}

			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		if (timer.ReadyForNextFrame() && msg.message != WM_QUIT)
		{
			g_pRaven->Update();

			//render 
			RedrawWindow(hWnd);
		}
		//Get the data for the neural network
		if (getDataFromHuman && currentTime == timeMax) {
			getDataFromHumanBot();
			currentTime = 0;
		}
		else currentTime++;
		//if (!once) {
		if ( NeuronIsActivated && currentTime == timeMax) {
			topology.clear();
			topology.push_back(11);
			topology.push_back(4);
			topology.push_back(4);
			topology.push_back(4);
			topology.push_back(1);
			Net myNet(topology);
			
			myNet.InitializeWithFile();
			
		//}
			getFrameInfo(&inputVals);
			myNet.FeedForward(inputVals);
			vector<double> res;
			myNet.GetResult(res);
			if (res.back() > 0.9) {
				g_pRaven->ClickLeftMouseButton(MAKEPOINTS(Vector2D(27,345)));
				humanBotShot = true;
			}
			else {
				humanBotShot = false;
			}
		}
		else currentTime++;
      //give the OS a little time
      Sleep(2);
     					
    }//end while

  }//end try block

  catch (const std::runtime_error& err)
  {
    ErrorBox(std::string(err.what()));
    //tidy up
    delete g_pRaven;
    UnregisterClass( g_szWindowClassName, winclass.hInstance );
    return 0;
  }
  
 //tidy up
 UnregisterClass( g_szWindowClassName, winclass.hInstance );
 delete g_pRaven;
 return msg.wParam;
}



