/*!
 * @brief モンスターが移動した結果、床のアイテムに重なった時の処理と、モンスターがアイテムを落とす処理
 * @date 2020/03/07
 * @author Hourier
 */

#include "monster-floor/monster-object.h"
#include "flavor/flavor-describer.h"
#include "floor/floor-object.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-processor-util.h"
#include "monster/smart-learn-types.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "object/object-mark-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief オブジェクトのフラグを更新する
 */
static void update_object_flags(const TrFlags &flgs, BIT_FLAGS *flg2, BIT_FLAGS *flg3, BIT_FLAGS *flgr)
{
    if (flgs.has(TR_SLAY_DRAGON))
        *flg3 |= (RF3_DRAGON);
    if (flgs.has(TR_KILL_DRAGON))
        *flg3 |= (RF3_DRAGON);
    if (flgs.has(TR_SLAY_TROLL))
        *flg3 |= (RF3_TROLL);
    if (flgs.has(TR_KILL_TROLL))
        *flg3 |= (RF3_TROLL);
    if (flgs.has(TR_SLAY_GIANT))
        *flg3 |= (RF3_GIANT);
    if (flgs.has(TR_KILL_GIANT))
        *flg3 |= (RF3_GIANT);
    if (flgs.has(TR_SLAY_ORC))
        *flg3 |= (RF3_ORC);
    if (flgs.has(TR_KILL_ORC))
        *flg3 |= (RF3_ORC);
    if (flgs.has(TR_SLAY_DEMON))
        *flg3 |= (RF3_DEMON);
    if (flgs.has(TR_KILL_DEMON))
        *flg3 |= (RF3_DEMON);
    if (flgs.has(TR_SLAY_UNDEAD))
        *flg3 |= (RF3_UNDEAD);
    if (flgs.has(TR_KILL_UNDEAD))
        *flg3 |= (RF3_UNDEAD);
    if (flgs.has(TR_SLAY_ANIMAL))
        *flg3 |= (RF3_ANIMAL);
    if (flgs.has(TR_KILL_ANIMAL))
        *flg3 |= (RF3_ANIMAL);
    if (flgs.has(TR_SLAY_EVIL))
        *flg3 |= (RF3_EVIL);
    if (flgs.has(TR_KILL_EVIL))
        *flg3 |= (RF3_EVIL);
    if (flgs.has(TR_SLAY_GOOD))
        *flg3 |= (RF3_GOOD);
    if (flgs.has(TR_KILL_GOOD))
        *flg3 |= (RF3_GOOD);
    if (flgs.has(TR_SLAY_HUMAN))
        *flg2 |= (RF2_HUMAN);
    if (flgs.has(TR_KILL_HUMAN))
        *flg2 |= (RF2_HUMAN);
    if (flgs.has(TR_BRAND_ACID))
        *flgr |= (RFR_IM_ACID);
    if (flgs.has(TR_BRAND_ELEC))
        *flgr |= (RFR_IM_ELEC);
    if (flgs.has(TR_BRAND_FIRE))
        *flgr |= (RFR_IM_FIRE);
    if (flgs.has(TR_BRAND_COLD))
        *flgr |= (RFR_IM_COLD);
    if (flgs.has(TR_BRAND_POIS))
        *flgr |= (RFR_IM_POIS);
}

/*!
 * @brief モンスターがアイテムを拾うか壊す処理
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_idx モンスターID
 * @param o_ptr オブジェクトへの参照ポインタ
 * @param is_special_object モンスターが拾えないアイテム (アーティファクト等)であればTRUE
 * @param ny 移動後の、モンスターのY座標
 * @param nx 移動後の、モンスターのX座標
 * @param m_name モンスター名
 * @param o_name アイテム名
 * @param this_o_idx モンスターが乗ったオブジェクトID
 */
static void monster_pickup_object(player_type *target_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, object_type *o_ptr, bool is_special_object,
    POSITION ny, POSITION nx, GAME_TEXT *m_name, GAME_TEXT *o_name, OBJECT_IDX this_o_idx)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    if (is_special_object) {
        if (turn_flags_ptr->do_take && (r_ptr->flags2 & RF2_STUPID)) {
            turn_flags_ptr->did_take_item = true;
            if (m_ptr->ml && player_can_see_bold(target_ptr, ny, nx)) {
                msg_format(_("%^sは%sを拾おうとしたが、だめだった。", "%^s tries to pick up %s, but fails."), m_name, o_name);
            }
        }

        return;
    }

    if (turn_flags_ptr->do_take) {
        turn_flags_ptr->did_take_item = true;
        if (player_can_see_bold(target_ptr, ny, nx)) {
            msg_format(_("%^sが%sを拾った。", "%^s picks up %s."), m_name, o_name);
        }

        excise_object_idx(target_ptr->current_floor_ptr, this_o_idx);
        o_ptr->marked &= OM_TOUCHED;
        o_ptr->iy = o_ptr->ix = 0;
        o_ptr->held_m_idx = m_idx;
        m_ptr->hold_o_idx_list.add(target_ptr->current_floor_ptr, this_o_idx);
        return;
    }

    if (is_pet(m_ptr))
        return;

    turn_flags_ptr->did_kill_item = true;
    if (player_has_los_bold(target_ptr, ny, nx)) {
        msg_format(_("%^sが%sを破壊した。", "%^s destroys %s."), m_name, o_name);
    }

    delete_object_idx(target_ptr, this_o_idx);
}

/*!
 * @brief モンスターの移動に伴うオブジェクト処理 (アイテム破壊等)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_idx モンスターID
 * @param ny 移動後の、モンスターのY座標
 * @param nx 移動後の、モンスターのX座標
 */
void update_object_by_monster_movement(player_type *target_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, POSITION ny, POSITION nx)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    grid_type *g_ptr;
    g_ptr = &target_ptr->current_floor_ptr->grid_array[ny][nx];

    turn_flags_ptr->do_take = (r_ptr->flags2 & RF2_TAKE_ITEM) != 0;
    for (auto it = g_ptr->o_idx_list.begin(); it != g_ptr->o_idx_list.end();) {
        BIT_FLAGS flg2 = 0L, flg3 = 0L, flgr = 0L;
        GAME_TEXT m_name[MAX_NLEN], o_name[MAX_NLEN];
        OBJECT_IDX this_o_idx = *it++;
        object_type *o_ptr = &target_ptr->current_floor_ptr->o_list[this_o_idx];

        if (turn_flags_ptr->do_take) {
            /* Skip gold, corpse and statue */
            if (o_ptr->tval == TV_GOLD || (o_ptr->tval == TV_CORPSE) || (o_ptr->tval == TV_STATUE))
                continue;
        }

        auto flgs = object_flags(o_ptr);
        describe_flavor(target_ptr, o_name, o_ptr, 0);
        monster_desc(target_ptr, m_name, m_ptr, MD_INDEF_HIDDEN);
        update_object_flags(flgs, &flg2, &flg3, &flgr);

        bool is_special_object = o_ptr->is_artifact() || ((r_ptr->flags3 & flg3) != 0) || ((r_ptr->flags2 & flg2) != 0)
            || (((~(r_ptr->flagsr) & flgr) != 0) && !(r_ptr->flagsr & RFR_RES_ALL));
        monster_pickup_object(target_ptr, turn_flags_ptr, m_idx, o_ptr, is_special_object, ny, nx, m_name, o_name, this_o_idx);
    }
}

/*!
 * @brief モンスターが盗みや拾いで確保していたアイテムを全てドロップさせる / Drop all items carried by a monster
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param m_ptr モンスター参照ポインタ
 */
void monster_drop_carried_objects(player_type *player_ptr, monster_type *m_ptr)
{
    for (auto it = m_ptr->hold_o_idx_list.begin(); it != m_ptr->hold_o_idx_list.end();) {
        object_type forge;
        object_type *o_ptr;
        object_type *q_ptr;
        const OBJECT_IDX this_o_idx = *it++;
        o_ptr = &player_ptr->current_floor_ptr->o_list[this_o_idx];
        q_ptr = &forge;
        q_ptr->copy_from(o_ptr);
        q_ptr->held_m_idx = 0;
        delete_object_idx(player_ptr, this_o_idx);
        (void)drop_near(player_ptr, q_ptr, -1, m_ptr->fy, m_ptr->fx);
    }

    m_ptr->hold_o_idx_list.clear();
}
