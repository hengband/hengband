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

// @todo このアドレスもそろそろ変えた方が良い気がする.
// そもそもこのご時世にメールによる連絡を試みるか？
constexpr auto MAINTAINER = "echizen@users.sourceforge.jp";

// clang-format off

#if defined(WIN32) && !defined(WINDOWS)
#define WINDOWS
#endif

/*
 * @details
 * WindowsはShift-JIS (ひらがなや漢字が全て2バイト/文字)で定義されていることとする.
 * Unix系はEUC (ひらがなや漢字が全て2バイト/文字)で定義されていることとする.
 * これらは、このゲームがUTF系の可変長文字列に非対応であるためである.
 * Unix系は、マルチユーザ対応としてSET_UIDを定義する.
 * フォントやデフォルトのlibパス等も定義する.
 * HAVE_CONFIG_Hは、autoconfによりコンパイラスイッチ「-DHAVE_CONFIG_H」が付加される.
 */
#ifdef JP
#define iseuckanji(x) (((unsigned char)(x) >= 0xa1 && (unsigned char)(x) <= 0xfe) || (unsigned char)(x) == 0x8e)
#endif
#undef PATH_SEP
#ifdef WINDOWS
  #define PATH_SEP "\\"
  #ifdef JP
    #define iskanji(x) ((0x81 <= (unsigned char)(x) && (unsigned char)(x) <= 0x9f) || (0xe0 <= (unsigned char)(x) && (unsigned char)(x) <= 0xfc))
    #define iskana(x) (((unsigned char)(x) >= 0xA0) && ((unsigned char)(x) <= 0xDF))
  #endif
#else
  #define PATH_SEP "/"
  #define SET_UID
  #define PRIVATE_USER_PATH "~/.angband"
  #define SAVEFILE_USE_UID
  
  #if !defined(HAVE_CONFIG_H)
    #define HAVE_USLEEP
  #endif
  
  #define SAFE_SETUID
  /* Pick up system's definition of _POSIX_SAVED_IDS. */
  #include <unistd.h>
  #ifdef _POSIX_SAVED_IDS
    #define SAFE_SETUID_POSIX
  #endif
  
  #ifndef DEFAULT_LIB_PATH
    #define DEFAULT_LIB_PATH "./lib/"
  #endif
  
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
  
  #define DEFAULT_X11_FONT_0 DEFAULT_X11_FONT
  #define DEFAULT_X11_FONT_1 DEFAULT_X11_FONT_SUB
  #define DEFAULT_X11_FONT_2 DEFAULT_X11_FONT_SUB
  #define DEFAULT_X11_FONT_3 DEFAULT_X11_FONT_SUB
  #define DEFAULT_X11_FONT_4 DEFAULT_X11_FONT_SUB
  #define DEFAULT_X11_FONT_5 DEFAULT_X11_FONT_SUB
  #define DEFAULT_X11_FONT_6 DEFAULT_X11_FONT_SUB
  #define DEFAULT_X11_FONT_7 DEFAULT_X11_FONT_SUB
  
  #ifdef JP
    #ifdef EUC
      #define iskanji(x) (iseuckanji(x))
      #define iskana(x) (0)
    #else
      #error Oops! Please define "EUC" for kanji-code of your system.
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
#endif

// clang-format on

#if !defined(__GNUC__)
#define __attribute__(x)
#endif
