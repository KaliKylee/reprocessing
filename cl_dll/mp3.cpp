// Background music through the Xash3D FWGS engine (no FMOD).
#include "hud.h"
#include "cl_util.h"
#include "mp3.h"

#include <stdio.h>
#include <string.h>

int CMP3::Initialize()
{
	m_iIsPlaying = 0;
	m_bPaused = false;
	return 1;
}

int CMP3::Shutdown()
{
	StopMP3();
	return 1;
}

int CMP3::StopMP3()
{
	if (m_iIsPlaying)
		gEngfuncs.pfnClientCmd("stopsound\n");
	m_iIsPlaying = 0;
	return 1;
}

bool CMP3::Update()
{
	return m_iIsPlaying != 0;
}

void CMP3::PauseMP3(bool pause)
{
	// The engine's music stream cannot be paused from the client dll.
	m_bPaused = pause;
}

int CMP3::PlayMP3(const char* pszSong)
{
	if (!pszSong || !*pszSong)
		return 0;

	strncpy(m_szSong, pszSong, sizeof(m_szSong) - 1);
	m_szSong[sizeof(m_szSong) - 1] = '\0';

	char cmd[320];
	snprintf(cmd, sizeof(cmd), "music \"%s\"\n", m_szSong);
	gEngfuncs.pfnClientCmd(cmd);

	m_iIsPlaying = 1;
	return 1;
}
