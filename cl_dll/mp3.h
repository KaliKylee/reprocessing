#pragma once
// Background music for Xash3D FWGS.
//
// The original CMP3 loaded FMOD (Windows DLL). On Android the engine already
// has its own music streamer, so this class just forwards to the engine.

#ifndef MP3_H
#define MP3_H

class CMP3
{
public:
	int Initialize();
	int Shutdown();
	int PlayMP3(const char* pszSong);
	int StopMP3();
	void PauseMP3(bool pause);
	bool Update(void);

	float fMp3time = 0;
	int m_iIsPlaying = 0;
	bool m_bPaused = false;
	char m_szSong[256] = {};
};

extern CMP3 gMP3;
#endif
