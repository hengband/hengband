#include "floor/floor-events.h"
#include "cmd-io/cmd-dump.h"
#include "core/disturbance.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/dungeon-feeling.h"
#include "floor/geometry.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/disturbance-options.h"
#include "game-option/map-screen-options.h"
#include "grid/grid.h"
#include "main/sound-of-music.h"
#include "mind/mind-ninja.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-status.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-mark-types.h"
#include "object/object-value.h"
#include "object/tval-types.h"
#include "perception/object-perception.h"
#include "player/special-defense-types.h"
#include "sv-definition/sv-amulet-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-ring-types.h"
#include "system/angband-system.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

static void update_sun_light(PlayerType *player_ptr)
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::MONSTER_STATUSES,
        StatusRecalculatingFlag::MONSTER_LITE,
    };
    rfu.set_flags(flags_srf);
    rfu.set_flag(MainWindowRedrawingFlag::MAP);
    static constexpr auto flags = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    rfu.set_flags(flags);
    if ((player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].info & CAVE_GLOW) != 0) {
        set_superstealth(player_ptr, false);
    }
}

void day_break(PlayerType *player_ptr)
{
    msg_print(_("夜が明けた。", "The sun has risen."));
    if (AngbandWorld::get_instance().is_wild_mode()) {
        update_sun_light(player_ptr);
        return;
    }

    auto &floor = *player_ptr->current_floor_ptr;
    for (auto y = 0; y < floor.height; y++) {
        for (auto x = 0; x < floor.width; x++) {
            auto &grid = floor.get_grid({ y, x });
            grid.add_info(CAVE_GLOW);
            if (view_perma_grids) {
                grid.add_info(CAVE_MARK);
            }

            note_spot(player_ptr, y, x);
        }
    }

    update_sun_light(player_ptr);
}

void night_falls(PlayerType *player_ptr)
{
    msg_print(_("日が沈んだ。", "The sun has fallen."));
    if (AngbandWorld::get_instance().is_wild_mode()) {
        update_sun_light(player_ptr);
        return;
    }

    auto &floor = *player_ptr->current_floor_ptr;
    for (auto y = 0; y < floor.height; y++) {
        for (auto x = 0; x < floor.width; x++) {
            const Pos2D pos(y, x);
            auto &grid = floor.get_grid(pos);
            const auto &terrain = grid.get_terrain(TerrainKind::MIMIC);
            using Tc = TerrainCharacteristics;
            if (grid.is_mirror() || terrain.flags.has(Tc::QUEST_ENTER) || terrain.flags.has(Tc::ENTRANCE)) {
                continue;
            }

            grid.info &= ~(CAVE_GLOW);
            if (terrain.flags.has_not(Tc::REMEMBER)) {
                grid.info &= ~(CAVE_MARK);
                note_spot(player_ptr, y, x);
            }
        }

        glow_deep_lava_and_bldg(player_ptr);
    }

    update_sun_light(player_ptr);
}

/*!
 * ダンジョンの雰囲気を計算するための非線形基準値 / Dungeon rating is no longer linear
 */
static int rating_boost(int delta)
{
    return delta * delta + 50 * delta;
}

/*!
 * @brief ダンジョンの雰囲気を算出する
 * @param floor フロアへの参照
 * @return 算出されたダンジョンの雰囲気ランク
 */
static int get_dungeon_feeling(const auto &floor)
{
    if (!floor.is_underground()) {
        return 0;
    }

    const auto base = 10;
    auto rating = 0;
    for (short i = 1; i < floor.m_max; i++) {
        const auto &monster = floor.m_list[i];
        auto delta = 0;
        if (!monster.is_valid() || monster.is_pet()) {
            continue;
        }

        const auto &monrace = monster.get_monrace();
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            if (monrace.level + 10 > floor.dun_level) {
                delta += (monrace.level + 10 - floor.dun_level) * 2 * base;
            }
        } else if (monrace.level > floor.dun_level) {
            delta += (monrace.level - floor.dun_level) * base;
        }

        if (monrace.misc_flags.has(MonsterMiscType::HAS_FRIENDS)) {
            if (5 <= get_monster_crowd_number(floor, i)) {
                delta += 1;
            }
        } else if (2 <= get_monster_crowd_number(floor, i)) {
            delta += 1;
        }

        rating += rating_boost(delta);
    }

    for (short i = 1; i < floor.o_max; i++) {
        const auto &item = floor.o_list[i];
        auto delta = 0;
        if (!item.is_valid() || (item.is_known() && item.marked.has(OmType::TOUCHED)) || ((item.ident & IDENT_SENSE) != 0)) {
            continue;
        }

        if (item.is_ego()) {
            const auto &ego = item.get_ego();
            delta += ego.rating * base;
        }

        if (item.is_fixed_or_random_artifact()) {
            const auto cost = object_value_real(&item);
            delta += 10 * base;
            if (cost > 10000L) {
                delta += 10 * base;
            }

            if (cost > 50000L) {
                delta += 10 * base;
            }

            if (cost > 100000L) {
                delta += 10 * base;
            }

            if (!preserve_mode) {
                return 1;
            }
        }

        if (item.bi_key.tval() == ItemKindType::DRAG_ARMOR) {
            delta += 30 * base;
        }

        if (item.bi_key == BaseitemKey(ItemKindType::SHIELD, SV_DRAGON_SHIELD)) {
            delta += 5 * base;
        }

        if (item.bi_key == BaseitemKey(ItemKindType::GLOVES, SV_SET_OF_DRAGON_GLOVES)) {
            delta += 5 * base;
        }

        if (item.bi_key == BaseitemKey(ItemKindType::BOOTS, SV_PAIR_OF_DRAGON_GREAVE)) {
            delta += 5 * base;
        }

        if (item.bi_key == BaseitemKey(ItemKindType::HELM, SV_DRAGON_HELM)) {
            delta += 5 * base;
        }

        if (item.bi_key == BaseitemKey(ItemKindType::RING, SV_RING_SPEED) && !item.is_cursed()) {
            delta += 25 * base;
        }

        if (item.bi_key == BaseitemKey(ItemKindType::RING, SV_RING_LORDLY) && !item.is_cursed()) {
            delta += 15 * base;
        }

        if (item.bi_key == BaseitemKey(ItemKindType::AMULET, SV_AMULET_THE_MAGI) && !item.is_cursed()) {
            delta += 15 * base;
        }

        const auto item_level = item.get_baseitem_level();
        if (!item.is_cursed() && !item.is_broken() && item_level > floor.dun_level) {
            delta += (item_level - floor.dun_level) * base;
        }

        rating += rating_boost(delta);
    }

    if (rating > rating_boost(1000)) {
        return 2;
    }

    if (rating > rating_boost(800)) {
        return 3;
    }

    if (rating > rating_boost(600)) {
        return 4;
    }

    if (rating > rating_boost(400)) {
        return 5;
    }

    if (rating > rating_boost(300)) {
        return 6;
    }

    if (rating > rating_boost(200)) {
        return 7;
    }

    if (rating > rating_boost(100)) {
        return 8;
    }

    if (rating > rating_boost(0)) {
        return 9;
    }

    return 10;
}

/*!
 * @brief ダンジョンの雰囲気を更新し、変化があった場合メッセージを表示する
 * / Update dungeon feeling, and announce it if changed
 */
void update_dungeon_feeling(PlayerType *player_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    if (!floor.is_underground()) {
        return;
    }

    if (AngbandSystem::get_instance().is_phase_out()) {
        return;
    }

    const auto delay = std::max(10, 150 - player_ptr->skill_fos) * (150 - floor.dun_level) * TURNS_PER_TICK / 100;
    const auto &world = AngbandWorld::get_instance();
    auto &df = DungeonFeeling::get_instance();
    if (world.game_turn < df.get_turns() + delay && !cheat_xtra) {
        return;
    }

    const auto quest_id = floor.get_quest_id();
    const auto &quests = QuestList::get_instance();

    auto dungeon_quest = (quest_id == QuestId::OBERON);
    dungeon_quest |= (quest_id == QuestId::SERPENT);
    dungeon_quest |= none_bits(quests.get_quest(quest_id).flags, QUEST_FLAG_PRESET);

    auto feeling_quest = inside_quest(quest_id);
    feeling_quest &= QuestType::is_fixed(quest_id);
    feeling_quest &= !dungeon_quest;
    if (feeling_quest) {
        return;
    }

    const auto new_feeling = get_dungeon_feeling(floor);
    df.set_turns(world.game_turn);
    if (df.get_feeling() == new_feeling) {
        return;
    }

    df.set_feeling(new_feeling);
    do_cmd_feeling(player_ptr);
    select_floor_music(player_ptr);
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::DEPTH);
    if (disturb_minor) {
        disturb(player_ptr, false, false);
    }
}

/*
 * Glow deep lava and building entrances in the floor
 */
void glow_deep_lava_and_bldg(PlayerType *player_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        return;
    }

    for (auto y = 0; y < floor.height; y++) {
        for (auto x = 0; x < floor.width; x++) {
            const Pos2D pos(y, x);
            const auto &grid = floor.get_grid(pos);
            if (grid.get_terrain(TerrainKind::MIMIC).flags.has_not(TerrainCharacteristics::GLOW)) {
                continue;
            }

            for (const auto &d : Direction::directions()) {
                const auto pos_neighbor = pos + d.vec();
                if (!in_bounds2(floor, pos_neighbor.y, pos_neighbor.x)) {
                    continue;
                }

                floor.get_grid(pos_neighbor).info |= CAVE_GLOW;
            }
        }
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::VIEW,
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::MONSTER_LITE,
    };
    rfu.set_flags(flags_srf);
    rfu.set_flag(MainWindowRedrawingFlag::MAP);
}

/*
 * Actually erase the entire "lite" array, redrawing every grid
 */
void forget_lite(FloorType &floor)
{
    if (!floor.lite_n) {
        return;
    }

    for (int i = 0; i < floor.lite_n; i++) {
        POSITION y = floor.lite_y[i];
        POSITION x = floor.lite_x[i];
        floor.grid_array[y][x].info &= ~(CAVE_LITE);
    }

    floor.lite_n = 0;
}

/*
 * Clear the viewable space
 */
void forget_view(FloorType &floor)
{
    if (!floor.view_n) {
        return;
    }

    for (int i = 0; i < floor.view_n; i++) {
        POSITION y = floor.view_y[i];
        POSITION x = floor.view_x[i];
        auto &grid = floor.grid_array[y][x];
        grid.info &= ~(CAVE_VIEW);
    }

    floor.view_n = 0;
}
