#include "angband.h"
#include "avatar.h"
#include "spells-status.h"
#include "projection.h"

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

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);
}
