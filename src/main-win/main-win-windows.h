#pragma once

/*
 * Exclude parts of WINDOWS.H that are not needed (Win32)
 */
#define WIN32_LEAN_AND_MEAN
#define NONLS /* All NLS defines and routines */
#define NOSERVICE /* All Service Controller routines, SERVICE_ equates, etc. */
#define NOMCX /* Modem Configuration Extensions */

/*
 * Include the "windows" support file
 */
#include <windows.h>
