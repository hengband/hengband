#pragma once

#include <mciapi.h>
#include <windows.h>

#include <string>

extern MCI_OPEN_PARMSW mci_open_parms;
extern MCI_PLAY_PARMS mci_play_parms;
extern std::wstring mci_device_type;

void setup_mci(HWND hWnd);
