/*!
 * @file main-win-mci.cpp
 * @brief Windows版固有実装(BGM再生用のMCI)
 */

#include "main-win/main-win-mci.h"

MCI_OPEN_PARMS mop;
char mci_device_type[MCI_DEVICE_TYPE_MAX_LENGTH];
