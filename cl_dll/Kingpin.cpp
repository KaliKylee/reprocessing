///////////////////////////////////////////////////////////////////////////////////////
// kingpin fake effects
///////////////////////////////////////////////////////////////////////////////////////

#include "hud.h"
#include "cl_util.h"

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"

#include "com_weapons.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "entity_types.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_materials.h"

#include "eventscripts.h"
#include "ev_hldm.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"

#include <string.h>

#include "r_studioint.h"
#include "com_model.h"
#include "Kingpin.h"

void V_PunchAxis(int axis, float punch);

void SpawnKingpinEffects(void)
{
	// Get screen dimensions
	int wide, tall;
	wide = ScreenWidth;
	tall = ScreenHeight;

	// Draw semi-transparent colored rectangle over entire screen
	gEngfuncs.pfnFillRGBABlend(0, 0, wide, tall,
		100,
		0,
		0,
		11);
	if (g_engfuncs.pfnRandomLong(0, 100) == 0)
	{
		SpawnFakeExpl();
	}
	/*
	if (g_engfuncs.pfnRandomLong(0, 500) == 1)
	{
		SpawnFakeGrenadeTE();
	}
	else if (g_engfuncs.pfnRandomLong(0, 1500) == 1)
	{
		SpawnFakeMonsterTE("models/islave.mdl");
	}
	else if (g_engfuncs.pfnRandomLong(0, 1500) == 1)
	{
		SpawnFakeMonsterTE("models/kingpin.mdl");
	}
	*/
}

Vector RandomVectorForFake(float min, float max)
{
	return Vector(
		g_engfuncs.pfnRandomFloat(min, max),
		g_engfuncs.pfnRandomFloat(min, max),
		g_engfuncs.pfnRandomFloat(min, max)
	);
}

Vector GetRandomFakeGrenadeThrow(const Vector& throwDirection, float basePower, float spread)
{
	Vector randomSpread = RandomVectorForFake(-spread, spread);
	Vector finalDir = (throwDirection + randomSpread).Normalize();

	float randomPower = g_engfuncs.pfnRandomFloat(basePower * 0.8, basePower * 1.2);

	return finalDir * randomPower;
}


void SpawnFakeGrenadeTE(void)
{
	Vector aim;
	gEngfuncs.GetViewAngles(aim);

	cl_entity_t* pPlayer = gEngfuncs.GetLocalPlayer();
	if (!pPlayer) return;

	Vector spawnOrigin = pPlayer->origin;

	spawnOrigin.x += g_engfuncs.pfnRandomFloat(-500, 500);
	spawnOrigin.y += g_engfuncs.pfnRandomFloat(-500, 500);

	// Create a temporary entity that follows grenade behavior
	int model = gEngfuncs.pEventAPI->EV_FindModelIndex("models/hgibs.mdl");
	TEMPENTITY* pTemp = gEngfuncs.pEfxAPI->R_TempModel(
		spawnOrigin,
		GetRandomFakeGrenadeThrow(aim, 1000, 45),
		Vector(0, 0, 0),
		3.0f, // Total lifetime
		model,
		0
	);
	gEngfuncs.pEventAPI->EV_PlaySound(-1, spawnOrigin, 0, "common/bodysplat.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);

	switch (g_engfuncs.pfnRandomLong(0, 4))
	{
	case 0:	gEngfuncs.pEventAPI->EV_PlaySound(-1, spawnOrigin, 0, "hgrunt/gr_die1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
	case 1:	gEngfuncs.pEventAPI->EV_PlaySound(-1, spawnOrigin, 0, "hgrunt/gr_die2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
	case 2:	gEngfuncs.pEventAPI->EV_PlaySound(-1, spawnOrigin, 0, "hgrunt/gr_die3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
	case 3: break;
	case 4: break;
	}

	spawnOrigin.x += g_engfuncs.pfnRandomFloat(-500, 500);
	spawnOrigin.y += g_engfuncs.pfnRandomFloat(-300, 250);

	if (pTemp)
	{
		pTemp->flags |= FTENT_GRAVITY | FTENT_COLLIDEWORLD | FTENT_ROTATE | FTENT_FADEOUT;
		pTemp->entity.baseline.angles = Vector(300, 200, 100); // Rotation
		pTemp->entity.curstate.rendermode = kRenderNormal;
		pTemp->entity.curstate.renderamt = 255;
	}
}

void SpawnFakeExpl(void)
{
	cl_entity_t* player = gEngfuncs.GetLocalPlayer();
	if (!player) return;
	switch (g_engfuncs.pfnRandomLong(0, 2))
	{
	case 0:	gEngfuncs.pEventAPI->EV_PlaySound(-1, player->origin, 0, "weapons/cbar_hitbod1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
	case 1:	gEngfuncs.pEventAPI->EV_PlaySound(-1, player->origin, 0, "weapons/cbar_hitbod2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
	case 2:	gEngfuncs.pEventAPI->EV_PlaySound(-1, player->origin, 0, "weapons/cbar_hitbod3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
	}
	V_PunchAxis(0, gEngfuncs.pfnRandomFloat(-5, 5));
	V_PunchAxis(1, gEngfuncs.pfnRandomFloat(-5, 5));
	gHUD.m_Health.CalcDamageDirection(Vector(gEngfuncs.pfnRandomFloat(-5, 5), gEngfuncs.pfnRandomFloat(-5, 5), gEngfuncs.pfnRandomFloat(-5, 5)));
	gHUD.m_Health.DrawPain(2.0);
}

void FakeMonCallback(struct tempent_s* ent, float frametime, float currenttime)
{
	if (!gHUD.m_Health.m_bNerve)
		ent->die = currenttime;
	else
		ent->die = currenttime + 1.0;
	if (ent->entity.curstate.frame >= ent->frameMax)
	{
		ent->entity.curstate.frame = ent->entity.curstate.frame - (int)(ent->entity.curstate.frame);
	}
}

void SpawnFakeMonsterTE(const char* modelName)
{
	//if (gHUD.m_iFakeMonsCount >= KINGPIN_MAX_FAKEMONS)
	//	return;
	int wide, tall;
	wide = ScreenWidth;
	tall = ScreenHeight;

	int model = gEngfuncs.pEventAPI->EV_FindModelIndex(modelName);

	gEngfuncs.pfnFillRGBABlend(0, 0, wide, tall,
		100,
		0,
		0,
		100);

	cl_entity_t* pPlayer = gEngfuncs.GetLocalPlayer();
	if (!pPlayer) return;

	Vector spawnOrigin = pPlayer->origin;

	spawnOrigin.x += g_engfuncs.pfnRandomFloat(-500, 500);
	spawnOrigin.y += g_engfuncs.pfnRandomFloat(-500, 500);

	pmtrace_t tr;

	gEngfuncs.pEventAPI->EV_SetTraceHull(1);
	gEngfuncs.pEventAPI->EV_PlayerTrace(spawnOrigin, spawnOrigin, PM_STUDIO_BOX, -1, &tr);

	if (tr.fraction < 1.0)
		return;

	Vector spawnEnd = spawnOrigin;
	spawnEnd.z -= 512;

	gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	gEngfuncs.pEventAPI->EV_PlayerTrace(spawnOrigin, spawnEnd, PM_STUDIO_BOX, -1, &tr);
	if (tr.fraction < 1.0)
	{
		spawnOrigin.z = tr.endpos.z;
	}

	// Create temporary entity
	gEngfuncs.pEfxAPI->R_TeleportSplash(spawnOrigin);
	TEMPENTITY* pTemp = gEngfuncs.pEfxAPI->R_TempModel(
		spawnOrigin,
		Vector(0, 0, 0), // No velocity
		Vector(0, 0, 0),
		4.0 , // 30 second lifetime
		model,
		0
	);

	if (pTemp)
	{
		pTemp->flags |= FTENT_SPRANIMATE | FTENT_SPRANIMATELOOP | FTENT_CLIENTCUSTOM;
		pTemp->entity.curstate.animtime = 0.1;
		pTemp->entity.curstate.framerate = 1.0;
		pTemp->entity.curstate.rendermode = kRenderNormal;
		pTemp->entity.curstate.renderamt = 255;
		pTemp->callback = FakeMonCallback;

		// Random rotation
		pTemp->entity.angles.y = g_engfuncs.pfnRandomFloat(-180, 180);

		gEngfuncs.pEventAPI->EV_PlaySound(-1, spawnOrigin, 0, "debris/beamstart7.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
	}
	//gHUD.m_iFakeMonsCount++;
}