/*!
 * @brief モンスター情報のアップデート処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster/monster-update.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "floor/cave.h"
#include "game-option/birth-options.h"
#include "game-option/disturbance-options.h"
#include "grid/grid.h"
#include "mind/drs-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "player/eldritch-horror.h"
#include "player/player-move.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "status/element-resistance.h"
#include "system/floor-type-definition.h"
#include "target/projection-path-calculator.h"

// Update Monster.
typedef struct um_type {
    monster_type *m_ptr;
    bool do_disturb;
    POSITION fy;
    POSITION fx;
    bool flag;
    bool easy;
    bool in_darkness;
    bool full;
} um_type;

/*!
 * @brief 騎乗中のモンスター情報を更新する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_idx モンスターID
 * @param oy 移動前の、モンスターのY座標
 * @param ox 移動前の、モンスターのX座標
 * @param ny 移動後の、モンスターのY座標
 * @param ox 移動後の、モンスターのX座標
 * @return アイテム等に影響を及ぼしたらTRUE
 */
bool update_riding_monster(player_type *target_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, POSITION oy, POSITION ox, POSITION ny, POSITION nx)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    grid_type *g_ptr = &target_ptr->current_floor_ptr->grid_array[ny][nx];
    monster_type *y_ptr = &target_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
    if (turn_flags_ptr->is_riding_mon)
        return move_player_effect(target_ptr, ny, nx, MPE_DONT_PICKUP);

    target_ptr->current_floor_ptr->grid_array[oy][ox].m_idx = g_ptr->m_idx;
    if (g_ptr->m_idx) {
        y_ptr->fy = oy;
        y_ptr->fx = ox;
        update_monster(target_ptr, g_ptr->m_idx, TRUE);
    }

    g_ptr->m_idx = m_idx;
    m_ptr->fy = ny;
    m_ptr->fx = nx;
    update_monster(target_ptr, m_idx, TRUE);

    lite_spot(target_ptr, oy, ox);
    lite_spot(target_ptr, ny, nx);
    return TRUE;
}

/*!
 * @brief updateフィールドを更新する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @return なし
 */
void update_player_type(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_race *r_ptr)
{
    if (turn_flags_ptr->do_view) {
        target_ptr->update |= PU_FLOW;
        target_ptr->window |= PW_OVERHEAD | PW_DUNGEON;
    }

    if (turn_flags_ptr->do_move
        && ((r_ptr->flags7 & (RF7_SELF_LD_MASK | RF7_HAS_DARK_1 | RF7_HAS_DARK_2))
            || ((r_ptr->flags7 & (RF7_HAS_LITE_1 | RF7_HAS_LITE_2)) && !target_ptr->phase_out))) {
        target_ptr->update |= PU_MON_LITE;
    }
}

/*!
 * @brief モンスターのフラグを更新する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @return なし
 */
void update_monster_race_flags(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    if (!is_original_ap_and_seen(target_ptr, m_ptr))
        return;

    if (turn_flags_ptr->did_open_door)
        r_ptr->r_flags2 |= RF2_OPEN_DOOR;

    if (turn_flags_ptr->did_bash_door)
        r_ptr->r_flags2 |= RF2_BASH_DOOR;

    if (turn_flags_ptr->did_take_item)
        r_ptr->r_flags2 |= RF2_TAKE_ITEM;

    if (turn_flags_ptr->did_kill_item)
        r_ptr->r_flags2 |= RF2_KILL_ITEM;

    if (turn_flags_ptr->did_move_body)
        r_ptr->r_flags2 |= RF2_MOVE_BODY;

    if (turn_flags_ptr->did_pass_wall)
        r_ptr->r_flags2 |= RF2_PASS_WALL;

    if (turn_flags_ptr->did_kill_wall)
        r_ptr->r_flags2 |= RF2_KILL_WALL;
}

/*!
 * @brief モンスターフラグの更新に基づき、モンスター表示を更新する
 * @param monster_race_idx モンスターID
 * @param window ウィンドウフラグ
 * @param old_race_flags_ptr モンスターフラグへの参照ポインタ
 * @return なし
 */
void update_player_window(player_type *target_ptr, old_race_flags *old_race_flags_ptr)
{
    monster_race *r_ptr;
    r_ptr = &r_info[target_ptr->monster_race_idx];
    if ((old_race_flags_ptr->old_r_flags1 != r_ptr->r_flags1) || (old_race_flags_ptr->old_r_flags2 != r_ptr->r_flags2)
        || (old_race_flags_ptr->old_r_flags3 != r_ptr->r_flags3) || (old_race_flags_ptr->old_r_flags4 != r_ptr->r_flags4)
        || (old_race_flags_ptr->old_r_flags5 != r_ptr->r_flags5) || (old_race_flags_ptr->old_r_flags6 != r_ptr->r_flags6)
        || (old_race_flags_ptr->old_r_flagsr != r_ptr->r_flagsr) || (old_race_flags_ptr->old_r_blows0 != r_ptr->r_blows[0])
        || (old_race_flags_ptr->old_r_blows1 != r_ptr->r_blows[1]) || (old_race_flags_ptr->old_r_blows2 != r_ptr->r_blows[2])
        || (old_race_flags_ptr->old_r_blows3 != r_ptr->r_blows[3]) || (old_race_flags_ptr->old_r_cast_spell != r_ptr->r_cast_spell)) {
        target_ptr->window |= PW_MONSTER;
    }
}

static um_type *initialize_um_type(player_type *subject_ptr, um_type *um_ptr, MONSTER_IDX m_idx, bool full)
{
    um_ptr->m_ptr = &subject_ptr->current_floor_ptr->m_list[m_idx];
    um_ptr->do_disturb = disturb_move;
    um_ptr->fy = um_ptr->m_ptr->fy;
    um_ptr->fx = um_ptr->m_ptr->fx;
    um_ptr->flag = FALSE;
    um_ptr->easy = FALSE;
    um_ptr->in_darkness = (d_info[subject_ptr->dungeon_idx].flags1 & DF1_DARKNESS) && !subject_ptr->see_nocto;
    um_ptr->full = full;
    return um_ptr;
}

static POSITION decide_updated_distance(player_type *subject_ptr, um_type *um_ptr)
{
    if (!um_ptr->full)
        return um_ptr->m_ptr->cdis;

    int dy = (subject_ptr->y > um_ptr->fy) ? (subject_ptr->y - um_ptr->fy) : (um_ptr->fy - subject_ptr->y);
    int dx = (subject_ptr->x > um_ptr->fx) ? (subject_ptr->x - um_ptr->fx) : (um_ptr->fx - subject_ptr->x);
    POSITION distance = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));
    if (distance > 255)
        distance = 255;

    if (!distance)
        distance = 1;

    um_ptr->m_ptr->cdis = distance;
    return distance;
}

static void update_smart_stupid_flags(monster_race *r_ptr)
{
    if (r_ptr->flags2 & RF2_SMART)
        r_ptr->r_flags2 |= RF2_SMART;

    if (r_ptr->flags2 & RF2_STUPID)
        r_ptr->r_flags2 |= RF2_STUPID;
}

/*!
 * @brief WEIRD_MINDフラグ持ちのモンスターを1/10の確率でテレパシーに引っかける
 * @param subject_ptr プレーヤーへの参照ポインタ
 * @param um_ptr モンスター情報アップデート構造体への参照ポインタ
 * @return WEIRD_MINDフラグがあるならTRUE
 */
static bool update_weird_telepathy(player_type *subject_ptr, um_type *um_ptr)
{
    monster_race *r_ptr = &r_info[um_ptr->m_ptr->r_idx];
    if ((r_ptr->flags2 & RF2_WEIRD_MIND) == 0)
        return FALSE;

    if (!one_in_(10))
        return TRUE;

    um_ptr->flag = TRUE;
    if (is_original_ap(um_ptr->m_ptr) && !subject_ptr->image) {
        r_ptr->r_flags2 |= RF2_WEIRD_MIND;
        update_smart_stupid_flags(r_ptr);
    }

    return TRUE;
}

static void update_telepathy_sight(player_type *subject_ptr, um_type *um_ptr)
{
    monster_race *r_ptr = &r_info[um_ptr->m_ptr->r_idx];
    if (subject_ptr->special_defense & KATA_MUSOU) {
        um_ptr->flag = TRUE;
        if (is_original_ap(um_ptr->m_ptr) && !subject_ptr->image)
            update_smart_stupid_flags(r_ptr);

        return;
    }

    if (!subject_ptr->telepathy)
        return;

    if (r_ptr->flags2 & RF2_EMPTY_MIND) {
        if (is_original_ap(um_ptr->m_ptr) && !subject_ptr->image)
            r_ptr->r_flags2 |= RF2_EMPTY_MIND;

        return;
    }

    if (update_weird_telepathy(subject_ptr, um_ptr))
        return;

    um_ptr->flag = TRUE;
    if (is_original_ap(um_ptr->m_ptr) && !subject_ptr->image)
        update_smart_stupid_flags(r_ptr);
}

static void update_specific_race_telepathy(player_type *subject_ptr, um_type *um_ptr)
{
    monster_race *r_ptr = &r_info[um_ptr->m_ptr->r_idx];
    if ((subject_ptr->esp_animal) && (r_ptr->flags3 & RF3_ANIMAL)) {
        um_ptr->flag = TRUE;
        if (is_original_ap(um_ptr->m_ptr) && !subject_ptr->image)
            r_ptr->r_flags3 |= RF3_ANIMAL;
    }

    if ((subject_ptr->esp_undead) && (r_ptr->flags3 & RF3_UNDEAD)) {
        um_ptr->flag = TRUE;
        if (is_original_ap(um_ptr->m_ptr) && !subject_ptr->image)
            r_ptr->r_flags3 |= RF3_UNDEAD;
    }

    if ((subject_ptr->esp_demon) && (r_ptr->flags3 & RF3_DEMON)) {
        um_ptr->flag = TRUE;
        if (is_original_ap(um_ptr->m_ptr) && !subject_ptr->image)
            r_ptr->r_flags3 |= RF3_DEMON;
    }

    if ((subject_ptr->esp_orc) && (r_ptr->flags3 & RF3_ORC)) {
        um_ptr->flag = TRUE;
        if (is_original_ap(um_ptr->m_ptr) && !subject_ptr->image)
            r_ptr->r_flags3 |= RF3_ORC;
    }

    if ((subject_ptr->esp_troll) && (r_ptr->flags3 & RF3_TROLL)) {
        um_ptr->flag = TRUE;
        if (is_original_ap(um_ptr->m_ptr) && !subject_ptr->image)
            r_ptr->r_flags3 |= RF3_TROLL;
    }

    if ((subject_ptr->esp_giant) && (r_ptr->flags3 & RF3_GIANT)) {
        um_ptr->flag = TRUE;
        if (is_original_ap(um_ptr->m_ptr) && !subject_ptr->image)
            r_ptr->r_flags3 |= RF3_GIANT;
    }

    if ((subject_ptr->esp_dragon) && (r_ptr->flags3 & RF3_DRAGON)) {
        um_ptr->flag = TRUE;
        if (is_original_ap(um_ptr->m_ptr) && !subject_ptr->image)
            r_ptr->r_flags3 |= RF3_DRAGON;
    }

    if ((subject_ptr->esp_human) && (r_ptr->flags2 & RF2_HUMAN)) {
        um_ptr->flag = TRUE;
        if (is_original_ap(um_ptr->m_ptr) && !subject_ptr->image)
            r_ptr->r_flags2 |= RF2_HUMAN;
    }

    if ((subject_ptr->esp_evil) && (r_ptr->flags3 & RF3_EVIL)) {
        um_ptr->flag = TRUE;
        if (is_original_ap(um_ptr->m_ptr) && !subject_ptr->image)
            r_ptr->r_flags3 |= RF3_EVIL;
    }

    if ((subject_ptr->esp_good) && (r_ptr->flags3 & RF3_GOOD)) {
        um_ptr->flag = TRUE;
        if (is_original_ap(um_ptr->m_ptr) && !subject_ptr->image)
            r_ptr->r_flags3 |= RF3_GOOD;
    }

    if ((subject_ptr->esp_nonliving) && ((r_ptr->flags3 & (RF3_DEMON | RF3_UNDEAD | RF3_NONLIVING)) == RF3_NONLIVING)) {
        um_ptr->flag = TRUE;
        if (is_original_ap(um_ptr->m_ptr) && !subject_ptr->image)
            r_ptr->r_flags3 |= RF3_NONLIVING;
    }

    if ((subject_ptr->esp_unique) && (r_ptr->flags1 & RF1_UNIQUE)) {
        um_ptr->flag = TRUE;
        if (is_original_ap(um_ptr->m_ptr) && !subject_ptr->image)
            r_ptr->r_flags1 |= RF1_UNIQUE;
    }
}

static bool check_cold_blood(player_type *subject_ptr, um_type *um_ptr, const POSITION distance)
{
    if (distance > subject_ptr->see_infra)
        return FALSE;

    monster_race *r_ptr = &r_info[um_ptr->m_ptr->r_idx];
    if ((r_ptr->flags2 & (RF2_COLD_BLOOD | RF2_AURA_FIRE)) == RF2_COLD_BLOOD)
        return FALSE;

    um_ptr->easy = TRUE;
    um_ptr->flag = TRUE;
    return TRUE;
}

static bool check_invisible(player_type *subject_ptr, um_type *um_ptr)
{
    if (!player_can_see_bold(subject_ptr, um_ptr->fy, um_ptr->fx))
        return FALSE;

    monster_race *r_ptr = &r_info[um_ptr->m_ptr->r_idx];
    if (r_ptr->flags2 & RF2_INVISIBLE) {
        if (subject_ptr->see_inv) {
            um_ptr->easy = TRUE;
            um_ptr->flag = TRUE;
        }
    } else {
        um_ptr->easy = TRUE;
        um_ptr->flag = TRUE;
    }

    return TRUE;
}

/*!
 * @brief テレパシー・赤外線視力・可視透明によってモンスターを感知できるかどうかの判定
 * @param subject_ptr プレーヤーへの参照ポインタ
 * @param um_ptr モンスター情報アップデート構造体への参照ポインタ
 * @return なし
 */
static void decide_sight_invisible_monster(player_type *subject_ptr, um_type *um_ptr)
{
    POSITION distance = decide_updated_distance(subject_ptr, um_ptr);
    monster_race *r_ptr = &r_info[um_ptr->m_ptr->r_idx];
    if (distance > (um_ptr->in_darkness ? MAX_SIGHT / 2 : MAX_SIGHT))
        return;

    if (!um_ptr->in_darkness || (distance <= MAX_SIGHT / 4)) {
        update_telepathy_sight(subject_ptr, um_ptr);
        update_specific_race_telepathy(subject_ptr, um_ptr);
    }

    if (!player_has_los_bold(subject_ptr, um_ptr->fy, um_ptr->fx) || subject_ptr->blind)
        return;

    if (subject_ptr->concent >= CONCENT_RADAR_THRESHOLD) {
        um_ptr->easy = TRUE;
        um_ptr->flag = TRUE;
    }

    bool do_cold_blood = check_cold_blood(subject_ptr, um_ptr, distance);
    bool do_invisible = check_invisible(subject_ptr, um_ptr);
    if (!um_ptr->flag || !is_original_ap(um_ptr->m_ptr) || subject_ptr->image)
        return;

    if (do_invisible)
        r_ptr->r_flags2 |= RF2_INVISIBLE;

    if (do_cold_blood)
        r_ptr->r_flags2 |= RF2_COLD_BLOOD;
}

/*!
 * @brief 壁の向こうにいるモンスターへのテレパシー・赤外線視力による冷血動物以外の透明モンスター・可視透明能力による透明モンスター
 * 以上を感知する
 * @param subject_ptr プレーヤーへの参照ポインタ
 * @param um_ptr モンスター情報アップデート構造体への参照ポインタ
 * @param m_idx フロアのモンスター番号
 * @return なし
 * @details 感知した結果、エルドリッチホラー持ちがいたら精神を破壊する
 */
static void update_invisible_monster(player_type *subject_ptr, um_type *um_ptr, MONSTER_IDX m_idx)
{
    if (um_ptr->m_ptr->ml)
        return;

    um_ptr->m_ptr->ml = TRUE;
    lite_spot(subject_ptr, um_ptr->fy, um_ptr->fx);

    if (subject_ptr->health_who == m_idx)
        subject_ptr->redraw |= PR_HEALTH;

    if (subject_ptr->riding == m_idx)
        subject_ptr->redraw |= PR_UHEALTH;

    if (!subject_ptr->image) {
        monster_race *r_ptr = &r_info[um_ptr->m_ptr->r_idx];
        if ((um_ptr->m_ptr->ap_r_idx == MON_KAGE) && (r_info[MON_KAGE].r_sights < MAX_SHORT))
            r_info[MON_KAGE].r_sights++;
        else if (is_original_ap(um_ptr->m_ptr) && (r_ptr->r_sights < MAX_SHORT))
            r_ptr->r_sights++;
    }

    if (r_info[um_ptr->m_ptr->ap_r_idx].flags2 & RF2_ELDRITCH_HORROR)
        sanity_blast(subject_ptr, um_ptr->m_ptr, FALSE);

    if (disturb_near
        && (projectable(subject_ptr, um_ptr->m_ptr->fy, um_ptr->m_ptr->fx, subject_ptr->y, subject_ptr->x)
            && projectable(subject_ptr, subject_ptr->y, subject_ptr->x, um_ptr->m_ptr->fy, um_ptr->m_ptr->fx))) {
        if (disturb_pets || is_hostile(um_ptr->m_ptr))
            disturb(subject_ptr, TRUE, TRUE);
    }
}

static void update_visible_monster(player_type *subject_ptr, um_type *um_ptr, MONSTER_IDX m_idx)
{
    if (!um_ptr->m_ptr->ml)
        return;

    um_ptr->m_ptr->ml = FALSE;
    lite_spot(subject_ptr, um_ptr->fy, um_ptr->fx);

    if (subject_ptr->health_who == m_idx)
        subject_ptr->redraw |= PR_HEALTH;

    if (subject_ptr->riding == m_idx)
        subject_ptr->redraw |= PR_UHEALTH;

    if (um_ptr->do_disturb && (disturb_pets || is_hostile(um_ptr->m_ptr)))
        disturb(subject_ptr, TRUE, TRUE);
}

static bool update_clear_monster(player_type *subject_ptr, um_type *um_ptr)
{
    if (!um_ptr->easy)
        return FALSE;

    if (!(um_ptr->m_ptr->mflag & MFLAG_VIEW)) {
        um_ptr->m_ptr->mflag |= MFLAG_VIEW;
        if (um_ptr->do_disturb && (disturb_pets || is_hostile(um_ptr->m_ptr)))
            disturb(subject_ptr, TRUE, TRUE);
    }

    return TRUE;
}

/*!
 * @brief モンスターの各情報を更新する / This function updates the monster record of the given monster
 * @param m_idx 更新するモンスター情報のID
 * @param full プレイヤーとの距離更新を行うならばtrue
 * @return なし
 */
void update_monster(player_type *subject_ptr, MONSTER_IDX m_idx, bool full)
{
    um_type tmp_um;
    um_type *um_ptr = initialize_um_type(subject_ptr, &tmp_um, m_idx, full);
    if (disturb_high) {
        monster_race *ap_r_ptr = &r_info[um_ptr->m_ptr->ap_r_idx];
        if (ap_r_ptr->r_tkills && ap_r_ptr->level >= subject_ptr->lev)
            um_ptr->do_disturb = TRUE;
    }

    if (um_ptr->m_ptr->mflag2 & MFLAG2_MARK)
        um_ptr->flag = TRUE;

    decide_sight_invisible_monster(subject_ptr, um_ptr);
    if (um_ptr->flag)
        update_invisible_monster(subject_ptr, um_ptr, m_idx);
    else
        update_visible_monster(subject_ptr, um_ptr, m_idx);
    
    if (update_clear_monster(subject_ptr, um_ptr) || ((um_ptr->m_ptr->mflag & MFLAG_VIEW) == 0))
        return;

    um_ptr->m_ptr->mflag &= ~(MFLAG_VIEW);
    if (um_ptr->do_disturb && (disturb_pets || is_hostile(um_ptr->m_ptr)))
        disturb(subject_ptr, TRUE, TRUE);
}

/*!
 * todo モンスターの感知状況しか更新していないように見える。関数名変更を検討する
 * @param player_ptr プレーヤーへの参照ポインタ
 * @brief 単純に生存している全モンスターの更新処理を行う / This function simply updates all the (non-dead) monsters (see above).
 * @param full 距離更新を行うならtrue
 * @return なし
 */
void update_monsters(player_type *player_ptr, bool full)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (MONSTER_IDX i = 1; i < floor_ptr->m_max; i++) {
        monster_type *m_ptr = &floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr))
            continue;

        update_monster(player_ptr, i, full);
    }
}

/*!
 * @brief SMART(適格に攻撃を行う)モンスターの学習状況を更新する / Learn about an "observed" resistance.
 * @param m_idx 更新を行う「モンスター情報ID
 * @param what 学習対象ID
 * @return なし
 */
void update_smart_learn(player_type *player_ptr, MONSTER_IDX m_idx, int what)
{
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    if (!smart_learn || ((r_ptr->flags2 & RF2_STUPID) != 0) || (((r_ptr->flags2 & RF2_SMART) == 0) && (randint0(100) < 50)))
        return;

    switch (what) {
    case DRS_ACID:
        if (has_resist_acid(player_ptr))
            m_ptr->smart |= SM_RES_ACID;

        if (is_oppose_acid(player_ptr))
            m_ptr->smart |= SM_OPP_ACID;

        if (has_immune_acid(player_ptr))
            m_ptr->smart |= SM_IMM_ACID;

        break;
    case DRS_ELEC:
        if (has_resist_elec(player_ptr))
            m_ptr->smart |= SM_RES_ELEC;

        if (is_oppose_elec(player_ptr))
            m_ptr->smart |= SM_OPP_ELEC;

        if (has_immune_elec(player_ptr))
            m_ptr->smart |= SM_IMM_ELEC;

        break;
    case DRS_FIRE:
        if (has_resist_fire(player_ptr))
            m_ptr->smart |= SM_RES_FIRE;

        if (is_oppose_fire(player_ptr))
            m_ptr->smart |= SM_OPP_FIRE;

        if (has_immune_fire(player_ptr))
            m_ptr->smart |= SM_IMM_FIRE;

        break;
    case DRS_COLD:
        if (has_resist_cold(player_ptr))
            m_ptr->smart |= SM_RES_COLD;

        if (is_oppose_cold(player_ptr))
            m_ptr->smart |= SM_OPP_COLD;

        if (has_immune_cold(player_ptr))
            m_ptr->smart |= SM_IMM_COLD;

        break;
    case DRS_POIS:
        if (has_resist_pois(player_ptr))
            m_ptr->smart |= SM_RES_POIS;

        if (is_oppose_pois(player_ptr))
            m_ptr->smart |= SM_OPP_POIS;

        break;
    case DRS_NETH:
        if (has_resist_neth(player_ptr))
            m_ptr->smart |= SM_RES_NETH;

        break;
    case DRS_LITE:
        if (has_resist_lite(player_ptr))
            m_ptr->smart |= SM_RES_LITE;

        break;
    case DRS_DARK:
        if (has_resist_dark(player_ptr))
            m_ptr->smart |= SM_RES_DARK;

        break;
    case DRS_FEAR:
        if (has_resist_fear(player_ptr))
            m_ptr->smart |= SM_RES_FEAR;

        break;
    case DRS_CONF:
        if (has_resist_conf(player_ptr))
            m_ptr->smart |= SM_RES_CONF;

        break;
    case DRS_CHAOS:
        if (has_resist_chaos(player_ptr))
            m_ptr->smart |= SM_RES_CHAOS;

        break;
    case DRS_DISEN:
        if (has_resist_disen(player_ptr))
            m_ptr->smart |= SM_RES_DISEN;

        break;
    case DRS_BLIND:
        if (has_resist_blind(player_ptr))
            m_ptr->smart |= SM_RES_BLIND;

        break;
    case DRS_NEXUS:
        if (has_resist_nexus(player_ptr))
            m_ptr->smart |= SM_RES_NEXUS;

        break;
    case DRS_SOUND:
        if (has_resist_sound(player_ptr))
            m_ptr->smart |= SM_RES_SOUND;

        break;
    case DRS_SHARD:
        if (has_resist_shard(player_ptr))
            m_ptr->smart |= SM_RES_SHARD;

        break;
    case DRS_FREE:
        if (player_ptr->free_act)
            m_ptr->smart |= SM_IMM_FREE;

        break;
    case DRS_MANA:
        if (!player_ptr->msp)
            m_ptr->smart |= SM_IMM_MANA;

        break;
    case DRS_REFLECT:
        if (has_reflect(player_ptr))
            m_ptr->smart |= SM_IMM_REFLECT;

        break;
    default:
        break;
    }
}
