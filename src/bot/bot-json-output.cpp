#include "bot/bot-json-output.h"
#include "game-option/runtime-arguments.h"
#include "locale/character-encoding.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "timed-effect/timed-effects.h"
#include "util/enum-converter.h"
#include "world/world.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace {
constexpr auto BOT_VIEW_RADIUS = 12;

nlohmann::json make_grid_json(const FloorType &floor, const Pos2D &pos, bool can_see_monsters)
{
    const auto &grid = floor.get_grid(pos);
    const auto is_known = grid.is_mark() || grid.is_view();
    if (!is_known) {
        return {
            { "y", pos.y },
            { "x", pos.x },
            { "known", false },
            { "flags", {
                           { "mark", false },
                           { "cave_known", false },
                           { "lite", false },
                           { "view", false },
                           { "room", false },
                           { "unsafe", false },
                       } },
        };
    }

    MONSTER_IDX visible_monster_index = 0;
    if (can_see_monsters && grid.has_monster()) {
        const auto &monster = floor.m_list[grid.m_idx];
        if (monster.is_valid() && monster.ml) {
            visible_monster_index = grid.m_idx;
        }
    }

    const auto visible_object_count = std::count_if(grid.o_idx_list.begin(), grid.o_idx_list.end(), [&floor](auto o_idx) {
        const auto &item = *floor.o_list[o_idx];
        return item.is_valid() && item.marked.has(OmType::FOUND);
    });

    return {
        { "y", pos.y },
        { "x", pos.x },
        { "known", true },
        { "terrain_id", grid.get_terrain_id() },
        { "monster_index", visible_monster_index },
        { "object_count", visible_object_count },
        { "flags", {
                       { "mark", grid.is_mark() },
                       { "cave_known", (grid.info & CAVE_KNOWN) != 0 },
                       { "lite", grid.is_lite() },
                       { "view", grid.is_view() },
                       { "room", grid.is_room() },
                       { "unsafe", (grid.info & CAVE_UNSAFE) != 0 },
                   } },
        { "terrain", {
                         { "floor", grid.has(TerrainCharacteristics::FLOOR) },
                         { "wall", grid.has(TerrainCharacteristics::WALL) },
                         { "move", grid.has(TerrainCharacteristics::MOVE) },
                         { "los", grid.has(TerrainCharacteristics::LOS) },
                         { "door", grid.has(TerrainCharacteristics::DOOR) },
                         { "trap", grid.has(TerrainCharacteristics::TRAP) },
                         { "stairs", grid.has(TerrainCharacteristics::STAIRS) },
                         { "up_stairs", grid.has(TerrainCharacteristics::UP_STAIRS) },
                         { "down_stairs", grid.has(TerrainCharacteristics::DOWN_STAIRS) },
                     } },
    };
}

nlohmann::json make_visible_monsters_json(const PlayerType &player)
{
    auto monsters = nlohmann::json::array();
    if (player.effects()->hallucination().is_hallucinated()) {
        return monsters;
    }

    const auto &floor = *player.current_floor_ptr;
    for (MONSTER_IDX m_idx = 1; m_idx < floor.m_max; ++m_idx) {
        const auto &monster = floor.m_list[m_idx];
        if (!monster.is_valid() || !monster.ml) {
            continue;
        }

        const auto &monrace = monster.get_apparent_monrace();
        const auto symbol_code = static_cast<uint8_t>(monrace.symbol_config.character);
        const auto symbol = symbol_code < 0x80 ? std::string(1, monrace.symbol_config.character) : "?";
        monsters.push_back({
            { "index", m_idx },
            { "race_id", enum2i(monrace.idx) },
            { "name", sys_to_utf8(monrace.name.string()).value_or("<encoding-error>") },
            { "y", monster.fy },
            { "x", monster.fx },
            { "hp", monster.hp },
            { "max_hp", monster.maxhp },
            { "speed", monster.mspeed },
            { "distance", monster.cdis },
            { "asleep", monster.is_asleep() },
            { "stunned", monster.is_stunned() },
            { "confused", monster.is_confused() },
            { "fearful", monster.is_fearful() },
            { "friendly", monster.is_friendly() },
            { "pet", monster.is_pet() },
            { "symbol", symbol },
            { "symbol_code", symbol_code },
            { "color", monrace.symbol_config.color },
        });
    }

    return monsters;
}

nlohmann::json make_nearby_grids_json(const PlayerType &player)
{
    const auto &floor = *player.current_floor_ptr;
    const auto can_see_monsters = !player.effects()->hallucination().is_hallucinated();
    auto grids = nlohmann::json::array();
    const auto y_min = std::max<POSITION>(0, player.y - BOT_VIEW_RADIUS);
    const auto y_max = std::min<POSITION>(floor.height - 1, player.y + BOT_VIEW_RADIUS);
    const auto x_min = std::max<POSITION>(0, player.x - BOT_VIEW_RADIUS);
    const auto x_max = std::min<POSITION>(floor.width - 1, player.x + BOT_VIEW_RADIUS);
    for (POSITION y = y_min; y <= y_max; ++y) {
        for (POSITION x = x_min; x <= x_max; ++x) {
            grids.push_back(make_grid_json(floor, { y, x }, can_see_monsters));
        }
    }

    return grids;
}

nlohmann::json make_snapshot(PlayerType *player_ptr)
{
    const auto &player = *player_ptr;
    const auto &floor = *player.current_floor_ptr;
    const auto &world = AngbandWorld::get_instance();
    return {
        { "type", "player_turn" },
        { "turn", world.game_turn },
        { "dungeon_turn", world.dungeon_turn },
        { "player", {
                        { "y", player.y },
                        { "x", player.x },
                        { "level", player.lev },
                        { "hp", player.chp },
                        { "max_hp", player.mhp },
                        { "mp", player.csp },
                        { "max_mp", player.msp },
                        { "exp", player.exp },
                        { "gold", player.au },
                        { "food", player.food },
                        { "speed", player.pspeed },
                        { "energy_need", player.energy_need },
                        { "action", player.action },
                        { "resting", player.resting },
                        { "word_recall", player.word_recall },
                    } },
        { "floor", {
                       { "dungeon_id", enum2i(floor.dungeon_id) },
                       { "level", floor.dun_level },
                       { "base_level", floor.base_level },
                       { "width", floor.width },
                       { "height", floor.height },
                       { "inside_arena", floor.inside_arena },
                       { "quest_id", enum2i(floor.quest_number) },
                   } },
        { "nearby_radius", BOT_VIEW_RADIUS },
        { "nearby_grids", make_nearby_grids_json(player) },
        { "visible_monsters", make_visible_monsters_json(player) },
    };
}

void write_snapshot(const nlohmann::json &snapshot)
{
    const auto serialized = snapshot.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
    if (arg_bot_json_output_path == "-") {
        std::cout << serialized << '\n';
        std::cout.flush();
        return;
    }

    static std::ofstream ofs;
    static std::string opened_path;
    if (!ofs.is_open() || opened_path != arg_bot_json_output_path) {
        ofs.close();
        opened_path = arg_bot_json_output_path;
        ofs.open(opened_path, std::ios::out | std::ios::app);
    }

    if (!ofs) {
        return;
    }

    ofs << serialized << '\n';
    ofs.flush();
}
}

void output_bot_json_snapshot(PlayerType *player_ptr)
{
    if (!arg_bot_json_output || player_ptr == nullptr || player_ptr->current_floor_ptr == nullptr) {
        return;
    }

    write_snapshot(make_snapshot(player_ptr));
}
