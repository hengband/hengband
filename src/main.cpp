/*
 * Copyright (c) 1997 Ben Harrison, and others
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

#include "core/asking-player.h"
#include "core/game-play.h"
#include "core/scores.h"
#include "game-option/runtime-arguments.h"
#include "io/files-util.h"
#include "io/inet.h"
#include "io/record-play-movie.h"
#include "io/signal-handlers.h"
#include "io/uid-checker.h"
#include "main/angband-initializer.h"
#include "player/process-name.h"
#include "system/angband-version.h"
#include "system/angband.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/string-processor.h"
#include "view/display-scores.h"
#include "wizard/spoiler-util.h"
#include "wizard/wizard-spoiler.h"
#include <filesystem>
#include <string>

/*
 * Available graphic modes
 */
#define GRAPHICS_NONE 0
#define GRAPHICS_ORIGINAL 1
#define GRAPHICS_ADAM_BOLT 2
#define GRAPHICS_HENGBAND 3

/*
 * Some machines have a "main()" function in their "main-xxx.c" file,
 * all the others use this file for their "main()" function.
 */

#ifndef WINDOWS
/*
 * A hook for "quit()".
 *
 * Close down, then fall back into "quit()".
 */
static void quit_hook(concptr s)
{
    /* Unused */
    (void)s;

    /* Scan windows */
    for (auto it = angband_terms.rbegin(); it != angband_terms.rend(); ++it) {
        auto term = *it;
        /* Unused */
        if (term == nullptr) {
            continue;
        }

        /* Nuke it */
        term_nuke(term);
    }
}

/*
 * Set the stack size and overlay buffer (see main-286.c")
 */
#ifdef PRIVATE_USER_PATH

/*
 * Create an ".angband/" directory in the users home directory.
 *
 * ToDo: Add error handling.
 * ToDo: Only create the directories when actually writing files.
 */
static void create_user_dir(void)
{
    const auto &dirpath = path_parse(PRIVATE_USER_PATH);
    const auto &dir_str = dirpath.string();
    mkdir(dir_str.data(), 0700);

    const auto &subdirpath = path_build(dirpath, VARIANT_NAME);
    const auto &subdir_str = subdirpath.string();
    mkdir(subdir_str.data(), 0700);
}

#endif /* PRIVATE_USER_PATH */

static void init_stuff()
{
    char libpath[1024]{};
    const auto tail = getenv("ANGBAND_PATH");
    strncpy(libpath, tail ? tail : DEFAULT_LIB_PATH, 511);
    if (!suffix(libpath, PATH_SEP)) {
        strcat(libpath, PATH_SEP);
    }

    init_file_paths(libpath);
}

/*
 * Handle a "-d<what>=<path>" option
 *
 * The "<what>" can be any string starting with the same letter as the
 * name of a subdirectory of the "lib" folder (i.e. "i" or "info").
 *
 * The "<path>" can be any legal path for the given system, and should
 * not end in any special path separator (i.e. "/tmp" or "~/.ang-info").
 */
static void change_path(concptr info)
{
    const auto s = angband_strchr(info, '=');

    /* Verify equal sign */
    if (!s) {
        quit_fmt("Try '-d<what>=<path>' not '-d%s'", info);
    }

    switch (tolower(info[0])) {
    case 'a':
        ANGBAND_DIR_APEX = s + 1;
        break;
    case 'f':
        ANGBAND_DIR_FILE = s + 1;
        break;
    case 'h':
        ANGBAND_DIR_HELP = s + 1;
        break;
    case 'i':
        ANGBAND_DIR_INFO = s + 1;
        break;
    case 'u':
        ANGBAND_DIR_USER = s + 1;
        break;
    case 'x':
        ANGBAND_DIR_XTRA = s + 1;
        break;
    case 'b':
        ANGBAND_DIR_BONE = s + 1;
        break;
    case 'd':
        ANGBAND_DIR_DATA = s + 1;
        break;
    case 'e':
        ANGBAND_DIR_EDIT = s + 1;
        break;
    case 's':
        ANGBAND_DIR_SAVE = s + 1;
        break;
    case 'z':
        ANGBAND_DIR_SCRIPT = s + 1;
        break;
    default:
        quit_fmt("Bad semantics in '-d%s'", info);
    }
}

static void display_usage(const char *program)
{
    /* Dump usage information */
    printf("Usage: %s [options] [-- subopts]\n", program);
    puts("  -n       Start a new character");
    puts("  -f       Request fiddle mode");
    puts("  -w       Request wizard mode");
    puts("  -b       Request BGM mode");
    puts("  -v       Request sound mode");
    puts("  -g       Request graphics mode");
    puts("  -o       Request original keyset");
    puts("  -r       Request rogue-like keyset");
    puts("  -M       Request monochrome mode");
    puts("  -s<num>  Show <num> high scores");
    puts("  -u<who>  Use your <who> savefile");
    puts("  -m<sys>  Force 'main-<sys>.c' usage");
    puts("  -d<def>  Define a 'lib' dir sub-path");
    puts("  --output-spoilers");
    puts("           Output auto generated spoilers and exit");
    puts("");

#ifdef USE_X11
    puts("  -mx11    To use X11");
    puts("  --       Sub options");
    puts("  -- -d    Set display name");
    puts("  -- -o    Request old 8x8 tile graphics");
    puts("  -- -a    Request Adam Bolt 16x16 tile graphics");
    puts("  -- -b    Request Bigtile graphics mode");
    puts("  -- -s    Turn off smoothscaling graphics");
    puts("  -- -n#   Number of terms to use");
    puts("");
#endif /* USE_X11 */

#ifdef USE_GCU
    puts("  -mgcu    To use GCU (GNU Curses)");
    puts("  --       Sub options");
    puts("  -- -o    old subwindow layout (no bigscreen)");
#endif /* USE_GCU */

#ifdef USE_CAP
    puts("  -mcap    To use CAP (\"Termcap\" calls)");
#endif /* USE_CAP */

    /* Actually abort the process */
    quit(nullptr);
}

/*
 * @brief 2文字以上のコマンドライン引数 (オプション)を実行する
 * @param opt コマンドライン引数
 * @return Usageを表示する必要があるか否か
 * @details v3.0.0 Alpha21時点では、スポイラー出力モードの判定及び実行を行う
 */
static bool parse_long_opt(const char *opt)
{
    if (strcmp(opt + 2, "output-spoilers") != 0) {
        return true;
    }

    init_stuff();
    init_angband(p_ptr, true);
    switch (output_all_spoilers()) {
    case SpoilerOutputResultType::SUCCESSFUL:
        puts("Successfully created a spoiler file.");
        quit(nullptr);
        break;
    case SpoilerOutputResultType::FILE_OPEN_FAILED:
        quit("Cannot create spoiler file.");
        break;
    case SpoilerOutputResultType::FILE_CLOSE_FAILED:
        quit("Cannot close spoiler file.");
        break;
    default:
        break;
    }

    return false;
}

/*
 * Simple "main" function for multiple platforms.
 *
 * Note the special "--" option which terminates the processing of
 * standard options.  All non-standard options (if any) are passed
 * directly to the "init_xxx()" function.
 */
int main(int argc, char *argv[])
{
    int i;

    bool done = false;
    bool new_game = false;
    int show_score = 0;
    concptr mstr = nullptr;
    bool args = true;

    /* Save the "program name" XXX XXX XXX */
    argv0 = argv[0];

#ifdef SET_UID

    /* Default permissions on files */
    (void)umask(022);

#endif

    /* Get the file paths */
    init_stuff();

#ifdef SET_UID

    /* Get the user id (?) */
    p_ptr->player_uid = getuid();

#ifdef VMS
    /* Mega-Hack -- Factor group id */
    p_ptr->player_uid += (getgid() * 1000);
#endif

#ifdef SAFE_SETUID

#ifdef _POSIX_SAVED_IDS

    /* Save some info for later */
    p_ptr->player_euid = geteuid();
    p_ptr->player_egid = getegid();

#endif

#endif

#endif

    /* Drop permissions */
    safe_setuid_drop();

#ifdef SET_UID

    /* Acquire the "user name" as a default player name */
    user_name(p_ptr->name, p_ptr->player_uid);

#ifdef PRIVATE_USER_PATH

    /* Create a directory for the users files. */
    create_user_dir();

#endif /* PRIVATE_USER_PATH */

#endif /* SET_UID */

    /* Process the command line arguments */
    bool browsing_movie = false;
    for (i = 1; args && (i < argc); i++) {
        /* Require proper options */
        if (argv[i][0] != '-') {
            display_usage(argv[0]);
            continue;
        }

        /* Analyze option */
        bool is_usage_needed = false;
        switch (argv[i][1]) {
        case 'N':
        case 'n': {
            new_game = true;
            break;
        }
        case 'B':
        case 'b': {
            arg_music = true;
            break;
        }
        case 'V':
        case 'v': {
            arg_sound = true;
            break;
        }
        case 'G':
        case 'g': {
            /* HACK - Graphics mode switches on the original tiles */
            arg_graphics = GRAPHICS_ORIGINAL;
            break;
        }
        case 'R':
        case 'r': {
            arg_force_roguelike = true;
            break;
        }
        case 'O':
        case 'o': {
            arg_force_original = true;
            break;
        }
        case 'S':
        case 's': {
            show_score = atoi(&argv[i][2]);
            if (show_score <= 0) {
                show_score = 10;
            }
            break;
        }
        case 'u':
        case 'U': {
            if (!argv[i][2]) {
                is_usage_needed = true;
                break;
            }

            strcpy(p_ptr->name, &argv[i][2]);
            break;
        }
        case 'm': {
            if (!argv[i][2]) {
                is_usage_needed = true;
                break;
            }

            mstr = &argv[i][2];
            break;
        }
        case 'M': {
            arg_monochrome = true;
            break;
        }
        case 'd':
        case 'D': {
            change_path(&argv[i][2]);
            break;
        }
        case 'x': {
            if (!argv[i][2]) {
                is_usage_needed = true;
                break;
            }

            prepare_browse_movie_with_path_build(&argv[i][2]);
            browsing_movie = true;
            break;
        }
        case '-': {
            if (argv[i][2] == '\0') {
                argv[i] = argv[0];
                argc = argc - i;
                argv = argv + i;
                args = false;
            } else {
                is_usage_needed = parse_long_opt(argv[i]);
            }
            break;
        }
        default: {
            is_usage_needed = true;
            break;
        }
        }

        if (!is_usage_needed) {
            continue;
        }

        display_usage(argv[0]);
    }

    /* Hack -- Forget standard args */
    if (args) {
        argc = 1;
        argv[1] = nullptr;
    }

    /* Process the player name */
    process_player_name(p_ptr, true);

    /* Install "quit" hook */
    quit_aux = quit_hook;

#ifdef USE_X11
    /* Attempt to use the "main-x11.c" support */
    if (!done && (!mstr || (streq(mstr, "x11")))) {
        extern errr init_x11(int, char **);
        if (0 == init_x11(argc, argv)) {
            ANGBAND_SYS = "x11";
            done = true;
        }
    }
#endif

#ifdef USE_GCU
    /* Attempt to use the "main-gcu.c" support */
    if (!done && (!mstr || (streq(mstr, "gcu")))) {
        extern errr init_gcu(int, char **);
        if (0 == init_gcu(argc, argv)) {
            ANGBAND_SYS = "gcu";
            done = true;
        }
    }
#endif

#ifdef USE_CAP
    /* Attempt to use the "main-cap.c" support */
    if (!done && (!mstr || (streq(mstr, "cap")))) {
        extern errr init_cap(int, char **);
        if (0 == init_cap(argc, argv)) {
            ANGBAND_SYS = "cap";
            done = true;
        }
    }
#endif

    /* Make sure we have a display! */
    if (!done) {
        quit("Unable to prepare any 'display module'!");
    }

    /* Hack -- If requested, display scores and quit */
    if (show_score > 0) {
        display_scores(0, show_score);
    }

    /* Catch nasty signals */
    signals_init();

    {
        TermCenteredOffsetSetter tcos(MAIN_TERM_MIN_COLS, MAIN_TERM_MIN_ROWS);

        /* Initialize */
        init_angband(p_ptr, false);

        /* Wait for response */
        pause_line(MAIN_TERM_MIN_ROWS - 1);
    }

    /* Play the game */
    play_game(p_ptr, new_game, browsing_movie);

    /* Quit */
    quit(nullptr);

    /* Exit */
    return 0;
}

#endif
