// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

#include "Common/ChunkFile.h"
#include "InputCommon/InputConfig.h"

enum
{
	WIIMOTE_CHAN_0 = 0,
	WIIMOTE_CHAN_1,
	WIIMOTE_CHAN_2,
	WIIMOTE_CHAN_3,
	WIIMOTE_BALANCE_BOARD,
	MAX_WIIMOTES = WIIMOTE_BALANCE_BOARD,
	MAX_BBMOTES = 5,
};


#define WIIMOTE_INI_NAME  "WiimoteNew"

enum
{
	WIIMOTE_SRC_NONE   = 0,
	WIIMOTE_SRC_EMU    = 1,
	WIIMOTE_SRC_REAL   = 2,
	WIIMOTE_SRC_HYBRID = 3, // emu + real
};

extern unsigned int g_wiimote_sources[MAX_BBMOTES];

namespace Wiimote
{

void Shutdown();
void Initialize(void* const hwnd, bool wait = false);
void Resume();
void Pause();

unsigned int GetAttached();
void DoState(u8 **ptr, PointerWrap::Mode mode);
void EmuStateChange(EMUSTATE_CHANGE newState);
InputPlugin *GetPlugin();

void ControlChannel(int number, u16 channelID, const void* pData, u32 Size);
void InterruptChannel(int number, u16 channelID, const void* pData, u32 Size);
void Update(int number);

}

namespace WiimoteReal
{

void Initialize(bool wait = false);
void Stop();
void Shutdown();
void Resume();
void Pause();
void Refresh();

void LoadSettings();

}
