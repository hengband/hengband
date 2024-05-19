/*!
 * @file info-initializer.cpp
 * @brief 変愚蛮怒のゲームデータ解析処理定義
 */

#include "main/info-initializer.h"
#include "external-lib/include-json.h"
#include "floor/wild.h"
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
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <fstream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#ifndef WINDOWS
#include <sys/types.h>
#endif

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
    head->digest = {};
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
    const auto path = path_build(ANGBAND_DIR_EDIT, filename);
    auto *fp = angband_fopen(path, FileOpenMode::READ);
    if (!fp) {
        quit_fmt(_("'%s'ファイルをオープンできません。", "Cannot open '%s' file."), filename.data());
    }

    constexpr auto info_is_vector = is_vector_v<InfoType>;
    if constexpr (info_is_vector) {
        using value_type = typename InfoType::value_type;
        info.assign(head.info_num, value_type{});
    }

    char buf[1024]{};
    const auto &[error_code, error_line] = init_info_txt(fp, buf, &head, parser);
    angband_fclose(fp);
    if (error_code != PARSE_ERROR_NONE) {
        const auto oops = (((error_code > 0) && (error_code < PARSE_ERROR_MAX)) ? err_str[error_code] : _("未知の", "unknown"));
#ifdef JP
        msg_format("'%s'ファイルの %d 行目にエラー。", filename.data(), error_line);
#else
        msg_format("Error %d at line %d of '%s'.", error_code, error_line, filename.data());
#endif
        msg_format(_("レコード %d は '%s' エラーがあります。", "Record %d contains a '%s' error."), error_idx, oops);
        msg_format(_("構文 '%s'。", "Parsing '%s'."), buf);
        msg_print(nullptr);
        quit_fmt(_("'%s'ファイルにエラー", "Error in '%s' file."), filename.data());
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
 * @brief 各種設定データをlib/edit/.jsoncから読み込み
 * Load data from lib/edit/.jsonc
 * @param filename ファイル名(拡張子jsonc)
 * @param head 処理に用いるヘッダ構造体
 * @param info データ保管先の構造体ポインタ
 * @return エラーコード
 * @note
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
template <typename InfoType>
static errr init_json(std::string_view filename, std::string_view keyname, angband_header &head, InfoType &info, JSONParser parser)
{
    const auto path = path_build(ANGBAND_DIR_EDIT, filename);
    std::ifstream ifs(path);

    if (!ifs) {
        quit_fmt(_("'%s'ファイルをオープンできません。", "Cannot open '%s' file."), filename.data());
    }

    std::istreambuf_iterator<char> ifs_iter(ifs);
    std::istreambuf_iterator<char> ifs_end;
    auto json_object = nlohmann::json::parse(ifs_iter, ifs_end, nullptr, true, true);

    constexpr auto info_is_vector = is_vector_v<InfoType>;
    if constexpr (info_is_vector) {
        using value_type = typename InfoType::value_type;
        info.assign(head.info_num, value_type{});
    }
    error_idx = -1;

    for (auto &element : json_object[keyname]) {
        const auto error_code = parser(element, &head);
        if (error_code != PARSE_ERROR_NONE) {
            quit_fmt(_("'%s'ファイルにエラー", "Error in '%s' file."), filename.data());
        }
    }

    util::SHA256 sha256;
    sha256.update(json_object.dump());
    head.digest = sha256.digest();

    return PARSE_ERROR_NONE;
}

/*!
 * @brief 固定アーティファクト情報読み込みのメインルーチン
 * @return エラーコード
 */
errr init_artifacts_info()
{
    init_header(&artifacts_header);
    return init_json("ArtifactDefinitions.jsonc", "artifacts", artifacts_header, ArtifactList::get_instance().get_raw_map(), parse_artifacts_info);
}

/*!
 * @brief ベースアイテム情報読み込みのメインルーチン
 * @return エラーコード
 */
errr init_baseitems_info()
{
    init_header(&baseitems_header);
    return init_json("BaseitemDefinitions.jsonc", "baseitems", baseitems_header, BaseitemList::get_instance().get_raw_vector(), parse_baseitems_info);
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
    auto &terrains = TerrainList::get_instance();
    return init_info("TerrainDefinitions.txt", terrains_header, terrains.get_raw_vector(), parser, retoucher);
}

/*!
 * @brief モンスター種族情報読み込みのメインルーチン
 * @return エラーコード
 */
errr init_monrace_definitions()
{
    init_header(&monraces_header);
    return init_json("MonraceDefinitions.jsonc", "monsters", monraces_header, monraces_info, parse_monraces_info);
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

static bool read_wilderness_definition(std::ifstream &ifs)
{
    std::string line;
    while (!ifs.eof()) {
        if (!std::getline(ifs, line)) {
            return false;
        }

        if (line.empty() || line.starts_with('#')) {
            continue;
        }

        const auto &splits = str_split(line, ':');
        if ((splits.size() != 3) || (splits[0] != "M")) {
            continue;
        }

        if (splits[1] == "WX") {
            w_ptr->max_wild_x = std::stoi(splits[2]);
        } else if (splits[1] == "WY") {
            w_ptr->max_wild_y = std::stoi(splits[2]);
        } else {
            return false;
        }

        if ((w_ptr->max_wild_x > 0) && (w_ptr->max_wild_y > 0)) {
            wilderness.assign(w_ptr->max_wild_y, std::vector<wilderness_type>(w_ptr->max_wild_x));
            init_wilderness_encounter();
            return true;
        }
    }

    return false;
}

/*!
 * @brief 荒野情報読み込み処理
 * @return 読み込みに成功したか
 */
bool init_wilderness()
{
    const auto path = path_build(ANGBAND_DIR_EDIT, WILDERNESS_DEFINITION);
    std::ifstream ifs(path);
    if (!ifs) {
        return false;
    }

    return read_wilderness_definition(ifs);
}
