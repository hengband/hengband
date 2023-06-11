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
#include "effect/attribute-types.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-attack/monster-attack-table.h"
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
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <string>

// Melee-post-process-type
struct mam_pp_type {
    mam_pp_type(PlayerType *player_ptr, MONSTER_IDX m_idx, int dam, bool *dead, bool *fear, std::string_view note, MONSTER_IDX who);
    MONSTER_IDX m_idx;
    MonsterEntity *m_ptr;
    int dam;
    bool *dead;
    bool *fear;
    std::string note;
    MONSTER_IDX who;
    bool seen;
    bool known; /* Can the player be aware of this attack? */
    std::string m_name;
};

mam_pp_type::mam_pp_type(PlayerType *player_ptr, MONSTER_IDX m_idx, int dam, bool *dead, bool *fear, std::string_view note, MONSTER_IDX who)
    : m_idx(m_idx)
    , m_ptr(&player_ptr->current_floor_ptr->m_list[m_idx])
    , dam(dam)
    , dead(dead)
    , fear(fear)
    , note(note)
    , who(who)
{
    this->seen = is_seen(player_ptr, this->m_ptr);
    this->known = this->m_ptr->cdis <= MAX_PLAYER_SIGHT;
    this->m_name = monster_desc(player_ptr, m_ptr, 0);
}

static void prepare_redraw(PlayerType *player_ptr, mam_pp_type *mam_pp_ptr)
{
    if (!mam_pp_ptr->m_ptr->ml) {
        return;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (player_ptr->health_who == mam_pp_ptr->m_idx) {
        rfu.set_flag(MainWindowRedrawingFlag::HEALTH);
    }

    if (player_ptr->riding == mam_pp_ptr->m_idx) {
        rfu.set_flag(MainWindowRedrawingFlag::UHEALTH);
    }
}

/*!
 * @brief モンスターが無敵だった場合の処理
 * @param mam_pp_ptr 標的モンスター構造体への参照ポインタ
 * @return 無敵ノーダメならTRUE、無敵でないか無敵を貫通したらFALSE
 */
static bool process_invulnerability(mam_pp_type *mam_pp_ptr)
{
    if (mam_pp_ptr->m_ptr->is_invulnerable() && randint0(PENETRATE_INVULNERABILITY)) {
        return false;
    }

    if (mam_pp_ptr->seen) {
        msg_format(_("%s^はダメージを受けない。", "%s^ is unharmed."), mam_pp_ptr->m_name.data());
    }

    return true;
}

/*!
 * @brief 魔法完全防御持ちの処理
 * @param mam_pp_ptr 標的モンスター構造体への参照ポインタ
 * @return ノーダメならTRUE、 そうでないならFALSE
 */
static bool process_all_resistances(mam_pp_type *mam_pp_ptr)
{
    auto *r_ptr = &monraces_info[mam_pp_ptr->m_ptr->r_idx];
    if (r_ptr->resistance_flags.has_not(MonsterResistanceType::RESIST_ALL)) {
        return false;
    }

    if (mam_pp_ptr->dam > 0) {
        mam_pp_ptr->dam /= 100;
        if ((mam_pp_ptr->dam == 0) && one_in_(3)) {
            mam_pp_ptr->dam = 1;
        }
    }

    if (mam_pp_ptr->dam != 0) {
        return false;
    }

    if (mam_pp_ptr->seen) {
        msg_format(_("%s^はダメージを受けない。", "%s^ is unharmed."), mam_pp_ptr->m_name.data());
    }

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
static void print_monster_dead_by_monster(PlayerType *player_ptr, mam_pp_type *mam_pp_ptr)
{
    if (!mam_pp_ptr->known) {
        return;
    }

    mam_pp_ptr->m_name = monster_desc(player_ptr, mam_pp_ptr->m_ptr, MD_TRUE_NAME);
    if (!mam_pp_ptr->seen) {
        player_ptr->current_floor_ptr->monster_noise = true;
        return;
    }

    if (!mam_pp_ptr->note.empty()) {
        sound_type kill_sound = mam_pp_ptr->m_ptr->has_living_flag() ? SOUND_KILL : SOUND_N_KILL;
        sound(kill_sound);
        msg_format(_("%s^%s", "%s^%s"), mam_pp_ptr->m_name.data(), mam_pp_ptr->note.data());
        return;
    }

    if (!mam_pp_ptr->m_ptr->has_living_flag()) {
        sound(SOUND_N_KILL);
        msg_format(_("%s^は破壊された。", "%s^ is destroyed."), mam_pp_ptr->m_name.data());
        return;
    }

    sound(SOUND_KILL);
    msg_format(_("%s^は殺された。", "%s^ is killed."), mam_pp_ptr->m_name.data());
}

/*!
 * @brief ダメージを受けたモンスターのHPが0未満になった際の処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param mam_pp_ptr 標的モンスター構造体への参照ポインタ
 * @return 生きていたらTRUE、それ以外 (ユニークは＠以外の攻撃では死なない)はFALSE
 */
static bool check_monster_hp(PlayerType *player_ptr, mam_pp_type *mam_pp_ptr)
{
    const auto &monrace = monraces_info[mam_pp_ptr->m_ptr->r_idx];
    if (mam_pp_ptr->m_ptr->hp < 0) {
        return false;
    }

    auto is_like_unique = monrace.kind_flags.has(MonsterKindType::UNIQUE);
    is_like_unique |= any_bits(monrace.flags1, RF1_QUESTOR);
    is_like_unique |= monrace.population_flags.has(MonsterPopulationType::NAZGUL);
    if (is_like_unique && !player_ptr->phase_out) {
        mam_pp_ptr->m_ptr->hp = 1;
        return false;
    }

    *(mam_pp_ptr->dead) = true;
    print_monster_dead_by_monster(player_ptr, mam_pp_ptr);
    monster_gain_exp(player_ptr, mam_pp_ptr->who, mam_pp_ptr->m_ptr->r_idx);
    monster_death(player_ptr, mam_pp_ptr->m_idx, false, AttributeType::NONE);
    delete_monster_idx(player_ptr, mam_pp_ptr->m_idx);
    *(mam_pp_ptr->fear) = false;
    return true;
}

/*!
 * @brief 死亡等で恐慌状態をキャンセルする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param mam_pp_ptr 標的モンスター構造体への参照ポインタ
 */
static void cancel_fear_by_pain(PlayerType *player_ptr, mam_pp_type *mam_pp_ptr)
{
    const auto &m_ref = *mam_pp_ptr->m_ptr;
    const auto dam = mam_pp_ptr->dam;
    if (!m_ref.is_fearful() || (dam <= 0) || !set_monster_monfear(player_ptr, mam_pp_ptr->m_idx, m_ref.get_remaining_fear() - randint1(dam / 4))) {
        return;
    }

    *(mam_pp_ptr->fear) = false;
}

/*!
 * @biref HP残量などに応じてモンスターを恐慌状態にする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param mam_pp_ptr 標的モンスター構造体への参照ポインタ
 */
static void make_monster_fear(PlayerType *player_ptr, mam_pp_type *mam_pp_ptr)
{
    auto *r_ptr = &monraces_info[mam_pp_ptr->m_ptr->r_idx];
    if (mam_pp_ptr->m_ptr->is_fearful() || ((r_ptr->flags3 & RF3_NO_FEAR) == 0)) {
        return;
    }

    int percentage = (100L * mam_pp_ptr->m_ptr->hp) / mam_pp_ptr->m_ptr->maxhp;
    bool can_make_fear = ((percentage <= 10) && (randint0(10) < percentage)) || ((mam_pp_ptr->dam >= mam_pp_ptr->m_ptr->hp) && (randint0(100) < 80));
    if (!can_make_fear) {
        return;
    }

    *(mam_pp_ptr->fear) = true;
    (void)set_monster_monfear(
        player_ptr, mam_pp_ptr->m_idx, (randint1(10) + (((mam_pp_ptr->dam >= mam_pp_ptr->m_ptr->hp) && (percentage > 7)) ? 20 : ((11 - percentage) * 5))));
}

/*!
 * @brief モンスター同士の乱闘による落馬処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param mam_pp_ptr 標的モンスター構造体への参照ポインタ
 */
static void fall_off_horse_by_melee(PlayerType *player_ptr, mam_pp_type *mam_pp_ptr)
{
    if (!player_ptr->riding || (player_ptr->riding != mam_pp_ptr->m_idx) || (mam_pp_ptr->dam <= 0)) {
        return;
    }

    mam_pp_ptr->m_name = monster_desc(player_ptr, mam_pp_ptr->m_ptr, 0);
    if (mam_pp_ptr->m_ptr->hp > mam_pp_ptr->m_ptr->maxhp / 3) {
        mam_pp_ptr->dam = (mam_pp_ptr->dam + 1) / 2;
    }

    if (process_fall_off_horse(player_ptr, (mam_pp_ptr->dam > 200) ? 200 : mam_pp_ptr->dam, false)) {
        msg_format(_("%s^に振り落とされた！", "You have been thrown off from %s!"), mam_pp_ptr->m_name.data());
    }
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
void mon_take_hit_mon(PlayerType *player_ptr, MONSTER_IDX m_idx, int dam, bool *dead, bool *fear, std::string_view note, MONSTER_IDX who)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    mam_pp_type tmp_mam_pp(player_ptr, m_idx, dam, dead, fear, note, who);
    mam_pp_type *mam_pp_ptr = &tmp_mam_pp;
    prepare_redraw(player_ptr, mam_pp_ptr);
    (void)set_monster_csleep(player_ptr, m_idx, 0);

    if (player_ptr->riding && (m_idx == player_ptr->riding)) {
        disturb(player_ptr, true, true);
    }

    if (process_invulnerability(mam_pp_ptr) || process_all_resistances(mam_pp_ptr)) {
        return;
    }

    m_ptr->hp -= dam;
    if (check_monster_hp(player_ptr, mam_pp_ptr)) {
        return;
    }

    *dead = false;
    cancel_fear_by_pain(player_ptr, mam_pp_ptr);
    make_monster_fear(player_ptr, mam_pp_ptr);
    if ((dam > 0) && !m_ptr->is_pet() && !m_ptr->is_friendly() && (mam_pp_ptr->who != m_idx)) {
        const auto &m_ref = floor_ptr->m_list[who];
        if (m_ref.is_pet() && !player_bold(player_ptr, m_ptr->target_y, m_ptr->target_x)) {
            set_target(m_ptr, m_ref.fy, m_ref.fx);
        }
    }

    fall_off_horse_by_melee(player_ptr, mam_pp_ptr);
}
