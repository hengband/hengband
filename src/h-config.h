/*!
 * @file h-config.h
 * @brief 主に変愚/Zang時追加された基本事項のヘッダーファイル /
 * The most basic "include" file. This file simply includes other low level header files.
 * @date 2014/08/15
 * @author
 * 不明(変愚蛮怒スタッフ？)
 * @details
 * <pre>
 * Choose the hardware, operating system, and compiler.
 * Also, choose various "system level" compilation options.
 * A lot of these definitions take effect in "h-system.h"
 * Note that you may find it simpler to define some of these
 * options in the "Makefile", especially any options describing
 * what "system" is being used.
 * no system definitions are needed for 4.3BSD, SUN OS, DG/UX
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
*/

#ifndef INCLUDED_H_CONFIG_H
#define INCLUDED_H_CONFIG_H

/*
 * OPTION: Compile on Windows (automatic)
 */
#ifndef WINDOWS
/* #define WINDOWS */
#endif

/*
 * OPTION: Compile on a HPUX version of UNIX
 */
#ifndef HPUX
/* #define HPUX */
#endif

/*
 * OPTION: Compile on an SGI running IRIX
 */
#ifndef SGI
/* #define SGI */
#endif

/*
 * OPTION: Compile on an ultrix/4.2BSD/Dynix/etc. version of UNIX,
 * Do not define this if you are on any kind of SunOS.
 */
#ifndef ULTRIX
/* #define ULTRIX */
#endif


/*
 * Extract the "ULTRIX" flag from the compiler
 */
#if defined(ultrix) || defined(Pyramid)
# ifndef ULTRIX
#  define ULTRIX
# endif
#endif

/*
 * Extract the "SGI" flag from the compiler
 */
#ifdef sgi
# ifndef SGI
#  define SGI
# endif
#endif

/*
 * Extract the "WINDOWS" flag from the compiler
 */
#if defined(_Windows) || defined(__WINDOWS__) || \
    defined(__WIN32__) || defined(WIN32) || \
    defined(__WINNT__) || defined(__NT__)
# ifndef WINDOWS
#  define WINDOWS
# endif
#endif



/*
 * OPTION: Define "L64" if a "long" is 64-bits.  See "h-types.h".
 * The only such platform that angband is ported to is currently
 * DEC Alpha AXP running OSF/1 (OpenVMS uses 32-bit longs).
 */
#if defined(__alpha) && defined(__osf__)
# define L64
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
#if !defined(WINDOWS) && !defined(VM)
# define SET_UID
#endif


/*
 * OPTION: Set "USG" for "System V" versions of Unix
 * This is used to choose a "lock()" function, and to choose
 * which header files ("string.h" vs "strings.h") to include.
 * It is also used to allow certain other options, such as options
 * involving userid's, or multiple users on a single machine, etc.
 */
#ifdef SET_UID
# if defined(HPUX) || defined(SGI)
#  ifndef USG
#   define USG
#  endif
# endif
#endif


/*
 * Every system seems to use its own symbol as a path separator.
 * Default to the standard Unix slash, but attempt to change this
 * for various other systems.  Note that any system that uses the
 * "period" as a separator will have to pretend that it uses the
 * slash, and do its own mapping of period <-> slash.
 * Note that the VM system uses a "flat" directory, and thus uses
 * the empty string for "PATH_SEP".
 */
#undef PATH_SEP
#define PATH_SEP "/"
#if defined(WINDOWS) || defined(WINNT)
# undef PATH_SEP
# define PATH_SEP "\\"
#endif
#if defined(OS2)
# undef PATH_SEP
# define PATH_SEP "\\"
#endif
#ifdef __GO32__
# undef PATH_SEP
# define PATH_SEP "/"
#endif


/*
 * The Macintosh allows the use of a "file type" when creating a file
 */
#if defined(MACH_O_CARBON)
# define FILE_TYPE_TEXT 'TEXT'
# define FILE_TYPE_DATA 'DATA'
# define FILE_TYPE_SAVE 'SAVE'
# define FILE_TYPE(X) (_ftype = (X))
#else
# define FILE_TYPE(X) ((void)0)
#endif


/*
 * OPTION: Define "HAS_STRICMP" only if "stricmp()" exists.
 * Note that "stricmp()" is not actually used by Angband.
 */
/* #define HAS_STRICMP */

/*
 * Linux has "stricmp()" with a different name
 */
#if defined(linux)
# define HAS_STRICMP
# define stricmp strcasecmp
#endif


/*
 * OPTION: Define "HAVE_USLEEP" only if "usleep()" exists.
 *
 * Note that this is only relevant for "SET_UID" machines.
 * Note that new "SGI" machines have "usleep()".
 */
#if defined(SET_UID) && !defined(HAVE_CONFIG_H)
# if !defined(HPUX) && !defined(ULTRIX) && !defined(ISC)
#  define HAVE_USLEEP
# endif
#endif

#ifdef JP
# if defined(EUC)
#  define iskanji(x) (((unsigned char)(x) >= 0xa1 && (unsigned char)(x) <= 0xfe) || (unsigned char)(x) == 0x8e)
#  define iskana(x)  (0)
# elif defined(SJIS)
#  define iskanji(x) ((0x81 <= (unsigned char)(x) && (unsigned char)(x) <= 0x9f) || (0xe0 <= (unsigned char)(x) && (unsigned char)(x) <= 0xfc))
#  define iskana(x)  (((unsigned char)(x) >= 0xA0) && ((unsigned char)(x) <= 0xDF))
# else
#  error Oops! Please define "EUC" or "SJIS" for kanji-code of your system.
# endif
#endif

#endif /* INCLUDED_H_CONFIG_H */

/* Allow various special stuff (sound, graphics, etc.) */
#define USE_SPECIAL

#ifndef HAVE_CONFIG_H

/*
 * USE_FONTSET and/or USE_XIM can be commented out
 * when you don't want to use it.
 */
#define USE_FONTSET

#ifdef JP
#define USE_XIM
#endif

#if defined(USE_FONTSET) || defined(USE_XIM)
#define USE_LOCALE
#endif

#if defined(JP) && !defined(USE_FONTSET)
#define USE_JP_FONTSTRUCT
#endif

#endif /* HAVE_CONFIG_H */


/*
 * Look through the following lines, and where a comment includes the
 * tag "OPTION:", examine the associated "#define" statements, and decide
 * whether you wish to keep, comment, or uncomment them.  You should not
 * have to modify any lines not indicated by "OPTION".
 *
 * Note: Also examine the "system" configuration file "h-config.h"
 * and the variable initialization file "variable.c".  If you change
 * anything in "variable.c", you only need to recompile that file.
 *
 * And finally, remember that the "Makefile" will specify some rather
 * important compile time options, like what visual module to use.
 */


/*
 * OPTION: define "SPECIAL_BSD" for using certain versions of UNIX
 * that use the 4.4BSD Lite version of Curses in "main-gcu.c"
 */
/* #define SPECIAL_BSD */


/*
 * OPTION: Use the POSIX "termios" methods in "main-gcu.c"
 */
/* #define USE_TPOSIX */

/*
 * OPTION: Use the "termio" methods in "main-gcu.c"
 */
/* #define USE_TERMIO */

/*
 * OPTION: Use the icky BSD "tchars" methods in "main-gcu.c"
 */
/* #define USE_TCHARS */

/*
 * OPTION: Include "ncurses.h" instead of "curses.h" in "main-gcu.c"
 */
/* #define USE_NCURSES */


/*
 * OPTION: for multi-user machines running the game setuid to some other
 * user (like 'games') this SAFE_SETUID option allows the program to drop
 * its privileges when saving files that allow for user specified pathnames.
 * This lets the game be installed system wide without major security
 * concerns.  There should not be any side effects on any machines.
 *
 * This will handle "gids" correctly once the permissions are set right.
 */
#define SAFE_SETUID


/*
 * This flag enables the "POSIX" methods for "SAFE_SETUID".
 */
#ifdef _POSIX_SAVED_IDS
# define SAFE_SETUID_POSIX
#endif


/*
 * OPTION: for the AFS distributed file system, define this to ensure that
 * the program is secure with respect to the setuid code.  This option has
 * not been tested (to the best of my knowledge).  This option may require
 * some weird tricks with "player_uid" and such involving "defines".
 * Note that this option used the AFS library routines Authenticate(),
 * bePlayer(), beGames() to enforce the proper priviledges.
 * You may need to turn "SAFE_SETUID" off to use this option.
 */
/* #define SECURE */




/*
 * OPTION: Verify savefile Checksums (Angband 2.7.0 and up)
 * This option can help prevent "corruption" of savefiles, and also
 * stop intentional modification by amateur users.
 */
#define VERIFY_CHECKSUMS


/*
 * OPTION: Forbid the use of "fiddled" savefiles.  As far as I can tell,
 * a fiddled savefile is one with an internal timestamp different from
 * the actual timestamp.  Thus, turning this option on forbids one from
 * copying a savefile to a different name.  Combined with disabling the
 * ability to save the game without quitting, and with some method of
 * stopping the user from killing the process at the tombstone screen,
 * this should prevent the use of backup savefiles.  It may also stop
 * the use of savefiles from other platforms, so be careful.
 */
/* #define VERIFY_TIMESTAMP */


/*
 * OPTION: Forbid the "savefile over-write" cheat, in which you simply
 * run another copy of the game, loading a previously saved savefile,
 * and let that copy over-write the "dead" savefile later.  This option
 * either locks the savefile, or creates a fake "xxx.lok" file to prevent
 * the use of the savefile until the file is deleted.  Not ready yet.
 */
/* #define VERIFY_SAVEFILE */

 /*
 * OPTION: Handle signals
 */
#define HANDLE_SIGNALS


/*
 * Allow "Wizards" to yield "high scores"
 */
/* #define SCORE_WIZARDS */

/*
 * Allow "Cheaters" to yield "high scores"
 */
/* #define SCORE_CHEATERS */


#ifdef USE_SPECIAL

/*
 * OPTION: Allow the use of "sound" in various places.
 */
#define USE_SOUND

/*
 * OPTION: Allow the use of "graphics" in various places
 */
#define USE_GRAPHICS

/*
 * OPTION: Allow the use of "music" in various places
 */
#define USE_MUSIC

#endif /* USE_SPECIAL */


/*
 * OPTION: Set the "default" path to the angband "lib" directory.
 *
 * See "main.c" for usage, and note that this value is only used on
 * certain machines, primarily Unix machines.  If this value is used,
 * it will be over-ridden by the "ANGBAND_PATH" environment variable,
 * if that variable is defined and accessable.  The final slash is
 * optional, but it may eventually be required.
 *
 * Using the value "./lib/" below tells Angband that, by default,
 * the user will run "angband" from the same directory that contains
 * the "lib" directory.  This is a reasonable (but imperfect) default.
 *
 * If at all possible, you should change this value to refer to the
 * actual location of the "lib" folder, for example, "/tmp/angband/lib/"
 * or "/usr/games/lib/angband/", or "/pkg/angband/lib".
 */
#ifndef DEFAULT_PATH
# define DEFAULT_PATH "./lib/"
#endif


/*
 * OPTION: Create and use a hidden directory in the users home directory
 * for storing pref-files and character-dumps.
 */
#ifdef SET_UID
#define PRIVATE_USER_PATH "~/.angband"
#endif /* SET_UID */


/*
 * On multiuser systems, add the "uid" to savefile names
 */
#ifdef SET_UID
# define SAVEFILE_USE_UID
#endif

/*
 * OPTION: Check the "load" against "lib/file/load.txt"
 * This may require the 'rpcsvs' library
 */
/* #define CHECK_LOAD */


/*
 * OPTION: Capitalize the "user_name" (for "default" player name)
 * This option is only relevant on SET_UID machines.
 */
#define CAPITALIZE_USER_NAME



/*
 * OPTION: Person to bother if something goes wrong.
 */
/* #define MAINTAINER	"rr9@angband.org" */
#define MAINTAINER	"echizen@users.sourceforge.jp"


#ifdef JP
#ifndef USE_FONTSET
/*
 * OPTION: Default font (when using X11).
 */
#define DEFAULT_X11_FONT  "a24"
#define DEFAULT_X11_KFONT "kanji24"
#define DEFAULT_X11_FONT_SUB  "a16"
#define DEFAULT_X11_KFONT_SUB "kanji16"


/*
 * OPTION: Default fonts (when using X11)
 */
#define DEFAULT_X11_FONT_0  DEFAULT_X11_FONT
#define DEFAULT_X11_KFONT_0 DEFAULT_X11_KFONT
#define DEFAULT_X11_FONT_1  DEFAULT_X11_FONT_SUB
#define DEFAULT_X11_KFONT_1 DEFAULT_X11_KFONT_SUB
#define DEFAULT_X11_FONT_2  DEFAULT_X11_FONT_SUB
#define DEFAULT_X11_KFONT_2 DEFAULT_X11_KFONT_SUB
#define DEFAULT_X11_FONT_3  DEFAULT_X11_FONT_SUB
#define DEFAULT_X11_KFONT_3 DEFAULT_X11_KFONT_SUB
#define DEFAULT_X11_FONT_4  DEFAULT_X11_FONT_SUB
#define DEFAULT_X11_KFONT_4 DEFAULT_X11_KFONT_SUB
#define DEFAULT_X11_FONT_5  DEFAULT_X11_FONT_SUB
#define DEFAULT_X11_KFONT_5 DEFAULT_X11_KFONT_SUB
#define DEFAULT_X11_FONT_6  DEFAULT_X11_FONT_SUB
#define DEFAULT_X11_KFONT_6 DEFAULT_X11_KFONT_SUB
#define DEFAULT_X11_FONT_7  DEFAULT_X11_FONT_SUB
#define DEFAULT_X11_KFONT_7 DEFAULT_X11_KFONT_SUB

#else
/*
 * OPTION: Default font (when using X11).
 */
#define DEFAULT_X11_FONT \
	"-*-*-medium-r-normal--24-*-*-*-*-*-iso8859-1" \
	",-*-*-medium-r-normal--24-*-*-*-*-*-jisx0208.1983-0"
/*	"12x24" \
	",kanji24"*/
#define DEFAULT_X11_FONT_SUB \
	"-*-*-medium-r-normal--16-*-*-*-*-*-iso8859-1" \
	",-*-*-medium-r-normal--16-*-*-*-*-*-jisx0208.1983-0"
/*	"8x16" \
	",kanji16"*/

/*
 * OPTION: Default fonts (when using X11)
 */
#define DEFAULT_X11_FONT_0		DEFAULT_X11_FONT
#define DEFAULT_X11_FONT_1		DEFAULT_X11_FONT_SUB
#define DEFAULT_X11_FONT_2		DEFAULT_X11_FONT_SUB
#define DEFAULT_X11_FONT_3		DEFAULT_X11_FONT_SUB
#define DEFAULT_X11_FONT_4		DEFAULT_X11_FONT_SUB
#define DEFAULT_X11_FONT_5		DEFAULT_X11_FONT_SUB
#define DEFAULT_X11_FONT_6		DEFAULT_X11_FONT_SUB
#define DEFAULT_X11_FONT_7		DEFAULT_X11_FONT_SUB
#endif

#else
/*
 * OPTION: Default font (when using X11).
 */
#define DEFAULT_X11_FONT		"9x15"

/*
 * OPTION: Default fonts (when using X11)
 */
#define DEFAULT_X11_FONT_0		"10x20"
#define DEFAULT_X11_FONT_1		"9x15"
#define DEFAULT_X11_FONT_2		"9x15"
#define DEFAULT_X11_FONT_3		"5x8"
#define DEFAULT_X11_FONT_4		"5x8"
#define DEFAULT_X11_FONT_5		"5x8"
#define DEFAULT_X11_FONT_6		"5x8"
#define DEFAULT_X11_FONT_7		"5x8"
#endif


/*
 * OPTION: Gamma correct X11 colours.
 */
 
#define SUPPORT_GAMMA

/*
 * Hack -- Mach-O (native binary format of OS X) is basically a Un*x
 * but has Mac OS/Windows-like user interface
 */
#ifdef MACH_O_CARBON
# ifdef PRIVATE_USER_PATH
#  undef PRIVATE_USER_PATH
# endif
# ifdef SAVEFILE_USE_UID
#  undef SAVEFILE_USE_UID
# endif
#endif

/*
 * OPTION: Attempt to prevent all "cheating"
 */
/* #define VERIFY_HONOR */


/*
 * React to the "VERIFY_HONOR" flag
 */
#ifdef VERIFY_HONOR
# define VERIFY_SAVEFILE
# define VERIFY_CHECKSUMS
# define VERIFY_TIMESTAMPS
#endif

/*
 * Check the modification time of *_info.raw files
 * (by Keldon Jones)
 */
#define CHECK_MODIFICATION_TIME

/*
 * Use the new sorting routines for creation
 * of the monster allocation table
 */
#define SORT_R_INFO


#ifndef HAVE_CONFIG_H
#define WORLD_SCORE
#endif /* HAVE_CONFIG_H */
