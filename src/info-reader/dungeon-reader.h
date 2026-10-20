#pragma once

#include "info-reader/parse-error-types.h"
#include <nlohmann/json_fwd.hpp>
#include <string_view>
#include <tl/expected.hpp>

class DungeonDefinition;

class DungeonReader {
public:
    explicit DungeonReader(const nlohmann::json &dungeon_data);
    DungeonReader(nlohmann::json &&) = delete;
    DungeonReader(const DungeonReader &) = delete;
    DungeonReader(DungeonReader &&) = delete;
    DungeonReader &operator=(const DungeonReader &) = delete;
    DungeonReader &operator=(DungeonReader &&) = delete;

    tl::expected<void, parse_error_type> read() const;
    static tl::expected<void, parse_error_type> validate_wilderness_positions(int height, int width);

private:
    bool grab_one_dungeon_flag(DungeonDefinition &dungeon, std::string_view what) const;
    bool grab_one_dungeon_mode(DungeonDefinition &dungeon, std::string_view what) const;
    bool grab_one_basic_monster_flag(DungeonDefinition &dungeon, std::string_view what) const;
    bool grab_one_spell_monster_flag(DungeonDefinition &dungeon, std::string_view what) const;
    tl::expected<void, parse_error_type> set_dungeon_description(DungeonDefinition &dungeon) const;
    tl::expected<void, parse_error_type> set_dungeon_generation(DungeonDefinition &dungeon) const;
    tl::expected<void, parse_error_type> set_dungeon_floor(DungeonDefinition &dungeon) const;
    tl::expected<void, parse_error_type> set_dungeon_streams(const nlohmann::json &streams_obj, DungeonDefinition &dungeon) const;
    tl::expected<void, parse_error_type> set_dungeon_wall(DungeonDefinition &dungeon) const;
    tl::expected<void, parse_error_type> set_dungeon_flags(DungeonDefinition &dungeon) const;
    tl::expected<void, parse_error_type> set_dungeon_final_floor(DungeonDefinition &dungeon) const;
    tl::expected<void, parse_error_type> set_dungeon_monster_flags(const nlohmann::json &flags_obj, DungeonDefinition &dungeon) const;
    tl::expected<void, parse_error_type> set_dungeon_monster_symbols(const nlohmann::json &symbols_obj, DungeonDefinition &dungeon) const;
    tl::expected<void, parse_error_type> set_dungeon_monster_spells(const nlohmann::json &spells_obj, DungeonDefinition &dungeon) const;
    tl::expected<void, parse_error_type> set_dungeon_monsters(DungeonDefinition &dungeon) const;

    const nlohmann::json &dungeon_data;
};
