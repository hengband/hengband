/*!
 * @file info-initializer.cpp
 * @brief 変愚蛮怒のゲームデータ解析処理定義
 */

#include "main/info-initializer.h"
#include "dungeon/dungeon.h"
#include "grid/feature.h"
#include "info-reader/artifact-reader.h"
#include "info-reader/dungeon-reader.h"
#include "info-reader/ego-reader.h"
#include "info-reader/feature-reader.h"
#include "info-reader/fixed-map-parser.h"
#include "info-reader/general-parser.h"
#include "info-reader/info-reader-util.h"
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
#include "player-info/class-info.h"
#include "player/player-skill.h"
#include "room/rooms-vault.h"
#include "system/angband-version.h"
#include "system/artifact-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <sys/stat.h>
#ifndef WINDOWS
#include <sys/types.h>
#endif
#include <string_view>

/*!
 * @brief 基本情報読み込みのメインルーチン /
 * Initialize misc. values
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return エラーコード
 */
errr init_misc(player_type *player_ptr)
{
    return parse_fixed_map(player_ptr, "misc.txt", 0, 0, 0, 0);
}

/*!
 * @brief ヘッダ構造体の更新
 * Initialize the header of an *_info.raw file.
 * @param head rawファイルのヘッダ
 * @param num データ数
 * @param len データの長さ
 * @return エラーコード
 */
static void init_header(angband_header *head, IDX num = 0)
{
    head->checksum = 0;
    head->info_num = (IDX)num;
}

/*!
 * @brief 各種設定データをlib/edit/のテキストから読み込み
 * Initialize the "*_info" array
 * @param filename ファイル名(拡張子txt)
 * @param head 処理に用いるヘッダ構造体
 * @param info データ保管先の構造体ポインタ
 * @return エラーコード
 * @note
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
template <typename InfoType>
static errr init_info(concptr filename, angband_header &head, std::vector<InfoType> &info, std::function<errr(std::string_view, angband_header *)> parser,
    void (*retouch)(angband_header *head))
{
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_EDIT, format("%s.txt", filename));

    FILE *fp = angband_fopen(buf, "r");

    if (!fp)
        quit(format(_("'%s.txt'ファイルをオープンできません。", "Cannot open '%s.txt' file."), filename));

    info = std::vector<InfoType>(head.info_num);

    errr err = init_info_txt(fp, buf, &head, parser);
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
        msg_print(nullptr);
        quit(format(_("'%s.txt'ファイルにエラー", "Error in '%s.txt' file."), filename));
    }

    info.shrink_to_fit();
    head.info_num = static_cast<uint16_t>(info.size());

    if (retouch)
        (*retouch)(&head);

    return 0;
}

/*!
 * @brief 地形情報読み込みのメインルーチン /
 * Initialize the "f_info" array
 * @return エラーコード
 */
errr init_f_info()
{
    init_header(&f_head);
    return init_info("f_info", f_head, f_info, parse_f_info, retouch_f_info);
}

/*!
 * @brief ベースアイテム情報読み込みのメインルーチン /
 * Initialize the "k_info" array
 * @return エラーコード
 */
errr init_k_info()
{
    init_header(&k_head);
    return init_info("k_info", k_head, k_info, parse_k_info, nullptr);
}

/*!
 * @brief 固定アーティファクト情報読み込みのメインルーチン /
 * Initialize the "a_info" array
 * @return エラーコード
 */
errr init_a_info()
{
    init_header(&a_head);
    return init_info("a_info", a_head, a_info, parse_a_info, nullptr);
}

/*!
 * @brief 固定アーティファクト情報読み込みのメインルーチン /
 * Initialize the "e_info" array
 * @return エラーコード
 */
errr init_e_info()
{
    init_header(&e_head);
    return init_info("e_info", e_head, e_info, parse_e_info, nullptr);
}

/*!
 * @brief モンスター種族情報読み込みのメインルーチン /
 * Initialize the "r_info" array
 * @return エラーコード
 */
errr init_r_info()
{
    init_header(&r_head);
    return init_info("r_info", r_head, r_info, parse_r_info, nullptr);
}

/*!
 * @brief ダンジョン情報読み込みのメインルーチン /
 * Initialize the "d_info" array
 * @return エラーコード
 */
errr init_d_info()
{
    init_header(&d_head);
    return init_info("d_info", d_head, d_info, parse_d_info, nullptr);
}

/*!
 * @brief Vault情報読み込みのメインルーチン /
 * Initialize the "v_info" array
 * @return エラーコード
 * @note
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
errr init_v_info()
{
    init_header(&v_head);
    return init_info("v_info", v_head, v_info, parse_v_info, nullptr);
}

/*!
 * @brief 職業技能情報読み込みのメインルーチン /
 * Initialize the "s_info" array
 * @return エラーコード
 */
errr init_s_info()
{
    init_header(&s_head, MAX_CLASS);
    return init_info("s_info", s_head, s_info, parse_s_info, nullptr);
}

/*!
 * @brief 職業魔法情報読み込みのメインルーチン /
 * Initialize the "m_info" array
 * @return エラーコード
 */
errr init_m_info()
{
    init_header(&m_head, MAX_CLASS);
    return init_info("m_info", m_head, m_info, parse_m_info, nullptr);
}
