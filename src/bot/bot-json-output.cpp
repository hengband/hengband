#include "bot/bot-json-output.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "game-option/runtime-arguments.h"
#include "inventory/inventory-slot-types.h"
#include "locale/character-encoding.h"
#include "object/tval-types.h"
#include "player-base/player-race.h"
#include "player-info/race-info.h"
#include "store/pricing.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/baseitem/baseitem-key.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/store-sale-type.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "timed-effect/timed-effects.h"
#include "util/enum-converter.h"
#include "world/world.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace {
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

    const auto is_store = grid.has(TerrainCharacteristics::STORE);
    auto result = nlohmann::json{
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
                         { "entrance", grid.has(TerrainCharacteristics::ENTRANCE) },
                         { "quest_enter", grid.has(TerrainCharacteristics::QUEST_ENTER) },
                         { "store", is_store },
                         { "can_dig", grid.has(TerrainCharacteristics::CAN_DIG) },
                     } },
    };
    // A store entrance: expose which store it is so the bot can walk to the
    // General Store (0) to shop. Emitted only on store tiles to avoid bloat.
    if (is_store) {
        result["store_number"] = enum2i(grid.get_terrain().store_sale_type);
    }
    return result;
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
    // Emit the player's entire memorised map (marked tiles) plus the current
    // view — not just a small window. The town is fully known from the start
    // (so the bot can see the dungeon entrance immediately, like the player),
    // and in the dungeon this lets it navigate to any already-explored feature.
    // Unknown tiles are omitted; the client treats an absent but in-bounds
    // neighbour as a frontier.
    for (POSITION y = 0; y < floor.height; ++y) {
        for (POSITION x = 0; x < floor.width; ++x) {
            const auto &grid = floor.get_grid({ y, x });
            if (!grid.is_mark() && !grid.is_view()) {
                continue;
            }

            grids.push_back(make_grid_json(floor, { y, x }, can_see_monsters));
        }
    }

    return grids;
}

nlohmann::json make_player_status_json(const PlayerType &player)
{
    return {
        { "blind", player.effects()->blindness().is_blind() },
        { "confused", player.effects()->confusion().is_confused() },
        { "afraid", player.effects()->fear().is_fearful() },
        { "poisoned", player.effects()->poison().is_poisoned() },
        { "stunned", player.effects()->stun().is_stunned() },
        { "cut", player.effects()->cut().is_cut() },
        { "paralyzed", player.effects()->paralysis().is_paralyzed() },
        { "hallucinated", player.effects()->hallucination().is_hallucinated() },
    };
}

nlohmann::json make_item_json(PlayerType *player_ptr, const ItemEntity &item)
{
    return {
        { "name", sys_to_utf8(describe_flavor(player_ptr, item, OD_OMIT_PREFIX)).value_or("<encoding-error>") },
        { "count", item.number },
        { "tval", enum2i(item.bi_key.tval()) },
        { "sval", item.bi_key.sval().value_or(-1) },
        { "charges", item.pval },
        { "aware", item.is_aware() },
        { "known", item.is_known() },
    };
}

nlohmann::json make_inventory_json(PlayerType *player_ptr)
{
    auto items = nlohmann::json::array();
    for (const auto i_idx : INVEN_PACK_SLOTS) {
        const auto &item_ptr = player_ptr->inventory[i_idx];
        if (!item_ptr || !item_ptr->is_valid()) {
            continue;
        }

        auto entry = make_item_json(player_ptr, *item_ptr);
        entry["slot"] = std::string(1, static_cast<char>('a' + static_cast<int>(i_idx)));
        items.push_back(std::move(entry));
    }

    return items;
}

const char *equipment_slot_label(inventory_slot_type slot)
{
    switch (slot) {
    case INVEN_MAIN_HAND:
        return "main_hand";
    case INVEN_SUB_HAND:
        return "sub_hand";
    case INVEN_BOW:
        return "bow";
    case INVEN_MAIN_RING:
        return "main_ring";
    case INVEN_SUB_RING:
        return "sub_ring";
    case INVEN_NECK:
        return "neck";
    case INVEN_LITE:
        return "light";
    case INVEN_BODY:
        return "body";
    case INVEN_OUTER:
        return "outer";
    case INVEN_HEAD:
        return "head";
    case INVEN_ARMS:
        return "arms";
    case INVEN_FEET:
        return "feet";
    default:
        return "unknown";
    }
}

nlohmann::json make_equipment_json(PlayerType *player_ptr)
{
    auto items = nlohmann::json::array();
    for (const auto i_idx : INVEN_WIELDING_SLOTS) {
        const auto &item_ptr = player_ptr->inventory[i_idx];
        if (!item_ptr || !item_ptr->is_valid()) {
            continue;
        }

        auto entry = make_item_json(player_ptr, *item_ptr);
        entry["slot"] = equipment_slot_label(i_idx);
        items.push_back(std::move(entry));
    }

    return items;
}

nlohmann::json make_store_json(PlayerType *player_ptr, StoreSaleType store_num)
{
    auto items = nlohmann::json::array();
    if (st_ptr != nullptr) {
        for (auto i = 0; i < st_ptr->stock_num; ++i) {
            // The store accepts the item letter RELATIVE to the currently visible
            // page: pressing 'a' selects stock[store_top]. Only items on the
            // current page (store_top .. store_top+store_bottom) are selectable,
            // so emit page-relative letters and skip off-page items (the bot does
            // not page, so it only ever sees page 1, but this stays correct if it
            // ever does). Mirrors display_entry()'s labelling.
            const auto page_pos = i - store_top;
            if (page_pos < 0 || page_pos >= store_bottom) {
                continue;
            }
            const auto &item = *st_ptr->stock[i];
            const auto letter = (page_pos < 26)
                                    ? std::string(1, static_cast<char>('a' + page_pos))
                                    : std::string(1, static_cast<char>('A' + (page_pos - 26)));
            const auto price = (ot_ptr != nullptr)
                                   ? price_item(player_ptr, item.calc_price(), ot_ptr->inflate, false, store_num)
                                   : item.calc_price();
            items.push_back({
                { "letter", letter },
                { "name", sys_to_utf8(describe_flavor(player_ptr, item, OD_OMIT_PREFIX)).value_or("<encoding-error>") },
                { "count", item.number },
                { "tval", enum2i(item.bi_key.tval()) },
                { "sval", item.bi_key.sval().value_or(-1) },
                { "price", price },
            });
        }
    }

    return {
        { "store_type", enum2i(store_num) },
        { "items", items },
    };
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
                        { "food_type", enum2i(PlayerRace(player_ptr).food()) },
                        { "status", make_player_status_json(player) },
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
        { "nearby_grids", make_nearby_grids_json(player) },
        { "visible_monsters", make_visible_monsters_json(player) },
        { "inventory", make_inventory_json(player_ptr) },
        { "equipment", make_equipment_json(player_ptr) },
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
        // Truncate at session start so the log does not grow without bound
        // across runs (full-map snapshots are large); the client tails it live.
        ofs.open(opened_path, std::ios::out | std::ios::trunc);
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

void output_bot_json_store_snapshot(PlayerType *player_ptr, StoreSaleType store_num)
{
    if (!arg_bot_json_output || player_ptr == nullptr || player_ptr->current_floor_ptr == nullptr) {
        return;
    }

    // Same base snapshot (player gold, inventory, equipment, town map) plus the
    // store's stock, so the bot can decide what to buy while at the store prompt.
    auto snapshot = make_snapshot(player_ptr);
    snapshot["type"] = "store";
    snapshot["store"] = make_store_json(player_ptr, store_num);
    write_snapshot(snapshot);
}
