/*!
 * @file info-initializer.cpp
 * @brief 変愚蛮怒のゲームデータ解析処理定義
 */

#include "main/info-initializer.h"
#include "info-reader/artifact-reader.h"
#include "info-reader/baseitem-reader.h"
#include "info-reader/dungeon-reader.h"
#include "info-reader/ego-reader.h"
#include "info-reader/feature-reader.h"
#include "info-reader/fixed-map-parser.h"
#include "info-reader/general-parser.h"
#include "info-reader/info-reader-util.h"
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
#include "player-info/class-info.h"
#include "player/player-skill.h"
#include "room/rooms-vault.h"
#include "system/angband-version.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info.h"
#include "system/dungeon-info.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <sys/stat.h>
#ifndef WINDOWS
#include <sys/types.h>
#endif
#include <string_view>

namespace {

using Retoucher = void (*)(angband_header *);

template <typename>
struct is_vector : std::false_type {
};

template <typename T, typename Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type {
};

/*!
 * @brief 与えられた型 T が std::vector 型かどうか調べる
 * T の型が std::vector<SomeType> に一致する時、is_vector_v<T> == true
 * 一致しない時、is_vector_v<T> == false となる
 * @tparam T 調べる型
 */
template <typename T>
constexpr bool is_vector_v = is_vector<T>::value;

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
static errr init_info(std::string_view filename, angband_header &head, InfoType &info, Parser parser, Retoucher retouch = nullptr)
{
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_EDIT, filename.data());

    auto *fp = angband_fopen(buf, "r");
    if (!fp) {
        quit_fmt(_("'%s'ファイルをオープンできません。", "Cannot open '%s' file."), filename);
    }

    constexpr auto info_is_vector = is_vector_v<InfoType>;
    if constexpr (info_is_vector) {
        using value_type = typename InfoType::value_type;
        info.assign(head.info_num, value_type{});
    }

    const auto err = init_info_txt(fp, buf, &head, parser);
    angband_fclose(fp);
    if (err) {
        const auto oops = (((err > 0) && (err < PARSE_ERROR_MAX)) ? err_str[err] : _("未知の", "unknown"));
#ifdef JP
        msg_format("'%s'ファイルの %d 行目にエラー。", filename, error_line);
#else
        msg_format("Error %d at line %d of '%s'.", err, error_line, filename);
#endif
        msg_format(_("レコード %d は '%s' エラーがあります。", "Record %d contains a '%s' error."), error_idx, oops);
        msg_format(_("構文 '%s'。", "Parsing '%s'."), buf);
        msg_print(nullptr);
        quit_fmt(_("'%s'ファイルにエラー", "Error in '%s' file."), filename);
    }

    if constexpr (info_is_vector) {
        info.shrink_to_fit();
    }

    head.info_num = static_cast<uint16_t>(info.size());
    if (retouch) {
        (*retouch)(&head);
    }

    return 0;
}

/*!
 * @brief 固定アーティファクト情報読み込みのメインルーチン
 * @return エラーコード
 */
errr init_artifacts_info()
{
    init_header(&artifacts_header);
    return init_info("ArtifactDefinitions.txt", artifacts_header, artifacts_info, parse_artifacts_info);
}

/*!
 * @brief ベースアイテム情報読み込みのメインルーチン
 * @return エラーコード
 */
errr init_baseitems_info()
{
    init_header(&baseitems_header);
    return init_info("BaseitemDefinitions.txt", baseitems_header, baseitems_info, parse_baseitems_info);
}

/*!
 * @brief 職業魔法情報読み込みのメインルーチン
 * @return エラーコード
 */
errr init_class_magics_info()
{
    init_header(&class_magics_header, PLAYER_CLASS_TYPE_MAX);
    auto *parser = parse_class_magics_info;
    return init_info("ClassMagicDefinitions.txt", class_magics_header, class_magics_info, parser);
}

/*!
 * @brief 職業技能情報読み込みのメインルーチン
 * @return エラーコード
 */
errr init_class_skills_info()
{
    init_header(&class_skills_header, PLAYER_CLASS_TYPE_MAX);
    return init_info("ClassSkillDefinitions.txt", class_skills_header, class_skills_info, parse_class_skills_info);
}
/*!
 * @brief ダンジョン情報読み込みのメインルーチン
 * @return エラーコード
 */
errr init_dungeons_info()
{
    init_header(&dungeons_header);
    return init_info("DungeonDefinitions.txt", dungeons_header, dungeons_info, parse_dungeons_info);
}

/*!
 * @brief エゴ情報読み込みのメインルーチン
 * @return エラーコード
 */
errr init_egos_info()
{
    init_header(&egos_header);
    return init_info("EgoDefinitions.txt", egos_header, egos_info, parse_egos_info);
}

/*!
 * @brief 地形情報読み込みのメインルーチン
 * @return エラーコード
 */
errr init_terrains_info()
{
    init_header(&terrains_header);
    auto *parser = parse_terrains_info;
    auto *retoucher = retouch_terrains_info;
    return init_info("TerrainDefinitions.txt", terrains_header, terrains_info, parser, retoucher);
}

/*!
 * @brief モンスター種族情報読み込みのメインルーチン
 * @return エラーコード
 */
errr init_monster_race_definitions()
{
    init_header(&monraces_header);
    return init_info("MonsterRaceDefinitions.txt", monraces_header, monraces_info, parse_monraces_info);
}

/*!
 * @brief Vault情報読み込みのメインルーチン
 * @return エラーコード
 * @note
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
errr init_vaults_info()
{
    init_header(&vaults_header);
    return init_info("VaultDefinitions.txt", vaults_header, vaults_info, parse_vaults_info);
}

/*!
 * @brief 基本情報読み込みのメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return エラーコード
 */
errr init_misc(PlayerType *player_ptr)
{
    return parse_fixed_map(player_ptr, "misc.txt", 0, 0, 0, 0);
}
