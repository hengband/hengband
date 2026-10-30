#pragma once

#include "info-reader/definition-id-checker.h"
#include "info-reader/parse-error-types.h"
#include <nlohmann/json_fwd.hpp>
#include <string_view>
#include <tl/expected.hpp>

class DungeonDefinition;

class DungeonReader {
public:
    using Session = DefinitionIdChecker;
    static Session create_session(tl::optional<int> last_id = tl::nullopt);

    DungeonReader(const nlohmann::json &dungeon_data, Session &session);
    DungeonReader(nlohmann::json &&, Session &) = delete;
    DungeonReader(const DungeonReader &) = delete;
    DungeonReader(DungeonReader &&) = delete;
    DungeonReader &operator=(const DungeonReader &) = delete;
    DungeonReader &operator=(DungeonReader &&) = delete;

    tl::expected<void, ParseErrorType> read() const;
    static tl::expected<void, ParseErrorType> validate_wilderness_positions(int height, int width);

private:
    bool grab_one_dungeon_flag(DungeonDefinition &dungeon, std::string_view what) const;
    bool grab_one_basic_monster_flag(DungeonDefinition &dungeon, std::string_view what) const;
    bool grab_one_spell_monster_flag(DungeonDefinition &dungeon, std::string_view what) const;
    tl::expected<void, ParseErrorType> set_dungeon_description(DungeonDefinition &dungeon) const;
    tl::expected<void, ParseErrorType> set_dungeon_generation(DungeonDefinition &dungeon) const;
    tl::expected<void, ParseErrorType> set_dungeon_floor(DungeonDefinition &dungeon) const;
    tl::expected<void, ParseErrorType> set_dungeon_streams(const nlohmann::json &streams_obj, DungeonDefinition &dungeon) const;
    tl::expected<void, ParseErrorType> set_dungeon_wall(DungeonDefinition &dungeon) const;
    tl::expected<void, ParseErrorType> set_dungeon_flags(DungeonDefinition &dungeon) const;
    tl::expected<void, ParseErrorType> set_dungeon_final_floor(DungeonDefinition &dungeon) const;
    tl::expected<void, ParseErrorType> set_dungeon_monster_flags(const nlohmann::json &flags_obj, DungeonDefinition &dungeon) const;
    tl::expected<void, ParseErrorType> set_dungeon_monster_symbols(const nlohmann::json &symbols_obj, DungeonDefinition &dungeon) const;
    tl::expected<void, ParseErrorType> set_dungeon_monster_spells(const nlohmann::json &spells_obj, DungeonDefinition &dungeon) const;
    tl::expected<void, ParseErrorType> set_dungeon_monsters(DungeonDefinition &dungeon) const;

    const nlohmann::json &dungeon_data;
    Session &session;
};
