#include "angband.h"
#include "effect/effect-monster-util.h"

static void substitute_effect_monster(effect_monster *effect_monster_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID effect_type, BIT_FLAGS flag, bool see_s_msg)
{
	effect_monster_ptr->who = who;
	effect_monster_ptr->r = r;
	effect_monster_ptr->y = y;
	effect_monster_ptr->x = x;
	effect_monster_ptr->dam = dam;
	effect_monster_ptr->effect_type = effect_type;
	effect_monster_ptr->flag = flag;
	effect_monster_ptr->see_s_msg = see_s_msg;
}


/*!
 * @brief effect_monster構造体を初期化する
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param effect_monster_ptr モンスター効果構造体への参照ポインタ
 * @param 魔法を発動したモンスター (0ならばプレーヤー)
 * @param 目標y座標
 * @param 目標x座標
 * @return なし
 */
void initialize_effect_monster(player_type *caster_ptr, effect_monster *effect_monster_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID effect_type, BIT_FLAGS flag, bool see_s_msg)
{
	substitute_effect_monster(effect_monster_ptr, who, r, y, x, dam, effect_type, flag, see_s_msg);

	floor_type *floor_ptr = caster_ptr->current_floor_ptr;
	effect_monster_ptr->g_ptr = &floor_ptr->grid_array[effect_monster_ptr->y][effect_monster_ptr->x];
	effect_monster_ptr->m_ptr = &floor_ptr->m_list[effect_monster_ptr->g_ptr->m_idx];
	effect_monster_ptr->m_caster_ptr = (effect_monster_ptr->who > 0) ? &floor_ptr->m_list[effect_monster_ptr->who] : NULL;
	effect_monster_ptr->r_ptr = &r_info[effect_monster_ptr->m_ptr->r_idx];
	effect_monster_ptr->seen = effect_monster_ptr->m_ptr->ml;
	effect_monster_ptr->seen_msg = is_seen(effect_monster_ptr->m_ptr);
	effect_monster_ptr->slept = (bool)MON_CSLEEP(effect_monster_ptr->m_ptr);
	effect_monster_ptr->obvious = FALSE;
	effect_monster_ptr->known = ((effect_monster_ptr->m_ptr->cdis <= MAX_SIGHT) || caster_ptr->phase_out);
	effect_monster_ptr->skipped = FALSE;
	effect_monster_ptr->get_angry = FALSE;
	effect_monster_ptr->do_poly = FALSE;
	effect_monster_ptr->do_dist = 0;
	effect_monster_ptr->do_conf = 0;
	effect_monster_ptr->do_stun = 0;
	effect_monster_ptr->do_sleep = 0;
	effect_monster_ptr->do_fear = 0;
	effect_monster_ptr->do_time = 0;
	effect_monster_ptr->heal_leper = FALSE;
	effect_monster_ptr->photo = 0;
	effect_monster_ptr->note = NULL;
	effect_monster_ptr->note_dies = extract_note_dies(real_r_idx(effect_monster_ptr->m_ptr));
	effect_monster_ptr->caster_lev = (effect_monster_ptr->who > 0) ? r_info[effect_monster_ptr->m_caster_ptr->r_idx].level : (caster_ptr->lev * 2);
}
