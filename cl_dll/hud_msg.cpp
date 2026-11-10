/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"
#include "mp3.h"

#include "particleman.h"
extern IParticleMan* g_pParticleMan;

extern BEAM* pBeam;
extern BEAM* pBeam2;
extern TEMPENTITY* pFlare; // Vit_amiN


/// USER-DEFINED SERVER MESSAGE HANDLERS

bool CHud::MsgFunc_ResetHUD(const char* pszName, int iSize, void* pbuf)
{
	ASSERT(iSize == 0);

	// clear all hud data
	HUDLIST* pList = m_pHudList;

	while (pList)
	{
		if (pList->p)
			pList->p->Reset();
		pList = pList->pNext;
	}

	//Reset weapon bits.
	m_iWeaponBits = 0ULL;

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	return true;
}

void CAM_ToFirstPerson();

void CHud::MsgFunc_ViewMode(const char* pszName, int iSize, void* pbuf)
{
	CAM_ToFirstPerson();
}

void CHud::MsgFunc_InitHUD(const char* pszName, int iSize, void* pbuf)
{
	// prepare all hud data
	HUDLIST* pList = m_pHudList;

	while (pList)
	{
		if (pList->p)
			pList->p->InitHUDData();
		pList = pList->pNext;
	}


	//TODO: needs to be called on every map change, not just when starting a new game
	if (g_pParticleMan)
		g_pParticleMan->ResetParticles();

	//Probably not a good place to put this.
	pBeam = pBeam2 = NULL;
	pFlare = NULL; // Vit_amiN: clear egon's beam flare
}


bool CHud::MsgFunc_GameMode(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	//Note: this user message could be updated to include multiple gamemodes, so make sure this checks for game mode 1
	//See CHalfLifeTeamplay::UpdateGameMode
	//TODO: define game mode constants
	m_Teamplay = READ_BYTE() == 1;

	return true;
}


bool CHud::MsgFunc_Damage(const char* pszName, int iSize, void* pbuf)
{
	int armor, blood;
	Vector from;
	int i;
	float count;

	BEGIN_READ(pbuf, iSize);
	armor = READ_BYTE();
	blood = READ_BYTE();

	for (i = 0; i < 3; i++)
		from[i] = READ_COORD();

	count = (blood * 0.5) + (armor * 0.5);

	if (count < 10)
		count = 10;

	// TODO: kick viewangles,  show damage visually

	return true;
}

bool CHud::MsgFunc_Concuss(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iConcussionEffect = READ_BYTE();
	if (0 != m_iConcussionEffect)
	{
		int r, g, b;
		UnpackRGB(r, g, b, RGB_YELLOWISH);
		this->m_StatusIcons.EnableIcon("dmg_concuss", r, g, b);
	}
	else
		this->m_StatusIcons.DisableIcon("dmg_concuss");
	return true;
}

bool CHud::MsgFunc_Weapons(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	const std::uint64_t lowerBits = READ_LONG();
	const std::uint64_t upperBits = READ_LONG();

	m_iWeaponBits = (lowerBits & 0XFFFFFFFF) | ((upperBits & 0XFFFFFFFF) << 32ULL);

	return true;
}
//RENDERERS START
#include "com_model.h"
#include "studio.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "dlight.h"
#include "triangleapi.h"

#include <stdio.h>
#include <string.h>
#include <memory.h>

#include "studio_util.h"
#include "r_studioint.h"

#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"
#include "Exports.h"

extern CGameStudioModelRenderer g_StudioRenderer;

int CHud::MsgFunc_StudioDecal(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	Vector pos, normal;
	pos.x = READ_COORD();
	pos.y = READ_COORD();
	pos.z = READ_COORD();
	normal.x = READ_COORD();
	normal.y = READ_COORD();
	normal.z = READ_COORD();
	int entindex = READ_SHORT();
	int decalType = 64;

	if (!entindex)
		return 1;

	cl_entity_t *pEntity = gEngfuncs.GetEntityByIndex(entindex);

	if (!pEntity)
		return 1;

	//g_StudioRenderer.StudioDecalForEntity(pos, normal, decalType, pEntity);
	//g_StudioRenderer.Decals_Add(entindex, pos, normal, decalType);

	return 1;
}
//RENDERERS END

int CHud::MsgFunc_PlayMP3(const char *pszName, int iSize, void *pbuf) //AJH -Killar MP3
{
	BEGIN_READ(pbuf, iSize);

	gMP3.PlayMP3(READ_STRING());

	return 1;
}

//Ku2zoff - Start

extern void EV_HLDM_Particles(vec_t Pos_X, vec_t Pos_Y, vec_t Pos_Z, float PosNorm_X, float PosNorm_Y, float PosNorm_Z, int DoPuff, int Material);

int CHud::MsgFunc_Impact(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int MatType = READ_SHORT();
	int DoPuffSpr = READ_BYTE();

	vec_t Pos_X, Pos_Y, Pos_Z;
	float PosNorm_X, PosNorm_Y, PosNorm_Z;

	Pos_X = READ_COORD();
	Pos_Y = READ_COORD();
	Pos_Z = READ_COORD();

	PosNorm_X = READ_COORD();
	PosNorm_Y = READ_COORD();
	PosNorm_Z = READ_COORD();

	EV_HLDM_Particles(Pos_X, Pos_Y, Pos_Z, PosNorm_X, PosNorm_Y, PosNorm_Z, DoPuffSpr, MatType);

	return 1;
}

//Ku2zoff - End