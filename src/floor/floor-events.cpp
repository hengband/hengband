#include "floor/floor-events.h"
#include "cmd-io/cmd-dump.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/disturbance-options.h"
#include "game-option/map-screen-options.h"
#include "grid/feature-flag-types.h"
#include "grid/feature.h"
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
#include "object/object-kind.h"
#include "object/object-mark-types.h"
#include "object/object-value.h"
#include "perception/object-perception.h"
#include "player/special-defense-types.h"
#include "sv-definition/sv-amulet-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-ring-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

void day_break(PlayerType *player_ptr)
{
    msg_print(_("夜が明けた。", "The sun has risen."));
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!player_ptr->wild_mode) {
        for (POSITION y = 0; y < floor_ptr->height; y++) {
            for (POSITION x = 0; x < floor_ptr->width; x++) {
                auto *g_ptr = &floor_ptr->grid_array[y][x];
                g_ptr->info |= CAVE_GLOW;
                if (view_perma_grids)
                    g_ptr->info |= CAVE_MARK;

                note_spot(player_ptr, y, x);
            }
        }
    }

    player_ptr->update |= PU_MONSTERS | PU_MON_LITE;
    player_ptr->redraw |= PR_MAP;
    player_ptr->window_flags |= PW_OVERHEAD | PW_DUNGEON;
    if ((floor_ptr->grid_array[player_ptr->y][player_ptr->x].info & CAVE_GLOW) != 0)
        set_superstealth(player_ptr, false);
}

void night_falls(PlayerType *player_ptr)
{
    msg_print(_("日が沈んだ。", "The sun has fallen."));
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!player_ptr->wild_mode) {
        for (POSITION y = 0; y < floor_ptr->height; y++) {
            for (POSITION x = 0; x < floor_ptr->width; x++) {
                auto *g_ptr = &floor_ptr->grid_array[y][x];
                auto *f_ptr = &f_info[g_ptr->get_feat_mimic()];
                if (g_ptr->is_mirror() || f_ptr->flags.has(FloorFeatureType::QUEST_ENTER) || f_ptr->flags.has(FloorFeatureType::ENTRANCE))
                    continue;

                g_ptr->info &= ~(CAVE_GLOW);
                if (f_ptr->flags.has_not(FloorFeatureType::REMEMBER)) {
                    g_ptr->info &= ~(CAVE_MARK);
                    note_spot(player_ptr, y, x);
                }
            }

            glow_deep_lava_and_bldg(player_ptr);
        }
    }

    player_ptr->update |= PU_MONSTERS | PU_MON_LITE;
    player_ptr->redraw |= PR_MAP;
    player_ptr->window_flags |= PW_OVERHEAD | PW_DUNGEON;

    if ((floor_ptr->grid_array[player_ptr->y][player_ptr->x].info & CAVE_GLOW) != 0)
        set_superstealth(player_ptr, false);
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
    if (!floor_ptr->dun_level)
        return 0;

    const int base = 10;
    int rating = 0;
    for (MONSTER_IDX i = 1; i < floor_ptr->m_max; i++) {
        auto *m_ptr = &floor_ptr->m_list[i];
        monster_race *r_ptr;
        int delta = 0;
        if (!monster_is_valid(m_ptr) || is_pet(m_ptr))
            continue;

        r_ptr = &r_info[m_ptr->r_idx];
        if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
            if (r_ptr->level + 10 > floor_ptr->dun_level)
                delta += (r_ptr->level + 10 - floor_ptr->dun_level) * 2 * base;
        } else if (r_ptr->level > floor_ptr->dun_level)
            delta += (r_ptr->level - floor_ptr->dun_level) * base;

        if (r_ptr->flags1 & RF1_FRIENDS) {
            if (5 <= get_monster_crowd_number(floor_ptr, i))
                delta += 1;
        } else if (2 <= get_monster_crowd_number(floor_ptr, i))
            delta += 1;

        rating += rating_boost(delta);
    }

    for (MONSTER_IDX i = 1; i < floor_ptr->o_max; i++) {
        auto *o_ptr = &floor_ptr->o_list[i];
        auto *k_ptr = &k_info[o_ptr->k_idx];
        int delta = 0;
        if (!o_ptr->is_valid() || (o_ptr->is_known() && ((o_ptr->marked & OM_TOUCHED) != 0)) || ((o_ptr->ident & IDENT_SENSE) != 0))
            continue;

        if (o_ptr->is_ego()) {
            ego_item_type *e_ptr = &e_info[enum2i<EgoType>(o_ptr->name2)];
            delta += e_ptr->rating * base;
        }

        if (o_ptr->is_artifact()) {
            PRICE cost = object_value_real(o_ptr);
            delta += 10 * base;
            if (cost > 10000L)
                delta += 10 * base;

            if (cost > 50000L)
                delta += 10 * base;

            if (cost > 100000L)
                delta += 10 * base;

            if (!preserve_mode)
                return 1;
        }

        if (o_ptr->tval == ItemKindType::DRAG_ARMOR)
            delta += 30 * base;

        if (o_ptr->tval == ItemKindType::SHIELD && o_ptr->sval == SV_DRAGON_SHIELD)
            delta += 5 * base;

        if (o_ptr->tval == ItemKindType::GLOVES && o_ptr->sval == SV_SET_OF_DRAGON_GLOVES)
            delta += 5 * base;

        if (o_ptr->tval == ItemKindType::BOOTS && o_ptr->sval == SV_PAIR_OF_DRAGON_GREAVE)
            delta += 5 * base;

        if (o_ptr->tval == ItemKindType::HELM && o_ptr->sval == SV_DRAGON_HELM)
            delta += 5 * base;

        if (o_ptr->tval == ItemKindType::RING && o_ptr->sval == SV_RING_SPEED && !o_ptr->is_cursed())
            delta += 25 * base;

        if (o_ptr->tval == ItemKindType::RING && o_ptr->sval == SV_RING_LORDLY && !o_ptr->is_cursed())
            delta += 15 * base;

        if (o_ptr->tval == ItemKindType::AMULET && o_ptr->sval == SV_AMULET_THE_MAGI && !o_ptr->is_cursed())
            delta += 15 * base;

        if (!o_ptr->is_cursed() && !o_ptr->is_broken() && k_ptr->level > floor_ptr->dun_level)
            delta += (k_ptr->level - floor_ptr->dun_level) * base;

        rating += rating_boost(delta);
    }

    if (rating > rating_boost(1000))
        return 2;

    if (rating > rating_boost(800))
        return 3;

    if (rating > rating_boost(600))
        return 4;

    if (rating > rating_boost(400))
        return 5;

    if (rating > rating_boost(300))
        return 6;

    if (rating > rating_boost(200))
        return 7;

    if (rating > rating_boost(100))
        return 8;

    if (rating > rating_boost(0))
        return 9;

    return 10;
}

/*!
 * @brief ダンジョンの雰囲気を更新し、変化があった場合メッセージを表示する
 * / Update dungeon feeling, and announce it if changed
 */
void update_dungeon_feeling(PlayerType *player_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!floor_ptr->dun_level)
        return;

    if (player_ptr->phase_out)
        return;

    int delay = std::max(10, 150 - player_ptr->skill_fos) * (150 - floor_ptr->dun_level) * TURNS_PER_TICK / 100;
    if (w_ptr->game_turn < player_ptr->feeling_turn + delay && !cheat_xtra)
        return;

    auto quest_num = quest_number(player_ptr, floor_ptr->dun_level);
    if (inside_quest(quest_num) && (quest_type::is_fixed(quest_num) && !((quest_num == QuestId::OBERON) || (quest_num == QuestId::SERPENT) || !(quest[enum2i(quest_num)].flags & QUEST_FLAG_PRESET))))
        return;

    byte new_feeling = get_dungeon_feeling(player_ptr);
    player_ptr->feeling_turn = w_ptr->game_turn;
    if (player_ptr->feeling == new_feeling)
        return;

    player_ptr->feeling = new_feeling;
    do_cmd_feeling(player_ptr);
    select_floor_music(player_ptr);
    player_ptr->redraw |= PR_DEPTH;
    if (disturb_minor)
        disturb(player_ptr, false, false);
}

/*
 * Glow deep lava and building entrances in the floor
 */
void glow_deep_lava_and_bldg(PlayerType *player_ptr)
{
    if (d_info[player_ptr->dungeon_idx].flags.has(DungeonFeatureType::DARKNESS))
        return;

    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (POSITION y = 0; y < floor_ptr->height; y++) {
        for (POSITION x = 0; x < floor_ptr->width; x++) {
            grid_type *g_ptr;
            g_ptr = &floor_ptr->grid_array[y][x];
            if (f_info[g_ptr->get_feat_mimic()].flags.has_not(FloorFeatureType::GLOW))
                continue;

            for (DIRECTION i = 0; i < 9; i++) {
                POSITION yy = y + ddy_ddd[i];
                POSITION xx = x + ddx_ddd[i];
                if (!in_bounds2(floor_ptr, yy, xx))
                    continue;

                floor_ptr->grid_array[yy][xx].info |= CAVE_GLOW;
            }
        }
    }

    player_ptr->update |= PU_VIEW | PU_LITE | PU_MON_LITE;
    player_ptr->redraw |= PR_MAP;
}

/*
 * Actually erase the entire "lite" array, redrawing every grid
 */
void forget_lite(floor_type *floor_ptr)
{
    if (!floor_ptr->lite_n)
        return;

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
void forget_view(floor_type *floor_ptr)
{
    if (!floor_ptr->view_n)
        return;

    for (int i = 0; i < floor_ptr->view_n; i++) {
        POSITION y = floor_ptr->view_y[i];
        POSITION x = floor_ptr->view_x[i];
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[y][x];
        g_ptr->info &= ~(CAVE_VIEW);
    }

    floor_ptr->view_n = 0;
}
