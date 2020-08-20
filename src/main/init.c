/*!
 * @brief ゲームデータ初期化2 / Initialization (part 2) -BEN-
 * @date 2014/01/28
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.\n
 * </pre>
 * @details
 * <pre>
 * This file is used to initialize various variables and arrays for the
 * Angband game.  Note the use of "fd_read()" and "fd_write()" to bypass
 * the common limitation of "read()" and "write()" to only 32767 bytes
 * at a time.
 * Several of the arrays for Angband are built from "template" files in
 * the "lib/file" directory, from which quick-load binary "image" files
 * are constructed whenever they are not present in the "lib/data"
 * directory, or if those files become obsolete, if we are allowed.
 * Warning -- the "ascii" file parsers use a minor hack to collect the
 * name and text information in a single pass.  Thus, the game will not
 * be able to load any template file with more than 20K of names or 60K
 * of text, even though technically, up to 64K should be legal.
 * The "init1.c" file is used only to parse the ascii template files,
 * to create the binary image files.  Note that the binary image files
 * are extremely system dependant.
 * </pre>
 */

#include "main/init.h"
#include "cmd-building/cmd-building.h"
#include "cmd-io/macro-util.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/floor-town.h"
#include "floor/floor-util.h"
#include "floor/wild.h"
#include "game-option/option-flags.h"
#include "game-option/option-types-table.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "info-reader/artifact-reader.h"
#include "info-reader/dungeon-reader.h"
#include "info-reader/ego-reader.h"
#include "info-reader/feature-reader.h"
#include "info-reader/fixed-map-parser.h"
#include "info-reader/general-parser.h"
#include "info-reader/kind-reader.h"
#include "info-reader/magic-reader.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-reader.h"
#include "info-reader/skill-reader.h"
#include "info-reader/vault-reader.h"
#include "io/files-util.h"
#include "io/read-pref-file.h"
#include "io/uid-checker.h"
#include "market/articles-on-sale.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags7.h"
#include "object-enchant/object-ego.h"
#include "object/object-kind.h"
#include "player/player-class.h"
#include "player/player-skill.h"
#include "room/rooms-builder.h"
#include "room/rooms-vault.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/alloc-entries.h"
#include "system/angband-version.h"
#include "system/artifact-type-definition.h"
#include "system/building-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/quarks.h"
#include "util/tag-sorter.h"
#include "view/display-messages.h"
#include "world/world.h"

#include <sys/stat.h>
#include <sys/types.h>

/*!
 * @brief マクロ登録の最大数 / Maximum number of macros (see "io.c")
 * @note Default: assume at most 256 macros are used
 */
static const int MACRO_MAX = 256;

/*!
 * @brief 各データファイルを読み取るためのパスを取得する
 * Find the default paths to all of our important sub-directories.
 * @param path パス保管先の文字列
 * @return なし
 * @details
 * <pre>
 * The purpose of each sub-directory is described in "variable.c".
 * All of the sub-directories should, by default, be located inside
 * the main "lib" directory, whose location is very system dependant.
 * This function takes a writable buffer, initially containing the
 * "path" to the "lib" directory, for example, "/pkg/lib/angband/",
 * or a system dependant string, for example, ":lib:".  The buffer
 * must be large enough to contain at least 32 more characters.
 * Various command line options may allow some of the important
 * directories to be changed to user-specified directories, most
 * importantly, the "info" and "user" and "save" directories,
 * but this is done after this function, see "main.c".
 * In general, the initial path should end in the appropriate "PATH_SEP"
 * string.  All of the "sub-directory" paths (created below or supplied
 * by the user) will NOT end in the "PATH_SEP" string, see the special
 * "path_build()" function in "util.c" for more information.
 * Mega-Hack -- support fat raw files under NEXTSTEP, using special
 * "suffixed" directories for the "ANGBAND_DIR_DATA" directory, but
 * requiring the directories to be created by hand by the user.
 * Hack -- first we free all the strings, since this is known
 * to succeed even if the strings have not been allocated yet,
 * as long as the variables start out as "NULL".  This allows
 * this function to be called multiple times, for example, to
 * try several base "path" values until a good one is found.
 * </pre>
 */
void init_file_paths(char *path)
{
    char *tail;
#ifdef PRIVATE_USER_PATH
    char buf[1024];
#endif
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
    string_free(ANGBAND_DIR_USER);
    string_free(ANGBAND_DIR_XTRA);

    ANGBAND_DIR = string_make(path);
    tail = path + strlen(path);
    strcpy(tail, "apex");
    ANGBAND_DIR_APEX = string_make(path);
    strcpy(tail, "bone");
    ANGBAND_DIR_BONE = string_make(path);
    strcpy(tail, "data");
    ANGBAND_DIR_DATA = string_make(path);
    strcpy(tail, "edit");
    ANGBAND_DIR_EDIT = string_make(path);
    strcpy(tail, "script");
    ANGBAND_DIR_SCRIPT = string_make(path);
    strcpy(tail, "file");
    ANGBAND_DIR_FILE = string_make(path);
    strcpy(tail, "help");
    ANGBAND_DIR_HELP = string_make(path);
    strcpy(tail, "info");
    ANGBAND_DIR_INFO = string_make(path);
    strcpy(tail, "pref");
    ANGBAND_DIR_PREF = string_make(path);
    strcpy(tail, "save");
    ANGBAND_DIR_SAVE = string_make(path);
#ifdef PRIVATE_USER_PATH
    path_build(buf, sizeof(buf), PRIVATE_USER_PATH, VERSION_NAME);
    ANGBAND_DIR_USER = string_make(buf);
#else
    strcpy(tail, "user");
    ANGBAND_DIR_USER = string_make(path);
#endif
    strcpy(tail, "xtra");
    ANGBAND_DIR_XTRA = string_make(path);
}

/*
 * Hack -- help give useful error messages
 */
int error_idx; /*!< データ読み込み/初期化時に汎用的にエラーコードを保存するグローバル変数 */
int error_line; /*!< データ読み込み/初期化時に汎用的にエラー行数を保存するグローバル変数 */

/*!
 * エラーメッセージの名称定義 / Standard error message text
 */
concptr err_str[PARSE_ERROR_MAX] = {
    NULL,
    _("文法エラー", "parse error"),
    _("古いファイル", "obsolete file"),
    _("記録ヘッダがない", "missing record header"),
    _("不連続レコード", "non-sequential records"),
    _("おかしなフラグ存在", "invalid flag specification"),
    _("未定義命令", "undefined directive"),
    _("メモリ不足", "out of memory"),
    _("座標範囲外", "coordinates out of bounds"),
    _("引数不足", "too few arguments"),
    _("未定義地形タグ", "undefined terrain tag"),
};

/*
 * File headers
 */
static angband_header v_head; /*!< Vault情報のヘッダ構造体 */
static angband_header k_head; /*!< ペースアイテム情報のヘッダ構造体 */
static angband_header a_head; /*!< 固定アーティファクト情報のヘッダ構造体 */
static angband_header e_head; /*!< アイテムエゴ情報のヘッダ構造体 */
static angband_header r_head; /*!< モンスター種族情報のヘッダ構造体 */
static angband_header d_head; /*!< ダンジョン情報のヘッダ構造体 */
static angband_header s_head; /*!< プレイヤー職業技能情報のヘッダ構造体 */
static angband_header m_head; /*!< プレイヤー職業魔法情報のヘッダ構造体 */

/*!
 * @brief テキストファイルとrawファイルの更新時刻を比較する
 * Find the default paths to all of our important sub-directories.
 * @param fd ファイルディスクリプタ
 * @param template_file ファイル名
 * @return テキストの方が新しいか、rawファイルがなく更新の必要がある場合-1、更新の必要がない場合0。
 */
static errr check_modification_date(int fd, concptr template_file)
{
    struct stat txt_stat, raw_stat;
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_EDIT, template_file);
    if (stat(buf, &txt_stat))
        return 0;

    if (fstat(fd, &raw_stat))
        return -1;

    if (txt_stat.st_mtime > raw_stat.st_mtime)
        return -1;

    return 0;
}

/*** Initialize from binary image files ***/

/*!
 * @brief rawファイルからのデータの読み取り処理
 * Initialize the "*_info" array, by parsing a binary "image" file
 * @param fd ファイルディスクリプタ
 * @param head rawファイルのヘッダ
 * @return エラーコード
 */
static errr init_info_raw(int fd, angband_header *head)
{
    angband_header test;
    if (fd_read(fd, (char *)(&test), sizeof(angband_header)) || (test.v_major != head->v_major) || (test.v_minor != head->v_minor)
        || (test.v_patch != head->v_patch) || (test.info_num != head->info_num) || (test.info_len != head->info_len) || (test.head_size != head->head_size)
        || (test.info_size != head->info_size)) {
        return -1;
    }

    *head = test;
    C_MAKE(head->info_ptr, head->info_size, char);
    fd_read(fd, head->info_ptr, head->info_size);
    if (head->name_size) {
        C_MAKE(head->name_ptr, head->name_size, char);
        fd_read(fd, head->name_ptr, head->name_size);
    }

    if (head->text_size) {
        C_MAKE(head->text_ptr, head->text_size, char);
        fd_read(fd, head->text_ptr, head->text_size);
    }

    if (head->tag_size) {
        C_MAKE(head->tag_ptr, head->tag_size, char);
        fd_read(fd, head->tag_ptr, head->tag_size);
    }

    return 0;
}

/*!
 * @brief ヘッダ構造体の更新
 * Initialize the header of an *_info.raw file.
 * @param head rawファイルのヘッダ
 * @param num データ数
 * @param len データの長さ
 * @return エラーコード
 */
static void init_header(angband_header *head, IDX num, int len)
{
    head->v_major = FAKE_VER_MAJOR;
    head->v_minor = FAKE_VER_MINOR;
    head->v_patch = FAKE_VER_PATCH;
    head->v_extra = 0;

    head->info_num = (IDX)num;
    head->info_len = len;

    head->head_size = sizeof(angband_header);
    head->info_size = head->info_num * head->info_len;
}

static void update_header(angband_header *head, void **info, char **name, char **text, char **tag)
{
    if (info)
        *info = head->info_ptr;

    if (name)
        *name = head->name_ptr;

    if (text)
        *text = head->text_ptr;

    if (tag)
        *tag = head->tag_ptr;
}

/*!
 * @brief ヘッダ構造体の更新
 * Initialize the "*_info" array
 * @param filename ファイル名(拡張子txt/raw)
 * @param head 処理に用いるヘッダ構造体
 * @param info データ保管先の構造体ポインタ
 * @param name 名称用可変文字列の保管先
 * @param text テキスト用可変文字列の保管先
 * @param tag タグ用可変文字列の保管先
 * @return エラーコード
 * @note
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
static errr init_info(player_type *player_ptr, concptr filename, angband_header *head, void **info, char **name, char **text, char **tag)
{
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_DATA, format(_("%s_j.raw", "%s.raw"), filename));
    int fd = fd_open(buf, O_RDONLY);
    errr err = 1;
    if (fd >= 0) {
        err = check_modification_date(fd, format("%s.txt", filename));
        if (!err)
            err = init_info_raw(fd, head);

        (void)fd_close(fd);
    }

    BIT_FLAGS file_permission = 0644;
    if (err == 0) {
        update_header(head, info, name, text, tag);
        return 0;
    }

    C_MAKE(head->info_ptr, head->info_size, char);
    if (name)
        C_MAKE(head->name_ptr, FAKE_NAME_SIZE, char);

    if (text)
        C_MAKE(head->text_ptr, FAKE_TEXT_SIZE, char);

    if (tag)
        C_MAKE(head->tag_ptr, FAKE_TAG_SIZE, char);

    if (info)
        *info = head->info_ptr;

    if (name)
        *name = head->name_ptr;

    if (text)
        *text = head->text_ptr;

    if (tag)
        *tag = head->tag_ptr;

    path_build(buf, sizeof(buf), ANGBAND_DIR_EDIT, format("%s.txt", filename));
    FILE *fp;
    fp = angband_fopen(buf, "r");
    if (!fp)
        quit(format(_("'%s.txt'ファイルをオープンできません。", "Cannot open '%s.txt' file."), filename));

    err = init_info_txt(fp, buf, head, head->parse_info_txt);
    angband_fclose(fp);
    if (err) {
        concptr oops;
        oops = (((err > 0) && (err < PARSE_ERROR_MAX)) ? err_str[err] : _("未知の", "unknown"));
#ifdef JP
        msg_format("'%s.txt'ファイルの %d 行目にエラー。", filename, error_line);
#else
        msg_format("Error %d at line %d of '%s.txt'.", err, error_line, filename);
#endif
        msg_format(_("レコード %d は '%s' エラーがあります。", "Record %d contains a '%s' error."), error_idx, oops);
        msg_format(_("構文 '%s'。", "Parsing '%s'."), buf);
        msg_print(NULL);
        quit(format(_("'%s.txt'ファイルにエラー", "Error in '%s.txt' file."), filename));
    }

    if (head->retouch)
        (*head->retouch)(head);

    path_build(buf, sizeof(buf), ANGBAND_DIR_DATA, format(_("%s_j.raw", "%s.raw"), filename));
    safe_setuid_grab(player_ptr);
    (void)fd_kill(buf);
    fd = fd_make(buf, file_permission);
    safe_setuid_drop();
    if (fd >= 0) {
        fd_write(fd, (concptr)(head), head->head_size);
        fd_write(fd, head->info_ptr, head->info_size);
        fd_write(fd, head->name_ptr, head->name_size);
        fd_write(fd, head->text_ptr, head->text_size);
        fd_write(fd, head->tag_ptr, head->tag_size);
        (void)fd_close(fd);
    }

    C_KILL(head->info_ptr, head->info_size, char);
    if (name)
        C_KILL(head->name_ptr, FAKE_NAME_SIZE, char);

    if (text)
        C_KILL(head->text_ptr, FAKE_TEXT_SIZE, char);

    if (tag)
        C_KILL(head->tag_ptr, FAKE_TAG_SIZE, char);

    path_build(buf, sizeof(buf), ANGBAND_DIR_DATA, format(_("%s_j.raw", "%s.raw"), filename));
    fd = fd_open(buf, O_RDONLY);
    if (fd < 0)
        quit(format(_("'%s_j.raw'ファイルをロードできません。", "Cannot load '%s.raw' file."), filename));

    err = init_info_raw(fd, head);
    (void)fd_close(fd);
    if (err)
        quit(format(_("'%s_j.raw'ファイルを解析できません。", "Cannot parse '%s.raw' file."), filename));

    update_header(head, info, name, text, tag);
    return 0;
}

/*!
 * @brief 地形情報読み込みのメインルーチン /
 * Initialize the "f_info" array
 * @return エラーコード
 */
static errr init_f_info(player_type *player_ptr)
{
    init_header(&f_head, max_f_idx, sizeof(feature_type));
    f_head.parse_info_txt = parse_f_info;
    f_head.retouch = retouch_f_info;
    return init_info(player_ptr, "f_info", &f_head, (void *)&f_info, &f_name, NULL, &f_tag);
}

/*!
 * @brief ベースアイテム情報読み込みのメインルーチン /
 * Initialize the "k_info" array
 * @return エラーコード
 */
static errr init_k_info(player_type *player_ptr)
{
    init_header(&k_head, max_k_idx, sizeof(object_kind));
    k_head.parse_info_txt = parse_k_info;
    return init_info(player_ptr, "k_info", &k_head, (void *)&k_info, &k_name, &k_text, NULL);
}

/*!
 * @brief 固定アーティファクト情報読み込みのメインルーチン /
 * Initialize the "a_info" array
 * @return エラーコード
 */
static errr init_a_info(player_type *player_ptr)
{
    init_header(&a_head, max_a_idx, sizeof(artifact_type));
    a_head.parse_info_txt = parse_a_info;
    return init_info(player_ptr, "a_info", &a_head, (void *)&a_info, &a_name, &a_text, NULL);
}

/*!
 * @brief 固定アーティファクト情報読み込みのメインルーチン /
 * Initialize the "e_info" array
 * @return エラーコード
 */
static errr init_e_info(player_type *player_ptr)
{
    init_header(&e_head, max_e_idx, sizeof(ego_item_type));
    e_head.parse_info_txt = parse_e_info;
    return init_info(player_ptr, "e_info", &e_head, (void *)&e_info, &e_name, &e_text, NULL);
}

/*!
 * @brief モンスター種族情報読み込みのメインルーチン /
 * Initialize the "r_info" array
 * @return エラーコード
 */
static errr init_r_info(player_type *player_ptr)
{
    init_header(&r_head, max_r_idx, sizeof(monster_race));
    r_head.parse_info_txt = parse_r_info;
    return init_info(player_ptr, "r_info", &r_head, (void *)&r_info, &r_name, &r_text, NULL);
}

/*!
 * @brief ダンジョン情報読み込みのメインルーチン /
 * Initialize the "d_info" array
 * @return エラーコード
 */
static errr init_d_info(player_type *player_ptr)
{
    init_header(&d_head, current_world_ptr->max_d_idx, sizeof(dungeon_type));
    d_head.parse_info_txt = parse_d_info;
    return init_info(player_ptr, "d_info", &d_head, (void *)&d_info, &d_name, &d_text, NULL);
}

/*!
 * @brief Vault情報読み込みのメインルーチン /
 * Initialize the "v_info" array
 * @return エラーコード
 * @note
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
errr init_v_info(player_type *player_ptr)
{
    init_header(&v_head, max_v_idx, sizeof(vault_type));
    v_head.parse_info_txt = parse_v_info;
    return init_info(player_ptr, "v_info", &v_head, (void *)&v_info, &v_name, &v_text, NULL);
}

/*!
 * @brief 職業技能情報読み込みのメインルーチン /
 * Initialize the "s_info" array
 * @return エラーコード
 */
static errr init_s_info(player_type *player_ptr)
{
    init_header(&s_head, MAX_CLASS, sizeof(skill_table));
    s_head.parse_info_txt = parse_s_info;
    return init_info(player_ptr, "s_info", &s_head, (void *)&s_info, NULL, NULL, NULL);
}

/*!
 * @brief 職業魔法情報読み込みのメインルーチン /
 * Initialize the "m_info" array
 * @return エラーコード
 */
static errr init_m_info(player_type *player_ptr)
{
    init_header(&m_head, MAX_CLASS, sizeof(player_magic));
    m_head.parse_info_txt = parse_m_info;
    return init_info(player_ptr, "m_info", &m_head, (void *)&m_info, NULL, NULL, NULL);
}

/*!
 * @brief 基本情報読み込みのメインルーチン /
 * Initialize misc. values
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return エラーコード
 */
static errr init_misc(player_type *player_ptr) { return parse_fixed_map(player_ptr, "misc.txt", 0, 0, 0, 0); }

/*!
 * @brief 町情報読み込みのメインルーチン /
 * Initialize town array
 * @return エラーコード
 */
static errr init_towns(void)
{
    C_MAKE(town_info, max_towns, town_type);
    for (int i = 1; i < max_towns; i++) {
        C_MAKE(town_info[i].store, MAX_STORES, store_type);
        for (int j = 0; j < MAX_STORES; j++) {
            store_type *store_ptr = &town_info[i].store[j];
            if ((i > 1) && (j == STORE_MUSEUM || j == STORE_HOME))
                continue;

            /*
             * 我が家が 20 ページまで使える隠し機能のための準備。
             * オプションが有効でもそうでなくても一応スペースを作っておく。
             */
            if (j == STORE_HOME) {
                store_ptr->stock_size = (STORE_INVEN_MAX * 10);
            } else if (j == STORE_MUSEUM) {
                store_ptr->stock_size = (STORE_INVEN_MAX * 50);
            } else {
                store_ptr->stock_size = STORE_INVEN_MAX;
            }

            C_MAKE(store_ptr->stock, store_ptr->stock_size, object_type);
            if ((j == STORE_BLACK) || (j == STORE_HOME) || (j == STORE_MUSEUM))
                continue;

            store_ptr->table_size = STORE_CHOICES;
            C_MAKE(store_ptr->table, store_ptr->table_size, s16b);
            for (int k = 0; k < STORE_CHOICES; k++) {
                KIND_OBJECT_IDX k_idx;
                int tv = store_table[j][k][0];
                int sv = store_table[j][k][1];
                for (k_idx = 1; k_idx < max_k_idx; k_idx++) {
                    object_kind *k_ptr = &k_info[k_idx];
                    if ((k_ptr->tval == tv) && (k_ptr->sval == sv))
                        break;
                }

                if (k_idx == max_k_idx)
                    continue;

                store_ptr->table[store_ptr->table_num++] = k_idx;
            }
        }
    }

    return 0;
}

/*!
 * @brief 店情報初期化のメインルーチン /
 * Initialize buildings
 * @return エラーコード
 */
errr init_buildings(void)
{
    for (int i = 0; i < MAX_BLDG; i++) {
        building[i].name[0] = '\0';
        building[i].owner_name[0] = '\0';
        building[i].owner_race[0] = '\0';

        for (int j = 0; j < 8; j++) {
            building[i].act_names[j][0] = '\0';
            building[i].member_costs[j] = 0;
            building[i].other_costs[j] = 0;
            building[i].letters[j] = 0;
            building[i].actions[j] = 0;
            building[i].action_restr[j] = 0;
        }

        for (int j = 0; j < MAX_CLASS; j++)
            building[i].member_class[j] = 0;

        for (int j = 0; j < MAX_RACES; j++)
            building[i].member_race[j] = 0;

        for (int j = 0; j < MAX_MAGIC + 1; j++)
            building[i].member_realm[j] = 0;
    }

    return 0;
}

/*!
 * @brief クエスト情報初期化のメインルーチン /
 * Initialize quest array
 * @return エラーコード
 */
static errr init_quests(void)
{
    C_MAKE(quest, max_q_idx, quest_type);
    for (int i = 0; i < max_q_idx; i++)
        quest[i].status = QUEST_STATUS_UNTAKEN;

    return 0;
}

/*!
 * @brief その他の初期情報更新 /
 * Initialize some other arrays
 * @return エラーコード
 */
static errr init_other(player_type *player_ptr)
{
    player_ptr->current_floor_ptr = &floor_info; // TODO:本当はこんなところで初期化したくない
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    C_MAKE(floor_ptr->o_list, current_world_ptr->max_o_idx, object_type);
    C_MAKE(floor_ptr->m_list, current_world_ptr->max_m_idx, monster_type);
    for (int i = 0; i < MAX_MTIMED; i++)
        C_MAKE(floor_ptr->mproc_list[i], current_world_ptr->max_m_idx, s16b);

    C_MAKE(max_dlv, current_world_ptr->max_d_idx, DEPTH);
    for (int i = 0; i < MAX_HGT; i++)
        C_MAKE(floor_ptr->grid_array[i], MAX_WID, grid_type);

    C_MAKE(macro__pat, MACRO_MAX, concptr);
    C_MAKE(macro__act, MACRO_MAX, concptr);
    C_MAKE(macro__cmd, MACRO_MAX, bool);
    C_MAKE(macro__buf, 1024, char);
    quark_init();

    C_MAKE(message__ptr, MESSAGE_MAX, u32b);
    C_MAKE(message__buf, MESSAGE_BUF, char);
    message__tail = MESSAGE_BUF;

    for (int i = 0; option_info[i].o_desc; i++) {
        int os = option_info[i].o_set;
        int ob = option_info[i].o_bit;
        if (!option_info[i].o_var)
            continue;

        option_mask[os] |= (1L << ob);
        if (option_info[i].o_norm)
            option_flag[os] |= (1L << ob);
        else
            option_flag[os] &= ~(1L << ob);
    }

    for (int n = 0; n < 8; n++)
        for (int i = 0; i < 32; i++)
            if (window_flag_desc[i])
                window_mask[n] |= (1L << i);

    /*
     *  Set the "default" window flags
     *  Window 1 : Display messages
     *  Window 2 : Display inven/equip
     */
    window_flag[1] = 1L << A_MAX;
    window_flag[2] = 1L << 0;
    (void)format("%s (%s).", "Mr.Hoge", MAINTAINER);
    return 0;
}

/*!
 * @brief オブジェクト配列を初期化する /
 * Initialize some other arrays
 * @return エラーコード
 */
static errr init_object_alloc(void)
{
    s16b aux[MAX_DEPTH];
    (void)C_WIPE(&aux, MAX_DEPTH, s16b);

    s16b num[MAX_DEPTH];
    (void)C_WIPE(&num, MAX_DEPTH, s16b);

    if (alloc_kind_table)
        C_KILL(alloc_kind_table, alloc_kind_size, alloc_entry);

    alloc_kind_size = 0;
    for (int i = 1; i < max_k_idx; i++) {
        object_kind *k_ptr;
        k_ptr = &k_info[i];
        for (int j = 0; j < 4; j++) {
            if (k_ptr->chance[j]) {
                alloc_kind_size++;
                num[k_ptr->locale[j]]++;
            }
        }
    }

    for (int i = 1; i < MAX_DEPTH; i++)
        num[i] += num[i - 1];

    if (!num[0])
        quit(_("町のアイテムがない！", "No town objects!"));

    C_MAKE(alloc_kind_table, alloc_kind_size, alloc_entry);
    alloc_entry *table;
    table = alloc_kind_table;
    for (int i = 1; i < max_k_idx; i++) {
        object_kind *k_ptr;
        k_ptr = &k_info[i];
        for (int j = 0; j < 4; j++) {
            if (k_ptr->chance[j] == 0)
                continue;

            int x = k_ptr->locale[j];
            int p = (100 / k_ptr->chance[j]);
            int y = (x > 0) ? num[x - 1] : 0;
            int z = y + aux[x];
            table[z].index = (KIND_OBJECT_IDX)i;
            table[z].level = (DEPTH)x;
            table[z].prob1 = (PROB)p;
            table[z].prob2 = (PROB)p;
            table[z].prob3 = (PROB)p;
            aux[x]++;
        }
    }

    return 0;
}

/*!
 * @brief モンスター配列と生成テーブルを初期化する /
 * Initialize some other arrays
 * @return エラーコード
 */
static errr init_alloc(void)
{
    monster_race *r_ptr;
    tag_type *elements;
    C_MAKE(elements, max_r_idx, tag_type);
    for (int i = 1; i < max_r_idx; i++) {
        elements[i].tag = r_info[i].level;
        elements[i].index = i;
    }

    tag_sort(elements, max_r_idx);
    alloc_race_size = max_r_idx;
    C_MAKE(alloc_race_table, alloc_race_size, alloc_entry);
    for (int i = 1; i < max_r_idx; i++) {
        r_ptr = &r_info[elements[i].index];
        if (r_ptr->rarity == 0)
            continue;

        int x = r_ptr->level;
        int p = (100 / r_ptr->rarity);
        alloc_race_table[i].index = (KIND_OBJECT_IDX)elements[i].index;
        alloc_race_table[i].level = (DEPTH)x;
        alloc_race_table[i].prob1 = (PROB)p;
        alloc_race_table[i].prob2 = (PROB)p;
        alloc_race_table[i].prob3 = (PROB)p;
    }

    C_KILL(elements, max_r_idx, tag_type);
    (void)init_object_alloc();
    return 0;
}

/*!
 * @brief 画面左下にシステムメッセージを表示する /
 * Hack -- take notes on line 23
 * @return なし
 */
static void init_note(concptr str)
{
    term_erase(0, 23, 255);
    term_putstr(20, 23, -1, TERM_WHITE, str);
    term_fresh();
}

/*!
 * @brief 全ゲームデータ読み込みのサブルーチン /
 * Hack -- Explain a broken "lib" folder and quit (see below).
 * @return なし
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
 * @return なし
 */
static void put_title(void)
{
    char title[120];
#if H_VER_EXTRA > 0
    sprintf(title, _("変愚蛮怒 %d.%d.%d.%d(%s)", "Hengband %d.%d.%d.%d(%s)"), H_VER_MAJOR, H_VER_MINOR, H_VER_PATCH, H_VER_EXTRA,
#else
    sprintf(title, _("変愚蛮怒 %d.%d.%d(%s)", "Hengband %d.%d.%d(%s)"), H_VER_MAJOR, H_VER_MINOR, H_VER_PATCH,
#endif
        IS_STABLE_VERSION ? _("安定版", "Stable") : _("開発版", "Developing"));
    int col = (80 - strlen(title)) / 2;
    col = col < 0 ? 0 : col;
    const int VER_INFO_ROW = 3; //!< タイトル表記(行)
    prt(title, VER_INFO_ROW, col);
}

/*!
 * @brief 全ゲームデータ読み込みのメインルーチン /
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param process_autopick_file_command 自動拾いファイル読み込み関数への関数ポインタ
 * @return なし
 */
void init_angband(player_type *player_ptr, process_autopick_file_command_pf process_autopick_file_command)
{
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, _("news_j.txt", "news.txt"));
    int fd = fd_open(buf, O_RDONLY);
    if (fd < 0) {
        char why[1024];
        sprintf(why, _("'%s'ファイルにアクセスできません!", "Cannot access the '%s' file!"), buf);
        init_angband_aux(why);
    }

    (void)fd_close(fd);
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
    path_build(buf, sizeof(buf), ANGBAND_DIR_APEX, "scores.raw");
    fd = fd_open(buf, O_RDONLY);
    BIT_FLAGS file_permission = 0664;
    if (fd < 0) {
        safe_setuid_grab(player_ptr);
        fd = fd_make(buf, file_permission);
        safe_setuid_drop();
        if (fd < 0) {
            char why[1024];
            sprintf(why, _("'%s'ファイルを作成できません!", "Cannot create the '%s' file!"), buf);
            init_angband_aux(why);
        }
    }

    (void)fd_close(fd);
    put_title();

    init_note(_("[変数を初期化しています...(その他)", "[Initializing values... (misc)]"));
    if (init_misc(player_ptr))
        quit(_("その他の変数を初期化できません", "Cannot initialize misc. values"));

    init_note(_("[データの初期化中... (地形)]", "[Initializing arrays... (features)]"));
    if (init_f_info(player_ptr))
        quit(_("地形初期化不能", "Cannot initialize features"));

    if (init_feat_variables())
        quit(_("地形初期化不能", "Cannot initialize features"));

    init_note(_("[データの初期化中... (アイテム)]", "[Initializing arrays... (objects)]"));
    if (init_k_info(player_ptr))
        quit(_("アイテム初期化不能", "Cannot initialize objects"));

    init_note(_("[データの初期化中... (伝説のアイテム)]", "[Initializing arrays... (artifacts)]"));
    if (init_a_info(player_ptr))
        quit(_("伝説のアイテム初期化不能", "Cannot initialize artifacts"));

    init_note(_("[データの初期化中... (名のあるアイテム)]", "[Initializing arrays... (ego-items)]"));
    if (init_e_info(player_ptr))
        quit(_("名のあるアイテム初期化不能", "Cannot initialize ego-items"));

    init_note(_("[データの初期化中... (モンスター)]", "[Initializing arrays... (monsters)]"));
    if (init_r_info(player_ptr))
        quit(_("モンスター初期化不能", "Cannot initialize monsters"));

    init_note(_("[データの初期化中... (ダンジョン)]", "[Initializing arrays... (dungeon)]"));
    if (init_d_info(player_ptr))
        quit(_("ダンジョン初期化不能", "Cannot initialize dungeon"));

    for (int i = 1; i < current_world_ptr->max_d_idx; i++)
        if (d_info[i].final_guardian)
            r_info[d_info[i].final_guardian].flags7 |= RF7_GUARDIAN;

    init_note(_("[データの初期化中... (魔法)]", "[Initializing arrays... (magic)]"));
    if (init_m_info(player_ptr))
        quit(_("魔法初期化不能", "Cannot initialize magic"));

    init_note(_("[データの初期化中... (熟練度)]", "[Initializing arrays... (skill)]"));
    if (init_s_info(player_ptr))
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
    if (init_quests())
        quit(_("クエストを初期化できません", "Cannot initialize quests"));

    if (init_v_info(player_ptr))
        quit(_("vault 初期化不能", "Cannot initialize vaults"));

    init_note(_("[データの初期化中... (その他)]", "[Initializing arrays... (other)]"));
    if (init_other(player_ptr))
        quit(_("その他のデータ初期化不能", "Cannot initialize other stuff"));

    init_note(_("[データの初期化中... (アロケーション)]", "[Initializing arrays... (alloc)]"));
    if (init_alloc())
        quit(_("アロケーション・スタッフ初期化不能", "Cannot initialize alloc stuff"));

    init_note(_("[ユーザー設定ファイルを初期化しています...]", "[Initializing user pref files...]"));
    strcpy(buf, "pref.prf");
    process_pref_file(player_ptr, buf, process_autopick_file_command);
    sprintf(buf, "pref-%s.prf", ANGBAND_SYS);
    process_pref_file(player_ptr, buf, process_autopick_file_command);
    init_note(_("[初期化終了]", "[Initialization complete]"));
}

/*!
 * @brief チェックサム情報を出力 / Get check sum in string form
 * @return チェックサム情報の文字列
 */
concptr get_check_sum(void)
{
    return format("%02x%02x%02x%02x%02x%02x%02x%02x%02x", f_head.v_extra, k_head.v_extra, a_head.v_extra, e_head.v_extra, r_head.v_extra, d_head.v_extra,
        m_head.v_extra, s_head.v_extra, v_head.v_extra);
}
