/*!
 * @file hex.c
 * @brief 呪術の処理実装 / Hex code
 * @date 2014/01/14
 * @author
 * 2014 Deskull rearranged comment for Doxygen.\n
 * @details
 * p_ptr-magic_num1\n
 * 0: Flag bits of spelling spells\n
 * 1: Flag bits of despelled spells\n
 * 2: Revange damage\n
 * p_ptr->magic_num2\n
 * 0: Number of spelling spells\n
 * 1: Type of revenge\n
 * 2: Turn count for revenge\n
 */

#include "angband.h"

#define MAX_KEEP 4 /*!<呪術の最大詠唱数 */

/*!
 * @brief プレイヤーが詠唱中の全呪術を停止する
 * @return なし
 */
bool stop_hex_spell_all(void)
{
	int i;

	for (i = 0; i < 32; i++)
	{
		u32b spell = 1L << i;
		if (hex_spelling(spell)) do_spell(REALM_HEX, spell, SPELL_STOP);
	}

	p_ptr->magic_num1[0] = 0;
	p_ptr->magic_num2[0] = 0;

	/* Print message */
	if (p_ptr->action == ACTION_SPELL) set_action(ACTION_NONE);

	/* Redraw status */
	p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
	p_ptr->redraw |= (PR_EXTRA | PR_HP | PR_MANA);

	return TRUE;
}

/*!
 * @brief プレイヤーが詠唱中の呪術から一つを選んで停止する
 * @return なし
 */
bool stop_hex_spell(void)
{
	int spell;
	char choice;
	char out_val[160];
	bool flag = FALSE;
	int y = 1;
	int x = 20;
	int sp[MAX_KEEP];

	if (!hex_spelling_any())
	{
#ifdef JP
		msg_print("呪文を詠唱していません。");
#else
		msg_print("You are casting no spell.");
#endif
		return FALSE;
	}

	/* Stop all spells */
	else if ((p_ptr->magic_num2[0] == 1) || (p_ptr->lev < 35))
	{
		return stop_hex_spell_all();
	}
	else
	{
#ifdef JP
		strnfmt(out_val, 78, "どの呪文の詠唱を中断しますか？(呪文 %c-%c, 'l'全て, ESC)",
			I2A(0), I2A(p_ptr->magic_num2[0] - 1));
#else
		strnfmt(out_val, 78, "Which spell do you stop casting? (Spell %c-%c, 'l' to all, ESC)",
			I2A(0), I2A(p_ptr->magic_num2[0] - 1));
#endif

		screen_save();

		while (!flag)
		{
			int n = 0;
			Term_erase(x, y, 255);
			prt("     名前", y, x + 5);
			for (spell = 0; spell < 32; spell++)
			{
				if (hex_spelling(spell))
				{
					Term_erase(x, y + n + 1, 255);
					put_str(format("%c)  %s", I2A(n), do_spell(REALM_HEX, spell, SPELL_NAME)), y + n + 1, x + 2);
					sp[n++] = spell;
				}
			}

			if (!get_com(out_val, &choice, TRUE)) break;
			if (isupper(choice)) choice = tolower(choice);

			if (choice == 'l')	/* All */
			{
				screen_load();
				return stop_hex_spell_all();
			}
			if ((choice < I2A(0)) || (choice > I2A(p_ptr->magic_num2[0] - 1))) continue;
			flag = TRUE;
		}
	}

	screen_load();

	if (flag)
	{
		int n = sp[A2I(choice)];

		do_spell(REALM_HEX, n, SPELL_STOP);
		p_ptr->magic_num1[0] &= ~(1L << n);
		p_ptr->magic_num2[0]--;
	}

	/* Redraw status */
	p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
	p_ptr->redraw |= (PR_EXTRA | PR_HP | PR_MANA);

	return flag;
}


/*!
 * @brief 一定時間毎に呪術で消費するMPを処理する /
 * Upkeeping hex spells Called from dungeon.c
 * @return なし
 */
void check_hex(void)
{
	int spell;
	s32b need_mana;
	u32b need_mana_frac;
	bool res = FALSE;

	/* Spells spelled by player */
	if (p_ptr->realm1 != REALM_HEX) return;
	if (!p_ptr->magic_num1[0] && !p_ptr->magic_num1[1]) return;

	if (p_ptr->magic_num1[1])
	{
		p_ptr->magic_num1[0] = p_ptr->magic_num1[1];
		p_ptr->magic_num1[1] = 0;
		res = TRUE;
	}

	/* Stop all spells when anti-magic ability is given */
	if (p_ptr->anti_magic)
	{
		stop_hex_spell_all();
		return;
	}

	need_mana = 0;
	for (spell = 0; spell < 32; spell++)
	{
		if (hex_spelling(spell))
		{
			const magic_type *s_ptr;
			s_ptr = &technic_info[REALM_HEX - MIN_TECHNIC][spell];
			need_mana += mod_need_mana(s_ptr->smana, spell, REALM_HEX);
		}
	}


	/* Culcurates final mana cost */
	need_mana_frac = 0;
	s64b_div(&need_mana, &need_mana_frac, 0, 3); /* Divide by 3 */
	need_mana += (p_ptr->magic_num2[0] - 1);


	/* Not enough mana */
	if (s64b_cmp(p_ptr->csp, p_ptr->csp_frac, need_mana, need_mana_frac) < 0)
	{
		stop_hex_spell_all();
		return;
	}

	/* Enough mana */
	else
	{
		s64b_sub(&(p_ptr->csp), &(p_ptr->csp_frac), need_mana, need_mana_frac);

		p_ptr->redraw |= PR_MANA;
		if (res)
		{
#ifdef JP
			msg_print("詠唱を再開した。");
#else
			msg_print("You restart spelling.");
#endif
			p_ptr->action = ACTION_SPELL;

			/* Recalculate bonuses */
			p_ptr->update |= (PU_BONUS | PU_HP);

			/* Redraw map and status bar */
			p_ptr->redraw |= (PR_MAP | PR_STATUS | PR_STATE);

			/* Update monsters */
			p_ptr->update |= (PU_MONSTERS);

			/* Window stuff */
			p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
		}
	}

	/* Gain experiences of spelling spells */
	for (spell = 0; spell < 32; spell++)
	{
		const magic_type *s_ptr;

		if (!hex_spelling(spell)) continue;

		s_ptr = &technic_info[REALM_HEX - MIN_TECHNIC][spell];

		if (p_ptr->spell_exp[spell] < SPELL_EXP_BEGINNER)
			p_ptr->spell_exp[spell] += 5;
		else if(p_ptr->spell_exp[spell] < SPELL_EXP_SKILLED)
		{ if (one_in_(2) && (dun_level > 4) && ((dun_level + 10) > p_ptr->lev)) p_ptr->spell_exp[spell] += 1; }
		else if(p_ptr->spell_exp[spell] < SPELL_EXP_EXPERT)
		{ if (one_in_(5) && ((dun_level + 5) > p_ptr->lev) && ((dun_level + 5) > s_ptr->slevel)) p_ptr->spell_exp[spell] += 1; }
		else if(p_ptr->spell_exp[spell] < SPELL_EXP_MASTER)
		{ if (one_in_(5) && ((dun_level + 5) > p_ptr->lev) && (dun_level > s_ptr->slevel)) p_ptr->spell_exp[spell] += 1; }
	}

	/* Do any effects of continual spells */
	for (spell = 0; spell < 32; spell++)
	{
		if (hex_spelling(spell))
		{
			do_spell(REALM_HEX, spell, SPELL_CONT);
		}
	}
}

/*!
 * @brief プレイヤーの呪術詠唱枠がすでに最大かどうかを返す
 * @return すでに全枠を利用しているならTRUEを返す
 */
bool hex_spell_fully(void)
{
	int k_max = 0;

	k_max = (p_ptr->lev / 15) + 1;

	/* Paranoia */
	k_max = MIN(k_max, MAX_KEEP);

	if (p_ptr->magic_num2[0] < k_max) return FALSE;

	return TRUE;
}

/*!
 * @brief 一定ゲームターン毎に復讐処理の残り期間の判定を行う
 * @return なし
 */
void revenge_spell(void)
{
	if (p_ptr->realm1 != REALM_HEX) return;
	if (p_ptr->magic_num2[2] <= 0) return;

	switch(p_ptr->magic_num2[1])
	{
	case 1: do_spell(REALM_HEX, HEX_PATIENCE, SPELL_CONT); break;
	case 2: do_spell(REALM_HEX, HEX_REVENGE, SPELL_CONT); break;
	}
}

/*!
 * @brief 復讐ダメージの追加を行う
 * @param dam 蓄積されるダメージ量
 * @return なし
 */
void revenge_store(int dam)
{
	if (p_ptr->realm1 != REALM_HEX) return;
	if (p_ptr->magic_num2[2] <= 0) return;

	p_ptr->magic_num1[2] += dam;
}

/*!
 * @brief 反テレポート結界の判定
 * @param m_idx 判定の対象となるモンスターID
 * @return 反テレポートの効果が適用されるならTRUEを返す
 */
bool teleport_barrier(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	if (!hex_spelling(HEX_ANTI_TELE)) return FALSE;
	if ((p_ptr->lev * 3 / 2) < randint1(r_ptr->level)) return FALSE;

	return TRUE;
}

/*!
 * @brief 反魔法結界の判定
 * @param m_idx 判定の対象となるモンスターID
 * @return 反魔法の効果が適用されるならTRUEを返す
 */
bool magic_barrier(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	if (!hex_spelling(HEX_ANTI_MAGIC)) return FALSE;
	if ((p_ptr->lev * 3 / 2) < randint1(r_ptr->level)) return FALSE;

	return TRUE;
}

/*!
 * @brief 反増殖結界の判定
 * @param m_idx 判定の対象となるモンスターID
 * @return 反増殖の効果が適用されるならTRUEを返す
 */
bool multiply_barrier(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	if (!hex_spelling(HEX_ANTI_MULTI)) return FALSE;
	if ((p_ptr->lev * 3 / 2) < randint1(r_ptr->level)) return FALSE;

	return TRUE;
}
