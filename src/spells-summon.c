#include "angband.h"
#include "util.h"

#include "spells.h"
#include "spells-summon.h"
#include "player-inventory.h"
#include "monster-status.h"
#include "floor.h"

/*!
* @brief トランプ魔法独自の召喚処理を行う / Handle summoning and failure of trump spells
* @param num summon_specific()関数を呼び出す回数
* @param pet ペット化として召喚されるか否か
* @param y 召喚位置のy座標
* @param x 召喚位置のx座標
* @param lev 召喚レベル
* @param type 召喚条件ID
* @param mode モンスター生成条件フラグ
* @return モンスターが（敵対も含めて）召還されたならばTRUEを返す。
*/
bool trump_summoning(player_type *caster_ptr, int num, bool pet, POSITION y, POSITION x, DEPTH lev, int type, BIT_FLAGS mode)
{
	PLAYER_LEVEL plev = caster_ptr->lev;

	MONSTER_IDX who;
	int i;
	bool success = FALSE;

	/* Default level */
	if (!lev) lev = plev * 2 / 3 + randint1(plev / 2);

	if (pet)
	{
		/* Become pet */
		mode |= PM_FORCE_PET;

		/* Only sometimes allow unique monster */
		if (mode & PM_ALLOW_UNIQUE)
		{
			/* Forbid often */
			if (randint1(50 + plev) >= plev / 10)
				mode &= ~PM_ALLOW_UNIQUE;
		}

		/* Player is who summons */
		who = -1;
	}
	else
	{
		/* Prevent taming, allow unique monster */
		mode |= PM_NO_PET;

		/* Behave as if they appear by themselfs */
		who = 0;
	}

	for (i = 0; i < num; i++)
	{
		if (summon_specific(who, y, x, lev, type, mode))
			success = TRUE;
	}

	if (!success)
	{
		msg_print(_("誰もあなたのカードの呼び声に答えない。", "Nobody answers to your Trump call."));
	}

	return success;
}


bool cast_summon_demon(player_type *caster_ptr, int power)
{
	u32b flg = 0L;
	bool pet = !one_in_(3);

	if (pet) flg |= PM_FORCE_PET;
	else flg |= PM_NO_PET;
	if (!(pet && (caster_ptr->lev < 50))) flg |= PM_ALLOW_GROUP;

	if (summon_specific((pet ? -1 : 0), caster_ptr->y, caster_ptr->x, power, SUMMON_DEMON, flg))
	{
		msg_print(_("硫黄の悪臭が充満した。", "The area fills with a stench of sulphur and brimstone."));
		if (pet)
		{
			msg_print(_("「ご用でございますか、ご主人様」", "'What is thy bidding... Master?'"));
		}
		else
		{
			msg_print(_("「卑しき者よ、我は汝の下僕にあらず！ お前の魂を頂くぞ！」",
				"'NON SERVIAM! Wretch! I shall feast on thy mortal soul!'"));
		}
	}
	return TRUE;
}

bool cast_summon_undead(player_type *creature_ptr, int power)
{
	bool pet = one_in_(3);
	int type;
	BIT_FLAGS mode = 0L;

	type = (creature_ptr->lev > 47 ? SUMMON_HI_UNDEAD : SUMMON_UNDEAD);

	if (!pet || ((creature_ptr->lev > 24) && one_in_(3))) mode |= PM_ALLOW_GROUP;
	if (pet) mode |= PM_FORCE_PET;
	else mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);

	if (summon_specific((pet ? -1 : 0), creature_ptr->y, creature_ptr->x, power, type, mode))
	{
		msg_print(_("冷たい風があなたの周りに吹き始めた。それは腐敗臭を運んでいる...",
			"Cold winds begin to blow around you, carrying with them the stench of decay..."));
		if (pet)
			msg_print(_("古えの死せる者共があなたに仕えるため土から甦った！",
				"Ancient, long-dead forms arise from the ground to serve you!"));
		else
			msg_print(_("死者が甦った。眠りを妨げるあなたを罰するために！",
				"'The dead arise... to punish you for disturbing them!'"));
	}
	return TRUE;
}


bool cast_summon_hound(player_type *creature_ptr, int power)
{
	BIT_FLAGS mode = PM_ALLOW_GROUP;
	bool pet = !one_in_(5);
	if (pet) mode |= PM_FORCE_PET;
	else mode |= PM_NO_PET;

	if (summon_specific((pet ? -1 : 0), creature_ptr->y, creature_ptr->x, power, SUMMON_HOUND, mode))
	{
		if (pet)
			msg_print(_("ハウンドがあなたの下僕として出現した。", "A group of hounds appear as your servant."));
		else
			msg_print(_("ハウンドはあなたに牙を向けている！", "A group of hounds appear as your enemy!"));
	}
	return TRUE;
}

bool cast_summon_elemental(player_type *creature_ptr, int power)
{
	bool pet = one_in_(3);
	BIT_FLAGS mode = 0L;

	if (!(pet && (creature_ptr->lev < 50))) mode |= PM_ALLOW_GROUP;
	if (pet) mode |= PM_FORCE_PET;
	else mode |= PM_NO_PET;

	if (summon_specific((pet ? -1 : 0), creature_ptr->y, creature_ptr->x, power, SUMMON_ELEMENTAL, mode))
	{
		msg_print(_("エレメンタルが現れた...", "An elemental materializes..."));
		if (pet)
			msg_print(_("あなたに服従しているようだ。", "It seems obedient to you."));
		else
			msg_print(_("それをコントロールできなかった！", "You fail to control it!"));
	}

	return TRUE;
}


bool cast_summon_octopus(player_type *creature_ptr)
{
	BIT_FLAGS mode = PM_ALLOW_GROUP;
	bool pet = !one_in_(5);
	if (pet) mode |= PM_FORCE_PET;

	if (summon_named_creature(0, creature_ptr->y, creature_ptr->x, MON_JIZOTAKO, mode))
	{
		if (pet)
			msg_print(_("蛸があなたの下僕として出現した。", "A group of octopuses appear as your servant."));
		else
			msg_print(_("蛸はあなたを睨んでいる！", "A group of octopuses appear as your enemy!"));
	}

	return TRUE;
}

/*!
* @brief 悪魔領域のグレーターデーモン召喚に利用可能な死体かどうかを返す。 / An "item_tester_hook" for offer
* @param o_ptr オブジェクト構造体の参照ポインタ
* @return 生贄に使用可能な死体ならばTRUEを返す。
*/
bool item_tester_offer(object_type *o_ptr)
{
	/* Flasks of oil are okay */
	if (o_ptr->tval != TV_CORPSE) return (FALSE);
	if (o_ptr->sval != SV_CORPSE) return (FALSE);

	if (my_strchr("pht", r_info[o_ptr->pval].d_char)) return (TRUE);

	/* Assume not okay */
	return (FALSE);
}

/*!
* @brief 悪魔領域のグレーターデーモン召喚を処理する / Daemon spell Summon Greater Demon
* @return 処理を実行したならばTRUEを返す。
*/
bool cast_summon_greater_demon(player_type *caster_ptr)
{
	PLAYER_LEVEL plev = caster_ptr->lev;
	OBJECT_IDX item;
	concptr q, s;
	int summon_lev;
	object_type *o_ptr;

	item_tester_hook = item_tester_offer;
	q = _("どの死体を捧げますか? ", "Sacrifice which corpse? ");
	s = _("捧げられる死体を持っていない。", "You have nothing to scrifice.");
	o_ptr = choose_object(caster_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), 0);
	if (!o_ptr) return FALSE;

	summon_lev = plev * 2 / 3 + r_info[o_ptr->pval].level;

	if (summon_specific(-1, caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_HI_DEMON, (PM_ALLOW_GROUP | PM_FORCE_PET)))
	{
		msg_print(_("硫黄の悪臭が充満した。", "The area fills with a stench of sulphur and brimstone."));
		msg_print(_("「ご用でございますか、ご主人様」", "'What is thy bidding... Master?'"));
		vary_item(caster_ptr, item, -1);
	}
	else
	{
		msg_print(_("悪魔は現れなかった。", "No Greater Demon arrive."));
	}

	return TRUE;
}

/*!
 * @brief 同族召喚(援軍)処理
 * @param level 召喚基準レベル
 * @param y 召喚先Y座標
 * @param x 召喚先X座標
 * @param mode 召喚オプション
 * @return ターンを消費した場合TRUEを返す
 */
bool summon_kin_player(DEPTH level, POSITION y, POSITION x, BIT_FLAGS mode)
{
	bool pet = (bool)(mode & PM_FORCE_PET);
	if (!pet) mode |= PM_NO_PET;
	return summon_specific((pet ? -1 : 0), y, x, level, SUMMON_KIN, mode);
}

/*!
 * @brief サイバーデーモンの召喚
 * @param who 召喚主のモンスターID(0ならばプレイヤー)
 * @param y 召喚位置Y座標
 * @param x 召喚位置X座標
 * @return 作用が実際にあった場合TRUEを返す
 */
int summon_cyber(MONSTER_IDX who, POSITION y, POSITION x)
{
	int i;
	int max_cyber = (easy_band ? 1 : (p_ptr->current_floor_ptr->dun_level / 50) + randint1(2));
	int count = 0;
	BIT_FLAGS mode = PM_ALLOW_GROUP;

	/* Summoned by a monster */
	if (who > 0)
	{
		monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[who];
		if (is_pet(m_ptr)) mode |= PM_FORCE_PET;
	}

	if (max_cyber > 4) max_cyber = 4;

	for (i = 0; i < max_cyber; i++)
	{
		count += summon_specific(who, y, x, 100, SUMMON_CYBER, mode);
	}

	return count;
}


void mitokohmon(player_type *kohmon_ptr)
{
	int count = 0, i;
	monster_type *m_ptr;
	concptr kakusan = "";

	if (summon_named_creature(0, kohmon_ptr->y, kohmon_ptr->x, MON_SUKE, PM_FORCE_PET))
	{
		msg_print(_("『助さん』が現れた。", "Suke-san apperars."));
		kakusan = "Suke-san";
		count++;
	}
	if (summon_named_creature(0, kohmon_ptr->y, kohmon_ptr->x, MON_KAKU, PM_FORCE_PET))
	{
		msg_print(_("『格さん』が現れた。", "Kaku-san appears."));
		kakusan = "Kaku-san";
		count++;
	}
	if (!count)
	{
		for (i = kohmon_ptr->current_floor_ptr->m_max - 1; i > 0; i--)
		{
			m_ptr = &kohmon_ptr->current_floor_ptr->m_list[i];
			if (!monster_is_valid(m_ptr)) continue;
			if (!((m_ptr->r_idx == MON_SUKE) || (m_ptr->r_idx == MON_KAKU))) continue;
			if (!los(kohmon_ptr->current_floor_ptr, m_ptr->fy, m_ptr->fx, kohmon_ptr->y, kohmon_ptr->x)) continue;
			if (!projectable(kohmon_ptr->current_floor_ptr, m_ptr->fy, m_ptr->fx, kohmon_ptr->y, kohmon_ptr->x)) continue;
			count++;
			break;
		}
	}

	if (count)
	{
		msg_format(_("「者ども、ひかえおろう！！！このお方をどなたとこころえる。」",
			"%^s says 'WHO do you think this person is! Bow your head, down your knees!'"), kakusan);
		sukekaku = TRUE;
		stun_monsters(kohmon_ptr, 120);
		confuse_monsters(kohmon_ptr, 120);
		turn_monsters(kohmon_ptr, 120);
		stasis_monsters(kohmon_ptr, 120);
		sukekaku = FALSE;
	}
	else
	{
		msg_print(_("しかし、何も起きなかった。", "Nothing happen."));
	}
}
