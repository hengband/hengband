/*!
 * @file mspells4.c
 * @brief スペル実行処理 / Spell launch
 * @date 2014/07/14
 * @author Habu
 */

#include "system/angband.h"
#include "main/sound-definitions-table.h"
#include "effect/effect-characteristics.h"
#include "grid/grid.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "mspell/monster-spell.h"
#include "spell/spells-type.h"
#include "combat/melee.h"
#include "player/player-effects.h"
#include "spell/process-effect.h"
#include "spell/spells3.h"
#include "mspell/mspell-summon.h"
#include "mspell/mspell-util.h"
#include "mspell/mspell-curse.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-breath.h"
#include "mspell/mspell-ball.h"
#include "mspell/mspell-bolt.h"
#include "mspell/mspell-floor.h"
#include "mspell/mspell-status.h"

/*!
* @brief RF4_ROCKETの処理。ロケット。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param y 対象の地点のy座標
* @param x 対象の地点のx座標
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
* @return ダメージ量を返す。
*/
HIT_POINT spell_RF4_ROCKET(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	HIT_POINT dam;

	monspell_message(target_ptr, m_idx, t_idx,
		_("%^sが何かを射った。", "%^s shoots something."),
		_("%^sがロケットを発射した。", "%^s fires a rocket."),
		_("%^sが%sにロケットを発射した。", "%^s fires a rocket at %s."),
		TARGET_TYPE);

	dam = monspell_damage(target_ptr, (MS_ROCKET), m_idx, DAM_ROLL);
	breath(target_ptr, y, x, m_idx, GF_ROCKET, dam, 2, FALSE, MS_ROCKET, TARGET_TYPE);
	if (TARGET_TYPE == MONSTER_TO_PLAYER)
		update_smart_learn(target_ptr, m_idx, DRS_SHARD);
	return dam;
}


/*!
* @brief RF6_HAND_DOOMの処理。破滅の手。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param y 対象の地点のy座標
* @param x 対象の地点のx座標
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
* @return ダメージ量を返す。
*/
HIT_POINT spell_RF6_HAND_DOOM(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	simple_monspell_message(target_ptr, m_idx, t_idx,
		_("%^sが<破滅の手>を放った！", "%^s invokes the Hand of Doom!"),
		_("%^sが%sに<破滅の手>を放った！", "%^s invokes the Hand of Doom upon %s!"),
		TARGET_TYPE);

	HIT_POINT dam = 0;
	if (TARGET_TYPE == MONSTER_TO_PLAYER)
	{
		dam = monspell_damage(target_ptr, (MS_HAND_DOOM), m_idx, DAM_ROLL);
		breath(target_ptr, y, x, m_idx, GF_HAND_DOOM, dam, 0, FALSE, MS_HAND_DOOM, MONSTER_TO_PLAYER);
	}
	else if (TARGET_TYPE == MONSTER_TO_MONSTER)
	{
		dam = 20; /* Dummy power */
		breath(target_ptr, y, x, m_idx, GF_HAND_DOOM, dam, 0, FALSE, MS_HAND_DOOM, MONSTER_TO_MONSTER);
	}

	return dam;
}


/*!
* @brief バーノール・ルパートのRF6_SPECIALの処理。分裂・合体。 /
* @param player_ptr プレーヤーへの参照ポインタ
* @param m_idx 呪文を唱えるモンスターID
*/
HIT_POINT spell_RF6_SPECIAL_BANORLUPART(player_type *target_ptr, MONSTER_IDX m_idx)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	HIT_POINT dummy_hp, dummy_maxhp;
	POSITION dummy_y = m_ptr->fy;
	POSITION dummy_x = m_ptr->fx;
	BIT_FLAGS mode = 0L;

	switch (m_ptr->r_idx)
	{
	case MON_BANORLUPART:
		dummy_hp = (m_ptr->hp + 1) / 2;
		dummy_maxhp = m_ptr->maxhp / 2;

		if (floor_ptr->inside_arena || target_ptr->phase_out || !summon_possible(target_ptr, m_ptr->fy, m_ptr->fx))
			return -1;

		delete_monster_idx(target_ptr, floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].m_idx);
		summon_named_creature(target_ptr, 0, dummy_y, dummy_x, MON_BANOR, mode);
		floor_ptr->m_list[hack_m_idx_ii].hp = dummy_hp;
		floor_ptr->m_list[hack_m_idx_ii].maxhp = dummy_maxhp;
		summon_named_creature(target_ptr, 0, dummy_y, dummy_x, MON_LUPART, mode);
		floor_ptr->m_list[hack_m_idx_ii].hp = dummy_hp;
		floor_ptr->m_list[hack_m_idx_ii].maxhp = dummy_maxhp;

		msg_print(_("『バーノール・ルパート』が分裂した！", "Banor=Rupart splits into two persons!"));
		break;

	case MON_BANOR:
	case MON_LUPART:
		dummy_hp = 0;
		dummy_maxhp = 0;

		if (!r_info[MON_BANOR].cur_num || !r_info[MON_LUPART].cur_num)
			return -1;

		for (MONSTER_IDX k = 1; k < floor_ptr->m_max; k++)
		{
			if (floor_ptr->m_list[k].r_idx == MON_BANOR || floor_ptr->m_list[k].r_idx == MON_LUPART)
			{
				dummy_hp += floor_ptr->m_list[k].hp;
				dummy_maxhp += floor_ptr->m_list[k].maxhp;
				if (floor_ptr->m_list[k].r_idx != m_ptr->r_idx)
				{
					dummy_y = floor_ptr->m_list[k].fy;
					dummy_x = floor_ptr->m_list[k].fx;
				}
				delete_monster_idx(target_ptr, k);
			}
		}
		summon_named_creature(target_ptr, 0, dummy_y, dummy_x, MON_BANORLUPART, mode);
		floor_ptr->m_list[hack_m_idx_ii].hp = dummy_hp;
		floor_ptr->m_list[hack_m_idx_ii].maxhp = dummy_maxhp;

		msg_print(_("『バーノール』と『ルパート』が合体した！", "Banor and Rupart combine into one!"));
		break;
	}

	return 0;
}


/*!
* @brief ロレントのRF6_SPECIALの処理。手榴弾の召喚。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param y 対象の地点のy座標
* @param x 対象の地点のx座標
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
* @return ダメージ量を返す。
*/
HIT_POINT spell_RF6_SPECIAL_ROLENTO(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	int count = 0, k;
	int num = 1 + randint1(3);
	BIT_FLAGS mode = 0L;

	monspell_message(target_ptr, m_idx, t_idx,
		_("%^sが何か大量に投げた。", "%^s spreads something."),
		_("%^sは手榴弾をばらまいた。", "%^s throws some hand grenades."),
		_("%^sは手榴弾をばらまいた。", "%^s throws some hand grenades."),
		TARGET_TYPE);

	for (k = 0; k < num; k++)
	{
		count += summon_named_creature(target_ptr, m_idx, y, x, MON_GRENADE, mode);
	}
	if (target_ptr->blind && count)
	{
		msg_print(_("多くのものが間近にばらまかれる音がする。", "You hear many things scattered nearby."));
	}
	return 0;
}


/*!
* @brief BシンボルのRF6_SPECIALの処理。投げ落とす攻撃。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param y 対象の地点のy座標
* @param x 対象の地点のx座標
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
* @return ダメージ量を返す。
*/
HIT_POINT spell_RF6_SPECIAL_B(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	HIT_POINT dam = -1;
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	monster_type *t_ptr = &floor_ptr->m_list[t_idx];
	monster_race *tr_ptr = &r_info[t_ptr->r_idx];
	bool monster_to_player = (TARGET_TYPE == MONSTER_TO_PLAYER);
	bool monster_to_monster = (TARGET_TYPE == MONSTER_TO_MONSTER);
	bool direct = player_bold(target_ptr, y, x);
	GAME_TEXT m_name[MAX_NLEN];
	monster_name(target_ptr, m_idx, m_name);

	disturb(target_ptr, TRUE, TRUE);
	if (one_in_(3) || !direct)
	{
		simple_monspell_message(target_ptr, m_idx, t_idx,
			_("%^sは突然視界から消えた!", "%^s suddenly go out of your sight!"),
			_("%^sは突然急上昇して視界から消えた!", "%^s suddenly go out of your sight!"),
			TARGET_TYPE);

		teleport_away(target_ptr, m_idx, 10, TELEPORT_NONMAGICAL);
		target_ptr->update |= (PU_MONSTERS);
		return dam;
	}

	int get_damage = 0;
	bool fear, dead; /* dummy */

	simple_monspell_message(target_ptr, m_idx, t_idx,
		_("%^sがあなたを掴んで空中から投げ落とした。", "%^s holds you, and drops from the sky."),
		_("%^sが%sを掴んで空中から投げ落とした。", "%^s holds %s, and drops from the sky."),
		TARGET_TYPE);

	dam = damroll(4, 8);

	if (monster_to_player || t_idx == target_ptr->riding)
		teleport_player_to(target_ptr, m_ptr->fy, m_ptr->fx, TELEPORT_NONMAGICAL | TELEPORT_PASSIVE);
	else
		teleport_monster_to(target_ptr, t_idx, m_ptr->fy, m_ptr->fx, 100, TELEPORT_NONMAGICAL | TELEPORT_PASSIVE);

	sound(SOUND_FALL);

	if ((monster_to_player && target_ptr->levitation) ||
		(monster_to_monster && (tr_ptr->flags7 & RF7_CAN_FLY)))
	{
		simple_monspell_message(target_ptr, m_idx, t_idx,
			_("あなたは静かに着地した。", "You float gently down to the ground."),
			_("%^sは静かに着地した。", "%^s floats gently down to the ground."),
			TARGET_TYPE);
	}
	else
	{
		simple_monspell_message(target_ptr, m_idx, t_idx,
			_("あなたは地面に叩きつけられた。", "You crashed into the ground."),
			_("%^sは地面に叩きつけられた。", "%^s crashed into the ground."),
			TARGET_TYPE);
		dam += damroll(6, 8);
	}

	if (monster_to_player ||
		(monster_to_monster && target_ptr->riding == t_idx))
	{
		get_damage = take_hit(target_ptr, DAMAGE_NOESCAPE, dam, m_name, -1);
		if (target_ptr->tim_eyeeye && get_damage > 0 && !target_ptr->is_dead)
		{
			GAME_TEXT m_name_self[80];
			monster_desc(target_ptr, m_name_self, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE);
			msg_format(_("攻撃が%s自身を傷つけた！", "The attack of %s has wounded %s!"), m_name, m_name_self);
			project(target_ptr, 0, 0, m_ptr->fy, m_ptr->fx, get_damage, GF_MISSILE, PROJECT_KILL, -1);
			set_tim_eyeeye(target_ptr, target_ptr->tim_eyeeye - 5, TRUE);
		}
	}

	if (monster_to_player && target_ptr->riding)
		mon_take_hit_mon(target_ptr, target_ptr->riding, dam, &dead, &fear, extract_note_dies(real_r_idx(&floor_ptr->m_list[target_ptr->riding])), m_idx);

	if (monster_to_monster)
		mon_take_hit_mon(target_ptr, t_idx, dam, &dead, &fear, extract_note_dies(real_r_idx(t_ptr)), m_idx);
	return dam;
}


/*!
* @brief RF6_SPECIALの処理。モンスターの種類によって実処理に振り分ける。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param y 対象の地点のy座標
* @param x 対象の地点のx座標
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
* @return ダメージ量を返す。
*/
HIT_POINT spell_RF6_SPECIAL(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	disturb(target_ptr, TRUE, TRUE);
	switch (m_ptr->r_idx)
	{
	case MON_OHMU:
		return -1;

	case MON_BANORLUPART:
	case MON_BANOR:
	case MON_LUPART:
		return spell_RF6_SPECIAL_BANORLUPART(target_ptr, m_idx);

	case MON_ROLENTO:
		return spell_RF6_SPECIAL_ROLENTO(target_ptr, y, x, m_idx, t_idx, TARGET_TYPE);
		break;

	default:
		if (r_ptr->d_char == 'B')
		{
			return spell_RF6_SPECIAL_B(target_ptr, y, x, m_idx, t_idx, TARGET_TYPE);
			break;
		}

		else return -1;
	}
}


/*!
* @brief RF6_PSY_SPEARの処理。光の剣。 /
* @param y 対象の地点のy座標
* @param x 対象の地点のx座標
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
* @return ダメージ量を返す。
*/
HIT_POINT spell_RF6_PSY_SPEAR(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	monspell_message(target_ptr, m_idx, t_idx,
		_("%^sが何かをつぶやいた。", "%^s mumbles."),
		_("%^sが光の剣を放った。", "%^s throw a Psycho-Spear."),
		_("%^sが%sに向かって光の剣を放った。", "%^s throw a Psycho-spear at %s."),
		TARGET_TYPE);

	HIT_POINT dam = monspell_damage(target_ptr, (MS_PSY_SPEAR), m_idx, DAM_ROLL);
	beam(target_ptr, m_idx, y, x, GF_PSY_SPEAR, dam, MS_PSY_SPEAR, MONSTER_TO_PLAYER);
	return dam;
}


/*!
* @brief モンスターからプレイヤーへの呪文の振り分け関数。 /
* @param SPELL_NUM モンスター魔法ID
* @param y 対象の地点のy座標
* @param x 対象の地点のx座標
* @param m_idx 呪文を唱えるモンスターID
* @return 攻撃呪文のダメージ、または召喚したモンスターの数を返す。その他の場合0。以降の処理を中断するなら-1を返す。
*/
HIT_POINT monspell_to_player(player_type* target_ptr, monster_spell_type ms_type, POSITION y, POSITION x, MONSTER_IDX m_idx)
{
    switch (ms_type)
	{
	case RF4_SPELL_START + 0:   spell_RF4_SHRIEK(m_idx, target_ptr, 0, MONSTER_TO_PLAYER); break;	/* RF4_SHRIEK */
	case RF4_SPELL_START + 1:   break;   /* RF4_XXX1 */
	case RF4_SPELL_START + 2:   spell_RF4_DISPEL(m_idx, target_ptr, 0, MONSTER_TO_PLAYER); break;	/* RF4_DISPEL */
	case RF4_SPELL_START + 3:   return spell_RF4_ROCKET(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER);  /* RF4_ROCKET */
	case RF4_SPELL_START + 4:   return spell_RF4_SHOOT(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER);   /* RF4_SHOOT */
	case RF4_SPELL_START + 5:   break;   /* RF4_XXX2 */
	case RF4_SPELL_START + 6:   break;   /* RF4_XXX3 */
	case RF4_SPELL_START + 7:   break;   /* RF4_XXX4 */
	case RF4_SPELL_START + 8:   return spell_RF4_BREATH(target_ptr, GF_ACID, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_ACID */
	case RF4_SPELL_START + 9:   return spell_RF4_BREATH(target_ptr, GF_ELEC, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_ELEC */
	case RF4_SPELL_START + 10:  return spell_RF4_BREATH(target_ptr, GF_FIRE, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_FIRE */
	case RF4_SPELL_START + 11:  return spell_RF4_BREATH(target_ptr, GF_COLD, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_COLD */
	case RF4_SPELL_START + 12:  return spell_RF4_BREATH(target_ptr, GF_POIS, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_POIS */
	case RF4_SPELL_START + 13:  return spell_RF4_BREATH(target_ptr, GF_NETHER, y, x, m_idx, 0, MONSTER_TO_PLAYER);   /* RF4_BR_NETH */
	case RF4_SPELL_START + 14:  return spell_RF4_BREATH(target_ptr, GF_LITE, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_LITE */
	case RF4_SPELL_START + 15:  return spell_RF4_BREATH(target_ptr, GF_DARK, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_DARK */
	case RF4_SPELL_START + 16:  return spell_RF4_BREATH(target_ptr, GF_CONFUSION, y, x, m_idx, 0, MONSTER_TO_PLAYER);	/* RF4_BR_CONF */
	case RF4_SPELL_START + 17:  return spell_RF4_BREATH(target_ptr, GF_SOUND, y, x, m_idx, 0, MONSTER_TO_PLAYER);	/* RF4_BR_SOUN */
	case RF4_SPELL_START + 18:  return spell_RF4_BREATH(target_ptr, GF_CHAOS, y, x, m_idx, 0, MONSTER_TO_PLAYER);	/* RF4_BR_CHAO */
	case RF4_SPELL_START + 19:  return spell_RF4_BREATH(target_ptr, GF_DISENCHANT, y, x, m_idx, 0, MONSTER_TO_PLAYER);   /* RF4_BR_DISE */
	case RF4_SPELL_START + 20:  return spell_RF4_BREATH(target_ptr, GF_NEXUS, y, x, m_idx, 0, MONSTER_TO_PLAYER);	/* RF4_BR_NEXU */
	case RF4_SPELL_START + 21:  return spell_RF4_BREATH(target_ptr, GF_TIME, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_TIME */
	case RF4_SPELL_START + 22:  return spell_RF4_BREATH(target_ptr, GF_INERTIAL, y, x, m_idx, 0, MONSTER_TO_PLAYER);  /* RF4_BR_INER */
	case RF4_SPELL_START + 23:  return spell_RF4_BREATH(target_ptr, GF_GRAVITY, y, x, m_idx, 0, MONSTER_TO_PLAYER);  /* RF4_BR_GRAV */
	case RF4_SPELL_START + 24:  return spell_RF4_BREATH(target_ptr, GF_SHARDS, y, x, m_idx, 0, MONSTER_TO_PLAYER);   /* RF4_BR_SHAR */
	case RF4_SPELL_START + 25:  return spell_RF4_BREATH(target_ptr, GF_PLASMA, y, x, m_idx, 0, MONSTER_TO_PLAYER);   /* RF4_BR_PLAS */
	case RF4_SPELL_START + 26:  return spell_RF4_BREATH(target_ptr, GF_FORCE, y, x, m_idx, 0, MONSTER_TO_PLAYER);	/* RF4_BR_WALL */
	case RF4_SPELL_START + 27:  return spell_RF4_BREATH(target_ptr, GF_MANA, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_MANA */
	case RF4_SPELL_START + 28:  return spell_RF4_BA_NUKE(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BA_NUKE */
	case RF4_SPELL_START + 29:  return spell_RF4_BREATH(target_ptr, GF_NUKE, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_NUKE */
	case RF4_SPELL_START + 30:  return spell_RF4_BA_CHAO(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BA_CHAO */
	case RF4_SPELL_START + 31:  return spell_RF4_BREATH(target_ptr, GF_DISINTEGRATE, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_DISI */
	case RF5_SPELL_START + 0:  return spell_RF5_BA_ACID(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BA_ACID */
	case RF5_SPELL_START + 1:  return spell_RF5_BA_ELEC(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BA_ELEC */
	case RF5_SPELL_START + 2:  return spell_RF5_BA_FIRE(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BA_FIRE */
	case RF5_SPELL_START + 3:  return spell_RF5_BA_COLD(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BA_COLD */
	case RF5_SPELL_START + 4:  return spell_RF5_BA_POIS(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BA_POIS */
	case RF5_SPELL_START + 5:  return spell_RF5_BA_NETH(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BA_NETH */
	case RF5_SPELL_START + 6:  return spell_RF5_BA_WATE(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BA_WATE */
	case RF5_SPELL_START + 7:  return spell_RF5_BA_MANA(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BA_MANA */
	case RF5_SPELL_START + 8:  return spell_RF5_BA_DARK(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BA_DARK */
	case RF5_SPELL_START + 9:  return spell_RF5_DRAIN_MANA(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER);  /* RF5_DRAIN_MANA */
	case RF5_SPELL_START + 10: return spell_RF5_MIND_BLAST(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER);  /* RF5_MIND_BLAST */
	case RF5_SPELL_START + 11: return spell_RF5_BRAIN_SMASH(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_MIND_BLAST */
	case RF5_SPELL_START + 12: return spell_RF5_CAUSE_1(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_CAUSE_1 */
	case RF5_SPELL_START + 13: return spell_RF5_CAUSE_2(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_CAUSE_2 */
	case RF5_SPELL_START + 14: return spell_RF5_CAUSE_3(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_CAUSE_3 */
	case RF5_SPELL_START + 15: return spell_RF5_CAUSE_4(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_CAUSE_4 */
	case RF5_SPELL_START + 16: return spell_RF5_BO_ACID(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BO_ACID */
	case RF5_SPELL_START + 17: return spell_RF5_BO_ELEC(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BO_ELEC */
	case RF5_SPELL_START + 18: return spell_RF5_BO_FIRE(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BO_FIRE */
	case RF5_SPELL_START + 19: return spell_RF5_BO_COLD(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BO_COLD */
	case RF5_SPELL_START + 20: return spell_RF5_BA_LITE(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BA_LITE */
	case RF5_SPELL_START + 21: return spell_RF5_BO_NETH(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BO_NETH */
	case RF5_SPELL_START + 22: return spell_RF5_BO_WATE(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BO_WATE */
	case RF5_SPELL_START + 23: return spell_RF5_BO_MANA(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BO_MANA */
	case RF5_SPELL_START + 24: return spell_RF5_BO_PLAS(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BO_PLAS */
	case RF5_SPELL_START + 25: return spell_RF5_BO_ICEE(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BO_ICEE */
	case RF5_SPELL_START + 26: return spell_RF5_MISSILE(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_MISSILE */
	case RF5_SPELL_START + 27: spell_RF5_SCARE(m_idx, target_ptr, 0, MONSTER_TO_PLAYER); break;   /* RF5_SCARE */
	case RF5_SPELL_START + 28: spell_RF5_BLIND(m_idx, target_ptr, 0, MONSTER_TO_PLAYER); break;   /* RF5_BLIND */
	case RF5_SPELL_START + 29: spell_RF5_CONF(m_idx, target_ptr, 0, MONSTER_TO_PLAYER); break;  /* RF5_CONF */
	case RF5_SPELL_START + 30: spell_RF5_SLOW(m_idx, target_ptr, 0, MONSTER_TO_PLAYER); break;  /* RF5_SLOW */
	case RF5_SPELL_START + 31: spell_RF5_HOLD(m_idx, target_ptr, 0, MONSTER_TO_PLAYER); break;  /* RF5_HOLD */
	case RF6_SPELL_START + 0:  spell_RF6_HASTE(target_ptr, m_idx, 0, MONSTER_TO_PLAYER); break;   /* RF6_HASTE */
	case RF6_SPELL_START + 1:  return spell_RF6_HAND_DOOM(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_HAND_DOOM */
	case RF6_SPELL_START + 2:  spell_RF6_HEAL(target_ptr, m_idx, 0, MONSTER_TO_PLAYER); break;	/* RF6_HEAL */
	case RF6_SPELL_START + 3:  spell_RF6_INVULNER(target_ptr, m_idx, 0, MONSTER_TO_PLAYER); break;	/* RF6_INVULNER */
	case RF6_SPELL_START + 4:  spell_RF6_BLINK(target_ptr, m_idx, MONSTER_TO_PLAYER, FALSE); break;   /* RF6_BLINK */
	case RF6_SPELL_START + 5:  spell_RF6_TPORT(target_ptr, m_idx, MONSTER_TO_PLAYER); break;   /* RF6_TPORT */
	case RF6_SPELL_START + 6:  return spell_RF6_WORLD(target_ptr, m_idx); break;	/* RF6_WORLD */
	case RF6_SPELL_START + 7:  return spell_RF6_SPECIAL(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER);   /* RF6_SPECIAL */
	case RF6_SPELL_START + 8:  spell_RF6_TELE_TO(target_ptr, m_idx, 0, MONSTER_TO_PLAYER); break; /* RF6_TELE_TO */
	case RF6_SPELL_START + 9:  spell_RF6_TELE_AWAY(target_ptr, m_idx, 0, MONSTER_TO_PLAYER); break;   /* RF6_TELE_AWAY */
	case RF6_SPELL_START + 10: spell_RF6_TELE_LEVEL(target_ptr, m_idx, 0, MONSTER_TO_PLAYER); break;  /* RF6_TELE_LEVEL */
	case RF6_SPELL_START + 11: spell_RF6_PSY_SPEAR(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break; /* RF6_PSY_SPEAR */
	case RF6_SPELL_START + 12: spell_RF6_DARKNESS(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;	/* RF6_DARKNESS */
	case RF6_SPELL_START + 13: spell_RF6_TRAPS(target_ptr, y, x, m_idx); break; /* RF6_TRAPS */
	case RF6_SPELL_START + 14: spell_RF6_FORGET(target_ptr, m_idx); break;  /* RF6_FORGET */
	case RF6_SPELL_START + 15: spell_RF6_RAISE_DEAD(target_ptr, m_idx, 0, MONSTER_TO_PLAYER); break;  /* RF6_RAISE_DEAD */
	case RF6_SPELL_START + 16: spell_RF6_S_KIN(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break; /* RF6_S_KIN */
	case RF6_SPELL_START + 17: spell_RF6_S_CYBER(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;   /* RF6_S_CYBER */
	case RF6_SPELL_START + 18: spell_RF6_S_MONSTER(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break; /* RF6_S_MONSTER */
	case RF6_SPELL_START + 19: spell_RF6_S_MONSTERS(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;	/* RF6_S_MONSTER */
	case RF6_SPELL_START + 20: spell_RF6_S_ANT(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break; /* RF6_S_ANT */
	case RF6_SPELL_START + 21: spell_RF6_S_SPIDER(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;  /* RF6_S_SPIDER */
	case RF6_SPELL_START + 22: spell_RF6_S_HOUND(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;   /* RF6_S_HOUND */
	case RF6_SPELL_START + 23: spell_RF6_S_HYDRA(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;   /* RF6_S_HYDRA */
	case RF6_SPELL_START + 24: spell_RF6_S_ANGEL(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;   /* RF6_S_ANGEL */
	case RF6_SPELL_START + 25: spell_RF6_S_DEMON(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;   /* RF6_S_DEMON */
	case RF6_SPELL_START + 26: spell_RF6_S_UNDEAD(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;  /* RF6_S_UNDEAD */
	case RF6_SPELL_START + 27: spell_RF6_S_DRAGON(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;  /* RF6_S_DRAGON */
	case RF6_SPELL_START + 28: spell_RF6_S_HI_UNDEAD(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;   /* RF6_S_HI_UNDEAD */
	case RF6_SPELL_START + 29: spell_RF6_S_HI_DRAGON(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;   /* RF6_S_HI_DRAGON */
	case RF6_SPELL_START + 30: spell_RF6_S_AMBERITES(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;   /* RF6_S_AMBERITES */
	case RF6_SPELL_START + 31: spell_RF6_S_UNIQUE(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;  /* RF6_S_UNIQUE */
	}

	return 0;
}


/*!
* todo モンスターからモンスターへの呪文なのにplayer_typeが引数になり得るのは間違っている……
* @brief モンスターからモンスターへの呪文の振り分け関数。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param SPELL_NUM モンスター魔法ID
* @param y 対象の地点のy座標
* @param x 対象の地点のx座標
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param is_special_spell 特殊な行動である時TRUE
* @return 攻撃呪文のダメージ、または召喚したモンスターの数を返す。その他の場合0。以降の処理を中断するなら-1を返す。
*/
HIT_POINT monspell_to_monster(player_type* target_ptr, monster_spell_type ms_type, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, bool is_special_spell)
{
    switch (ms_type)
	{
	case RF4_SPELL_START + 0:   spell_RF4_SHRIEK(m_idx, target_ptr, t_idx, MONSTER_TO_MONSTER); break;   /* RF4_SHRIEK */
	case RF4_SPELL_START + 1:   return -1;   /* RF4_XXX1 */
	case RF4_SPELL_START + 2:   spell_RF4_DISPEL(m_idx, target_ptr, t_idx, MONSTER_TO_MONSTER); break;   /* RF4_DISPEL */
	case RF4_SPELL_START + 3:   return spell_RF4_ROCKET(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF4_ROCKET */
	case RF4_SPELL_START + 4:   return spell_RF4_SHOOT(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);  /* RF4_SHOOT */
	case RF4_SPELL_START + 5:   return -1;   /* RF4_XXX2 */
	case RF4_SPELL_START + 6:   return -1;   /* RF4_XXX3 */
	case RF4_SPELL_START + 7:   return -1;   /* RF4_XXX4 */
	case RF4_SPELL_START + 8:   return spell_RF4_BREATH(target_ptr, GF_ACID, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_ACID */
	case RF4_SPELL_START + 9:   return spell_RF4_BREATH(target_ptr, GF_ELEC, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_ELEC */
	case RF4_SPELL_START + 10:  return spell_RF4_BREATH(target_ptr, GF_FIRE, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_FIRE */
	case RF4_SPELL_START + 11:  return spell_RF4_BREATH(target_ptr, GF_COLD, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_COLD */
	case RF4_SPELL_START + 12:  return spell_RF4_BREATH(target_ptr, GF_POIS, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_POIS */
	case RF4_SPELL_START + 13:  return spell_RF4_BREATH(target_ptr, GF_NETHER, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);  /* RF4_BR_NETH */
	case RF4_SPELL_START + 14:  return spell_RF4_BREATH(target_ptr, GF_LITE, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_LITE */
	case RF4_SPELL_START + 15:  return spell_RF4_BREATH(target_ptr, GF_DARK, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_DARK */
	case RF4_SPELL_START + 16:  return spell_RF4_BREATH(target_ptr, GF_CONFUSION, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);   /* RF4_BR_CONF */
	case RF4_SPELL_START + 17:  return spell_RF4_BREATH(target_ptr, GF_SOUND, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);   /* RF4_BR_SOUN */
	case RF4_SPELL_START + 18:  return spell_RF4_BREATH(target_ptr, GF_CHAOS, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);   /* RF4_BR_CHAO */
	case RF4_SPELL_START + 19:  return spell_RF4_BREATH(target_ptr, GF_DISENCHANT, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);  /* RF4_BR_DISE */
	case RF4_SPELL_START + 20:  return spell_RF4_BREATH(target_ptr, GF_NEXUS, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);   /* RF4_BR_NEXU */
	case RF4_SPELL_START + 21:  return spell_RF4_BREATH(target_ptr, GF_TIME, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_TIME */
	case RF4_SPELL_START + 22:  return spell_RF4_BREATH(target_ptr, GF_INERTIAL, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF4_BR_INER */
	case RF4_SPELL_START + 23:  return spell_RF4_BREATH(target_ptr, GF_GRAVITY, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF4_BR_GRAV */
	case RF4_SPELL_START + 24:  return spell_RF4_BREATH(target_ptr, GF_SHARDS, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);  /* RF4_BR_SHAR */
	case RF4_SPELL_START + 25:  return spell_RF4_BREATH(target_ptr, GF_PLASMA, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);  /* RF4_BR_PLAS */
	case RF4_SPELL_START + 26:  return spell_RF4_BREATH(target_ptr, GF_FORCE, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);   /* RF4_BR_WALL */
	case RF4_SPELL_START + 27:  return spell_RF4_BREATH(target_ptr, GF_MANA, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_MANA */
	case RF4_SPELL_START + 28:  return spell_RF4_BA_NUKE(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BA_NUKE */
	case RF4_SPELL_START + 29:  return spell_RF4_BREATH(target_ptr, GF_NUKE, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_NUKE */
	case RF4_SPELL_START + 30:  return spell_RF4_BA_CHAO(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BA_CHAO */
	case RF4_SPELL_START + 31:  return spell_RF4_BREATH(target_ptr, GF_DISINTEGRATE, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_DISI */
	case RF5_SPELL_START + 0:  return spell_RF5_BA_ACID(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BA_ACID */
	case RF5_SPELL_START + 1:  return spell_RF5_BA_ELEC(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BA_ELEC */
	case RF5_SPELL_START + 2:  return spell_RF5_BA_FIRE(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BA_FIRE */
	case RF5_SPELL_START + 3:  return spell_RF5_BA_COLD(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BA_COLD */
	case RF5_SPELL_START + 4:  return spell_RF5_BA_POIS(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BA_POIS */
	case RF5_SPELL_START + 5:  return spell_RF5_BA_NETH(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BA_NETH */
	case RF5_SPELL_START + 6:  return spell_RF5_BA_WATE(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BA_WATE */
	case RF5_SPELL_START + 7:  return spell_RF5_BA_MANA(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BA_MANA */
	case RF5_SPELL_START + 8:  return spell_RF5_BA_DARK(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BA_DARK */
	case RF5_SPELL_START + 9:  return spell_RF5_DRAIN_MANA(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF5_DRAIN_MANA */
	case RF5_SPELL_START + 10: return spell_RF5_MIND_BLAST(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF5_MIND_BLAST */
	case RF5_SPELL_START + 11: return spell_RF5_BRAIN_SMASH(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BRAIN_SMASH */
	case RF5_SPELL_START + 12: return spell_RF5_CAUSE_1(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_CAUSE_1 */
	case RF5_SPELL_START + 13: return spell_RF5_CAUSE_2(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_CAUSE_2 */
	case RF5_SPELL_START + 14: return spell_RF5_CAUSE_3(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_CAUSE_3 */
	case RF5_SPELL_START + 15: return spell_RF5_CAUSE_4(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_CAUSE_4 */
	case RF5_SPELL_START + 16: return spell_RF5_BO_ACID(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BO_ACID */
	case RF5_SPELL_START + 17: return spell_RF5_BO_ELEC(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BO_ELEC */
	case RF5_SPELL_START + 18: return spell_RF5_BO_FIRE(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BO_FIRE */
	case RF5_SPELL_START + 19: return spell_RF5_BO_COLD(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BO_COLD */
	case RF5_SPELL_START + 20: return spell_RF5_BA_LITE(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BA_LITE */
	case RF5_SPELL_START + 21: return spell_RF5_BO_NETH(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BO_NETH */
	case RF5_SPELL_START + 22: return spell_RF5_BO_WATE(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BO_WATE */
	case RF5_SPELL_START + 23: return spell_RF5_BO_MANA(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BO_MANA */
	case RF5_SPELL_START + 24: return spell_RF5_BO_PLAS(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BO_PLAS */
	case RF5_SPELL_START + 25: return spell_RF5_BO_ICEE(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BO_ICEE */
	case RF5_SPELL_START + 26: return spell_RF5_MISSILE(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_MISSILE */
	case RF5_SPELL_START + 27: spell_RF5_SCARE(m_idx, target_ptr, t_idx, MONSTER_TO_MONSTER); break;  /* RF5_SCARE */
	case RF5_SPELL_START + 28: spell_RF5_BLIND(m_idx, target_ptr, t_idx, MONSTER_TO_MONSTER); break;  /* RF5_BLIND */
	case RF5_SPELL_START + 29: spell_RF5_CONF(m_idx, target_ptr, t_idx, MONSTER_TO_MONSTER); break;   /* RF5_CONF */
	case RF5_SPELL_START + 30: spell_RF5_SLOW(m_idx, target_ptr, t_idx, MONSTER_TO_MONSTER); break;   /* RF5_SLOW */
	case RF5_SPELL_START + 31: spell_RF5_HOLD(m_idx, target_ptr, t_idx, MONSTER_TO_MONSTER); break;  /* RF5_HOLD */
	case RF6_SPELL_START + 0:  spell_RF6_HASTE(target_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); break;   /* RF6_HASTE */
	case RF6_SPELL_START + 1:  return spell_RF6_HAND_DOOM(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_HAND_DOOM */
	case RF6_SPELL_START + 2:  spell_RF6_HEAL(target_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); break;	/* RF6_HEAL */
	case RF6_SPELL_START + 3:  spell_RF6_INVULNER(target_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); break;	/* RF6_INVULNER */
	case RF6_SPELL_START + 4:  spell_RF6_BLINK(target_ptr, m_idx, MONSTER_TO_MONSTER, is_special_spell); break;   /* RF6_BLINK */
	case RF6_SPELL_START + 5:  spell_RF6_TPORT(target_ptr, m_idx, MONSTER_TO_MONSTER); break;   /* RF6_TPORT */
	case RF6_SPELL_START + 6:  return -1; break;	/* RF6_WORLD */
	case RF6_SPELL_START + 7:  return spell_RF6_SPECIAL(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);   /* RF6_SPECIAL */
	case RF6_SPELL_START + 8:  spell_RF6_TELE_TO(target_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF6_TELE_TO */
	case RF6_SPELL_START + 9:  spell_RF6_TELE_AWAY(target_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); break;   /* RF6_TELE_AWAY */
	case RF6_SPELL_START + 10: spell_RF6_TELE_LEVEL(target_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); break;  /* RF6_TELE_LEVEL */
	case RF6_SPELL_START + 11: return spell_RF6_PSY_SPEAR(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF6_PSY_SPEAR */
	case RF6_SPELL_START + 12: spell_RF6_DARKNESS(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;	/* RF6_DARKNESS */
	case RF6_SPELL_START + 13: return -1; /* RF6_TRAPS */
	case RF6_SPELL_START + 14: return -1;  /* RF6_FORGET */
	case RF6_SPELL_START + 15: spell_RF6_RAISE_DEAD(target_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); break;  /* RF6_RAISE_DEAD */
	case RF6_SPELL_START + 16: spell_RF6_S_KIN(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF6_S_KIN */
	case RF6_SPELL_START + 17: spell_RF6_S_CYBER(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;   /* RF6_S_CYBER */
	case RF6_SPELL_START + 18: spell_RF6_S_MONSTER(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF6_S_MONSTER */
	case RF6_SPELL_START + 19: spell_RF6_S_MONSTERS(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;	/* RF6_S_MONSTER */
	case RF6_SPELL_START + 20: spell_RF6_S_ANT(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF6_S_ANT */
	case RF6_SPELL_START + 21: spell_RF6_S_SPIDER(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;  /* RF6_S_SPIDER */
	case RF6_SPELL_START + 22: spell_RF6_S_HOUND(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;   /* RF6_S_HOUND */
	case RF6_SPELL_START + 23: spell_RF6_S_HYDRA(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;   /* RF6_S_HYDRA */
	case RF6_SPELL_START + 24: spell_RF6_S_ANGEL(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;   /* RF6_S_ANGEL */
	case RF6_SPELL_START + 25: spell_RF6_S_DEMON(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;   /* RF6_S_DEMON */
	case RF6_SPELL_START + 26: spell_RF6_S_UNDEAD(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;  /* RF6_S_UNDEAD */
	case RF6_SPELL_START + 27: spell_RF6_S_DRAGON(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;  /* RF6_S_DRAGON */
	case RF6_SPELL_START + 28: spell_RF6_S_HI_UNDEAD(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;   /* RF6_S_HI_UNDEAD */
	case RF6_SPELL_START + 29: spell_RF6_S_HI_DRAGON(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;   /* RF6_S_HI_DRAGON */
	case RF6_SPELL_START + 30: spell_RF6_S_AMBERITES(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;   /* RF6_S_AMBERITES */
	case RF6_SPELL_START + 31: spell_RF6_S_UNIQUE(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;  /* RF6_S_UNIQUE */
	}

	return 0;
}
