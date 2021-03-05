﻿#include "effect/effect-feature.h"
#include "core/player-update-types.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h" // 暫定、後で消す.
#include "floor/cave.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-ninja.h"
#include "monster/monster-update.h"
#include "player/special-defense-types.h"
#include "room/door-definition.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

/*
 * Determine if a "legal" grid is an "naked" floor grid
 *
 * Line 1 -- forbid non-clean gird
 * Line 2 -- forbid monsters
 * Line 3 -- forbid the player
 */
static bool cave_naked_bold(player_type *caster_ptr, POSITION y, POSITION x)
{
    floor_type *floor_ptr = caster_ptr->current_floor_ptr;
    return cave_clean_bold(floor_ptr, y, x) && (floor_ptr->grid_array[y][x].m_idx == 0) && !player_bold(caster_ptr, y, x);
}

/*!
 * @brief 汎用的なビーム/ボルト/ボール系による地形効果処理 / We are called from "project()" to "damage" terrain features
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param who 魔法を発動したモンスター(0ならばプレイヤー) / Index of "source" monster (zero for "player")
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標Y座標 / Target y location (or location to travel "towards")
 * @param x 目標X座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param typ 効果属性 / Type of damage to apply to monsters (and objects)
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 * @details
 * <pre>
 * We are called both for "beam" effects and "ball" effects.
 *
 * The "r" parameter is the "distance from ground zero".
 *
 * Note that we determine if the player can "see" anything that happens
 * by taking into account: blindness, line-of-sight, and illumination.
 *
 * We return "TRUE" if the effect of the projection is "obvious".
 *
 * We also "see" grids which are "memorized", probably a hack
 *
 * Perhaps we should affect doors?
 * </pre>
 */
bool affect_feature(player_type *caster_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ)
{
    floor_type *floor_ptr = caster_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    feature_type *f_ptr = &f_info[g_ptr->feat];

    bool obvious = FALSE;
    bool known = player_has_los_bold(caster_ptr, y, x);

    who = who ? who : 0;
    dam = (dam + r) / (r + 1);

    if (has_flag(f_ptr->flags, FF_TREE)) {
        concptr message;
        switch (typ) {
        case GF_POIS:
        case GF_NUKE:
        case GF_DEATH_RAY:
            message = _("枯れた", "was blasted.");
            break;
        case GF_TIME:
            message = _("縮んだ", "shrank.");
            break;
        case GF_ACID:
            message = _("溶けた", "melted.");
            break;
        case GF_COLD:
        case GF_ICE:
            message = _("凍り、砕け散った", "was frozen and smashed.");
            break;
        case GF_FIRE:
        case GF_ELEC:
        case GF_PLASMA:
            message = _("燃えた", "burns up!");
            break;
        case GF_METEOR:
        case GF_CHAOS:
        case GF_MANA:
        case GF_SEEKER:
        case GF_SUPER_RAY:
        case GF_SHARDS:
        case GF_ROCKET:
        case GF_SOUND:
        case GF_DISENCHANT:
        case GF_FORCE:
        case GF_GRAVITY:
            message = _("粉砕された", "was crushed.");
            break;
        default:
            message = NULL;
            break;
        }

        if (message) {
            msg_format(_("木は%s。", "A tree %s"), message);
            cave_set_feat(caster_ptr, y, x, one_in_(3) ? feat_brake : feat_grass);

            /* Observe */
            if (g_ptr->info & (CAVE_MARK))
                obvious = TRUE;
        }
    }

    /* Analyze the type */
    switch (typ) {
        /* Ignore most effects */
    case GF_CAPTURE:
    case GF_HAND_DOOM:
    case GF_CAUSE_1:
    case GF_CAUSE_2:
    case GF_CAUSE_3:
    case GF_CAUSE_4:
    case GF_MIND_BLAST:
    case GF_BRAIN_SMASH:
    case GF_DRAIN_MANA:
    case GF_PSY_SPEAR:
    case GF_FORCE:
    case GF_HOLY_FIRE:
    case GF_HELL_FIRE:
    case GF_PSI:
    case GF_PSI_DRAIN:
    case GF_TELEKINESIS:
    case GF_DOMINATION:
    case GF_IDENTIFY:
    case GF_ATTACK:
    case GF_ACID:
    case GF_ELEC:
    case GF_COLD:
    case GF_ICE:
    case GF_FIRE:
    case GF_PLASMA:
    case GF_METEOR:
    case GF_CHAOS:
    case GF_MANA:
    case GF_SEEKER:
    case GF_SUPER_RAY: {
        break;
    }
    case GF_KILL_TRAP: {
        if (is_hidden_door(caster_ptr, g_ptr)) {
            disclose_grid(caster_ptr, y, x);
            if (known) {
                obvious = TRUE;
            }
        }

        if (is_trap(caster_ptr, g_ptr->feat)) {
            if (known) {
                msg_print(_("まばゆい閃光が走った！", "There is a bright flash of light!"));
                obvious = TRUE;
            }

            cave_alter_feat(caster_ptr, y, x, FF_DISARM);
        }

        if (is_closed_door(caster_ptr, g_ptr->feat) && f_ptr->power && has_flag(f_ptr->flags, FF_OPEN)) {
            FEAT_IDX old_feat = g_ptr->feat;
            cave_alter_feat(caster_ptr, y, x, FF_DISARM);
            if (known && (old_feat != g_ptr->feat)) {
                msg_print(_("カチッと音がした！", "Click!"));
                obvious = TRUE;
            }
        }

        if (caster_ptr->blind || !player_has_los_bold(caster_ptr, y, x))
            break;

        g_ptr->info &= ~(CAVE_UNSAFE);
        lite_spot(caster_ptr, y, x);
        obvious = TRUE;
        break;
    }
    case GF_KILL_DOOR: {
        if (is_trap(caster_ptr, g_ptr->feat) || has_flag(f_ptr->flags, FF_DOOR)) {
            if (known) {
                msg_print(_("まばゆい閃光が走った！", "There is a bright flash of light!"));
                obvious = TRUE;
            }

            cave_alter_feat(caster_ptr, y, x, FF_TUNNEL);
        }

        if (caster_ptr->blind || !player_has_los_bold(caster_ptr, y, x))
            break;

        g_ptr->info &= ~(CAVE_UNSAFE);
        lite_spot(caster_ptr, y, x);
        obvious = TRUE;
        break;
    }
    case GF_JAM_DOOR: {
        if (!has_flag(f_ptr->flags, FF_SPIKE))
            break;

        s16b old_mimic = g_ptr->mimic;
        feature_type *mimic_f_ptr = &f_info[get_feat_mimic(g_ptr)];

        cave_alter_feat(caster_ptr, y, x, FF_SPIKE);
        g_ptr->mimic = old_mimic;

        note_spot(caster_ptr, y, x);
        lite_spot(caster_ptr, y, x);

        if (!known || !has_flag(mimic_f_ptr->flags, FF_OPEN))
            break;

        msg_format(_("%sに何かがつっかえて開かなくなった。", "The %s seems stuck."), f_name + mimic_f_ptr->name);
        obvious = TRUE;
        break;
    }
    case GF_KILL_WALL: {
        if (!has_flag(f_ptr->flags, FF_HURT_ROCK))
            break;

        if (known && (g_ptr->info & (CAVE_MARK))) {
            msg_format(_("%sが溶けて泥になった！", "The %s turns into mud!"), f_name + f_info[get_feat_mimic(g_ptr)].name);
            obvious = TRUE;
        }

        cave_alter_feat(caster_ptr, y, x, FF_HURT_ROCK);
        caster_ptr->update |= (PU_FLOW);
        break;
    }
    case GF_MAKE_DOOR: {
        if (!cave_naked_bold(caster_ptr, y, x))
            break;
        if (player_bold(caster_ptr, y, x))
            break;
        cave_set_feat(caster_ptr, y, x, feat_door[DOOR_DOOR].closed);
        if (g_ptr->info & (CAVE_MARK))
            obvious = TRUE;
        break;
    }
    case GF_MAKE_TRAP: {
        place_trap(caster_ptr, y, x);
        break;
    }
    case GF_MAKE_TREE: {
        if (!cave_naked_bold(caster_ptr, y, x))
            break;
        if (player_bold(caster_ptr, y, x))
            break;
        cave_set_feat(caster_ptr, y, x, feat_tree);
        if (g_ptr->info & (CAVE_MARK))
            obvious = TRUE;
        break;
    }
    case GF_MAKE_RUNE_PROTECTION: {
        if (!cave_naked_bold(caster_ptr, y, x))
            break;
        g_ptr->info |= CAVE_OBJECT;
        g_ptr->mimic = feat_rune_protection;
        note_spot(caster_ptr, y, x);
        lite_spot(caster_ptr, y, x);
        break;
    }
    case GF_STONE_WALL: {
        if (!cave_naked_bold(caster_ptr, y, x))
            break;
        if (player_bold(caster_ptr, y, x))
            break;
        cave_set_feat(caster_ptr, y, x, feat_granite);
        break;
    }
    case GF_LAVA_FLOW: {
        if (has_flag(f_ptr->flags, FF_PERMANENT))
            break;
        if (dam == 1) {
            if (!has_flag(f_ptr->flags, FF_FLOOR))
                break;
            cave_set_feat(caster_ptr, y, x, feat_shallow_lava);
        } else if (dam) {
            cave_set_feat(caster_ptr, y, x, feat_deep_lava);
        }

        break;
    }
    case GF_WATER_FLOW: {
        if (has_flag(f_ptr->flags, FF_PERMANENT))
            break;
        if (dam == 1) {
            if (!has_flag(f_ptr->flags, FF_FLOOR))
                break;
            cave_set_feat(caster_ptr, y, x, feat_shallow_water);
        } else if (dam) {
            cave_set_feat(caster_ptr, y, x, feat_deep_water);
        }

        break;
    }
    case GF_LITE_WEAK:
    case GF_LITE: {
        if ((d_info[caster_ptr->dungeon_idx].flags1 & DF1_DARKNESS) != 0)
            break;

        g_ptr->info |= (CAVE_GLOW);
        note_spot(caster_ptr, y, x);
        lite_spot(caster_ptr, y, x);
        update_local_illumination(caster_ptr, y, x);

        if (player_can_see_bold(caster_ptr, y, x))
            obvious = TRUE;
        if (g_ptr->m_idx)
            update_monster(caster_ptr, g_ptr->m_idx, FALSE);

        if (caster_ptr->special_defense & NINJA_S_STEALTH) {
            if (player_bold(caster_ptr, y, x))
                set_superstealth(caster_ptr, FALSE);
        }

        break;
    }
    case GF_DARK_WEAK:
    case GF_DARK: {
        bool do_dark = !caster_ptr->phase_out && !is_mirror_grid(g_ptr);
        if (!do_dark)
            break;

        if ((floor_ptr->dun_level > 0) || !is_daytime()) {
            for (int j = 0; j < 9; j++) {
                int by = y + ddy_ddd[j];
                int bx = x + ddx_ddd[j];

                if (!in_bounds2(floor_ptr, by, bx))
                    continue;

                grid_type *cc_ptr = &floor_ptr->grid_array[by][bx];
                if (has_flag(f_info[get_feat_mimic(cc_ptr)].flags, FF_GLOW)) {
                    do_dark = FALSE;
                    break;
                }
            }

            if (!do_dark)
                break;
        }

        g_ptr->info &= ~(CAVE_GLOW);

        /* Hack -- Forget "boring" grids */
        if (!has_flag(f_ptr->flags, FF_REMEMBER)) {
            /* Forget */
            g_ptr->info &= ~(CAVE_MARK);
            note_spot(caster_ptr, y, x);
        }

        lite_spot(caster_ptr, y, x);

        update_local_illumination(caster_ptr, y, x);

        if (player_can_see_bold(caster_ptr, y, x))
            obvious = TRUE;
        if (g_ptr->m_idx)
            update_monster(caster_ptr, g_ptr->m_idx, FALSE);

        break;
    }
    case GF_SHARDS:
    case GF_ROCKET: {
        if (is_mirror_grid(g_ptr)) {
            msg_print(_("鏡が割れた！", "The mirror was shattered!"));
            sound(SOUND_GLASS);
            remove_mirror(caster_ptr, y, x);
            project(caster_ptr, 0, 2, y, x, caster_ptr->lev / 2 + 5, GF_SHARDS,
                (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI), -1);
        }

        if (!has_flag(f_ptr->flags, FF_GLASS) || has_flag(f_ptr->flags, FF_PERMANENT) || (dam < 50))
            break;

        if (known && (g_ptr->info & CAVE_MARK)) {
            msg_format(_("%sが割れた！", "The %s crumbled!"), f_name + f_info[get_feat_mimic(g_ptr)].name);
            sound(SOUND_GLASS);
        }

        cave_alter_feat(caster_ptr, y, x, FF_HURT_ROCK);
        caster_ptr->update |= (PU_FLOW);
        break;
    }
    case GF_SOUND: {
        if (is_mirror_grid(g_ptr) && caster_ptr->lev < 40) {
            msg_print(_("鏡が割れた！", "The mirror was shattered!"));
            sound(SOUND_GLASS);
            remove_mirror(caster_ptr, y, x);
            project(caster_ptr, 0, 2, y, x, caster_ptr->lev / 2 + 5, GF_SHARDS,
                (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI), -1);
        }

        if (!has_flag(f_ptr->flags, FF_GLASS) || has_flag(f_ptr->flags, FF_PERMANENT) || (dam < 200))
            break;

        if (known && (g_ptr->info & CAVE_MARK)) {
            msg_format(_("%sが割れた！", "The %s crumbled!"), f_name + f_info[get_feat_mimic(g_ptr)].name);
            sound(SOUND_GLASS);
        }

        cave_alter_feat(caster_ptr, y, x, FF_HURT_ROCK);
        caster_ptr->update |= (PU_FLOW);
        break;
    }
    case GF_DISINTEGRATE: {
        if (is_mirror_grid(g_ptr) || is_rune_protection_grid(g_ptr) || is_rune_explosion_grid(g_ptr))
            remove_mirror(caster_ptr, y, x);

        if (!has_flag(f_ptr->flags, FF_HURT_DISI) || has_flag(f_ptr->flags, FF_PERMANENT))
            break;

        cave_alter_feat(caster_ptr, y, x, FF_HURT_DISI);
        caster_ptr->update |= (PU_FLOW);
        break;
    }
    }

    lite_spot(caster_ptr, y, x);
    return (obvious);
}
