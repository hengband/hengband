/*!
 * @file main-win-mci.cpp
 * @brief Windows版固有実装(BGM再生用のMCI)
 */

#include "main-win/main-win-mci.h"

MCI_OPEN_PARMSW mci_open_parms;
MCI_PLAY_PARMS mci_play_parms;
std::wstring mci_device_type;

/*!
 * @brief MCI用設定の初期化
 * @param hWnd MCIコマンドのコールバックウインドウ
 */
void setup_mci(HWND hWnd)
{
    mci_open_parms.dwCallback = (DWORD)hWnd;
    mci_play_parms.dwCallback = (DWORD)hWnd;
}
