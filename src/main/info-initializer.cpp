/*!
 * @file info-initializer.cpp
 * @brief 変愚蛮怒のゲームデータ解析処理定義
 */

#include "main/info-initializer.h"
#include "floor/wild.h"
#include "info-reader/artifact-reader.h"
#include "info-reader/baseitem-reader.h"
#include "info-reader/definition-hash-data.h"
#include "info-reader/dungeon-reader.h"
#include "info-reader/ego-reader.h"
#include "info-reader/fixed-map-parser.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/magic-reader.h"
#include "info-reader/message-reader.h"
#include "info-reader/race-reader.h"
#include "info-reader/skill-reader.h"
#include "info-reader/spell-reader.h"
#include "info-reader/terrain-reader.h"
#include "info-reader/vault-reader.h"
#include "info-reader/wilderness-reader.h"
#include "io/files-util.h"
#include "io/uid-checker.h"
#include "object-enchant/object-ego.h"
#include "player-info/class-info.h"
#include "player/player-skill.h"
#include "room/rooms-vault.h"
#include "system/angband-version.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/artifact/artifact-record.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "system/spell-info-list.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include <fmt/format.h>
#include <fstream>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <sys/stat.h>
#ifndef _WIN32
#include <sys/types.h>
#endif

namespace {

template <typename T>
concept HasShrinkToFit = requires(T t) {
    t.shrink_to_fit();
};

/*!
 * @brief 各種設定データをlib/edit/.jsoncから読み込み
 * Load data from lib/edit/.jsonc
 * @param filename ファイル名(拡張子jsonc)
 * @param keyname 定義配列を格納するルートオブジェクトのキー
 * @param dhdt 定義データのハッシュ保存先を識別する種別
 * @param definition_list 読み込み後に容量を調整する定義コンテナ
 * @param json_parser 各定義要素を解析して格納する関数
 * @param retouch 読み込み後に実行する追加処理 (省略可)
 * @param allow_empty 定義配列が空であることを許可するか
 */
template <typename DefinitionList>
void init_json(std::string_view filename, std::string_view keyname, DefinitionHashDataType dhdt, DefinitionList &definition_list, std::function<int(nlohmann::json &)> json_parser, std::function<void()> retouch = nullptr, bool allow_empty = true)
{
    const auto path = path_build(ANGBAND_DIR_EDIT, filename);
    std::ifstream ifs(path);

    if (!ifs) {
        quit(fmt::format(_("'{}'ファイルをオープンできません。", "Cannot open '{}' file."), filename));
    }

    std::istreambuf_iterator<char> ifs_iter(ifs);
    std::istreambuf_iterator<char> ifs_end;
    auto json_object = nlohmann::json::parse(ifs_iter, ifs_end, nullptr, true, true, true);

    if (const auto err = info_validate_json_array(json_object, keyname, allow_empty); err != PARSE_ERROR_NONE) {
        if (err == PARSE_ERROR_INVALID_VALUE) {
            quit(fmt::format(_("{}: $.{}: 空配列は許可されません", "{}: $.{}: empty array is not allowed"), filename, keyname));
        }
        quit(fmt::format(_("{}: ルートオブジェクトに配列 '{}' が必要です", "{}: expected a root object containing array '{}'"), filename, keyname));
    }

    error_idx = -1;

    for (auto &element : json_object[keyname]) {
        const auto error_code = json_parser(element);
        if (error_code != PARSE_ERROR_NONE) {
            msg_erase();
            quit(fmt::format(_("'{}'ファイルにエラー", "Error in '{}' file."), filename));
        }
    }

    util::SHA256 sha256;
    sha256.update(json_object.dump());
    DefinitionHashData::get_instance().set_digest(dhdt, sha256.digest());

    if constexpr (HasShrinkToFit<DefinitionList>) {
        definition_list.shrink_to_fit();
    }

    if (retouch) {
        retouch();
    }
}

/*!
 * @brief Readerクラスを用いてlib/edit/.jsoncを読み込む
 * @details init_jsonのjson_parserに「JSON要素からReaderを構築してread()する」定型ラムダを与える処理を共通化したもの。
 */
template <typename Reader, typename DefinitionList>
void init_json_reader(std::string_view filename, std::string_view keyname, DefinitionHashDataType dhdt, DefinitionList &definition_list, std::function<void()> retouch = nullptr, bool allow_empty = true)
{
    auto parser = [](nlohmann::json &element) { return Reader(element).read(); };
    init_json(filename, keyname, dhdt, definition_list, parser, retouch, allow_empty);
}
}

/*!
 * @brief 固定アーティファクト情報読み込みのメインルーチン
 */
void init_artifacts_info()
{
    auto &artifacts = ArtifactList::get_instance();
    init_json_reader<ArtifactReader>("ArtifactDefinitions.jsonc", "artifacts", DefinitionHashDataType::ARTIFACTS, artifacts);
    ArtifactRecords::get_instance().initialize(artifacts.size());
}

/*!
 * @brief ベースアイテム情報読み込みのメインルーチン
 */
void init_baseitems_info()
{
    init_json_reader<BaseitemReader>("BaseitemDefinitions.jsonc", "baseitems", DefinitionHashDataType::BASEITEMS, BaseitemList::get_instance());
}

/*!
 * @brief 職業魔法情報読み込みのメインルーチン
 */
void init_class_magics_info()
{
    class_magics_info.assign(PLAYER_CLASS_TYPE_MAX, {});
    init_json_reader<MagicReader>("ClassMagicDefinitions.jsonc", "classes", DefinitionHashDataType::CLASS_MAGICS, class_magics_info);
}

/*!
 * @brief 職業技能情報読み込みのメインルーチン
 */
void init_class_skills_info()
{
    class_skills_info.assign(PLAYER_CLASS_TYPE_MAX, {});
    auto parser = [](nlohmann::json &element) {
        SkillReader reader(element);
        const auto err = reader.read();
        if (err != PARSE_ERROR_NONE) {
            const auto &diagnostic = reader.error().value();
            quit(fmt::format(_("ClassSkillDefinitions.jsonc: 職業{}の{}: {}", "ClassSkillDefinitions.jsonc: class {} at {}: {}"), diagnostic.class_id, diagnostic.path, diagnostic.reason));
        }
        return err;
    };
    init_json("ClassSkillDefinitions.jsonc", "classes", DefinitionHashDataType::CLASS_SKILLS, class_skills_info, parser, [] {
        if (error_idx != PLAYER_CLASS_TYPE_MAX - 1) {
            quit(fmt::format(_("ClassSkillDefinitions.jsonc: 職業{}の$.classes: 職業レコードが不足しています", "ClassSkillDefinitions.jsonc: class {} at $.classes: missing class records"), error_idx + 1));
        }
    });
}

/*!
 * @brief ダンジョン情報読み込みのメインルーチン
 */
void init_dungeons_info()
{
    auto &dungeons = DungeonList::get_instance();
    init_json_reader<DungeonReader>("DungeonDefinitions.jsonc", "dungeons", DefinitionHashDataType::DUNGEONS, dungeons, [&dungeons] { dungeons.retouch(); });
}

/*!
 * @brief エゴ情報読み込みのメインルーチン
 */
void init_egos_info()
{
    init_json_reader<EgoReader>("EgoDefinitions.jsonc", "egos", DefinitionHashDataType::EGOS, egos_info, nullptr, false);
}

/*!
 * @brief 地形情報読み込みのメインルーチン
 */
void init_terrains_info()
{
    auto &terrains = TerrainList::get_instance();
    init_json_reader<TerrainReader>("TerrainDefinitions.jsonc", "terrains", DefinitionHashDataType::TERRAINS, terrains, [&terrains] { terrains.retouch(); });
}

/*!
 * @brief 地形の派生情報を初期化する
 */
void init_feat_variables()
{
    TerrainList::get_instance().emplace_tags();
    init_wilderness_terrains();
}

/*!
 * @brief モンスター種族情報読み込みのメインルーチン
 */
void init_monrace_definitions()
{
    init_json_reader<RaceReader>("MonraceDefinitions.jsonc", "monsters", DefinitionHashDataType::MONRACES, MonraceList::get_instance());
}

/*!
 * @brief モンスターメッセージ読み込みのメインルーチン
 */
void init_monster_message_definitions()
{
    init_json_reader<MessageReader>("MonsterMessages.jsonc", "groups", DefinitionHashDataType::MONSTER_MESSAGES, MonraceMessageList::get_instance());
}

/*!
 * @brief 呪文情報読み込みのメインルーチン
 */
void init_spell_info()
{
    auto &spell_info_list = SpellInfoList::get_instance();
    spell_info_list.initialize();
    auto parser = [&spell_info_list](nlohmann::json &spell_data) {
        return SpellReader(spell_data, spell_info_list).read();
    };
    init_json("SpellDefinitions.jsonc", "realms", DefinitionHashDataType::SPELLS, spell_info_list, parser);
}

/*!
 * @brief Vault情報読み込みのメインルーチン
 * @note
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
void init_vaults_info()
{
    auto parser = [](nlohmann::json &element) {
        VaultReader reader(element);
        const auto err = reader.read();
        if (err != PARSE_ERROR_NONE) {
            const auto &diagnostic = reader.error().value();
            quit(fmt::format(_("VaultDefinitions.jsonc: Vault{}の{}: {}", "VaultDefinitions.jsonc: vault {} at {}: {}"), diagnostic.id, diagnostic.path, diagnostic.reason));
        }
        return err;
    };
    init_json("VaultDefinitions.jsonc", "vaults", DefinitionHashDataType::VAULTS, vaults_info, parser, nullptr, false);
}

/*!
 * @brief 荒野情報読み込み処理
 */
void init_wilderness()
{
    if (!initialize_wilderness_definition()) {
        quit(_("荒野を初期化できません", "Cannot initialize wilderness"));
    }
}
