/*!
 * @file angband-initializer.cpp
 * @brief 変愚蛮怒のシステム初期化
 * @date 2014/01/28
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.\n
 * </pre>
 */

#include "main/angband-initializer.h"
#include "dungeon/dungeon.h"
#include "floor/wild.h"
#include "info-reader/feature-reader.h"
#include "io/files-util.h"
#include "io/read-pref-file.h"
#include "io/uid-checker.h"
#include "main/game-data-initializer.h"
#include "main/info-initializer.h"
#include "market/building-initializer.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags7.h"
#include "system/angband-version.h"
#include "system/monster-race-definition.h"
#include "system/system-variables.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "time.h"
#include "util/angband-files.h"
#include "world/world.h"
#ifndef WINDOWS
#include <dirent.h>
#include "util/string-processor.h"
#endif

/*!
 * @brief 各データファイルを読み取るためのパスを取得する
 * Find the default paths to all of our important sub-directories.
 * @param path パス保管先の文字列
 */
void init_file_paths(char *libpath, char *varpath)
{
    char *libtail, *vartail;
    char buf[1024];

    string_free(ANGBAND_DIR);
    string_free(ANGBAND_DIR_APEX);
    string_free(ANGBAND_DIR_BONE);
    string_free(ANGBAND_DIR_DATA);
    string_free(ANGBAND_DIR_EDIT);
    string_free(ANGBAND_DIR_SCRIPT);
    string_free(ANGBAND_DIR_FILE);
    string_free(ANGBAND_DIR_HELP);
    string_free(ANGBAND_DIR_INFO);
    string_free(ANGBAND_DIR_SAVE);
    string_free(ANGBAND_DIR_DEBUG_SAVE);
    string_free(ANGBAND_DIR_USER);
    string_free(ANGBAND_DIR_XTRA);

    ANGBAND_DIR = string_make(libpath);
    libtail = libpath + strlen(libpath);
    vartail = varpath + strlen(varpath);
    strcpy(vartail, "apex");
    ANGBAND_DIR_APEX = string_make(varpath);
    strcpy(vartail, "bone");
    ANGBAND_DIR_BONE = string_make(varpath);
    strcpy(vartail, "data");
    ANGBAND_DIR_DATA = string_make(varpath);
    strcpy(libtail, "edit");
    ANGBAND_DIR_EDIT = string_make(libpath);
    strcpy(libtail, "script");
    ANGBAND_DIR_SCRIPT = string_make(libpath);
    strcpy(libtail, "file");
    ANGBAND_DIR_FILE = string_make(libpath);
    strcpy(libtail, "help");
    ANGBAND_DIR_HELP = string_make(libpath);
    strcpy(libtail, "info");
    ANGBAND_DIR_INFO = string_make(libpath);
    strcpy(libtail, "pref");
    ANGBAND_DIR_PREF = string_make(libpath);
    strcpy(vartail, "save");
    ANGBAND_DIR_SAVE = string_make(varpath);
    path_build(buf, sizeof(buf), ANGBAND_DIR_SAVE, "log");
    ANGBAND_DIR_DEBUG_SAVE = string_make(buf);
#ifdef PRIVATE_USER_PATH
    path_build(buf, sizeof(buf), PRIVATE_USER_PATH, VERSION_NAME);
    ANGBAND_DIR_USER = string_make(buf);
#else
    strcpy(vartail, "user");
    ANGBAND_DIR_USER = string_make(varpath);
#endif
    strcpy(libtail, "xtra");
    ANGBAND_DIR_XTRA = string_make(libpath);

    time_t now = time(nullptr);
    struct tm *t = localtime(&now);
    char tmp[128];
    strftime(tmp, sizeof(tmp), "%Y-%m-%d-%H-%M-%S", t);
    path_build(debug_savefile, sizeof(debug_savefile), ANGBAND_DIR_DEBUG_SAVE, tmp);

#ifdef WINDOWS
    struct _finddata_t c_file;
    intptr_t hFile;
    char log_file_expr[1024];
    path_build(log_file_expr, sizeof(log_file_expr), ANGBAND_DIR_DEBUG_SAVE, "*-*");

    if ((hFile = _findfirst(log_file_expr, &c_file)) != -1L) {
        do {
            if (((t->tm_yday + 365 - localtime(&c_file.time_write)->tm_yday) % 365) > 7) {
                char c_file_fullpath[1024];
                path_build(c_file_fullpath, sizeof(c_file_fullpath), ANGBAND_DIR_DEBUG_SAVE, c_file.name);
                remove(c_file_fullpath);
            }
        } while (_findnext(hFile, &c_file) == 0);
        _findclose(hFile);
    }
#else
    {
        DIR *saves_dir = opendir(ANGBAND_DIR_DEBUG_SAVE);

        if (saves_dir) {
            struct dirent *next_entry;

            while ((next_entry = readdir(saves_dir))) {
                if (angband_strchr(next_entry->d_name, '-')) {
                    char path[1024];
                    struct stat next_stat;

                    path_build(path, sizeof(path), ANGBAND_DIR_DEBUG_SAVE, next_entry->d_name);
                    /*
                     * Remove if modified more than a week ago,
                     * 7*24*60*60 seconds.
                     */
                    if (stat(path, &next_stat) == 0 &&
                            difftime(now, next_stat.st_mtime) > 604800) {
                        remove(path);
                    }
                }
            }
            closedir(saves_dir);
        }
    }
#endif
}

/*!
 * @brief 画面左下にシステムメッセージを表示する / Take notes on line 23
 * @param str 初期化中のコンテンツ文字列
 */
static void init_note_term(concptr str)
{
    term_erase(0, 23, 255);
    term_putstr(20, 23, -1, TERM_WHITE, str);
    term_fresh();
}

/*!
 * @brief ゲーム画面無しの時の初期化メッセージ出力
 * @param str 初期化中のコンテンツ文字列
 */
static void init_note_no_term(concptr str)
{
    /* Don't show initialization message when there is no game terminal. */
    (void)str;
}

/*!
 * @brief 全ゲームデータ読み込みのサブルーチン / Explain a broken "lib" folder and quit (see below).
 * @param なし
 * @note
 * <pre>
 * This function is "messy" because various things
 * may or may not be initialized, but the "plog()" and "quit()"
 * functions are "supposed" to work under any conditions.
 * </pre>
 */
static void init_angband_aux(concptr why)
{
    plog(why);
    plog(_("'lib'ディレクトリが存在しないか壊れているようです。", "The 'lib' directory is probably missing or broken."));
    plog(_("ひょっとするとアーカイブが正しく解凍されていないのかもしれません。", "The 'lib' directory is probably missing or broken."));
    plog(_("該当する'README'ファイルを読んで確認してみて下さい。", "See the 'README' file for more information."));
    quit(_("致命的なエラー。", "Fatal Error."));
}

/*!
 * @brief タイトル記述
 * @param なし
 */
static void put_title(void)
{
    char title[120];
    put_version(title);

    int col = (80 - strlen(title)) / 2;
    col = col < 0 ? 0 : col;
    const int VER_INFO_ROW = 3; //!< タイトル表記(行)
    prt(title, VER_INFO_ROW, col);
}

/*!
 * @brief 全ゲームデータ読み込みのメインルーチン /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param no_term TRUEならゲーム画面無しの状態で初期化を行う。
 *                コマンドラインからスポイラーの出力のみを行う時の使用を想定する。
 */
void init_angband(player_type *player_ptr, bool no_term)
{
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, _("news_j.txt", "news.txt"));
    int fd = fd_open(buf, O_RDONLY);
    if (fd < 0) {
        char why[sizeof(buf) + 128];
        sprintf(why, _("'%s'ファイルにアクセスできません!", "Cannot access the '%s' file!"), buf);
        init_angband_aux(why);
    }

    (void)fd_close(fd);

    if (!no_term) {
        term_clear();

        path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, _("news_j.txt", "news.txt"));
        FILE *fp;
        fp = angband_fopen(buf, "r");
        if (fp) {
            int i = 0;
            while (0 == angband_fgets(fp, buf, sizeof(buf)))
                term_putstr(0, i++, -1, TERM_WHITE, buf);

            angband_fclose(fp);
        }

        term_flush();
    }

    path_build(buf, sizeof(buf), ANGBAND_DIR_APEX, "scores.raw");
    fd = fd_open(buf, O_RDONLY);
    BIT_FLAGS file_permission = 0664;
    if (fd < 0) {
        safe_setuid_grab(player_ptr);
        fd = fd_make(buf, file_permission);
        safe_setuid_drop();
        if (fd < 0) {
            char why[sizeof(buf) + 128];
            sprintf(why, _("'%s'ファイルを作成できません!", "Cannot create the '%s' file!"), buf);
            init_angband_aux(why);
        }
    }

    (void)fd_close(fd);
    if (!no_term) {
        put_title();
    }

    void (*init_note)(concptr) = (no_term ? init_note_no_term : init_note_term);

    init_note(_("[変数を初期化しています...(その他)", "[Initializing values... (misc)]"));
    if (init_misc(player_ptr))
        quit(_("その他の変数を初期化できません", "Cannot initialize misc. values"));

    init_note(_("[データの初期化中... (地形)]", "[Initializing arrays... (features)]"));
    if (init_f_info())
        quit(_("地形初期化不能", "Cannot initialize features"));

    if (init_feat_variables())
        quit(_("地形初期化不能", "Cannot initialize features"));

    init_note(_("[データの初期化中... (アイテム)]", "[Initializing arrays... (objects)]"));
    if (init_k_info())
        quit(_("アイテム初期化不能", "Cannot initialize objects"));

    init_note(_("[データの初期化中... (伝説のアイテム)]", "[Initializing arrays... (artifacts)]"));
    if (init_a_info())
        quit(_("伝説のアイテム初期化不能", "Cannot initialize artifacts"));

    init_note(_("[データの初期化中... (名のあるアイテム)]", "[Initializing arrays... (ego-items)]"));
    if (init_e_info())
        quit(_("名のあるアイテム初期化不能", "Cannot initialize ego-items"));

    init_note(_("[データの初期化中... (モンスター)]", "[Initializing arrays... (monsters)]"));
    if (init_r_info())
        quit(_("モンスター初期化不能", "Cannot initialize monsters"));

    init_note(_("[データの初期化中... (ダンジョン)]", "[Initializing arrays... (dungeon)]"));
    if (init_d_info())
        quit(_("ダンジョン初期化不能", "Cannot initialize dungeon"));

    for (const auto &d_ref : d_info)
        if (d_ref.idx > 0 && d_ref.final_guardian)
            r_info[d_ref.final_guardian].flags7 |= RF7_GUARDIAN;

    init_note(_("[データの初期化中... (魔法)]", "[Initializing arrays... (magic)]"));
    if (init_m_info())
        quit(_("魔法初期化不能", "Cannot initialize magic"));

    init_note(_("[データの初期化中... (熟練度)]", "[Initializing arrays... (skill)]"));
    if (init_s_info())
        quit(_("熟練度初期化不能", "Cannot initialize skill"));

    init_note(_("[配列を初期化しています... (荒野)]", "[Initializing arrays... (wilderness)]"));
    if (init_wilderness())
        quit(_("荒野を初期化できません", "Cannot initialize wilderness"));

    init_note(_("[配列を初期化しています... (街)]", "[Initializing arrays... (towns)]"));
    if (init_towns())
        quit(_("街を初期化できません", "Cannot initialize towns"));

    init_note(_("[配列を初期化しています... (建物)]", "[Initializing arrays... (buildings)]"));
    if (init_buildings())
        quit(_("建物を初期化できません", "Cannot initialize buildings"));

    init_note(_("[配列を初期化しています... (クエスト)]", "[Initializing arrays... (quests)]"));
    init_quests();
    if (init_v_info())
        quit(_("vault 初期化不能", "Cannot initialize vaults"));

    init_note(_("[データの初期化中... (その他)]", "[Initializing arrays... (other)]"));
    init_other(player_ptr);
    init_note(_("[データの初期化中... (アロケーション)]", "[Initializing arrays... (alloc)]"));
    init_alloc();
    init_note(_("[ユーザー設定ファイルを初期化しています...]", "[Initializing user pref files...]"));
    strcpy(buf, "pref.prf");
    process_pref_file(player_ptr, buf);
    sprintf(buf, "pref-%s.prf", ANGBAND_SYS);
    process_pref_file(player_ptr, buf);
    init_note(_("[初期化終了]", "[Initialization complete]"));
}
