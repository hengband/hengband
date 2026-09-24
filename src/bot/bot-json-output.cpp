#include "bot/bot-json-output.h"
#include "artifact/fixed-art-types.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-util.h"
#include "avatar/avatar.h"
#include "combat/attack-power-table.h"
#include "combat/shoot.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/dungeon-feeling.h"
#include "floor/floor-util.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/map-screen-options.h"
#include "game-option/runtime-arguments.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-requester.h"
#include "locale/character-encoding.h"
#include "lore/lore-util.h"
#include "lore/monster-lore.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "player-ability/player-ability-types.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/alignment.h"
#include "player-info/class-info.h"
#include "player-info/equipment-info.h"
#include "player-info/mane-data-type.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/permanent-resistances.h"
#include "player/player-personality.h"
#include "player/player-realm.h"
#include "player/player-sex.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/player-status-table.h"
#include "player/race-info-table.h"
#include "player/race-resistances.h"
#include "player/temporary-resistances.h"
#include "spell/technic-info-table.h"
#include "store/pricing.h"
#include "store/store-owners.h"
#include "store/store-screen.h"
#include "store/store-util.h"
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
#include "system/inner-game-data.h"
#include "system/item/identification-flags.h"
#include "system/item/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monrace/monrace-records.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "target/target-describer.h"
#include "target/target-preparation.h"
#include "target/target-types.h"
#include "term/term-color-types.h"
#include "timed-effect/player-cut.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "tracking/health-bar-tracker.h"
#include "util/enum-converter.h"
#include "view/display-characteristic.h"
#include "view/display-map.h"
#include "view/display-messages.h"
#include "view/display-player-middle.h"
#include "view/display-player-stat-info.h"
#include "view/status-first-page.h"
#include "window/main-window-left-frame.h"
#include "window/main-window-stat-poster.h"
#include "window/main-window-util.h"
#include "world/world.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
constexpr std::streamoff BOT_JSON_OUTPUT_MAX_BYTES = 256LL * 1024 * 1024;
constexpr int BOT_JSON_MAX_MESSAGES = 32;

/*!
 * @brief 画面の表示色をJSONに載せる名前に変換する
 * @param color 表示色 (TERM_*)
 * @return 色名
 */
const char *term_color_name(TERM_COLOR color)
{
    static constexpr std::array<const char *, 16> names{ {
        "dark",
        "white",
        "slate",
        "orange",
        "red",
        "green",
        "blue",
        "umber",
        "light_dark",
        "light_white",
        "violet",
        "yellow",
        "light_red",
        "light_green",
        "light_blue",
        "light_umber",
    } };
    return color < names.size() ? names[color] : "unknown";
}

/*!
 * @brief 画面下部の状態表示の各項目に付けるキー
 * @details 添字は bar_definition_type と一致させる。項目を足したら末尾に追加すること。
 */
constexpr std::array<const char *, BAR_IMMDARK + 1> STATUS_BAR_KEYS{ {
    "tsuyoshi",
    "hallucination",
    "blindness",
    "paralyzed",
    "confused",
    "poisoned",
    "afraid",
    "levitation",
    "reflection",
    "pass_wall",
    "wraith_form",
    "protection_from_evil",
    "kawarimi",
    "magic_armour",
    "expand",
    "stone_skin",
    "multishadow",
    "resist_magic",
    "ultimate_resistance",
    "invulnerability",
    "immune_acid",
    "resist_acid",
    "immune_elec",
    "resist_elec",
    "immune_fire",
    "resist_fire",
    "immune_cold",
    "resist_cold",
    "resist_pois",
    "resist_nether",
    "resist_time",
    "dust_robe",
    "aura_fire",
    "aura_force",
    "aura_holy",
    "eye_for_eye",
    "blessed",
    "heroism",
    "berserk",
    "brand_fire",
    "brand_cold",
    "brand_elec",
    "brand_acid",
    "brand_pois",
    "confusing_touch",
    "see_invisible",
    "telepathy",
    "regeneration",
    "infravision",
    "stealth",
    "super_stealth",
    "recall",
    "alter_reality",
    "aura_cold",
    "aura_elec",
    "aura_shadow",
    "extra_might",
    "build_up",
    "anti_multiply",
    "anti_teleport",
    "anti_magic",
    "patience",
    "revenge",
    "rune_sword",
    "vampiric",
    "cure",
    "esp_evil",
    "nightsight",
    "resist_lite",
    "resist_dark",
    "resist_fear",
    "emission",
    "exorcism",
    "immune_dark",
} };
static_assert(STATUS_BAR_KEYS.size() + 1 == MAX_STAT_BARS, "STATUS_BAR_KEYS must cover every status bar entry");
static_assert(STATUS_BAR_KEYS.back() != nullptr, "STATUS_BAR_KEYS must name every status bar entry");

/*!
 * @brief 画面下部の状態表示に出ている項目を、表示順 (bar_definition_type 順) に並べる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 項目の配列。各要素は id (bar_definition_type)・key・label (長い表記) を持つ
 * @details 表示の可否は print_status() と同じ collect_status_bar_flags() で判定する。
 * 端末の幅が足りない場合、画面では右端の項目が切れることがあるが、JSONには全項目を載せる
 * (端末を広げれば表示される項目であり、表示の可否そのものは幅に依存しないため)。
 */
nlohmann::json make_status_bar_json(PlayerType *player_ptr)
{
    const auto flags = collect_status_bar_flags(player_ptr);
    auto entries = nlohmann::json::array();
    for (std::size_t i = 0; i < STATUS_BAR_KEYS.size(); ++i) {
        if (!flags.test(i)) {
            continue;
        }

        entries.push_back({
            { "id", i },
            { "key", STATUS_BAR_KEYS[i] },
            { "label", to_json_utf8(stat_bars[i].lstr) },
        });
    }

    return entries;
}

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
    // The level is printed (look command, monster list sub-window) only once the
    // race has been killed, and never for a kage; otherwise "Level ???" / "??".
    const auto level_known = (monrace.r_tkills > 0) && monster.mflag2.has_not(MonsterConstantFlagType::KAGE);
    return {
        { "index", m_idx },
        { "race_id", enum2i(monrace.idx) },
        { "name", to_json_utf8(monrace.name.string()) },
        { "level", level_known ? nlohmann::json(monrace.level) : nlohmann::json(nullptr) },
        { "health", monster_health_band(monster) },
        { "asleep", monster.is_asleep() },
        { "stunned", monster.is_stunned() },
        { "confused", monster.is_confused() },
        { "fearful", monster.is_fearful() },
        // 加速・減速・無敵は、体力ゲージで追跡しているモンスターの状態欄と同じ表示の対象
        { "fast", monster.is_accelerated() },
        { "slow", monster.is_decelerated() },
        { "invulnerable", monster.is_invulnerable() },
        { "friendly", monster.is_friendly() },
        { "pet", monster.is_pet() },
    };
}

/*!
 * @brief 画面左の体力ゲージ (追跡中のモンスター、または騎乗中のモンスター) を出力する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param riding trueなら騎乗中のモンスターのゲージ、falseなら追跡中のモンスターのゲージ
 * @return ゲージを表示していないならnull
 * @details print_health() の描画と同じ条件で出力する。見えていない・幻覚中・死亡したモンスターは
 * 画面では "[----------]" だけが描かれ、どのモンスターかも分からないので index も出さない。
 * 状態欄 (conditions) は追跡中のゲージにだけ描かれる。画面では端末の高さに応じて行が切れる
 * ことがあるが、全項目を載せる。
 */
nlohmann::json make_health_bar_json(PlayerType *player_ptr, bool riding)
{
    short m_idx = 0;
    if (riding) {
        m_idx = player_ptr->riding;
    } else if (const auto &tracker = HealthBarTracker::get_instance(); tracker.is_tracking()) {
        m_idx = tracker.get_trackee();
    }

    if (m_idx <= 0) {
        return nullptr;
    }

    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    if (!monster.ml || player_ptr->effects()->hallucination().is_active() || monster.is_dead()) {
        return { { "known", false } };
    }

    const auto &[color, length] = monster.get_hp_bar_data();
    auto result = nlohmann::json{
        { "known", true },
        { "index", m_idx },
        { "length", length },
        { "color", term_color_name(color) },
    };
    if (riding) {
        return result;
    }

    auto conditions = nlohmann::json::array();
    const auto add_condition = [&conditions](bool active, const char *key) {
        if (active) {
            conditions.push_back(key);
        }
    };
    add_condition(monster.is_invulnerable(), "invulnerable");
    add_condition(monster.is_accelerated(), "fast");
    add_condition(monster.is_decelerated(), "slow");
    add_condition(monster.is_fearful(), "afraid");
    add_condition(monster.is_confused(), "confused");
    add_condition(monster.is_asleep(), "asleep");
    add_condition(monster.is_stunned(), "stunned");
    result["conditions"] = std::move(conditions);
    return result;
}

nlohmann::json make_visible_monsters_json(const PlayerType &player)
{
    auto monsters = nlohmann::json::array();
    const auto &floor = *player.current_floor_ptr;
    const auto is_hallucinated = player.effects()->hallucination().is_active();
    for (short m_idx = 1; m_idx < floor.m_max; ++m_idx) {
        const auto &monster = floor.m_list[m_idx];
        // The always-on visible list is deliberately direct-sight-only. ESP and
        // detection perceptions belong in detected_monsters, while look follows
        // the command's broader ml-only gate; do not unify these three gates.
        if (!monster.is_valid() || !monster.ml || !floor.get_grid(monster.get_position()).is_view()) {
            continue;
        }

        auto monster_json = make_visible_monster_json(m_idx, monster, is_hallucinated);
        // A monster can be perceived without knowing its terrain (infravision,
        // detection), and state(map=false) has no map sidecar to supply position.
        const auto &pos = monster.get_position();
        monster_json["y"] = pos.y;
        monster_json["x"] = pos.x;
        monsters.push_back(std::move(monster_json));
    }

    return monsters;
}

nlohmann::json make_detected_monsters_json(const PlayerType &player)
{
    auto monsters = nlohmann::json::array();
    const auto &floor = *player.current_floor_ptr;
    const auto is_hallucinated = player.effects()->hallucination().is_active();
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

using GridSignature = std::array<std::int64_t, 4>;

struct GridSignatureHash {
    std::size_t operator()(const GridSignature &signature) const noexcept
    {
        auto hash = std::size_t{ 0 };
        for (const auto value : signature) {
            hash ^= std::hash<std::int64_t>{}(value) + 0x9e3779b9U + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

GridSignature make_grid_signature(const PlayerType &player, const Pos2D &pos, const Grid &source_grid)
{
    // Wire bit positions are append-only; keep tools/bot/README.md in sync.
    const auto &terrain = source_grid.get_terrain(TerrainKind::MIMIC);
    const auto has_terrain = [&terrain](TerrainCharacteristics flag) { return terrain.flags.has(flag); };
    constexpr std::array<TerrainCharacteristics, 18> terrain_flags{ {
        TerrainCharacteristics::BLDG,
        TerrainCharacteristics::CAN_DIG,
        TerrainCharacteristics::DOOR,
        TerrainCharacteristics::DOWN_STAIRS,
        TerrainCharacteristics::ENTRANCE,
        TerrainCharacteristics::FLOOR,
        TerrainCharacteristics::HAS_GOLD,
        TerrainCharacteristics::LOS,
        TerrainCharacteristics::MOVE,
        TerrainCharacteristics::PERMANENT,
        TerrainCharacteristics::QUEST_ENTER,
        TerrainCharacteristics::QUEST_EXIT,
        TerrainCharacteristics::STAIRS,
        TerrainCharacteristics::STORE,
        TerrainCharacteristics::TRAP,
        TerrainCharacteristics::TUNNEL,
        TerrainCharacteristics::UP_STAIRS,
        TerrainCharacteristics::WALL,
    } };
    // Protocol 3: the slot carries the lighting variant the map draws the
    // terrain with (F_LIT_STANDARD=0 / F_LIT_LITE=1 / F_LIT_DARK=2), decided by
    // the same function as map_info(). That is exactly what the player reads
    // off the tile colour (torch-lit, lit room in view, remembered/dark), and
    // never raw CAVE_* bits.
    const auto flag_bits = static_cast<std::int64_t>(decide_map_terrain_lighting(player, pos));
    auto terrain_bits = std::int64_t{ 0 };
    for (std::size_t i = 0; i < terrain_flags.size(); ++i) {
        terrain_bits |= static_cast<std::int64_t>(has_terrain(terrain_flags[i])) << i;
    }
    return { { source_grid.get_terrain_id(TerrainKind::MIMIC), flag_bits, terrain_bits, 1 } };
}

nlohmann::json make_unsafe_rows_json(const FloorType &floor)
{
    if (!floor.is_underground() || !view_unsafe_grids) {
        return nullptr;
    }

    constexpr std::string_view hex_digits = "0123456789abcdef";
    auto rows = nlohmann::json::array();
    for (auto y = 0; y < floor.height; ++y) {
        std::string row;
        for (auto x = 0; x < floor.width; x += 4) {
            auto bits = 0U;
            for (auto bit = 0; bit < 4 && x + bit < floor.width; ++bit) {
                if ((floor.get_grid({ y, x + bit }).info & CAVE_UNSAFE) != 0) {
                    bits |= 1U << bit;
                }
            }
            row.push_back(hex_digits[bits]);
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

nlohmann::json make_grid_map_json(PlayerType *player_ptr)
{
    const auto &player = *player_ptr;
    const auto &floor = *player.current_floor_ptr;
    auto palette = nlohmann::json::array();
    auto runs = nlohmann::json::array();
    auto cells = nlohmann::json::array();
    auto found_items = nlohmann::json::array();
    auto found_item_names = nlohmann::json::array();
    auto unsafe_rows = make_unsafe_rows_json(floor);
    std::unordered_map<GridSignature, std::size_t, GridSignatureHash> palette_indices;

    auto previous_y = -1;
    auto previous_x = -2;
    std::size_t previous_palette_index = 0;
    auto has_previous_run = false;
    const auto is_hallucinated = player.effects()->hallucination().is_active();
    // Emit the player's entire memorised map (marked tiles) plus the current
    // view — not just a small window. The town is fully known from the start
    // (so the bot can see the dungeon entrance immediately, like the player),
    // and in the dungeon this lets it navigate to any already-explored feature.
    // An omitted tile supplies no new terrain observation. Clients retain
    // their own observation history rather than treating omission as deletion.
    for (const auto &pos : floor.get_area()) {
        const auto &source_grid = floor.get_grid(pos);
        const auto is_known = is_map_terrain_visible(player, pos);
        auto found_item_count = 0;
        auto found_item_tvals = nlohmann::json::array();
        auto found_item_name_list = nlohmann::json::array();
        auto visible_object_count = 0;
        auto visible_object_tvals = nlohmann::json::array();
        for (const auto o_idx : source_grid.o_idx_list) {
            const auto &item = *floor.o_list[o_idx];
            if (!item.is_valid() || item.marked.has_not(OmType::FOUND)) {
                continue;
            }

            if (is_known) {
                ++visible_object_count;
                if (!is_hallucinated) {
                    visible_object_tvals.push_back(enum2i(item.bi_key.tval()));
                }
            }
            if (item.number > 0 && item.bi_key.tval() != ItemKindType::GOLD) {
                ++found_item_count;
                // Hide the real class during hallucination without changing the
                // count/row-length contract. NONE is an unknown-class placeholder.
                const auto tval = is_hallucinated ? ItemKindType::NONE : item.bi_key.tval();
                found_item_tvals.push_back(enum2i(tval));
                // The name the look command prints for the stack. describe_flavor()
                // honours flavor awareness and identification, so an unknown item
                // reads as its flavor, never its real kind. While hallucinating
                // the look command cannot name items either.
                found_item_name_list.push_back(is_hallucinated ? nlohmann::json(nullptr) : nlohmann::json(to_json_utf8(describe_flavor(player_ptr, item, 0))));
            }
        }
        if (found_item_count != 0) {
            auto found_item_row = nlohmann::json::array({ pos.y, pos.x, found_item_count });
            for (auto &tval : found_item_tvals) {
                found_item_row.push_back(std::move(tval));
            }
            found_items.push_back(std::move(found_item_row));
            auto found_item_name_row = nlohmann::json::array({ pos.y, pos.x });
            for (auto &name : found_item_name_list) {
                found_item_name_row.push_back(std::move(name));
            }
            found_item_names.push_back(std::move(found_item_name_row));
        }

        if (!is_known) {
            continue;
        }

        const auto &terrain = source_grid.get_terrain(TerrainKind::MIMIC);
        const auto has_terrain = [&terrain](TerrainCharacteristics flag) { return terrain.flags.has(flag); };
        const auto signature = make_grid_signature(player, pos, source_grid);
        const auto next_palette_index = palette_indices.size();
        const auto [palette_it, inserted] = palette_indices.emplace(signature, next_palette_index);
        if (inserted) {
            palette.push_back(signature);
        }
        const auto palette_index = palette_it->second;

        if (has_previous_run && pos.y == previous_y && pos.x == previous_x + 1 && palette_index == previous_palette_index) {
            runs.back()[2] = runs.back()[2].get<int>() + 1;
        } else {
            runs.push_back({ pos.y, pos.x, 1, palette_index });
            has_previous_run = true;
        }
        previous_y = pos.y;
        previous_x = pos.x;
        previous_palette_index = palette_index;

        short visible_monster_index = 0;
        if (source_grid.is_view() && source_grid.has_monster()) {
            const auto &monster = floor.m_list[source_grid.m_idx];
            if (monster.is_valid() && monster.ml) {
                visible_monster_index = source_grid.m_idx;
            }
        }
        auto cell = nlohmann::json{ { "y", pos.y }, { "x", pos.x } };
        if (visible_monster_index != 0) {
            cell["m"] = visible_monster_index;
        }
        if (visible_object_count != 0) {
            cell["o"] = visible_object_count;
            cell["t"] = std::move(visible_object_tvals);
        }
        // Sparse terrain metadata is kept in the same sidecar. These short keys
        // preserve the old record exactly without polluting the common palette.
        if (has_terrain(TerrainCharacteristics::STORE)) {
            cell["s"] = enum2i(terrain.store_sale_type);
        }
        if (has_terrain(TerrainCharacteristics::ENTRANCE)) {
            cell["e"] = source_grid.special;
        }
        if (has_terrain(TerrainCharacteristics::QUEST_ENTER) || has_terrain(TerrainCharacteristics::QUEST_EXIT)) {
            cell["q"] = source_grid.special;
        }
        if (has_terrain(TerrainCharacteristics::BLDG)) {
            cell["b"] = enum2i(terrain.building_type);
            cell["p"] = source_grid.special;
        }
        if (cell.size() > 2) {
            cells.push_back(std::move(cell));
        }
    }

    return {
        { "w", floor.width },
        { "h", floor.height },
        { "palette", std::move(palette) },
        { "runs", std::move(runs) },
        { "cells", std::move(cells) },
        { "found_items", std::move(found_items) },
        { "found_item_names", std::move(found_item_names) },
        { "unsafe_rows", std::move(unsafe_rows) },
    };
}

/*!
 * @brief 前回のJSONL出力から新しく履歴に加わったメッセージを古い順に返す
 * @details 新着の数は履歴の件数ではなく message_sequence() の差で数える。履歴の件数は上限に
 * 達すると増えなくなり、差で数えると新着を取りこぼすため。件数に上限は設けない
 * (プレイヤーはメッセージ履歴 (Ctrl-P) で全件を読めるので、取りこぼせば見えている情報が欠ける)。
 * 新しい行が無く最新行の文字列だけが変わった場合は、同じメッセージの繰り返し (<xN>) なので最新行を返す。
 */
nlohmann::json make_recent_messages_json()
{
    static std::uint64_t previous_sequence = 0;
    static std::string previous_latest_message = "";
    static auto initialized = false;

    auto messages = nlohmann::json::array();
    const auto current_sequence = message_sequence();
    const auto current_message_count = message_num();
    const auto latest_message = current_message_count > 0 ? *message_str(0) : std::string{};
    if (!initialized) {
        initialized = true;
        previous_sequence = current_sequence;
        previous_latest_message = latest_message;
        return messages;
    }

    const auto new_lines = current_sequence - previous_sequence;
    auto recent_message_count = static_cast<int>(std::min<std::uint64_t>(new_lines, static_cast<std::uint64_t>(current_message_count)));
    if ((recent_message_count == 0) && (latest_message != previous_latest_message)) {
        recent_message_count = std::min(1, current_message_count);
    }

    messages = make_message_history_json(recent_message_count);
    previous_sequence = current_sequence;
    previous_latest_message = latest_message;
    return messages;
}

const char *cut_rank_name(PlayerCutRank rank)
{
    switch (rank) {
    case PlayerCutRank::GRAZING:
        return "graze";
    case PlayerCutRank::LIGHT:
        return "light";
    case PlayerCutRank::BAD:
        return "bad";
    case PlayerCutRank::NASTY:
        return "nasty";
    case PlayerCutRank::SEVERE:
        return "severe";
    case PlayerCutRank::DEEP:
        return "deep_gash";
    case PlayerCutRank::MORTAL:
        return "mortal_wound";
    default:
        return "none";
    }
}

const char *stun_rank_name(PlayerStunRank rank)
{
    switch (rank) {
    case PlayerStunRank::SLIGHT:
        return "slight";
    case PlayerStunRank::NORMAL:
        return "stun";
    case PlayerStunRank::HARD:
        return "heavy";
    case PlayerStunRank::UNCONSCIOUS:
        return "unconscious";
    case PlayerStunRank::KNOCKED:
        return "knocked_out";
    default:
        return "none";
    }
}

nlohmann::json make_player_status_json(const TimedEffects &effects)
{
    // 負傷と朦朧は画面左下に段階名 (かすり傷〜致命傷、やや朦朧〜昏倒) で表示されるので段階まで出す
    return {
        { "blind", effects.blindness().is_active() },
        { "confused", effects.confusion().is_active() },
        { "afraid", effects.fear().is_active() },
        { "poisoned", effects.poison().is_active() },
        { "stunned", effects.stun().is_active() },
        { "stun_rank", enum2i(effects.stun().get_rank()) },
        { "stun_rank_name", stun_rank_name(effects.stun().get_rank()) },
        { "cut", effects.cut().is_active() },
        { "cut_rank", enum2i(effects.cut().get_rank()) },
        { "cut_rank_name", cut_rank_name(effects.cut().get_rank()) },
        { "paralyzed", effects.paralysis().is_active() },
        { "hallucinated", effects.hallucination().is_active() },
    };
}

nlohmann::json make_player_stats_json(const PlayerType &player)
{
    // Only values the screens print. use = the value on the main screen, top =
    // the undrained total the character sheet prints next to a drained stat,
    // max = the character sheet's "Base" column. drained is the reduced stat
    // name / yellow colour, at_racial_max the '!' mark next to the stat name.
    // The raw natural value (stat_cur) is never printed and is not derivable
    // from these (a negative modifier collapses 18/xx values and floors at 3),
    // so protocol 3 no longer emits it.
    auto stat = [&player](int index) {
        return nlohmann::json{
            { "max", player.stat_max[index] },
            { "top", player.stat_top[index] },
            { "use", player.stat_use[index] },
            { "index", player.stat_index[index] },
            { "drained", player.stat_cur[index] < player.stat_max[index] },
            { "at_racial_max", player.stat_max[index] == player.stat_max_max[index] },
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
        { "resist_time", sources(TR_RES_TIME) },
        { "resist_water", sources(TR_RES_WATER) },
        { "resist_curse", sources(TR_RES_CURSE) },
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

nlohmann::json make_item_json(PlayerType *player_ptr, const ItemEntity &item, bool store_identified = false)
{
    // Commercial stock is fully identified by Store::carry(), independently of
    // the player's flavor awareness. Home/museum items must use normal knowledge.
    const auto aware = store_identified || item.is_aware();
    const auto known = store_identified || item.is_known();
    auto result = nlohmann::json{
        { "name", to_json_utf8(describe_flavor(player_ptr, item, OD_OMIT_PREFIX)) },
        { "count", item.number },
        { "tval", enum2i(item.bi_key.tval()) },
        { "aware", aware },
        { "known", known },
        { "fully_known", store_identified || item.is_fully_known() },
        { "is_equipment", item.is_equipment() },
        { "weight", item.weight },
        // This matches the player's bounty knowledge: daily targets only count
        // after they have been learned, while the fixed wanted list is public.
        { "is_bounty", item.is_bounty() },
        // Player-authored text is visible even on unidentified items.
        { "inscription", item.is_inscribed() ? to_json_utf8(*item.inscription) : "" },
    };

    // Match what the normal item description reveals. An unaware flavor does
    // not reveal its base kind, and an unidentified instance does not reveal
    // charges, pval-derived values, or remaining fuel.
    if (aware) {
        result["sval"] = item.bi_key.sval().value_or(-1);
    }
    if (item.has_identification_flag(IdentificationFlag::SENSE)) {
        result["pseudo_feeling"] = pseudo_feeling_name(static_cast<item_feel_type>(item.feeling));
    }
    if (known) {
        result["charges"] = item.pval;
        result["pval"] = item.pval;
        // Protocol 3: the exact recharge timeout and raw fuel are not printed.
        // The description shows "(charging)" / "(N charging)" and, for
        // non-artifact light sources, "(with N turns of light)" (x2 for the
        // long-life ego); emit exactly those.
        const auto charging_count = calc_displayed_charging_count(item);
        result["charging"] = charging_count > 0;
        if (item.bi_key.tval() == ItemKindType::ROD) {
            result["charging_count"] = charging_count;
        }
        if (const auto light_turns = calc_displayed_lamp_turns(item)) {
            result["light_turns"] = *light_turns;
        }
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
        const auto flags = store_identified ? item.get_flags() : item.get_flags_known();
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
                // Protocol 3: the weapon proficiency list (~d) prints the rank,
                // and the number only while show_actual_value is on.
                const auto exp = exp_it->second[*sval];
                result["weapon_proficiency_rank"] = enum2i(PlayerSkill::weapon_skill_rank(exp));
                if (show_actual_value) {
                    result["weapon_proficiency"] = exp;
                }
            }
        }
    }

    return result;
}

nlohmann::json make_look_json(PlayerType *player_ptr)
{
    const auto &player = *player_ptr;
    const auto &floor = *player.current_floor_ptr;
    const auto is_hallucinated = player.effects()->hallucination().is_active();
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
                if (!is_hallucinated) {
                    // The rest of the look line: "[kills to level up] name (Level N,
                    // damage, attitude, clone)". "level" is already in the monster record.
                    monster_json["clone"] = monster.mflag2.has(MonsterConstantFlagType::CLONED);
                    monster_json["kills_to_level"] = evaluate_monster_exp(player_ptr, monster);
                    monster_json["description"] = to_json_utf8(monster.build_looking_description(true));
                }
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
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param screen 出力する店舗の画面
 */
nlohmann::json make_store_json(PlayerType *player_ptr, const StoreScreen &screen)
{
    const auto &store = screen.get_store();
    const auto store_num = store.get_sale_type();
    auto items = nlohmann::json::array();
    const auto is_personal_storage = store_num == StoreSaleType::HOME || store_num == StoreSaleType::MUSEUM;
    const auto page_top = screen.get_page_top();
    const auto page_item_count = screen.get_page_item_count();
    for (auto page_pos = 0; page_pos < page_item_count; ++page_pos) {
        // The store accepts the item letter RELATIVE to the currently visible
        // page: pressing 'a' selects stock[page_top]. Only items on the
        // current page are selectable, so emit page-relative letters for the
        // current page only. Mirrors display_entry()'s labelling.
        const auto &item = *store.stock[page_top + page_pos];
        const auto letter = (page_pos < 26)
                                ? std::string(1, static_cast<char>('a' + page_pos))
                                : std::string(1, static_cast<char>('A' + (page_pos - 26)));
        if (is_personal_storage) {
            auto entry = make_item_json(player_ptr, item);
            entry["letter"] = letter;
            items.push_back(std::move(entry));
            continue;
        }

        const auto price = price_item(player_ptr, item.calc_price(), store, StoreTradeType::PLAYER_BUYS);
        auto entry = make_item_json(player_ptr, item, true);
        entry["letter"] = letter;
        entry["name"] = to_json_utf8(describe_flavor(player_ptr, item, OD_STORE | OD_OMIT_PREFIX));
        entry["price"] = price;
        items.push_back(std::move(entry));
    }

    // Paging facts the player already reads off the screen: the listing shows
    // one page of `page_size` slots starting at `page_top`, and the store's
    // own prompt tells the player whether more pages follow.  Without them the
    // page count can only be guessed, and a full page (letters running a..Z)
    // is NOT evidence that the stock ends there.
    auto result = nlohmann::json{
        { "store_type", enum2i(store_num) },
        { "stock_num", store.stock_num },
        { "page_top", page_top },
        { "page_size", screen.get_page_size() },
        { "items", items },
    };

    // Header lines of display_store(): a shop shows "owner (race)" and
    // "shop name (purse limit)"; the home and the museum show "Objects: n/capacity".
    if (is_personal_storage) {
        auto capacity = store.stock_size;
        if (store_num == StoreSaleType::HOME && !powerup_home) {
            capacity /= 10;
        }
        result["capacity"] = capacity;
    } else {
        const auto &owner = store.get_owner();
        result["owner_name"] = to_json_utf8(owner.owner_name);
        result["owner_race"] = to_json_utf8(race_info[enum2i(owner.owner_race)].title.string());
        result["store_name"] = to_json_utf8(screen.get_name());
        result["max_cost"] = owner.max_cost;
    }

    return result;
}

/*!
 * @brief キャラクター画面1ページ目の技能評価を出力する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 技能ごとの評価。画面の文字列 (text)・色・評価段階 (rating) を持ち、
 * 表示オプション show_actual_value が有効な場合だけ画面に出る数値 (value) を持つ
 */
nlohmann::json make_skill_ratings_json(PlayerType *player_ptr)
{
    static constexpr std::array<const char *, SKILL_RATING_MAX> keys{ {
        "fighting",
        "shooting",
        "saving_throw",
        "stealth",
        "perception",
        "searching",
        "disarming",
        "magic_device",
        "digging",
    } };
    static constexpr std::array<const char *, 10> ratings{ {
        "very_bad",
        "bad",
        "poor",
        "fair",
        "good",
        "very_good",
        "excellent",
        "superb",
        "heroic",
        "legendary",
    } };
    auto result = nlohmann::json::object();
    const auto sources = calc_skill_rating_sources(player_ptr);
    for (std::size_t i = 0; i < std::min(sources.size(), keys.size()); ++i) {
        const auto &source = sources[i];
        const auto &[text, color] = describe_skill_rating(source.value, source.divisor);
        const auto [rating, legendary_level] = classify_skill_rating(source.value, source.divisor);
        auto row = nlohmann::json{
            { "text", to_json_utf8(text) },
            { "color", term_color_name(color) },
            { "rating", ratings[enum2i(rating)] },
        };
        if (rating == SkillRating::LEGENDARY) {
            row["legendary_level"] = legendary_level;
        }
        if (show_actual_value) {
            row["value"] = source.value;
        }
        result[keys[i]] = std::move(row);
    }

    return result;
}

/*!
 * @brief 画面右下の日付と時刻を出力する
 * @return 日目・時・分。日目は画面が "***" と表示する1000日目以降はnull
 */
nlohmann::json make_clock_json()
{
    const auto &[day, hour, min] = AngbandWorld::get_instance().extract_date_time(InnerGameData::get_instance().get_start_race());
    return {
        { "day", day < 1000 ? nlohmann::json(day) : nlohmann::json(nullptr) },
        { "hour", hour },
        { "minute", min },
    };
}

const char *player_action_name(int action)
{
    switch (action) {
    case ACTION_SEARCH:
        return "search";
    case ACTION_REST:
        return "rest";
    case ACTION_LEARN:
        return "learn";
    case ACTION_FISH:
        return "fish";
    case ACTION_MONK_STANCE:
        return "monk_stance";
    case ACTION_SAMURAI_STANCE:
        return "samurai_stance";
    case ACTION_SING:
        return "sing";
    case ACTION_HAYAGAKE:
        return "hayagake";
    case ACTION_SPELL:
        return "spell";
    default:
        return "none";
    }
}

/*!
 * @brief 画面左下の行動状態欄を出力する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details print_state() と同じく、繰り返し回数があればそれだけが表示されるので、
 * その場合は kind を "repeat" にして持続行動は出さない。
 */
nlohmann::json make_action_state_json(PlayerType *player_ptr)
{
    const auto &[text, color] = describe_player_state(player_ptr);
    auto result = nlohmann::json{
        { "text", to_json_utf8(text) },
        { "color", term_color_name(color) },
    };
    if (command_rep) {
        result["kind"] = "repeat";
        result["repeat_count"] = command_rep;
        return result;
    }

    result["kind"] = player_action_name(player_ptr->action);
    if (player_ptr->action == ACTION_REST) {
        if (player_ptr->resting > 0) {
            result["rest"] = player_ptr->resting;
        } else if (player_ptr->resting == COMMAND_ARG_REST_FULL_HEALING) {
            result["rest"] = "full_healing";
        } else if (player_ptr->resting == COMMAND_ARG_REST_UNTIL_DONE) {
            result["rest"] = "until_done";
        }
    }

    return result;
}

/*!
 * @brief 画面右下の速度欄を出力する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details 色は一時的な加速・減速や騎乗中のモンスターの加減速を表すので、画面と同じ色名を出す。
 */
nlohmann::json make_speed_display_json(PlayerType *player_ptr)
{
    const auto &[text, color] = describe_player_speed(player_ptr);
    return {
        { "value", player_ptr->pspeed - STANDARD_SPEED },
        { "text", to_json_utf8(text) },
        { "color", term_color_name(color) },
        { "riding", player_ptr->riding != 0 },
    };
}

/*!
 * @brief 画面右下の「学習」「まね」表示を出力する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details 同じ位置に print_study() の後で print_imitation() が描くので、ものまね師では
 * ものまねの表示が優先される (print_frame_extra() の描画順)。
 */
nlohmann::json make_study_indicator_json(PlayerType *player_ptr)
{
    PlayerClass pc(player_ptr);
    if (pc.equals(PlayerClassType::IMITATOR)) {
        const auto mane_data = pc.get_specific_data<mane_data_type>();
        if (mane_data->mane_list.empty()) {
            return nullptr;
        }

        return { { "kind", "imitation" }, { "new", mane_data->new_mane } };
    }

    if (player_ptr->new_spells) {
        return { { "kind", "study" }, { "new", false } };
    }

    return nullptr;
}

/*!
 * @brief 次のレベルに必要な経験値 (キャラクター画面の表示) を返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 必要経験値。最高レベルでは画面が "*****" と表示するのでnull
 */
nlohmann::json calc_exp_to_advance(PlayerType *player_ptr)
{
    if (player_ptr->lev >= PY_MAX_LEVEL) {
        return nullptr;
    }

    const auto &table = PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID) ? player_exp_a : player_exp;
    return table[player_ptr->lev - 1] * player_ptr->expfact / 100;
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
 * @param include_map grid_map を含めるか
 * @param messages_source messages に載せるメッセージの選び方
 * @details grid_map はフロア全域を走査して作る処理で、
 * 出力量は既知の地形に依存する。地図を出さない呼び出し
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
        { "protocol_version", BOT_JSON_PROTOCOL_VERSION },
        // Documented exception to the fair-play rule (user decision 2026-09-22,
        // "例外として残す"): the exact game turn is not printed anywhere, but the
        // bot uses it only for decision boundaries and log alignment. The
        // human-visible day and clock are in "clock".
        { "turn", world.game_turn },
        { "clock", make_clock_json() },
        { "player", {
                        { "y", player.y },
                        { "x", player.x },
                        { "can_see_own_grid", !no_lite(player_ptr) },
                        { "light_radius", player.cur_lite },
                        { "level", player.lev },
                        { "hp", player.chp },
                        { "max_hp", player.mhp },
                        { "mp", player.csp },
                        { "max_mp", player.msp },
                        { "exp", player.exp },
                        // The main screen marks drained experience ("x経験" / yellow)
                        // and a drained level ("xレベル" / yellow); the character sheet
                        // prints the maximum experience and the next-level threshold.
                        { "max_exp", player.max_exp },
                        { "exp_drained", player.exp < player.max_exp },
                        { "max_level", player.max_plv },
                        { "level_drained", player.lev < player.max_plv },
                        { "exp_to_advance", calc_exp_to_advance(player_ptr) },
                        { "title", to_json_utf8(get_player_title(player_ptr)) },
                        { "race_title", to_json_utf8(get_displayed_race_title(player_ptr)) },
                        { "mimic_form", enum2i(player.mimic_form) },
                        { "gold", player.au },
                        { "food_state", food_state(player) },
                        { "speed", player.pspeed },
                        { "speed_display", make_speed_display_json(player_ptr) },
                        { "action", make_action_state_json(player_ptr) },
                        { "study", make_study_indicator_json(player_ptr) },
                        { "status_bar", make_status_bar_json(player_ptr) },
                        { "recalling", player.word_recall != 0 },
                        { "food_type", enum2i(PlayerRace(player_ptr).food()) },
                        { "class_id", enum2i(player.pclass) },
                        { "race_id", enum2i(player.prace) },
                        { "personality_id", enum2i(player.ppersonality) },
                        { "ac", player.dis_ac + player.dis_to_a },
                        // Protocol 3: the raw skill numbers are replaced by the ratings the
                        // character sheet prints (numbers only under show_actual_value).
                        // The two-weapon / shield proficiencies moved to knowledge "skill_exp".
                        { "skill_ratings", make_skill_ratings_json(player_ptr) },
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
                       // The map name printed at the bottom right (town, wilderness, dungeon or quest).
                       { "dungeon_name", to_json_utf8(map_name(player_ptr)) },
                       { "level", floor.dun_level },
                       // The grade is player-visible: it selects the depth-display
                       // color, and Ctrl-F reports the distinct grade message.
                       { "feeling", DungeonFeeling::get_instance().get_feeling() },
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
        { "health_bar", make_health_bar_json(player_ptr, false) },
        { "riding_health_bar", make_health_bar_json(player_ptr, true) },
        { "visible_monsters", make_visible_monsters_json(player) },
        { "detected_monsters", make_detected_monsters_json(player) },
        { "messages", (messages_source == BotSnapshotMessages::HISTORY) ? make_message_history_json(BOT_JSON_MAX_MESSAGES) : make_recent_messages_json() },
        { "inventory", make_inventory_json(player_ptr) },
        { "equipment", make_equipment_json(player_ptr) },
    };
    if (include_map) {
        snapshot["grid_map"] = make_grid_map_json(player_ptr);
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
    // Melee lines of the character sheet: one line per attacking hand, labelled
    // bare-handed / two-handed / right / left as display_player_middle() does.
    auto displayed_melee = nlohmann::json::array();
    const auto bare_hand = !has_melee_weapon(player_ptr, INVEN_MAIN_HAND) && !has_melee_weapon(player_ptr, INVEN_SUB_HAND);
    for (auto hand = 0; hand < 2; ++hand) {
        const auto can_attack = (hand == 0) ? can_attack_with_main_hand(player_ptr) : can_attack_with_sub_hand(player_ptr);
        if (!can_attack) {
            continue;
        }

        const auto [hand_to_h, hand_to_d] = calc_displayed_melee_bonus(player_ptr, hand);
        const char *label = (hand == 0) ? (left_hander ? "left_hand" : "right_hand") : (left_hander ? "right_hand" : "left_hand");
        if (bare_hand) {
            label = "bare_hand";
        } else if (has_two_handed_weapons(player_ptr)) {
            label = "two_hands";
        }
        displayed_melee.push_back({ { "hand", hand }, { "label", label }, { "to_h", hand_to_h }, { "to_d", hand_to_d } });
    }
    const auto [bow_to_h, bow_to_d] = calc_displayed_bow_bonus(player_ptr);
    const auto [base_speed, temporary_speed] = calc_displayed_speed(player_ptr);

    // Stat modifier grid (character sheet page "abcdefghijkl@"): one symbol per
    // equipment slot and the '@' column, exactly as display_player_stat_info() draws.
    auto stat_modifiers = nlohmann::json::array();
    for (auto stat = 0; stat < A_MAX; ++stat) {
        auto slots = nlohmann::json::array();
        for (const auto slot : INVEN_WIELDING_SLOTS) {
            const auto symbol = get_equipment_stat_symbol(*player.inventory[slot], stat);
            slots.push_back({ { "slot", equipment_slot_label(slot) }, { "symbol", std::string(1, symbol.character) }, { "color", term_color_name(symbol.color) } });
        }
        const auto intrinsic = get_intrinsic_stat_symbol(player_ptr, stat);
        stat_modifiers.push_back({
            { "stat_id", stat },
            { "equipment", std::move(slots) },
            { "player", { { "symbol", std::string(1, intrinsic.character) }, { "color", term_color_name(intrinsic.color) } } },
        });
    }

    // Curse row of the characteristics page: '+' cursed, '*' permanently cursed, '.' none/unknown.
    auto curse_marks = nlohmann::json::array();
    for (const auto slot : INVEN_WIELDING_SLOTS) {
        curse_marks.push_back({ { "slot", equipment_slot_label(slot) }, { "mark", std::string(1, get_equipment_curse_mark(*player.inventory[slot])) } });
    }

    auto history = nlohmann::json::array();
    for (const auto &line : player.history) {
        history.push_back(to_json_utf8(line));
    }

    const auto &[day, hour, minute] = AngbandWorld::get_instance().extract_date_time(InnerGameData::get_instance().get_start_race());
    PlayerAlignment alignment(player_ptr);
    return {
        // Protocol 3: the skill ratings as printed (numbers only under show_actual_value).
        { "skill_ratings", make_skill_ratings_json(player_ptr) },
        { "name", to_json_utf8(player.name) },
        { "sex", to_json_utf8(sp_ptr->title) },
        { "race_title", to_json_utf8(get_displayed_race_title(player_ptr)) },
        { "class_title", to_json_utf8(cp_ptr->title) },
        { "personality_title", to_json_utf8(ap_ptr->title) },
        { "history", std::move(history) },
        { "displayed_melee", std::move(displayed_melee) },
        { "displayed_shooting", { { "to_h", bow_to_h }, { "to_d", bow_to_d } } },
        { "base_ac", player.dis_ac },
        { "ac_bonus", player.dis_to_a },
        { "speed", {
                       { "base", base_speed - temporary_speed },
                       { "temporary", temporary_speed },
                       { "lightspeed", player.lightspeed != 0 },
                       { "riding", player.riding != 0 },
                   } },
        { "exp", {
                     { "current", player.exp },
                     { "max", PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID) ? nlohmann::json(nullptr) : nlohmann::json(player.max_exp) },
                     { "to_advance", calc_exp_to_advance(player_ptr) },
                 } },
        { "day", day < MAX_DAYS ? nlohmann::json(day) : nlohmann::json(nullptr) },
        { "hour", hour },
        { "minute", minute },
        { "play_time", to_json_utf8(AngbandWorld::get_instance().format_real_playtime()) },
        { "stat_modifiers", std::move(stat_modifiers) },
        { "curse_marks", std::move(curse_marks) },
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
        // Protocol 3: the sheet prints the alignment label, with the number only
        // under show_actual_value.
        { "alignment_label", to_json_utf8(alignment.get_alignment_description()) },
        { "alignment_value", show_actual_value ? nlohmann::json(player.alignment) : nlohmann::json(nullptr) },
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
        "monsters",
        "kill_count",
    };
    static constexpr const char keys[] = { '1', '2', '3', '4', '5', '8', '9', '0', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', '6', '7' };
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
            if ((!cheat_know && !records.has_been_seen(id)) || !monrace->is_valid() || !monrace->should_display(alive)) {
                continue;
            }
            auto row = nlohmann::json{ { "id", enum2i(id) }, { "name", to_json_utf8(monrace->name.string()) }, { "level", monrace->level } };
            // The dead list prints "- level NN - h:mm:ss" (the player's level and play time at the kill).
            if (!alive && monrace->defeat_level && monrace->defeat_time) {
                row["defeat_level"] = monrace->defeat_level;
                row["defeat_time"] = monrace->defeat_time;
            }
            rows.push_back(std::move(row));
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
                // The list prints the rank and a '!' once the class maximum is
                // reached; "exp/max" only under show_actual_value.
                const auto exp = exp_it->second[*sval];
                const auto max = max_it->second[*sval];
                auto row = nlohmann::json{ { "tval", enum2i(tval) }, { "sval", *sval }, { "name", to_json_utf8(baseitem.stripped_name()) },
                    { "at_max", exp >= max }, { "rank", enum2i(PlayerSkill::weapon_skill_rank(exp)) } };
                if (show_actual_value) {
                    row["exp"] = exp;
                    row["max"] = max;
                }
                rows.push_back(std::move(row));
            }
        }
        result["weapons"] = std::move(rows);
        break;
    case BotKnowledgeCategory::SKILL_EXP:
        for (const auto skill : PLAYER_SKILL_KIND_TYPE_RANGE) {
            // Same as do_cmd_knowledge_skill_exp(): rank, '!' at the class
            // maximum, and "exp/max" (exp capped at max) only under show_actual_value.
            const auto exp = player_ptr->skill_exp[skill];
            const auto max = class_skills_info[enum2i(player_ptr->pclass)].s_max[skill];
            const auto rank = (skill == PlayerSkillKindType::RIDING) ? PlayerSkill::riding_skill_rank(exp) : PlayerSkill::weapon_skill_rank(exp);
            auto row = nlohmann::json{ { "id", enum2i(skill) }, { "name", to_json_utf8(PlayerSkill::skill_name(skill)) },
                { "at_max", exp >= max }, { "rank", enum2i(rank) } };
            if (show_actual_value) {
                row["exp"] = std::min(exp, max);
                row["max"] = max;
            }
            rows.push_back(std::move(row));
        }
        result["skills"] = std::move(rows);
        break;
    case BotKnowledgeCategory::VIRTUES:
        // The page prints the alignment line and one sentence per virtue
        // ("You are virtuous in Honour."); numbers only under show_actual_value.
        result["alignment_label"] = to_json_utf8(PlayerAlignment(player_ptr).get_alignment_description());
        result["alignment_value"] = show_actual_value ? nlohmann::json(player_ptr->alignment) : nlohmann::json(nullptr);
        for (auto i = 0; i < 8; ++i) {
            const auto name_it = virtue_names.find(player_ptr->vir_types[i]);
            const auto name = to_json_utf8(name_it != virtue_names.end() ? name_it->second : _("不明", "Oops. No info"));
            rows.push_back({ { "id", enum2i(player_ptr->vir_types[i]) }, { "name", name },
                { "text", to_json_utf8(describe_virtue(player_ptr, i)) },
                { "value", show_actual_value ? nlohmann::json(player_ptr->virtues[i]) : nlohmann::json(nullptr) } });
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
                // Only the first realm's list masks the hissatsu rank ("[--]"), as
                // do_cmd_knowledge_spell_exp() does. The '!' mark is MASTER for the
                // first realm and EXPERT for the second; "exp/max" only under
                // show_actual_value.
                const auto is_hissatsu = (offset == 0) && realm.equals(RealmType::HISSATSU);
                const auto rank = PlayerSkill::spell_skill_rank(exp);
                const auto mark_rank = (offset == 0) ? PlayerSkillRank::MASTER : PlayerSkillRank::EXPERT;
                auto row = nlohmann::json{ { "realm_id", enum2i(realm.to_enum()) }, { "spell_id", spell_id },
                    { "name", to_json_utf8(realm.get_spell_name(spell_id)) },
                    { "rank", is_hissatsu ? nlohmann::json(nullptr) : nlohmann::json(enum2i(rank)) },
                    { "at_max", !is_hissatsu && (rank >= mark_rank) },
                    { "masked", is_hissatsu } };
                if (show_actual_value && !is_hissatsu) {
                    row["exp"] = exp;
                    row["max"] = PlayerSkill::spell_exp_at(PlayerSkillRank::MASTER);
                }
                rows.push_back(std::move(row));
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
    case BotKnowledgeCategory::MONSTERS: {
        // The monster knowledge browser (~6) lists every race the player has
        // seen (all, with cheat_know) with the kill count, or alive/dead for
        // uniques. The recall text of a race is emitted as a "lore" snapshot
        // when the player opens it.
        const auto &records = MonraceRecords::get_instance();
        for (const auto &[id, monrace] : MonraceList::get_instance()) {
            if (!monrace->is_valid() || (!cheat_know && !records.has_been_seen(id))) {
                continue;
            }
            auto row = nlohmann::json{ { "id", enum2i(id) }, { "name", to_json_utf8(monrace->name.string()) } };
            if (monrace->kind_flags.has(MonsterKindType::UNIQUE)) {
                row["dead"] = monrace->is_dead_unique();
            } else {
                row["kills"] = monrace->r_pkills;
            }
            rows.push_back(std::move(row));
        }
        result["monsters"] = std::move(rows);
        break;
    }
    case BotKnowledgeCategory::KILL_COUNT: {
        // Same content as do_cmd_knowledge_kill_count().
        const auto &monraces = MonraceList::get_instance();
        result["total"] = monraces.calc_defeat_count();
        for (const auto monrace_id : monraces.get_valid_monrace_ids()) {
            const auto &monrace = monraces.get_monrace(monrace_id);
            if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
                if (!monrace.is_dead_unique()) {
                    continue;
                }
                auto row = nlohmann::json{ { "id", enum2i(monrace_id) }, { "name", to_json_utf8(monrace.name.string()) }, { "unique", true } };
                if (monrace.defeat_level && monrace.defeat_time) {
                    row["defeat_level"] = monrace.defeat_level;
                    row["defeat_time"] = monrace.defeat_time;
                }
                rows.push_back(std::move(row));
                continue;
            }
            if (monrace.r_pkills <= 0) {
                continue;
            }
            rows.push_back({ { "id", enum2i(monrace_id) }, { "name", to_json_utf8(monrace.name.string()) }, { "unique", false }, { "kills", monrace.r_pkills } });
        }
        result["kills"] = std::move(rows);
        break;
    }
    case BotKnowledgeCategory::MAX:
        break;
    }
    return result;
}

using BotJsonClock = std::chrono::steady_clock;

constexpr std::streamoff BOT_TIMING_MAX_BYTES = 10LL * 1024 * 1024;
constexpr std::size_t BOT_TIMING_BACKUPS = 3;

void validate_timing_path(const std::filesystem::path &path, const std::filesystem::path &json_path)
{
    // Inspect the entry itself: exists() would miss dangling symlinks and an
    // ofstream would follow them when creating/truncating the target.
    std::error_code error;
    const auto status = std::filesystem::symlink_status(path, error);
    if (status.type() == std::filesystem::file_type::not_found) {
        return;
    }
    if (error) {
        throw std::filesystem::filesystem_error("inspect timing path", path, error);
    }
    if (!std::filesystem::is_regular_file(status)) {
        throw std::runtime_error("timing path is not a regular non-symlink file");
    }
    if (std::filesystem::equivalent(path, json_path)) {
        throw std::runtime_error("timing path aliases JSON output");
    }
    if (std::filesystem::hard_link_count(path) > 1) {
        throw std::runtime_error("timing path has multiple hard links");
    }
}

void rotate_timing_log(const std::filesystem::path &path, const std::filesystem::path &json_path)
{
    std::array<std::filesystem::path, BOT_TIMING_BACKUPS + 1> paths;
    paths[0] = path;
    for (std::size_t i = 1; i < paths.size(); ++i) {
        paths[i] = path;
        paths[i] += "." + std::to_string(i);
    }

    // Check every destination before removing/renaming anything. The user may
    // have chosen a backup name (or an alias of it) for the live JSON stream.
    for (const auto &candidate : paths) {
        validate_timing_path(candidate, json_path);
    }

    std::filesystem::remove(paths.back());
    for (std::size_t i = BOT_TIMING_BACKUPS; i > 0; --i) {
        if (std::filesystem::exists(paths[i - 1])) {
            std::filesystem::rename(paths[i - 1], paths[i]);
        }
    }
}

std::int64_t elapsed_microseconds(BotJsonClock::time_point start)
{
    return std::chrono::duration_cast<std::chrono::microseconds>(BotJsonClock::now() - start).count();
}

void write_snapshot(const nlohmann::json &snapshot, std::int64_t build_us)
{
    const auto dump_started = BotJsonClock::now();
    const auto serialized = snapshot.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
    const auto dump_us = elapsed_microseconds(dump_started);
    if (arg_bot_json_output_path == "-") {
        std::cout << serialized << '\n';
        std::cout.flush();
        return;
    }

    static std::ofstream ofs;
    static std::string opened_path;
    static unsigned int output_open_attempts = 0;
    static bool output_disabled = false;
    static std::ofstream timing_ofs;
    static std::filesystem::path timing_path;
    static unsigned int timing_open_attempts = 0;
    static bool timing_disabled = false;
    if (!output_disabled && (!ofs.is_open() || opened_path != arg_bot_json_output_path)) {
        ofs.close();
        ofs.clear();
        timing_ofs.close();
        timing_ofs.clear();
        opened_path = arg_bot_json_output_path;
        if (arg_bot_json_timing && !timing_disabled) {
            try {
                timing_path = std::filesystem::absolute(std::filesystem::path(opened_path));
                timing_path += ".timing.log";
            } catch (const std::filesystem::filesystem_error &error) {
                std::cerr << "BOT JSON timing disabled: " << error.what() << '\n';
                timing_disabled = true;
            }
        }
        // Truncate at session start so the log does not grow without bound
        // across runs (full-map snapshots are large); the client tails it live.
        ofs.open(opened_path, std::ios::out | std::ios::trunc);
        if (ofs) {
            output_open_attempts = 0;
        } else if (++output_open_attempts >= 2) {
            std::cerr << "BOT JSON output disabled after two consecutive failed opens: " << opened_path << '\n';
            output_disabled = true;
        }
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
            if (++output_open_attempts >= 2) {
                std::cerr << "BOT JSON output disabled after two consecutive failed opens: " << opened_path << '\n';
                output_disabled = true;
            }
            return;
        }
        output_open_attempts = 0;
    }

    const auto write_started = BotJsonClock::now();
    ofs << serialized << '\n';
    ofs.flush();
    const auto write_us = elapsed_microseconds(write_started);

    if (!arg_bot_json_timing || timing_disabled) {
        return;
    }

    if (!timing_disabled && !timing_ofs.is_open()) {
        // The JSON stream is already open, so equivalent() also detects case
        // aliases on Windows and existing hard/symbolic links to the same file.
        try {
            validate_timing_path(timing_path, opened_path);
        } catch (const std::exception &error) {
            std::cerr << "BOT JSON timing disabled: " << error.what() << '\n';
            timing_disabled = true;
        }
        if (!timing_disabled) {
            timing_ofs.open(timing_path, std::ios::out | std::ios::trunc);
            if (timing_ofs) {
                timing_open_attempts = 0;
            } else if (++timing_open_attempts >= 2) {
                std::cerr << "BOT JSON timing disabled after two failed opens: " << timing_path << '\n';
                timing_disabled = true;
            }
        }
    }
    if (!timing_disabled && timing_ofs.is_open() && timing_ofs) {
        if (timing_ofs.tellp() >= BOT_TIMING_MAX_BYTES) {
            timing_ofs.close();
            try {
                if (!timing_ofs) {
                    throw std::runtime_error("failed to close timing log");
                }
                rotate_timing_log(timing_path, opened_path);
                validate_timing_path(timing_path, opened_path);
                timing_ofs.open(timing_path, std::ios::out | std::ios::trunc);
                if (!timing_ofs) {
                    throw std::runtime_error("failed to reopen timing log");
                }
            } catch (const std::exception &error) {
                std::cerr << "BOT JSON timing disabled: " << error.what() << '\n';
                timing_disabled = true;
                return;
            }
        }
        const auto snapshot_type = snapshot.contains("type") ? snapshot.at("type").get<std::string>() : "player_turn";
        const auto snapshot_turn = snapshot.contains("turn") ? snapshot.at("turn") : nlohmann::json(0);
        timing_ofs << "type=" << snapshot_type << " turn=" << snapshot_turn
                   << " build_us=" << build_us << " dump_us=" << dump_us << " write_us=" << write_us
                   << " bytes=" << serialized.size() << '\n';
        timing_ofs.flush();
        if (!timing_ofs) {
            std::cerr << "BOT JSON timing disabled: failed to write timing log\n";
            timing_disabled = true;
            timing_ofs.close();
        }
    }
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
 * @param include_map grid_mapを含めるか
 * @pre player_ptrとplayer_ptr->current_floor_ptrがnullptrでないことを呼び出し側が保証すること
 * @return スナップショットのJSONオブジェクト
 * @details
 * --bot-json-outputの有無に関わらず生成する。制御サーバから利用する。
 * messagesは差分ではなく履歴を載せる。リクエストの度に差分を進めると、
 * --bot-json-outputと併用した際にJSONL側からメッセージが失われるためである。
 */
nlohmann::json make_bot_json_snapshot(PlayerType *player_ptr, bool include_map)
{
    return make_snapshot(player_ptr, include_map, BotSnapshotMessages::HISTORY);
}

void output_bot_json_snapshot(PlayerType *player_ptr)
{
    if (!arg_bot_json_output || player_ptr == nullptr || player_ptr->current_floor_ptr == nullptr) {
        return;
    }

    const auto build_started = BotJsonClock::now();
    auto snapshot = make_snapshot(player_ptr);
    write_snapshot(snapshot, elapsed_microseconds(build_started));
}

void output_bot_json_store_snapshot(PlayerType *player_ptr, const StoreScreen &screen)
{
    if (!arg_bot_json_output || player_ptr == nullptr || player_ptr->current_floor_ptr == nullptr) {
        return;
    }

    // Same base snapshot (player gold, inventory, equipment) plus the store's
    // stock, so the bot can decide what to buy while at the store prompt.
    //
    // The map is deliberately omitted here.  At the store prompt the player
    // cannot move and the floor cannot change, so grid_map would repeat the
    // surface snapshot's map verbatim. A store
    // session emits one snapshot per processed key (see cmd-store.cpp), so
    // paging through a large inventory multiplies that cost by the key count.
    // This reveals nothing new to the bot; it only stops re-sending what the
    // preceding surface snapshot already carried.
    const auto build_started = BotJsonClock::now();
    auto snapshot = make_snapshot(player_ptr, false);
    snapshot["type"] = "store";
    snapshot["store"] = make_store_json(player_ptr, screen);
    write_snapshot(snapshot, elapsed_microseconds(build_started));
}

void output_bot_json_character_snapshot(PlayerType *player_ptr)
{
    if (!arg_bot_json_output || player_ptr == nullptr || player_ptr->current_floor_ptr == nullptr) {
        return;
    }
    const auto build_started = BotJsonClock::now();
    auto snapshot = make_snapshot(player_ptr);
    snapshot["type"] = "character";
    snapshot["character"] = make_character_json(player_ptr);
    write_snapshot(snapshot, elapsed_microseconds(build_started));
}

void output_bot_json_knowledge_snapshot(PlayerType *player_ptr, BotKnowledgeCategory category)
{
    if (!arg_bot_json_output || player_ptr == nullptr || player_ptr->current_floor_ptr == nullptr) {
        return;
    }
    const auto build_started = BotJsonClock::now();
    auto snapshot = make_snapshot(player_ptr);
    snapshot["type"] = "knowledge";
    snapshot["knowledge"] = make_knowledge_json(player_ptr, category);
    write_snapshot(snapshot, elapsed_microseconds(build_started));
}

void output_bot_json_look_snapshot(PlayerType *player_ptr)
{
    if (!arg_bot_json_output || player_ptr == nullptr || player_ptr->current_floor_ptr == nullptr) {
        return;
    }
    const auto build_started = BotJsonClock::now();
    auto snapshot = make_snapshot(player_ptr);
    snapshot["type"] = "look";
    snapshot["look"] = make_look_json(player_ptr);
    write_snapshot(snapshot, elapsed_microseconds(build_started));
}

/*!
 * @brief 呪文一覧 (魔法書の閲覧・詠唱メニュー) を表示したときのスナップショットを出力する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param realm_id 表示している領域
 * @param rows print_spells() が画面に書いた各行
 */
void output_bot_json_spell_list_snapshot(PlayerType *player_ptr, int realm_id, const std::vector<BotSpellListRow> &rows)
{
    if (!arg_bot_json_output || player_ptr == nullptr || player_ptr->current_floor_ptr == nullptr) {
        return;
    }
    const auto build_started = BotJsonClock::now();
    auto spells = nlohmann::json::array();
    for (const auto &row : rows) {
        auto spell = nlohmann::json{ { "spell_id", row.spell_id }, { "status", row.status } };
        if (row.status != "illegible") {
            spell["name"] = to_json_utf8(row.name);
            spell["level"] = row.level;
            spell["mana"] = row.mana;
        }
        if (row.fail) {
            spell["fail"] = *row.fail;
            spell["proficiency"] = to_json_utf8(row.proficiency);
            spell["proficiency_mark"] = row.proficiency_mark;
            spell["info"] = to_json_utf8(row.info);
        }
        spells.push_back(std::move(spell));
    }

    auto snapshot = make_snapshot(player_ptr, false);
    snapshot["type"] = "spell_list";
    snapshot["spell_list"] = { { "realm_id", realm_id }, { "spells", std::move(spells) } };
    write_snapshot(snapshot, elapsed_microseconds(build_started));
}

/*!
 * @brief 特殊能力の一覧を表示したときのスナップショットを出力する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param kind 一覧の種類 ("racial": 種族・職業・突然変異のパワー、"mind": 超能力・練気術・剣術等)
 * @param rows 一覧の全行 (ページ送りで見られる行を含む)
 * @param page 表示中のページ
 * @param browse_mode 閲覧モードか
 */
void output_bot_json_power_list_snapshot(PlayerType *player_ptr, std::string_view kind, const std::vector<BotPowerListRow> &rows, int page, bool browse_mode)
{
    if (!arg_bot_json_output || player_ptr == nullptr || player_ptr->current_floor_ptr == nullptr) {
        return;
    }
    const auto build_started = BotJsonClock::now();
    auto powers = nlohmann::json::array();
    for (const auto &row : rows) {
        powers.push_back({
            { "letter", std::string(1, row.letter) },
            { "page", row.page },
            { "name", to_json_utf8(row.name) },
            { "level", row.level },
            { "cost", row.cost },
            { "fail", row.fail },
            { "info", to_json_utf8(row.info) },
        });
    }

    auto snapshot = make_snapshot(player_ptr, false);
    snapshot["type"] = "power_list";
    snapshot["power_list"] = { { "kind", kind }, { "page", page }, { "browse_mode", browse_mode }, { "powers", std::move(powers) } };
    write_snapshot(snapshot, elapsed_microseconds(build_started));
}

namespace {
std::string *lore_capture_buffer = nullptr;

void capture_lore_text(TERM_COLOR, std::string_view str)
{
    if (lore_capture_buffer != nullptr) {
        lore_capture_buffer->append(str);
    }
}
}

/*!
 * @brief モンスターの思い出を表示したときのスナップショットを出力する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monrace_id 表示するモンスターの種族ID
 * @param lore_mode 表示モード (monster_lore_mode)
 * @details 画面と同じ process_monster_lore() の出力を、描画先だけ差し替えて文字列として取り出す。
 * 思い出の文章は乱数や状態変更を伴わないので、画面の描画の前に一度余分に組み立てても結果は変わらない。
 */
void output_bot_json_lore_snapshot(PlayerType *player_ptr, MonraceId monrace_id, int lore_mode)
{
    if (!arg_bot_json_output || player_ptr == nullptr || player_ptr->current_floor_ptr == nullptr) {
        return;
    }
    const auto build_started = BotJsonClock::now();
    std::string text;
    const auto previous_hook = hook_c_roff;
    lore_capture_buffer = &text;
    hook_c_roff = capture_lore_text;
    process_monster_lore(player_ptr, monrace_id, static_cast<monster_lore_mode>(lore_mode));
    hook_c_roff = previous_hook;
    lore_capture_buffer = nullptr;

    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    auto snapshot = make_snapshot(player_ptr, false);
    snapshot["type"] = "lore";
    snapshot["lore"] = {
        { "race_id", enum2i(monrace_id) },
        { "name", to_json_utf8(monrace.name.string()) },
        { "text", to_json_utf8(text) },
    };
    write_snapshot(snapshot, elapsed_microseconds(build_started));
}
