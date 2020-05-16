/*!
 * @file mspells4.c
 * @brief スペル実行処理 / Spell launch
 * @date 2014/07/14
 * @author Habu
 */

#include "system/angband.h"
#include "util/util.h"
#include "main/sound-definitions-table.h"

#include "effect/effect-characteristics.h"
#include "grid/grid.h"
#include "object/artifact.h"
#include "player/player-status.h"
#include "player/player-class.h"
#include "player/player-personality.h"
#include "player/player-damage.h"
#include "realm/realm-hex.h"
#include "player/player-move.h"
#include "spell/monster-spell.h"
#include "monster/monster-status.h"
#include "spell/spells-type.h"
#include "combat/melee.h"
#include "player/player-effects.h"
#include "spell/process-effect.h"
#include "spell/spells2.h"
#include "spell/spells3.h"
#include "spell/mspell-summon.h"
#include "spell/mspell-util.h"
#include "spell/mspell-curse.h"
#include "spell/mspell-type.h"
#include "spell/mspell-damage-calculator.h"
#include "spell/mspell-breath.h"
#include "spell/mspell-ball.h"
#include "spell/mspell-bolt.h"

/*!
 * @brief RF4_SHRIEKの処理。叫び。 /
 * @param m_idx 呪文を唱えるモンスターID
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_RF4_SHRIEK(MONSTER_IDX m_idx, player_type *target_ptr, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	simple_monspell_message(target_ptr, m_idx, t_idx,
		_("%^sがかん高い金切り声をあげた。", "%^s makes a high pitched shriek."),
		_("%^sが%sに向かって叫んだ。", "%^s shrieks at %s."),
		TARGET_TYPE);

	if (TARGET_TYPE == MONSTER_TO_PLAYER)
	{
		aggravate_monsters(target_ptr, m_idx);
		return;
	}

	if (TARGET_TYPE == MONSTER_TO_MONSTER)
	{
		set_monster_csleep(target_ptr, t_idx, 0);
	}
}


/*!
 * @brief RF4_DISPELの処理。魔力消去。 /
 * @param m_idx 呪文を唱えるモンスターID
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_RF4_DISPEL(MONSTER_IDX m_idx, player_type *target_ptr, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN];
	monster_name(target_ptr, m_idx, m_name);
	monster_name(target_ptr, t_idx, t_name);

	monspell_message(target_ptr, m_idx, t_idx,
		_("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."),
		_("%^sが魔力消去の呪文を念じた。", "%^s invokes a dispel magic."),
		_("%^sが%sに対して魔力消去の呪文を念じた。", "%^s invokes a dispel magic at %s."),
		TARGET_TYPE);

	if (TARGET_TYPE == MONSTER_TO_PLAYER)
	{
		dispel_player(target_ptr);
		if (target_ptr->riding) dispel_monster_status(target_ptr, target_ptr->riding);

		if (IS_ECHIZEN(target_ptr))
			msg_print(_("やりやがったな！", ""));
		else if ((target_ptr->pseikaku == SEIKAKU_CHARGEMAN))
		{
			if (randint0(2) == 0) msg_print(_("ジュラル星人め！", ""));
			else msg_print(_("弱い者いじめは止めるんだ！", ""));
		}

		learn_spell(target_ptr, MS_DISPEL);
		return;
	}

	if (TARGET_TYPE == MONSTER_TO_MONSTER)
	{
		if (t_idx == target_ptr->riding) dispel_player(target_ptr);
		dispel_monster_status(target_ptr, t_idx);
	}
}


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
* @brief RF5_DRAIN_MANAの処理。魔力吸収。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param y 対象の地点のy座標
* @param x 対象の地点のx座標
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
* @return ダメージ量を返す。
*/
HIT_POINT spell_RF5_DRAIN_MANA(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	HIT_POINT dam;
	GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN];
	monster_name(target_ptr, m_idx, m_name);
	monster_name(target_ptr, t_idx, t_name);


	if (TARGET_TYPE == MONSTER_TO_PLAYER)
	{
		disturb(target_ptr, TRUE, TRUE);
	}
	else if (TARGET_TYPE == MONSTER_TO_MONSTER && see_monster(target_ptr->current_floor_ptr, m_idx))
	{
		/* Basic message */
		msg_format(_("%^sは精神エネルギーを%sから吸いとった。", "%^s draws psychic energy from %s."), m_name, t_name);
	}

	dam = monspell_damage(target_ptr, (MS_DRAIN_MANA), m_idx, DAM_ROLL);
	breath(target_ptr, y, x, m_idx, GF_DRAIN_MANA, dam, 0, FALSE, MS_DRAIN_MANA, TARGET_TYPE);
	if (TARGET_TYPE == MONSTER_TO_PLAYER)
		update_smart_learn(target_ptr, m_idx, DRS_MANA);

	return dam;
}


/*!
* @brief RF5_MIND_BLASTの処理。精神攻撃。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param y 対象の地点のy座標
* @param x 対象の地点のx座標
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
* @return ダメージ量を返す。
*/
HIT_POINT spell_RF5_MIND_BLAST(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	bool seen = (!target_ptr->blind && m_ptr->ml);
	HIT_POINT dam;
	GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN];
	monster_name(target_ptr, m_idx, m_name);
	monster_name(target_ptr, t_idx, t_name);


	if (TARGET_TYPE == MONSTER_TO_PLAYER)
	{
		disturb(target_ptr, TRUE, TRUE);
		if (!seen)
			msg_print(_("何かがあなたの精神に念を放っているようだ。", "You feel something focusing on your mind."));
		else
			msg_format(_("%^sがあなたの瞳をじっとにらんでいる。", "%^s gazes deep into your eyes."), m_name);
	}
	else if (TARGET_TYPE == MONSTER_TO_MONSTER && see_monster(floor_ptr, m_idx))
	{
		msg_format(_("%^sは%sをじっと睨んだ。", "%^s gazes intently at %s."), m_name, t_name);
	}

	dam = monspell_damage(target_ptr, (MS_MIND_BLAST), m_idx, DAM_ROLL);
	breath(target_ptr, y, x, m_idx, GF_MIND_BLAST, dam, 0, FALSE, MS_MIND_BLAST, TARGET_TYPE);
	return dam;
}


/*!
* @brief RF5_BRAIN_SMASHの処理。脳攻撃。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param y 対象の地点のy座標
* @param x 対象の地点のx座標
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
* @return ダメージ量を返す。
*/
HIT_POINT spell_RF5_BRAIN_SMASH(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	bool seen = (!target_ptr->blind && m_ptr->ml);
	HIT_POINT dam;
	GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN];
	monster_name(target_ptr, m_idx, m_name);
	monster_name(target_ptr, t_idx, t_name);


	if (TARGET_TYPE == MONSTER_TO_PLAYER)
	{
		disturb(target_ptr, TRUE, TRUE);
		if (!seen)
			msg_print(_("何かがあなたの精神に念を放っているようだ。", "You feel something focusing on your mind."));
		else
			msg_format(_("%^sがあなたの瞳をじっとにらんでいる。", "%^s gazes deep into your eyes."), m_name);
	}
	else if (TARGET_TYPE == MONSTER_TO_MONSTER && see_monster(floor_ptr, m_idx))
	{
		msg_format(_("%^sは%sをじっと睨んだ。", "%^s gazes intently at %s."), m_name, t_name);
	}

	dam = monspell_damage(target_ptr, (MS_BRAIN_SMASH), m_idx, DAM_ROLL);
	breath(target_ptr, y, x, m_idx, GF_BRAIN_SMASH, dam, 0, FALSE, MS_BRAIN_SMASH, TARGET_TYPE);
	return dam;
}

/*!
* @brief 状態異常呪文のメッセージ処理関数。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param msg1 対プレイヤーなら盲目時メッセージ。対モンスターなら通常時メッセージ。
* @param msg2 対プレイヤーなら非盲目時メッセージ。対モンスターなら耐性有メッセージ。
* @param msg3 対プレイヤーなら耐性有メッセージ。対モンスターなら抵抗時メッセージ。
* @param msg4 対プレイヤーなら抵抗時メッセージ。対モンスターなら成功時メッセージ。
* @param resist 耐性の有無を判別するフラグ
* @param saving_throw 抵抗に成功したか判別するフラグ
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
*/
void spell_badstatus_message(player_type *target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, concptr msg1, concptr msg2, concptr msg3, concptr msg4, bool resist, bool saving_throw, int TARGET_TYPE)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	bool see_either = see_monster(floor_ptr, m_idx) || see_monster(floor_ptr, t_idx);
	bool see_t = see_monster(floor_ptr, t_idx);
	bool known = monster_near_player(floor_ptr, m_idx, t_idx);
	GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN];
	monster_name(target_ptr, m_idx, m_name);
	monster_name(target_ptr, t_idx, t_name);

	if (TARGET_TYPE == MONSTER_TO_PLAYER)
	{
		disturb(target_ptr, TRUE, TRUE);
		if (target_ptr->blind)
			msg_format(msg1, m_name);
		else
			msg_format(msg2, m_name);

		if (resist)
		{
			msg_print(msg3);
		}
		else if (saving_throw)
		{
			msg_print(msg4);
		}

		return;
	}

	if (TARGET_TYPE != MONSTER_TO_MONSTER) return;

	if (known)
	{
		if (see_either)
		{
			msg_format(msg1, m_name, t_name);
		}
		else
		{
			floor_ptr->monster_noise = TRUE;
		}
	}

	if (resist)
	{
		if (see_t) msg_format(msg2, t_name);
	}
	else if (saving_throw)
	{
		if (see_t) msg_format(msg3, t_name);
	}
	else
	{
		if (see_t) msg_format(msg4, t_name);
	}

	set_monster_csleep(target_ptr, t_idx, 0);
}


/*!
 * @brief RF5_SCAREの処理。恐怖。 /
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_RF5_SCARE(MONSTER_IDX m_idx, player_type *target_ptr, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *t_ptr = &floor_ptr->m_list[t_idx];
	monster_race	*tr_ptr = &r_info[t_ptr->r_idx];
	DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
	bool resist, saving_throw;

	if (TARGET_TYPE == MONSTER_TO_PLAYER)
	{
		resist = target_ptr->resist_fear;
		saving_throw = (randint0(100 + rlev / 2) < target_ptr->skill_sav);
		spell_badstatus_message(target_ptr, m_idx, t_idx,
			_("%^sが何かをつぶやくと、恐ろしげな音が聞こえた。", "%^s mumbles, and you hear scary noises."),
			_("%^sが恐ろしげな幻覚を作り出した。", "%^s casts a fearful illusion."),
			_("しかし恐怖に侵されなかった。", "You refuse to be frightened."),
			_("しかし恐怖に侵されなかった。", "You refuse to be frightened."),
			resist, saving_throw, TARGET_TYPE);

		if (!resist && !saving_throw)
		{
			(void)set_afraid(target_ptr, target_ptr->afraid + randint0(4) + 4);
		}

		learn_spell(target_ptr, MS_SCARE);
		update_smart_learn(target_ptr, m_idx, DRS_FEAR);
		return;
	}

	if (TARGET_TYPE != MONSTER_TO_MONSTER) return;

	resist = tr_ptr->flags3 & RF3_NO_FEAR;
	saving_throw = (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

	spell_badstatus_message(target_ptr, m_idx, t_idx,
		_("%^sが恐ろしげな幻覚を作り出した。", "%^s casts a fearful illusion in front of %s."),
		_("%^sは恐怖を感じない。", "%^s refuses to be frightened."),
		_("%^sは恐怖を感じない。", "%^s refuses to be frightened."),
		_("%^sは恐怖して逃げ出した！", "%^s flees in terror!"),
		resist, saving_throw, TARGET_TYPE);

	if (!resist && !saving_throw)
	{
		set_monster_monfear(target_ptr, t_idx, MON_MONFEAR(t_ptr) + randint0(4) + 4);
	}
}


/*!
 * @brief RF5_BLINDの処理。盲目。 /
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_RF5_BLIND(MONSTER_IDX m_idx, player_type *target_ptr, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *t_ptr = &floor_ptr->m_list[t_idx];
	monster_race *tr_ptr = &r_info[t_ptr->r_idx];
	DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
	bool resist, saving_throw;

	if (TARGET_TYPE == MONSTER_TO_PLAYER)
	{
		resist = target_ptr->resist_blind;
		saving_throw = (randint0(100 + rlev / 2) < target_ptr->skill_sav);
		spell_badstatus_message(target_ptr, m_idx, t_idx,
			_("%^sが何かをつぶやいた。", "%^s mumbles."),
			_("%^sが呪文を唱えてあなたの目をくらました！", "%^s casts a spell, burning your eyes!"),
			_("しかし効果がなかった！", "You are unaffected!"),
			_("しかし効力を跳ね返した！", "You resist the effects!"),
			resist, saving_throw, TARGET_TYPE);

		if (!resist && !saving_throw)
		{
			(void)set_blind(target_ptr, 12 + randint0(4));
		}

		learn_spell(target_ptr, MS_BLIND);
		update_smart_learn(target_ptr, m_idx, DRS_BLIND);
		return;
	}

	if (TARGET_TYPE != MONSTER_TO_MONSTER) return;

	concptr msg1;
	GAME_TEXT t_name[MAX_NLEN];
	monster_name(target_ptr, t_idx, t_name);

	if (streq(t_name, "it"))
	{
		msg1 = _("%sは呪文を唱えて%sの目を焼き付かせた。", "%^s casts a spell, burning %ss eyes.");
	}
	else
	{
		msg1 = _("%sは呪文を唱えて%sの目を焼き付かせた。", "%^s casts a spell, burning %s's eyes.");
	}

	resist = tr_ptr->flags3 & RF3_NO_CONF;
	saving_throw = (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

	spell_badstatus_message(target_ptr, m_idx, t_idx,
		msg1,
		_("%^sには効果がなかった。", "%^s is unaffected."),
		_("%^sには効果がなかった。", "%^s is unaffected."),
		_("%^sは目が見えなくなった！ ", "%^s is blinded!"),
		resist, saving_throw, TARGET_TYPE);

	if (!resist && !saving_throw)
	{
		(void)set_monster_confused(target_ptr, t_idx, MON_CONFUSED(t_ptr) + 12 + randint0(4));
	}
}


/*!
 * @brief RF5_CONFの処理。混乱。/
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_RF5_CONF(MONSTER_IDX m_idx, player_type *target_ptr, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *t_ptr = &floor_ptr->m_list[t_idx];
	monster_race	*tr_ptr = &r_info[t_ptr->r_idx];
	DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
	bool resist, saving_throw;

	if (TARGET_TYPE == MONSTER_TO_PLAYER)
	{
		resist = target_ptr->resist_conf;
		saving_throw = (randint0(100 + rlev / 2) < target_ptr->skill_sav);
		spell_badstatus_message(target_ptr, m_idx, t_idx,
			_("%^sが何かをつぶやくと、頭を悩ます音がした。", "%^s mumbles, and you hear puzzling noises."),
			_("%^sが誘惑的な幻覚を作り出した。", "%^s creates a mesmerising illusion."),
			_("しかし幻覚にはだまされなかった。", "You disbelieve the feeble spell."),
			_("しかし幻覚にはだまされなかった。", "You disbelieve the feeble spell."),
			resist, saving_throw, TARGET_TYPE);

		if (!resist && !saving_throw)
		{
			(void)set_confused(target_ptr, target_ptr->confused + randint0(4) + 4);
		}

		learn_spell(target_ptr, MS_CONF);
		update_smart_learn(target_ptr, m_idx, DRS_CONF);
		return;
	}

	if (TARGET_TYPE != MONSTER_TO_MONSTER) return;

	resist = tr_ptr->flags3 & RF3_NO_CONF;
	saving_throw = (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

	spell_badstatus_message(target_ptr, m_idx, t_idx,
		_("%^sが%sの前に幻惑的な幻をつくり出した。", "%^s casts a mesmerizing illusion in front of %s."),
		_("%^sは惑わされなかった。", "%^s disbelieves the feeble spell."),
		_("%^sは惑わされなかった。", "%^s disbelieves the feeble spell."),
		_("%^sは混乱したようだ。", "%^s seems confused."),
		resist, saving_throw, TARGET_TYPE);

	if (!resist && !saving_throw)
	{
		(void)set_monster_confused(target_ptr, t_idx, MON_CONFUSED(t_ptr) + 12 + randint0(4));
	}
}


/*!
 * @brief RF5_SLOWの処理。減速。 /
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_RF5_SLOW(MONSTER_IDX m_idx, player_type *target_ptr, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *t_ptr = &floor_ptr->m_list[t_idx];
	monster_race	*tr_ptr = &r_info[t_ptr->r_idx];
	DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
	bool resist, saving_throw;

	if (TARGET_TYPE == MONSTER_TO_PLAYER)
	{
		resist = target_ptr->resist_conf;
		saving_throw = (randint0(100 + rlev / 2) < target_ptr->skill_sav);
		spell_badstatus_message(target_ptr, m_idx, t_idx,
			_("%^sがあなたの筋力を吸い取ろうとした！", "%^s drains power from your muscles!"),
			_("%^sがあなたの筋力を吸い取ろうとした！", "%^s drains power from your muscles!"),
			_("しかし効果がなかった！", "You are unaffected!"),
			_("しかし効力を跳ね返した！", "You resist the effects!"),
			resist, saving_throw, TARGET_TYPE);

		if (!resist && !saving_throw)
		{
			(void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, FALSE);
		}

		learn_spell(target_ptr, MS_SLOW);
		update_smart_learn(target_ptr, m_idx, DRS_FREE);
		return;
	}

	if (TARGET_TYPE != MONSTER_TO_MONSTER) return;

	concptr msg1;
	GAME_TEXT t_name[MAX_NLEN];
	monster_name(target_ptr, t_idx, t_name);

	if (streq(t_name, "it"))
	{
		msg1 = _("%sが%sの筋肉から力を吸いとった。", "%^s drains power from %ss muscles.");
	}
	else
	{
		msg1 = _("%sが%sの筋肉から力を吸いとった。", "%^s drains power from %s's muscles.");
	}

	resist = tr_ptr->flags1 & RF1_UNIQUE;
	saving_throw = (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

	spell_badstatus_message(target_ptr, m_idx, t_idx,
		msg1,
		_("%^sには効果がなかった。", "%^s is unaffected."),
		_("%^sには効果がなかった。", "%^s is unaffected."),
		_("%sの動きが遅くなった。", "%^s starts moving slower."),
		resist, saving_throw, TARGET_TYPE);

	if (!resist && !saving_throw)
	{
		set_monster_slow(target_ptr, t_idx, MON_SLOW(t_ptr) + 50);
	}
}


/*!
 * @brief RF5_HOLDの処理。麻痺。 /
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_RF5_HOLD(MONSTER_IDX m_idx, player_type *target_ptr, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *t_ptr = &floor_ptr->m_list[t_idx];
	monster_race	*tr_ptr = &r_info[t_ptr->r_idx];
	DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
	bool resist, saving_throw;

	if (TARGET_TYPE == MONSTER_TO_PLAYER)
	{
		resist = target_ptr->free_act;
		saving_throw = (randint0(100 + rlev / 2) < target_ptr->skill_sav);
		spell_badstatus_message(target_ptr, m_idx, t_idx,
			_("%^sが何かをつぶやいた。", "%^s mumbles."),
			_("%^sがあなたの目をじっと見つめた！", "%^s stares deep into your eyes!"),
			_("しかし効果がなかった！", "You are unaffected!"),
			_("しかし効力を跳ね返した！", "You resist the effects!"),
			resist, saving_throw, TARGET_TYPE);

		if (!resist && !saving_throw)
		{
			(void)set_paralyzed(target_ptr, target_ptr->paralyzed + randint0(4) + 4);
		}

		learn_spell(target_ptr, MS_SLEEP);
		update_smart_learn(target_ptr, m_idx, DRS_FREE);
		return;
	}

	if (TARGET_TYPE != MONSTER_TO_MONSTER) return;

	resist = (tr_ptr->flags1 & RF1_UNIQUE) || (tr_ptr->flags3 & RF3_NO_STUN);
	saving_throw = (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

	spell_badstatus_message(target_ptr, m_idx, t_idx,
		_("%^sは%sをじっと見つめた。", "%^s stares intently at %s."),
		_("%^sには効果がなかった。", "%^s is unaffected."),
		_("%^sには効果がなかった。", "%^s is unaffected."),
		_("%^sは麻痺した！", "%^s is paralyzed!"),
		resist, saving_throw, TARGET_TYPE);

	if (!resist && !saving_throw)
	{
		(void)set_monster_stunned(target_ptr, t_idx, MON_STUNNED(t_ptr) + randint1(4) + 4);
	}
}


/*!
* @brief RF6_HASTEの処理。加速。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
*/
void spell_RF6_HASTE(player_type *target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	bool see_m = see_monster(floor_ptr, m_idx);
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	GAME_TEXT m_name[MAX_NLEN];
	monster_name(target_ptr, m_idx, m_name);
	char m_poss[10];
	monster_desc(target_ptr, m_poss, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);

	monspell_message_base(target_ptr, m_idx, t_idx,
		_("%^sが何かをつぶやいた。", "%^s mumbles."),
		_("%^sが自分の体に念を送った。", format("%%^s concentrates on %s body.", m_poss)),
		_("%^sが自分の体に念を送った。", format("%%^s concentrates on %s body.", m_poss)),
		_("%^sが自分の体に念を送った。", format("%%^s concentrates on %s body.", m_poss)),
		target_ptr->blind > 0, TARGET_TYPE);

	if (set_monster_fast(target_ptr, m_idx, MON_FAST(m_ptr) + 100))
	{
		if (TARGET_TYPE == MONSTER_TO_PLAYER ||
			(TARGET_TYPE == MONSTER_TO_MONSTER && see_m))
			msg_format(_("%^sの動きが速くなった。", "%^s starts moving faster."), m_name);
	}
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
* @brief RF6_HEALの処理。治癒。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
*/
void spell_RF6_HEAL(player_type *target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
	bool seen = (!target_ptr->blind && m_ptr->ml);
	GAME_TEXT m_name[MAX_NLEN];
	monster_name(target_ptr, m_idx, m_name);
	char m_poss[10];
	monster_desc(target_ptr, m_poss, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);

	disturb(target_ptr, TRUE, TRUE);

	monspell_message_base(target_ptr, m_idx, t_idx,
		_("%^sが何かをつぶやいた。", "%^s mumbles."),
		_("%^sは自分の傷に念を集中した。", format("%%^s concentrates on %s wounds.", m_poss)),
		_("%^sが自分の傷に集中した。", format("%%^s concentrates on %s wounds.", m_poss)),
		_("%^sは自分の傷に念を集中した。", format("%%^s concentrates on %s wounds.", m_poss)),
		target_ptr->blind > 0, TARGET_TYPE);

	m_ptr->hp += (rlev * 6);
	if (m_ptr->hp >= m_ptr->maxhp)
	{
		/* Fully healed */
		m_ptr->hp = m_ptr->maxhp;

		monspell_message_base(target_ptr, m_idx, t_idx,
			_("%^sは完全に治ったようだ！", "%^s sounds completely healed!"),
			_("%^sは完全に治ったようだ！", "%^s sounds completely healed!"),
			_("%^sは完全に治った！", "%^s looks completely healed!"),
			_("%^sは完全に治った！", "%^s looks completely healed!"),
			!seen, TARGET_TYPE);
	}
	else
	{
		monspell_message_base(target_ptr, m_idx, t_idx,
			_("%^sは体力を回復したようだ。", "%^s sounds healthier."),
			_("%^sは体力を回復したようだ。", "%^s sounds healthier."),
			_("%^sは体力を回復したようだ。", "%^s looks healthier."),
			_("%^sは体力を回復したようだ。", "%^s looks healthier."),
			!seen, TARGET_TYPE);
	}

	if (target_ptr->health_who == m_idx) target_ptr->redraw |= (PR_HEALTH);
	if (target_ptr->riding == m_idx) target_ptr->redraw |= (PR_UHEALTH);

	if (!MON_MONFEAR(m_ptr)) return;

	(void)set_monster_monfear(target_ptr, m_idx, 0);

	if (see_monster(floor_ptr, m_idx))
		msg_format(_("%^sは勇気を取り戻した。", format("%%^s recovers %s courage.", m_poss)), m_name);
}


/*!
* @brief RF6_INVULNERの処理。無敵。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
*/
void spell_RF6_INVULNER(player_type *target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
	bool seen = (!target_ptr->blind && m_ptr->ml);

	monspell_message_base(target_ptr, m_idx, t_idx,
		_("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."),
		_("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."),
		_("%sは無傷の球の呪文を唱えた。", "%^s casts a Globe of Invulnerability."),
		_("%sは無傷の球の呪文を唱えた。", "%^s casts a Globe of Invulnerability."),
		!seen, TARGET_TYPE);

	if (!MON_INVULNER(m_ptr)) (void)set_monster_invulner(target_ptr, m_idx, randint1(4) + 4, FALSE);
}


/*!
* @brief RF6_BLINKの処理。ショート・テレポート。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param m_idx 呪文を唱えるモンスターID
* @param is_quantum_effect 量子的効果によるショート・テレポートの場合時TRUE
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
*/
void spell_RF6_BLINK(player_type *target_ptr, MONSTER_IDX m_idx, int TARGET_TYPE, bool is_quantum_effect)
{
	GAME_TEXT m_name[MAX_NLEN];
	monster_name(target_ptr, m_idx, m_name);

	if (TARGET_TYPE == MONSTER_TO_PLAYER)
		disturb(target_ptr, TRUE, TRUE);

	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	if (!is_quantum_effect && teleport_barrier(target_ptr, m_idx))
	{
		if (see_monster(floor_ptr, m_idx))
			msg_format(_("魔法のバリアが%^sのテレポートを邪魔した。",
				"Magic barrier obstructs teleporting of %^s."), m_name);
		return;
	}

	if (see_monster(floor_ptr, m_idx))
		msg_format(_("%^sが瞬時に消えた。", "%^s blinks away."), m_name);

	teleport_away(target_ptr, m_idx, 10, TELEPORT_SPONTANEOUS);

	if (TARGET_TYPE == MONSTER_TO_PLAYER)
		target_ptr->update |= (PU_MONSTERS);
}


/*!
* @brief RF6_TPORTの処理。テレポート。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param m_idx 呪文を唱えるモンスターID
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
*/
void spell_RF6_TPORT(player_type *target_ptr, MONSTER_IDX m_idx, int TARGET_TYPE)
{
	GAME_TEXT m_name[MAX_NLEN];
	monster_name(target_ptr, m_idx, m_name);

	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	if (TARGET_TYPE == MONSTER_TO_PLAYER)
		disturb(target_ptr, TRUE, TRUE);
	if (teleport_barrier(target_ptr, m_idx))
	{
		if (see_monster(floor_ptr, m_idx))
			msg_format(_("魔法のバリアが%^sのテレポートを邪魔した。",
				"Magic barrier obstructs teleporting of %^s."), m_name);
		return;
	}

	if (see_monster(floor_ptr, m_idx))
		msg_format(_("%^sがテレポートした。", "%^s teleports away."), m_name);

	teleport_away_followable(target_ptr, m_idx);
}


/*!
* @brief RF6_WORLDの処理。時を止める。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param m_idx 呪文を唱えるモンスターID
*/
HIT_POINT spell_RF6_WORLD(player_type *target_ptr, MONSTER_IDX m_idx)
{
	monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
	MONSTER_IDX who = 0;
	GAME_TEXT m_name[MAX_NLEN];
	monster_name(target_ptr, m_idx, m_name);

	disturb(target_ptr, TRUE, TRUE);
	if (m_ptr->r_idx == MON_DIO) who = 1;
	else if (m_ptr->r_idx == MON_WONG) who = 3;
	if (!set_monster_timewalk(target_ptr, randint1(2) + 2, who, TRUE)) return FALSE;
	return who;
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
* @brief RF6_TELE_TOの処理。テレポート・バック。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
* @return ダメージ量を返す。
*/
void spell_RF6_TELE_TO(player_type *target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	monster_type *t_ptr = &floor_ptr->m_list[t_idx];
	monster_race *tr_ptr = &r_info[t_ptr->r_idx];

	simple_monspell_message(target_ptr, m_idx, t_idx,
		_("%^sがあなたを引き戻した。", "%^s commands you to return."),
		_("%^sが%sを引き戻した。", "%^s commands %s to return."),
		TARGET_TYPE);

	if (TARGET_TYPE == MONSTER_TO_PLAYER)
	{
		teleport_player_to(target_ptr, m_ptr->fy, m_ptr->fx, TELEPORT_PASSIVE);
		learn_spell(target_ptr, MS_TELE_TO);
		return;
	}

	if (TARGET_TYPE != MONSTER_TO_MONSTER) return;

	bool resists_tele = FALSE;
	GAME_TEXT t_name[MAX_NLEN];
	monster_name(target_ptr, t_idx, t_name);

	if (tr_ptr->flagsr & RFR_RES_TELE)
	{
		if ((tr_ptr->flags1 & RF1_UNIQUE) || (tr_ptr->flagsr & RFR_RES_ALL))
		{
			if (is_original_ap_and_seen(target_ptr, t_ptr)) tr_ptr->r_flagsr |= RFR_RES_TELE;
			if (see_monster(floor_ptr, t_idx))
			{
				msg_format(_("%^sには効果がなかった。", "%^s is unaffected!"), t_name);
			}
			resists_tele = TRUE;
		}
		else if (tr_ptr->level > randint1(100))
		{
			if (is_original_ap_and_seen(target_ptr, t_ptr)) tr_ptr->r_flagsr |= RFR_RES_TELE;
			if (see_monster(floor_ptr, t_idx))
			{
				msg_format(_("%^sは耐性を持っている！", "%^s resists!"), t_name);
			}
			resists_tele = TRUE;
		}
	}

	if (resists_tele)
	{
		set_monster_csleep(target_ptr, t_idx, 0);
		return;
	}

	if (t_idx == target_ptr->riding)
		teleport_player_to(target_ptr, m_ptr->fy, m_ptr->fx, TELEPORT_PASSIVE);
	else
		teleport_monster_to(target_ptr, t_idx, m_ptr->fy, m_ptr->fx, 100, TELEPORT_PASSIVE);
	set_monster_csleep(target_ptr, t_idx, 0);
}


/*!
* @brief RF6_TELE_AWAYの処理。テレポート・アウェイ。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
* @return ダメージ量を返す。
*/
void spell_RF6_TELE_AWAY(player_type *target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *t_ptr = &floor_ptr->m_list[t_idx];
	monster_race *tr_ptr = &r_info[t_ptr->r_idx];

	simple_monspell_message(target_ptr, m_idx, t_idx,
		_("%^sにテレポートさせられた。", "%^s teleports you away."),
		_("%^sは%sをテレポートさせた。", "%^s teleports %s away."),
		TARGET_TYPE);

	if (TARGET_TYPE == MONSTER_TO_PLAYER)
	{
		if (IS_ECHIZEN(target_ptr))
			msg_print(_("くっそ～", ""));
		else if ((target_ptr->pseikaku == SEIKAKU_CHARGEMAN))
		{
			if (randint0(2) == 0) msg_print(_("ジュラル星人め！", ""));
			else msg_print(_("弱い者いじめは止めるんだ！", ""));
		}

		learn_spell(target_ptr, MS_TELE_AWAY);
		teleport_player_away(m_idx, target_ptr, 100, FALSE);
		return;
	}

	if (TARGET_TYPE != MONSTER_TO_MONSTER) return;

	bool resists_tele = FALSE;
	GAME_TEXT t_name[MAX_NLEN];
	monster_name(target_ptr, t_idx, t_name);

	if (tr_ptr->flagsr & RFR_RES_TELE)
	{
		if ((tr_ptr->flags1 & RF1_UNIQUE) || (tr_ptr->flagsr & RFR_RES_ALL))
		{
			if (is_original_ap_and_seen(target_ptr, t_ptr)) tr_ptr->r_flagsr |= RFR_RES_TELE;
			if (see_monster(floor_ptr, t_idx))
			{
				msg_format(_("%^sには効果がなかった。", "%^s is unaffected!"), t_name);
			}
			resists_tele = TRUE;
		}
		else if (tr_ptr->level > randint1(100))
		{
			if (is_original_ap_and_seen(target_ptr, t_ptr)) tr_ptr->r_flagsr |= RFR_RES_TELE;
			if (see_monster(floor_ptr, t_idx))
			{
				msg_format(_("%^sは耐性を持っている！", "%^s resists!"), t_name);
			}
			resists_tele = TRUE;
		}
	}

	if (resists_tele)
	{
		set_monster_csleep(target_ptr, t_idx, 0);
		return;
	}

	if (t_idx == target_ptr->riding)
		teleport_player_away(m_idx, target_ptr, MAX_SIGHT * 2 + 5, FALSE);
	else
		teleport_away(target_ptr, t_idx, MAX_SIGHT * 2 + 5, TELEPORT_PASSIVE);
	set_monster_csleep(target_ptr, t_idx, 0);
}


/*!
* @brief RF6_TELE_LEVELの処理。テレポート・レベル。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
* @return ダメージ量を返す。
*/
void spell_RF6_TELE_LEVEL(player_type *target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *t_ptr = &floor_ptr->m_list[t_idx];
	monster_race *tr_ptr = &r_info[t_ptr->r_idx];
	DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
	bool resist, saving_throw;

	if (TARGET_TYPE == MONSTER_TO_PLAYER)
	{
		resist = target_ptr->resist_nexus;
		saving_throw = (randint0(100 + rlev / 2) < target_ptr->skill_sav);
		spell_badstatus_message(target_ptr, m_idx, t_idx,
			_("%^sが何か奇妙な言葉をつぶやいた。", "%^s mumbles strangely."),
			_("%^sがあなたの足を指さした。", "%^s gestures at your feet."),
			_("しかし効果がなかった！", "You are unaffected!"),
			_("しかし効力を跳ね返した！", "You resist the effects!"),
			resist, saving_throw, TARGET_TYPE);

		if (!resist && !saving_throw)
		{
			teleport_level(target_ptr, 0);
		}

		learn_spell(target_ptr, MS_TELE_LEVEL);
		update_smart_learn(target_ptr, m_idx, DRS_NEXUS);
		return;
	}

	if (TARGET_TYPE != MONSTER_TO_MONSTER) return;

	resist = tr_ptr->flagsr & (RFR_EFF_RES_NEXU_MASK | RFR_RES_TELE);
	saving_throw = (tr_ptr->flags1 & RF1_QUESTOR) ||
		(tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

	spell_badstatus_message(target_ptr, m_idx, t_idx,
		_("%^sが%sの足を指さした。", "%^s gestures at %s's feet."),
		_("%^sには効果がなかった。", "%^s is unaffected!"),
		_("%^sは効力を跳ね返した！", "%^s resist the effects!"),
		"",
		resist, saving_throw, TARGET_TYPE);

	if (!resist && !saving_throw)
	{
		teleport_level(target_ptr, (t_idx == target_ptr->riding) ? 0 : t_idx);
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
* @brief RF6_DARKNESSの処理。暗闇or閃光。 /
* @param target_type プレーヤーへの参照ポインタ
* @param y 対象の地点のy座標
* @param x 対象の地点のx座標
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
*/
void spell_RF6_DARKNESS(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	monster_type *t_ptr = &floor_ptr->m_list[t_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	bool can_use_lite_area = FALSE;
	bool monster_to_monster = TARGET_TYPE == MONSTER_TO_MONSTER;
	bool monster_to_player = TARGET_TYPE == MONSTER_TO_PLAYER;
	GAME_TEXT t_name[MAX_NLEN];
	monster_name(target_ptr, t_idx, t_name);

	if ((target_ptr->pclass == CLASS_NINJA) &&
		!(r_ptr->flags3 & (RF3_UNDEAD | RF3_HURT_LITE)) &&
		!(r_ptr->flags7 & RF7_DARK_MASK))
		can_use_lite_area = TRUE;

	if (monster_to_monster && !is_hostile(t_ptr))
		can_use_lite_area = FALSE;


	if (can_use_lite_area)
	{
		monspell_message(target_ptr, m_idx, t_idx,
			_("%^sが何かをつぶやいた。", "%^s mumbles."),
			_("%^sが辺りを明るく照らした。", "%^s cast a spell to light up."),
			_("%^sが辺りを明るく照らした。", "%^s cast a spell to light up."),
			TARGET_TYPE);

		if (see_monster(floor_ptr, t_idx) && monster_to_monster)
		{
			msg_format(_("%^sは白い光に包まれた。", "%^s is surrounded by a white light."), t_name);
		}
	}
	else
	{
		monspell_message(target_ptr, m_idx, t_idx,
			_("%^sが何かをつぶやいた。", "%^s mumbles."),
			_("%^sが暗闇の中で手を振った。", "%^s gestures in shadow."),
			_("%^sが暗闇の中で手を振った。", "%^s gestures in shadow."),
			TARGET_TYPE);

		if (see_monster(floor_ptr, t_idx) && monster_to_monster)
		{
			msg_format(_("%^sは暗闇に包まれた。", "%^s is surrounded by darkness."), t_name);
		}
	}

	if (monster_to_player)
	{
		if (can_use_lite_area)
		{
			(void)lite_area(target_ptr, 0, 3);
		}
		else
		{
			learn_spell(target_ptr, MS_DARKNESS);
			(void)unlite_area(target_ptr, 0, 3);
		}

		return;
	}

	if (!monster_to_monster) return;

	int lite_area = can_use_lite_area ? -1 : MS_DARKNESS;
	(void)project(target_ptr, m_idx, 3, y, x, 0, GF_LITE_WEAK, PROJECT_GRID | PROJECT_KILL, lite_area);
	lite_room(target_ptr, y, x);
}


/*!
* @brief RF6_TRAPSの処理。トラップ。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param y 対象の地点のy座標
* @param x 対象の地点のx座標
* @param m_idx 呪文を唱えるモンスターID
* @param なし
*/
void spell_RF6_TRAPS(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx)
{
	GAME_TEXT m_name[MAX_NLEN];
	monster_name(target_ptr, m_idx, m_name);
	disturb(target_ptr, TRUE, TRUE);

	if (target_ptr->blind)
		msg_format(_("%^sが何かをつぶやいて邪悪に微笑んだ。",
			"%^s mumbles, and then cackles evilly."), m_name);
	else
		msg_format(_("%^sが呪文を唱えて邪悪に微笑んだ。",
			"%^s casts a spell and cackles evilly."), m_name);

	learn_spell(target_ptr, MS_MAKE_TRAP);
	(void)trap_creation(target_ptr, y, x);
}


/*!
* @brief RF6_FORGETの処理。記憶消去。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param m_idx 呪文を唱えるモンスターID
* @param なし
*/
void spell_RF6_FORGET(player_type *target_ptr, MONSTER_IDX m_idx)
{
	DEPTH rlev = monster_level_idx(target_ptr->current_floor_ptr, m_idx);
	GAME_TEXT m_name[MAX_NLEN];
	monster_name(target_ptr, m_idx, m_name);

	disturb(target_ptr, TRUE, TRUE);

	msg_format(_("%^sがあなたの記憶を消去しようとしている。",
		"%^s tries to blank your mind."), m_name);

	if (randint0(100 + rlev / 2) < target_ptr->skill_sav)
	{
		msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
	}
	else if (lose_all_info(target_ptr))
	{
		msg_print(_("記憶が薄れてしまった。", "Your memories fade away."));
	}

	learn_spell(target_ptr, MS_FORGET);
}


/*!
* @brief RF6_RAISE_DEADの処理。死者復活。 /
* @param target_ptr プレーヤーへの参照ポインタ
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
*/
void spell_RF6_RAISE_DEAD(player_type *target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
	monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];

	monspell_message(target_ptr, m_idx, t_idx,
		_("%^sが何かをつぶやいた。", "%^s mumbles."),
		_("%^sが死者復活の呪文を唱えた。", "%^s casts a spell to revive corpses."),
		_("%^sが死者復活の呪文を唱えた。", "%^s casts a spell to revive corpses."),
		TARGET_TYPE);

	animate_dead(target_ptr, m_idx, m_ptr->fy, m_ptr->fx);
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
