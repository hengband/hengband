#include "temporary-resistances.h"
#include "realm/realm-hex.h"
#include "realm/realm-song.h"
#include "player/player-races-table.h"
#include "object-enchant/tr-types.h"
#include "spell/spells-hex.h"

/*!
 * @brief プレイヤーの一時的魔法効果による耐性を返す
 * Prints ratings on certain abilities
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flgs フラグを保管する配列
 * @return なし
 * @todo
 * xtra1.c周りと多重実装になっているのを何とかする
 */
void tim_player_flags(player_type *creature_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE])
{
	for (int i = 0; i < TR_FLAG_SIZE; i++)
		flgs[i] = 0L;

	if (IS_HERO(creature_ptr) || creature_ptr->shero)
		add_flag(flgs, TR_RES_FEAR);
	if (creature_ptr->tim_invis)
		add_flag(flgs, TR_SEE_INVIS);
	if (creature_ptr->tim_regen)
		add_flag(flgs, TR_REGEN);
	if (is_time_limit_esp(creature_ptr))
		add_flag(flgs, TR_TELEPATHY);
	if (IS_FAST(creature_ptr) || creature_ptr->slow)
		add_flag(flgs, TR_SPEED);

	if (is_oppose_acid(creature_ptr) && !(creature_ptr->special_defense & DEFENSE_ACID) && !(PRACE_IS_(creature_ptr, RACE_YEEK) && (creature_ptr->lev > 19)))
		add_flag(flgs, TR_RES_ACID);
	if (is_oppose_elec(creature_ptr) && !(creature_ptr->special_defense & DEFENSE_ELEC))
		add_flag(flgs, TR_RES_ELEC);
	if (is_oppose_fire(creature_ptr) && !(creature_ptr->special_defense & DEFENSE_FIRE))
		add_flag(flgs, TR_RES_FIRE);
	if (is_oppose_cold(creature_ptr) && !(creature_ptr->special_defense & DEFENSE_COLD))
		add_flag(flgs, TR_RES_COLD);
	if (is_oppose_pois(creature_ptr))
		add_flag(flgs, TR_RES_POIS);

	if (creature_ptr->special_attack & ATTACK_ACID)
		add_flag(flgs, TR_BRAND_ACID);
	if (creature_ptr->special_attack & ATTACK_ELEC)
		add_flag(flgs, TR_BRAND_ELEC);
	if (creature_ptr->special_attack & ATTACK_FIRE)
		add_flag(flgs, TR_BRAND_FIRE);
	if (creature_ptr->special_attack & ATTACK_COLD)
		add_flag(flgs, TR_BRAND_COLD);
	if (creature_ptr->special_attack & ATTACK_POIS)
		add_flag(flgs, TR_BRAND_POIS);
	if (creature_ptr->special_defense & DEFENSE_ACID)
		add_flag(flgs, TR_IM_ACID);
	if (creature_ptr->special_defense & DEFENSE_ELEC)
		add_flag(flgs, TR_IM_ELEC);
	if (creature_ptr->special_defense & DEFENSE_FIRE)
		add_flag(flgs, TR_IM_FIRE);
	if (creature_ptr->special_defense & DEFENSE_COLD)
		add_flag(flgs, TR_IM_COLD);
	if (creature_ptr->wraith_form)
		add_flag(flgs, TR_REFLECT);
	if (creature_ptr->tim_reflect)
		add_flag(flgs, TR_REFLECT);

	if (creature_ptr->magicdef)
	{
		add_flag(flgs, TR_RES_BLIND);
		add_flag(flgs, TR_RES_CONF);
		add_flag(flgs, TR_REFLECT);
		add_flag(flgs, TR_FREE_ACT);
		add_flag(flgs, TR_LEVITATION);
	}

	if (creature_ptr->tim_res_nether) add_flag(flgs, TR_RES_NETHER);

	if (creature_ptr->tim_sh_fire) add_flag(flgs, TR_SH_FIRE);

	if (creature_ptr->ult_res)
	{
		add_flag(flgs, TR_RES_FEAR);
		add_flag(flgs, TR_RES_LITE);
		add_flag(flgs, TR_RES_DARK);
		add_flag(flgs, TR_RES_BLIND);
		add_flag(flgs, TR_RES_CONF);
		add_flag(flgs, TR_RES_SOUND);
		add_flag(flgs, TR_RES_SHARDS);
		add_flag(flgs, TR_RES_NETHER);
		add_flag(flgs, TR_RES_NEXUS);
		add_flag(flgs, TR_RES_CHAOS);
		add_flag(flgs, TR_RES_DISEN);
		add_flag(flgs, TR_REFLECT);
		add_flag(flgs, TR_HOLD_EXP);
		add_flag(flgs, TR_FREE_ACT);
		add_flag(flgs, TR_SH_FIRE);
		add_flag(flgs, TR_SH_ELEC);
		add_flag(flgs, TR_SH_COLD);
		add_flag(flgs, TR_LEVITATION);
		add_flag(flgs, TR_LITE_1);
		add_flag(flgs, TR_SEE_INVIS);
		add_flag(flgs, TR_TELEPATHY);
		add_flag(flgs, TR_SLOW_DIGEST);
		add_flag(flgs, TR_REGEN);
		add_flag(flgs, TR_SUST_STR);
		add_flag(flgs, TR_SUST_INT);
		add_flag(flgs, TR_SUST_WIS);
		add_flag(flgs, TR_SUST_DEX);
		add_flag(flgs, TR_SUST_CON);
		add_flag(flgs, TR_SUST_CHR);
	}

	if (creature_ptr->realm1 != REALM_HEX) return;

	if (hex_spelling(creature_ptr, HEX_DEMON_AURA))
	{
		add_flag(flgs, TR_SH_FIRE);
		add_flag(flgs, TR_REGEN);
	}

	if (hex_spelling(creature_ptr, HEX_ICE_ARMOR)) add_flag(flgs, TR_SH_COLD);
	if (hex_spelling(creature_ptr, HEX_SHOCK_CLOAK)) add_flag(flgs, TR_SH_ELEC);
}
