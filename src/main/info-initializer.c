#include "main/info-initializer.h"
#include "dungeon/dungeon.h"
#include "grid/feature.h"
#include "info-reader/artifact-reader.h"
#include "info-reader/dungeon-reader.h"
#include "info-reader/ego-reader.h"
#include "info-reader/feature-reader.h"
#include "info-reader/fixed-map-parser.h"
#include "info-reader/general-parser.h"
#include "info-reader/kind-reader.h"
#include "info-reader/magic-reader.h"
#include "info-reader/race-reader.h"
#include "info-reader/skill-reader.h"
#include "info-reader/vault-reader.h"
#include "io/files-util.h"
#include "io/uid-checker.h"
#include "main/angband-headers.h"
#include "main/init-error-messages-table.h"
#include "monster-race/monster-race.h"
#include "object-enchant/object-ego.h"
#include "object/object-kind.h"
#include "player/player-class.h"
#include "player/player-skill.h"
#include "room/rooms-vault.h"
#include "system/angband-version.h"
#include "system/artifact-type-definition.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <sys/stat.h>
#ifndef WINDOWS
#include <sys/types.h>
#endif

/*!
 * @brief 基本情報読み込みのメインルーチン /
 * Initialize misc. values
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return エラーコード
 */
errr init_misc(player_type *player_ptr) { return parse_fixed_map(player_ptr, "misc.txt", 0, 0, 0, 0); }

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
        concptr oops = (((err > 0) && (err < PARSE_ERROR_MAX)) ? err_str[err] : _("未知の", "unknown"));
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
errr init_f_info(player_type *player_ptr)
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
errr init_k_info(player_type *player_ptr)
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
errr init_a_info(player_type *player_ptr)
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
errr init_e_info(player_type *player_ptr)
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
errr init_r_info(player_type *player_ptr)
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
errr init_d_info(player_type *player_ptr)
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
errr init_s_info(player_type *player_ptr)
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
errr init_m_info(player_type *player_ptr)
{
    init_header(&m_head, MAX_CLASS, sizeof(player_magic));
    m_head.parse_info_txt = parse_m_info;
    return init_info(player_ptr, "m_info", &m_head, (void *)&m_info, NULL, NULL, NULL);
}
