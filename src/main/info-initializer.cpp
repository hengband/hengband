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
#include "info-reader/message-reader.h"
#include "info-reader/race-reader.h"
#include "info-reader/skill-reader.h"
#include "info-reader/vault-reader.h"
#include "io/files-util.h"
#include "io/uid-checker.h"
#include "main/angband-headers.h"
#include "main/init-error-messages-table.h"
#include "object-enchant/object-ego.h"
#include "player-info/class-info.h"
#include "player/player-skill.h"
#include "room/rooms-vault.h"
#include "system/angband-version.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/floor/wilderness-grid.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "system/spell-info-list.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/angband-files.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <fstream>
#include <functional>
#include <string>
#include <string_view>
#include <sys/stat.h>
#ifndef WINDOWS
#include <sys/types.h>
#endif

namespace {

/// @note clang-formatによるconceptの整形が安定していないので抑制しておく
// clang-format off
template <typename T>
concept HasShrinkToFit = requires(T t) {
    t.shrink_to_fit();
};
// clang-format on
}

/*!
 * @brief ヘッダ構造体の更新
 * Initialize the header of an *_info.raw file.
 * @param head ヘッダ構造体
 * @return エラーコード
 */
static void init_header(angband_header *head)
{
    head->digest = {};
}

/*!
 * @brief 各種設定データをlib/edit/のテキストから読み込み
 * Initialize the "*_info" array
 * @param filename ファイル名(拡張子txt)
 * @param head 処理に用いるヘッダ構造体
 * @param info データ保管先の構造体ポインタ
 * @note
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
template <typename InfoType>
static void init_info(std::string_view filename, angband_header &head, InfoType &info, Parser parser, std::function<void()> retouch = nullptr)
{
    const auto path = path_build(ANGBAND_DIR_EDIT, filename);
    auto *fp = angband_fopen(path, FileOpenMode::READ);
    if (!fp) {
        quit_fmt(_("'%s'ファイルをオープンできません。", "Cannot open '%s' file."), filename.data());
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
        msg_erase();
        quit_fmt(_("'%s'ファイルにエラー", "Error in '%s' file."), filename.data());
    }

    if constexpr (HasShrinkToFit<InfoType>) {
        info.shrink_to_fit();
    }

    if (retouch) {
        retouch();
    }
}

/*!
 * @brief 各種設定データをlib/edit/.jsoncから読み込み
 * Load data from lib/edit/.jsonc
 * @param filename ファイル名(拡張子jsonc)
 * @param head 処理に用いるヘッダ構造体
 * @param info データ保管先の構造体ポインタ
 * @note
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
template <typename InfoType>
static void init_json(std::string_view filename, std::string_view keyname, angband_header &head, InfoType &info, JSONParser parser)
{
    const auto path = path_build(ANGBAND_DIR_EDIT, filename);
    std::ifstream ifs(path);

    if (!ifs) {
        quit_fmt(_("'%s'ファイルをオープンできません。", "Cannot open '%s' file."), filename.data());
    }

    std::istreambuf_iterator<char> ifs_iter(ifs);
    std::istreambuf_iterator<char> ifs_end;
    auto json_object = nlohmann::json::parse(ifs_iter, ifs_end, nullptr, true, true, true);

    error_idx = -1;

    for (auto &element : json_object[keyname]) {
        const auto error_code = parser(element, &head);
        if (error_code != PARSE_ERROR_NONE) {
            msg_erase();
            quit_fmt(_("'%s'ファイルにエラー", "Error in '%s' file."), filename.data());
        }
    }

    util::SHA256 sha256;
    sha256.update(json_object.dump());
    head.digest = sha256.digest();

    if constexpr (HasShrinkToFit<InfoType>) {
        info.shrink_to_fit();
    }
}

/*!
 * @brief 固定アーティファクト情報読み込みのメインルーチン
 */
void init_artifacts_info()
{
    init_header(&artifacts_header);
    init_json("ArtifactDefinitions.jsonc", "artifacts", artifacts_header, ArtifactList::get_instance(), parse_artifacts_info);
}

/*!
 * @brief ベースアイテム情報読み込みのメインルーチン
 */
void init_baseitems_info()
{
    init_header(&baseitems_header);
    init_json("BaseitemDefinitions.jsonc", "baseitems", baseitems_header, BaseitemList::get_instance(), parse_baseitems_info);
}

/*!
 * @brief 職業魔法情報読み込みのメインルーチン
 */
void init_class_magics_info()
{
    init_header(&class_magics_header);
    class_magics_info.assign(PLAYER_CLASS_TYPE_MAX, {});
    init_json("ClassMagicDefinitions.jsonc", "classes", class_magics_header, class_magics_info, parse_class_magics_info);
}

/*!
 * @brief 職業技能情報読み込みのメインルーチン
 */
void init_class_skills_info()
{
    init_header(&class_skills_header);
    class_skills_info.assign(PLAYER_CLASS_TYPE_MAX, {});
    init_info("ClassSkillDefinitions.txt", class_skills_header, class_skills_info, parse_class_skills_info);
}
/*!
 * @brief ダンジョン情報読み込みのメインルーチン
 */
void init_dungeons_info()
{
    init_header(&dungeons_header);
    auto &dungeons = DungeonList::get_instance();
    init_info("DungeonDefinitions.txt", dungeons_header, dungeons, parse_dungeons_info, [&dungeons] { dungeons.retouch(); });
}

/*!
 * @brief エゴ情報読み込みのメインルーチン
 */
void init_egos_info()
{
    init_header(&egos_header);
    init_info("EgoDefinitions.txt", egos_header, egos_info, parse_egos_info);
}

/*!
 * @brief 地形情報読み込みのメインルーチン
 */
void init_terrains_info()
{
    init_header(&terrains_header);
    auto *parser = parse_terrains_info;
    auto &terrains = TerrainList::get_instance();
    init_info("TerrainDefinitions.txt", terrains_header, terrains, parser, [&terrains] { terrains.retouch(); });
}

/*!
 * @brief モンスター種族情報読み込みのメインルーチン
 */
void init_monrace_definitions()
{
    init_header(&monraces_header);
    init_json("MonraceDefinitions.jsonc", "monsters", monraces_header, MonraceList::get_instance(), parse_monraces_info);
}

/*!
 * @brief モンスターメッセージ読み込みのメインルーチン
 */
void init_monster_message_definitions()
{
    init_header(&monster_messages_header);
    init_json("MonsterMessages.jsonc", "groups", monster_messages_header, MonraceMessageList::get_instance(), parse_monster_messages_info);
}

/*!
 * @brief 呪文情報読み込みのメインルーチン
 */
void init_spell_info()
{
    init_header(&spells_header);
    auto &spell_info_list = SpellInfoList::get_instance();
    spell_info_list.initialize();
    auto parser = [&spell_info_list](nlohmann::json &spell_data, angband_header *) {
        return spell_info_list.parse(spell_data);
    };
    init_json("SpellDefinitions.jsonc", "realms", spells_header, spell_info_list, parser);
}

/*!
 * @brief Vault情報読み込みのメインルーチン
 * @note
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
void init_vaults_info()
{
    init_header(&vaults_header);
    init_info("VaultDefinitions.txt", vaults_header, vaults_info, parse_vaults_info);
}

static bool read_wilderness_definition(std::ifstream &ifs)
{
    auto &wilderness = WildernessGrids::get_instance();
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
            wilderness.initialize_width(std::stoi(splits[2]));
        } else if (splits[1] == "WY") {
            wilderness.initialize_height(std::stoi(splits[2]));
        } else {
            return false;
        }

        if (wilderness.is_height_initialized() && wilderness.is_width_initialized()) {
            wilderness.initialize_grids();
            wilderness.set_ambushes(false);
            return true;
        }
    }

    return false;
}

/*!
 * @brief 荒野情報読み込み処理
 */
void init_wilderness()
{
    const auto path = path_build(ANGBAND_DIR_EDIT, WILDERNESS_DEFINITION);
    std::ifstream ifs(path);
    if (!ifs) {
        quit_fmt(_("'%s'ファイルをオープンできません。", "Cannot open '%s' file."), WILDERNESS_DEFINITION);
    }

    if (!read_wilderness_definition(ifs)) {
        quit(_("荒野を初期化できません", "Cannot initialize wilderness"));
    }
}
