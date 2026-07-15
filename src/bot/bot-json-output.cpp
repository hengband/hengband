#include "bot/bot-json-output.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "game-option/runtime-arguments.h"
#include "inventory/inventory-slot-types.h"
#include "locale/character-encoding.h"
#include "object-enchant/item-feeling.h"
#include "object/tval-types.h"
#include "player-ability/player-ability-types.h"
#include "player-base/player-race.h"
#include "player-info/race-info.h"
#include "player/digestion-processor.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "store/pricing.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/baseitem/baseitem-key.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/dungeon/dungeon-record.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/store-sale-type.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/enums/terrain/terrain-kind.h"
#include "system/floor/floor-info.h"
#include "system/floor/town-records.h"
#include "system/grid-type-definition.h"
#include "system/item/identification-flags.h"
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
constexpr std::streamoff BOT_JSON_OUTPUT_MAX_BYTES = 256LL * 1024 * 1024;

const char *pseudo_feeling_name(item_feel_type feeling)
{
    switch (feeling) {
    case FEEL_BROKEN:
        return "broken";
    case FEEL_TERRIBLE:
        return "terrible";
    case FEEL_WORTHLESS:
        return "worthless";
    case FEEL_CURSED:
        return "cursed";
    case FEEL_UNCURSED:
        return "uncursed";
    case FEEL_AVERAGE:
        return "average";
    case FEEL_GOOD:
        return "good";
    case FEEL_EXCELLENT:
        return "excellent";
    case FEEL_SPECIAL:
        return "special";
    default:
        return "none";
    }
}

nlohmann::json make_grid_json(const FloorType &floor, const Pos2D &pos)
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
    // Keep this in lockstep with make_visible_monsters_json(): ESP-only monsters
    // are deliberately excluded because the bot interface exposes direct sight.
    // A hallucinating player still SEES a monster at its real tile (the map draws
    // a random symbol there), so the position is emitted regardless — only the
    // monster's identity is redacted, over in make_visible_monsters_json().
    if (grid.is_view() && grid.has_monster()) {
        const auto &monster = floor.m_list[grid.m_idx];
        if (monster.is_valid() && monster.ml) {
            visible_monster_index = grid.m_idx;
        }
    }

    const auto visible_object_count = std::count_if(grid.o_idx_list.begin(), grid.o_idx_list.end(), [&floor](auto o_idx) {
        const auto &item = *floor.o_list[o_idx];
        return item.is_valid() && item.marked.has(OmType::FOUND);
    });

    const auto &terrain = grid.get_terrain(TerrainKind::MIMIC);
    const auto has_terrain = [&terrain](TerrainCharacteristics flag) { return terrain.flags.has(flag); };
    const auto is_store = has_terrain(TerrainCharacteristics::STORE);
    auto result = nlohmann::json{
        { "y", pos.y },
        { "x", pos.x },
        { "known", true },
        { "terrain_id", grid.get_terrain_id(TerrainKind::MIMIC) },
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
                         { "floor", has_terrain(TerrainCharacteristics::FLOOR) },
                         { "wall", has_terrain(TerrainCharacteristics::WALL) },
                         { "move", has_terrain(TerrainCharacteristics::MOVE) },
                         { "los", has_terrain(TerrainCharacteristics::LOS) },
                         { "door", has_terrain(TerrainCharacteristics::DOOR) },
                         { "trap", has_terrain(TerrainCharacteristics::TRAP) },
                         { "stairs", has_terrain(TerrainCharacteristics::STAIRS) },
                         { "up_stairs", has_terrain(TerrainCharacteristics::UP_STAIRS) },
                         { "down_stairs", has_terrain(TerrainCharacteristics::DOWN_STAIRS) },
                         { "entrance", has_terrain(TerrainCharacteristics::ENTRANCE) },
                         { "quest_enter", has_terrain(TerrainCharacteristics::QUEST_ENTER) },
                         { "quest_exit", has_terrain(TerrainCharacteristics::QUEST_EXIT) },
                         { "store", is_store },
                         { "can_dig", has_terrain(TerrainCharacteristics::CAN_DIG) },
                         { "has_gold", has_terrain(TerrainCharacteristics::HAS_GOLD) },
                         { "building", has_terrain(TerrainCharacteristics::BLDG) },
                     } },
    };
    // A store entrance: expose which store it is so the bot can walk to the
    // General Store (0) to shop. Emitted only on store tiles to avoid bloat.
    if (is_store) {
        result["store_number"] = enum2i(terrain.store_sale_type);
    }
    if (has_terrain(TerrainCharacteristics::ENTRANCE)) {
        result["entrance_dungeon_id"] = grid.special;
    }
    if (has_terrain(TerrainCharacteristics::QUEST_ENTER) || has_terrain(TerrainCharacteristics::QUEST_EXIT)) {
        result["quest_id"] = grid.special;
    }
    if (has_terrain(TerrainCharacteristics::BLDG)) {
        result["building_type"] = enum2i(terrain.building_type);
        result["building_special"] = grid.special;
    }
    return result;
}

nlohmann::json make_quests_json()
{
    auto result = nlohmann::json::array();
    const auto &quests = QuestList::get_instance();
    for (const auto quest_id : quests.get_sorted_quest_ids()) {
        const auto &quest = quests.get_quest(quest_id);
        result.push_back({
            { "id", enum2i(quest_id) },
            { "name", quest.name },
            { "status", enum2i(quest.status) },
            { "type", enum2i(quest.type) },
            { "level", quest.level },
            { "dungeon_id", enum2i(quest.dungeon) },
            { "r_idx", enum2i(quest.r_idx) },
            { "cur_num", quest.cur_num },
            { "max_num", quest.max_num },
            { "num_mon", quest.num_mon },
            { "flags", quest.flags },
            { "complev", quest.complev },
            { "comptime", quest.comptime },
            { "fixed", QuestType::is_fixed(quest_id) },
            { "has_reward", quest.has_reward() },
            { "reward_artifact_id", quest.get_reward().has_value() ? nlohmann::json(enum2i(*quest.get_reward())) : nlohmann::json(nullptr) },
            { "reward_baseitem_id", quest.get_reward_bi_id() },
            { "reward_instant_artifact", quest.is_reward_instant_artifact() },
        });
    }

    return result;
}

const char *monster_health_band(const MonsterEntity &monster)
{
    if (monster.hp >= monster.maxhp) {
        return "unhurt";
    }

    const auto percent = monster.maxhp > 0 ? 100L * monster.hp / monster.maxhp : 0;
    if (percent >= 60) {
        return "lightly_wounded";
    }
    if (percent >= 25) {
        return "wounded";
    }
    if (percent >= 10) {
        return "badly_wounded";
    }

    return "almost_dead";
}

nlohmann::json make_visible_monsters_json(const PlayerType &player)
{
    auto monsters = nlohmann::json::array();
    const auto &floor = *player.current_floor_ptr;
    const auto is_hallucinated = player.effects()->hallucination().is_hallucinated();
    for (MONSTER_IDX m_idx = 1; m_idx < floor.m_max; ++m_idx) {
        const auto &monster = floor.m_list[m_idx];
        if (!monster.is_valid() || !monster.ml || !floor.get_grid(monster.get_position()).is_view()) {
            continue;
        }

        // While hallucinating, the player sees SOMETHING at the tile but cannot
        // tell what it is or how hurt it is (the map shows a random symbol).
        // Emit the position-bearing index and friend/foe only; redact identity,
        // health, and status so the bot defends against an unknown threat rather
        // than reading true stats it could not perceive.
        if (is_hallucinated) {
            monsters.push_back({
                { "index", m_idx },
                { "hallucinated", true },
                { "friendly", monster.is_friendly() },
                { "pet", monster.is_pet() },
            });
            continue;
        }

        const auto &monrace = monster.get_apparent_monrace();
        monsters.push_back({
            { "index", m_idx },
            { "race_id", enum2i(monrace.idx) },
            { "name", sys_to_utf8(monrace.name.string()).value_or("<encoding-error>") },
            { "health", monster_health_band(monster) },
            { "asleep", monster.is_asleep() },
            { "stunned", monster.is_stunned() },
            { "confused", monster.is_confused() },
            { "fearful", monster.is_fearful() },
            { "friendly", monster.is_friendly() },
            { "pet", monster.is_pet() },
        });
    }

    return monsters;
}

nlohmann::json make_nearby_grids_json(const PlayerType &player)
{
    const auto &floor = *player.current_floor_ptr;
    auto grids = nlohmann::json::array();
    grids.get_ref<nlohmann::json::array_t &>().reserve(static_cast<std::size_t>(floor.height) * floor.width);
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

            grids.push_back(make_grid_json(floor, { y, x }));
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

nlohmann::json make_player_stats_json(const PlayerType &player)
{
    // Current vs maximal natural value per ability. The character sheet shows both
    // (a drained stat renders reduced / recoloured), so this reveals nothing the
    // player cannot already read on screen. drained = cur < max tells the bot which
    // stat to restore.
    auto stat = [&player](int index) {
        return nlohmann::json{
            { "cur", player.stat_cur[index] },
            { "max", player.stat_max[index] },
            { "use", player.stat_use[index] },
            { "index", player.stat_index[index] },
            { "drained", player.stat_cur[index] < player.stat_max[index] },
        };
    };
    return {
        { "str", stat(A_STR) },
        { "int", stat(A_INT) },
        { "wis", stat(A_WIS) },
        { "dex", stat(A_DEX) },
        { "con", stat(A_CON) },
        { "chr", stat(A_CHR) },
    };
}

nlohmann::json make_player_abilities_json(PlayerType *player_ptr)
{
    // Resistances / ESP / free action the character actually has, aggregated from
    // race, class, mutations and every worn item — exactly what the 'C'haracter
    // screen shows, so this reveals nothing hidden. The bot gates its dive depth on
    // these (the depth-requirement table lives in the bot's AGENTS.md). Each query
    // returns BIT_FLAGS; non-zero means the ability is present.
    return {
        { "resist_fire", has_resist_fire(player_ptr) != 0 },
        { "resist_cold", has_resist_cold(player_ptr) != 0 },
        { "resist_elec", has_resist_elec(player_ptr) != 0 },
        { "resist_acid", has_resist_acid(player_ptr) != 0 },
        { "resist_pois", has_resist_pois(player_ptr) != 0 },
        { "resist_conf", has_resist_conf(player_ptr) != 0 },
        { "resist_chaos", has_resist_chaos(player_ptr) != 0 },
        { "resist_blind", has_resist_blind(player_ptr) != 0 },
        { "resist_fear", has_resist_fear(player_ptr) != 0 },
        { "resist_neth", has_resist_neth(player_ptr) != 0 },
        { "resist_nexus", has_resist_nexus(player_ptr) != 0 },
        { "resist_sound", has_resist_sound(player_ptr) != 0 },
        { "resist_shard", has_resist_shard(player_ptr) != 0 },
        { "resist_disen", has_resist_disen(player_ptr) != 0 },
        { "resist_lite", has_resist_lite(player_ptr) != 0 },
        { "resist_dark", has_resist_dark(player_ptr) != 0 },
        { "telepathy", has_esp_telepathy(player_ptr) != 0 },
        { "free_action", has_free_act(player_ptr) != 0 },
        { "see_invisible", has_see_inv(player_ptr) != 0 },
    };
}

const char *food_state(const PlayerType &player)
{
    if (player.food < PY_FOOD_FAINT) {
        return "fainting";
    }
    if (player.food < PY_FOOD_WEAK) {
        return "weak";
    }
    if (player.food < PY_FOOD_ALERT) {
        return "hungry";
    }
    if (player.food < PY_FOOD_FULL) {
        return "normal";
    }
    if (player.food < PY_FOOD_MAX) {
        return "full";
    }

    return "gorged";
}

nlohmann::json make_item_json(PlayerType *player_ptr, const ItemEntity &item)
{
    auto result = nlohmann::json{
        { "name", sys_to_utf8(describe_flavor(player_ptr, item, OD_OMIT_PREFIX)).value_or("<encoding-error>") },
        { "count", item.number },
        { "tval", enum2i(item.bi_key.tval()) },
        { "aware", item.is_aware() },
        { "known", item.is_known() },
        { "fully_known", item.is_fully_known() },
        { "is_equipment", item.is_equipment() },
        { "weight", item.weight },
        // This matches the player's bounty knowledge: daily targets only count
        // after they have been learned, while the fixed wanted list is public.
        { "is_bounty", item.is_bounty() },
    };

    // Match what the normal item description reveals. An unaware flavor does
    // not reveal its base kind, and an unidentified instance does not reveal
    // charges, pval-derived values, or remaining fuel.
    if (item.is_aware()) {
        result["sval"] = item.bi_key.sval().value_or(-1);
    }
    if (item.has_identification_flag(IdentificationFlag::SENSE)) {
        result["pseudo_feeling"] = pseudo_feeling_name(static_cast<item_feel_type>(item.feeling));
    }
    if (item.is_known()) {
        result["charges"] = item.pval;
        result["pval"] = item.pval;
        result["fuel"] = item.fuel;
        result["timeout"] = item.timeout;
        result["is_ego"] = item.is_ego();
        result["is_artifact"] = item.is_fixed_or_random_artifact();
        result["is_cursed"] = item.is_cursed();
        result["is_broken"] = item.is_broken();
        result["to_h"] = item.to_h;
        result["to_d"] = item.to_d;
        result["to_a"] = item.to_a;
        result["ac"] = item.ac;
        result["damage_dice"] = {
            { "num", item.damage_dice.num },
            { "sides", item.damage_dice.sides },
        };
        auto known_flags = nlohmann::json::array();
        const auto flags = item.get_flags_known();
        for (auto i = 0; i < static_cast<int>(TR_FLAG_MAX); ++i) {
            if (flags.has(static_cast<tr_type>(i))) {
                known_flags.push_back(i);
            }
        }
        result["known_flags"] = std::move(known_flags);
        if (item.is_melee_weapon()) {
            const auto tval = item.bi_key.tval();
            const auto sval = item.bi_key.sval();
            const auto exp_it = player_ptr->weapon_exp.find(tval);
            if (sval && exp_it != player_ptr->weapon_exp.end()) {
                result["weapon_proficiency"] = exp_it->second[*sval];
            }
        }
    }

    return result;
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
    const auto is_personal_storage = store_num == StoreSaleType::HOME || store_num == StoreSaleType::MUSEUM;
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
            if (is_personal_storage) {
                auto entry = make_item_json(player_ptr, item);
                entry["letter"] = letter;
                items.push_back(std::move(entry));
                continue;
            }

            const auto price = price_item(player_ptr, item.calc_price(), ot_ptr->inflate, false, store_num);
            items.push_back({
                { "letter", letter },
                { "name", sys_to_utf8(describe_flavor(player_ptr, item, OD_STORE | OD_OMIT_PREFIX)).value_or("<encoding-error>") },
                { "count", item.number },
                { "tval", enum2i(item.bi_key.tval()) },
                { "sval", item.bi_key.sval().value_or(-1) },
                { "aware", true },
                { "known", true },
                { "fully_known", true },
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
    const auto &dungeons = DungeonList::get_instance();
    const auto &dungeon_records = DungeonRecords::get_instance();
    auto entered_dungeon_ids = nlohmann::json::array();
    auto conquered_dungeon_ids = nlohmann::json::array();
    for (const auto dungeon_id : dungeon_records.collect_entered_dungeon_ids()) {
        entered_dungeon_ids.push_back(enum2i(dungeon_id));
        // A conquered dungeon's final guardian is dead — its best drop is already
        // taken. The bot uses this (fair-play: the player knows which dungeons they
        // have cleared) to steer toward an UNCONQUERED dungeon it can safely clear
        // for the guardian's equipment.
        if (dungeons.get_dungeon(dungeon_id).is_conquered()) {
            conquered_dungeon_ids.push_back(enum2i(dungeon_id));
        }
    }
    return {
        { "type", "player_turn" },
        { "turn", world.game_turn },
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
                        { "food_state", food_state(player) },
                        { "speed", player.pspeed },
                        { "recalling", player.word_recall != 0 },
                        { "food_type", enum2i(PlayerRace(player_ptr).food()) },
                        { "class_id", enum2i(player.pclass) },
                        { "race_id", enum2i(player.prace) },
                        { "personality_id", enum2i(player.ppersonality) },
                        { "ac", player.dis_ac + player.dis_to_a },
                        { "skills", {
                                        { "melee", player.skill_thn },
                                        { "saving", player.skill_sav },
                                        { "device", player.skill_dev },
                                        { "stealth", player.skill_stl },
                                        { "two_weapon", player.skill_exp.at(PlayerSkillKindType::TWO_WEAPON) },
                                        { "shield", player.skill_exp.at(PlayerSkillKindType::SHIELD) },
                                    } },
                        { "melee", {
                                       { "main_hand_blows", player.num_blow[0] },
                                       { "sub_hand_blows", player.num_blow[1] },
                                       { "main_hand_to_h", player.dis_to_h[0] },
                                       { "sub_hand_to_h", player.dis_to_h[1] },
                                       { "main_hand_to_d", player.dis_to_d[0] },
                                       { "sub_hand_to_d", player.dis_to_d[1] },
                                   } },
                        { "status", make_player_status_json(player) },
                        { "stats", make_player_stats_json(player) },
                        { "abilities", make_player_abilities_json(player_ptr) },
                    } },
        { "floor", {
                       { "dungeon_id", enum2i(floor.dungeon_id) },
                       { "level", floor.dun_level },
                       { "width", floor.width },
                       { "height", floor.height },
                       { "inside_arena", floor.inside_arena },
                       { "quest_id", enum2i(floor.quest_number) },
                       { "town_id", world.is_in_any_town() ? static_cast<int>(world.get_town_index()) - 1 : -1 },
                       { "town_index", world.is_in_any_town() ? static_cast<int>(world.get_town_index()) : 0 },
                       // True only while standing ON an actual town tile. Gate on
                       // dun_level == 0: is_in_any_town() (current_town_index > 0)
                       // is NOT cleared on entering a dungeon, so on its own it
                       // reads true in the dungeon too. On the surface it still
                       // separates the town from the open, out-of-depth wilderness
                       // tile (both share dungeon_id 0 / level 0).
                       { "in_town", floor.dun_level == 0 && world.is_in_any_town() },
                   } },
        { "progress", {
                          { "recall_dungeon_id", enum2i(player.recall_dungeon) },
                          { "entered_dungeon_ids", std::move(entered_dungeon_ids) },
                          { "conquered_dungeon_ids", std::move(conquered_dungeon_ids) },
                          // Deepest level reached in the recall-target dungeon: this
                          // is exactly where Word of Recall lands and what the player
                          // sees on the character screen, so it reveals nothing hidden.
                          // The bot uses it to seed its deepest-level watermark, which
                          // otherwise resets to 1 on every restart.
                          { "recall_depth", dungeon_records.get_record(player.recall_dungeon).get_max_level() },
                          { "yeek_cave_conquered", dungeons.get_dungeon(DungeonId::GALGALS).is_conquered() },
                          // Whether the player can Word-of-Recall into Angband. A
                          // dungeon becomes recallable once its record has a max
                          // level (> 0): set either by physically entering it, or
                          // by hearing its rumor at the inn, which calls
                          // set_max_level(mindepth) and tells the player "You can
                          // recall to Angband." has_entered() alone missed the
                          // rumor-unlock case, so the bot never noticed the unlock
                          // and looped reading rumors. This is player-visible state,
                          // not hidden information.
                          { "angband_recall_unlocked", dungeon_records.get_record(DungeonId::ANGBAND).get_max_level() > 0 },
                          { "quests", make_quests_json() },
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

    // The bot only consumes the newest complete snapshot. Bound the live JSONL
    // instead of retaining an ever-growing history; the client detects the
    // shrink and resumes reading from the new first line.
    if (ofs.tellp() >= BOT_JSON_OUTPUT_MAX_BYTES) {
        ofs.close();
        ofs.clear();
        ofs.open(opened_path, std::ios::out | std::ios::trunc);
        if (!ofs) {
            return;
        }
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
