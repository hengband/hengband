#include "floor/floor-events.h"
#include "cmd-io/cmd-dump.h"
#include "core/disturbance.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/disturbance-options.h"
#include "game-option/map-screen-options.h"
#include "grid/feature-flag-types.h"
#include "grid/grid.h"
#include "main/sound-of-music.h"
#include "mind/mind-ninja.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
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
    if ((player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].info & CAVE_GLOW) != 0) {
        set_superstealth(player_ptr, false);
    }
}

void day_break(PlayerType *player_ptr)
{
    msg_print(_("夜が明けた。", "The sun has risen."));
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (player_ptr->wild_mode) {
        update_sun_light(player_ptr);
        return;
    }

    for (auto y = 0; y < floor_ptr->height; y++) {
        for (auto x = 0; x < floor_ptr->width; x++) {
            auto *g_ptr = &floor_ptr->grid_array[y][x];
            g_ptr->info |= CAVE_GLOW;
            if (view_perma_grids) {
                g_ptr->info |= CAVE_MARK;
            }

            note_spot(player_ptr, y, x);
        }
    }

    update_sun_light(player_ptr);
}

void night_falls(PlayerType *player_ptr)
{
    msg_print(_("日が沈んだ。", "The sun has fallen."));
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (player_ptr->wild_mode) {
        update_sun_light(player_ptr);
        return;
    }

    for (auto y = 0; y < floor_ptr->height; y++) {
        for (auto x = 0; x < floor_ptr->width; x++) {
            auto *g_ptr = &floor_ptr->grid_array[y][x];
            auto *f_ptr = &terrains_info[g_ptr->get_feat_mimic()];
            using Tc = TerrainCharacteristics;
            if (g_ptr->is_mirror() || f_ptr->flags.has(Tc::QUEST_ENTER) || f_ptr->flags.has(Tc::ENTRANCE)) {
                continue;
            }

            g_ptr->info &= ~(CAVE_GLOW);
            if (f_ptr->flags.has_not(Tc::REMEMBER)) {
                g_ptr->info &= ~(CAVE_MARK);
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
 * @brief ダンジョンの雰囲気を算出する。
 * / Examine all monsters and unidentified objects, and get the feeling of current dungeon floor
 * @return 算出されたダンジョンの雰囲気ランク
 */
static byte get_dungeon_feeling(PlayerType *player_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
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

        r_ptr = &monraces_info[m_ptr->r_idx];
        if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
            if (r_ptr->level + 10 > floor_ptr->dun_level) {
                delta += (r_ptr->level + 10 - floor_ptr->dun_level) * 2 * base;
            }
        } else if (r_ptr->level > floor_ptr->dun_level) {
            delta += (r_ptr->level - floor_ptr->dun_level) * base;
        }

        if (r_ptr->flags1 & RF1_FRIENDS) {
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
    const auto &floor = *player_ptr->current_floor_ptr;
    if (!floor.dun_level) {
        return;
    }

    if (player_ptr->phase_out) {
        return;
    }

    int delay = std::max(10, 150 - player_ptr->skill_fos) * (150 - floor.dun_level) * TURNS_PER_TICK / 100;
    if (w_ptr->game_turn < player_ptr->feeling_turn + delay && !cheat_xtra) {
        return;
    }

    auto quest_num = floor.get_quest_id();
    const auto &quest_list = QuestList::get_instance();

    auto dungeon_quest = (quest_num == QuestId::OBERON);
    dungeon_quest |= (quest_num == QuestId::SERPENT);
    dungeon_quest |= !(quest_list[quest_num].flags & QUEST_FLAG_PRESET);

    auto feeling_quest = inside_quest(quest_num);
    feeling_quest &= QuestType::is_fixed(quest_num);
    feeling_quest &= !dungeon_quest;
    if (feeling_quest) {
        return;
    }
    byte new_feeling = get_dungeon_feeling(player_ptr);
    player_ptr->feeling_turn = w_ptr->game_turn;
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
void glow_deep_lava_and_bldg(PlayerType *player_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (floor_ptr->get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        return;
    }

    for (POSITION y = 0; y < floor_ptr->height; y++) {
        for (POSITION x = 0; x < floor_ptr->width; x++) {
            grid_type *g_ptr;
            g_ptr = &floor_ptr->grid_array[y][x];
            if (terrains_info[g_ptr->get_feat_mimic()].flags.has_not(TerrainCharacteristics::GLOW)) {
                continue;
            }

            for (DIRECTION i = 0; i < 9; i++) {
                POSITION yy = y + ddy_ddd[i];
                POSITION xx = x + ddx_ddd[i];
                if (!in_bounds2(floor_ptr, yy, xx)) {
                    continue;
                }

                floor_ptr->grid_array[yy][xx].info |= CAVE_GLOW;
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
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[y][x];
        g_ptr->info &= ~(CAVE_VIEW);
    }

    floor_ptr->view_n = 0;
}
