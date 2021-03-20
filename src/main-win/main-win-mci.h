#pragma once

#include "main-win/main-win-windows.h"

#include <mciapi.h>

extern MCI_OPEN_PARMS mop;
#define MCI_DEVICE_TYPE_MAX_LENGTH 256
extern char mci_device_type[MCI_DEVICE_TYPE_MAX_LENGTH];
