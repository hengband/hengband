/* File: z-config.h */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

/* Purpose: Angband specific configuration stuff */

/* Allow debug commands */
#define USE_DEBUG

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
 * OPTION: See the Makefile(s), where several options may be declared.
 *
 * Some popular options include "USE_GCU" (allow use with Unix "curses"),
 * "USE_X11" (allow basic use with Unix X11), "USE_XAW" (allow use with
 * Unix X11 plus the Athena Widget set), and "USE_CAP" (allow use with
 * the "termcap" library, or with hard-coded vt100 terminals).
 *
 * The old "USE_NCU" option has been replaced with "USE_GCU".
 *
 * Several other such options are available for non-unix machines,
 * such as "MACINTOSH", "WINDOWS", "USE_IBM", "USE_EMX".
 *
 * You may also need to specify the "system", using defines such as
 * "SOLARIS" (for Solaris), etc, see "h-config.h" for more info.
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
 * OPTION: Use "blocking getch() calls" in "main-gcu.c".
 * Hack -- Note that this option will NOT work on many BSD machines
 * Currently used whenever available, if you get a warning about
 * "nodelay()" undefined, then make sure to undefine this.
 */
#if defined(SYS_V) || defined(AMIGA)
# define USE_GETCH
#endif


/*
 * OPTION: Use the "curs_set()" call in "main-gcu.c".
 * Hack -- This option will not work on most BSD machines
 */
#ifdef SYS_V
# define USE_CURS_SET
#endif


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
 * Prevent problems on (non-Solaris) Suns using "SAFE_SETUID".
 * The SAFE_SETUID code is weird, use it at your own risk...
 */
#if defined(SUNOS) && !defined(SOLARIS)
# undef SAFE_SETUID_POSIX
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
 * OPTION: Hack -- Compile in support for "Cyborg" mode
 */
/*#define ALLOW_BORG*/

#ifdef USE_DEBUG

/*
 * OPTION: Hack -- Compile in support for "Wizard Commands"
 */
#define ALLOW_WIZARD

/*
 * OPTION: Hack -- Compile in support for "Spoiler Generation"
 */
#define ALLOW_SPOILERS

#endif /* USE_DEBUG */

/*
 * OPTION: Allow "do_cmd_colors" at run-time
 */
#define ALLOW_COLORS

/*
 * OPTION: Allow "do_cmd_visuals" at run-time
 */
#define ALLOW_VISUALS

/*
 * OPTION: Allow "do_cmd_macros" at run-time
 */
#define ALLOW_MACROS

/*
 * OPTION: Allow characteres to be "auto-rolled"
 */
#define ALLOW_AUTOROLLER


/*
 * OPTION: Allow monsters to "flee" when hit hard
 */
#define ALLOW_FEAR

/*
 * OPTION: Allow monsters to "flee" from strong players
 */
#define ALLOW_TERROR


/*
 * OPTION: Allow parsing of the ascii template files in "init.c".
 * This must be defined if you do not have valid binary image files.
 * It should be usually be defined anyway to allow easy "updating".
 */
#define ALLOW_TEMPLATES

/*
 * OPTION: Allow loading of pre-2.7.0 savefiles.  Note that it takes
 * about 15K of code in "save-old.c" to parse the old savefile format.
 * Angband 2.8.0 will ignore a lot of info from pre-2.7.0 savefiles.
 */
#define ALLOW_OLD_SAVEFILES


/*
 * OPTION: Handle signals
 */
#define HANDLE_SIGNALS


/*
 * Allow "Wizards" to yield "high scores"
 */
/* #define SCORE_WIZARDS */

/*
 * Allow "Borgs" to yield "high scores"
 */
/*#define SCORE_BORGS*/

/*
 * Allow "Cheaters" to yield "high scores"
 */
/* #define SCORE_CHEATERS */



/*
 * OPTION: Maximum flow depth when using "MONSTER_FLOW"
 */
#define MONSTER_FLOW_DEPTH 32


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
 * OPTION: Hack -- EMX stuff
 */
#ifdef USE_EMX

/* Do not handle signals */
# undef HANDLE_SIGNALS

#endif


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
 * OPTION: Check the "time" against "lib/file/hours.txt"
 */
/* #define CHECK_TIME */

/*
 * OPTION: Check the "load" against "lib/file/load.txt"
 * This may require the 'rpcsvs' library
 */
/* #define CHECK_LOAD */


/*
 * OPTION: For some brain-dead computers with no command line interface,
 * namely Macintosh, there has to be some way of "naming" your savefiles.
 * The current "Macintosh" hack is to make it so whenever the character
 * name changes, the savefile is renamed accordingly.  But on normal
 * machines, once you manage to "load" a savefile, it stays that way.
 * Macintosh is particularly weird because you can load savefiles that
 * are not contained in the "lib:save:" folder, and if you change the
 * player's name, it will then save the savefile elsewhere.  Note that
 * this also gives a method of "bypassing" the "VERIFY_TIMESTAMP" code.
 */
/*
#if defined(MACINTOSH) || defined(WINDOWS) || defined(AMIGA)
# define SAVEFILE_MUTABLE
#endif
*/

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
 * Hack -- Special "ancient machine" versions
 */
#if defined(USE_286) || defined(ANGBAND_LITE_MAC)
# ifndef ANGBAND_LITE
#  define ANGBAND_LITE
# endif
#endif

/*
 * OPTION: Attempt to minimize the size of the game
 */
#ifndef ANGBAND_LITE
/* #define ANGBAND_LITE */
#endif

/*
 * Hack -- React to the "ANGBAND_LITE" flag
 */
#ifdef ANGBAND_LITE
# undef ALLOW_COLORS
# undef ALLOW_VISUALS
# undef ALLOW_MACROS
# undef ALLOW_OLD_SAVEFILES
# undef ALLOW_BORG
# undef ALLOW_WIZARD
# undef ALLOW_SPOILERS
# undef ALLOW_TEMPLATES
# undef DELAY_LOAD_R_TEXT
# define DELAY_LOAD_R_TEXT
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

/* Zangband options: */

/* Allow hordes of 'similar' monsters */
# define MONSTER_HORDES

/*
 * OPTION: Repeat last command -- TNB
 */
#define ALLOW_REPEAT

/*
 * OPTION: Make opening and closing things easy -- TNB
 */
#define ALLOW_EASY_OPEN

/*
 * OPTION: Make disarming traps easy -- TNB
 */
#define ALLOW_EASY_DISARM

/*
 * OPTION: Make floor stacks easy -- TNB
 */
#define ALLOW_EASY_FLOOR

/*
 * Check the modification time of *_info.raw files
 * (by Keldon Jones)
 */
#ifndef MAC_MPW
#define CHECK_MODIFICATION_TIME
#endif

/*
 * Use the new sorting routines for creation
 * of the monster allocation table
 */
#define SORT_R_INFO


#ifndef HAVE_CONFIG_H

#ifndef MSDOS
/*
 * Use world score server
 */
#define WORLD_SCORE
#endif

#endif /* HAVE_CONFIG_H */
