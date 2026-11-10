// input_xash.cpp
//
// Input backend for Xash3D FWGS (Android / touch). Replaces inputw32.cpp, which
// is Windows/SDL2-only. Movement and look events come from the engine touch
// code through IN_ClientMoveEvent / IN_ClientLookEvent (mobility API).
// Based on hlsdk-portable's FWGSInput.

#include "hud.h"
#include "cl_util.h"
#include "camera.h"
#include "kbutton.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"
#include "in_defs.h"
#include "../public/keydefs.h"
#include "view.h"
#include "Exports.h"
#include "mobility_int.h"

extern cl_enginefunc_t gEngfuncs;
extern bool iMouseInUse;

extern kbutton_t in_strafe;
extern kbutton_t in_mlook;
extern kbutton_t in_speed;
extern kbutton_t in_jlook;
extern kbutton_t in_forward;
extern kbutton_t in_back;
extern kbutton_t in_moveleft;
extern kbutton_t in_moveright;

extern cvar_t* cl_pitchdown;
extern cvar_t* cl_pitchup;
extern cvar_t* cl_sidespeed;
extern cvar_t* cl_forwardspeed;
extern cvar_t* cl_movespeedkey;

// Defined here (used by hud.cpp / input.cpp / hud_redraw.cpp)
cvar_t* sensitivity = nullptr;
cvar_t* in_joystick = nullptr;
bool g_iVisibleMouse = false;

mobile_engfuncs_t* gMobileEngfuncs = nullptr;

static cvar_t* cl_laddermode = nullptr;
static Vector dead_viewangles(0, 0, 0);

namespace
{
constexpr unsigned F = 1U << 0; // Forward
constexpr unsigned B = 1U << 1; // Back
constexpr unsigned L = 1U << 2; // Left
constexpr unsigned R = 1U << 3; // Right
constexpr unsigned T = 1U << 4; // Forward stop
constexpr unsigned S = 1U << 5; // Side stop

constexpr int BUTTON_DOWN = 1;

float ac_forwardmove = 0.0f;
float ac_sidemove = 0.0f;
int ac_movecount = 0;
float rel_yaw = 0.0f;
float rel_pitch = 0.0f;

int IsDead()
{
	return (gHUD.m_Health.m_iHealth <= 0) ? 1 : 0;
}

float GetMouseSensitivity()
{
	// Absurdly high sensitivity values can cause the game to hang, so clamp
	if (sensitivity->value > 10000.0f)
		gEngfuncs.Cvar_SetValue("sensitivity", 10000.0f);
	else if (sensitivity->value < 0.01f)
		gEngfuncs.Cvar_SetValue("sensitivity", 0.01f);
	return sensitivity->value;
}

void ToggleButtons(float forwardmove, float sidemove)
{
	static unsigned int moveflags = T | S;

	if (forwardmove)
		moveflags &= ~T;
	else if (!(moveflags & T))
	{
		in_forward.state &= ~BUTTON_DOWN;
		in_back.state &= ~BUTTON_DOWN;
		moveflags |= T;
	}

	if (sidemove)
		moveflags &= ~S;
	else if (!(moveflags & S))
	{
		in_moveleft.state &= ~BUTTON_DOWN;
		in_moveright.state &= ~BUTTON_DOWN;
		moveflags |= S;
	}

	if (forwardmove > 0.7f && !(moveflags & F))
	{
		moveflags |= F;
		in_forward.state |= BUTTON_DOWN;
	}
	if (forwardmove < 0.7f && (moveflags & F))
	{
		moveflags &= ~F;
		in_forward.state &= ~BUTTON_DOWN;
	}
	if (forwardmove < -0.7f && !(moveflags & B))
	{
		moveflags |= B;
		in_back.state |= BUTTON_DOWN;
	}
	if (forwardmove > -0.7f && (moveflags & B))
	{
		moveflags &= ~B;
		in_back.state &= ~BUTTON_DOWN;
	}
	if (sidemove > 0.9f && !(moveflags & R))
	{
		moveflags |= R;
		in_moveright.state |= BUTTON_DOWN;
	}
	if (sidemove < 0.9f && (moveflags & R))
	{
		moveflags &= ~R;
		in_moveright.state &= ~BUTTON_DOWN;
	}
	if (sidemove < -0.9f && !(moveflags & L))
	{
		moveflags |= L;
		in_moveleft.state |= BUTTON_DOWN;
	}
	if (sidemove > -0.9f && (moveflags & L))
	{
		moveflags &= ~L;
		in_moveleft.state &= ~BUTTON_DOWN;
	}
}
} // namespace

extern "C"
{
	// Touch movement from the engine: values in range [-1, 1]
	void DLLEXPORT IN_ClientMoveEvent(float forwardmove, float sidemove)
	{
		ac_forwardmove += forwardmove;
		ac_sidemove += sidemove;
		ac_movecount++;
	}

	// Touch look from the engine: relative angles in degrees
	void DLLEXPORT IN_ClientLookEvent(float relyaw, float relpitch)
	{
		rel_yaw += relyaw;
		rel_pitch += relpitch;
	}

	// Called by Xash3D FWGS to hand over the mobility API
	int DLLEXPORT HUD_MobilityInterface(mobile_engfuncs_t* gpMobileEngfuncs)
	{
		if (gpMobileEngfuncs->version != MOBILITY_API_VERSION)
			return 1;

		gMobileEngfuncs = gpMobileEngfuncs;
		return 0;
	}

	void DLLEXPORT IN_ActivateMouse() {}
	void DLLEXPORT IN_DeactivateMouse() {}
	void DLLEXPORT IN_Accumulate() {}
	void DLLEXPORT IN_ClearStates() {}

	void DLLEXPORT IN_MouseEvent(int mstate)
	{
		static int mouse_oldbuttonstate = 0;

		for (int i = 0; i < 5; i++)
		{
			if ((mstate & (1 << i)) && !(mouse_oldbuttonstate & (1 << i)))
				gEngfuncs.Key_Event(K_MOUSE1 + i, 1);

			if (!(mstate & (1 << i)) && (mouse_oldbuttonstate & (1 << i)))
				gEngfuncs.Key_Event(K_MOUSE1 + i, 0);
		}

		mouse_oldbuttonstate = mstate;
	}
}

void IN_ResetMouse() {}

void IN_GetMousePos(int* mx, int* my)
{
	// Xash3D reports the cursor through the engine
	gEngfuncs.GetMousePosition(mx, my);
}

void IN_Commands() {}

void IN_Shutdown() {}

// Rotate camera and add move values to usercmd
void IN_Move(float frametime, usercmd_t* cmd)
{
	Vector viewangles;
	bool fLadder = false;

	if (gHUD.m_iIntermission)
		return; // we can't move during intermission

	if (cl_laddermode->value != 2)
	{
		cl_entity_t* pplayer = gEngfuncs.GetLocalPlayer();
		if (pplayer)
			fLadder = pplayer->curstate.movetype == MOVETYPE_FLY;
	}

	if (IsDead())
		viewangles = dead_viewangles; // HACKHACK: see below
	else
		gEngfuncs.GetViewAngles(viewangles);

	const float mouse_sensitivity = gHUD.GetSensitivity() != 0 ? gHUD.GetSensitivity() : GetMouseSensitivity();
	rel_yaw *= mouse_sensitivity;
	rel_pitch *= mouse_sensitivity;
	viewangles[YAW] += rel_yaw;

	if (fLadder)
	{
		if (cl_laddermode->value == 1)
			viewangles[YAW] -= ac_sidemove * 5;
		ac_sidemove = 0;
	}

	viewangles[PITCH] += rel_pitch;

	if (viewangles[PITCH] > cl_pitchdown->value)
		viewangles[PITCH] = cl_pitchdown->value;
	if (viewangles[PITCH] < -cl_pitchup->value)
		viewangles[PITCH] = -cl_pitchup->value;

	// HACKHACK: change viewangles directly in viewcode, so viewangles when the
	// player is dead will not be changed on the server
	if (!IsDead())
		gEngfuncs.SetViewAngles(viewangles);

	dead_viewangles = viewangles; // keep them actual

	if (ac_movecount)
	{
		ToggleButtons(ac_forwardmove / ac_movecount, ac_sidemove / ac_movecount);

		if (ac_forwardmove)
			cmd->forwardmove = ac_forwardmove * cl_forwardspeed->value / ac_movecount;
		if (ac_sidemove)
			cmd->sidemove = ac_sidemove * cl_sidespeed->value / ac_movecount;
		if ((in_speed.state & 1) && (ac_sidemove || ac_forwardmove))
		{
			cmd->forwardmove *= cl_movespeedkey->value;
			cmd->sidemove *= cl_movespeedkey->value;
		}
	}

	ac_sidemove = ac_forwardmove = rel_pitch = rel_yaw = 0;
	ac_movecount = 0;
}

// Register cvars and reset data
void IN_Init()
{
	sensitivity = gEngfuncs.pfnRegisterVariable("sensitivity", "3", FCVAR_ARCHIVE);
	in_joystick = gEngfuncs.pfnRegisterVariable("joystick", "0", FCVAR_ARCHIVE);
	cl_laddermode = gEngfuncs.pfnRegisterVariable("cl_laddermode", "2", FCVAR_ARCHIVE);

	ac_forwardmove = ac_sidemove = rel_yaw = rel_pitch = 0;
	ac_movecount = 0;
}
