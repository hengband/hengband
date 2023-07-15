#pragma once

#include <windows.h>

/*
 * Exclude parts of MMSYSTEM.H that are not needed
 */
#define MMNODRV /* Installable driver support */
#define MMNOWAVE /* Waveform support */
#define MMNOMIDI /* MIDI support */
#define MMNOAUX /* Auxiliary audio support */
#define MMNOTIMER /* Timer support */
#define MMNOJOY /* Joystick support */
#define MMNOMCI /* MCI support */
#define MMNOMMIO /* Multimedia file I/O support */

/*
 * Include some more files. Note: the Cygnus Cygwin compiler
 * doesn't use mmsystem.h instead it includes the winmm library
 * which performs a similar function.
 */
#include <mmsystem.h>
