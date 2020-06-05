/*!
 * @brief 魔法効果の実装/ Spell code
 * @date 2014/07/15
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * </pre>
 */

#include "spell/spells2.h"
#include "cmd-action/cmd-attack.h"
#include "cmd-action/cmd-pet.h"
#include "cmd-io/cmd-dump.h"
#include "combat/combat-options-type.h"
#include "core/stuff-handler.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "effect/spells-effect-util.h"
#include "floor/floor-events.h"
#include "floor/floor-object.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "inventory/player-inventory.h"
#include "io/targeting.h"
#include "io/write-diary.h"
#include "locale/english.h"
#include "main/sound-definitions-table.h"
#include "monster/creature.h"
#include "monster/monster-race.h"
#include "monster/monster-status.h"
#include "mutation/mutation.h"
#include "object-enchant/item-feeling.h"
#include "object/item-use-flags.h"
#include "perception/object-perception.h"
#include "object/object-flavor.h"
#include "object/object-generator.h"
#include "object/object-kind-hook.h"
#include "object/object-mark-types.h"
#include "object-enchant/special-object-flags.h"
#include "pet/pet-fall-off.h"
#include "pet/pet-util.h"
#include "perception/simple-perception.h"
#include "player/avatar.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-effects.h"
#include "player/player-skill.h"
#include "player/player-status.h"
#include "spell/process-effect.h"
#include "spell/spells-diceroll.h"
#include "spell-kind/spells-chaos.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-genocide.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-specific-bolt.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "spell/spells-type.h"
#include "spell/range-calc.h"
#include "spell/spells3.h"
#include "sv-definition/sv-food-types.h"
#include "system/system-variables.h"
#include "util/util.h"
#include "view/display-main-window.h"
#include "world/world.h"

/*!
 * @brief チャーム・モンスター(1体)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev パワー
 * @return 作用が実際にあった場合TRUEを返す
 */
bool charm_monster(player_type *caster_ptr, DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(caster_ptr, GF_CHARM, dir, plev, flg));
}


/*!
 * @brief アンデッド支配(1体)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev パワー
 * @return 作用が実際にあった場合TRUEを返す
 */
bool control_one_undead(player_type *caster_ptr, DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(caster_ptr, GF_CONTROL_UNDEAD, dir, plev, flg));
}


/*!
 * @brief 悪魔支配(1体)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev パワー
 * @return 作用が実際にあった場合TRUEを返す
 */
bool control_one_demon(player_type *caster_ptr, DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(caster_ptr, GF_CONTROL_DEMON, dir, plev, flg));
}


/*!
 * @brief 動物支配(1体)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev パワー
 * @return 作用が実際にあった場合TRUEを返す
 */
bool charm_animal(player_type *caster_ptr, DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(caster_ptr, GF_CONTROL_ANIMAL, dir, plev, flg));
}


/*!
* @brief カオス魔法「流星群」の処理としてプレイヤーを中心に隕石落下処理を10+1d10回繰り返す。
* / Drop 10+1d10 meteor ball at random places near the player
* @param caster_ptr プレーヤーへの参照ポインタ
* @param dam ダメージ
* @param rad 効力の半径
* @return なし
*/
void cast_meteor(player_type *caster_ptr, HIT_POINT dam, POSITION rad)
{
	int b = 10 + randint1(10);
	for (int i = 0; i < b; i++)
	{
		POSITION y = 0, x = 0;
		int count;

		for (count = 0; count <= 20; count++)
		{
			int dy, dx, d;

			x = caster_ptr->x - 8 + randint0(17);
			y = caster_ptr->y - 8 + randint0(17);
			dx = (caster_ptr->x > x) ? (caster_ptr->x - x) : (x - caster_ptr->x);
			dy = (caster_ptr->y > y) ? (caster_ptr->y - y) : (y - caster_ptr->y);
			d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));

			if (d >= 9) continue;

			floor_type *floor_ptr = caster_ptr->current_floor_ptr;
			if (!in_bounds(floor_ptr, y, x) || !projectable(caster_ptr, caster_ptr->y, caster_ptr->x, y, x)
				|| !cave_have_flag_bold(floor_ptr, y, x, FF_PROJECT)) continue;

			break;
		}

		if (count > 20) continue;

		project(caster_ptr, 0, rad, y, x, dam, GF_METEOR, PROJECT_KILL | PROJECT_JUMP | PROJECT_ITEM, -1);
	}
}


/*!
* @brief 破邪魔法「神の怒り」の処理としてターゲットを指定した後分解のボールを最大20回発生させる。
* @param caster_ptr プレーヤーへの参照ポインタ
* @param dam ダメージ
* @param rad 効力の半径
* @return ターゲットを指定し、実行したならばTRUEを返す。
*/
bool cast_wrath_of_the_god(player_type *caster_ptr, HIT_POINT dam, POSITION rad)
{
	DIRECTION dir;
	if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

	POSITION tx = caster_ptr->x + 99 * ddx[dir];
	POSITION ty = caster_ptr->y + 99 * ddy[dir];
	if ((dir == 5) && target_okay(caster_ptr))
	{
		tx = target_col;
		ty = target_row;
	}

	POSITION x = caster_ptr->x;
	POSITION y = caster_ptr->y;
	POSITION nx, ny;
	while (TRUE)
	{
		if ((y == ty) && (x == tx)) break;

		ny = y;
		nx = x;
		mmove2(&ny, &nx, caster_ptr->y, caster_ptr->x, ty, tx);
		if (MAX_RANGE <= distance(caster_ptr->y, caster_ptr->x, ny, nx)) break;
		if (!cave_have_flag_bold(caster_ptr->current_floor_ptr, ny, nx, FF_PROJECT)) break;
		if ((dir != 5) && caster_ptr->current_floor_ptr->grid_array[ny][nx].m_idx != 0) break;

		x = nx;
		y = ny;
	}

	tx = x;
	ty = y;

	int b = 10 + randint1(10);
	for (int i = 0; i < b; i++)
	{
		int count = 20, d = 0;

		while (count--)
		{
			int dx, dy;

			x = tx - 5 + randint0(11);
			y = ty - 5 + randint0(11);

			dx = (tx > x) ? (tx - x) : (x - tx);
			dy = (ty > y) ? (ty - y) : (y - ty);

			d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));
			if (d < 5) break;
		}

		if (count < 0) continue;

		if (!in_bounds(caster_ptr->current_floor_ptr, y, x) ||
			cave_stop_disintegration(caster_ptr->current_floor_ptr, y, x) ||
			!in_disintegration_range(caster_ptr->current_floor_ptr, ty, tx, y, x))
			continue;

		project(caster_ptr, 0, rad, y, x, dam, GF_DISINTEGRATE, PROJECT_JUMP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL, -1);
	}

	return TRUE;
}


/*!
* @brief トランプ領域の「シャッフル」の効果をランダムに決めて処理する。
* @param caster_ptr プレーヤーへの参照ポインタ
* @return なし
*/
void cast_shuffle(player_type *caster_ptr)
{
	PLAYER_LEVEL plev = caster_ptr->lev;
	DIRECTION dir;
	int die;
	int vir = virtue_number(caster_ptr, V_CHANCE);
	int i;

	if ((caster_ptr->pclass == CLASS_ROGUE) ||
		(caster_ptr->pclass == CLASS_HIGH_MAGE) ||
		(caster_ptr->pclass == CLASS_SORCERER))
		die = (randint1(110)) + plev / 5;
	else
		die = randint1(120);

	if (vir)
	{
		if (caster_ptr->virtues[vir - 1] > 0)
		{
			while (randint1(400) < caster_ptr->virtues[vir - 1]) die++;
		}
		else
		{
			while (randint1(400) < (0 - caster_ptr->virtues[vir - 1])) die--;
		}
	}

	msg_print(_("あなたはカードを切って一枚引いた...", "You shuffle the deck and draw a card..."));

	if (die < 30)
	{
		chg_virtue(caster_ptr, V_CHANCE, 1);
	}

	floor_type *floor_ptr = caster_ptr->current_floor_ptr;
	if (die < 7)
	{
		msg_print(_("なんてこった！《死》だ！", "Oh no! It's Death!"));

		for (i = 0; i < randint1(3); i++)
		{
			activate_hi_summon(caster_ptr, caster_ptr->y, caster_ptr->x, FALSE);
		}

		return;
	}

	if (die < 14)
	{
		msg_print(_("なんてこった！《悪魔》だ！", "Oh no! It's the Devil!"));
		summon_specific(caster_ptr, 0, caster_ptr->y, caster_ptr->x, floor_ptr->dun_level, SUMMON_DEMON, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
		return;
	}

	if (die < 18)
	{
		int count = 0;
		msg_print(_("なんてこった！《吊られた男》だ！", "Oh no! It's the Hanged Man."));
		activate_ty_curse(caster_ptr, FALSE, &count);
		return;
	}

	if (die < 22)
	{
		msg_print(_("《不調和の剣》だ。", "It's the swords of discord."));
		aggravate_monsters(caster_ptr, 0);
		return;
	}

	if (die < 26)
	{
		msg_print(_("《愚者》だ。", "It's the Fool."));
		do_dec_stat(caster_ptr, A_INT);
		do_dec_stat(caster_ptr, A_WIS);
		return;
	}

	if (die < 30)
	{
		msg_print(_("奇妙なモンスターの絵だ。", "It's the picture of a strange monster."));
		trump_summoning(caster_ptr, 1, FALSE, caster_ptr->y, caster_ptr->x, (floor_ptr->dun_level * 3 / 2), (32 + randint1(6)), PM_ALLOW_GROUP | PM_ALLOW_UNIQUE);
		return;
	}

	if (die < 33)
	{
		msg_print(_("《月》だ。", "It's the Moon."));
		unlite_area(caster_ptr, 10, 3);
		return;
	}

	if (die < 38)
	{
		msg_print(_("《運命の輪》だ。", "It's the Wheel of Fortune."));
		wild_magic(caster_ptr, randint0(32));
		return;
	}

	if (die < 40)
	{
		msg_print(_("テレポート・カードだ。", "It's a teleport trump card."));
		teleport_player(caster_ptr, 10, TELEPORT_PASSIVE);
		return;
	}

	if (die < 42)
	{
		msg_print(_("《正義》だ。", "It's Justice."));
		set_blessed(caster_ptr, caster_ptr->lev, FALSE);
		return;
	}

	if (die < 47)
	{
		msg_print(_("テレポート・カードだ。", "It's a teleport trump card."));
		teleport_player(caster_ptr, 100, TELEPORT_PASSIVE);
		return;
	}

	if (die < 52)
	{
		msg_print(_("テレポート・カードだ。", "It's a teleport trump card."));
		teleport_player(caster_ptr, 200, TELEPORT_PASSIVE);
		return;
	}

	if (die < 60)
	{
		msg_print(_("《塔》だ。", "It's the Tower."));
		wall_breaker(caster_ptr);
		return;
	}

	if (die < 72)
	{
		msg_print(_("《節制》だ。", "It's Temperance."));
		sleep_monsters_touch(caster_ptr);
		return;
	}

	if (die < 80)
	{
		msg_print(_("《塔》だ。", "It's the Tower."));
		earthquake(caster_ptr, caster_ptr->y, caster_ptr->x, 5, 0);
		return;
	}

	if (die < 82)
	{
		msg_print(_("友好的なモンスターの絵だ。", "It's the picture of a friendly monster."));
		trump_summoning(caster_ptr, 1, TRUE, caster_ptr->y, caster_ptr->x, (floor_ptr->dun_level * 3 / 2), SUMMON_MOLD, 0L);
		return;
	}

	if (die < 84)
	{
		msg_print(_("友好的なモンスターの絵だ。", "It's the picture of a friendly monster."));
		trump_summoning(caster_ptr, 1, TRUE, caster_ptr->y, caster_ptr->x, (floor_ptr->dun_level * 3 / 2), SUMMON_BAT, 0L);
		return;
	}

	if (die < 86)
	{
		msg_print(_("友好的なモンスターの絵だ。", "It's the picture of a friendly monster."));
		trump_summoning(caster_ptr, 1, TRUE, caster_ptr->y, caster_ptr->x, (floor_ptr->dun_level * 3 / 2), SUMMON_VORTEX, 0L);
		return;
	}

	if (die < 88)
	{
		msg_print(_("友好的なモンスターの絵だ。", "It's the picture of a friendly monster."));
		trump_summoning(caster_ptr, 1, TRUE, caster_ptr->y, caster_ptr->x, (floor_ptr->dun_level * 3 / 2), SUMMON_COIN_MIMIC, 0L);
		return;
	}

	if (die < 96)
	{
		msg_print(_("《恋人》だ。", "It's the Lovers."));

		if (get_aim_dir(caster_ptr, &dir))
		{
			charm_monster(caster_ptr, dir, MIN(caster_ptr->lev, 20));
		}

		return;
	}

	if (die < 101)
	{
		msg_print(_("《隠者》だ。", "It's the Hermit."));
		wall_stone(caster_ptr);
		return;
	}

	if (die < 111)
	{
		msg_print(_("《審判》だ。", "It's the Judgement."));
		roll_hitdice(caster_ptr, 0L);
		lose_all_mutations(caster_ptr);
		return;
	}

	if (die < 120)
	{
		msg_print(_("《太陽》だ。", "It's the Sun."));
		chg_virtue(caster_ptr, V_KNOWLEDGE, 1);
		chg_virtue(caster_ptr, V_ENLIGHTEN, 1);
		wiz_lite(caster_ptr, FALSE);
		return;
	}

	msg_print(_("《世界》だ。", "It's the World."));
	if (caster_ptr->exp >= PY_MAX_EXP)
	{
		return;
	}

	s32b ee = (caster_ptr->exp / 25) + 1;
	if (ee > 5000) ee = 5000;
	msg_print(_("更に経験を積んだような気がする。", "You feel more experienced."));
	gain_exp(caster_ptr, ee);
}


bool vampirism(player_type *caster_ptr)
{
	if (d_info[caster_ptr->dungeon_idx].flags1 & DF1_NO_MELEE)
	{
		msg_print(_("なぜか攻撃することができない。", "Something prevents you from attacking."));
		return FALSE;
	}

	DIRECTION dir;
	if (!get_direction(caster_ptr, &dir, FALSE, FALSE)) return FALSE;

	POSITION y = caster_ptr->y + ddy[dir];
	POSITION x = caster_ptr->x + ddx[dir];
	grid_type *g_ptr;
	g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][x];
	stop_mouth(caster_ptr);
	if (!(g_ptr->m_idx))
	{
		msg_print(_("何もない場所に噛みついた！", "You bite into thin air!"));
		return FALSE;
	}

	msg_print(_("あなたはニヤリとして牙をむいた...", "You grin and bare your fangs..."));

	int dummy = caster_ptr->lev * 2;
	if (!hypodynamic_bolt(caster_ptr, dir, dummy))
	{
		msg_print(_("げぇ！ひどい味だ。", "Yechh. That tastes foul."));
		return TRUE;
	}

	if (caster_ptr->food < PY_FOOD_FULL)
		(void)hp_player(caster_ptr, dummy);
	else
		msg_print(_("あなたは空腹ではありません。", "You were not hungry."));

	/* Gain nutritional sustenance: 150/hp drained */
	/* A Food ration gives 5000 food points (by contrast) */
	/* Don't ever get more than "Full" this way */
	/* But if we ARE Gorged,  it won't cure us */
	dummy = caster_ptr->food + MIN(5000, 100 * dummy);
	if (caster_ptr->food < PY_FOOD_MAX)   /* Not gorged already */
		(void)set_food(caster_ptr, dummy >= PY_FOOD_MAX ? PY_FOOD_MAX - 1 : dummy);
	return TRUE;
}


/*!
* ヒット＆アウェイのレイシャルパワー/突然変異
* @param caster_ptr プレーヤーへの参照ポインタ
* @return コマンドの入力先にモンスターがいたらTRUE
*/
bool hit_and_away(player_type *caster_ptr)
{
	DIRECTION dir;
	if (!get_direction(caster_ptr, &dir, FALSE, FALSE)) return FALSE;
	POSITION y = caster_ptr->y + ddy[dir];
	POSITION x = caster_ptr->x + ddx[dir];
	if (caster_ptr->current_floor_ptr->grid_array[y][x].m_idx)
	{
		do_cmd_attack(caster_ptr, y, x, 0);
		if (randint0(caster_ptr->skill_dis) < 7)
			msg_print(_("うまく逃げられなかった。", "You failed to run away."));
		else
			teleport_player(caster_ptr, 30, TELEPORT_SPONTANEOUS);
		return TRUE;
	}

	msg_print(_("その方向にはモンスターはいません。", "You don't see any monster in this direction"));
	msg_print(NULL);
	return FALSE;
}


bool android_inside_weapon(player_type *creature_ptr)
{
	DIRECTION dir;
	if (!get_aim_dir(creature_ptr, &dir)) return FALSE;

	if (creature_ptr->lev < 10)
	{
		msg_print(_("レイガンを発射した。", "You fire your ray gun."));
		fire_bolt(creature_ptr, GF_MISSILE, dir, (creature_ptr->lev + 1) / 2);
		return TRUE;
	}

	if (creature_ptr->lev < 25)
	{
		msg_print(_("ブラスターを発射した。", "You fire your blaster."));
		fire_bolt(creature_ptr, GF_MISSILE, dir, creature_ptr->lev);
		return TRUE;
	}

	if (creature_ptr->lev < 35)
	{
		msg_print(_("バズーカを発射した。", "You fire your bazooka."));
		fire_ball(creature_ptr, GF_MISSILE, dir, creature_ptr->lev * 2, 2);
		return TRUE;
	}

	if (creature_ptr->lev < 45)
	{
		msg_print(_("ビームキャノンを発射した。", "You fire a beam cannon."));
		fire_beam(creature_ptr, GF_MISSILE, dir, creature_ptr->lev * 2);
		return TRUE;
	}

	msg_print(_("ロケットを発射した。", "You fire a rocket."));
	fire_rocket(creature_ptr, GF_ROCKET, dir, creature_ptr->lev * 5, 2);
	return TRUE;
}


bool create_ration(player_type *creature_ptr)
{
	object_type *q_ptr;
	object_type forge;
	q_ptr = &forge;

	/* Create the food ration */
	object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_RATION));

	/* Drop the object from heaven */
	(void)drop_near(creature_ptr, q_ptr, -1, creature_ptr->y, creature_ptr->x);
	msg_print(_("食事を料理して作った。", "You cook some food."));
	return TRUE;
}


void hayagake(player_type *creature_ptr)
{
	if (creature_ptr->action == ACTION_HAYAGAKE)
	{
		set_action(creature_ptr, ACTION_NONE);
		creature_ptr->energy_use = 0;
		return;
	}

	grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x];
	feature_type *f_ptr = &f_info[g_ptr->feat];

	if (!have_flag(f_ptr->flags, FF_PROJECT) ||
		(!creature_ptr->levitation && have_flag(f_ptr->flags, FF_DEEP)))
	{
		msg_print(_("ここでは素早く動けない。", "You cannot run in here."));
	}
	else
	{
		set_action(creature_ptr, ACTION_HAYAGAKE);
	}

	creature_ptr->energy_use = 0;
}


bool double_attack(player_type *creature_ptr)
{
	DIRECTION dir;
	if (!get_rep_dir(creature_ptr, &dir, FALSE)) return FALSE;
	POSITION y = creature_ptr->y + ddy[dir];
	POSITION x = creature_ptr->x + ddx[dir];
	if (!creature_ptr->current_floor_ptr->grid_array[y][x].m_idx)
	{
		msg_print(_("その方向にはモンスターはいません。", "You don't see any monster in this direction"));
		msg_print(NULL);
		return TRUE;
	}

	if (one_in_(3))
		msg_print(_("あーたたたたたたたたたたたたたたたたたたたたたた！！！",
			"Ahhhtatatatatatatatatatatatatatataatatatatattaaaaa!!!!"));
	else if (one_in_(2))
		msg_print(_("無駄無駄無駄無駄無駄無駄無駄無駄無駄無駄無駄無駄！！！",
			"Mudamudamudamudamudamudamudamudamudamudamudamudamuda!!!!"));
	else
		msg_print(_("オラオラオラオラオラオラオラオラオラオラオラオラ！！！",
			"Oraoraoraoraoraoraoraoraoraoraoraoraoraoraoraoraora!!!!"));

	do_cmd_attack(creature_ptr, y, x, 0);
	if (creature_ptr->current_floor_ptr->grid_array[y][x].m_idx)
	{
		handle_stuff(creature_ptr);
		do_cmd_attack(creature_ptr, y, x, 0);
	}

	creature_ptr->energy_need += ENERGY_NEED();
	return TRUE;
}


bool comvert_hp_to_mp(player_type *creature_ptr)
{
	int gain_sp = take_hit(creature_ptr, DAMAGE_USELIFE, creature_ptr->lev, _("ＨＰからＭＰへの無謀な変換", "thoughtless conversion from HP to SP"), -1) / 5;
	if (!gain_sp)
	{
		msg_print(_("変換に失敗した。", "You failed to convert."));
		creature_ptr->redraw |= (PR_HP | PR_MANA);
		return TRUE;
	}

	creature_ptr->csp += gain_sp;
	if (creature_ptr->csp > creature_ptr->msp)
	{
		creature_ptr->csp = creature_ptr->msp;
		creature_ptr->csp_frac = 0;
	}

	creature_ptr->redraw |= (PR_HP | PR_MANA);
	return TRUE;
}


bool comvert_mp_to_hp(player_type *creature_ptr)
{
	if (creature_ptr->csp >= creature_ptr->lev / 5)
	{
		creature_ptr->csp -= creature_ptr->lev / 5;
		hp_player(creature_ptr, creature_ptr->lev);
	}
	else
	{
		msg_print(_("変換に失敗した。", "You failed to convert."));
	}

	creature_ptr->redraw |= (PR_HP | PR_MANA);
	return TRUE;
}


/*!
 * 剣の舞い
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return 常にTRUE
*/
bool sword_dancing(player_type *creature_ptr)
{
	DIRECTION dir;
	POSITION y = 0, x = 0;
	grid_type *g_ptr;
	for (int i = 0; i < 6; i++)
	{
		dir = randint0(8);
		y = creature_ptr->y + ddy_ddd[dir];
		x = creature_ptr->x + ddx_ddd[dir];
		g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];

		/* Hack -- attack monsters */
		if (g_ptr->m_idx)
			do_cmd_attack(creature_ptr, y, x, 0);
		else
		{
			msg_print(_("攻撃が空をきった。", "You attack the empty air."));
		}
	}

	return TRUE;
}


bool clear_mind(player_type *creature_ptr)
{
	if (total_friends)
	{
		msg_print(_("今はペットを操ることに集中していないと。", "Your pets demand all of your attention."));
		return FALSE;
	}

	msg_print(_("少し頭がハッキリした。", "You feel your head clear a little."));

	creature_ptr->csp += (3 + creature_ptr->lev / 20);
	if (creature_ptr->csp >= creature_ptr->msp)
	{
		creature_ptr->csp = creature_ptr->msp;
		creature_ptr->csp_frac = 0;
	}

	creature_ptr->redraw |= (PR_MANA);
	return TRUE;
}
