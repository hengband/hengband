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
#include "dungeon/quest.h"
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
#include "system/dungeon-info.h"
#include "system/monster-race-info.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "time.h"
#include "util/angband-files.h"
#include "world/world.h"
#ifndef WINDOWS
#include "util/string-processor.h"
#include <dirent.h>
#endif
#ifdef PRIVATE_USER_PATH
#include <string>
#endif

/*!
 * @brief 各データファイルを読み取るためのパスを取得する.
 * @param libpath 各PCのインストール環境における"lib/" を表す絶対パス
 */
void init_file_paths(const std::filesystem::path &libpath)
{
    ANGBAND_DIR = std::filesystem::path(libpath);
    ANGBAND_DIR_APEX = std::filesystem::path(libpath).append("apex");
    ANGBAND_DIR_BONE = std::filesystem::path(libpath).append("bone");
    ANGBAND_DIR_DATA = std::filesystem::path(libpath).append("data");
    ANGBAND_DIR_EDIT = std::filesystem::path(libpath).append("edit");
    ANGBAND_DIR_SCRIPT = std::filesystem::path(libpath).append("script");
    ANGBAND_DIR_FILE = std::filesystem::path(libpath).append("file");
    ANGBAND_DIR_HELP = std::filesystem::path(libpath).append("help");
    ANGBAND_DIR_INFO = std::filesystem::path(libpath).append("info");
    ANGBAND_DIR_PREF = std::filesystem::path(libpath).append("pref");
    ANGBAND_DIR_SAVE = std::filesystem::path(libpath).append("save");
    ANGBAND_DIR_DEBUG_SAVE = std::filesystem::path(ANGBAND_DIR_SAVE).append("log");
#ifdef PRIVATE_USER_PATH
    ANGBAND_DIR_USER = std::filesystem::path(PRIVATE_USER_PATH).append(VARIANT_NAME);
#else
    ANGBAND_DIR_USER = std::filesystem::path(libpath).append("user");
#endif
    ANGBAND_DIR_XTRA = std::filesystem::path(libpath).append("xtra");

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
    const auto &debug_save_str = ANGBAND_DIR_DEBUG_SAVE.string();
    DIR *saves_dir = opendir(debug_save_str.data());
    if (saves_dir == nullptr) {
        return;
    }

    struct dirent *next_entry;
    while ((next_entry = readdir(saves_dir))) {
        if (!angband_strchr(next_entry->d_name, '-')) {
            continue;
        }

        char path[1024];
        struct stat next_stat;
        path_build(path, sizeof(path), ANGBAND_DIR_DEBUG_SAVE, next_entry->d_name);
        constexpr auto one_week = 7 * 24 * 60 * 60;
        if ((stat(path, &next_stat) == 0) && (difftime(now, next_stat.st_mtime) > one_week)) {
            remove(path);
        }
    }

    closedir(saves_dir);
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
static void init_angband_aux(const std::string &why)
{
    plog(why.data());
    plog(_("'lib'ディレクトリが存在しないか壊れているようです。", "The 'lib' directory is probably missing or broken."));
    plog(_("ひょっとするとアーカイブが正しく解凍されていないのかもしれません。", "The 'lib' directory is probably missing or broken."));
    plog(_("該当する'README'ファイルを読んで確認してみて下さい。", "See the 'README' file for more information."));
    quit(_("致命的なエラー。", "Fatal Error."));
}

/*!
 * @brief タイトル記述
 * @param なし
 */
static void put_title()
{
    const auto title = get_version();

    auto col = (title.length() <= MAIN_TERM_MIN_COLS) ? (MAIN_TERM_MIN_COLS - title.length()) / 2 : 0;
    constexpr auto VER_INFO_ROW = 3; //!< タイトル表記(行)
    prt(title, VER_INFO_ROW, col);
}

/*!
 * @brief 全ゲームデータ読み込みのメインルーチン /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param no_term TRUEならゲーム画面無しの状態で初期化を行う。
 *                コマンドラインからスポイラーの出力のみを行う時の使用を想定する。
 */
void init_angband(PlayerType *player_ptr, bool no_term)
{
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, _("news_j.txt", "news.txt"));
    auto fd = fd_open(buf, O_RDONLY);
    if (fd < 0) {
        std::string why = _("'", "Cannot access the '");
        why.append(buf);
        why.append(_("'ファイルにアクセスできません!", "' file!"));
        init_angband_aux(why);
    }

    (void)fd_close(fd);
    if (!no_term) {
        term_clear();
        path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, _("news_j.txt", "news.txt"));
        auto *fp = angband_fopen(buf, FileOpenMode::READ);
        if (fp) {
            int i = 0;
            while (0 == angband_fgets(fp, buf, sizeof(buf))) {
                term_putstr(0, i++, -1, TERM_WHITE, buf);
            }

            angband_fclose(fp);
        }

        term_flush();
    }

    path_build(buf, sizeof(buf), ANGBAND_DIR_APEX, "scores.raw");
    fd = fd_open(buf, O_RDONLY);
    if (fd < 0) {
        safe_setuid_grab(player_ptr);
        fd = fd_make(buf, true);
        safe_setuid_drop();
        if (fd < 0) {
            std::string why = _("'", "Cannot create the '");
            why.append(buf);
            why.append(_("'ファイルを作成できません!", "' file!"));
            init_angband_aux(why);
        }
    }

    (void)fd_close(fd);
    if (!no_term) {
        put_title();
    }

    void (*init_note)(concptr) = (no_term ? init_note_no_term : init_note_term);

    init_note(_("[データの初期化中... (地形)]", "[Initializing arrays... (features)]"));
    if (init_terrains_info()) {
        quit(_("地形初期化不能", "Cannot initialize features"));
    }

    if (init_feat_variables()) {
        quit(_("地形初期化不能", "Cannot initialize features"));
    }

    init_note(_("[データの初期化中... (アイテム)]", "[Initializing arrays... (objects)]"));
    if (init_baseitems_info()) {
        quit(_("アイテム初期化不能", "Cannot initialize objects"));
    }

    init_note(_("[データの初期化中... (伝説のアイテム)]", "[Initializing arrays... (artifacts)]"));
    if (init_artifacts_info()) {
        quit(_("伝説のアイテム初期化不能", "Cannot initialize artifacts"));
    }

    init_note(_("[データの初期化中... (名のあるアイテム)]", "[Initializing arrays... (ego-items)]"));
    if (init_egos_info()) {
        quit(_("名のあるアイテム初期化不能", "Cannot initialize ego-items"));
    }

    init_note(_("[データの初期化中... (モンスター)]", "[Initializing arrays... (monsters)]"));
    if (init_monster_race_definitions()) {
        quit(_("モンスター初期化不能", "Cannot initialize monsters"));
    }

    init_note(_("[データの初期化中... (ダンジョン)]", "[Initializing arrays... (dungeon)]"));
    if (init_dungeons_info()) {
        quit(_("ダンジョン初期化不能", "Cannot initialize dungeon"));
    }

    for (const auto &d_ref : dungeons_info) {
        if (d_ref.idx > 0 && MonsterRace(d_ref.final_guardian).is_valid()) {
            monraces_info[d_ref.final_guardian].flags7 |= RF7_GUARDIAN;
        }
    }

    init_note(_("[データの初期化中... (魔法)]", "[Initializing arrays... (magic)]"));
    if (init_class_magics_info()) {
        quit(_("魔法初期化不能", "Cannot initialize magic"));
    }

    init_note(_("[データの初期化中... (熟練度)]", "[Initializing arrays... (skill)]"));
    if (init_class_skills_info()) {
        quit(_("熟練度初期化不能", "Cannot initialize skill"));
    }

    init_note(_("[配列を初期化しています... (荒野)]", "[Initializing arrays... (wilderness)]"));
    if (!init_wilderness()) {
        quit(_("荒野を初期化できません", "Cannot initialize wilderness"));
    }

    init_note(_("[配列を初期化しています... (街)]", "[Initializing arrays... (towns)]"));
    init_towns();

    init_note(_("[配列を初期化しています... (建物)]", "[Initializing arrays... (buildings)]"));
    init_buildings();

    init_note(_("[配列を初期化しています... (クエスト)]", "[Initializing arrays... (quests)]"));
    QuestList::get_instance().initialize();
    if (init_vaults_info()) {
        quit(_("vault 初期化不能", "Cannot initialize vaults"));
    }

    init_note(_("[データの初期化中... (その他)]", "[Initializing arrays... (other)]"));
    init_other(player_ptr);
    init_note(_("[データの初期化中... (アロケーション)]", "[Initializing arrays... (alloc)]"));
    init_alloc();
    init_note(_("[ユーザー設定ファイルを初期化しています...]", "[Initializing user pref files...]"));
    process_pref_file(player_ptr, "pref.prf");
    process_pref_file(player_ptr, std::string("pref-").append(ANGBAND_SYS).append(".prf").data());
    init_note(_("[初期化終了]", "[Initialization complete]"));
}
