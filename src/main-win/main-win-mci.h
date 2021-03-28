#pragma once

#include "main-win/main-win-windows.h"

#include <mciapi.h>

extern MCI_OPEN_PARMS mci_open_parms;
extern MCI_PLAY_PARMS mci_play_parms;
#define MCI_DEVICE_TYPE_MAX_LENGTH 256
extern char mci_device_type[MCI_DEVICE_TYPE_MAX_LENGTH];

void setup_mci(HWND hWnd);
