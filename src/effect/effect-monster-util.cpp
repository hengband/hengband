/*!
 * @brief effect_monster_type構造体の初期化処理
 * @date 2020/04/29
 * @author Hourier
 */

#include "effect/effect-monster-util.h"
#include "floor/geometry.h"
#include "monster-floor/monster-death.h"
#include "monster-race/monster-race.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief affect_monster() に亘ってきた引数をeffect_monster_type構造体に代入する
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @param who 魔法を発動したモンスター (0ならばプレイヤー)
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標y座標 / Target y location (or location to travel "towards")
 * @param x 目標x座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param attribute 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flag 効果フラグ
 * @param see_s_msg TRUEならばメッセージを表示する
 */
static void substitute_effect_monster(effect_monster_type *em_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, int dam, AttributeType attribute, BIT_FLAGS flag, bool see_s_msg)
{
    em_ptr->who = who;
    em_ptr->r = r;
    em_ptr->y = y;
    em_ptr->x = x;
    em_ptr->dam = dam;
    em_ptr->attribute = attribute;
    em_ptr->flag = flag;
    em_ptr->see_s_msg = see_s_msg;
}

/*!
 * @brief effect_monster_type構造体を初期化する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @param who 魔法を発動したモンスター (0ならばプレイヤー)
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標y座標 / Target y location (or location to travel "towards")
 * @param x 目標x座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param attribute 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flag 効果フラグ
 * @param see_s_msg TRUEならばメッセージを表示する
 */
effect_monster_type *initialize_effect_monster(PlayerType *player_ptr, effect_monster_type *em_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, int dam, AttributeType attribute, BIT_FLAGS flag, bool see_s_msg)
{
    substitute_effect_monster(em_ptr, who, r, y, x, dam, attribute, flag, see_s_msg);

    auto *floor_ptr = player_ptr->current_floor_ptr;
    em_ptr->g_ptr = &floor_ptr->grid_array[em_ptr->y][em_ptr->x];
    em_ptr->m_ptr = &floor_ptr->m_list[em_ptr->g_ptr->m_idx];
    em_ptr->m_caster_ptr = (em_ptr->who > 0) ? &floor_ptr->m_list[em_ptr->who] : nullptr;
    em_ptr->r_ptr = &monraces_info[em_ptr->m_ptr->r_idx];
    em_ptr->seen = em_ptr->m_ptr->ml;
    em_ptr->seen_msg = is_seen(player_ptr, em_ptr->m_ptr);
    em_ptr->slept = em_ptr->m_ptr->is_asleep();
    em_ptr->obvious = false;
    em_ptr->known = ((em_ptr->m_ptr->cdis <= MAX_PLAYER_SIGHT) || player_ptr->phase_out);
    em_ptr->skipped = false;
    em_ptr->get_angry = false;
    em_ptr->do_polymorph = false;
    em_ptr->do_dist = 0;
    em_ptr->do_conf = 0;
    em_ptr->do_stun = 0;
    em_ptr->do_sleep = 0;
    em_ptr->do_fear = 0;
    em_ptr->do_time = 0;
    em_ptr->heal_leper = false;
    em_ptr->photo = 0;
    em_ptr->note = nullptr;
    em_ptr->note_dies = extract_note_dies(em_ptr->m_ptr->get_real_r_idx());
    em_ptr->caster_lev = (em_ptr->who > 0) ? monraces_info[em_ptr->m_caster_ptr->r_idx].level : (player_ptr->lev * 2);
    return em_ptr;
}
