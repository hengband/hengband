﻿/*!
 * @brief effect_monster_type構造体の初期化処理
 * @date 2020/04/29
 * @author Hourier
 */

#include "effect/effect-monster-util.h"
#include "monster-floor/monster-death.h"
#include "monster-race/monster-race.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "system/floor-type-definition.h"

/*!
 * @brief affect_monster() に亘ってきた引数をeffect_monster_type構造体に代入する
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @param who 魔法を発動したモンスター (0ならばプレーヤー)
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標y座標 / Target y location (or location to travel "towards")
 * @param x 目標x座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param effect_type 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flag 効果フラグ
 * @param see_s_msg TRUEならばメッセージを表示する
 * @return なし
 */
static void substitute_effect_monster(effect_monster_type *em_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID effect_type, BIT_FLAGS flag, bool see_s_msg)
{
	em_ptr->who = who;
	em_ptr->r = r;
	em_ptr->y = y;
	em_ptr->x = x;
	em_ptr->dam = dam;
	em_ptr->effect_type = effect_type;
	em_ptr->flag = flag;
	em_ptr->see_s_msg = see_s_msg;
}


/*!
 * @brief effect_monster_type構造体を初期化する
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @param who 魔法を発動したモンスター (0ならばプレーヤー)
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標y座標 / Target y location (or location to travel "towards")
 * @param x 目標x座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param effect_type 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flag 効果フラグ
 * @param see_s_msg TRUEならばメッセージを表示する
 * @return なし
 */
effect_monster_type *initialize_effect_monster(player_type *caster_ptr, effect_monster_type *em_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID effect_type, BIT_FLAGS flag, bool see_s_msg)
{
	substitute_effect_monster(em_ptr, who, r, y, x, dam, effect_type, flag, see_s_msg);

	floor_type *floor_ptr = caster_ptr->current_floor_ptr;
	em_ptr->g_ptr = &floor_ptr->grid_array[em_ptr->y][em_ptr->x];
	em_ptr->m_ptr = &floor_ptr->m_list[em_ptr->g_ptr->m_idx];
	em_ptr->m_caster_ptr = (em_ptr->who > 0) ? &floor_ptr->m_list[em_ptr->who] : NULL;
	em_ptr->r_ptr = &r_info[em_ptr->m_ptr->r_idx];
	em_ptr->seen = em_ptr->m_ptr->ml;
        em_ptr->seen_msg = is_seen(caster_ptr, em_ptr->m_ptr);
	em_ptr->slept = (bool)monster_csleep_remaining(em_ptr->m_ptr);
	em_ptr->obvious = FALSE;
	em_ptr->known = ((em_ptr->m_ptr->cdis <= MAX_SIGHT) || caster_ptr->phase_out);
	em_ptr->skipped = FALSE;
	em_ptr->get_angry = FALSE;
	em_ptr->do_polymorph = FALSE;
	em_ptr->do_dist = 0;
	em_ptr->do_conf = 0;
	em_ptr->do_stun = 0;
	em_ptr->do_sleep = 0;
	em_ptr->do_fear = 0;
	em_ptr->do_time = 0;
	em_ptr->heal_leper = FALSE;
	em_ptr->photo = 0;
	em_ptr->note = NULL;
	em_ptr->note_dies = extract_note_dies(real_r_idx(em_ptr->m_ptr));
	em_ptr->caster_lev = (em_ptr->who > 0) ? r_info[em_ptr->m_caster_ptr->r_idx].level : (caster_ptr->lev * 2);
	return em_ptr;
}
