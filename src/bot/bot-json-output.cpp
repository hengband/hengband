#include "bot/bot-json-output.h"
#include "artifact/fixed-art-types.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-util.h"
#include "avatar/avatar.h"
#include "combat/attack-power-table.h"
#include "combat/shoot.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "game-option/runtime-arguments.h"
#include "inventory/inventory-slot-types.h"
#include "locale/character-encoding.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "player-ability/player-ability-types.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/equipment-info.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player/digestion-processor.h"
#include "player/permanent-resistances.h"
#include "player/player-realm.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/player-status-table.h"
#include "player/race-resistances.h"
#include "player/temporary-resistances.h"
#include "spell/technic-info-table.h"
#include "store/pricing.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "store/store.h"
#include "sv-definition/sv-bow-types.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/artifact/artifact-record.h"
#include "system/baseitem/baseitem-key.h"
#include "system/baseitem/baseitem-list.h"
#include "system/baseitem/baseitem-record.h"
#include "system/baseitem/baseitem-records.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/dungeon/dungeon-record.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/store-sale-type.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/enums/terrain/terrain-kind.h"
#include "system/enums/terrain/terrain-tag.h"
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
#include "target/target-preparation.h"
#include "target/target-types.h"
#include "timed-effect/timed-effects.h"
#include "util/enum-converter.h"
#include "view/display-map.h"
#include "view/display-messages.h"
#include "view/status-first-page.h"
#include "window/main-window-util.h"
#include "world/world.h"
#include <algorithm>
#include <array>
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

bool is_grid_perceivable(const PlayerType &player, const Pos2D &pos)
{
    const auto &floor = *player.current_floor_ptr;
    const auto &grid = floor.get_grid(pos);
    if (grid.get_terrain(TerrainKind::MIMIC).flags.has(TerrainCharacteristics::REMEMBER)) {
        return grid.is_mark() && is_revealed_wall(floor, pos);
    }

    if (player.effects()->blindness().is_blind()) {
        return false;
    }

    const auto is_visible = (grid.info & (CAVE_MARK | CAVE_LITE | CAVE_MNLT)) != 0;
    const auto is_glowing = (grid.info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW;
    return is_visible || (grid.is_view() && (is_glowing || player.see_nocto != 0));
}

nlohmann::json make_grid_json(const PlayerType &player, const Pos2D &pos)
{
    const auto &floor = *player.current_floor_ptr;
    const auto &grid = floor.get_grid(pos);
    const auto is_known = is_grid_perceivable(player, pos);
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

    short visible_monster_index = 0;
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
    if (!player.effects()->hallucination().is_hallucinated()) {
        for (const auto o_idx : grid.o_idx_list) {
            const auto &item = *floor.o_list[o_idx];
            if (item.is_valid() && item.marked.has(OmType::FOUND)) {
                // The object-class glyph is visible even when flavor and charges are
                // unknown. MANA races use this to prioritize edible devices.
                visible_object_tvals.push_back(enum2i(item.bi_key.tval()));
            }
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

nlohmann::json make_disclosed_quests_json()
{
    auto result = nlohmann::json::array();
    const auto &quests = QuestList::get_instance();
    auto append_quest = [&result, &quests](QuestId quest_id) {
        const auto &quest = quests.get_quest(quest_id);
        const auto is_completed = quest.status == QuestStatusType::FINISHED;
        const auto is_failed = quest.status == QuestStatusType::FAILED || quest.status == QuestStatusType::FAILED_DONE;
        const auto displayed_name = (is_completed || is_failed) && quest.type == QuestKindType::RANDOM && quest.get_bounty().is_valid()
                                        ? quest.get_bounty().name.string()
                                        : quest.name;
        auto row = nlohmann::json{
            { "id", enum2i(quest_id) },
            { "name", to_json_utf8(displayed_name) },
            { "status", enum2i(quest.status) },
            { "type", enum2i(quest.type) },
            { "level", quest.level },
            { "fixed", QuestType::is_fixed(quest_id) },
        };
        if (is_completed || is_failed) {
            row["complev"] = quest.complev;
            row["comptime"] = quest.comptime;
            if (quest.type == QuestKindType::RANDOM && quest.get_bounty().is_valid()) {
                row["r_idx"] = enum2i(quest.r_idx);
            }
        } else if (quest.type == QuestKindType::RANDOM || quest.type == QuestKindType::KILL_LEVEL) {
            row["r_idx"] = enum2i(quest.r_idx);
            if (quest.type == QuestKindType::KILL_LEVEL && quest.max_num > 1) {
                row["cur_num"] = quest.cur_num;
                row["max_num"] = quest.max_num;
            }
        } else if (quest.type == QuestKindType::KILL_NUMBER) {
            row["cur_num"] = quest.cur_num;
            row["max_num"] = quest.max_num;
        } else if (quest.type == QuestKindType::FIND_ARTIFACT && quest.status == QuestStatusType::TAKEN && quest.has_reward()) {
            row["reward_artifact_id"] = quest.get_reward() ? nlohmann::json(enum2i(*quest.get_reward())) : nlohmann::json(nullptr);
            row["reward_baseitem_id"] = quest.get_reward_bi_id();
        }
        result.push_back(std::move(row));
    };

    const auto disclosed_random_quest_id = quests.find_shallowest_random_quest_id();
    for (const auto quest_id : quests.get_sorted_quest_ids()) {
        const auto &quest = quests.get_quest(quest_id);
        if (quest_id == QuestId::NONE || (quest.flags & QUEST_FLAG_SILENT)) {
            continue;
        }

        const auto is_current = quest.status == QuestStatusType::TAKEN || quest.status == QuestStatusType::COMPLETED || (quest.status == QuestStatusType::STAGE_COMPLETED && quest.type == QuestKindType::TOWER);
        const auto is_completed = quest.status == QuestStatusType::FINISHED;
        const auto is_failed = quest.status == QuestStatusType::FAILED || quest.status == QuestStatusType::FAILED_DONE;
        if (!is_current && !is_completed && !is_failed) {
            continue;
        }

        // 遂行中のランダムクエストは画面と同じく最も浅い1件だけを開示する
        if (is_current && quest.type == QuestKindType::RANDOM) {
            continue;
        }

        append_quest(quest_id);
    }

    if (disclosed_random_quest_id) {
        append_quest(*disclosed_random_quest_id);
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

nlohmann::json make_visible_monster_json(short m_idx, const MonsterEntity &monster, bool is_hallucinated)
{
    // While hallucinating, the player sees SOMETHING at the tile but cannot
    // tell what it is or how hurt it is (the map shows a random symbol).
    // Emit the position-bearing index and friend/foe only; redact identity,
    // health, and status so the bot defends against an unknown threat rather
    // than reading true stats it could not perceive.
    if (is_hallucinated) {
        return {
            { "index", m_idx },
            { "hallucinated", true },
            { "pet", monster.is_pet() },
        };
    }

    const auto &monrace = monster.get_apparent_monrace();
    return {
        { "index", m_idx },
        { "race_id", enum2i(monrace.idx) },
        { "name", to_json_utf8(monrace.name.string()) },
        { "health", monster_health_band(monster) },
        { "asleep", monster.is_asleep() },
        { "stunned", monster.is_stunned() },
        { "confused", monster.is_confused() },
        { "fearful", monster.is_fearful() },
        { "friendly", monster.is_friendly() },
        { "pet", monster.is_pet() },
    };
}

nlohmann::json make_visible_monsters_json(const PlayerType &player)
{
    auto monsters = nlohmann::json::array();
    const auto &floor = *player.current_floor_ptr;
    const auto is_hallucinated = player.effects()->hallucination().is_hallucinated();
    for (short m_idx = 1; m_idx < floor.m_max; ++m_idx) {
        const auto &monster = floor.m_list[m_idx];
        // The always-on visible list is deliberately direct-sight-only. ESP and
        // detection perceptions belong in detected_monsters, while look follows
        // the command's broader ml-only gate; do not unify these three gates.
        if (!monster.is_valid() || !monster.ml || !floor.get_grid(monster.get_position()).is_view()) {
            continue;
        }

        monsters.push_back(make_visible_monster_json(m_idx, monster, is_hallucinated));
    }

    return monsters;
}

nlohmann::json make_detected_monsters_json(const PlayerType &player)
{
    auto monsters = nlohmann::json::array();
    const auto &floor = *player.current_floor_ptr;
    const auto is_hallucinated = player.effects()->hallucination().is_hallucinated();
    for (short m_idx = 1; m_idx < floor.m_max; ++m_idx) {
        const auto &monster = floor.m_list[m_idx];
        // This list is deliberately the ml-but-not-direct-sight partition. The
        // sight-only visible list and the look command's ml-only record have
        // different consumers; do not unify these three gates.
        if (!monster.is_valid() || !monster.ml) {
            continue;
        }

        const auto &pos = monster.get_position();
        if (floor.get_grid(pos).is_view()) {
            continue;
        }

        auto monster_json = make_visible_monster_json(m_idx, monster, is_hallucinated);
        monster_json["y"] = pos.y;
        monster_json["x"] = pos.x;
        monster_json["esp"] = monster.mflag.has(MonsterTemporaryFlagType::ESP);
        monster_json["detected"] = monster.mflag2.has(MonsterConstantFlagType::MARK);
        monsters.push_back(std::move(monster_json));
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
    for (const auto &pos : floor.get_area()) {
        if (!is_grid_perceivable(player, pos)) {
            continue;
        }

        grids.push_back(make_grid_json(player, pos));
    }

    return grids;
}

nlohmann::json make_recent_messages_json()
{
    static auto previous_message_count = 0;
    static std::string previous_latest_message = "";
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

    auto recent_message_count = std::max(0, current_message_count - previous_message_count);
    if ((recent_message_count == 0) && (latest_message != previous_latest_message)) {
        recent_message_count = std::min(1, current_message_count);
    }

    recent_message_count = std::min(recent_message_count, BOT_JSON_MAX_MESSAGES);
    messages = make_message_history_json(recent_message_count);

    previous_message_count = current_message_count;
    previous_latest_message = latest_message;
    return messages;
}

nlohmann::json make_player_status_json(const TimedEffects &effects)
{
    return {
        { "blind", effects.blindness().is_blind() },
        { "confused", effects.confusion().is_confused() },
        { "afraid", effects.fear().is_fearful() },
        { "poisoned", effects.poison().is_poisoned() },
        { "stunned", effects.stun().is_stunned() },
        { "cut", effects.cut().is_cut() },
        { "paralyzed", effects.paralysis().is_paralyzed() },
        { "hallucinated", effects.hallucination().is_hallucinated() },
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

struct PlayerKnownFlags {
    TrFlags equipment;
    TrFlags permanent;
    TrFlags temporary;
};

PlayerKnownFlags collect_player_known_flags(PlayerType *player_ptr)
{
    PlayerKnownFlags result;
    for (const auto slot : INVEN_WIELDING_SLOTS) {
        result.equipment.set(player_ptr->inventory[slot]->get_flags_known());
    }
    player_flags(player_ptr, result.permanent);
    tim_player_flags(player_ptr, result.temporary);
    return result;
}

/*!
 * @brief 1つの能力について、供給源ごとの有無を出力する
 * @details 装備・恒久（種族/職業/変異/性格）・一時（時限効果と構え）は別々に判定する。
 * まとめて1つの真偽値にすると「装備で得ている耐性」と「もうすぐ切れる一時耐性」が
 * 区別できず、一時耐性が切れた瞬間に無防備になる装備選択を防げないため。
 * キャラクター画面も同じ3系統を別の列として表示しているので、フェアプレイ上の
 * 開示範囲は変わらない。
 *
 * permanent は「このスナップショット時点の種族・職業・変異・性格から出ている」という
 * 意味であって、失効しない保証ではない。変身（tim_mimic）中は player_flags() が変身先の
 * 種族を参照するため、悪魔変身の火耐性のような時限の能力も permanent に載る。
 * キャラクター画面も同じく種族欄が変身先の名前に変わるだけなので表示との乖離はないが、
 * スナップショットは変身状態そのものを出していないので、消費側は permanent を
 * 「切れない」と読んではならない。
 */
nlohmann::json make_ability_sources_json(const PlayerKnownFlags &flags, tr_type flag, tr_type greater_flag = TR_FLAG_MAX)
{
    const auto has = [flag, greater_flag](const TrFlags &source) {
        return source.has(flag) || ((greater_flag != TR_FLAG_MAX) && source.has(greater_flag));
    };
    return {
        { "equipment", has(flags.equipment) },
        { "permanent", has(flags.permanent) },
        { "temporary", has(flags.temporary) },
    };
}

/*!
 * @brief 能力の有無を供給源ごとに出力する
 * @details 出力キーは ability_sources であって abilities ではない。#5498 の abilities は
 * 1能力 1真偽値だったので、同じキーのまま値をオブジェクトに変えると、供給源が全て false
 * でも空でないオブジェクトは Python でも JavaScript でも真と評価され、消費側は例外も出さずに
 * 「全能力あり」と誤読する。キーを分けておけば、旧来の消費側は KeyError / undefined で
 * 即座に破綻する。
 */
nlohmann::json make_player_ability_sources_json(const PlayerKnownFlags &flags)
{
    // Aggregate only identified equipment flags plus intrinsic and temporary
    // flags. A known immunity upgrades its resistance on the character screen.
    const auto sources = [&flags](tr_type flag, tr_type greater_flag = TR_FLAG_MAX) {
        return make_ability_sources_json(flags, flag, greater_flag);
    };
    return {
        { "resist_fire", sources(TR_RES_FIRE, TR_IM_FIRE) },
        { "resist_cold", sources(TR_RES_COLD, TR_IM_COLD) },
        { "resist_elec", sources(TR_RES_ELEC, TR_IM_ELEC) },
        { "resist_acid", sources(TR_RES_ACID, TR_IM_ACID) },
        { "resist_pois", sources(TR_RES_POIS) },
        { "resist_conf", sources(TR_RES_CONF) },
        { "resist_chaos", sources(TR_RES_CHAOS) },
        { "resist_blind", sources(TR_RES_BLIND) },
        { "resist_fear", sources(TR_RES_FEAR) },
        { "resist_neth", sources(TR_RES_NETHER) },
        { "resist_nexus", sources(TR_RES_NEXUS) },
        { "resist_sound", sources(TR_RES_SOUND) },
        { "resist_shard", sources(TR_RES_SHARDS) },
        { "resist_disen", sources(TR_RES_DISEN) },
        { "resist_lite", sources(TR_RES_LITE, TR_IM_LITE) },
        { "resist_dark", sources(TR_RES_DARK, TR_IM_DARK) },
        { "telepathy", sources(TR_TELEPATHY) },
        { "free_action", sources(TR_FREE_ACT) },
        { "see_invisible", sources(TR_SEE_INVIS) },
    };
}

/*!
 * @brief 免疫（ダメージ0）を供給源ごとに出力する
 * @details 耐性とは軽減率が別物（耐性は 1/3、免疫は 0）で、ability_sources では免疫を耐性に
 * 畳み込んでしまうため、どの属性を無傷で受けられるかを別に出す。装備の免疫はキャラクター
 * 画面の免疫欄、一時的な元素免疫はステータスバー（BAR_IMMFIRE 等）でプレイヤーも常時
 * 確認できる。
 */
nlohmann::json make_player_immunities_json(PlayerType *player_ptr, const PlayerKnownFlags &flags)
{
    const auto sources = [&flags](tr_type flag) {
        return nlohmann::json{
            { "equipment", flags.equipment.has(flag) },
            { "permanent", flags.permanent.has(flag) },
            { "temporary", flags.temporary.has(flag) },
        };
    };
    // 地獄だけは TR_IM_* が存在しない。スペクターの地獄無効は effect_player_nether() に
    // 種族判定として直書きされている（被害0のうえ 1/4 を回復する）ので種族から直接出す。
    auto nether = nlohmann::json{
        { "equipment", false },
        { "permanent", PlayerRace(player_ptr).equals(PlayerRaceType::SPECTRE) },
        { "temporary", false },
    };
    return {
        { "fire", sources(TR_IM_FIRE) },
        { "cold", sources(TR_IM_COLD) },
        { "elec", sources(TR_IM_ELEC) },
        { "acid", sources(TR_IM_ACID) },
        { "lite", sources(TR_IM_LITE) },
        { "dark", sources(TR_IM_DARK) },
        { "nether", std::move(nether) },
    };
}

/*!
 * @brief 弱点（被ダメージ増加）を供給源ごとに出力する
 * @details 出力の基準はキャラクター画面の表示であって、ダメージ計算式そのものではない。
 * 酸・電撃・火炎・冷気は calc_*_damage_rate() が has_vuln_*() の要因ビットを1つずつ見て
 * 変異由来なら ×2、それ以外は ×4/3 を積算するので、供給源はほぼそのまま倍率に対応する。
 * 閃光だけは calc_lite_damage_rate() が種族の TR_VUL_LITE しか見ておらず、装備由来の
 * TR_VUL_LITE はキャラクター画面には "v" として出るのにダメージには乗らない。
 * lite.equipment はその表示に合わせてあるので、消費側は倍率と同一視してはならない。
 */
nlohmann::json make_player_vulnerabilities_json(PlayerType *player_ptr, const PlayerKnownFlags &flags)
{
    // 変異「元素に弱い」は player_flags() が拾わない（has_vuln_*() が FLAG_CAUSE_MUTATION
    // として別に立てている）ため、恒久側へ明示的に足す。
    const auto has_element_mutation = player_ptr->muta.has(PlayerMutationType::VULN_ELEM);
    const auto sources = [&flags, has_element_mutation](tr_type flag, bool elemental) {
        return nlohmann::json{
            { "equipment", flags.equipment.has(flag) },
            { "permanent", flags.permanent.has(flag) || (elemental && has_element_mutation) },
            { "temporary", flags.temporary.has(flag) },
        };
    };
    return {
        { "acid", sources(TR_VUL_ACID, true) },
        { "elec", sources(TR_VUL_ELEC, true) },
        { "fire", sources(TR_VUL_FIRE, true) },
        { "cold", sources(TR_VUL_COLD, true) },
        { "lite", sources(TR_VUL_LITE, false) },
        // has_vuln_curse() は TR_VUL_CURSE に加えて装備の CurseTraitType::VUL_CURSE も
        // 弱点の要因に数えるが、ここでは意図的に拾わない。キャラクター画面の装備欄
        // (process_inventory_characteristic()) は get_flags_known() しか見ておらず、
        // 呪い特性由来の呪力弱点はプレイヤーにはどこにも表示されないので、拾えば
        // プレイヤーが見られない情報を出すことになる。
        { "curse", sources(TR_VUL_CURSE, false) },
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
        { "name", to_json_utf8(describe_flavor(player_ptr, item, OD_OMIT_PREFIX)) },
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
        result["inscription"] = item.is_inscribed() ? to_json_utf8(*item.inscription) : "";
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

nlohmann::json make_look_json(PlayerType *player_ptr)
{
    const auto &player = *player_ptr;
    const auto &floor = *player.current_floor_ptr;
    const auto is_hallucinated = player.effects()->hallucination().is_hallucinated();
    const auto positions = target_set_prepare(player_ptr, TARGET_LOOK);
    auto grids = nlohmann::json::array();
    grids.get_ref<nlohmann::json::array_t &>().reserve(positions.size());
    for (std::size_t index = 0; index < positions.size(); ++index) {
        const auto &pos = positions[index];
        const auto &grid = floor.get_grid(pos);
        const auto terrain_redacted = !grid.is_mark() && !player_can_see_bold(player_ptr, pos.y, pos.x);
        const auto &terrain = terrain_redacted
                                  ? TerrainList::get_instance().get_terrain(TerrainTag::NONE)
                                  : grid.get_terrain(TerrainKind::MIMIC);
        auto monster_json = nlohmann::json(nullptr);
        auto carried_items = nlohmann::json::array();
        // This look record mirrors what the look command itself reports:
        // target_set_accept() uses ml alone, so telepathy/ESP-visible monsters
        // are included as fair-play information the player can read on screen.
        // The always-on visible_monsters field intentionally remains narrower,
        // and detected_monsters is its through-wall partition; do not unify
        // these three gates.
        if (grid.has_monster()) {
            const auto &monster = floor.m_list[grid.m_idx];
            if (monster.is_valid() && monster.ml) {
                monster_json = make_visible_monster_json(grid.m_idx, monster, is_hallucinated);
                for (const auto o_idx : monster.hold_o_idx_list) {
                    const auto &item = *floor.o_list[o_idx];
                    if (item.is_valid()) {
                        carried_items.push_back(make_item_json(player_ptr, item));
                    }
                }
            }
        }

        auto items = nlohmann::json::array();
        for (const auto o_idx : grid.o_idx_list) {
            const auto &item = *floor.o_list[o_idx];
            if (item.is_valid() && item.marked.has(OmType::FOUND)) {
                items.push_back(make_item_json(player_ptr, item));
            }
        }

        grids.push_back({
            { "index", index },
            { "y", pos.y },
            { "x", pos.x },
            { "distance", Grid::calc_distance(player.get_position(), pos) },
            { "is_player_position", pos == player.get_position() },
            { "terrain", {
                             { "terrain_id", terrain.idx },
                             { "name", to_json_utf8(terrain.name) },
                             { "redacted", terrain_redacted },
                         } },
            { "monster", std::move(monster_json) },
            { "items", std::move(items) },
            { "carried_items", std::move(carried_items) },
        });
    }

    return {
        { "hallucinated", is_hallucinated },
        { "panel", {
                       { "row_min", panel_row_min },
                       { "row_max", panel_row_max },
                       { "col_min", panel_col_min },
                       { "col_max", panel_col_max },
                   } },
        { "grids", std::move(grids) },
    };
}

nlohmann::json make_inventory_json(PlayerType *player_ptr)
{
    auto items = nlohmann::json::array();
    for (const auto i_idx : INVEN_PACK_SLOTS) {
        const auto &item = player_ptr->inventory[i_idx];
        if (!item || !item->is_valid()) {
            continue;
        }

        auto entry = make_item_json(player_ptr, *item);
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
        const auto &item = player_ptr->inventory[i_idx];
        if (!item || !item->is_valid()) {
            continue;
        }

        auto entry = make_item_json(player_ptr, *item);
        entry["slot"] = equipment_slot_label(i_idx);
        items.push_back(std::move(entry));
    }

    return items;
}

/*!
 * @brief 店の在庫とページング状況を出力する
 * @details 呼び出し口は do_cmd_store() のコマンドループ1箇所だけで、そこに到達する時点で
 * st_ptr は &store に、store_bottom は MIN_STOCK + xtra_stock に設定済みである。
 * st_ptr だけを null ガードしても store_top / store_bottom は初期値 0 のまま出てしまい、
 * ページ数を割り算する消費側をゼロ除算させるだけなので、3つとも同じ前提に揃えて扱う。
 */
nlohmann::json make_store_json(PlayerType *player_ptr, StoreSaleType store_num)
{
    auto items = nlohmann::json::array();
    const auto is_personal_storage = store_num == StoreSaleType::HOME || store_num == StoreSaleType::MUSEUM;
    for (auto i = 0; i < st_ptr->stock_num; ++i) {
        // The store accepts the item letter RELATIVE to the currently visible
        // page: pressing 'a' selects stock[store_top]. Only items on the
        // current page (store_top .. store_top+store_bottom) are selectable,
        // so emit page-relative letters and skip off-page items. Mirrors
        // display_entry()'s labelling.
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
            { "name", to_json_utf8(describe_flavor(player_ptr, item, OD_STORE | OD_OMIT_PREFIX)) },
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

    // Paging facts the player already reads off the screen: the listing shows
    // one page of `page_size` slots starting at `page_top`, and the store's
    // own prompt tells the player whether more pages follow.  Without them the
    // page count can only be guessed, and a full page (letters running a..Z)
    // is NOT evidence that the stock ends there.
    return {
        { "store_type", enum2i(store_num) },
        { "stock_num", st_ptr->stock_num },
        { "page_top", store_top },
        { "page_size", store_bottom },
        { "items", items },
    };
}

/*!
 * @brief スナップショットのmessagesに何を載せるか
 * @details
 * make_recent_messages_json()はJSONL出力の連続したスナップショット間の差分を返すため、
 * 呼ぶたびにstaticな差分状態を進める。制御サーバのstateがこれを呼ぶと、
 * --bot-json-outputと併用した際にstate側が差分を消費し、JSONL側からメッセージが失われる。
 * 差分を進めてよいのは連続した記録を書くJSONL出力だけなので、出力先ごとに選べるようにする。
 */
enum class BotSnapshotMessages {
    RECENT_DIFF, //!< 前回のスナップショットからの新着 (JSONL出力用)
    HISTORY, //!< 直近の履歴 (制御サーバ用。差分状態を進めない)
};

/*!
 * @brief スナップショット本体を組み立てる
 * @param include_map nearby_grids を含めるか
 * @param messages_source messages に載せるメッセージの選び方
 * @details nearby_grids はフロア全域を走査して1万件規模の配列を作る処理で、
 * スナップショット1件のバイト数の99%超を占める。地図を出さない呼び出し
 * （店内スナップショット）では作ってから消すのではなく最初から作らない。
 */
nlohmann::json make_snapshot(PlayerType *player_ptr, bool include_map = true,
    BotSnapshotMessages messages_source = BotSnapshotMessages::RECENT_DIFF)
{
    const auto &player = *player_ptr;
    const auto &floor = *player.current_floor_ptr;
    const auto &world = AngbandWorld::get_instance();
    const auto &dungeons = DungeonList::get_instance();
    const auto &dungeon_records = DungeonRecords::get_instance();
    const auto known_flags = collect_player_known_flags(player_ptr);
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
    auto snapshot = nlohmann::json{
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
                        { "status", make_player_status_json(*player.effects()) },
                        { "stats", make_player_stats_json(player) },
                        { "ability_sources", make_player_ability_sources_json(known_flags) },
                        { "immunities", make_player_immunities_json(player_ptr, known_flags) },
                        { "vulnerabilities", make_player_vulnerabilities_json(player_ptr, known_flags) },
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
                          // recall to Angband." has_entered() alone does not cover
                          // the rumor-unlock case. This is player-visible state,
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
                               for (size_t i = 0; i < records.size(); i++) {
                                   if (records.has_visited(i2enum<TownId>(i))) {
                                       ids.push_back(static_cast<int>(i));
                                   }
                               }
                               return ids;
                           }() },
                          { "quests", make_disclosed_quests_json() },
                      } },
        { "visible_monsters", make_visible_monsters_json(player) },
        { "detected_monsters", make_detected_monsters_json(player) },
        { "messages", (messages_source == BotSnapshotMessages::HISTORY) ? make_message_history_json(BOT_JSON_MAX_MESSAGES) : make_recent_messages_json() },
        { "inventory", make_inventory_json(player_ptr) },
        { "equipment", make_equipment_json(player_ptr) },
    };
    if (include_map) {
        snapshot["nearby_grids"] = make_nearby_grids_json(player);
    }

    return snapshot;
}

nlohmann::json make_flag_table_json(PlayerType *player_ptr)
{
    static constexpr tr_type flags[] = {
        TR_RES_ACID,
        TR_RES_ELEC,
        TR_RES_FIRE,
        TR_RES_COLD,
        TR_RES_POIS,
        TR_RES_LITE,
        TR_RES_DARK,
        TR_RES_SHARDS,
        TR_RES_BLIND,
        TR_RES_CONF,
        TR_RES_SOUND,
        TR_RES_NETHER,
        TR_RES_NEXUS,
        TR_RES_CHAOS,
        TR_RES_DISEN,
        TR_RES_TIME,
        TR_RES_WATER,
        TR_RES_FEAR,
        TR_RES_CURSE,
        TR_SPEED,
        TR_FREE_ACT,
        TR_SEE_INVIS,
        TR_HOLD_EXP,
        TR_WARNING,
        TR_SLOW_DIGEST,
        TR_REGEN,
        TR_LEVITATION,
        TR_REFLECT,
        TR_SUST_STR,
        TR_SUST_INT,
        TR_SUST_WIS,
        TR_SUST_DEX,
        TR_SUST_CON,
        TR_SUST_CHR,
        TR_IM_ACID,
        TR_IM_ELEC,
        TR_IM_FIRE,
        TR_IM_COLD,
        TR_IM_DARK,
        TR_IM_LITE,
        TR_VUL_ACID,
        TR_VUL_ELEC,
        TR_VUL_FIRE,
        TR_VUL_COLD,
        TR_VUL_LITE,
        TR_VUL_CURSE,
        TR_SLAY_EVIL,
        TR_KILL_EVIL,
        TR_SLAY_GOOD,
        TR_KILL_GOOD,
        TR_SLAY_HUMAN,
        TR_KILL_HUMAN,
        TR_SLAY_ANIMAL,
        TR_KILL_ANIMAL,
        TR_SLAY_DRAGON,
        TR_KILL_DRAGON,
        TR_SLAY_ORC,
        TR_KILL_ORC,
        TR_SLAY_TROLL,
        TR_KILL_TROLL,
        TR_SLAY_GIANT,
        TR_KILL_GIANT,
        TR_SLAY_DEMON,
        TR_KILL_DEMON,
        TR_SLAY_UNDEAD,
        TR_KILL_UNDEAD,
        TR_BRAND_ACID,
        TR_BRAND_ELEC,
        TR_BRAND_FIRE,
        TR_BRAND_COLD,
        TR_BRAND_POIS,
        TR_BRAND_MAGIC,
        TR_VORPAL,
        TR_VAMPIRIC,
        TR_CHAOTIC,
        TR_FORCE_WEAPON,
        TR_IMPACT,
        TR_EARTHQUAKE,
        TR_BLESSED,
        TR_ESP_ANIMAL,
        TR_ESP_UNDEAD,
        TR_ESP_DEMON,
        TR_ESP_ORC,
        TR_ESP_TROLL,
        TR_ESP_GIANT,
        TR_ESP_DRAGON,
        TR_ESP_HUMAN,
        TR_ESP_EVIL,
        TR_ESP_GOOD,
        TR_ESP_NONLIVING,
        TR_ESP_UNIQUE,
        TR_TELEPATHY,
        TR_THROW,
        TR_BLOWS,
        TR_XTRA_SHOTS,
        TR_MAGIC_MASTERY,
        TR_DEC_MANA,
        TR_EASY_SPELL,
        TR_INFRA,
        TR_STEALTH,
        TR_SEARCH,
        TR_TUNNEL,
        TR_ACTIVATE,
        TR_RIDING,
        TR_LITE_1,
        TR_LITE_2,
        TR_LITE_3,
        TR_LITE_M1,
        TR_LITE_M2,
        TR_LITE_M3,
        TR_AGGRAVATE,
        TR_TY_CURSE,
        TR_ADD_L_CURSE,
        TR_ADD_H_CURSE,
        TR_PERSISTENT_CURSE,
        TR_DRAIN_EXP,
        TR_DRAIN_HP,
        TR_DRAIN_MANA,
        TR_FAST_DIGEST,
        TR_SLOW_REGEN,
        TR_COWARDICE,
        TR_LOW_MELEE,
        TR_LOW_AC,
        TR_HARD_SPELL,
        TR_NO_TELE,
        TR_NO_MAGIC,
        TR_BERS_RAGE,
        TR_SH_FIRE,
        TR_SH_ELEC,
        TR_SH_COLD,
        TR_SELF_FIRE,
        TR_SELF_ELEC,
        TR_SELF_COLD,
        TR_INVULN_ARROW,
        TR_SUPPORTIVE,
        TR_DOWN_SAVING,
    };
    const auto known_flags = collect_player_known_flags(player_ptr);
    TrFlags immunity;
    TrFlags temporary_immunity;
    TrFlags vulnerability;
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
            { "player", known_flags.permanent.has(flag) },
            { "temporary", known_flags.temporary.has(flag) || (flag == TR_LITE_1 && player_ptr->tim_emission > 0) },
            { "immunity", immunity.has(flag) },
            { "temporary_immunity", temporary_immunity.has(flag) },
            { "vulnerability", vulnerability.has(flag) },
        });
    }
    return rows;
}

nlohmann::json disclosed_stat_maximum(const PlayerType &player, int stat_id)
{
    return ((player.knowledge & KNOW_STAT) || player.stat_max[stat_id] == player.stat_max_max[stat_id])
               ? nlohmann::json(player.stat_max_max[stat_id])
               : nlohmann::json(nullptr);
}

nlohmann::json make_character_json(PlayerType *player_ptr)
{
    auto &player = *player_ptr;
    std::array<int, 2> damage{};
    std::array<int, 2> to_h{};
    calc_player_two_hands(player_ptr, damage.data(), to_h.data());
    int shots = 0;
    int shot_frac = 0;
    const auto &bow = *player.inventory[INVEN_BOW];
    calc_player_shot_params(player_ptr, player.inventory[INVEN_BOW].get(), &shots, &shot_frac);
    auto stat_details = nlohmann::json::array();
    for (auto i = 0; i < A_MAX; ++i) {
        stat_details.push_back({
            { "stat_id", i },
            { "top", player.stat_top[i] },
            { "maximum", disclosed_stat_maximum(player, i) },
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
            realm_json.push_back({ { "id", enum2i(realm.to_enum()) }, { "name", to_json_utf8(realm.get_name().string()) } });
        }
    }
    auto shooting_multiplier = 0;
    if (bow.is_valid()) {
        shooting_multiplier = (bow.get_arrow_magnification() + (player.xtra_might ? 1 : 0)) * (100 + static_cast<int>(adj_str_td[player.stat_index[A_STR]]) - 128);
    }
    return {
        { "skills", {
                        { "thn", player.skill_thn },
                        { "thb", player.skill_thb },
                        { "sav", player.skill_sav },
                        { "dev", player.skill_dev },
                        { "stl", player.skill_stl },
                        { "dis", player.skill_dis },
                        { "srh", player.skill_srh },
                        { "fos", player.skill_fos },
                        { "dig", player.skill_dig },
                    } },
        { "ranged", {
                        { "to_h_b", player.to_h_b },
                        { "shots", shots },
                        { "shot_frac", shot_frac },
                        { "shooting_multiplier", shooting_multiplier },
                        { "see_infra", player.see_infra },
                    } },
        { "melee", {
                       { "expected_damage_x100", { damage[0], damage[1] } },
                       { "expected_damage_per_round_x100", { player.num_blow[0] * damage[0], player.num_blow[1] * damage[1] } },
                       { "bare_hand", !has_melee_weapon(player_ptr, INVEN_MAIN_HAND) && !has_melee_weapon(player_ptr, INVEN_SUB_HAND) },
                       { "two_handed", has_two_handed_weapons(player_ptr) },
                       { "monk_stance", enum2i(PlayerClass(player_ptr).get_monk_stance()) },
                   } },
        { "stats", std::move(stat_details) },
        { "age", player.age },
        { "height", player.ht },
        { "weight", player.wt },
        { "social_class", player.sc },
        { "alignment", player.alignment },
        { "realms", std::move(realm_json) },
        { "chaos_patron", player.chaos_patron },
        { "mutations", std::move(mutations) },
        { "characteristics", make_flag_table_json(player_ptr) },
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
        "artifacts_known",
        "artifacts_identified",
        "objects_known",
        "uniques_alive",
        "uniques_dead",
        "bounty",
        "home",
        "equip_resistances",
        "features",
        "self_info",
        "mutations",
        "weapon_exp",
        "spell_exp",
        "skill_exp",
        "virtues",
        "dungeons",
        "quests",
        "pets",
        "autopick",
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
                { "name", to_json_utf8(baseitem.stripped_name()) } });
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
            rows.push_back({ { "id", enum2i(id) }, { "name", to_json_utf8(monrace->name.string()) }, { "level", monrace->level } });
        }
        result["uniques"] = std::move(rows);
        break;
    }
    case BotKnowledgeCategory::BOUNTY: {
        const auto &world = AngbandWorld::get_instance();
        result["today"] = world.knows_daily_bounty
                              ? nlohmann::json{ { "id", enum2i(world.today_mon) }, { "name", to_json_utf8(world.get_today_bounty().name.string()) } }
                              : nlohmann::json(nullptr);
        for (const auto &[id, achieved] : world.bounties) {
            if (!achieved) {
                rows.push_back({ { "id", enum2i(id) }, { "name", to_json_utf8(MonraceList::get_instance().get_monrace(id).name.string()) } });
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
    case BotKnowledgeCategory::EQUIP_RESISTANCES: {
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
                rows.push_back({ { "id", terrain.idx }, { "name", to_json_utf8(terrain.name) } });
            }
        }
        result["features"] = std::move(rows);
        break;
    case BotKnowledgeCategory::SELF_INFO:
        result["life_rating"] = (player_ptr->knowledge & KNOW_HPRATE) ? nlohmann::json(player_ptr->calc_life_rating()) : nlohmann::json(nullptr);
        for (auto i = 0; i < A_MAX; ++i) {
            rows.push_back({ { "stat_id", i }, { "maximum", disclosed_stat_maximum(*player_ptr, i) } });
        }
        result["stat_limits"] = std::move(rows);
        break;
    case BotKnowledgeCategory::MUTATIONS:
        for (auto i = 0; i < enum2i(PlayerMutationType::MAX); ++i) {
            if (player_ptr->muta.has(i2enum<PlayerMutationType>(i))) {
                rows.push_back(i);
            }
        }
        result["mutation_ids"] = std::move(rows);
        break;
    case BotKnowledgeCategory::WEAPON_EXP:
        for (const auto &baseitem : BaseitemList::get_instance()) {
            const auto tval = baseitem.bi_key.tval();
            const auto sval = baseitem.bi_key.sval();
            const auto displayed_tval = tval == ItemKindType::SWORD || tval == ItemKindType::POLEARM || tval == ItemKindType::HAFTED || tval == ItemKindType::DIGGING || tval == ItemKindType::BOW;
            const auto excluded_bow = tval == ItemKindType::BOW && sval && (*sval == SV_CRIMSON || *sval == SV_HARP);
            const auto exp_it = player_ptr->weapon_exp.find(tval);
            const auto max_it = player_ptr->weapon_exp_max.find(tval);
            if (displayed_tval && !excluded_bow && sval && exp_it != player_ptr->weapon_exp.end() && max_it != player_ptr->weapon_exp_max.end()) {
                rows.push_back({ { "tval", enum2i(tval) }, { "sval", *sval }, { "name", to_json_utf8(baseitem.stripped_name()) },
                    { "exp", exp_it->second[*sval] }, { "max", max_it->second[*sval] },
                    { "rank", enum2i(PlayerSkill::weapon_skill_rank(exp_it->second[*sval])) } });
            }
        }
        result["weapons"] = std::move(rows);
        break;
    case BotKnowledgeCategory::SKILL_EXP:
        for (const auto skill : PLAYER_SKILL_KIND_TYPE_RANGE) {
            rows.push_back({ { "id", enum2i(skill) }, { "name", to_json_utf8(PlayerSkill::skill_name(skill)) },
                { "exp", player_ptr->skill_exp[skill] } });
        }
        result["skills"] = std::move(rows);
        break;
    case BotKnowledgeCategory::VIRTUES:
        for (auto i = 0; i < 8; ++i) {
            const auto name_it = virtue_names.find(player_ptr->vir_types[i]);
            const auto name = to_json_utf8(name_it != virtue_names.end() ? name_it->second : _("不明", "Oops. No info"));
            rows.push_back({ { "id", enum2i(player_ptr->vir_types[i]) }, { "name", name },
                { "value", player_ptr->virtues[i] } });
        }
        result["virtues"] = std::move(rows);
        break;
    case BotKnowledgeCategory::QUESTS:
        result["quests"] = make_disclosed_quests_json();
        break;
    case BotKnowledgeCategory::PETS:
        for (auto i = 1; i < player_ptr->current_floor_ptr->m_max; ++i) {
            const auto &monster = player_ptr->current_floor_ptr->m_list[i];
            if (monster.is_valid() && monster.is_pet()) {
                rows.push_back({ { "index", i }, { "race_id", enum2i(monster.get_monrace_id()) },
                    { "name", to_json_utf8(monster.get_monrace().name.string()) } });
            }
        }
        result["pets"] = std::move(rows);
        break;
    case BotKnowledgeCategory::AUTOPICK:
        for (const auto &entry : autopick_list) {
            rows.push_back({ { "rule", to_json_utf8(autopick_line_from_entry(entry)) } });
        }
        result["rules"] = std::move(rows);
        break;
    case BotKnowledgeCategory::SPELL_EXP: {
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
                const auto is_hissatsu = realm.equals(RealmType::HISSATSU);
                rows.push_back({ { "realm_id", enum2i(realm.to_enum()) }, { "spell_id", spell_id },
                    { "name", to_json_utf8(realm.get_spell_name(spell_id)) },
                    { "exp", is_hissatsu ? nlohmann::json(nullptr) : nlohmann::json(exp) },
                    { "rank", is_hissatsu ? nlohmann::json(nullptr) : nlohmann::json(enum2i(PlayerSkill::spell_skill_rank(exp))) },
                    { "masked", is_hissatsu } });
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
                rows.push_back({ { "id", enum2i(dungeon_id) }, { "name", to_json_utf8(dungeon->name) },
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

/*!
 * @brief ゲーム内部の文字コードの文字列をJSONに載せられるUTF-8文字列へ変換する
 * @param str 変換する文字列
 * @return UTF-8に変換した文字列。変換に失敗した場合は代替文字列
 * @details 変換の失敗でスナップショットやリクエスト全体を落とさないよう、代替文字列で穴埋めする。
 */
std::string to_json_utf8(std::string_view str)
{
    return sys_to_utf8(str).value_or("<encoding-error>");
}

/*!
 * @brief メッセージ履歴を新しい順ではなく古い順に並べて生成する
 * @param count 取得する件数 (履歴の件数を超える分と負値は切り詰める)
 * @return メッセージ文字列のJSON配列
 */
nlohmann::json make_message_history_json(int count)
{
    auto messages = nlohmann::json::array();
    for (auto age = std::clamp<int>(count, 0, message_num()); age-- > 0;) {
        messages.push_back(to_json_utf8(*message_str(age)));
    }

    return messages;
}

/*!
 * @brief ゲームの内部状態のスナップショットをJSONで生成する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @pre player_ptrとplayer_ptr->current_floor_ptrがnullptrでないことを呼び出し側が保証すること
 * @return スナップショットのJSONオブジェクト
 * @details
 * --bot-json-outputの有無に関わらず生成する。制御サーバから利用する。
 * messagesは差分ではなく履歴を載せる。リクエストの度に差分を進めると、
 * --bot-json-outputと併用した際にJSONL側からメッセージが失われるためである。
 */
nlohmann::json make_bot_json_snapshot(PlayerType *player_ptr)
{
    return make_snapshot(player_ptr, true, BotSnapshotMessages::HISTORY);
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

    // Same base snapshot (player gold, inventory, equipment) plus the store's
    // stock, so the bot can decide what to buy while at the store prompt.
    //
    // The map is deliberately omitted here.  At the store prompt the player
    // cannot move and the floor cannot change, so nearby_grids would repeat
    // the surface snapshot's map verbatim -- yet it is over 99% of a snapshot's
    // bytes (measured: 5.09 MB per record, 10,419 grid entries).  A store
    // session emits one snapshot per processed key (see cmd-store.cpp), so
    // paging through a large inventory multiplies that cost by the key count.
    // This reveals nothing new to the bot; it only stops re-sending what the
    // preceding surface snapshot already carried.
    auto snapshot = make_snapshot(player_ptr, false);
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

void output_bot_json_look_snapshot(PlayerType *player_ptr)
{
    if (!arg_bot_json_output || player_ptr == nullptr || player_ptr->current_floor_ptr == nullptr) {
        return;
    }
    auto snapshot = make_snapshot(player_ptr);
    snapshot["type"] = "look";
    snapshot["look"] = make_look_json(player_ptr);
    write_snapshot(snapshot);
}
