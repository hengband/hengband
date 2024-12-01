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
#include "system/angband-system.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/monster-race-info.h"
#include "system/services/baseitem-monrace-service.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "time.h"
#include "util/angband-files.h"
#include "world/world.h"

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
#ifdef PRIVATE_USER_PATH
    ANGBAND_DIR_USER = std::filesystem::path(PRIVATE_USER_PATH).append(VARIANT_NAME);
#else
    ANGBAND_DIR_USER = std::filesystem::path(libpath).append("user");
#endif
    ANGBAND_DIR_XTRA = std::filesystem::path(libpath).append("xtra");
}

/*!
 * @brief 画面左下にシステムメッセージを表示する / Take notes on line 23
 * @param str 初期化中のコンテンツ文字列
 */
static void init_note_term(concptr str)
{
    term_erase(0, 23);
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
    const auto title = AngbandSystem::get_instance().build_version_expression(VersionExpression::FULL);
    const auto col = (title.length() <= MAIN_TERM_MIN_COLS) ? (MAIN_TERM_MIN_COLS - title.length()) / 2 : 0;
    constexpr auto row_version_info = 3; //!< タイトル表記(行)
    prt(title, row_version_info, col);
}

/*!
 * @brief 全ゲームデータ読み込みのメインルーチン /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param no_term TRUEならゲーム画面無しの状態で初期化を行う。
 *                コマンドラインからスポイラーの出力のみを行う時の使用を想定する。
 */
void init_angband(PlayerType *player_ptr, bool no_term)
{
    const auto path_news = path_build(ANGBAND_DIR_FILE, _("news_j.txt", "news.txt"));
    auto fd = fd_open(path_news, O_RDONLY);
    if (fd < 0) {
        std::string why = _("'", "Cannot access the '");
        why.append(path_news.string());
        why.append(_("'ファイルにアクセスできません!", "' file!"));
        init_angband_aux(why);
    }

    (void)fd_close(fd);
    if (!no_term) {
        term_clear();
        auto *fp = angband_fopen(path_news, FileOpenMode::READ);
        if (fp) {
            int i = 0;
            while (true) {
                const auto buf = angband_fgets(fp);
                if (!buf) {
                    break;
                }
                term_putstr(0, i++, -1, TERM_WHITE, *buf);
            }

            angband_fclose(fp);
        }

        term_flush();
    }

    const auto path_score = path_build(ANGBAND_DIR_APEX, "scores.raw");
    fd = fd_open(path_score, O_RDONLY);
    if (fd < 0) {
        safe_setuid_grab();
        fd = fd_make(path_score, true);
        safe_setuid_drop();
        if (fd < 0) {
            const auto &filename_score = path_score.string();
            std::string why = _("'", "Cannot create the '");
            why.append(filename_score);
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
    try {
        init_terrains_info();
        init_feat_variables();
    } catch (const std::exception &e) {
        quit_fmt("地形初期化不能: %s", e.what());
    }

    init_note(_("[データの初期化中... (アイテム)]", "[Initializing arrays... (objects)]"));
    init_baseitems_info();

    init_note(_("[データの初期化中... (伝説のアイテム)]", "[Initializing arrays... (artifacts)]"));
    init_artifacts_info();

    init_note(_("[データの初期化中... (名のあるアイテム)]", "[Initializing arrays... (ego-items)]"));
    init_egos_info();

    init_note(_("[データの初期化中... (モンスター)]", "[Initializing arrays... (monsters)]"));
    init_monrace_definitions();
    const auto error = BaseitemMonraceService::check_specific_drop_gold_flags_duplication();
    if (error) {
        quit(*error);
    }

    init_note(_("[データの初期化中... (ダンジョン)]", "[Initializing arrays... (dungeon)]"));
    init_dungeons_info();

    init_note(_("[データの初期化中... (呪文情報)]", "[Initializing arrays... (magic)]"));
    init_spell_info();

    init_note(_("[データの初期化中... (魔法)]", "[Initializing arrays... (magic)]"));
    init_class_magics_info();

    init_note(_("[データの初期化中... (熟練度)]", "[Initializing arrays... (skill)]"));
    init_class_skills_info();

    init_note(_("[配列を初期化しています... (荒野)]", "[Initializing arrays... (wilderness)]"));
    init_wilderness();

    init_note(_("[配列を初期化しています... (街)]", "[Initializing arrays... (towns)]"));
    init_towns();

    init_note(_("[配列を初期化しています... (建物)]", "[Initializing arrays... (buildings)]"));
    init_buildings();

    init_note(_("[配列を初期化しています... (クエスト)]", "[Initializing arrays... (quests)]"));
    QuestList::get_instance().initialize();

    init_note(_("[データの初期化中... (宝物庫)]", "[Initializing arrays... (vaults)]"));
    init_vaults_info();

    init_note(_("[データの初期化中... (その他)]", "[Initializing arrays... (other)]"));
    init_other(player_ptr);

    init_note(_("[データの初期化中... (モンスターアロケーション)]", "[Initializing arrays... (monsters alloc)]"));
    init_monsters_alloc();

    init_note(_("[データの初期化中... (アイテムアロケーション)]", "[Initializing arrays... (items alloc)]"));
    init_items_alloc();

    init_note(_("[ユーザー設定ファイルを初期化しています...]", "[Initializing user pref files...]"));
    process_pref_file(player_ptr, "pref.prf");
    process_pref_file(player_ptr, std::string("pref-").append(ANGBAND_SYS).append(".prf"));

    init_note(_("[初期化終了]", "[Initialization complete]"));
}
