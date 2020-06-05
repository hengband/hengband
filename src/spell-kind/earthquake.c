#include "spell-kind/earthquake.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/floor-events.h"
#include "floor/floor-object.h"
#include "floor/floor.h"
#include "io/write-diary.h"
#include "monster/monster-status.h"
#include "player/player-damage.h"
#include "player/player-effects.h"
#include "player/player-move.h"

/*!
 * @brief 地震処理(サブルーチン) /
 * Induce an "earthquake" of the given radius at the given location.
 * @param caster_ptrプレーヤーへの参照ポインタ
 * @param cy 中心Y座標
 * @param cx 中心X座標
 * @param r 効果半径
 * @param m_idx 地震を起こしたモンスターID(0ならばプレイヤー)
 * @return 効力があった場合TRUEを返す
 */
bool earthquake(player_type *caster_ptr, POSITION cy, POSITION cx, POSITION r, MONSTER_IDX m_idx)
{
    /* Prevent destruction of quest levels and town */
    floor_type *floor_ptr = caster_ptr->current_floor_ptr;
    if ((floor_ptr->inside_quest && is_fixed_quest_idx(floor_ptr->inside_quest)) || !floor_ptr->dun_level) {
        return FALSE;
    }

    /* Paranoia -- Enforce maximum range */
    if (r > 12)
        r = 12;

    /* Clear the "maximal blast" area */
    POSITION y, x;
    bool map[32][32];
    for (y = 0; y < 32; y++) {
        for (x = 0; x < 32; x++) {
            map[y][x] = FALSE;
        }
    }

    /* Check around the epicenter */
    POSITION yy, xx, dy, dx;
    int damage = 0;
    bool hurt = FALSE;
    for (dy = -r; dy <= r; dy++) {
        for (dx = -r; dx <= r; dx++) {
            yy = cy + dy;
            xx = cx + dx;

            if (!in_bounds(floor_ptr, yy, xx))
                continue;

            /* Skip distant grids */
            if (distance(cy, cx, yy, xx) > r)
                continue;
            grid_type *g_ptr;
            g_ptr = &floor_ptr->grid_array[yy][xx];

            /* Lose room and vault / Lose light and knowledge */
            g_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY | CAVE_UNSAFE);
            g_ptr->info &= ~(CAVE_GLOW | CAVE_MARK | CAVE_KNOWN);

            /* Skip the epicenter */
            if (!dx && !dy)
                continue;

            /* Skip most grids */
            if (randint0(100) < 85)
                continue;

            /* Damage this grid */
            map[16 + yy - cy][16 + xx - cx] = TRUE;

            /* Hack -- Take note of player damage */
            if (player_bold(caster_ptr, yy, xx))
                hurt = TRUE;
        }
    }

    /* First, affect the player (if necessary) */
    int sn = 0;
    POSITION sy = 0, sx = 0;
    if (hurt && !caster_ptr->pass_wall && !caster_ptr->kill_wall) {
        /* Check around the player */
        for (DIRECTION i = 0; i < 8; i++) {
            y = caster_ptr->y + ddy_ddd[i];
            x = caster_ptr->x + ddx_ddd[i];

            /* Skip non-empty grids */
            if (!is_cave_empty_bold(caster_ptr, y, x))
                continue;

            /* Important -- Skip "quake" grids */
            if (map[16 + y - cy][16 + x - cx])
                continue;

            if (floor_ptr->grid_array[y][x].m_idx)
                continue;

            /* Count "safe" grids */
            sn++;

            /* Randomize choice */
            if (randint0(sn) > 0)
                continue;

            /* Save the safe location */
            sy = y;
            sx = x;
        }

        /* Random message */
        switch (randint1(3)) {
        case 1: {
            msg_print(_("ダンジョンの壁が崩れた！", "The dungeon's ceiling collapses!"));
            break;
        }
        case 2: {
            msg_print(_("ダンジョンの床が不自然にねじ曲がった！", "The dungeon's floor twists in an unnatural way!"));
            break;
        }
        default: {
            msg_print(_("ダンジョンが揺れた！崩れた岩が頭に降ってきた！", "The dungeon quakes!  You are pummeled with debris!"));
            break;
        }
        }

        /* Hurt the player a lot */
        if (!sn) {
            /* Message and damage */
            msg_print(_("あなたはひどい怪我を負った！", "You are severely crushed!"));
            damage = 200;
        }

        /* Destroy the grid, and push the player to safety */
        else {
            /* Calculate results */
            switch (randint1(3)) {
            case 1: {
                msg_print(_("降り注ぐ岩をうまく避けた！", "You nimbly dodge the blast!"));
                damage = 0;
                break;
            }
            case 2: {
                msg_print(_("岩石があなたに直撃した!", "You are bashed by rubble!"));
                damage = damroll(10, 4);
                (void)set_stun(caster_ptr, caster_ptr->stun + randint1(50));
                break;
            }
            case 3: {
                msg_print(_("あなたは床と壁との間に挟まれてしまった！", "You are crushed between the floor and ceiling!"));
                damage = damroll(10, 4);
                (void)set_stun(caster_ptr, caster_ptr->stun + randint1(50));
                break;
            }
            }

            /* Move the player to the safe location */
            (void)move_player_effect(caster_ptr, sy, sx, MPE_DONT_PICKUP);
        }

        /* Important -- no wall on player */
        map[16 + caster_ptr->y - cy][16 + caster_ptr->x - cx] = FALSE;

        if (damage) {
            concptr killer;

            if (m_idx) {
                GAME_TEXT m_name[MAX_NLEN];
                monster_type *m_ptr = &floor_ptr->m_list[m_idx];
                monster_desc(caster_ptr, m_name, m_ptr, MD_WRONGDOER_NAME);
                killer = format(_("%sの起こした地震", "an earthquake caused by %s"), m_name);
            } else {
                killer = _("地震", "an earthquake");
            }

            take_hit(caster_ptr, DAMAGE_ATTACK, damage, killer, -1);
        }
    }

    /* Examine the quaked region */
    for (dy = -r; dy <= r; dy++) {
        for (dx = -r; dx <= r; dx++) {
            yy = cy + dy;
            xx = cx + dx;

            /* Skip unaffected grids */
            if (!map[16 + yy - cy][16 + xx - cx])
                continue;

            grid_type *g_ptr;
            g_ptr = &floor_ptr->grid_array[yy][xx];

            if (g_ptr->m_idx == caster_ptr->riding)
                continue;

            /* Process monsters */
            if (!g_ptr->m_idx)
                continue;

            monster_type *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
            monster_race *r_ptr = &r_info[m_ptr->r_idx];

            /* Quest monsters */
            if (r_ptr->flags1 & RF1_QUESTOR) {
                /* No wall on quest monsters */
                map[16 + yy - cy][16 + xx - cx] = FALSE;

                continue;
            }

            /* Most monsters cannot co-exist with rock */
            if ((r_ptr->flags2 & RF2_KILL_WALL) || (r_ptr->flags2 & RF2_PASS_WALL))
                continue;

            GAME_TEXT m_name[MAX_NLEN];

            /* Assume not safe */
            sn = 0;

            /* Monster can move to escape the wall */
            if (!(r_ptr->flags1 & RF1_NEVER_MOVE)) {
                /* Look for safety */
                for (DIRECTION i = 0; i < 8; i++) {
                    y = yy + ddy_ddd[i];
                    x = xx + ddx_ddd[i];

                    /* Skip non-empty grids */
                    if (!is_cave_empty_bold(caster_ptr, y, x))
                        continue;

                    /* Hack -- no safety on glyph of warding */
                    if (is_glyph_grid(&floor_ptr->grid_array[y][x]))
                        continue;
                    if (is_explosive_rune_grid(&floor_ptr->grid_array[y][x]))
                        continue;

                    /* ... nor on the Pattern */
                    if (pattern_tile(floor_ptr, y, x))
                        continue;

                    /* Important -- Skip "quake" grids */
                    if (map[16 + y - cy][16 + x - cx])
                        continue;

                    if (floor_ptr->grid_array[y][x].m_idx)
                        continue;
                    if (player_bold(caster_ptr, y, x))
                        continue;

                    /* Count "safe" grids */
                    sn++;

                    /* Randomize choice */
                    if (randint0(sn) > 0)
                        continue;

                    /* Save the safe grid */
                    sy = y;
                    sx = x;
                }
            }

            monster_desc(caster_ptr, m_name, m_ptr, 0);

            /* Scream in pain */
            if (!ignore_unview || is_seen(m_ptr))
                msg_format(_("%^sは苦痛で泣きわめいた！", "%^s wails out in pain!"), m_name);

            /* Take damage from the quake */
            damage = (sn ? damroll(4, 8) : (m_ptr->hp + 1));

            /* Monster is certainly awake */
            (void)set_monster_csleep(caster_ptr, g_ptr->m_idx, 0);

            /* Apply damage directly */
            m_ptr->hp -= damage;

            /* Delete (not kill) "dead" monsters */
            if (m_ptr->hp < 0) {
                if (!ignore_unview || is_seen(m_ptr))
                    msg_format(_("%^sは岩石に埋もれてしまった！", "%^s is embedded in the rock!"), m_name);

                if (g_ptr->m_idx) {
                    if (record_named_pet && is_pet(&floor_ptr->m_list[g_ptr->m_idx]) && floor_ptr->m_list[g_ptr->m_idx].nickname) {
                        char m2_name[MAX_NLEN];

                        monster_desc(caster_ptr, m2_name, m_ptr, MD_INDEF_VISIBLE);
                        exe_write_diary(caster_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_EARTHQUAKE, m2_name);
                    }
                }

                delete_monster(caster_ptr, yy, xx);

                sn = 0;
            }

            /* Hack -- Escape from the rock */
            if (sn == 0)
                continue;

            IDX m_idx_aux = floor_ptr->grid_array[yy][xx].m_idx;

            /* Update the old location */
            floor_ptr->grid_array[yy][xx].m_idx = 0;

            /* Update the new location */
            floor_ptr->grid_array[sy][sx].m_idx = m_idx_aux;

            /* Move the monster */
            m_ptr->fy = sy;
            m_ptr->fx = sx;

            update_monster(caster_ptr, m_idx, TRUE);
            lite_spot(caster_ptr, yy, xx);
            lite_spot(caster_ptr, sy, sx);
        }
    }

    /* Lose monster light */
    clear_mon_lite(floor_ptr);

    /* Examine the quaked region */
    for (dy = -r; dy <= r; dy++) {
        for (dx = -r; dx <= r; dx++) {
            yy = cy + dy;
            xx = cx + dx;

            /* Skip unaffected grids */
            if (!map[16 + yy - cy][16 + xx - cx])
                continue;

            /* Destroy location (if valid) */
            if (!cave_valid_bold(floor_ptr, yy, xx))
                continue;

            delete_all_items_from_floor(caster_ptr, yy, xx);

            /* Wall (or floor) type */
            int t = cave_have_flag_bold(floor_ptr, yy, xx, FF_PROJECT) ? randint0(100) : 200;

            /* Create granite wall */
            if (t < 20) {
                cave_set_feat(caster_ptr, yy, xx, feat_granite);
                continue;
            }

            /* Create quartz vein */
            if (t < 70) {
                cave_set_feat(caster_ptr, yy, xx, feat_quartz_vein);
                continue;
            }

            /* Create magma vein */
            if (t < 100) {
                cave_set_feat(caster_ptr, yy, xx, feat_magma_vein);
                continue;
            }

            /* Create floor */
            cave_set_feat(caster_ptr, yy, xx, feat_ground_type[randint0(100)]);
        }
    }

    /* Process "re-glowing" */
    for (dy = -r; dy <= r; dy++) {
        for (dx = -r; dx <= r; dx++) {
            yy = cy + dy;
            xx = cx + dx;

            if (!in_bounds(floor_ptr, yy, xx))
                continue;

            /* Skip distant grids */
            if (distance(cy, cx, yy, xx) > r)
                continue;
            grid_type *g_ptr;
            g_ptr = &floor_ptr->grid_array[yy][xx];

            if (is_mirror_grid(g_ptr)) {
                g_ptr->info |= CAVE_GLOW;
                continue;
            }

            if ((d_info[caster_ptr->dungeon_idx].flags1 & DF1_DARKNESS))
                continue;

            DIRECTION ii;
            POSITION yyy, xxx;
            grid_type *cc_ptr;

            for (ii = 0; ii < 9; ii++) {
                yyy = yy + ddy_ddd[ii];
                xxx = xx + ddx_ddd[ii];
                if (!in_bounds2(floor_ptr, yyy, xxx))
                    continue;
                cc_ptr = &floor_ptr->grid_array[yyy][xxx];
                if (have_flag(f_info[get_feat_mimic(cc_ptr)].flags, FF_GLOW)) {
                    g_ptr->info |= CAVE_GLOW;
                    break;
                }
            }
        }
    }

    /* Mega-Hack -- Forget the view and lite */
    caster_ptr->update |= (PU_UN_VIEW | PU_UN_LITE | PU_VIEW | PU_LITE | PU_FLOW | PU_MON_LITE | PU_MONSTERS);
    caster_ptr->redraw |= (PR_HEALTH | PR_UHEALTH | PR_MAP);
    caster_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

    if (caster_ptr->special_defense & NINJA_S_STEALTH) {
        if (floor_ptr->grid_array[caster_ptr->y][caster_ptr->x].info & CAVE_GLOW)
            set_superstealth(caster_ptr, FALSE);
    }

    /* Success */
    return TRUE;
}
