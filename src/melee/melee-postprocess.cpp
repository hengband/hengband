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

#include "melee/melee-postprocess.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-attack/monster-attack-types.h"
#include "monster-floor/monster-death.h"
#include "monster-floor/monster-move.h"
#include "monster-floor/monster-remover.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "pet/pet-fall-off.h"
#include "player-info/class-info.h"
#include "player-info/race-types.h"
#include "player/player-personality-types.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

// Melee-post-process-type
typedef struct mam_pp_type {
    MONSTER_IDX m_idx;
    monster_type *m_ptr;
    bool seen;
    GAME_TEXT m_name[160];
    HIT_POINT dam;
    bool known; /* Can the player be aware of this attack? */
    concptr note;
    bool *dead;
    bool *fear;
    MONSTER_IDX who;
} mam_pp_type;

mam_pp_type *initialize_mam_pp_type(
    player_type *player_ptr, mam_pp_type *mam_pp_ptr, MONSTER_IDX m_idx, HIT_POINT dam, bool *dead, bool *fear, concptr note, MONSTER_IDX who)
{
    mam_pp_ptr->m_idx = m_idx;
    mam_pp_ptr->m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    mam_pp_ptr->seen = is_seen(player_ptr, mam_pp_ptr->m_ptr);
    mam_pp_ptr->dam = dam;
    mam_pp_ptr->known = mam_pp_ptr->m_ptr->cdis <= MAX_SIGHT;
    mam_pp_ptr->dead = dead;
    mam_pp_ptr->fear = fear;
    mam_pp_ptr->note = note;
    mam_pp_ptr->who = who;
    return mam_pp_ptr;
}

static void prepare_redraw(player_type *player_ptr, mam_pp_type *mam_pp_ptr)
{
    if (!mam_pp_ptr->m_ptr->ml)
        return;

    if (player_ptr->health_who == mam_pp_ptr->m_idx)
        player_ptr->redraw |= (PR_HEALTH);

    if (player_ptr->riding == mam_pp_ptr->m_idx)
        player_ptr->redraw |= (PR_UHEALTH);
}

/*!
 * @brief モンスターが無敵だった場合の処理
 * @param mam_pp_ptr 標的モンスター構造体への参照ポインタ
 * @return 無敵ノーダメならTRUE、無敵でないか無敵を貫通したらFALSE
 */
static bool process_invulnerability(mam_pp_type *mam_pp_ptr)
{
    if (monster_invulner_remaining(mam_pp_ptr->m_ptr) && randint0(PENETRATE_INVULNERABILITY))
        return false;

    if (mam_pp_ptr->seen)
        msg_format(_("%^sはダメージを受けない。", "%^s is unharmed."), mam_pp_ptr->m_name);

    return true;
}

/*!
 * @brief 魔法完全防御持ちの処理
 * @param mam_pp_ptr 標的モンスター構造体への参照ポインタ
 * @return ノーダメならTRUE、 そうでないならFALSE
 */
static bool process_all_resistances(mam_pp_type *mam_pp_ptr)
{
    monster_race *r_ptr = &r_info[mam_pp_ptr->m_ptr->r_idx];
    if ((r_ptr->flagsr & RFR_RES_ALL) == 0)
        return false;

    if (mam_pp_ptr->dam > 0) {
        mam_pp_ptr->dam /= 100;
        if ((mam_pp_ptr->dam == 0) && one_in_(3))
            mam_pp_ptr->dam = 1;
    }

    if (mam_pp_ptr->dam != 0)
        return false;

    if (mam_pp_ptr->seen)
        msg_format(_("%^sはダメージを受けない。", "%^s is unharmed."), mam_pp_ptr->m_name);

    return true;
}

/*!
 * @brief モンスター死亡時のメッセージ表示
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param mam_pp_ptr 標的モンスター構造体への参照ポインタ
 * @details
 * 見えない位置で死んだら何も表示しない
 * 爆発して粉々になった等ならその旨を、残りは生命か無生命かで分岐
 */
static void print_monster_dead_by_monster(player_type *player_ptr, mam_pp_type *mam_pp_ptr)
{
    if (!mam_pp_ptr->known)
        return;

    monster_desc(player_ptr, mam_pp_ptr->m_name, mam_pp_ptr->m_ptr, MD_TRUE_NAME);
    if (!mam_pp_ptr->seen) {
        player_ptr->current_floor_ptr->monster_noise = true;
        return;
    }

    if (mam_pp_ptr->note) {
        sound_type kill_sound = monster_living(mam_pp_ptr->m_ptr->r_idx) ? SOUND_KILL : SOUND_N_KILL;
        sound(kill_sound);
        msg_format(_("%^s%s", "%^s%s"), mam_pp_ptr->m_name, mam_pp_ptr->note);
        return;
    }

    if (!monster_living(mam_pp_ptr->m_ptr->r_idx)) {
        sound(SOUND_N_KILL);
        msg_format(_("%^sは破壊された。", "%^s is destroyed."), mam_pp_ptr->m_name);
        return;
    }

    sound(SOUND_KILL);
    msg_format(_("%^sは殺された。", "%^s is killed."), mam_pp_ptr->m_name);
}

/*!
 * @brief ダメージを受けたモンスターのHPが0未満になった際の処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param mam_pp_ptr 標的モンスター構造体への参照ポインタ
 * @return 生きていたらTRUE、それ以外 (ユニークは＠以外の攻撃では死なない)はFALSE
 */
static bool check_monster_hp(player_type *player_ptr, mam_pp_type *mam_pp_ptr)
{
    monster_race *r_ptr = &r_info[mam_pp_ptr->m_ptr->r_idx];
    if (mam_pp_ptr->m_ptr->hp < 0)
        return false;

    if (((r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) || (r_ptr->flags7 & RF7_NAZGUL)) && !player_ptr->phase_out) {
        mam_pp_ptr->m_ptr->hp = 1;
        return false;
    }

    *(mam_pp_ptr->dead) = true;
    print_monster_dead_by_monster(player_ptr, mam_pp_ptr);
    monster_gain_exp(player_ptr, mam_pp_ptr->who, mam_pp_ptr->m_ptr->r_idx);
    monster_death(player_ptr, mam_pp_ptr->m_idx, false, GF_NONE);
    delete_monster_idx(player_ptr, mam_pp_ptr->m_idx);
    *(mam_pp_ptr->fear) = false;
    return true;
}

/*!
 * @brief 死亡等で恐慌状態をキャンセルする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param mam_pp_ptr 標的モンスター構造体への参照ポインタ
 */
static void cancel_fear_by_pain(player_type *player_ptr, mam_pp_type *mam_pp_ptr)
{
    if (!monster_fear_remaining(mam_pp_ptr->m_ptr) || (mam_pp_ptr->dam <= 0)
        || !set_monster_monfear(player_ptr, mam_pp_ptr->m_idx, monster_fear_remaining(mam_pp_ptr->m_ptr) - randint1(mam_pp_ptr->dam / 4)))
        return;

    *(mam_pp_ptr->fear) = false;
}

/*!
 * @biref HP残量などに応じてモンスターを恐慌状態にする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param mam_pp_ptr 標的モンスター構造体への参照ポインタ
 */
static void make_monster_fear(player_type *player_ptr, mam_pp_type *mam_pp_ptr)
{
    monster_race *r_ptr = &r_info[mam_pp_ptr->m_ptr->r_idx];
    if (monster_fear_remaining(mam_pp_ptr->m_ptr) || ((r_ptr->flags3 & RF3_NO_FEAR) == 0))
        return;

    int percentage = (100L * mam_pp_ptr->m_ptr->hp) / mam_pp_ptr->m_ptr->maxhp;
    bool can_make_fear = ((percentage <= 10) && (randint0(10) < percentage)) || ((mam_pp_ptr->dam >= mam_pp_ptr->m_ptr->hp) && (randint0(100) < 80));
    if (!can_make_fear)
        return;

    *(mam_pp_ptr->fear) = true;
    (void)set_monster_monfear(
        player_ptr, mam_pp_ptr->m_idx, (randint1(10) + (((mam_pp_ptr->dam >= mam_pp_ptr->m_ptr->hp) && (percentage > 7)) ? 20 : ((11 - percentage) * 5))));
}

/*!
 * @brief モンスター同士の乱闘による落馬処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param mam_pp_ptr 標的モンスター構造体への参照ポインタ
 */
static void fall_off_horse_by_melee(player_type *player_ptr, mam_pp_type *mam_pp_ptr)
{
    if (!player_ptr->riding || (player_ptr->riding != mam_pp_ptr->m_idx) || (mam_pp_ptr->dam <= 0))
        return;

    monster_desc(player_ptr, mam_pp_ptr->m_name, mam_pp_ptr->m_ptr, 0);
    if (mam_pp_ptr->m_ptr->hp > mam_pp_ptr->m_ptr->maxhp / 3)
        mam_pp_ptr->dam = (mam_pp_ptr->dam + 1) / 2;

    if (process_fall_off_horse(player_ptr, (mam_pp_ptr->dam > 200) ? 200 : mam_pp_ptr->dam, false))
        msg_format(_("%^sに振り落とされた！", "You have been thrown off from %s!"), mam_pp_ptr->m_name);
}

/*!
 * @brief モンスターが敵モンスターに行う打撃処理 /
 * Hack, based on mon_take_hit... perhaps all monster attacks on other monsters should use this?
 * @param m_idx 目標となるモンスターの参照ID
 * @param dam ダメージ量
 * @param dead 目標となったモンスターの死亡状態を返す参照ポインタ
 * @param fear 目標となったモンスターの恐慌状態を返す参照ポインタ
 * @param note 目標モンスターが死亡した場合の特別メッセージ(nullptrならば標準表示を行う)
 * @param who 打撃を行ったモンスターの参照ID
 * @todo 打撃が当たった時の後処理 (爆発持ちのモンスターを爆発させる等)なので、関数名を変更する必要あり
 */
void mon_take_hit_mon(player_type *player_ptr, MONSTER_IDX m_idx, HIT_POINT dam, bool *dead, bool *fear, concptr note, MONSTER_IDX who)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    mam_pp_type tmp_mam_pp;
    mam_pp_type *mam_pp_ptr = initialize_mam_pp_type(player_ptr, &tmp_mam_pp, m_idx, dam, dead, fear, note, who);
    monster_desc(player_ptr, mam_pp_ptr->m_name, m_ptr, 0);
    prepare_redraw(player_ptr, mam_pp_ptr);
    (void)set_monster_csleep(player_ptr, m_idx, 0);

    if (player_ptr->riding && (m_idx == player_ptr->riding))
        disturb(player_ptr, true, true);

    if (process_invulnerability(mam_pp_ptr) || process_all_resistances(mam_pp_ptr))
        return;

    m_ptr->hp -= dam;
    if (check_monster_hp(player_ptr, mam_pp_ptr))
        return;

    *dead = false;
    cancel_fear_by_pain(player_ptr, mam_pp_ptr);
    make_monster_fear(player_ptr, mam_pp_ptr);
    if ((dam > 0) && !is_pet(m_ptr) && !is_friendly(m_ptr) && (mam_pp_ptr->who != m_idx)) {
        if (is_pet(&floor_ptr->m_list[mam_pp_ptr->who]) && !player_bold(player_ptr, m_ptr->target_y, m_ptr->target_x)) {
            set_target(m_ptr, floor_ptr->m_list[mam_pp_ptr->who].fy, floor_ptr->m_list[mam_pp_ptr->who].fx);
        }
    }

    fall_off_horse_by_melee(player_ptr, mam_pp_ptr);
}
