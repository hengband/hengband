/*!
 * @file h-config.h
 * @brief OSごとの差異を吸収してコンパイルするためのプリプロ群
 * The most basic "include" file.
 * This file simply includes other low level header files.
 * @date 2021/08/26
 * @author Hourier
 * @details
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 */

#pragma once

#if defined(WIN32) && !defined(WINDOWS)
#define WINDOWS
#endif

/*
 * OPTION: set "SET_UID" if the machine is a "multi-user" machine.
 * This option is used to verify the use of "uids" and "gids" for
 * various "Unix" calls, and of "pids" for getting a random seed,
 * and of the "umask()" call for various reasons, and to guess if
 * the "kill()" function is available, and for permission to use
 * functions to extract user names and expand "tildes" in filenames.
 * It is also used for "locking" and "unlocking" the score file.
 * Basically, SET_UID should *only* be set for "Unix" machines,
 * or for the "Atari" platform which is Unix-like, apparently
 */
#undef PATH_SEP
#ifdef WINDOWS
#define PATH_SEP "\\"
#else

#define PATH_SEP "/"
#define SET_UID
#define PRIVATE_USER_PATH "~/.angband"
#define SAVEFILE_USE_UID

#if !defined(HAVE_CONFIG_H) && !defined(ISC)
#define HAVE_USLEEP
#endif

#define SAFE_SETUID
#ifdef _POSIX_SAVED_IDS
#define SAFE_SETUID_POSIX
#endif

#ifndef DEFAULT_LIB_PATH
#define DEFAULT_LIB_PATH "./lib/"
#endif

#ifndef DEFAULT_VAR_PATH
#define DEFAULT_VAR_PATH DEFAULT_LIB_PATH
#endif

#endif

// clang-format off

#ifdef JP
  #ifdef SJIS
  #define iskanji(x) ((0x81 <= (unsigned char)(x) && (unsigned char)(x) <= 0x9f) || (0xe0 <= (unsigned char)(x) && (unsigned char)(x) <= 0xfc))
  #define iskana(x) (((unsigned char)(x) >= 0xA0) && ((unsigned char)(x) <= 0xDF))
  #elif defined EUC
  #define iskanji(x) (((unsigned char)(x) >= 0xa1 && (unsigned char)(x) <= 0xfe) || (unsigned char)(x) == 0x8e)
  #define iskana(x) (0)
  #else
  #error Oops! Please define "EUC" or "SJIS" for kanji-code of your system.
  #endif
#endif

#ifndef HAVE_CONFIG_H
  #ifdef JP
  #define USE_XIM
  #endif

  #if defined(USE_XIM)
  #define USE_LOCALE
  #endif
#endif

/*
 * OPTION: Default font (when using X11).
 */
#ifdef USE_XFT
  #ifdef JP
  #define DEFAULT_X11_FONT "monospace-24:lang=ja:spacing=90"
  #define DEFAULT_X11_FONT_SUB "sans-serif-16:lang=ja"
  #else
  #define DEFAULT_X11_FONT "monospace-24:lang=en:spacing=90"
  #define DEFAULT_X11_FONT_SUB "sans-serif-16:lang=en"
  #endif
#else
  #ifdef JP
  #define DEFAULT_X11_FONT "-*-*-medium-r-normal--24-*-*-*-*-*-iso8859-1,-*-*-medium-r-normal--24-*-*-*-*-*-jisx0208.1983-0"
  #define DEFAULT_X11_FONT_SUB "-*-*-medium-r-normal--16-*-*-*-*-*-iso8859-1,-*-*-medium-r-normal--16-*-*-*-*-*-jisx0208.1983-0"
  #else
  #define DEFAULT_X11_FONT "-*-*-medium-r-normal--24-*-*-*-*-*-iso8859-1"
  #define DEFAULT_X11_FONT_SUB "-*-*-medium-r-normal--16-*-*-*-*-*-iso8859-1"
  #endif
#endif

// clang-format on

/*
 * OPTION: Person to bother if something goes wrong.
 */
#define MAINTAINER "echizen@users.sourceforge.jp"

/*
 * OPTION: Default fonts (when using X11)
 */
#define DEFAULT_X11_FONT_0 DEFAULT_X11_FONT
#define DEFAULT_X11_FONT_1 DEFAULT_X11_FONT_SUB
#define DEFAULT_X11_FONT_2 DEFAULT_X11_FONT_SUB
#define DEFAULT_X11_FONT_3 DEFAULT_X11_FONT_SUB
#define DEFAULT_X11_FONT_4 DEFAULT_X11_FONT_SUB
#define DEFAULT_X11_FONT_5 DEFAULT_X11_FONT_SUB
#define DEFAULT_X11_FONT_6 DEFAULT_X11_FONT_SUB
#define DEFAULT_X11_FONT_7 DEFAULT_X11_FONT_SUB
