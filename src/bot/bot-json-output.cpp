#include "bot/bot-json-output.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-util.h"
#include "avatar/avatar.h"
#include "artifact/fixed-art-types.h"
#include "combat/attack-power-table.h"
#include "combat/shoot.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "game-option/runtime-arguments.h"
#include "inventory/inventory-slot-types.h"
#include "locale/character-encoding.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "player-ability/player-ability-types.h"
#include "player-base/player-race.h"
#include "player-base/player-class.h"
#include "player-info/race-info.h"
#include "player-info/equipment-info.h"
#include "player/player-realm.h"
#include "player/permanent-resistances.h"
#include "player/race-resistances.h"
#include "player/temporary-resistances.h"
#include "player/digestion-processor.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/player-status-table.h"
#include "store/pricing.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "store/store.h"
#include "spell/technic-info-table.h"
#include "sv-definition/sv-bow-types.h"
#include "system/artifact/artifact-list.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-record.h"
#include "system/baseitem/baseitem-record.h"
#include "system/baseitem/baseitem-list.h"
#include "system/baseitem/baseitem-records.h"
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
#include "system/floor/town-list.h"
#include "system/floor/town-records.h"
#include "system/grid-type-definition.h"
#include "system/item/identification-flags.h"
#include "system/item/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monrace/monrace-records.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "timed-effect/timed-effects.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include "view/status-first-page.h"
#include "world/world.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace {
constexpr std::streamoff BOT_JSON_OUTPUT_MAX_BYTES = 256LL * 1024 * 1024;
constexpr int BOT_JSON_MAX_MESSAGES = 32;

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
    auto visible_object_tvals = nlohmann::json::array();
    for (const auto o_idx : grid.o_idx_list) {
        const auto &item = *floor.o_list[o_idx];
        if (item.is_valid() && item.marked.has(OmType::FOUND)) {
            // The object-class glyph is visible even when flavor and charges are
            // unknown. MANA races use this to prioritize edible devices.
            visible_object_tvals.push_back(enum2i(item.bi_key.tval()));
        }
    }

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
        { "object_tvals", std::move(visible_object_tvals) },
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
                         { "tunnel", has_terrain(TerrainCharacteristics::TUNNEL) },
                         { "permanent", has_terrain(TerrainCharacteristics::PERMANENT) },
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

nlohmann::json make_recent_messages_json()
{
    static auto previous_message_count = int32_t{ 0 };
    static auto previous_latest_message = std::string{};
    static auto initialized = false;

    auto messages = nlohmann::json::array();
    const auto current_message_count = message_num();
    const auto latest_message = current_message_count > 0 ? *message_str(0) : std::string{};
    if (!initialized) {
        initialized = true;
        previous_message_count = current_message_count;
        previous_latest_message = latest_message;
        return messages;
    }

    auto recent_message_count = std::max<int32_t>(0, current_message_count - previous_message_count);
    if ((recent_message_count == 0) && (latest_message != previous_latest_message)) {
        recent_message_count = std::min<int32_t>(1, current_message_count);
    }

    recent_message_count = std::min<int32_t>(recent_message_count, BOT_JSON_MAX_MESSAGES);
    for (auto age = recent_message_count; age-- > 0;) {
        messages.push_back(sys_to_utf8(*message_str(age)).value_or("<encoding-error>"));
    }

    previous_message_count = current_message_count;
    previous_latest_message = latest_message;
    return messages;
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
        // Player-authored inscription (the {...} tag). The player sets and
        // sees it, so exposing it is fair-play. The bot uses it as durable,
        // savefile-persistent memory (e.g. tagging a confirmed HEAVY_CURSE
        // item so a restart does not re-attempt normal remove-curse).
        result["inscription"] = item.is_inscribed() ? *item.inscription : "";
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
        if (item.is_melee_weapon() || item.bi_key.tval() == ItemKindType::BOW) {
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
                // Shop stock is fully identified and its listing already shows
                // charges/fuel to the player, so exposing pval reveals nothing
                // hidden. Without it a MANA race can never evaluate charge food
                // (every shelf wand/staff read as 0 charges).
                { "pval", item.pval },
                { "charges", item.pval },
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
    auto dungeon_recall_depths = nlohmann::json::object();
    auto conquered_dungeon_ids = nlohmann::json::array();
    for (const auto dungeon_id : dungeon_records.collect_entered_dungeon_ids()) {
        const auto dungeon_id_value = enum2i(dungeon_id);
        const auto recall_depth = dungeon_records.get_record(dungeon_id).get_max_level();
        entered_dungeon_ids.push_back(dungeon_id_value);
        const auto dungeon_id_key = std::to_string(dungeon_id_value);
        dungeon_recall_depths.emplace(dungeon_id_key, recall_depth);
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
                                        { "shooting", player.skill_thb },
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
                          { "dungeon_recall_depths", std::move(dungeon_recall_depths) },
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
                          // Towns the player knows the way to: marked visited by
                          // physically entering them or by hearing the town's rumor
                          // at the inn ("You know the way to ..."). The inn's
                          // "Teleport to other town" menu lists exactly these towns,
                          // so this mirrors player-visible state. IDs follow the
                          // floor.town_id convention (town_index - 1).
                          { "visited_town_ids", [] {
                               std::vector<int> ids;
                               const auto &records = TownRecords::get_instance();
                               const auto towns_size = TownList::get_instance().size();
                               for (size_t i = 1; i < towns_size; i++) {
                                   if (records.has_visited(i2enum<TownId>(i - 1))) {
                                       ids.push_back(static_cast<int>(i) - 1);
                                   }
                               }
                               return ids;
                           }() },
                          { "quests", make_quests_json() },
                      } },
        { "nearby_grids", make_nearby_grids_json(player) },
        { "visible_monsters", make_visible_monsters_json(player) },
        { "messages", make_recent_messages_json() },
        { "inventory", make_inventory_json(player_ptr) },
        { "equipment", make_equipment_json(player_ptr) },
    };
}

nlohmann::json make_flag_table_json(PlayerType *player_ptr)
{
    static constexpr tr_type flags[] = {
        TR_RES_ACID, TR_RES_ELEC, TR_RES_FIRE, TR_RES_COLD, TR_RES_POIS, TR_RES_LITE, TR_RES_DARK, TR_RES_SHARDS,
        TR_RES_BLIND, TR_RES_CONF, TR_RES_SOUND, TR_RES_NETHER, TR_RES_NEXUS, TR_RES_CHAOS, TR_RES_DISEN, TR_RES_TIME,
        TR_RES_WATER, TR_RES_FEAR, TR_RES_CURSE, TR_SPEED, TR_FREE_ACT, TR_SEE_INVIS, TR_HOLD_EXP, TR_WARNING,
        TR_SLOW_DIGEST, TR_REGEN, TR_LEVITATION, TR_REFLECT, TR_SUST_STR, TR_SUST_INT, TR_SUST_WIS, TR_SUST_DEX,
        TR_SUST_CON, TR_SUST_CHR, TR_IM_ACID, TR_IM_ELEC, TR_IM_FIRE, TR_IM_COLD, TR_IM_DARK, TR_IM_LITE,
        TR_VUL_ACID, TR_VUL_ELEC, TR_VUL_FIRE, TR_VUL_COLD, TR_VUL_LITE, TR_VUL_CURSE, TR_SLAY_EVIL, TR_KILL_EVIL,
        TR_SLAY_GOOD, TR_KILL_GOOD, TR_SLAY_HUMAN, TR_KILL_HUMAN, TR_SLAY_ANIMAL, TR_KILL_ANIMAL, TR_SLAY_DRAGON,
        TR_KILL_DRAGON, TR_SLAY_ORC, TR_KILL_ORC, TR_SLAY_TROLL, TR_KILL_TROLL, TR_SLAY_GIANT, TR_KILL_GIANT,
        TR_SLAY_DEMON, TR_KILL_DEMON, TR_SLAY_UNDEAD, TR_KILL_UNDEAD, TR_BRAND_ACID, TR_BRAND_ELEC, TR_BRAND_FIRE,
        TR_BRAND_COLD, TR_BRAND_POIS, TR_BRAND_MAGIC, TR_VORPAL, TR_VAMPIRIC, TR_CHAOTIC, TR_FORCE_WEAPON,
        TR_IMPACT, TR_EARTHQUAKE, TR_BLESSED, TR_ESP_ANIMAL, TR_ESP_UNDEAD, TR_ESP_DEMON, TR_ESP_ORC, TR_ESP_TROLL,
        TR_ESP_GIANT, TR_ESP_DRAGON, TR_ESP_HUMAN, TR_ESP_EVIL, TR_ESP_GOOD, TR_ESP_NONLIVING, TR_ESP_UNIQUE,
        TR_TELEPATHY, TR_THROW, TR_BLOWS, TR_XTRA_SHOTS, TR_MAGIC_MASTERY, TR_DEC_MANA, TR_EASY_SPELL, TR_INFRA,
        TR_STEALTH, TR_SEARCH, TR_TUNNEL, TR_ACTIVATE, TR_RIDING, TR_LITE_1, TR_AGGRAVATE, TR_TY_CURSE,
        TR_ADD_L_CURSE, TR_ADD_H_CURSE, TR_PERSISTENT_CURSE, TR_DRAIN_EXP, TR_DRAIN_HP, TR_DRAIN_MANA, TR_FAST_DIGEST,
        TR_SLOW_REGEN, TR_COWARDICE, TR_LOW_MELEE, TR_LOW_AC, TR_HARD_SPELL, TR_NO_TELE, TR_NO_MAGIC, TR_BERS_RAGE,
        TR_SH_FIRE, TR_SH_ELEC, TR_SH_COLD, TR_SELF_FIRE, TR_SELF_ELEC, TR_SELF_COLD, TR_INVULN_ARROW, TR_SUPPORTIVE,
        TR_DOWN_SAVING,
    };
    TrFlags permanent;
    TrFlags temporary;
    TrFlags immunity;
    TrFlags temporary_immunity;
    TrFlags vulnerability;
    player_flags(player_ptr, permanent);
    tim_player_flags(player_ptr, temporary);
    player_immunity(player_ptr, immunity);
    tim_player_immunity(player_ptr, temporary_immunity);
    player_vulnerability_flags(player_ptr, vulnerability);

    auto rows = nlohmann::json::array();
    for (const auto flag : flags) {
        auto equipment = nlohmann::json::array();
        for (const auto slot : INVEN_WIELDING_SLOTS) {
            equipment.push_back({
                { "slot", equipment_slot_label(slot) },
                { "has", player_ptr->inventory[slot]->get_flags_known().has(flag) },
            });
        }
        rows.push_back({
            { "flag_id", enum2i(flag) },
            { "equipment", std::move(equipment) },
            { "player", permanent.has(flag) },
            { "temporary", temporary.has(flag) },
            { "immunity", immunity.has(flag) },
            { "temporary_immunity", temporary_immunity.has(flag) },
            { "vulnerability", vulnerability.has(flag) },
        });
    }
    return rows;
}

nlohmann::json make_character_json(PlayerType *player_ptr)
{
    auto &player = *player_ptr;
    int damage[2]{};
    int to_h[2]{};
    calc_player_two_hands(player_ptr, damage, to_h);
    int shots = 0;
    int shot_frac = 0;
    const auto &bow = *player.inventory[INVEN_BOW];
    calc_player_shot_params(player_ptr, player.inventory[INVEN_BOW].get(), &shots, &shot_frac);
    auto stat_details = nlohmann::json::array();
    for (auto i = 0; i < A_MAX; ++i) {
        stat_details.push_back({
            { "stat_id", i },
            { "top", player.stat_top[i] },
            { "maximum", player.stat_max_max[i] },
            { "at_maximum", player.stat_max[i] == player.stat_max_max[i] },
        });
    }
    auto mutations = nlohmann::json::array();
    for (auto i = 0; i < enum2i(PlayerMutationType::MAX); ++i) {
        if (player.muta.has(i2enum<PlayerMutationType>(i))) {
            mutations.push_back(i);
        }
    }
    PlayerRealm realms(player_ptr);
    auto realm_json = nlohmann::json::array();
    for (const auto &realm : { realms.realm1(), realms.realm2() }) {
        if (realm.is_available()) {
            realm_json.push_back({ { "id", enum2i(realm.to_enum()) }, { "name", sys_to_utf8(realm.get_name().string()).value_or("<encoding-error>") } });
        }
    }
    auto shooting_multiplier = 0;
    if (bow.is_valid()) {
        shooting_multiplier = (bow.get_arrow_magnification() + (player.xtra_might ? 1 : 0)) * (100 + static_cast<int>(adj_str_td[player.stat_index[A_STR]]) - 128);
    }
    return {
        { "skills", {
                        { "thn", player.skill_thn }, { "thb", player.skill_thb }, { "sav", player.skill_sav },
                        { "dev", player.skill_dev }, { "stl", player.skill_stl }, { "dis", player.skill_dis },
                        { "srh", player.skill_srh }, { "fos", player.skill_fos }, { "dig", player.skill_dig },
                    } },
        { "ranged", {
                         { "to_h_b", player.to_h_b }, { "shots", shots }, { "shot_frac", shot_frac },
                         { "shooting_multiplier", shooting_multiplier }, { "see_infra", player.see_infra },
                     } },
        { "melee", {
                        { "expected_damage_x100", { damage[0], damage[1] } },
                        { "expected_damage_per_round_x100", { player.num_blow[0] * damage[0], player.num_blow[1] * damage[1] } },
                        { "bare_hand", !has_melee_weapon(player_ptr, INVEN_MAIN_HAND) && !has_melee_weapon(player_ptr, INVEN_SUB_HAND) },
                        { "two_handed", has_two_handed_weapons(player_ptr) },
                        { "monk_stance", enum2i(PlayerClass(player_ptr).get_monk_stance()) },
                    } },
        { "stats", std::move(stat_details) },
        { "age", player.age }, { "height", player.ht }, { "weight", player.wt }, { "social_class", player.sc },
        { "alignment", player.alignment }, { "realms", std::move(realm_json) }, { "chaos_patron", player.chaos_patron },
        { "mutations", std::move(mutations) }, { "characteristics", make_flag_table_json(player_ptr) },
    };
}

nlohmann::json make_artifacts_knowledge_json(PlayerType *player_ptr, bool identified)
{
    auto rows = nlohmann::json::array();
    auto ids = identified ? ArtifactRecords::get_instance().collect_identified_ids() : ArtifactRecords::get_instance().collect_known_ids();
    const auto &artifacts = ArtifactList::get_instance();
    for (const auto id : ids) {
        const auto &artifact = artifacts.get_artifact(id);
        ItemEntity item(artifact.bi_key);
        item.fa_id = id;
        item.set_identification_flag(IdentificationFlag::STORE);
        rows.push_back({ { "id", enum2i(id) }, { "name", make_item_json(player_ptr, item)["name"] } });
    }
    return rows;
}

nlohmann::json make_knowledge_json(PlayerType *player_ptr, BotKnowledgeCategory category)
{
    static constexpr const char *slugs[] = {
        "artifacts_known", "artifacts_identified", "objects_known", "uniques_alive", "uniques_dead", "bounty", "home",
        "equip_resistances", "features", "self_info", "mutations", "weapon_exp", "spell_exp", "skill_exp", "virtues",
        "dungeons", "quests", "pets", "autopick",
    };
    static constexpr const char keys[] = { '1', '2', '3', '4', '5', '8', '9', '0', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k' };
    static_assert(sizeof(slugs) / sizeof(slugs[0]) == enum2i(BotKnowledgeCategory::MAX));
    static_assert(sizeof(keys) / sizeof(keys[0]) == enum2i(BotKnowledgeCategory::MAX));
    const auto index = enum2i(category);
    auto result = nlohmann::json{ { "category", slugs[index] }, { "menu_key", std::string(1, keys[index]) } };
    auto rows = nlohmann::json::array();
    switch (category) {
    case BotKnowledgeCategory::ARTIFACTS_KNOWN:
    case BotKnowledgeCategory::ARTIFACTS_IDENTIFIED:
        result["artifacts"] = make_artifacts_knowledge_json(player_ptr, category == BotKnowledgeCategory::ARTIFACTS_IDENTIFIED);
        break;
    case BotKnowledgeCategory::OBJECTS_KNOWN:
        for (const auto id : BaseitemList::get_instance().collect_valid_bi_ids()) {
            const auto &record = BaseitemRecords::get_instance().get_record(id);
            const auto &baseitem = BaseitemList::get_instance().get_baseitem(id);
            const auto has_allocation = std::any_of(baseitem.alloc_tables.begin(), baseitem.alloc_tables.end(), [](const auto &table) { return table.chance > 0; });
            if (!record.is_apparent() || !record.is_aware() || !has_allocation) {
                continue;
            }
            rows.push_back({ { "id", id }, { "tval", enum2i(baseitem.bi_key.tval()) }, { "sval", baseitem.bi_key.sval().value_or(-1) },
                { "name", sys_to_utf8(baseitem.stripped_name()).value_or("<encoding-error>") } });
        }
        result["objects"] = std::move(rows);
        break;
    case BotKnowledgeCategory::UNIQUES_ALIVE:
    case BotKnowledgeCategory::UNIQUES_DEAD: {
        const auto alive = category == BotKnowledgeCategory::UNIQUES_ALIVE;
        const auto &records = MonraceRecords::get_instance();
        for (const auto &[id, monrace] : MonraceList::get_instance()) {
            if (!records.has_been_seen(id) || !monrace->is_valid() || !monrace->should_display(alive)) {
                continue;
            }
            rows.push_back({ { "id", enum2i(id) }, { "name", sys_to_utf8(monrace->name.string()).value_or("<encoding-error>") }, { "level", monrace->level } });
        }
        result["uniques"] = std::move(rows);
        break;
    }
    case BotKnowledgeCategory::BOUNTY: {
        const auto &world = AngbandWorld::get_instance();
        result["today"] = world.knows_daily_bounty
            ? nlohmann::json{ { "id", enum2i(world.today_mon) }, { "name", sys_to_utf8(world.get_today_bounty().name.string()).value_or("<encoding-error>") } }
            : nlohmann::json(nullptr);
        for (const auto &[id, achieved] : world.bounties) {
            if (!achieved) {
                rows.push_back({ { "id", enum2i(id) }, { "name", sys_to_utf8(MonraceList::get_instance().get_monrace(id).name.string()).value_or("<encoding-error>") } });
            }
        }
        result["wanted"] = std::move(rows);
        break;
    }
    case BotKnowledgeCategory::HOME: {
        const auto &store = TownList::get_instance().get_town(1).get_store(StoreSaleType::HOME);
        for (auto i = 0; i < store.stock_num; ++i) {
            auto item = make_item_json(player_ptr, *store.stock[i]);
            item["slot"] = i;
            rows.push_back(std::move(item));
        }
        result["items"] = std::move(rows);
        break;
    }
    case BotKnowledgeCategory::EQUIP_RESISTANCES:
    {
        const auto add_item = [&](const ItemEntity &item, std::string_view source, int slot) {
            if (!item.is_valid() || !item.is_fully_known() || !item.is_equipment()) {
                return;
            }
            auto row = make_item_json(player_ptr, item);
            row["source"] = source;
            row["slot"] = slot;
            rows.push_back(std::move(row));
        };
        for (const auto slot : INVEN_WIELDING_SLOTS) {
            add_item(*player_ptr->inventory[slot], "equipment", slot);
        }
        for (const auto slot : INVEN_PACK_SLOTS) {
            add_item(*player_ptr->inventory[slot], "inventory", slot);
        }
        const auto &home = TownList::get_instance().get_town(1).get_store(StoreSaleType::HOME);
        for (auto slot = 0; slot < home.stock_num; ++slot) {
            add_item(*home.stock[slot], "home", slot);
        }
        result["equipment"] = std::move(rows);
        break;
    }
    case BotKnowledgeCategory::FEATURES:
        for (const auto &terrain : TerrainList::get_instance()) {
            if (!terrain.name.empty() && terrain.mimic == terrain.idx) {
                rows.push_back({ { "id", terrain.idx }, { "name", sys_to_utf8(terrain.name).value_or("<encoding-error>") } });
            }
        }
        result["features"] = std::move(rows);
        break;
    case BotKnowledgeCategory::SELF_INFO:
        result["life_rating"] = (player_ptr->knowledge & KNOW_HPRATE) ? nlohmann::json(player_ptr->calc_life_rating()) : nlohmann::json(nullptr);
        for (auto i = 0; i < A_MAX; ++i) {
            rows.push_back({ { "stat_id", i }, { "maximum", ((player_ptr->knowledge & KNOW_STAT) || player_ptr->stat_max[i] == player_ptr->stat_max_max[i])
                                                                  ? nlohmann::json(player_ptr->stat_max_max[i])
                                                                  : nlohmann::json(nullptr) } });
        }
        result["stat_limits"] = std::move(rows);
        break;
    case BotKnowledgeCategory::MUTATIONS:
        for (auto i = 0; i < enum2i(PlayerMutationType::MAX); ++i) {
            if (player_ptr->muta.has(i2enum<PlayerMutationType>(i))) rows.push_back(i);
        }
        result["mutation_ids"] = std::move(rows);
        break;
    case BotKnowledgeCategory::WEAPON_EXP:
        for (const auto &baseitem : BaseitemList::get_instance()) {
            const auto tval = baseitem.bi_key.tval();
            const auto sval = baseitem.bi_key.sval();
            const auto displayed_tval = tval == ItemKindType::SWORD || tval == ItemKindType::POLEARM || tval == ItemKindType::HAFTED
                || tval == ItemKindType::DIGGING || tval == ItemKindType::BOW;
            const auto excluded_bow = tval == ItemKindType::BOW && sval && (*sval == SV_CRIMSON || *sval == SV_HARP);
            if (displayed_tval && !excluded_bow && sval && player_ptr->weapon_exp.contains(tval)) {
                rows.push_back({ { "tval", enum2i(tval) }, { "sval", *sval }, { "name", sys_to_utf8(baseitem.stripped_name()).value_or("<encoding-error>") },
                    { "exp", player_ptr->weapon_exp[tval][*sval] }, { "max", player_ptr->weapon_exp_max[tval][*sval] },
                    { "rank", enum2i(PlayerSkill::weapon_skill_rank(player_ptr->weapon_exp[tval][*sval])) } });
            }
        }
        result["weapons"] = std::move(rows);
        break;
    case BotKnowledgeCategory::SKILL_EXP:
        for (const auto skill : PLAYER_SKILL_KIND_TYPE_RANGE) {
            rows.push_back({ { "id", enum2i(skill) }, { "name", sys_to_utf8(PlayerSkill::skill_name(skill)).value_or("<encoding-error>") },
                { "exp", player_ptr->skill_exp[skill] } });
        }
        result["skills"] = std::move(rows);
        break;
    case BotKnowledgeCategory::VIRTUES:
        for (auto i = 0; i < 8; ++i) {
            rows.push_back({ { "id", enum2i(player_ptr->vir_types[i]) }, { "name", sys_to_utf8(virtue_names.at(player_ptr->vir_types[i])).value_or("<encoding-error>") },
                { "value", player_ptr->virtues[i] } });
        }
        result["virtues"] = std::move(rows);
        break;
    case BotKnowledgeCategory::QUESTS:
        for (const auto &[id, quest] : QuestList::get_instance()) {
            auto displayed = quest.status == QuestStatusType::TAKEN || quest.status == QuestStatusType::COMPLETED
                || quest.status == QuestStatusType::FINISHED || quest.status == QuestStatusType::FAILED
                || quest.status == QuestStatusType::FAILED_DONE;
            displayed |= quest.status == QuestStatusType::STAGE_COMPLETED && quest.type == QuestKindType::TOWER;
            if (id == QuestId::NONE || !displayed || (quest.flags & QUEST_FLAG_SILENT)) {
                continue;
            }
            rows.push_back({ { "id", enum2i(id) }, { "name", sys_to_utf8(quest.name).value_or("<encoding-error>") },
                { "status", enum2i(quest.status) }, { "type", enum2i(quest.type) }, { "level", quest.level },
                { "cur_num", quest.cur_num }, { "max_num", quest.max_num }, { "complev", quest.complev }, { "comptime", quest.comptime } });
        }
        result["quests"] = std::move(rows);
        break;
    case BotKnowledgeCategory::PETS:
        for (auto i = 1; i < player_ptr->current_floor_ptr->m_max; ++i) {
            const auto &monster = player_ptr->current_floor_ptr->m_list[i];
            if (monster.is_valid() && monster.is_pet()) {
                rows.push_back({ { "index", i }, { "race_id", enum2i(monster.get_monrace_id()) },
                    { "name", sys_to_utf8(monster.get_monrace().name.string()).value_or("<encoding-error>") } });
            }
        }
        result["pets"] = std::move(rows);
        break;
    case BotKnowledgeCategory::AUTOPICK:
        for (const auto &entry : autopick_list) {
            rows.push_back({ { "rule", sys_to_utf8(autopick_line_from_entry(entry)).value_or("<encoding-error>") } });
        }
        result["rules"] = std::move(rows);
        break;
    case BotKnowledgeCategory::SPELL_EXP:
    {
        PlayerRealm realms(player_ptr);
        auto offset = 0;
        for (const auto &realm : { realms.realm1(), realms.realm2() }) {
            if (!realm.is_available()) {
                offset += 32;
                continue;
            }
            for (auto spell_id = 0; spell_id < 32; ++spell_id) {
                const auto &spell = realm.get_spell_info(spell_id);
                if (spell.slevel >= 99) {
                    continue;
                }
                const auto exp = player_ptr->spell_exp[offset + spell_id];
                rows.push_back({ { "realm_id", enum2i(realm.to_enum()) }, { "spell_id", spell_id },
                    { "name", sys_to_utf8(realm.get_spell_name(spell_id)).value_or("<encoding-error>") },
                    { "exp", exp }, { "rank", enum2i(PlayerSkill::spell_skill_rank(exp)) } });
            }
            offset += 32;
        }
        result["spells"] = std::move(rows);
        break;
    }
    case BotKnowledgeCategory::DUNGEONS:
        for (const auto &[dungeon_id, dungeon] : DungeonList::get_instance()) {
            const auto &record = DungeonRecords::get_instance().get_record(dungeon_id);
            if (record.has_entered()) {
                rows.push_back({ { "id", enum2i(dungeon_id) }, { "name", sys_to_utf8(dungeon->name).value_or("<encoding-error>") },
                    { "max_level", record.get_max_level() }, { "conquered", dungeon->is_conquered() } });
            }
        }
        result["dungeons"] = std::move(rows);
        break;
    case BotKnowledgeCategory::MAX:
        break;
    }
    return result;
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

void output_bot_json_character_snapshot(PlayerType *player_ptr)
{
    if (!arg_bot_json_output || player_ptr == nullptr || player_ptr->current_floor_ptr == nullptr) {
        return;
    }
    auto snapshot = make_snapshot(player_ptr);
    snapshot["type"] = "character";
    snapshot["character"] = make_character_json(player_ptr);
    write_snapshot(snapshot);
}

void output_bot_json_knowledge_snapshot(PlayerType *player_ptr, BotKnowledgeCategory category)
{
    if (!arg_bot_json_output || player_ptr == nullptr || player_ptr->current_floor_ptr == nullptr) {
        return;
    }
    auto snapshot = make_snapshot(player_ptr);
    snapshot["type"] = "knowledge";
    snapshot["knowledge"] = make_knowledge_json(player_ptr, category);
    write_snapshot(snapshot);
}
