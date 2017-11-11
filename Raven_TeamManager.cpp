#include "Raven_TeamManager.h"

#include "Debug\DebugConsole.h"



Raven_TeamManager::Raven_TeamManager(Vector2D weaponspawn)
	: m_pTarget(NULL), m_WeaponSpawn(weaponspawn), m_WeaponAvailable(false)
{
}


Raven_TeamManager::~Raven_TeamManager()
{
}


void Raven_TeamManager::AddTeammate(Raven_Teammate* pBot)
{
	if (pBot)
	{
		if(m_pTarget)
			debug_con << "ADDTEAMMATE : TEAM IS TARGETING " << m_pTarget->ID() << "";
		else
			debug_con << "ADDTEAMMATE : TEAM HAS NO TARGET" << "";

		//set the new teammate's target to the team target
		pBot->GetTargetSys()->SetTarget(m_pTarget);
		//add it to the team list
		m_Teammates.push_back(pBot);
	}
}


Raven_Bot* Raven_TeamManager::RemoveATeammate()
{
	Raven_Bot* pTeammate = m_Teammates.back();

	m_Teammates.pop_back();

	return pTeammate;
}


bool Raven_TeamManager::isTeammate(Raven_Bot* pTarget)
{
	if (!pTarget)
		return false;

	std::list<Raven_Teammate*>::const_iterator curBot = m_Teammates.begin();
	for (curBot; curBot != m_Teammates.end(); ++curBot)
	{
		if ((*curBot)->ID() == pTarget->ID())
			return true;
	}

	return false;
}


void Raven_TeamManager::Update()
{
	debug_con << "TEAM UPDATING" << "";

	//if the team has only one member
	if (m_Teammates.size() == 1)
	{
		//update the teammate's targeting system
		SearchNewTeamTarget();
	}

	//if the team has no target or if the last target died
	if (!m_pTarget || m_pTarget->isDead())
	{
		debug_con << "TEAM HAS NO TARGET" << "";
		//update the targeting system of each teammate to find the new target
		//if a target has been found
		if (SearchNewTeamTarget())
		{
			debug_con << "TEAM NOW TARGETING " << m_pTarget->ID() << "";
			//set the targeting system of each teammate to the team target
			std::list<Raven_Teammate*>::const_iterator curBot = m_Teammates.begin();
			for (curBot; curBot != m_Teammates.end(); ++curBot)
			{
				(*curBot)->GetTargetSys()->SetTarget(m_pTarget);
				debug_con << "TEAM UPDATE TEAMMATE " << (*curBot)->ID() << " : targeting bot " << (*curBot)->GetTargetBot()->ID() << "";
			}
		}
	}
	else
	{
		debug_con << "TEAM IS TARGETING " << m_pTarget->ID() << "";
		//set the targeting system of each teammate to the team target
		std::list<Raven_Teammate*>::const_iterator curBot = m_Teammates.begin();
		for (curBot; curBot != m_Teammates.end(); ++curBot)
		{
			(*curBot)->GetTargetSys()->SetTarget(m_pTarget);
			debug_con << "TEAM UPDATE TEAMMATE " << (*curBot)->ID() << " : targeting bot " << (*curBot)->GetTargetBot()->ID() << "";
		}
	}
}


bool Raven_TeamManager::SearchNewTeamTarget()
{
	std::list<Raven_Teammate*>::const_iterator curBot = m_Teammates.begin();
	Raven_Bot* curTarget = NULL;
	for (curBot; curBot != m_Teammates.end(); ++curBot)
	{
		//we update each teammate's targeting system to find a new target
		(*curBot)->GetTargetSys()->Update();
		curTarget = (*curBot)->GetTargetBot();

		//if a new target that does not belong to the team is found
		if (curTarget && !isTeammate(curTarget))
		{
			//it becomes the new team target
			m_pTarget = curTarget;
			return true;
		}
	}

	return false;
}


void Raven_TeamManager::ClearTarget()
{
	m_pTarget = NULL;

	std::list<Raven_Teammate*>::const_iterator curBot = m_Teammates.begin();
	for (curBot; curBot != m_Teammates.end(); ++curBot)
	{
		(*curBot)->GetTargetSys()->ClearTarget();
	}
}


void Raven_TeamManager::Clear()
{
	m_pTarget = NULL;
	m_Teammates.clear();
}