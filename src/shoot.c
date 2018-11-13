#include "angband.h"

/*!
* @brief プレイヤーからモンスターへの射撃命中判定 /
* Determine if the player "hits" a monster (normal combat).
* @param chance 基本命中値
* @param m_ptr モンスターの構造体参照ポインタ
* @param vis 目標を視界に捕らえているならばTRUEを指定
* @param o_name メッセージ表示時のモンスター名
* @return 命中と判定された場合TRUEを返す
* @note Always miss 5%, always hit 5%, otherwise random.
*/
bool test_hit_fire(int chance, monster_type *m_ptr, int vis, char* o_name)
{
	int k;
	ARMOUR_CLASS ac;
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Percentile dice */
	k = randint1(100);

	/* Snipers with high-concentration reduce instant miss percentage.*/
	k += p_ptr->concent;

	/* Hack -- Instant miss or hit */
	if (k <= 5) return (FALSE);
	if (k > 95) return (TRUE);

	if (p_ptr->pseikaku == SEIKAKU_NAMAKE)
		if (one_in_(20)) return (FALSE);

	/* Never hit */
	if (chance <= 0) return (FALSE);

	ac = r_ptr->ac;
	if (p_ptr->concent)
	{
		ac *= (8 - p_ptr->concent);
		ac /= 8;
	}

	if (m_ptr->r_idx == MON_GOEMON && !MON_CSLEEP(m_ptr)) ac *= 3;

	/* Invisible monsters are harder to hit */
	if (!vis) chance = (chance + 1) / 2;

	/* Power competes against armor */
	if (randint0(chance) < (ac * 3 / 4))
	{
		if (m_ptr->r_idx == MON_GOEMON && !MON_CSLEEP(m_ptr))
		{
			char m_name[80];

			/* Extract monster name */
			monster_desc(m_name, m_ptr, 0);
			msg_format(_("%sは%sを斬り捨てた！", "%s cuts down %s!"), m_name, o_name);
		}
		return (FALSE);
	}

	/* Assume hit */
	return (TRUE);
}




/*!
* @brief プレイヤーからモンスターへの射撃クリティカル判定 /
* Critical hits (from objects thrown by player) Factor in item weight, total plusses, and player level.
* @param weight 矢弾の重量
* @param plus_ammo 矢弾の命中修正
* @param plus_bow 弓の命中修正
* @param dam 現在算出中のダメージ値
* @return クリティカル修正が入ったダメージ値
*/
HIT_POINT critical_shot(WEIGHT weight, int plus_ammo, int plus_bow, HIT_POINT dam)
{
	int i, k;
	object_type *j_ptr = &inventory[INVEN_BOW];

	/* Extract "shot" power */
	i = p_ptr->to_h_b + plus_ammo;

	if (p_ptr->tval_ammo == TV_BOLT)
		i = (p_ptr->skill_thb + (p_ptr->weapon_exp[0][j_ptr->sval] / 400 + i) * BTH_PLUS_ADJ);
	else
		i = (p_ptr->skill_thb + ((p_ptr->weapon_exp[0][j_ptr->sval] - (WEAPON_EXP_MASTER / 2)) / 200 + i) * BTH_PLUS_ADJ);


	/* Snipers can shot more critically with crossbows */
	if (p_ptr->concent) i += ((i * p_ptr->concent) / 5);
	if ((p_ptr->pclass == CLASS_SNIPER) && (p_ptr->tval_ammo == TV_BOLT)) i *= 2;

	/* Good bow makes more critical */
	i += plus_bow * 8 * (p_ptr->concent ? p_ptr->concent + 5 : 5);

	/* Critical hit */
	if (randint1(10000) <= i)
	{
		k = weight * randint1(500);

		if (k < 900)
		{
			msg_print(_("手ごたえがあった！", "It was a good hit!"));
			dam += (dam / 2);
		}
		else if (k < 1350)
		{
			msg_print(_("かなりの手ごたえがあった！", "It was a great hit!"));
			dam *= 2;
		}
		else
		{
			msg_print(_("会心の一撃だ！", "It was a superb hit!"));
			dam *= 3;
		}
	}

	return (dam);
}


