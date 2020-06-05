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
#include "spell-kind/spells-charm.h"
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
