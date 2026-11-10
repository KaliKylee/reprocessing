#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "decals.h"
#include "soundent.h"
#include "player.h"
#include "animation.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define ZOMBIE_AE_ATTACK_RIGHT			0x01
#define ZOMBIE_AE_ATTACK_LEFT			0x02
#define ZOMBIE_AE_ATTACK_GUTS_GRAB		0x03
#define ZOMBIE_AE_ATTACK_GUTS_THROW		4
#define GONOME_AE_ATTACK_BITE_FIRST		19
#define GONOME_AE_ATTACK_BITE_SECOND	20
#define GONOME_AE_ATTACK_BITE_THIRD		21
#define GONOME_AE_ATTACK_BITE_FINISH	22

#define ZOMBIE_FLINCH_DELAY		2

class COFGonomeGuts : public CBaseEntity
{
public:
	bool	Save(CSave& save);
	bool Restore(CRestore& restore);
	static TYPEDESCRIPTION m_SaveData[];

	void Spawn();
	void EXPORT TouchGut(CBaseEntity* pOther);
	void EXPORT Animate();
	static void Shoot(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity);
	static COFGonomeGuts* GonomeGutsCreate(const Vector& origin);
	void Launch(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity);
	int m_maxFrame;
};

TYPEDESCRIPTION COFGonomeGuts::m_SaveData[] =
{
	DEFINE_FIELD(COFGonomeGuts, m_maxFrame, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(COFGonomeGuts, CBaseEntity);

LINK_ENTITY_TO_CLASS(gonomeguts, COFGonomeGuts);

void COFGonomeGuts::Spawn()
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING("gonomeguts");

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;
	SET_MODEL(edict(), "sprites/bigspit.spr");
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 0;
	pev->rendercolor.z = 0;
	pev->frame = 0;
	pev->scale = 0.5;
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	m_maxFrame = MODEL_FRAMES(pev->modelindex) - 1;
}

void COFGonomeGuts::TouchGut(CBaseEntity* pOther)
{
	int iPitch = RANDOM_LONG(90, 110);
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "bullchicken/bc_acid1.wav", 1, ATTN_NORM, 0, iPitch);

	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit2.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	}

	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
	if (!pOther->pev->takedamage)
	{
		TraceResult tr;
		Vector norm = pev->velocity.Normalize();
		Vector vecSpot = pev->origin - norm * 16;

		UTIL_TraceLine(vecSpot, vecSpot + norm * 32, ignore_monsters, ENT(pev), &tr);
		UTIL_BloodDecalTrace(&tr, BLOOD_COLOR_RED);
		UTIL_BloodDrips(tr.vecEndPos, tr.vecPlaneNormal, BLOOD_COLOR_RED, 35);
	}
	else
		pOther->TakeDamage(pev, pev, gSkillData.gonomeDmgGuts, DMG_POISON);

	SetThink(&COFGonomeGuts::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.05f;
}

void COFGonomeGuts::Animate()
{
	pev->nextthink = gpGlobals->time + 0.1;
	if (pev->frame++)
	{
		if (pev->frame > m_maxFrame)
			pev->frame = 0;
	}
}

void COFGonomeGuts::Shoot(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity)
{
	COFGonomeGuts* pGuts = GetClassPtr<COFGonomeGuts>(NULL);
	pGuts->Spawn();

	UTIL_SetOrigin(pGuts->pev, vecStart);
	pGuts->pev->velocity = vecVelocity;
	pGuts->pev->owner = ENT(pevOwner);

	if (pGuts->m_maxFrame > 0)
	{
		pGuts->SetThink(&COFGonomeGuts::Animate);
		pGuts->pev->nextthink = gpGlobals->time + 0.1;
	}
}

COFGonomeGuts* COFGonomeGuts::GonomeGutsCreate(const Vector& origin)
{
	COFGonomeGuts* pGuts = GetClassPtr<COFGonomeGuts>(NULL);
	pGuts->Spawn();
	pGuts->pev->origin = origin;
	return pGuts;
}

void COFGonomeGuts::Launch(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity)
{
	UTIL_SetOrigin(pev, vecStart);
	pev->velocity = vecVelocity;
	pev->owner = ENT(pevOwner);
	SetThink(&COFGonomeGuts::Animate);
	pev->nextthink = gpGlobals->time + 0.1;
	SetTouch(&COFGonomeGuts::TouchGut);
}

enum
{
	TASK_GONOME_GET_PATH_TO_ENEMY_CORPSE = LAST_COMMON_TASK + 1,
};

class COFGonome : public CBaseMonster
{
public:
	bool	Save(CSave& save);
	bool Restore(CRestore& restore);

	static TYPEDESCRIPTION m_SaveData[];

	void Spawn();
	void Precache();
	void RunTask(Task_t* pTask);
	void SetYawSpeed();
	int Classify();
	void HandleAnimEvent(MonsterEvent_t* pEvent);
	int IgnoreConditions();

	void PainSound();
	void AlertSound();
	void IdleSound();
	void DeathSound(void);

	static const char* pIdleSounds[];
	static const char* pAlertSounds[];
	static const char* pPainSounds[];
	static const char* pAttackHitSounds[];
	static const char* pAttackMissSounds[];
	static const char* pDeathSounds[];

	bool CheckRangeAttack1(float flDot, float flDist);
	bool CheckRangeAttack2(float flDot, float flDist) { return false; }
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	bool CheckMeleeAttack1(float flDot, float flDist);

	bool IsRusher() override { return true; }

	Schedule_t* GetScheduleOfType(int Type);

	void Killed(entvars_t* pevAttacker, int iGib);

	void StartTask(Task_t* pTask);

	void SetActivity(Activity NewActivity);
	void UpdateOnRemove();

	CUSTOM_SCHEDULES;

	float m_flNextFlinch;
	float m_flNextThrowTime;

	COFGonomeGuts* m_pGonomeGuts;
	EHANDLE m_PlayerLocked;

	bool m_meleeAttack2;
	bool m_playedAttackSound;
};

TYPEDESCRIPTION COFGonome::m_SaveData[] =
{
	DEFINE_FIELD(COFGonome, m_flNextFlinch, FIELD_TIME),
	DEFINE_FIELD(COFGonome, m_flNextThrowTime, FIELD_TIME),
	DEFINE_FIELD(COFGonome, m_PlayerLocked, FIELD_EHANDLE),
};

IMPLEMENT_SAVERESTORE(COFGonome, CBaseMonster);

LINK_ENTITY_TO_CLASS(monster_gonome, COFGonome);
LINK_ENTITY_TO_CLASS(monster_gonome_xlab, COFGonome);

const char* COFGonome::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char* COFGonome::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char* COFGonome::pIdleSounds[] =
{
	"gonome/gonome_idle1.wav",
	"gonome/gonome_idle2.wav",
	"gonome/gonome_idle3.wav",
};

const char* COFGonome::pAlertSounds[] =
{
	"zombie/zo_alert10.wav",
	"zombie/zo_alert20.wav",
	"zombie/zo_alert30.wav",
};

const char* COFGonome::pPainSounds[] =
{
	"gonome/gonome_pain1.wav",
	"gonome/gonome_pain2.wav",
	"gonome/gonome_pain3.wav",
	"gonome/gonome_pain4.wav",
};

const char* COFGonome::pDeathSounds[] =
{
	"gonome/gonome_death2.wav",
	"gonome/gonome_death3.wav",
	"gonome/gonome_death4.wav",
};

Task_t tlGonomeVictoryDance[] =
{
	{TASK_STOP_MOVING, 0},
	{TASK_SET_FAIL_SCHEDULE, SCHED_IDLE_STAND},
	{TASK_WAIT, 0.2},
	{TASK_GONOME_GET_PATH_TO_ENEMY_CORPSE, 0},
	{TASK_WALK_PATH, 0},
	{TASK_WAIT_FOR_MOVEMENT, 0},
	{TASK_FACE_ENEMY, 0},
	{TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE},
	{TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE},
	{TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE},
	{TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE},
	{TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE},
	{TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE},
	{TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE},
};

Schedule_t slGonomeVictoryDance[] =
{
	{
		tlGonomeVictoryDance,
		ARRAYSIZE(tlGonomeVictoryDance),
		bits_COND_NEW_ENEMY |
			bits_COND_LIGHT_DAMAGE |
			bits_COND_HEAVY_DAMAGE,
		bits_SOUND_NONE,
		"BabyVoltigoreVictoryDance" 
	},
};

DEFINE_CUSTOM_SCHEDULES(COFGonome) { slGonomeVictoryDance, };
IMPLEMENT_CUSTOM_SCHEDULES(COFGonome, CBaseMonster);

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int COFGonome::Classify()
{
	return CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void COFGonome::SetYawSpeed()
{
	pev->yaw_speed = 120;
}

bool COFGonome::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if (bitsDamageType == DMG_BULLET)
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce(flDamage);
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.3;
	}

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void COFGonome::PainSound()
{
	int pitch = 95 + RANDOM_LONG(0, 9);
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0, ARRAYSIZE(pPainSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

void COFGonome::AlertSound()
{
	int pitch = 95 + RANDOM_LONG(0, 9);
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

void COFGonome::IdleSound()
{
	int pitch = 100 + RANDOM_LONG(-5, 5);
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pIdleSounds[RANDOM_LONG(0, ARRAYSIZE(pIdleSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

void COFGonome::DeathSound(void)
{
	int pitch = 100 + RANDOM_LONG(-5, 5);
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pDeathSounds[RANDOM_LONG(0, ARRAYSIZE(pDeathSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void COFGonome::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case ZOMBIE_AE_ATTACK_RIGHT:
	{
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.gonomeDmgOneSlash, DMG_SLASH);
		if (pHurt)
		{
			if ((pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) != 0)
			{
				pHurt->pev->punchangle.z = -9;
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 25;
			}

			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
	}
	break;

	case ZOMBIE_AE_ATTACK_LEFT:
	{
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.gonomeDmgOneSlash, DMG_SLASH);
		if (pHurt)
		{
			if ((pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) != 0)
			{
				pHurt->pev->punchangle.z = 9;
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 25;
			}
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
	}
	break;

	case ZOMBIE_AE_ATTACK_GUTS_GRAB:
	{
		Vector vecGutsPos, vecGutsAngles;
		GetAttachment(0, vecGutsPos, vecGutsAngles);

		if (!m_pGonomeGuts)
			m_pGonomeGuts = COFGonomeGuts::GonomeGutsCreate(vecGutsPos);

		m_pGonomeGuts->pev->skin = entindex();
		m_pGonomeGuts->pev->body = 1;
		m_pGonomeGuts->pev->aiment = edict();
		m_pGonomeGuts->pev->movetype = MOVETYPE_FOLLOW;
		UTIL_BloodDrips(vecGutsPos, UTIL_RandomBloodVector(), BLOOD_COLOR_RED, 35);
	}
	break;

	case ZOMBIE_AE_ATTACK_GUTS_THROW:
	{
		Vector vecGutsPos, vecGutsAngles;
		GetAttachment(0, vecGutsPos, vecGutsAngles);
		UTIL_MakeVectors(pev->angles);

		if (m_pGonomeGuts)
		{
			UTIL_Remove(m_pGonomeGuts);
			m_pGonomeGuts = NULL;
		}

		COFGonomeGuts* guts = COFGonomeGuts::GonomeGutsCreate(vecGutsPos);
		guts->pev->skin = 0;
		guts->pev->body = 0;
		guts->pev->aiment = NULL;
		guts->pev->movetype = MOVETYPE_FLY;
		UTIL_BloodDrips(vecGutsPos, UTIL_RandomBloodVector(), BLOOD_COLOR_RED, 35);

		if (m_hEnemy)
		{
			Vector vecSpitDir = ((m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs) - vecGutsPos).Normalize();
			guts->Launch(pev, vecGutsPos, vecSpitDir * 900);
		}
	}
	break;

	case GONOME_AE_ATTACK_BITE_FIRST:
	case GONOME_AE_ATTACK_BITE_SECOND:
	case GONOME_AE_ATTACK_BITE_THIRD:
	{
		if (m_hEnemy == NULL || (pev->origin - m_hEnemy->pev->origin).Length2D() < 48)
		{
			CBasePlayer* prevPlayer = (CBasePlayer*)((CBaseEntity*)m_PlayerLocked);
			m_PlayerLocked = NULL;
			if (prevPlayer && prevPlayer->IsAlive())
				prevPlayer->EnableControl(true);

			CBaseEntity* enemy = m_hEnemy;
			if (enemy && enemy->IsPlayer() && enemy->IsAlive())
			{
				static_cast<CBasePlayer*>(enemy)->EnableControl(false);
				m_PlayerLocked = enemy;
			}
		}

		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.gonomeDmgOneBite, DMG_SLASH);
		if (pHurt)
		{
			if ((pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) != 0)
			{
				pHurt->pev->punchangle.x = 9;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 25;
			}
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
	}
	break;

	case GONOME_AE_ATTACK_BITE_FINISH:
	{
		CBasePlayer* enemy = (CBasePlayer*)((CBaseEntity*)m_PlayerLocked);
		if (enemy && enemy->IsAlive())
			static_cast<CBasePlayer*>(enemy)->EnableControl(true);

		m_PlayerLocked = NULL;
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.gonomeDmgOneBite, DMG_SLASH);
		if (pHurt)
		{
			if ((pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) != 0)
			{
				pHurt->pev->punchangle.x = 9;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 25;
			}

			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
	}
	break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void COFGonome::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/gonome.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->health = gSkillData.gonomeHealth;
	pev->view_ofs = VEC_VIEW; // position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.5;	  // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	m_flNextThrowTime = gpGlobals->time;
	m_pGonomeGuts = NULL;
	m_PlayerLocked = NULL;

	if (FClassnameIs(pev, "monster_gonome_xlab"))
	{
		pev->skin = 3;
	}

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void COFGonome::Precache()
{
	int i;

	PRECACHE_MODEL("models/gonome.mdl");
	PRECACHE_MODEL("sprites/bigspit.spr");

	for (i = 0; i < ARRAYSIZE(pAttackHitSounds); i++)
		PRECACHE_SOUND((char*)pAttackHitSounds[i]);

	for (i = 0; i < ARRAYSIZE(pAttackMissSounds); i++)
		PRECACHE_SOUND((char*)pAttackMissSounds[i]);

	for (i = 0; i < ARRAYSIZE(pIdleSounds); i++)
		PRECACHE_SOUND((char*)pIdleSounds[i]);

	for (i = 0; i < ARRAYSIZE(pAlertSounds); i++)
		PRECACHE_SOUND((char*)pAlertSounds[i]);

	for (i = 0; i < ARRAYSIZE(pPainSounds); i++)
		PRECACHE_SOUND((char*)pPainSounds[i]);

	for (i = 0; i < ARRAYSIZE(pDeathSounds); i++)
		PRECACHE_SOUND((char*)pDeathSounds[i]);

	PRECACHE_SOUND("gonome/gonome_death2.wav");
	PRECACHE_SOUND("gonome/gonome_death3.wav");
	PRECACHE_SOUND("gonome/gonome_death4.wav");

	PRECACHE_SOUND("gonome/gonome_jumpattack.wav");

	PRECACHE_SOUND("gonome/gonome_melee1.wav");
	PRECACHE_SOUND("gonome/gonome_melee2.wav");

	PRECACHE_SOUND("gonome/gonome_run.wav");
	PRECACHE_SOUND("gonome/gonome_eat.wav");

	PRECACHE_SOUND("gonome/gonome_step1.wav");
	PRECACHE_SOUND("gonome/gonome_step2.wav");

	PRECACHE_SOUND("bullchicken/bc_acid1.wav");
	PRECACHE_SOUND("bullchicken/bc_spithit1.wav");
	PRECACHE_SOUND("bullchicken/bc_spithit2.wav");
}

void COFGonome::RunTask(Task_t* pTask)
{
	if (pTask->iTask == TASK_MELEE_ATTACK1)
	{
		if (!m_playedAttackSound)
		{
			if (m_meleeAttack2)
				EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "gonome/gonome_melee2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
			else
				EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "gonome/gonome_melee1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
			m_playedAttackSound = true;
		}
	}
	else
		m_playedAttackSound = false;

	CBaseMonster::RunTask(pTask);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
int COFGonome::IgnoreConditions()
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if (m_Activity == ACT_RANGE_ATTACK1)
		iIgnore |= bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_TOOFAR | bits_COND_ENEMY_OCCLUDED;
	else if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK1))
	{
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + ZOMBIE_FLINCH_DELAY;
	}

	return iIgnore;
}

bool COFGonome::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist <= 64.0 && flDot >= 0.7 && m_hEnemy)
		return (m_hEnemy->pev->flags & FL_ONGROUND) != 0;
	return false;
}

bool COFGonome::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist < 256)
		return false;

	if (IsMoving() && flDist >= 512)
		return false;

	if (flDist > 64 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextThrowTime)
	{
		if (m_hEnemy != 0)
		{
			if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 256)
				return false;
		}

		if (IsMoving())
			m_flNextThrowTime = gpGlobals->time + 5;
		else
			m_flNextThrowTime = gpGlobals->time + 0.5;

		return true;
	}

	return false;
}

Schedule_t* COFGonome::GetScheduleOfType(int Type)
{
	if (Type == SCHED_VICTORY_DANCE)
		return slGonomeVictoryDance;
	else
		return CBaseMonster::GetScheduleOfType(Type);
}

void COFGonome::Killed(entvars_t* pevAttacker, int iGib)
{
	if (m_pGonomeGuts)
	{
		UTIL_Remove(m_pGonomeGuts);
		m_pGonomeGuts = NULL;
	}

	CBasePlayer* player = (CBasePlayer*)((CBaseEntity*)m_PlayerLocked);
	if (player)
	{
		if (player && player->IsAlive())
			player->EnableControl(true);

		m_PlayerLocked = NULL;
	}

	CBaseMonster::Killed(pevAttacker, iGib);
}

void COFGonome::StartTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_GONOME_GET_PATH_TO_ENEMY_CORPSE:
	{
		if (m_pGonomeGuts)
		{
			UTIL_Remove(m_pGonomeGuts);
			m_pGonomeGuts = NULL;
		}

		UTIL_MakeVectors(pev->angles);
		if (BuildRoute(m_vecEnemyLKP - 64 * gpGlobals->v_forward, 64, NULL))
			TaskComplete();
		else
		{
			ALERT(at_aiconsole, "GonomeGetPathToEnemyCorpse failed!!\n");
			TaskFail();
		}
	}
	break;

	default:
		CBaseMonster::StartTask(pTask);
		break;
	}
}

void COFGonome::UpdateOnRemove()
{
	if (m_pGonomeGuts)
	{
		UTIL_Remove(m_pGonomeGuts);
		m_pGonomeGuts = NULL;
	}

	CBaseMonster::UpdateOnRemove();
}

void COFGonome::SetActivity(Activity NewActivity)
{
	int iSequence = ACTIVITY_NOT_AVAILABLE;
	void* pmodel = GET_MODEL_PTR(ENT(pev));

	if (NewActivity != ACT_RANGE_ATTACK1 && m_pGonomeGuts)
	{
		UTIL_Remove(m_pGonomeGuts);
		m_pGonomeGuts = NULL;
	}

	CBasePlayer* player = (CBasePlayer*)((CBaseEntity*)m_PlayerLocked);
	if (player)
	{
		if (NewActivity != ACT_MELEE_ATTACK1)
		{
			if (player && player->IsAlive())
				player->EnableControl(true);
			m_PlayerLocked = NULL;
		}
	}

	switch (NewActivity)
	{
	case ACT_MELEE_ATTACK1:
		if (m_hEnemy)
		{
			if ((pev->origin - m_hEnemy->pev->origin).Length2D() >= 48)
			{
				m_meleeAttack2 = false;
				iSequence = LookupSequence("attack1");
			}
			else
			{
				m_meleeAttack2 = true;
				iSequence = LookupSequence("attack2");
			}
		}
		else
			iSequence = LookupActivity(ACT_MELEE_ATTACK1);
		break;
	case ACT_RUN:
		if (m_hEnemy)
		{
			if ((pev->origin - m_hEnemy->pev->origin).Length() <= 512)
				iSequence = LookupSequence("runshort");
			else
				iSequence = LookupSequence("runlong");
		}
		else
			iSequence = LookupActivity(ACT_RUN);
		break;
	default:
		iSequence = LookupActivity(NewActivity);
		break;
	}

	if (iSequence > ACTIVITY_NOT_AVAILABLE)
	{
		if (pev->sequence != iSequence || !m_fSequenceLoops)
			pev->frame = 0;

		pev->sequence = iSequence;
		ResetSequenceInfo();
		SetYawSpeed();
	}
	else
	{
		ALERT(at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity);
		pev->sequence = 0;
	}

	m_Activity = NewActivity;
	m_IdealActivity = NewActivity;
}

//=========================================================
// DEAD GONOME PROP
//=========================================================
class CDeadGonome : public CBaseMonster
{
public:
	void Spawn();
	int Classify() { return CLASS_ALIEN_PASSIVE; }

	bool KeyValue(KeyValueData* pkvd);

	int m_iPose; // which sequence to display	-- temporary, don't need to save
	static char* m_szPoses[3];
};

char* CDeadGonome::m_szPoses[] = { "dead_on_stomach1", "dead_on_back", "dead_on_side" };

bool CDeadGonome::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		return true;
	}
	else
		return CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_gonome_dead, CDeadGonome);

//=========================================================
// ********** DeadGonome SPAWN **********
//=========================================================
void CDeadGonome::Spawn()
{
	PRECACHE_MODEL("models/gonome.mdl");
	SET_MODEL(ENT(pev), "models/gonome.mdl");

	pev->effects = 0;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_GREEN;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);

	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead gonome with bad pose\n");
	}

	// Corpses have less health
	pev->health = 8;

	MonsterInitDead();
}
