#include "angband.h"
#include "avatar.h"
#include "spells-status.h"
#include "projection.h"
#include "spells.h"

/*!
 * @brief モンスター回復処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool heal_monster(DIRECTION dir, HIT_POINT dam)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_OLD_HEAL, dir, dam, flg));
}

/*!
 * @brief モンスター加速処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param power 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool speed_monster(DIRECTION dir, int power)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_OLD_SPEED, dir, power, flg));
}

/*!
 * @brief モンスター減速処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param power 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool slow_monster(DIRECTION dir, int power)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_OLD_SLOW, dir, power, flg));
}

/*!
 * @brief モンスター催眠処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param power 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool sleep_monster(DIRECTION dir, int power)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_OLD_SLEEP, dir, power, flg));
}

/*!
 * @brief モンスター拘束(STASIS)処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 * @details 威力はプレイヤーレベル*2に固定
 */
bool stasis_monster(DIRECTION dir)
{
	return (fire_ball_hide(GF_STASIS, dir, p_ptr->lev * 2, 0));
}

/*!
 * @brief 邪悪なモンスター拘束(STASIS)処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 * @details 威力はプレイヤーレベル*2に固定
 */
bool stasis_evil(DIRECTION dir)
{
	return (fire_ball_hide(GF_STASIS_EVIL, dir, p_ptr->lev * 2, 0));
}

/*!
 * @brief モンスター混乱処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev プレイヤーレベル(=効力)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool confuse_monster(DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_OLD_CONF, dir, plev, flg));
}

/*!
 * @brief モンスター朦朧処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev プレイヤーレベル(=効力)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool stun_monster(DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_STUN, dir, plev, flg));
}

/*!
 * @brief チェンジモンスター処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param power 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool poly_monster(DIRECTION dir, int power)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	bool tester = (project_hook(GF_OLD_POLY, dir, power, flg));
	if (tester)
		chg_virtue(V_CHANCE, 1);
	return(tester);
}

/*!
 * @brief クローンモンスター処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool clone_monster(DIRECTION dir)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_OLD_CLONE, dir, 0, flg));
}

/*!
 * @brief モンスター恐慌処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev プレイヤーレベル(=効力)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool fear_monster(DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_TURN_ALL, dir, plev, flg));
}

/*!
* @brief 歌の停止を処理する / Stop singing if the player is a Bard
* @return なし
*/
void stop_singing(void)
{
	if (p_ptr->pclass != CLASS_BARD) return;

	/* Are there interupted song? */
	if (INTERUPTING_SONG_EFFECT(p_ptr))
	{
		/* Forget interupted song */
		INTERUPTING_SONG_EFFECT(p_ptr) = MUSIC_NONE;
		return;
	}

	/* The player is singing? */
	if (!SINGING_SONG_EFFECT(p_ptr)) return;

	/* Hack -- if called from set_action(), avoid recursive loop */
	if (p_ptr->action == ACTION_SING) set_action(ACTION_NONE);

	/* Message text of each song or etc. */
	do_spell(REALM_MUSIC, SINGING_SONG_ID(p_ptr), SPELL_STOP);

	SINGING_SONG_EFFECT(p_ptr) = MUSIC_NONE;
	SINGING_SONG_ID(p_ptr) = 0;
	p_ptr->update |= (PU_BONUS);
	p_ptr->redraw |= (PR_STATUS);
}

bool time_walk(player_type *creature_ptr)
{
	if (world_player)
	{
		msg_print(_("既に時は止まっている。", "Time is already stopped."));
		return (FALSE);
	}
	world_player = TRUE;
	msg_print(_("「時よ！」", "You yell 'Time!'"));
	msg_print(NULL);

	/* Hack */
	p_ptr->energy_need -= 1000 + (100 + p_ptr->csp - 50)*TURNS_PER_TICK / 10;
	p_ptr->redraw |= (PR_MAP);
	p_ptr->update |= (PU_MONSTERS);
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
	handle_stuff();
	return TRUE;
}

/*!
 * @brief プレイヤーのヒットダイスを振る / Role Hitpoints
 * @param options スペル共通オプション
 * @return なし
 */
void roll_hitdice(SPOP_FLAGS options)
{
	PERCENTAGE percent;

	/* Minimum hitpoints at highest level */
	HIT_POINT min_value = p_ptr->hitdie + ((PY_MAX_LEVEL + 2) * (p_ptr->hitdie + 1)) * 3 / 8;

	/* Maximum hitpoints at highest level */
	HIT_POINT max_value = p_ptr->hitdie + ((PY_MAX_LEVEL + 2) * (p_ptr->hitdie + 1)) * 5 / 8;

	int i;

	/* Rerate */
	while (1)
	{
		/* Pre-calculate level 1 hitdice */
		p_ptr->player_hp[0] = (HIT_POINT)p_ptr->hitdie;

		for (i = 1; i < 4; i++)
		{
			p_ptr->player_hp[0] += randint1(p_ptr->hitdie);
		}

		/* Roll the hitpoint values */
		for (i = 1; i < PY_MAX_LEVEL; i++)
		{
			p_ptr->player_hp[i] = p_ptr->player_hp[i - 1] + randint1(p_ptr->hitdie);
		}

		/* Require "valid" hitpoints at highest level */
		if ((p_ptr->player_hp[PY_MAX_LEVEL - 1] >= min_value) &&
			(p_ptr->player_hp[PY_MAX_LEVEL - 1] <= max_value)) break;
	}

	percent = (int)(((long)p_ptr->player_hp[PY_MAX_LEVEL - 1] * 200L) /
		(2 * p_ptr->hitdie + ((PY_MAX_LEVEL - 1 + 3) * (p_ptr->hitdie + 1))));

	/* Update and redraw hitpoints */
	p_ptr->update |= (PU_HP);
	p_ptr->redraw |= (PR_HP);
	p_ptr->window |= (PW_PLAYER);

	if (!(options & SPOP_NO_UPDATE)) handle_stuff();

	if (options & SPOP_DISPLAY_MES)
	{
		if (options & SPOP_DEBUG)
		{
			msg_format(_("現在の体力ランクは %d/100 です。", "Your life rate is %d/100 now."), percent);
			p_ptr->knowledge |= KNOW_HPRATE;
		}
		else
		{
			msg_print(_("体力ランクが変わった。", "Life rate is changed."));
			p_ptr->knowledge &= ~(KNOW_HPRATE);
		}
	}
}
