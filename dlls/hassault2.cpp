/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/


#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"effects.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HASSAULT_AE_FIRE		( 1 )
#define		HASSAULT_AE_GREN		( 2 )

class CHassault : public CSquadMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int Classify(void);
	void HandleAnimEvent(MonsterEvent_t* pEvent);

	void DeathSound(void);
	void PainSound( void );

	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckMeleeAttack2(float flDot, float flDist) { return FALSE; }
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2(float flDot, float flDist);

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	void Shoot(void);

	void CheckAmmo ( void );

	static const char *pDeathSounds[];
	static const char *pPainSounds[];
	static const char *pBurstSounds[];

//	Schedule_t	*GetSchedule( void );

	BOOL	m_fThrowGrenade;
	float m_flNextGrenadeCheck;
	Vector	m_vecTossVelocity;

	int		m_cClipSize;

private:
	int		m_iShell;
};

LINK_ENTITY_TO_CLASS( monster_human_assault, CHassault );

TYPEDESCRIPTION	CHassault::m_SaveData[] = 
{
	DEFINE_FIELD( CHassault, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CHassault, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CHassault, m_cClipSize, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CHassault, CSquadMonster );

const char *CHassault::pPainSounds[] =
{
	"hgrunt/gr_pain1.wav",
	"hgrunt/gr_pain2.wav",
	"hgrunt/gr_pain3.wav",
	"hgrunt/gr_pain4.wav",
	"hgrunt/gr_pain5.wav",
};

const char *CHassault::pDeathSounds[] =
{
	"hgrunt/gr_die1.wav",
	"hgrunt/gr_die2.wav",
	"hgrunt/gr_die3.wav",
};

const char *CHassault::pBurstSounds[] =
{
	"hassault/hw_shoot1.wav",
	"hassault/hw_shoot2.wav",
	"hassault/hw_shoot3.wav",
};

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHassault::SetYawSpeed(void)
{
	int ys;

	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 150;
		break;
	case ACT_RUN:
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 120;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// PainSound
//=========================================================
void CHassault :: PainSound ( void )
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1, ATTN_NORM);
}

//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CHassault :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHassault::HandleAnimEvent(MonsterEvent_t *pEvent)
{	
	switch (pEvent->event)
	{
	case HASSAULT_AE_FIRE:

		Shoot();
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pBurstSounds), 1, ATTN_NORM);
		break;
	case HASSAULT_AE_GREN:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM);
		CGrenade::ShootContact( pev, GetGunPosition(), m_vecTossVelocity );
		m_fThrowGrenade = FALSE;
		if (g_iSkillLevel == SKILL_HARD)
			m_flNextGrenadeCheck = gpGlobals->time + RANDOM_FLOAT( 2, 5 );// wait a random amount of time before shooting again
		else
			m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
		break;

	default:
		CSquadMonster::HandleAnimEvent(pEvent);
		break;
	}
}


//=========================================================
// Spawn
//=========================================================
void CHassault::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/hassault.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	m_flNextGrenadeCheck = gpGlobals->time + 1;

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = 100;
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_MELEE_ATTACK1 | bits_CAP_DOORS_GROUP;

	m_HackedGunPos = Vector(0, 0, 55);

	m_cAmmoLoaded = 1;
	ClearConditions(bits_COND_NO_AMMO_LOADED);

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHassault::Precache()
{
	PRECACHE_MODEL("models/hassault.mdl");

	PRECACHE_SOUND("hassault/hw_gun4.wav");

	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pBurstSounds);

	PRECACHE_SOUND("hassault/hw_spin.wav");
	PRECACHE_SOUND("hassault/hw_spindown.wav");
	PRECACHE_SOUND("hassault/hw_spinup.wav");

	PRECACHE_SOUND( "weapons/glauncher.wav" );

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shell
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CHassault::Classify(void)
{
	return CLASS_HUMAN_MILITARY;
}

//=========================================================
// CheckRangeAttack1 - overridden for HGrunt, cause 
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the HGrunt can attack when the enemy is
// occluded (throw grenade over wall, etc). We must 
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
BOOL CHassault :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 && NoFriendlyFire() )
	{
		TraceResult	tr;

		if ( !m_hEnemy->IsPlayer() && flDist <= 64 )
		{
			// kick nonclients, but don't shoot at them.
			return FALSE;
		}

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 )
		{
			return TRUE;
		}
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack. 
//=========================================================
BOOL CHassault :: CheckRangeAttack2 ( float flDot, float flDist )
{
	// if the grunt isn't moving, it's ok to check.
	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck )
	{
		return m_fThrowGrenade;
	}

	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}
	
	Vector vecTarget;

	// find target
	// vecTarget = m_hEnemy->BodyTarget( pev->origin );
	vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
	// estimate position
	if (HasConditions( bits_COND_SEE_ENEMY))
		vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / gSkillData.hgruntGrenadeSpeed) * m_hEnemy->pev->velocity;

	// are any of my squad members near the intended grenade impact area?
	if ( InSquad() )
	{
		if (SquadMemberInRange( vecTarget, 256 ))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
			m_fThrowGrenade = FALSE;
		}
	}
	
	if ( ( vecTarget - pev->origin ).Length2D() <= 256 )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	Vector vecToss = VecCheckThrow( pev, GetGunPosition(), vecTarget, gSkillData.hgruntGrenadeSpeed, 0.5 );

	if ( vecToss != g_vecZero )
	{
		m_vecTossVelocity = vecToss;

		// throw a hand grenade
		m_fThrowGrenade = TRUE;
		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->time + 0.3; // 1/3 second.
	}
	else
	{
		// don't throw
		m_fThrowGrenade = FALSE;
		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
	}
	return m_fThrowGrenade;
}

//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CHassault :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	CBaseMonster *pEnemy;

	if ( m_hEnemy != NULL )
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if ( !pEnemy )
		{
			return FALSE;
		}
	}

	if ( flDist <= 64 && flDot >= 0.7	&& 
		 pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		 pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// Shoot
//=========================================================
void CHassault::Shoot(void)
{
	if( m_hEnemy == 0 )
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 2048, BULLET_MONSTER_MP5); // shoot +-5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	// m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
}


//=========================================================
// DeathSound 
//=========================================================
void CHassault::DeathSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), 1, ATTN_IDLE);
}
