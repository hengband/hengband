/*!
 * @brief モンスターが移動した結果、床のアイテムに重なった時の処理と、モンスターがアイテムを落とす処理
 * @date 2020/03/07
 * @author Hourier
 */

#include "monster-floor/monster-object.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-resistance-mask.h"
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
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include <string_view>

/*!
 * @brief オブジェクトのフラグを更新する
 */
static void update_object_flags(const TrFlags &flags, EnumClassFlagGroup<MonsterKindType> &flg_monster_kind, EnumClassFlagGroup<MonsterResistanceType> &flgr)
{
    if (flags.has(TR_SLAY_DRAGON)) {
        flg_monster_kind.set(MonsterKindType::DRAGON);
    }
    if (flags.has(TR_KILL_DRAGON)) {
        flg_monster_kind.set(MonsterKindType::DRAGON);
    }
    if (flags.has(TR_SLAY_TROLL)) {
        flg_monster_kind.set(MonsterKindType::TROLL);
    }
    if (flags.has(TR_KILL_TROLL)) {
        flg_monster_kind.set(MonsterKindType::TROLL);
    }
    if (flags.has(TR_SLAY_GIANT)) {
        flg_monster_kind.set(MonsterKindType::GIANT);
    }
    if (flags.has(TR_KILL_GIANT)) {
        flg_monster_kind.set(MonsterKindType::GIANT);
    }
    if (flags.has(TR_SLAY_ORC)) {
        flg_monster_kind.set(MonsterKindType::ORC);
    }
    if (flags.has(TR_KILL_ORC)) {
        flg_monster_kind.set(MonsterKindType::ORC);
    }
    if (flags.has(TR_SLAY_DEMON)) {
        flg_monster_kind.set(MonsterKindType::DEMON);
    }
    if (flags.has(TR_KILL_DEMON)) {
        flg_monster_kind.set(MonsterKindType::DEMON);
    }
    if (flags.has(TR_SLAY_UNDEAD)) {
        flg_monster_kind.set(MonsterKindType::UNDEAD);
    }
    if (flags.has(TR_KILL_UNDEAD)) {
        flg_monster_kind.set(MonsterKindType::UNDEAD);
    }
    if (flags.has(TR_SLAY_ANIMAL)) {
        flg_monster_kind.set(MonsterKindType::ANIMAL);
    }
    if (flags.has(TR_KILL_ANIMAL)) {
        flg_monster_kind.set(MonsterKindType::ANIMAL);
    }
    if (flags.has(TR_SLAY_EVIL)) {
        flg_monster_kind.set(MonsterKindType::EVIL);
    }
    if (flags.has(TR_KILL_EVIL)) {
        flg_monster_kind.set(MonsterKindType::EVIL);
    }
    if (flags.has(TR_SLAY_GOOD)) {
        flg_monster_kind.set(MonsterKindType::GOOD);
    }
    if (flags.has(TR_KILL_GOOD)) {
        flg_monster_kind.set(MonsterKindType::GOOD);
    }
    if (flags.has(TR_SLAY_HUMAN)) {
        flg_monster_kind.set(MonsterKindType::HUMAN);
    }
    if (flags.has(TR_KILL_HUMAN)) {
        flg_monster_kind.set(MonsterKindType::HUMAN);
    }
    if (flags.has(TR_BRAND_ACID)) {
        flgr.set(MonsterResistanceType::IMMUNE_ACID);
    }
    if (flags.has(TR_BRAND_ELEC)) {
        flgr.set(MonsterResistanceType::IMMUNE_ELEC);
    }
    if (flags.has(TR_BRAND_FIRE)) {
        flgr.set(MonsterResistanceType::IMMUNE_FIRE);
    }
    if (flags.has(TR_BRAND_COLD)) {
        flgr.set(MonsterResistanceType::IMMUNE_COLD);
    }
    if (flags.has(TR_BRAND_POIS)) {
        flgr.set(MonsterResistanceType::IMMUNE_POISON);
    }
}

/*!
 * @brief モンスターがアイテムを拾うか壊す処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_idx モンスターID
 * @param o_ptr オブジェクトへの参照ポインタ
 * @param is_unpickable_object モンスターが拾えないアイテム (アーティファクト等)であればTRUE
 * @param ny 移動後の、モンスターのY座標
 * @param nx 移動後の、モンスターのX座標
 * @param m_name モンスター名
 * @param o_name アイテム名
 * @param this_o_idx モンスターが乗ったオブジェクトID
 */
static void monster_pickup_object(PlayerType *player_ptr, turn_flags *turn_flags_ptr, const MONSTER_IDX m_idx, ItemEntity *o_ptr, const bool is_unpickable_object,
    const POSITION ny, const POSITION nx, std::string_view m_name, std::string_view o_name, const OBJECT_IDX this_o_idx)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    if (is_unpickable_object) {
        if (turn_flags_ptr->do_take && r_ptr->behavior_flags.has(MonsterBehaviorType::STUPID)) {
            turn_flags_ptr->did_take_item = true;
            if (m_ptr->ml && player_can_see_bold(player_ptr, ny, nx)) {
                msg_format(_("%s^は%sを拾おうとしたが、だめだった。", "%s^ tries to pick up %s, but fails."), m_name.data(), o_name.data());
            }
        }

        return;
    }

    if (turn_flags_ptr->do_take) {
        turn_flags_ptr->did_take_item = true;
        if (player_can_see_bold(player_ptr, ny, nx)) {
            msg_format(_("%s^が%sを拾った。", "%s^ picks up %s."), m_name.data(), o_name.data());
        }

        excise_object_idx(player_ptr->current_floor_ptr, this_o_idx);
        // 意図としては OmType::TOUCHED を維持しつつ OmType::FOUND を消す事と思われるが一応元のロジックを維持しておく
        o_ptr->marked &= { OmType::TOUCHED };
        o_ptr->iy = o_ptr->ix = 0;
        o_ptr->held_m_idx = m_idx;
        m_ptr->hold_o_idx_list.add(player_ptr->current_floor_ptr, this_o_idx);
        player_ptr->window_flags |= PW_FOUND_ITEM_LIST;
        return;
    }

    if (m_ptr->is_pet()) {
        return;
    }

    turn_flags_ptr->did_kill_item = true;
    if (player_has_los_bold(player_ptr, ny, nx)) {
        msg_format(_("%s^が%sを破壊した。", "%s^ destroys %s."), m_name.data(), o_name.data());
    }

    delete_object_idx(player_ptr, this_o_idx);
}

/*!
 * @brief モンスターの移動に伴うオブジェクト処理 (アイテム破壊等)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_idx モンスターID
 * @param ny 移動後の、モンスターのY座標
 * @param nx 移動後の、モンスターのX座標
 */
void update_object_by_monster_movement(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, POSITION ny, POSITION nx)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[ny][nx];
    turn_flags_ptr->do_take = r_ptr->behavior_flags.has(MonsterBehaviorType::TAKE_ITEM);
    for (auto it = g_ptr->o_idx_list.begin(); it != g_ptr->o_idx_list.end();) {
        EnumClassFlagGroup<MonsterKindType> flg_monster_kind;
        EnumClassFlagGroup<MonsterResistanceType> flgr;
        GAME_TEXT o_name[MAX_NLEN];
        OBJECT_IDX this_o_idx = *it++;
        auto *o_ptr = &player_ptr->current_floor_ptr->o_list[this_o_idx];

        if (turn_flags_ptr->do_take) {
            const auto tval = o_ptr->bi_key.tval();
            if (tval == ItemKindType::GOLD || (tval == ItemKindType::CORPSE) || (tval == ItemKindType::STATUE)) {
                continue;
            }
        }

        auto flags = object_flags(o_ptr);
        describe_flavor(player_ptr, o_name, o_ptr, 0);
        const auto m_name = monster_desc(player_ptr, m_ptr, MD_INDEF_HIDDEN);
        update_object_flags(flags, flg_monster_kind, flgr);

        auto is_unpickable_object = o_ptr->is_fixed_or_random_artifact();
        is_unpickable_object |= r_ptr->kind_flags.has_any_of(flg_monster_kind);
        is_unpickable_object |= !r_ptr->resistance_flags.has_all_of(flgr) && r_ptr->resistance_flags.has_not(MonsterResistanceType::RESIST_ALL);
        monster_pickup_object(player_ptr, turn_flags_ptr, m_idx, o_ptr, is_unpickable_object, ny, nx, m_name, o_name, this_o_idx);
    }
}

/*!
 * @brief モンスターが盗みや拾いで確保していたアイテムを全てドロップさせる / Drop all items carried by a monster
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_ptr モンスター参照ポインタ
 */
void monster_drop_carried_objects(PlayerType *player_ptr, MonsterEntity *m_ptr)
{
    for (auto it = m_ptr->hold_o_idx_list.begin(); it != m_ptr->hold_o_idx_list.end();) {
        ItemEntity forge;
        ItemEntity *o_ptr;
        ItemEntity *q_ptr;
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
