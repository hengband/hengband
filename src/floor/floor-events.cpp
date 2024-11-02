#include "floor/floor-events.h"
#include "cmd-io/cmd-dump.h"
#include "core/disturbance.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/floor-list.h"
#include "floor/geometry.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/disturbance-options.h"
#include "game-option/map-screen-options.h"
#include "grid/feature-flag-types.h"
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
#include "system/baseitem-info.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain-type-definition.h"
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
    const auto &floor = FloorList::get_instance().get_floor(0);
    if (any_bits(floor.grid_array[player_ptr->y][player_ptr->x].info, CAVE_GLOW)) {
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

    auto &floor = FloorList::get_instance().get_floor(0);
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

    auto &floor = FloorList::get_instance().get_floor(0);
    for (auto y = 0; y < floor.height; y++) {
        for (auto x = 0; x < floor.width; x++) {
            const Pos2D pos(y, x);
            auto &grid = floor.get_grid(pos);
            const auto &terrain = grid.get_terrain_mimic();
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

        glow_deep_lava_and_bldg();
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
 * @brief ダンジョンの雰囲気を算出する。
 * / Examine all monsters and unidentified objects, and get the feeling of current dungeon floor
 * @return 算出されたダンジョンの雰囲気ランク
 */
static byte get_dungeon_feeling()
{
    auto *floor_ptr = &FloorList::get_instance().get_floor(0);
    if (!floor_ptr->dun_level) {
        return 0;
    }

    const int base = 10;
    int rating = 0;
    for (MONSTER_IDX i = 1; i < floor_ptr->m_max; i++) {
        auto *m_ptr = &floor_ptr->m_list[i];
        MonsterRaceInfo *r_ptr;
        int delta = 0;
        if (!m_ptr->is_valid() || m_ptr->is_pet()) {
            continue;
        }

        r_ptr = &m_ptr->get_monrace();
        if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
            if (r_ptr->level + 10 > floor_ptr->dun_level) {
                delta += (r_ptr->level + 10 - floor_ptr->dun_level) * 2 * base;
            }
        } else if (r_ptr->level > floor_ptr->dun_level) {
            delta += (r_ptr->level - floor_ptr->dun_level) * base;
        }

        if (r_ptr->misc_flags.has(MonsterMiscType::HAS_FRIENDS)) {
            if (5 <= get_monster_crowd_number(floor_ptr, i)) {
                delta += 1;
            }
        } else if (2 <= get_monster_crowd_number(floor_ptr, i)) {
            delta += 1;
        }

        rating += rating_boost(delta);
    }

    for (MONSTER_IDX i = 1; i < floor_ptr->o_max; i++) {
        auto *o_ptr = &floor_ptr->o_list[i];
        int delta = 0;
        if (!o_ptr->is_valid() || (o_ptr->is_known() && o_ptr->marked.has(OmType::TOUCHED)) || ((o_ptr->ident & IDENT_SENSE) != 0)) {
            continue;
        }

        if (o_ptr->is_ego()) {
            const auto &ego = o_ptr->get_ego();
            delta += ego.rating * base;
        }

        if (o_ptr->is_fixed_or_random_artifact()) {
            PRICE cost = object_value_real(o_ptr);
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

        if (o_ptr->bi_key.tval() == ItemKindType::DRAG_ARMOR) {
            delta += 30 * base;
        }

        if (o_ptr->bi_key == BaseitemKey(ItemKindType::SHIELD, SV_DRAGON_SHIELD)) {
            delta += 5 * base;
        }

        if (o_ptr->bi_key == BaseitemKey(ItemKindType::GLOVES, SV_SET_OF_DRAGON_GLOVES)) {
            delta += 5 * base;
        }

        if (o_ptr->bi_key == BaseitemKey(ItemKindType::BOOTS, SV_PAIR_OF_DRAGON_GREAVE)) {
            delta += 5 * base;
        }

        if (o_ptr->bi_key == BaseitemKey(ItemKindType::HELM, SV_DRAGON_HELM)) {
            delta += 5 * base;
        }

        if (o_ptr->bi_key == BaseitemKey(ItemKindType::RING, SV_RING_SPEED) && !o_ptr->is_cursed()) {
            delta += 25 * base;
        }

        if (o_ptr->bi_key == BaseitemKey(ItemKindType::RING, SV_RING_LORDLY) && !o_ptr->is_cursed()) {
            delta += 15 * base;
        }

        if (o_ptr->bi_key == BaseitemKey(ItemKindType::AMULET, SV_AMULET_THE_MAGI) && !o_ptr->is_cursed()) {
            delta += 15 * base;
        }

        const auto &baseitem = o_ptr->get_baseitem();
        if (!o_ptr->is_cursed() && !o_ptr->is_broken() && baseitem.level > floor_ptr->dun_level) {
            delta += (baseitem.level - floor_ptr->dun_level) * base;
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
    const auto &floor = FloorList::get_instance().get_floor(0);
    if (!floor.dun_level) {
        return;
    }

    if (AngbandSystem::get_instance().is_phase_out()) {
        return;
    }

    const auto delay = std::max(10, 150 - player_ptr->skill_fos) * (150 - floor.dun_level) * TURNS_PER_TICK / 100;
    const auto &world = AngbandWorld::get_instance();
    if (world.game_turn < player_ptr->feeling_turn + delay && !cheat_xtra) {
        return;
    }

    const auto quest_id = floor.get_quest_id();
    const auto &quests = QuestList::get_instance();

    auto dungeon_quest = (quest_id == QuestId::OBERON);
    dungeon_quest |= (quest_id == QuestId::SERPENT);
    dungeon_quest |= !(quests.get_quest(quest_id).flags & QUEST_FLAG_PRESET);

    auto feeling_quest = inside_quest(quest_id);
    feeling_quest &= QuestType::is_fixed(quest_id);
    feeling_quest &= !dungeon_quest;
    if (feeling_quest) {
        return;
    }
    byte new_feeling = get_dungeon_feeling();
    player_ptr->feeling_turn = world.game_turn;
    if (player_ptr->feeling == new_feeling) {
        return;
    }

    player_ptr->feeling = new_feeling;
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
void glow_deep_lava_and_bldg()
{
    auto &floor = FloorList::get_instance().get_floor(0);
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        return;
    }

    for (auto y = 0; y < floor.height; y++) {
        for (auto x = 0; x < floor.width; x++) {
            const auto &grid = floor.get_grid({ y, x });
            if (grid.get_terrain_mimic().flags.has_not(TerrainCharacteristics::GLOW)) {
                continue;
            }

            for (auto i = 0; i < 9; i++) {
                const Pos2D pos(y + ddy_ddd[i], x + ddx_ddd[i]);
                if (!in_bounds2(&floor, pos.y, pos.x)) {
                    continue;
                }

                floor.get_grid(pos).info |= CAVE_GLOW;
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
void forget_lite(FloorType *floor_ptr)
{
    if (!floor_ptr->lite_n) {
        return;
    }

    for (int i = 0; i < floor_ptr->lite_n; i++) {
        POSITION y = floor_ptr->lite_y[i];
        POSITION x = floor_ptr->lite_x[i];
        floor_ptr->grid_array[y][x].info &= ~(CAVE_LITE);
    }

    floor_ptr->lite_n = 0;
}

/*
 * Clear the viewable space
 */
void forget_view(FloorType *floor_ptr)
{
    if (!floor_ptr->view_n) {
        return;
    }

    for (int i = 0; i < floor_ptr->view_n; i++) {
        POSITION y = floor_ptr->view_y[i];
        POSITION x = floor_ptr->view_x[i];
        Grid *g_ptr;
        g_ptr = &floor_ptr->grid_array[y][x];
        g_ptr->info &= ~(CAVE_VIEW);
    }

    floor_ptr->view_n = 0;
}
