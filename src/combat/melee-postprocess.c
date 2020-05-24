/*!
 * @brief モンスター同士の打撃後処理 / Melee post-process.
 * @date 2014/01/17
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 * @details
 */

#include "combat/melee-postprocess.h"
#include "cmd-pet.h"
#include "combat/monster-attack-types.h"
#include "grid/grid.h"
#include "main/sound-definitions-table.h"
#include "monster/monster-race-hook.h"
#include "monster/monster-status.h"
#include "monster/monster.h"
#include "mspell/monster-spell.h"
#include "player/player-class.h"
#include "player/player-move.h"
#include "player/player-personalities-table.h"
#include "player/player-races-table.h"
#include "realm/realm-song.h"
#include "util/util.h"

// Melee-post-process-type
typedef struct mam_pp_type {
    monster_type *m_ptr;
    bool seen;
    GAME_TEXT m_name[160];
} mam_pp_type;

mam_pp_type *initialize_mam_pp_type(mam_pp_type *mam_pp_ptr, monster_type *m_ptr)
{
    mam_pp_ptr->m_ptr = m_ptr;
    mam_pp_ptr->seen = is_seen(m_ptr);
    return mam_pp_ptr;
}

/*!
 * todo 打撃が当たった時の後処理 (爆発持ちのモンスターを爆発させる等)なので、関数名を変更する必要あり
 * @brief モンスターが敵モンスターに行う打撃処理 /
 * Hack, based on mon_take_hit... perhaps all monster attacks on other monsters should use this?
 * @param m_idx 目標となるモンスターの参照ID
 * @param dam ダメージ量
 * @param dead 目標となったモンスターの死亡状態を返す参照ポインタ
 * @param fear 目標となったモンスターの恐慌状態を返す参照ポインタ
 * @param note 目標モンスターが死亡した場合の特別メッセージ(NULLならば標準表示を行う)
 * @param who 打撃を行ったモンスターの参照ID
 * @return なし
 */
void mon_take_hit_mon(player_type *player_ptr, MONSTER_IDX m_idx, HIT_POINT dam, bool *dead, bool *fear, concptr note, MONSTER_IDX who)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    mam_pp_type tmp_mam_pp;
    mam_pp_type *mam_pp_ptr = initialize_mam_pp_type(&tmp_mam_pp, m_ptr);

    /* Can the player be aware of this attack? */
    bool known = (m_ptr->cdis <= MAX_SIGHT);

    monster_desc(player_ptr, mam_pp_ptr->m_name, m_ptr, 0);

    /* Redraw (later) if needed */
    if (m_ptr->ml) {
        if (player_ptr->health_who == m_idx)
            player_ptr->redraw |= (PR_HEALTH);
        if (player_ptr->riding == m_idx)
            player_ptr->redraw |= (PR_UHEALTH);
    }

    (void)set_monster_csleep(player_ptr, m_idx, 0);

    if (player_ptr->riding && (m_idx == player_ptr->riding))
        disturb(player_ptr, TRUE, TRUE);

    if (MON_INVULNER(mam_pp_ptr->m_ptr) && randint0(PENETRATE_INVULNERABILITY)) {
        if (mam_pp_ptr->seen) {
            msg_format(_("%^sはダメージを受けない。", "%^s is unharmed."), mam_pp_ptr->m_name);
        }
        return;
    }

    if (r_ptr->flagsr & RFR_RES_ALL) {
        if (dam > 0) {
            dam /= 100;
            if ((dam == 0) && one_in_(3))
                dam = 1;
        }
        if (dam == 0) {
            if (mam_pp_ptr->seen) {
                msg_format(_("%^sはダメージを受けない。", "%^s is unharmed."), mam_pp_ptr->m_name);
            }
            return;
        }
    }

    /* Hurt it */
    m_ptr->hp -= dam;

    /* It is dead now... or is it? */
    if (m_ptr->hp < 0) {
        if (((r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) || (r_ptr->flags7 & RF7_NAZGUL)) && !player_ptr->phase_out) {
            m_ptr->hp = 1;
        } else {
            /* Make a sound */
            if (!monster_living(m_ptr->r_idx)) {
                sound(SOUND_N_KILL);
            } else {
                sound(SOUND_KILL);
            }

            *dead = TRUE;

            if (known) {
                monster_desc(player_ptr, mam_pp_ptr->m_name, m_ptr, MD_TRUE_NAME);
                /* Unseen death by normal attack */
                if (!mam_pp_ptr->seen) {
                    floor_ptr->monster_noise = TRUE;
                }
                /* Death by special attack */
                else if (note) {
                    msg_format(_("%^s%s", "%^s%s"), mam_pp_ptr->m_name, note);
                }
                /* Death by normal attack -- nonliving monster */
                else if (!monster_living(m_ptr->r_idx)) {
                    msg_format(_("%^sは破壊された。", "%^s is destroyed."), mam_pp_ptr->m_name);
                }
                /* Death by normal attack -- living monster */
                else {
                    msg_format(_("%^sは殺された。", "%^s is killed."), mam_pp_ptr->m_name);
                }
            }

            monster_gain_exp(player_ptr, who, m_ptr->r_idx);
            monster_death(player_ptr, m_idx, FALSE);
            delete_monster_idx(player_ptr, m_idx);

            /* Not afraid */
            (*fear) = FALSE;

            /* Monster is dead */
            return;
        }
    }

    *dead = FALSE;

    /* Mega-Hack -- Pain cancels fear */
    if (MON_MONFEAR(m_ptr) && (dam > 0)) {
        /* Cure fear */
        if (set_monster_monfear(player_ptr, m_idx, MON_MONFEAR(m_ptr) - randint1(dam / 4))) {
            /* No more fear */
            (*fear) = FALSE;
        }
    }

    /* Sometimes a monster gets scared by damage */
    if (!MON_MONFEAR(m_ptr) && !(r_ptr->flags3 & RF3_NO_FEAR)) {
        /* Percentage of fully healthy */
        int percentage = (100L * m_ptr->hp) / m_ptr->maxhp;

        /*
         * Run (sometimes) if at 10% or less of max hit points,
         * or (usually) when hit for half its current hit points
         */
        if (((percentage <= 10) && (randint0(10) < percentage)) || ((dam >= m_ptr->hp) && (randint0(100) < 80))) {
            /* Hack -- note fear */
            (*fear) = TRUE;

            /* Hack -- Add some timed fear */
            (void)set_monster_monfear(player_ptr, m_idx, (randint1(10) + (((dam >= m_ptr->hp) && (percentage > 7)) ? 20 : ((11 - percentage) * 5))));
        }
    }

    if ((dam > 0) && !is_pet(m_ptr) && !is_friendly(m_ptr) && (who != m_idx)) {
        if (is_pet(&floor_ptr->m_list[who]) && !player_bold(player_ptr, m_ptr->target_y, m_ptr->target_x)) {
            set_target(m_ptr, floor_ptr->m_list[who].fy, floor_ptr->m_list[who].fx);
        }
    }

    if (player_ptr->riding && (player_ptr->riding == m_idx) && (dam > 0)) {
        monster_desc(player_ptr, mam_pp_ptr->m_name, m_ptr, 0);

        if (m_ptr->hp > m_ptr->maxhp / 3)
            dam = (dam + 1) / 2;
        if (rakuba(player_ptr, (dam > 200) ? 200 : dam, FALSE)) {
            msg_format(_("%^sに振り落とされた！", "You have been thrown off from %s!"), mam_pp_ptr->m_name);
        }
    }
}
