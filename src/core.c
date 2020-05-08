/*!
	@file dungeon.c
	@brief Angbandゲームエンジン / Angband game engine
	@date 2013/12/31
	@author
	Copyright (c) 1989 James E. Wilson, Robert A. Koeneke\n
	This software may be copied and distributed for educational, research, and\n
	not for profit purposes provided that this copyright and statement are\n
	included in all such copies.\n
	2013 Deskull rearranged comment for Doxygen.
 */

#include "angband.h"
#include "io/signal-handlers.h"
#include "util.h"
#include "main/music-definitions-table.h"
#include "main/sound-definitions-table.h"
#include "core.h"
#include "core/angband-version.h"
#include "core/stuff-handler.h"
#include "core/special-internal-keys.h"
#include "inet.h"
#include "gameterm.h"
#include "chuukei.h"

#include "creature.h"

#include "birth.h"
#include "market/building.h"
#include "io/write-diary.h"
#include "cmd/cmd-activate.h"
#include "cmd/cmd-autopick.h"
#include "cmd/cmd-diary.h"
#include "cmd/cmd-draw.h"
#include "cmd/cmd-dump.h"
#include "cmd/cmd-process-screen.h"
#include "cmd/cmd-eat.h"
#include "cmd/cmd-help.h"
#include "cmd/cmd-hissatsu.h"
#include "cmd/cmd-item.h"
#include "cmd/cmd-knowledge.h"
#include "cmd/cmd-magiceat.h"
#include "cmd/cmd-mane.h"
#include "cmd/cmd-macro.h"
#include "cmd/cmd-quaff.h"
#include "cmd/cmd-read.h"
#include "cmd/cmd-save.h"
#include "cmd/cmd-smith.h"
#include "cmd/cmd-usestaff.h"
#include "cmd/cmd-zaprod.h"
#include "cmd/cmd-zapwand.h"
#include "cmd/cmd-pet.h"
#include "cmd/cmd-basic.h"
#include "cmd/cmd-visuals.h"
#include "racial.h"
#include "snipe.h"
#include "dungeon.h"
#include "feature.h"
#include "floor.h"
#include "floor-events.h"
#include "floor-town.h"
#include "grid.h"
#include "object-ego.h"
#include "object-curse.h"
#include "object-flavor.h"
#include "knowledge/knowledge-autopick.h"
#include "knowledge/knowledge-quests.h"
#include "market/store.h"
#include "spell/technic-info-table.h"
#include "spells-summon.h"
#include "spells-object.h"
#include "spells-status.h"
#include "spells-floor.h"
#include "monster-spell.h"
#include "mind.h"
#include "world.h"
#include "mutation.h"
#include "market/arena-info-table.h"
#include "market/store-util.h"
#include "quest.h"
#include "artifact.h"
#include "avatar.h"
#include "view/display-player.h"
#include "player/process-name.h"
#include "player-move.h"
#include "player-status.h"
#include "player-class.h"
#include "player-race.h"
#include "player-personality.h"
#include "player-damage.h"
#include "player-effects.h"
#include "cmd-spell.h"
#include "realm/realm-hex.h"
#include "object/object-kind.h"
#include "object-hook.h"
#include "wild.h"
#include "monster-process.h"
#include "monster-status.h"
#include "monsterrace-hook.h"
#include "floor-save.h"
#include "feature.h"
#include "player-skill.h"
#include "player-inventory.h"

#include "view/display-main-window.h"
#include "dungeon-file.h"
#include "io/uid-checker.h"
#include "player/process-death.h"
#include "io/read-pref-file.h"
#include "files.h"
#include "scores.h"
#include "autopick/autopick.h"
#include "autopick/autopick-pref-processor.h"
#include "autopick/autopick-reader-writer.h"
#include "save.h"
#include "realm/realm.h"
#include "realm/realm-song.h"
#include "targeting.h"
#include "spell/spells-util.h"
#include "spell/spells-execution.h"
#include "spell/spells2.h"

 /*!
  * コピーライト情報 /
  * Hack -- Link a copyright message into the executable
  */
const concptr copyright[5] =
{
	"Copyright (c) 1989 James E. Wilson, Robert A. Keoneke",
	"",
	"This software may be copied and distributed for educational, research,",
	"and not for profit purposes provided that this copyright and statement",
	"are included in all such copies."
};

bool can_save = FALSE;

COMMAND_CODE now_message;

bool repair_monsters;
bool repair_objects;

concptr ANGBAND_SYS = "xxx";

#ifdef JP
concptr ANGBAND_KEYBOARD = "JAPAN";
#else
concptr ANGBAND_KEYBOARD = "0";
#endif

concptr ANGBAND_GRAF = "ascii";

static bool load = TRUE; /*!<ロード処理中の分岐フラグ*/
static int wild_regen = 20; /*!<広域マップ移動時の自然回復処理カウンタ（広域マップ1マス毎に20回処理を基本とする）*/

/*
 * Flags for initialization
 */
int init_flags;

/*!
 * @brief 擬似鑑定を実際に行い判定を反映する
 * @param slot 擬似鑑定を行うプレイヤーの所持リストID
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param heavy 重度の擬似鑑定を行うならばTRUE
 * @return なし
 */
static void sense_inventory_aux(player_type *creature_ptr, INVENTORY_IDX slot, bool heavy)
{
	byte feel;
	object_type *o_ptr = &creature_ptr->inventory_list[slot];
	GAME_TEXT o_name[MAX_NLEN];
	if (o_ptr->ident & (IDENT_SENSE))return;
	if (object_is_known(o_ptr)) return;

	feel = (heavy ? value_check_aux1(o_ptr) : value_check_aux2(o_ptr));
	if (!feel) return;

	if ((creature_ptr->muta3 & MUT3_BAD_LUCK) && !randint0(13))
	{
		switch (feel)
		{
		case FEEL_TERRIBLE:
		{
			feel = FEEL_SPECIAL;
			break;
		}
		case FEEL_WORTHLESS:
		{
			feel = FEEL_EXCELLENT;
			break;
		}
		case FEEL_CURSED:
		{
			if (heavy)
				feel = randint0(3) ? FEEL_GOOD : FEEL_AVERAGE;
			else
				feel = FEEL_UNCURSED;
			break;
		}
		case FEEL_AVERAGE:
		{
			feel = randint0(2) ? FEEL_CURSED : FEEL_GOOD;
			break;
		}
		case FEEL_GOOD:
		{
			if (heavy)
				feel = randint0(3) ? FEEL_CURSED : FEEL_AVERAGE;
			else
				feel = FEEL_CURSED;
			break;
		}
		case FEEL_EXCELLENT:
		{
			feel = FEEL_WORTHLESS;
			break;
		}
		case FEEL_SPECIAL:
		{
			feel = FEEL_TERRIBLE;
			break;
		}
		}
	}

	if (disturb_minor) disturb(creature_ptr, FALSE, FALSE);

	object_desc(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
	if (slot >= INVEN_RARM)
	{
#ifdef JP
		msg_format("%s%s(%c)は%sという感じがする...",
			describe_use(creature_ptr, slot), o_name, index_to_label(slot), game_inscriptions[feel]);
#else
		msg_format("You feel the %s (%c) you are %s %s %s...",
			o_name, index_to_label(slot), describe_use(creature_ptr, slot),
			((o_ptr->number == 1) ? "is" : "are"),
			game_inscriptions[feel]);
#endif

	}
	else
	{
#ifdef JP
		msg_format("ザックの中の%s(%c)は%sという感じがする...",
			o_name, index_to_label(slot), game_inscriptions[feel]);
#else
		msg_format("You feel the %s (%c) in your pack %s %s...",
			o_name, index_to_label(slot),
			((o_ptr->number == 1) ? "is" : "are"),
			game_inscriptions[feel]);
#endif

	}

	o_ptr->ident |= (IDENT_SENSE);
	o_ptr->feeling = feel;

	autopick_alter_item(creature_ptr, slot, destroy_feeling);
	creature_ptr->update |= (PU_COMBINE | PU_REORDER);
	creature_ptr->window |= (PW_INVEN | PW_EQUIP);
}


/*!
 * @brief 1プレイヤーターン毎に武器、防具の擬似鑑定が行われるかを判定する。
 * @return なし
 * @details
 * Sense the inventory\n
 *\n
 *   Class 0 = Warrior --> fast and heavy\n
 *   Class 1 = Mage    --> slow and light\n
 *   Class 2 = Priest  --> fast but light\n
 *   Class 3 = Rogue   --> okay and heavy\n
 *   Class 4 = Ranger  --> slow but heavy  (changed!)\n
 *   Class 5 = Paladin --> slow but heavy\n
 */
static void sense_inventory1(player_type *creature_ptr)
{
	PLAYER_LEVEL plev = creature_ptr->lev;
	bool heavy = FALSE;
	object_type *o_ptr;
	if (creature_ptr->confused) return;

	switch (creature_ptr->pclass)
	{
	case CLASS_WARRIOR:
	case CLASS_ARCHER:
	case CLASS_SAMURAI:
	case CLASS_CAVALRY:
	{
		if (0 != randint0(9000L / (plev * plev + 40))) return;

		heavy = TRUE;
		break;
	}
	case CLASS_SMITH:
	{
		if (0 != randint0(6000L / (plev * plev + 50))) return;

		heavy = TRUE;
		break;
	}
	case CLASS_MAGE:
	case CLASS_HIGH_MAGE:
	case CLASS_SORCERER:
	case CLASS_MAGIC_EATER:
	{
		if (0 != randint0(240000L / (plev + 5))) return;

		break;
	}
	case CLASS_PRIEST:
	case CLASS_BARD:
	{
		if (0 != randint0(10000L / (plev * plev + 40))) return;

		break;
	}
	case CLASS_ROGUE:
	case CLASS_NINJA:
	{
		if (0 != randint0(20000L / (plev * plev + 40))) return;

		heavy = TRUE;
		break;
	}
	case CLASS_RANGER:
	{
		if (0 != randint0(95000L / (plev * plev + 40))) return;

		heavy = TRUE;
		break;
	}
	case CLASS_PALADIN:
	case CLASS_SNIPER:
	{
		if (0 != randint0(77777L / (plev * plev + 40))) return;

		heavy = TRUE;
		break;
	}
	case CLASS_WARRIOR_MAGE:
	case CLASS_RED_MAGE:
	{
		if (0 != randint0(75000L / (plev * plev + 40))) return;

		break;
	}
	case CLASS_MINDCRAFTER:
	case CLASS_IMITATOR:
	case CLASS_BLUE_MAGE:
	case CLASS_MIRROR_MASTER:
	{
		if (0 != randint0(55000L / (plev * plev + 40))) return;

		break;
	}
	case CLASS_CHAOS_WARRIOR:
	{
		if (0 != randint0(80000L / (plev * plev + 40))) return;

		heavy = TRUE;
		break;
	}
	case CLASS_MONK:
	case CLASS_FORCETRAINER:
	{
		if (0 != randint0(20000L / (plev * plev + 40))) return;

		break;
	}
	case CLASS_TOURIST:
	{
		if (0 != randint0(20000L / ((plev + 50)*(plev + 50)))) return;

		heavy = TRUE;
		break;
	}
	case CLASS_BEASTMASTER:
	{
		if (0 != randint0(65000L / (plev * plev + 40))) return;

		break;
	}
	case CLASS_BERSERKER:
	{
		heavy = TRUE;
		break;
	}
	}

	if (compare_virtue(creature_ptr, V_KNOWLEDGE, 100, VIRTUE_LARGE)) heavy = TRUE;

	for (INVENTORY_IDX i = 0; i < INVEN_TOTAL; i++)
	{
		bool okay = FALSE;

		o_ptr = &creature_ptr->inventory_list[i];

		if (!o_ptr->k_idx) continue;

		switch (o_ptr->tval)
		{
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_BOW:
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		case TV_CARD:
		{
			okay = TRUE;
			break;
		}
		}

		if (!okay) continue;
		if ((i < INVEN_RARM) && (0 != randint0(5))) continue;

		if ((creature_ptr->muta3 & MUT3_GOOD_LUCK) && !randint0(13))
		{
			heavy = TRUE;
		}

		sense_inventory_aux(creature_ptr, i, heavy);
	}
}


/*!
 * @brief 1プレイヤーターン毎に武器、防具以外の擬似鑑定が行われるかを判定する。
 * @return なし
 */
static void sense_inventory2(player_type *creature_ptr)
{
	PLAYER_LEVEL plev = creature_ptr->lev;
	object_type *o_ptr;

	if (creature_ptr->confused) return;

	switch (creature_ptr->pclass)
	{
	case CLASS_WARRIOR:
	case CLASS_ARCHER:
	case CLASS_SAMURAI:
	case CLASS_CAVALRY:
	case CLASS_BERSERKER:
	case CLASS_SNIPER:
	{
		return;
	}
	case CLASS_SMITH:
	case CLASS_PALADIN:
	case CLASS_CHAOS_WARRIOR:
	case CLASS_IMITATOR:
	case CLASS_BEASTMASTER:
	case CLASS_NINJA:
	{
		if (0 != randint0(240000L / (plev + 5))) return;

		break;
	}
	case CLASS_RANGER:
	case CLASS_WARRIOR_MAGE:
	case CLASS_RED_MAGE:
	case CLASS_MONK:
	{
		if (0 != randint0(95000L / (plev * plev + 40))) return;

		break;
	}
	case CLASS_PRIEST:
	case CLASS_BARD:
	case CLASS_ROGUE:
	case CLASS_FORCETRAINER:
	case CLASS_MINDCRAFTER:
	{
		if (0 != randint0(20000L / (plev * plev + 40))) return;

		break;
	}
	case CLASS_MAGE:
	case CLASS_HIGH_MAGE:
	case CLASS_SORCERER:
	case CLASS_MAGIC_EATER:
	case CLASS_MIRROR_MASTER:
	case CLASS_BLUE_MAGE:
	{
		if (0 != randint0(9000L / (plev * plev + 40))) return;

		break;
	}
	case CLASS_TOURIST:
	{
		if (0 != randint0(20000L / ((plev + 50)*(plev + 50)))) return;

		break;
	}
	}

	for (INVENTORY_IDX i = 0; i < INVEN_TOTAL; i++)
	{
		bool okay = FALSE;
		o_ptr = &creature_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;

		switch (o_ptr->tval)
		{
		case TV_RING:
		case TV_AMULET:
		case TV_LITE:
		case TV_FIGURINE:
		{
			okay = TRUE;
			break;
		}
		}

		if (!okay) continue;
		if ((i < INVEN_RARM) && (0 != randint0(5))) continue;

		sense_inventory_aux(creature_ptr, i, TRUE);
	}
}


/*!
 * @brief パターン終点到達時のテレポート処理を行う
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void pattern_teleport(player_type *creature_ptr)
{
	DEPTH min_level = 0;
	DEPTH max_level = 99;

	if (get_check(_("他の階にテレポートしますか？", "Teleport level? ")))
	{
		char ppp[80];
		char tmp_val[160];

		if (ironman_downward)
			min_level = creature_ptr->current_floor_ptr->dun_level;

		if (creature_ptr->dungeon_idx == DUNGEON_ANGBAND)
		{
			if (creature_ptr->current_floor_ptr->dun_level > 100)
				max_level = MAX_DEPTH - 1;
			else if (creature_ptr->current_floor_ptr->dun_level == 100)
				max_level = 100;
		}
		else
		{
			max_level = d_info[creature_ptr->dungeon_idx].maxdepth;
			min_level = d_info[creature_ptr->dungeon_idx].mindepth;
		}

		sprintf(ppp, _("テレポート先:(%d-%d)", "Teleport to level (%d-%d): "), (int)min_level, (int)max_level);
		sprintf(tmp_val, "%d", (int)creature_ptr->current_floor_ptr->dun_level);
		if (!get_string(ppp, tmp_val, 10)) return;

		command_arg = (COMMAND_ARG)atoi(tmp_val);
	}
	else if (get_check(_("通常テレポート？", "Normal teleport? ")))
	{
		teleport_player(creature_ptr, 200, TELEPORT_SPONTANEOUS);
		return;
	}
	else
	{
		return;
	}

	if (command_arg < min_level) command_arg = (COMMAND_ARG)min_level;
	if (command_arg > max_level) command_arg = (COMMAND_ARG)max_level;

	msg_format(_("%d 階にテレポートしました。", "You teleport to dungeon level %d."), command_arg);
	if (autosave_l) do_cmd_save_game(creature_ptr, TRUE);

	creature_ptr->current_floor_ptr->dun_level = command_arg;
	leave_quest_check(creature_ptr);
	if (record_stair) exe_write_diary(creature_ptr, DIARY_PAT_TELE, 0, NULL);

	creature_ptr->current_floor_ptr->inside_quest = 0;
	free_turn(creature_ptr);

	/*
	 * Clear all saved floors
	 * and create a first saved floor
	 */
	prepare_change_floor_mode(creature_ptr, CFM_FIRST_FLOOR);
	creature_ptr->leaving = TRUE;
}


/*!
 * @brief 各種パターン地形上の特別な処理 / Returns TRUE if we are on the Pattern...
 * @return 実際にパターン地形上にプレイヤーが居た場合はTRUEを返す。
 */
static bool pattern_effect(player_type *creature_ptr)
{
	floor_type *floor_ptr = creature_ptr->current_floor_ptr;
	if (!pattern_tile(floor_ptr, creature_ptr->y, creature_ptr->x)) return FALSE;

	if ((PRACE_IS_(creature_ptr, RACE_AMBERITE)) &&
		(creature_ptr->cut > 0) && one_in_(10))
	{
		wreck_the_pattern(creature_ptr);
	}

	int pattern_type = f_info[floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].feat].subtype;
	switch (pattern_type)
	{
	case PATTERN_TILE_END:
		(void)set_image(creature_ptr, 0);
		(void)restore_all_status(creature_ptr);
		(void)restore_level(creature_ptr);
		(void)cure_critical_wounds(creature_ptr, 1000);

		cave_set_feat(creature_ptr, creature_ptr->y, creature_ptr->x, feat_pattern_old);
		msg_print(_("「パターン」のこの部分は他の部分より強力でないようだ。", "This section of the Pattern looks less powerful."));

		/*
		 * We could make the healing effect of the
		 * Pattern center one-time only to avoid various kinds
		 * of abuse, like luring the win monster into fighting you
		 * in the middle of the pattern...
		 */
		break;

	case PATTERN_TILE_OLD:
		/* No effect */
		break;

	case PATTERN_TILE_TELEPORT:
		pattern_teleport(creature_ptr);
		break;

	case PATTERN_TILE_WRECKED:
		if (!IS_INVULN(creature_ptr))
			take_hit(creature_ptr, DAMAGE_NOESCAPE, 200, _("壊れた「パターン」を歩いたダメージ", "walking the corrupted Pattern"), -1);
		break;

	default:
		if (PRACE_IS_(creature_ptr, RACE_AMBERITE) && !one_in_(2))
			return TRUE;
		else if (!IS_INVULN(creature_ptr))
			take_hit(creature_ptr, DAMAGE_NOESCAPE, damroll(1, 3), _("「パターン」を歩いたダメージ", "walking the Pattern"), -1);
		break;
	}

	return TRUE;
}


/*!
 * @brief プレイヤーのHP自然回復処理 / Regenerate hit points -RAK-
 * @param percent 回復比率
 * @return なし
 */
static void regenhp(player_type *creature_ptr, int percent)
{
	if (creature_ptr->special_defense & KATA_KOUKIJIN) return;
	if (creature_ptr->action == ACTION_HAYAGAKE) return;

	HIT_POINT old_chp = creature_ptr->chp;

	/*
	 * Extract the new hitpoints
	 *
	 * 'percent' is the Regen factor in unit (1/2^16)
	 */
	HIT_POINT new_chp = 0;
	u32b new_chp_frac = (creature_ptr->mhp * percent + PY_REGEN_HPBASE);
	s64b_LSHIFT(new_chp, new_chp_frac, 16);
	s64b_add(&(creature_ptr->chp), &(creature_ptr->chp_frac), new_chp, new_chp_frac);
	if (0 < s64b_cmp(creature_ptr->chp, creature_ptr->chp_frac, creature_ptr->mhp, 0))
	{
		creature_ptr->chp = creature_ptr->mhp;
		creature_ptr->chp_frac = 0;
	}

	if (old_chp != creature_ptr->chp)
	{
		creature_ptr->redraw |= (PR_HP);
		creature_ptr->window |= (PW_PLAYER);
		wild_regen = 20;
	}
}


/*!
 * @brief プレイヤーのMP自然回復処理(regen_magic()のサブセット) / Regenerate mana points
 * @param upkeep_factor ペット維持によるMPコスト量
 * @param regen_amount 回復量
 * @return なし
 */
static void regenmana(player_type *creature_ptr, MANA_POINT upkeep_factor, MANA_POINT regen_amount)
{
	MANA_POINT old_csp = creature_ptr->csp;
	s32b regen_rate = regen_amount * 100 - upkeep_factor * PY_REGEN_NORMAL;

	/*
	 * Excess mana will decay 32 times faster than normal
	 * regeneration rate.
	 */
	if (creature_ptr->csp > creature_ptr->msp)
	{
		s32b decay = 0;
		u32b decay_frac = (creature_ptr->msp * 32 * PY_REGEN_NORMAL + PY_REGEN_MNBASE);
		s64b_LSHIFT(decay, decay_frac, 16);
		s64b_sub(&(creature_ptr->csp), &(creature_ptr->csp_frac), decay, decay_frac);
		if (creature_ptr->csp < creature_ptr->msp)
		{
			creature_ptr->csp = creature_ptr->msp;
			creature_ptr->csp_frac = 0;
		}
	}

	/* Regenerating mana (unless the player has excess mana) */
	else if (regen_rate > 0)
	{
		MANA_POINT new_mana = 0;
		u32b new_mana_frac = (creature_ptr->msp * regen_rate / 100 + PY_REGEN_MNBASE);
		s64b_LSHIFT(new_mana, new_mana_frac, 16);
		s64b_add(&(creature_ptr->csp), &(creature_ptr->csp_frac), new_mana, new_mana_frac);
		if (creature_ptr->csp >= creature_ptr->msp)
		{
			creature_ptr->csp = creature_ptr->msp;
			creature_ptr->csp_frac = 0;
		}
	}

	/* Reduce mana (even when the player has excess mana) */
	if (regen_rate < 0)
	{
		s32b reduce_mana = 0;
		u32b reduce_mana_frac = (creature_ptr->msp * (-1) * regen_rate / 100 + PY_REGEN_MNBASE);
		s64b_LSHIFT(reduce_mana, reduce_mana_frac, 16);
		s64b_sub(&(creature_ptr->csp), &(creature_ptr->csp_frac), reduce_mana, reduce_mana_frac);
		if (creature_ptr->csp < 0)
		{
			creature_ptr->csp = 0;
			creature_ptr->csp_frac = 0;
		}
	}

	if (old_csp != creature_ptr->csp)
	{
		creature_ptr->redraw |= (PR_MANA);
		creature_ptr->window |= (PW_PLAYER);
		creature_ptr->window |= (PW_SPELL);
		wild_regen = 20;
	}
}


/*!
 * @brief プレイヤーのMP自然回復処理 / Regenerate magic regen_amount: PY_REGEN_NORMAL * 2 (if resting) * 2 (if having regenarate)
 * @param regen_amount 回復量
 * @return なし
 */
static void regenmagic(player_type *creature_ptr, int regen_amount)
{
	MANA_POINT new_mana;
	int dev = 30;
	int mult = (dev + adj_mag_mana[creature_ptr->stat_ind[A_INT]]); /* x1 to x2 speed bonus for recharging */

	for (int i = 0; i < EATER_EXT * 2; i++)
	{
		if (!creature_ptr->magic_num2[i]) continue;
		if (creature_ptr->magic_num1[i] == ((long)creature_ptr->magic_num2[i] << 16)) continue;

		/* Increase remaining charge number like float value */
		new_mana = (regen_amount * mult * ((long)creature_ptr->magic_num2[i] + 13)) / (dev * 8);
		creature_ptr->magic_num1[i] += new_mana;

		/* Check maximum charge */
		if (creature_ptr->magic_num1[i] > (creature_ptr->magic_num2[i] << 16))
		{
			creature_ptr->magic_num1[i] = ((long)creature_ptr->magic_num2[i] << 16);
		}

		wild_regen = 20;
	}

	for (int i = EATER_EXT * 2; i < EATER_EXT * 3; i++)
	{
		if (!creature_ptr->magic_num1[i]) continue;
		if (!creature_ptr->magic_num2[i]) continue;

		/* Decrease remaining period for charging */
		new_mana = (regen_amount * mult * ((long)creature_ptr->magic_num2[i] + 10) * EATER_ROD_CHARGE)
			/ (dev * 16 * PY_REGEN_NORMAL);
		creature_ptr->magic_num1[i] -= new_mana;

		/* Check minimum remaining period for charging */
		if (creature_ptr->magic_num1[i] < 0) creature_ptr->magic_num1[i] = 0;
		wild_regen = 20;
	}
}


/*!
 * @brief 100ゲームターン毎のモンスターのHP自然回復処理 / Regenerate the monsters (once per 100 game turns)
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @note Should probably be done during monster turns.
 */
static void regenerate_monsters(player_type *player_ptr)
{
	for (int i = 1; i < player_ptr->current_floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		if (!monster_is_valid(m_ptr)) continue;

		if (m_ptr->hp < m_ptr->maxhp)
		{
			int frac = m_ptr->maxhp / 100;
			if (!frac) if (one_in_(2)) frac = 1;

			if (r_ptr->flags2 & RF2_REGENERATE) frac *= 2;

			m_ptr->hp += frac;
			if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

			if (player_ptr->health_who == i) player_ptr->redraw |= (PR_HEALTH);
			if (player_ptr->riding == i) player_ptr->redraw |= (PR_UHEALTH);
		}
	}
}


/*!
 * @brief 30ゲームターン毎のボール中モンスターのHP自然回復処理 / Regenerate the captured monsters (once per 30 game turns)
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @note Should probably be done during monster turns.
 */
static void regenerate_captured_monsters(player_type *creature_ptr)
{
	bool heal = FALSE;
	for (int i = 0; i < INVEN_TOTAL; i++)
	{
		monster_race *r_ptr;
		object_type *o_ptr = &creature_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;
		if (o_ptr->tval != TV_CAPTURE) continue;
		if (!o_ptr->pval) continue;

		heal = TRUE;
		r_ptr = &r_info[o_ptr->pval];
		if (o_ptr->xtra4 < o_ptr->xtra5)
		{
			int frac = o_ptr->xtra5 / 100;
			if (!frac) if (one_in_(2)) frac = 1;

			if (r_ptr->flags2 & RF2_REGENERATE) frac *= 2;

			o_ptr->xtra4 += (XTRA16)frac;
			if (o_ptr->xtra4 > o_ptr->xtra5) o_ptr->xtra4 = o_ptr->xtra5;
		}
	}

	if (heal)
	{
		creature_ptr->update |= (PU_COMBINE);
		creature_ptr->window |= (PW_INVEN);
		creature_ptr->window |= (PW_EQUIP);
		wild_regen = 20;
	}
}


/*!
 * @brief 寿命つき光源の警告メッセージ処理
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 現在光源として使っているオブジェクトの構造体参照ポインタ
 * @return なし
 */
static void notice_lite_change(player_type *creature_ptr, object_type *o_ptr)
{
	if ((o_ptr->xtra4 < 100) || (!(o_ptr->xtra4 % 100)))
	{
		creature_ptr->window |= (PW_EQUIP);
	}

	if (creature_ptr->blind)
	{
		if (o_ptr->xtra4 == 0) o_ptr->xtra4++;
	}
	else if (o_ptr->xtra4 == 0)
	{
		disturb(creature_ptr, FALSE, TRUE);
		msg_print(_("明かりが消えてしまった！", "Your light has gone out!"));
		creature_ptr->update |= (PU_TORCH);
		creature_ptr->update |= (PU_BONUS);
	}
	else if (o_ptr->name2 == EGO_LITE_LONG)
	{
		if ((o_ptr->xtra4 < 50) && (!(o_ptr->xtra4 % 5))
			&& (current_world_ptr->game_turn % (TURNS_PER_TICK * 2)))
		{
			if (disturb_minor) disturb(creature_ptr, FALSE, TRUE);
			msg_print(_("明かりが微かになってきている。", "Your light is growing faint."));
		}
	}
	else if ((o_ptr->xtra4 < 100) && (!(o_ptr->xtra4 % 10)))
	{
		if (disturb_minor) disturb(creature_ptr, FALSE, TRUE);
		msg_print(_("明かりが微かになってきている。", "Your light is growing faint."));
	}
}


/*!
 * @brief !!を刻んだ魔道具の時間経過による再充填を知らせる処理 / If player has inscribed the object with "!!", let him know when it's recharged. -LM-
 * @param o_ptr 対象オブジェクトの構造体参照ポインタ
 * @return なし
 */
static void recharged_notice(player_type *owner_ptr, object_type *o_ptr)
{
	if (!o_ptr->inscription) return;

	concptr s = my_strchr(quark_str(o_ptr->inscription), '!');
	while (s)
	{
		if (s[1] == '!')
		{
			GAME_TEXT o_name[MAX_NLEN];
			object_desc(owner_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
			msg_format("%sは再充填された。", o_name);
#else
			if (o_ptr->number > 1)
				msg_format("Your %s are recharged.", o_name);
			else
				msg_format("Your %s is recharged.", o_name);
#endif
			disturb(owner_ptr, FALSE, FALSE);
			return;
		}

		s = my_strchr(s + 1, '!');
	}
}


/*!
 * @brief プレイヤーの歌に関する継続処理
 * @return なし
 */
static void check_music(player_type *caster_ptr)
{
	if (caster_ptr->pclass != CLASS_BARD) return;
	if (!SINGING_SONG_EFFECT(caster_ptr) && !INTERUPTING_SONG_EFFECT(caster_ptr)) return;

	if (caster_ptr->anti_magic)
	{
		stop_singing(caster_ptr);
		return;
	}

	int spell = SINGING_SONG_ID(caster_ptr);
	const magic_type *s_ptr;
	s_ptr = &technic_info[REALM_MUSIC - MIN_TECHNIC][spell];

	MANA_POINT need_mana = mod_need_mana(caster_ptr, s_ptr->smana, spell, REALM_MUSIC);
	u32b need_mana_frac = 0;

	s64b_RSHIFT(need_mana, need_mana_frac, 1);
	if (s64b_cmp(caster_ptr->csp, caster_ptr->csp_frac, need_mana, need_mana_frac) < 0)
	{
		stop_singing(caster_ptr);
		return;
	}
	else
	{
		s64b_sub(&(caster_ptr->csp), &(caster_ptr->csp_frac), need_mana, need_mana_frac);

		caster_ptr->redraw |= PR_MANA;
		if (INTERUPTING_SONG_EFFECT(caster_ptr))
		{
			SINGING_SONG_EFFECT(caster_ptr) = INTERUPTING_SONG_EFFECT(caster_ptr);
			INTERUPTING_SONG_EFFECT(caster_ptr) = MUSIC_NONE;
			msg_print(_("歌を再開した。", "You restart singing."));
			caster_ptr->action = ACTION_SING;
			caster_ptr->update |= (PU_BONUS | PU_HP | PU_MONSTERS);
			caster_ptr->redraw |= (PR_MAP | PR_STATUS | PR_STATE);
			caster_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
		}
	}

	if (caster_ptr->spell_exp[spell] < SPELL_EXP_BEGINNER)
		caster_ptr->spell_exp[spell] += 5;
	else if (caster_ptr->spell_exp[spell] < SPELL_EXP_SKILLED)
	{
		if (one_in_(2) && (caster_ptr->current_floor_ptr->dun_level > 4) && ((caster_ptr->current_floor_ptr->dun_level + 10) > caster_ptr->lev)) caster_ptr->spell_exp[spell] += 1;
	}
	else if (caster_ptr->spell_exp[spell] < SPELL_EXP_EXPERT)
	{
		if (one_in_(5) && ((caster_ptr->current_floor_ptr->dun_level + 5) > caster_ptr->lev) && ((caster_ptr->current_floor_ptr->dun_level + 5) > s_ptr->slevel)) caster_ptr->spell_exp[spell] += 1;
	}
	else if (caster_ptr->spell_exp[spell] < SPELL_EXP_MASTER)
	{
		if (one_in_(5) && ((caster_ptr->current_floor_ptr->dun_level + 5) > caster_ptr->lev) && (caster_ptr->current_floor_ptr->dun_level > s_ptr->slevel)) caster_ptr->spell_exp[spell] += 1;
	}

	exe_spell(caster_ptr, REALM_MUSIC, spell, SPELL_CONT);
}


/*!
 * @brief 現在呪いを保持している装備品を一つランダムに探し出す / Choose one of items that have cursed flag
 * @param flag 探し出したい呪いフラグ配列
 * @return 該当の呪いが一つでもあった場合にランダムに選ばれた装備品のオブジェクト構造体参照ポインタを返す。\n
 * 呪いがない場合NULLを返す。
 */
static object_type *choose_cursed_obj_name(player_type *player_ptr, BIT_FLAGS flag)
{
	int choices[INVEN_TOTAL - INVEN_RARM];
	int number = 0;
	if (!(player_ptr->cursed & flag)) return NULL;

	for (int i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr = &player_ptr->inventory_list[i];
		if (o_ptr->curse_flags & flag)
		{
			choices[number] = i;
			number++;
		}
		else if ((flag == TRC_ADD_L_CURSE) ||
			(flag == TRC_ADD_H_CURSE) ||
			(flag == TRC_DRAIN_HP) ||
			(flag == TRC_DRAIN_MANA) ||
			(flag == TRC_CALL_ANIMAL) ||
			(flag == TRC_CALL_DEMON) ||
			(flag == TRC_CALL_DRAGON) ||
			(flag == TRC_CALL_UNDEAD) ||
			(flag == TRC_COWARDICE) ||
			(flag == TRC_LOW_MELEE) ||
			(flag == TRC_LOW_AC) ||
			(flag == TRC_LOW_MAGIC) ||
			(flag == TRC_FAST_DIGEST) ||
			(flag == TRC_SLOW_REGEN))
		{
			u32b cf = 0L;
			BIT_FLAGS flgs[TR_FLAG_SIZE];
			object_flags(o_ptr, flgs);
			switch (flag)
			{
			case TRC_ADD_L_CURSE: cf = TR_ADD_L_CURSE; break;
			case TRC_ADD_H_CURSE: cf = TR_ADD_H_CURSE; break;
			case TRC_DRAIN_HP: cf = TR_DRAIN_HP; break;
			case TRC_DRAIN_MANA: cf = TR_DRAIN_MANA; break;
			case TRC_CALL_ANIMAL: cf = TR_CALL_ANIMAL; break;
			case TRC_CALL_DEMON: cf = TR_CALL_DEMON; break;
			case TRC_CALL_DRAGON: cf = TR_CALL_DRAGON; break;
			case TRC_CALL_UNDEAD: cf = TR_CALL_UNDEAD; break;
			case TRC_COWARDICE: cf = TR_COWARDICE; break;
			case TRC_LOW_MELEE: cf = TR_LOW_MELEE; break;
			case TRC_LOW_AC: cf = TR_LOW_AC; break;
			case TRC_LOW_MAGIC: cf = TR_LOW_MAGIC; break;
			case TRC_FAST_DIGEST: cf = TR_FAST_DIGEST; break;
			case TRC_SLOW_REGEN: cf = TR_SLOW_REGEN; break;
			default: break;
			}
			if (have_flag(flgs, cf))
			{
				choices[number] = i;
				number++;
			}
		}
	}

	return &player_ptr->inventory_list[choices[randint0(number)]];
}


/*!
 * @brief 10ゲームターンが進行するごとにプレイヤーの空腹状態を飢餓方向に向かわせる
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void process_world_aux_digestion(player_type *creature_ptr)
{
	if (creature_ptr->phase_out) return;

	if (creature_ptr->food >= PY_FOOD_MAX)
	{
		(void)set_food(creature_ptr, creature_ptr->food - 100);
	}
	else if (!(current_world_ptr->game_turn % (TURNS_PER_TICK * 5)))
	{
		int digestion = SPEED_TO_ENERGY(creature_ptr->pspeed);
		if (creature_ptr->regenerate)
			digestion += 20;
		if (creature_ptr->special_defense & (KAMAE_MASK | KATA_MASK))
			digestion += 20;
		if (creature_ptr->cursed & TRC_FAST_DIGEST)
			digestion += 30;

		if (creature_ptr->slow_digest)
			digestion -= 5;

		if (digestion < 1) digestion = 1;
		if (digestion > 100) digestion = 100;

		(void)set_food(creature_ptr, creature_ptr->food - digestion);
	}

	if ((creature_ptr->food >= PY_FOOD_FAINT)) return;

	if (!creature_ptr->paralyzed && (randint0(100) < 10))
	{
		msg_print(_("あまりにも空腹で気絶してしまった。", "You faint from the lack of food."));
		disturb(creature_ptr, TRUE, TRUE);
		(void)set_paralyzed(creature_ptr, creature_ptr->paralyzed + 1 + randint0(5));
	}

	if (creature_ptr->food < PY_FOOD_STARVE)
	{
		HIT_POINT dam = (PY_FOOD_STARVE - creature_ptr->food) / 10;
		if (!IS_INVULN(creature_ptr)) take_hit(creature_ptr, DAMAGE_LOSELIFE, dam, _("空腹", "starvation"), -1);
	}
}


/*!
 * @brief 10ゲームターンが進行するごとにプレイヤーのHPとMPの増減処理を行う。
 *  / Handle timed damage and regeneration every 10 game turns
 * @return なし
 */
static void process_world_aux_hp_and_sp(player_type *creature_ptr)
{
	feature_type *f_ptr = &f_info[creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].feat];
	bool cave_no_regen = FALSE;
	int upkeep_factor = 0;
	int regen_amount = PY_REGEN_NORMAL;
	if (creature_ptr->poisoned && !IS_INVULN(creature_ptr))
	{
		take_hit(creature_ptr, DAMAGE_NOESCAPE, 1, _("毒", "poison"), -1);
	}

	if (creature_ptr->cut && !IS_INVULN(creature_ptr))
	{
		HIT_POINT dam;
		if (creature_ptr->cut > 1000)
		{
			dam = 200;
		}
		else if (creature_ptr->cut > 200)
		{
			dam = 80;
		}
		else if (creature_ptr->cut > 100)
		{
			dam = 32;
		}
		else if (creature_ptr->cut > 50)
		{
			dam = 16;
		}
		else if (creature_ptr->cut > 25)
		{
			dam = 7;
		}
		else if (creature_ptr->cut > 10)
		{
			dam = 3;
		}
		else
		{
			dam = 1;
		}

		take_hit(creature_ptr, DAMAGE_NOESCAPE, dam, _("致命傷", "a fatal wound"), -1);
	}

	if (PRACE_IS_(creature_ptr, RACE_VAMPIRE) || (creature_ptr->mimic_form == MIMIC_VAMPIRE))
	{
		if (!creature_ptr->current_floor_ptr->dun_level && !creature_ptr->resist_lite && !IS_INVULN(creature_ptr) && is_daytime())
		{
			if ((creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW)
			{
				msg_print(_("日光があなたのアンデッドの肉体を焼き焦がした！", "The sun's rays scorch your undead flesh!"));
				take_hit(creature_ptr, DAMAGE_NOESCAPE, 1, _("日光", "sunlight"), -1);
				cave_no_regen = TRUE;
			}
		}

		if (creature_ptr->inventory_list[INVEN_LITE].tval && (creature_ptr->inventory_list[INVEN_LITE].name2 != EGO_LITE_DARKNESS) &&
			!creature_ptr->resist_lite)
		{
			object_type *o_ptr = &creature_ptr->inventory_list[INVEN_LITE];
			GAME_TEXT o_name[MAX_NLEN];
			char ouch[MAX_NLEN + 40];
			object_desc(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
			msg_format(_("%sがあなたのアンデッドの肉体を焼き焦がした！", "The %s scorches your undead flesh!"), o_name);

			cave_no_regen = TRUE;
			object_desc(creature_ptr, o_name, o_ptr, OD_NAME_ONLY);
			sprintf(ouch, _("%sを装備したダメージ", "wielding %s"), o_name);

			if (!IS_INVULN(creature_ptr)) take_hit(creature_ptr, DAMAGE_NOESCAPE, 1, ouch, -1);
		}
	}

	if (have_flag(f_ptr->flags, FF_LAVA) && !IS_INVULN(creature_ptr) && !creature_ptr->immune_fire)
	{
		int damage = 0;

		if (have_flag(f_ptr->flags, FF_DEEP))
		{
			damage = 6000 + randint0(4000);
		}
		else if (!creature_ptr->levitation)
		{
			damage = 3000 + randint0(2000);
		}

		if (damage)
		{
			if (PRACE_IS_(creature_ptr, RACE_ENT)) damage += damage / 3;
			if (creature_ptr->resist_fire) damage = damage / 3;
			if (is_oppose_fire(creature_ptr)) damage = damage / 3;
			if (creature_ptr->levitation) damage = damage / 5;

			damage = damage / 100 + (randint0(100) < (damage % 100));

			if (creature_ptr->levitation)
			{
				msg_print(_("熱で火傷した！", "The heat burns you!"));
				take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, format(_("%sの上に浮遊したダメージ", "flying over %s"),
					f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name), -1);
			}
			else
			{
				concptr name = f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name;
				msg_format(_("%sで火傷した！", "The %s burns you!"), name);
				take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, name, -1);
			}

			cave_no_regen = TRUE;
		}
	}

	if (have_flag(f_ptr->flags, FF_COLD_PUDDLE) && !IS_INVULN(creature_ptr) && !creature_ptr->immune_cold)
	{
		int damage = 0;

		if (have_flag(f_ptr->flags, FF_DEEP))
		{
			damage = 6000 + randint0(4000);
		}
		else if (!creature_ptr->levitation)
		{
			damage = 3000 + randint0(2000);
		}

		if (damage)
		{
			if (creature_ptr->resist_cold) damage = damage / 3;
			if (is_oppose_cold(creature_ptr)) damage = damage / 3;
			if (creature_ptr->levitation) damage = damage / 5;

			damage = damage / 100 + (randint0(100) < (damage % 100));

			if (creature_ptr->levitation)
			{
				msg_print(_("冷気に覆われた！", "The cold engulfs you!"));
				take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, format(_("%sの上に浮遊したダメージ", "flying over %s"),
					f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name), -1);
			}
			else
			{
				concptr name = f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name;
				msg_format(_("%sに凍えた！", "The %s frostbites you!"), name);
				take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, name, -1);
			}

			cave_no_regen = TRUE;
		}
	}

	if (have_flag(f_ptr->flags, FF_ELEC_PUDDLE) && !IS_INVULN(creature_ptr) && !creature_ptr->immune_elec)
	{
		int damage = 0;

		if (have_flag(f_ptr->flags, FF_DEEP))
		{
			damage = 6000 + randint0(4000);
		}
		else if (!creature_ptr->levitation)
		{
			damage = 3000 + randint0(2000);
		}

		if (damage)
		{
			if (creature_ptr->resist_elec) damage = damage / 3;
			if (is_oppose_elec(creature_ptr)) damage = damage / 3;
			if (creature_ptr->levitation) damage = damage / 5;

			damage = damage / 100 + (randint0(100) < (damage % 100));

			if (creature_ptr->levitation)
			{
				msg_print(_("電撃を受けた！", "The electricity shocks you!"));
				take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, format(_("%sの上に浮遊したダメージ", "flying over %s"),
					f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name), -1);
			}
			else
			{
				concptr name = f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name;
				msg_format(_("%sに感電した！", "The %s shocks you!"), name);
				take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, name, -1);
			}

			cave_no_regen = TRUE;
		}
	}

	if (have_flag(f_ptr->flags, FF_ACID_PUDDLE) && !IS_INVULN(creature_ptr) && !creature_ptr->immune_acid)
	{
		int damage = 0;

		if (have_flag(f_ptr->flags, FF_DEEP))
		{
			damage = 6000 + randint0(4000);
		}
		else if (!creature_ptr->levitation)
		{
			damage = 3000 + randint0(2000);
		}

		if (damage)
		{
			if (creature_ptr->resist_acid) damage = damage / 3;
			if (is_oppose_acid(creature_ptr)) damage = damage / 3;
			if (creature_ptr->levitation) damage = damage / 5;

			damage = damage / 100 + (randint0(100) < (damage % 100));

			if (creature_ptr->levitation)
			{
				msg_print(_("酸が飛び散った！", "The acid melts you!"));
				take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, format(_("%sの上に浮遊したダメージ", "flying over %s"),
					f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name), -1);
			}
			else
			{
				concptr name = f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name;
				msg_format(_("%sに溶かされた！", "The %s melts you!"), name);
				take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, name, -1);
			}

			cave_no_regen = TRUE;
		}
	}

	if (have_flag(f_ptr->flags, FF_POISON_PUDDLE) && !IS_INVULN(creature_ptr))
	{
		int damage = 0;

		if (have_flag(f_ptr->flags, FF_DEEP))
		{
			damage = 6000 + randint0(4000);
		}
		else if (!creature_ptr->levitation)
		{
			damage = 3000 + randint0(2000);
		}

		if (damage)
		{
			if (creature_ptr->resist_pois) damage = damage / 3;
			if (is_oppose_pois(creature_ptr)) damage = damage / 3;
			if (creature_ptr->levitation) damage = damage / 5;

			damage = damage / 100 + (randint0(100) < (damage % 100));

			if (creature_ptr->levitation)
			{
				msg_print(_("毒気を吸い込んだ！", "The gas poisons you!"));
				take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, format(_("%sの上に浮遊したダメージ", "flying over %s"),
					f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name), -1);
				if (creature_ptr->resist_pois) (void)set_poisoned(creature_ptr, creature_ptr->poisoned + 1);
			}
			else
			{
				concptr name = f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name;
				msg_format(_("%sに毒された！", "The %s poisons you!"), name);
				take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, name, -1);
				if (creature_ptr->resist_pois) (void)set_poisoned(creature_ptr, creature_ptr->poisoned + 3);
			}

			cave_no_regen = TRUE;
		}
	}

	if (have_flag(f_ptr->flags, FF_WATER) && have_flag(f_ptr->flags, FF_DEEP) &&
		!creature_ptr->levitation && !creature_ptr->can_swim && !creature_ptr->resist_water)
	{
		if (creature_ptr->total_weight > weight_limit(creature_ptr))
		{
			msg_print(_("溺れている！", "You are drowning!"));
			take_hit(creature_ptr, DAMAGE_NOESCAPE, randint1(creature_ptr->lev), _("溺れ", "drowning"), -1);
			cave_no_regen = TRUE;
		}
	}

	if (creature_ptr->riding)
	{
		HIT_POINT damage;
		if ((r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].flags2 & RF2_AURA_FIRE) && !creature_ptr->immune_fire)
		{
			damage = r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].level / 2;
			if (PRACE_IS_(creature_ptr, RACE_ENT)) damage += damage / 3;
			if (creature_ptr->resist_fire) damage = damage / 3;
			if (is_oppose_fire(creature_ptr)) damage = damage / 3;
			msg_print(_("熱い！", "It's hot!"));
			take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, _("炎のオーラ", "Fire aura"), -1);
		}
		if ((r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].flags2 & RF2_AURA_ELEC) && !creature_ptr->immune_elec)
		{
			damage = r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].level / 2;
			if (PRACE_IS_(creature_ptr, RACE_ANDROID)) damage += damage / 3;
			if (creature_ptr->resist_elec) damage = damage / 3;
			if (is_oppose_elec(creature_ptr)) damage = damage / 3;
			msg_print(_("痛い！", "It hurts!"));
			take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, _("電気のオーラ", "Elec aura"), -1);
		}
		if ((r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].flags3 & RF3_AURA_COLD) && !creature_ptr->immune_cold)
		{
			damage = r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].level / 2;
			if (creature_ptr->resist_cold) damage = damage / 3;
			if (is_oppose_cold(creature_ptr)) damage = damage / 3;
			msg_print(_("冷たい！", "It's cold!"));
			take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, _("冷気のオーラ", "Cold aura"), -1);
		}
	}

	/* Spectres -- take damage when moving through walls */
	/*
	 * Added: ANYBODY takes damage if inside through walls
	 * without wraith form -- NOTE: Spectres will never be
	 * reduced below 0 hp by being inside a stone wall; others
	 * WILL BE!
	 */
	if (!have_flag(f_ptr->flags, FF_MOVE) && !have_flag(f_ptr->flags, FF_CAN_FLY))
	{
		if (!IS_INVULN(creature_ptr) && !creature_ptr->wraith_form && !creature_ptr->kabenuke && ((creature_ptr->chp > (creature_ptr->lev / 5)) || !creature_ptr->pass_wall))
		{
			concptr dam_desc;
			cave_no_regen = TRUE;

			if (creature_ptr->pass_wall)
			{
				msg_print(_("体の分子が分解した気がする！", "Your molecules feel disrupted!"));
				dam_desc = _("密度", "density");
			}
			else
			{
				msg_print(_("崩れた岩に押し潰された！", "You are being crushed!"));
				dam_desc = _("硬い岩", "solid rock");
			}

			take_hit(creature_ptr, DAMAGE_NOESCAPE, 1 + (creature_ptr->lev / 5), dam_desc, -1);
		}
	}

	if (creature_ptr->food < PY_FOOD_WEAK)
	{
		if (creature_ptr->food < PY_FOOD_STARVE)
		{
			regen_amount = 0;
		}
		else if (creature_ptr->food < PY_FOOD_FAINT)
		{
			regen_amount = PY_REGEN_FAINT;
		}
		else
		{
			regen_amount = PY_REGEN_WEAK;
		}
	}

	if (pattern_effect(creature_ptr))
	{
		cave_no_regen = TRUE;
	}
	else
	{
		if (creature_ptr->regenerate)
		{
			regen_amount = regen_amount * 2;
		}
		if (creature_ptr->special_defense & (KAMAE_MASK | KATA_MASK))
		{
			regen_amount /= 2;
		}
		if (creature_ptr->cursed & TRC_SLOW_REGEN)
		{
			regen_amount /= 5;
		}
	}

	if ((creature_ptr->action == ACTION_SEARCH) || (creature_ptr->action == ACTION_REST))
	{
		regen_amount = regen_amount * 2;
	}

	upkeep_factor = calculate_upkeep(creature_ptr);
	if ((creature_ptr->action == ACTION_LEARN) ||
		(creature_ptr->action == ACTION_HAYAGAKE) ||
		(creature_ptr->special_defense & KATA_KOUKIJIN))
	{
		upkeep_factor += 100;
	}

	regenmana(creature_ptr, upkeep_factor, regen_amount);
	if (creature_ptr->pclass == CLASS_MAGIC_EATER)
	{
		regenmagic(creature_ptr, regen_amount);
	}

	if ((creature_ptr->csp == 0) && (creature_ptr->csp_frac == 0))
	{
		while (upkeep_factor > 100)
		{
			msg_print(_("こんなに多くのペットを制御できない！", "Too many pets to control at once!"));
			msg_print(NULL);
			do_cmd_pet_dismiss(creature_ptr);

			upkeep_factor = calculate_upkeep(creature_ptr);

			msg_format(_("維持ＭＰは %d%%", "Upkeep: %d%% mana."), upkeep_factor);
			msg_print(NULL);
		}
	}

	if (creature_ptr->poisoned) regen_amount = 0;
	if (creature_ptr->cut) regen_amount = 0;
	if (cave_no_regen) regen_amount = 0;

	regen_amount = (regen_amount * creature_ptr->mutant_regenerate_mod) / 100;
	if ((creature_ptr->chp < creature_ptr->mhp) && !cave_no_regen)
	{
		regenhp(creature_ptr, regen_amount);
	}
}


/*!
 * @brief 10ゲームターンが進行するごとに魔法効果の残りターンを減らしていく処理
 * / Handle timeout every 10 game turns
 * @return なし
 */
static void process_world_aux_timeout(player_type *creature_ptr)
{
	const int dec_count = (easy_band ? 2 : 1);
	if (creature_ptr->tim_mimic)
	{
		(void)set_mimic(creature_ptr, creature_ptr->tim_mimic - 1, creature_ptr->mimic_form, TRUE);
	}

	if (creature_ptr->image)
	{
		(void)set_image(creature_ptr, creature_ptr->image - dec_count);
	}

	if (creature_ptr->blind)
	{
		(void)set_blind(creature_ptr, creature_ptr->blind - dec_count);
	}

	if (creature_ptr->tim_invis)
	{
		(void)set_tim_invis(creature_ptr, creature_ptr->tim_invis - 1, TRUE);
	}

	if (creature_ptr->suppress_multi_reward)
	{
		creature_ptr->suppress_multi_reward = FALSE;
	}

	if (creature_ptr->tim_esp)
	{
		(void)set_tim_esp(creature_ptr, creature_ptr->tim_esp - 1, TRUE);
	}

	if (creature_ptr->ele_attack)
	{
		creature_ptr->ele_attack--;
		if (!creature_ptr->ele_attack) set_ele_attack(creature_ptr, 0, 0);
	}

	if (creature_ptr->ele_immune)
	{
		creature_ptr->ele_immune--;
		if (!creature_ptr->ele_immune) set_ele_immune(creature_ptr, 0, 0);
	}

	if (creature_ptr->tim_infra)
	{
		(void)set_tim_infra(creature_ptr, creature_ptr->tim_infra - 1, TRUE);
	}

	if (creature_ptr->tim_stealth)
	{
		(void)set_tim_stealth(creature_ptr, creature_ptr->tim_stealth - 1, TRUE);
	}

	if (creature_ptr->tim_levitation)
	{
		(void)set_tim_levitation(creature_ptr, creature_ptr->tim_levitation - 1, TRUE);
	}

	if (creature_ptr->tim_sh_touki)
	{
		(void)set_tim_sh_touki(creature_ptr, creature_ptr->tim_sh_touki - 1, TRUE);
	}

	if (creature_ptr->tim_sh_fire)
	{
		(void)set_tim_sh_fire(creature_ptr, creature_ptr->tim_sh_fire - 1, TRUE);
	}

	if (creature_ptr->tim_sh_holy)
	{
		(void)set_tim_sh_holy(creature_ptr, creature_ptr->tim_sh_holy - 1, TRUE);
	}

	if (creature_ptr->tim_eyeeye)
	{
		(void)set_tim_eyeeye(creature_ptr, creature_ptr->tim_eyeeye - 1, TRUE);
	}

	if (creature_ptr->resist_magic)
	{
		(void)set_resist_magic(creature_ptr, creature_ptr->resist_magic - 1, TRUE);
	}

	if (creature_ptr->tim_regen)
	{
		(void)set_tim_regen(creature_ptr, creature_ptr->tim_regen - 1, TRUE);
	}

	if (creature_ptr->tim_res_nether)
	{
		(void)set_tim_res_nether(creature_ptr, creature_ptr->tim_res_nether - 1, TRUE);
	}

	if (creature_ptr->tim_res_time)
	{
		(void)set_tim_res_time(creature_ptr, creature_ptr->tim_res_time - 1, TRUE);
	}

	if (creature_ptr->tim_reflect)
	{
		(void)set_tim_reflect(creature_ptr, creature_ptr->tim_reflect - 1, TRUE);
	}

	if (creature_ptr->multishadow)
	{
		(void)set_multishadow(creature_ptr, creature_ptr->multishadow - 1, TRUE);
	}

	if (creature_ptr->dustrobe)
	{
		(void)set_dustrobe(creature_ptr, creature_ptr->dustrobe - 1, TRUE);
	}

	if (creature_ptr->kabenuke)
	{
		(void)set_kabenuke(creature_ptr, creature_ptr->kabenuke - 1, TRUE);
	}

	if (creature_ptr->paralyzed)
	{
		(void)set_paralyzed(creature_ptr, creature_ptr->paralyzed - dec_count);
	}

	if (creature_ptr->confused)
	{
		(void)set_confused(creature_ptr, creature_ptr->confused - dec_count);
	}

	if (creature_ptr->afraid)
	{
		(void)set_afraid(creature_ptr, creature_ptr->afraid - dec_count);
	}

	if (creature_ptr->fast)
	{
		(void)set_fast(creature_ptr, creature_ptr->fast - 1, TRUE);
	}

	if (creature_ptr->slow)
	{
		(void)set_slow(creature_ptr, creature_ptr->slow - dec_count, TRUE);
	}

	if (creature_ptr->protevil)
	{
		(void)set_protevil(creature_ptr, creature_ptr->protevil - 1, TRUE);
	}

	if (creature_ptr->invuln)
	{
		(void)set_invuln(creature_ptr, creature_ptr->invuln - 1, TRUE);
	}

	if (creature_ptr->wraith_form)
	{
		(void)set_wraith_form(creature_ptr, creature_ptr->wraith_form - 1, TRUE);
	}

	if (creature_ptr->hero)
	{
		(void)set_hero(creature_ptr, creature_ptr->hero - 1, TRUE);
	}

	if (creature_ptr->shero)
	{
		(void)set_shero(creature_ptr, creature_ptr->shero - 1, TRUE);
	}

	if (creature_ptr->blessed)
	{
		(void)set_blessed(creature_ptr, creature_ptr->blessed - 1, TRUE);
	}

	if (creature_ptr->shield)
	{
		(void)set_shield(creature_ptr, creature_ptr->shield - 1, TRUE);
	}

	if (creature_ptr->tsubureru)
	{
		(void)set_tsubureru(creature_ptr, creature_ptr->tsubureru - 1, TRUE);
	}

	if (creature_ptr->magicdef)
	{
		(void)set_magicdef(creature_ptr, creature_ptr->magicdef - 1, TRUE);
	}

	if (creature_ptr->tsuyoshi)
	{
		(void)set_tsuyoshi(creature_ptr, creature_ptr->tsuyoshi - 1, TRUE);
	}

	if (creature_ptr->oppose_acid)
	{
		(void)set_oppose_acid(creature_ptr, creature_ptr->oppose_acid - 1, TRUE);
	}

	if (creature_ptr->oppose_elec)
	{
		(void)set_oppose_elec(creature_ptr, creature_ptr->oppose_elec - 1, TRUE);
	}

	if (creature_ptr->oppose_fire)
	{
		(void)set_oppose_fire(creature_ptr, creature_ptr->oppose_fire - 1, TRUE);
	}

	if (creature_ptr->oppose_cold)
	{
		(void)set_oppose_cold(creature_ptr, creature_ptr->oppose_cold - 1, TRUE);
	}

	if (creature_ptr->oppose_pois)
	{
		(void)set_oppose_pois(creature_ptr, creature_ptr->oppose_pois - 1, TRUE);
	}

	if (creature_ptr->ult_res)
	{
		(void)set_ultimate_res(creature_ptr, creature_ptr->ult_res - 1, TRUE);
	}

	if (creature_ptr->poisoned)
	{
		int adjust = adj_con_fix[creature_ptr->stat_ind[A_CON]] + 1;
		(void)set_poisoned(creature_ptr, creature_ptr->poisoned - adjust);
	}

	if (creature_ptr->stun)
	{
		int adjust = adj_con_fix[creature_ptr->stat_ind[A_CON]] + 1;
		(void)set_stun(creature_ptr, creature_ptr->stun - adjust);
	}

	if (creature_ptr->cut)
	{
		int adjust = adj_con_fix[creature_ptr->stat_ind[A_CON]] + 1;
		if (creature_ptr->cut > 1000) adjust = 0;
		(void)set_cut(creature_ptr, creature_ptr->cut - adjust);
	}
}


/*!
 * @brief 10ゲームターンが進行する毎に光源の寿命を減らす処理
 * / Handle burning fuel every 10 game turns
 * @return なし
 */
static void process_world_aux_light(player_type *creature_ptr)
{
	object_type *o_ptr = &creature_ptr->inventory_list[INVEN_LITE];
	if (o_ptr->tval == TV_LITE)
	{
		if (!(object_is_fixed_artifact(o_ptr) || o_ptr->sval == SV_LITE_FEANOR) && (o_ptr->xtra4 > 0))
		{
			if (o_ptr->name2 == EGO_LITE_LONG)
			{
				if (current_world_ptr->game_turn % (TURNS_PER_TICK * 2)) o_ptr->xtra4--;
			}
			else o_ptr->xtra4--;

			notice_lite_change(creature_ptr, o_ptr);
		}
	}
}


/*!
 * @brief 10ゲームターンが進行するごとに突然変異の発動判定を行う処理
 * / Handle mutation effects once every 10 game turns
 * @return なし
 */
static void process_world_aux_mutation(player_type *creature_ptr)
{
	if (!creature_ptr->muta2) return;
	if (creature_ptr->phase_out) return;
	if (creature_ptr->wild_mode) return;

	if ((creature_ptr->muta2 & MUT2_BERS_RAGE) && one_in_(3000))
	{
		disturb(creature_ptr, FALSE, TRUE);
		msg_print(_("ウガァァア！", "RAAAAGHH!"));
		msg_print(_("激怒の発作に襲われた！", "You feel a fit of rage coming over you!"));
		(void)set_shero(creature_ptr, 10 + randint1(creature_ptr->lev), FALSE);
		(void)set_afraid(creature_ptr, 0);
	}

	if ((creature_ptr->muta2 & MUT2_COWARDICE) && (randint1(3000) == 13))
	{
		if (!creature_ptr->resist_fear)
		{
			disturb(creature_ptr, FALSE, TRUE);
			msg_print(_("とても暗い... とても恐い！", "It's so dark... so scary!"));
			set_afraid(creature_ptr, creature_ptr->afraid + 13 + randint1(26));
		}
	}

	if ((creature_ptr->muta2 & MUT2_RTELEPORT) && (randint1(5000) == 88))
	{
		if (!creature_ptr->resist_nexus && !(creature_ptr->muta1 & MUT1_VTELEPORT) && !creature_ptr->anti_tele)
		{
			disturb(creature_ptr, FALSE, TRUE);
			msg_print(_("あなたの位置は突然ひじょうに不確定になった...", "Your position suddenly seems very uncertain..."));
			msg_print(NULL);
			teleport_player(creature_ptr, 40, TELEPORT_PASSIVE);
		}
	}

	if ((creature_ptr->muta2 & MUT2_ALCOHOL) && (randint1(6400) == 321))
	{
		if (!creature_ptr->resist_conf && !creature_ptr->resist_chaos)
		{
			disturb(creature_ptr, FALSE, TRUE);
			creature_ptr->redraw |= PR_EXTRA;
			msg_print(_("いひきがもーろーとひてきたきがふる...ヒック！", "You feel a SSSCHtupor cOmINg over yOu... *HIC*!"));
		}

		if (!creature_ptr->resist_conf)
		{
			(void)set_confused(creature_ptr, creature_ptr->confused + randint0(20) + 15);
		}

		if (!creature_ptr->resist_chaos)
		{
			if (one_in_(20))
			{
				msg_print(NULL);
				if (one_in_(3)) lose_all_info(creature_ptr);
				else wiz_dark(creature_ptr);
				(void)teleport_player_aux(creature_ptr, 100, FALSE, TELEPORT_NONMAGICAL | TELEPORT_PASSIVE);
				wiz_dark(creature_ptr);
				msg_print(_("あなたは見知らぬ場所で目が醒めた...頭が痛い。", "You wake up somewhere with a sore head..."));
				msg_print(_("何も覚えていない。どうやってここに来たかも分からない！", "You can't remember a thing or how you got here!"));
			}
			else
			{
				if (one_in_(3))
				{
					msg_print(_("き～れいなちょおちょらとんれいる～", "Thishcischs GooDSChtuff!"));
					(void)set_image(creature_ptr, creature_ptr->image + randint0(150) + 150);
				}
			}
		}
	}

	if ((creature_ptr->muta2 & MUT2_HALLU) && (randint1(6400) == 42))
	{
		if (!creature_ptr->resist_chaos)
		{
			disturb(creature_ptr, FALSE, TRUE);
			creature_ptr->redraw |= PR_EXTRA;
			(void)set_image(creature_ptr, creature_ptr->image + randint0(50) + 20);
		}
	}

	if ((creature_ptr->muta2 & MUT2_FLATULENT) && (randint1(3000) == 13))
	{
		disturb(creature_ptr, FALSE, TRUE);
		msg_print(_("ブゥーーッ！おっと。", "BRRAAAP! Oops."));
		msg_print(NULL);
		fire_ball(creature_ptr, GF_POIS, 0, creature_ptr->lev, 3);
	}

	if ((creature_ptr->muta2 & MUT2_PROD_MANA) &&
		!creature_ptr->anti_magic && one_in_(9000))
	{
		int dire = 0;
		disturb(creature_ptr, FALSE, TRUE);
		msg_print(_("魔法のエネルギーが突然あなたの中に流れ込んできた！エネルギーを解放しなければならない！",
			"Magical energy flows through you! You must release it!"));

		flush();
		msg_print(NULL);
		(void)get_hack_dir(creature_ptr, &dire);
		fire_ball(creature_ptr, GF_MANA, dire, creature_ptr->lev * 2, 3);
	}

	if ((creature_ptr->muta2 & MUT2_ATT_DEMON) && !creature_ptr->anti_magic && (randint1(6666) == 666))
	{
		bool pet = one_in_(6);
		BIT_FLAGS mode = PM_ALLOW_GROUP;

		if (pet) mode |= PM_FORCE_PET;
		else mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);

		if (summon_specific(creature_ptr, (pet ? -1 : 0), creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_DEMON, mode))
		{
			msg_print(_("あなたはデーモンを引き寄せた！", "You have attracted a demon!"));
			disturb(creature_ptr, FALSE, TRUE);
		}
	}

	if ((creature_ptr->muta2 & MUT2_SPEED_FLUX) && one_in_(6000))
	{
		disturb(creature_ptr, FALSE, TRUE);
		if (one_in_(2))
		{
			msg_print(_("精力的でなくなった気がする。", "You feel less energetic."));

			if (creature_ptr->fast > 0)
			{
				set_fast(creature_ptr, 0, TRUE);
			}
			else
			{
				set_slow(creature_ptr, randint1(30) + 10, FALSE);
			}
		}
		else
		{
			msg_print(_("精力的になった気がする。", "You feel more energetic."));

			if (creature_ptr->slow > 0)
			{
				set_slow(creature_ptr, 0, TRUE);
			}
			else
			{
				set_fast(creature_ptr, randint1(30) + 10, FALSE);
			}
		}
		msg_print(NULL);
	}
	if ((creature_ptr->muta2 & MUT2_BANISH_ALL) && one_in_(9000))
	{
		disturb(creature_ptr, FALSE, TRUE);
		msg_print(_("突然ほとんど孤独になった気がする。", "You suddenly feel almost lonely."));

		banish_monsters(creature_ptr, 100);
		if (!creature_ptr->current_floor_ptr->dun_level && creature_ptr->town_num)
		{
			int n;
			do
			{
				n = randint0(MAX_STORES);
			} while ((n == STORE_HOME) || (n == STORE_MUSEUM));

			msg_print(_("店の主人が丘に向かって走っている！", "You see one of the shopkeepers running for the hills!"));
			store_shuffle(creature_ptr, n);
		}
		msg_print(NULL);
	}

	if ((creature_ptr->muta2 & MUT2_EAT_LIGHT) && one_in_(3000))
	{
		object_type *o_ptr;

		msg_print(_("影につつまれた。", "A shadow passes over you."));
		msg_print(NULL);

		if ((creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW)
		{
			hp_player(creature_ptr, 10);
		}

		o_ptr = &creature_ptr->inventory_list[INVEN_LITE];

		if (o_ptr->tval == TV_LITE)
		{
			if (!object_is_fixed_artifact(o_ptr) && (o_ptr->xtra4 > 0))
			{
				hp_player(creature_ptr, o_ptr->xtra4 / 20);
				o_ptr->xtra4 /= 2;
				msg_print(_("光源からエネルギーを吸収した！", "You absorb energy from your light!"));
				notice_lite_change(creature_ptr, o_ptr);
			}
		}

		/*
		 * Unlite the area (radius 10) around player and
		 * do 50 points damage to every affected monster
		 */
		unlite_area(creature_ptr, 50, 10);
	}

	if ((creature_ptr->muta2 & MUT2_ATT_ANIMAL) && !creature_ptr->anti_magic && one_in_(7000))
	{
		bool pet = one_in_(3);
		BIT_FLAGS mode = PM_ALLOW_GROUP;

		if (pet) mode |= PM_FORCE_PET;
		else mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);

		if (summon_specific(creature_ptr, (pet ? -1 : 0), creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_ANIMAL, mode))
		{
			msg_print(_("動物を引き寄せた！", "You have attracted an animal!"));
			disturb(creature_ptr, FALSE, TRUE);
		}
	}

	if ((creature_ptr->muta2 & MUT2_RAW_CHAOS) && !creature_ptr->anti_magic && one_in_(8000))
	{
		disturb(creature_ptr, FALSE, TRUE);
		msg_print(_("周りの空間が歪んでいる気がする！", "You feel the world warping around you!"));

		msg_print(NULL);
		fire_ball(creature_ptr, GF_CHAOS, 0, creature_ptr->lev, 8);
	}

	if ((creature_ptr->muta2 & MUT2_NORMALITY) && one_in_(5000))
	{
		if (!lose_mutation(creature_ptr, 0))
			msg_print(_("奇妙なくらい普通になった気がする。", "You feel oddly normal."));
	}

	if ((creature_ptr->muta2 & MUT2_WRAITH) && !creature_ptr->anti_magic && one_in_(3000))
	{
		disturb(creature_ptr, FALSE, TRUE);
		msg_print(_("非物質化した！", "You feel insubstantial!"));

		msg_print(NULL);
		set_wraith_form(creature_ptr, randint1(creature_ptr->lev / 2) + (creature_ptr->lev / 2), FALSE);
	}

	if ((creature_ptr->muta2 & MUT2_POLY_WOUND) && one_in_(3000))
	{
		do_poly_wounds(creature_ptr);
	}

	if ((creature_ptr->muta2 & MUT2_WASTING) && one_in_(3000))
	{
		int which_stat = randint0(A_MAX);
		int sustained = FALSE;

		switch (which_stat)
		{
		case A_STR:
			if (creature_ptr->sustain_str) sustained = TRUE;
			break;
		case A_INT:
			if (creature_ptr->sustain_int) sustained = TRUE;
			break;
		case A_WIS:
			if (creature_ptr->sustain_wis) sustained = TRUE;
			break;
		case A_DEX:
			if (creature_ptr->sustain_dex) sustained = TRUE;
			break;
		case A_CON:
			if (creature_ptr->sustain_con) sustained = TRUE;
			break;
		case A_CHR:
			if (creature_ptr->sustain_chr) sustained = TRUE;
			break;
		default:
			msg_print(_("不正な状態！", "Invalid stat chosen!"));
			sustained = TRUE;
		}

		if (!sustained)
		{
			disturb(creature_ptr, FALSE, TRUE);
			msg_print(_("自分が衰弱していくのが分かる！", "You can feel yourself wasting away!"));
			msg_print(NULL);
			(void)dec_stat(creature_ptr, which_stat, randint1(6) + 6, one_in_(3));
		}
	}

	if ((creature_ptr->muta2 & MUT2_ATT_DRAGON) && !creature_ptr->anti_magic && one_in_(3000))
	{
		bool pet = one_in_(5);
		BIT_FLAGS mode = PM_ALLOW_GROUP;

		if (pet) mode |= PM_FORCE_PET;
		else mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);

		if (summon_specific(creature_ptr, (pet ? -1 : 0), creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_DRAGON, mode))
		{
			msg_print(_("ドラゴンを引き寄せた！", "You have attracted a dragon!"));
			disturb(creature_ptr, FALSE, TRUE);
		}
	}

	if ((creature_ptr->muta2 & MUT2_WEIRD_MIND) && !creature_ptr->anti_magic && one_in_(3000))
	{
		if (creature_ptr->tim_esp > 0)
		{
			msg_print(_("精神にもやがかかった！", "Your mind feels cloudy!"));
			set_tim_esp(creature_ptr, 0, TRUE);
		}
		else
		{
			msg_print(_("精神が広がった！", "Your mind expands!"));
			set_tim_esp(creature_ptr, creature_ptr->lev, FALSE);
		}
	}

	if ((creature_ptr->muta2 & MUT2_NAUSEA) && !creature_ptr->slow_digest && one_in_(9000))
	{
		disturb(creature_ptr, FALSE, TRUE);
		msg_print(_("胃が痙攣し、食事を失った！", "Your stomach roils, and you lose your lunch!"));
		msg_print(NULL);
		set_food(creature_ptr, PY_FOOD_WEAK);
		if (music_singing_any(creature_ptr)) stop_singing(creature_ptr);
		if (hex_spelling_any(creature_ptr)) stop_hex_spell_all(creature_ptr);
	}

	if ((creature_ptr->muta2 & MUT2_WALK_SHAD) && !creature_ptr->anti_magic && one_in_(12000) && !creature_ptr->current_floor_ptr->inside_arena)
	{
		reserve_alter_reality(creature_ptr);
	}

	if ((creature_ptr->muta2 & MUT2_WARNING) && one_in_(1000))
	{
		int danger_amount = 0;
		for (MONSTER_IDX monster = 0; monster < creature_ptr->current_floor_ptr->m_max; monster++)
		{
			monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[monster];
			monster_race *r_ptr = &r_info[m_ptr->r_idx];
			if (!monster_is_valid(m_ptr)) continue;

			if (r_ptr->level >= creature_ptr->lev)
			{
				danger_amount += r_ptr->level - creature_ptr->lev + 1;
			}
		}

		if (danger_amount > 100)
			msg_print(_("非常に恐ろしい気がする！", "You feel utterly terrified!"));
		else if (danger_amount > 50)
			msg_print(_("恐ろしい気がする！", "You feel terrified!"));
		else if (danger_amount > 20)
			msg_print(_("非常に心配な気がする！", "You feel very worried!"));
		else if (danger_amount > 10)
			msg_print(_("心配な気がする！", "You feel paranoid!"));
		else if (danger_amount > 5)
			msg_print(_("ほとんど安全な気がする。", "You feel almost safe."));
		else
			msg_print(_("寂しい気がする。", "You feel lonely."));
	}

	if ((creature_ptr->muta2 & MUT2_INVULN) && !creature_ptr->anti_magic && one_in_(5000))
	{
		disturb(creature_ptr, FALSE, TRUE);
		msg_print(_("無敵な気がする！", "You feel invincible!"));
		msg_print(NULL);
		(void)set_invuln(creature_ptr, randint1(8) + 8, FALSE);
	}

	if ((creature_ptr->muta2 & MUT2_SP_TO_HP) && one_in_(2000))
	{
		MANA_POINT wounds = (MANA_POINT)(creature_ptr->mhp - creature_ptr->chp);

		if (wounds > 0)
		{
			HIT_POINT healing = creature_ptr->csp;
			if (healing > wounds) healing = wounds;

			hp_player(creature_ptr, healing);
			creature_ptr->csp -= healing;
			creature_ptr->redraw |= (PR_HP | PR_MANA);
		}
	}

	if ((creature_ptr->muta2 & MUT2_HP_TO_SP) && !creature_ptr->anti_magic && one_in_(4000))
	{
		HIT_POINT wounds = (HIT_POINT)(creature_ptr->msp - creature_ptr->csp);

		if (wounds > 0)
		{
			HIT_POINT healing = creature_ptr->chp;
			if (healing > wounds) healing = wounds;

			creature_ptr->csp += healing;
			creature_ptr->redraw |= (PR_HP | PR_MANA);
			take_hit(creature_ptr, DAMAGE_LOSELIFE, healing, _("頭に昇った血", "blood rushing to the head"), -1);
		}
	}

	if ((creature_ptr->muta2 & MUT2_DISARM) && one_in_(10000))
	{
		disturb(creature_ptr, FALSE, TRUE);
		msg_print(_("足がもつれて転んだ！", "You trip over your own feet!"));
		take_hit(creature_ptr, DAMAGE_NOESCAPE, randint1(creature_ptr->wt / 6), _("転倒", "tripping"), -1);
		drop_weapons(creature_ptr);
	}
}


/*!
 * @brief 10ゲームターンが進行するごとに装備効果の発動判定を行う処理
 * / Handle curse effects once every 10 game turns
 * @return なし
 */
static void process_world_aux_curse(player_type *creature_ptr)
{
	if ((creature_ptr->cursed & TRC_P_FLAG_MASK) && !creature_ptr->phase_out && !creature_ptr->wild_mode)
	{
		/*
		 * Hack: Uncursed teleporting items (e.g. Trump Weapons)
		 * can actually be useful!
		 */
		if ((creature_ptr->cursed & TRC_TELEPORT_SELF) && one_in_(200))
		{
			GAME_TEXT o_name[MAX_NLEN];
			object_type *o_ptr;
			int i_keep = 0, count = 0;
			for (int i = INVEN_RARM; i < INVEN_TOTAL; i++)
			{
				BIT_FLAGS flgs[TR_FLAG_SIZE];
				o_ptr = &creature_ptr->inventory_list[i];
				if (!o_ptr->k_idx) continue;

				object_flags(o_ptr, flgs);

				if (have_flag(flgs, TR_TELEPORT))
				{
					/* {.} will stop random teleportation. */
					if (!o_ptr->inscription || !my_strchr(quark_str(o_ptr->inscription), '.'))
					{
						count++;
						if (one_in_(count)) i_keep = i;
					}
				}
			}

			o_ptr = &creature_ptr->inventory_list[i_keep];
			object_desc(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
			msg_format(_("%sがテレポートの能力を発動させようとしている。", "Your %s is activating teleportation."), o_name);
			if (get_check_strict(_("テレポートしますか？", "Teleport? "), CHECK_OKAY_CANCEL))
			{
				disturb(creature_ptr, FALSE, TRUE);
				teleport_player(creature_ptr, 50, TELEPORT_SPONTANEOUS);
			}
			else
			{
				msg_format(_("%sに{.}(ピリオド)と銘を刻むと発動を抑制できます。",
					"You can inscribe {.} on your %s to disable random teleportation. "), o_name);
				disturb(creature_ptr, TRUE, TRUE);
			}
		}

		if ((creature_ptr->cursed & TRC_CHAINSWORD) && one_in_(CHAINSWORD_NOISE))
		{
			char noise[1024];
			if (!get_rnd_line(_("chainswd_j.txt", "chainswd.txt"), 0, noise))
				msg_print(noise);
			disturb(creature_ptr, FALSE, FALSE);
		}

		if ((creature_ptr->cursed & TRC_TY_CURSE) && one_in_(TY_CURSE_CHANCE))
		{
			int count = 0;
			(void)activate_ty_curse(creature_ptr, FALSE, &count);
		}

		if (creature_ptr->prace != RACE_ANDROID && ((creature_ptr->cursed & TRC_DRAIN_EXP) && one_in_(4)))
		{
			creature_ptr->exp -= (creature_ptr->lev + 1) / 2;
			if (creature_ptr->exp < 0) creature_ptr->exp = 0;
			creature_ptr->max_exp -= (creature_ptr->lev + 1) / 2;
			if (creature_ptr->max_exp < 0) creature_ptr->max_exp = 0;
			check_experience(creature_ptr);
		}

		if ((creature_ptr->cursed & TRC_ADD_L_CURSE) && one_in_(2000))
		{
			object_type *o_ptr;
			o_ptr = choose_cursed_obj_name(creature_ptr, TRC_ADD_L_CURSE);
			BIT_FLAGS new_curse = get_curse(0, o_ptr);
			if (!(o_ptr->curse_flags & new_curse))
			{
				GAME_TEXT o_name[MAX_NLEN];
				object_desc(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
				o_ptr->curse_flags |= new_curse;
				msg_format(_("悪意に満ちた黒いオーラが%sをとりまいた...", "There is a malignant black aura surrounding your %s..."), o_name);
				o_ptr->feeling = FEEL_NONE;
				creature_ptr->update |= (PU_BONUS);
			}
		}

		if ((creature_ptr->cursed & TRC_ADD_H_CURSE) && one_in_(2000))
		{
			object_type *o_ptr;
			o_ptr = choose_cursed_obj_name(creature_ptr, TRC_ADD_H_CURSE);
			BIT_FLAGS new_curse = get_curse(1, o_ptr);
			if (!(o_ptr->curse_flags & new_curse))
			{
				GAME_TEXT o_name[MAX_NLEN];

				object_desc(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

				o_ptr->curse_flags |= new_curse;
				msg_format(_("悪意に満ちた黒いオーラが%sをとりまいた...", "There is a malignant black aura surrounding your %s..."), o_name);
				o_ptr->feeling = FEEL_NONE;

				creature_ptr->update |= (PU_BONUS);
			}
		}

		if ((creature_ptr->cursed & TRC_CALL_ANIMAL) && one_in_(2500))
		{
			if (summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_ANIMAL, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)))
			{
				GAME_TEXT o_name[MAX_NLEN];
				object_desc(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC_CALL_ANIMAL), (OD_OMIT_PREFIX | OD_NAME_ONLY));
				msg_format(_("%sが動物を引き寄せた！", "Your %s has attracted an animal!"), o_name);
				disturb(creature_ptr, FALSE, TRUE);
			}
		}

		if ((creature_ptr->cursed & TRC_CALL_DEMON) && one_in_(1111))
		{
			if (summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_DEMON, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)))
			{
				GAME_TEXT o_name[MAX_NLEN];
				object_desc(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC_CALL_DEMON), (OD_OMIT_PREFIX | OD_NAME_ONLY));
				msg_format(_("%sが悪魔を引き寄せた！", "Your %s has attracted a demon!"), o_name);
				disturb(creature_ptr, FALSE, TRUE);
			}
		}

		if ((creature_ptr->cursed & TRC_CALL_DRAGON) && one_in_(800))
		{
			if (summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_DRAGON,
				(PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)))
			{
				GAME_TEXT o_name[MAX_NLEN];
				object_desc(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC_CALL_DRAGON), (OD_OMIT_PREFIX | OD_NAME_ONLY));
				msg_format(_("%sがドラゴンを引き寄せた！", "Your %s has attracted an dragon!"), o_name);
				disturb(creature_ptr, FALSE, TRUE);
			}
		}

		if ((creature_ptr->cursed & TRC_CALL_UNDEAD) && one_in_(1111))
		{
			if (summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_UNDEAD,
				(PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)))
			{
				GAME_TEXT o_name[MAX_NLEN];
				object_desc(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC_CALL_UNDEAD), (OD_OMIT_PREFIX | OD_NAME_ONLY));
				msg_format(_("%sが死霊を引き寄せた！", "Your %s has attracted an undead!"), o_name);
				disturb(creature_ptr, FALSE, TRUE);
			}
		}

		if ((creature_ptr->cursed & TRC_COWARDICE) && one_in_(1500))
		{
			if (!creature_ptr->resist_fear)
			{
				disturb(creature_ptr, FALSE, TRUE);
				msg_print(_("とても暗い... とても恐い！", "It's so dark... so scary!"));
				set_afraid(creature_ptr, creature_ptr->afraid + 13 + randint1(26));
			}
		}

		if ((creature_ptr->cursed & TRC_TELEPORT) && one_in_(200) && !creature_ptr->anti_tele)
		{
			disturb(creature_ptr, FALSE, TRUE);
			teleport_player(creature_ptr, 40, TELEPORT_PASSIVE);
		}

		if ((creature_ptr->cursed & TRC_DRAIN_HP) && one_in_(666))
		{
			GAME_TEXT o_name[MAX_NLEN];
			object_desc(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC_DRAIN_HP), (OD_OMIT_PREFIX | OD_NAME_ONLY));
			msg_format(_("%sはあなたの体力を吸収した！", "Your %s drains HP from you!"), o_name);
			take_hit(creature_ptr, DAMAGE_LOSELIFE, MIN(creature_ptr->lev * 2, 100), o_name, -1);
		}

		if ((creature_ptr->cursed & TRC_DRAIN_MANA) && creature_ptr->csp && one_in_(666))
		{
			GAME_TEXT o_name[MAX_NLEN];
			object_desc(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC_DRAIN_MANA), (OD_OMIT_PREFIX | OD_NAME_ONLY));
			msg_format(_("%sはあなたの魔力を吸収した！", "Your %s drains mana from you!"), o_name);
			creature_ptr->csp -= MIN(creature_ptr->lev, 50);
			if (creature_ptr->csp < 0)
			{
				creature_ptr->csp = 0;
				creature_ptr->csp_frac = 0;
			}

			creature_ptr->redraw |= PR_MANA;
		}
	}

	if (one_in_(999) && !creature_ptr->anti_magic)
	{
		object_type *o_ptr = &creature_ptr->inventory_list[INVEN_LITE];
		if (o_ptr->name1 == ART_JUDGE)
		{
			if (object_is_known(o_ptr))
				msg_print(_("『審判の宝石』はあなたの体力を吸収した！", "The Jewel of Judgement drains life from you!"));
			else
				msg_print(_("なにかがあなたの体力を吸収した！", "Something drains life from you!"));
			take_hit(creature_ptr, DAMAGE_LOSELIFE, MIN(creature_ptr->lev, 50), _("審判の宝石", "the Jewel of Judgement"), -1);
		}
	}
}


/*!
 * @brief 10ゲームターンが進行するごとに魔道具の自然充填を行う処理
 * / Handle recharging objects once every 10 game turns
 * @return なし
 */
static void process_world_aux_recharge(player_type *creature_ptr)
{
	int i;
	bool changed;

	for (changed = FALSE, i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr = &creature_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;

		if (o_ptr->timeout > 0)
		{
			o_ptr->timeout--;
			if (!o_ptr->timeout)
			{
				recharged_notice(creature_ptr, o_ptr);
				changed = TRUE;
			}
		}
	}

	if (changed)
	{
		creature_ptr->window |= (PW_EQUIP);
		wild_regen = 20;
	}

	/*
	 * Recharge rods.  Rods now use timeout to control charging status,
	 * and each charging rod in a stack decreases the stack's timeout by
	 * one per turn. -LM-
	 */
	for (changed = FALSE, i = 0; i < INVEN_PACK; i++)
	{
		object_type *o_ptr = &creature_ptr->inventory_list[i];
		object_kind *k_ptr = &k_info[o_ptr->k_idx];
		if (!o_ptr->k_idx) continue;

		if ((o_ptr->tval == TV_ROD) && (o_ptr->timeout))
		{
			TIME_EFFECT temp = (o_ptr->timeout + (k_ptr->pval - 1)) / k_ptr->pval;
			if (temp > o_ptr->number) temp = (TIME_EFFECT)o_ptr->number;

			o_ptr->timeout -= temp;
			if (o_ptr->timeout < 0) o_ptr->timeout = 0;

			if (!(o_ptr->timeout))
			{
				recharged_notice(creature_ptr, o_ptr);
				changed = TRUE;
			}
			else if (o_ptr->timeout % k_ptr->pval)
			{
				changed = TRUE;
			}
		}
	}

	if (changed)
	{
		creature_ptr->window |= (PW_INVEN);
		wild_regen = 20;
	}

	for (i = 1; i < creature_ptr->current_floor_ptr->o_max; i++)
	{
		object_type *o_ptr = &creature_ptr->current_floor_ptr->o_list[i];
		if (!OBJECT_IS_VALID(o_ptr)) continue;

		if ((o_ptr->tval == TV_ROD) && (o_ptr->timeout))
		{
			o_ptr->timeout -= (TIME_EFFECT)o_ptr->number;
			if (o_ptr->timeout < 0) o_ptr->timeout = 0;
		}
	}
}


/*!
 * @brief 10ゲームターンが進行するごとに帰還や現実変容などの残り時間カウントダウンと発動を処理する。
 * / Handle involuntary movement once every 10 game turns
 * @return なし
 */
static void process_world_aux_movement(player_type *creature_ptr)
{
	floor_type *floor_ptr = creature_ptr->current_floor_ptr;
	if (creature_ptr->word_recall)
	{
		/*
		 * HACK: Autosave BEFORE resetting the recall counter (rr9)
		 * The player is yanked up/down as soon as
		 * he loads the autosaved game.
		 */
		if (autosave_l && (creature_ptr->word_recall == 1) && !creature_ptr->phase_out)
			do_cmd_save_game(creature_ptr, TRUE);

		creature_ptr->word_recall--;
		creature_ptr->redraw |= (PR_STATUS);
		if (!creature_ptr->word_recall)
		{
			disturb(creature_ptr, FALSE, TRUE);
			if (floor_ptr->dun_level || creature_ptr->current_floor_ptr->inside_quest || creature_ptr->enter_dungeon)
			{
				msg_print(_("上に引っ張りあげられる感じがする！", "You feel yourself yanked upwards!"));
				if (creature_ptr->dungeon_idx) creature_ptr->recall_dungeon = creature_ptr->dungeon_idx;
				if (record_stair)
					exe_write_diary(creature_ptr, DIARY_RECALL, floor_ptr->dun_level, NULL);

				floor_ptr->dun_level = 0;
				creature_ptr->dungeon_idx = 0;
				leave_quest_check(creature_ptr);
				leave_tower_check(creature_ptr);
				creature_ptr->current_floor_ptr->inside_quest = 0;
				creature_ptr->leaving = TRUE;
			}
			else
			{
				msg_print(_("下に引きずり降ろされる感じがする！", "You feel yourself yanked downwards!"));
				creature_ptr->dungeon_idx = creature_ptr->recall_dungeon;
				if (record_stair)
					exe_write_diary(creature_ptr, DIARY_RECALL, floor_ptr->dun_level, NULL);

				floor_ptr->dun_level = max_dlv[creature_ptr->dungeon_idx];
				if (floor_ptr->dun_level < 1) floor_ptr->dun_level = 1;
				if (ironman_nightmare && !randint0(666) && (creature_ptr->dungeon_idx == DUNGEON_ANGBAND))
				{
					if (floor_ptr->dun_level < 50)
					{
						floor_ptr->dun_level *= 2;
					}
					else if (floor_ptr->dun_level < 99)
					{
						floor_ptr->dun_level = (floor_ptr->dun_level + 99) / 2;
					}
					else if (floor_ptr->dun_level > 100)
					{
						floor_ptr->dun_level = d_info[creature_ptr->dungeon_idx].maxdepth - 1;
					}
				}

				if (creature_ptr->wild_mode)
				{
					creature_ptr->wilderness_y = creature_ptr->y;
					creature_ptr->wilderness_x = creature_ptr->x;
				}
				else
				{
					creature_ptr->oldpx = creature_ptr->x;
					creature_ptr->oldpy = creature_ptr->y;
				}

				creature_ptr->wild_mode = FALSE;

				/*
				 * Clear all saved floors
				 * and create a first saved floor
				 */
				prepare_change_floor_mode(creature_ptr, CFM_FIRST_FLOOR);
				creature_ptr->leaving = TRUE;

				if (creature_ptr->dungeon_idx == DUNGEON_ANGBAND)
				{
					for (int i = MIN_RANDOM_QUEST; i < MAX_RANDOM_QUEST + 1; i++)
					{
						quest_type* const q_ptr = &quest[i];
						if ((q_ptr->type == QUEST_TYPE_RANDOM) &&
							(q_ptr->status == QUEST_STATUS_TAKEN) &&
							(q_ptr->level < floor_ptr->dun_level))
						{
							q_ptr->status = QUEST_STATUS_FAILED;
							q_ptr->complev = (byte)creature_ptr->lev;
							update_playtime();
							q_ptr->comptime = current_world_ptr->play_time;
							r_info[q_ptr->r_idx].flags1 &= ~(RF1_QUESTOR);
						}
					}
				}
			}

			sound(SOUND_TPLEVEL);
		}
	}

	if (creature_ptr->alter_reality)
	{
		if (autosave_l && (creature_ptr->alter_reality == 1) && !creature_ptr->phase_out)
			do_cmd_save_game(creature_ptr, TRUE);

		creature_ptr->alter_reality--;
		creature_ptr->redraw |= (PR_STATUS);
		if (!creature_ptr->alter_reality)
		{
			disturb(creature_ptr, FALSE, TRUE);
			if (!quest_number(creature_ptr, floor_ptr->dun_level) && floor_ptr->dun_level)
			{
				msg_print(_("世界が変わった！", "The world changes!"));

				/*
				 * Clear all saved floors
				 * and create a first saved floor
				 */
				prepare_change_floor_mode(creature_ptr, CFM_FIRST_FLOOR);
				creature_ptr->leaving = TRUE;
			}
			else
			{
				msg_print(_("世界が少しの間変化したようだ。", "The world seems to change for a moment!"));
			}

			sound(SOUND_TPLEVEL);
		}
	}
}


/*!
 * @brief 10ゲームターンが進行する毎にゲーム世界全体の処理を行う。
 * / Handle certain things once every 10 game turns
 * @return なし
 */
static void process_world(player_type *player_ptr)
{
	const s32b A_DAY = TURNS_PER_TICK * TOWN_DAWN;
	s32b prev_turn_in_today = ((current_world_ptr->game_turn - TURNS_PER_TICK) % A_DAY + A_DAY / 4) % A_DAY;
	int prev_min = (1440 * prev_turn_in_today / A_DAY) % 60;

	int day, hour, min;
	extract_day_hour_min(player_ptr, &day, &hour, &min);
	update_dungeon_feeling(player_ptr);

	/* 帰還無しモード時のレベルテレポバグ対策 / Fix for level teleport bugs on ironman_downward.*/
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	if (ironman_downward && (player_ptr->dungeon_idx != DUNGEON_ANGBAND && player_ptr->dungeon_idx != 0))
	{
		floor_ptr->dun_level = 0;
		player_ptr->dungeon_idx = 0;
		prepare_change_floor_mode(player_ptr, CFM_FIRST_FLOOR | CFM_RAND_PLACE);
		floor_ptr->inside_arena = FALSE;
		player_ptr->wild_mode = FALSE;
		player_ptr->leaving = TRUE;
	}

	if (player_ptr->phase_out && !player_ptr->leaving)
	{
		int win_m_idx = 0;
		int number_mon = 0;
		for (int i2 = 0; i2 < floor_ptr->width; ++i2)
		{
			for (int j2 = 0; j2 < floor_ptr->height; j2++)
			{
				grid_type *g_ptr = &floor_ptr->grid_array[j2][i2];
				if ((g_ptr->m_idx > 0) && (g_ptr->m_idx != player_ptr->riding))
				{
					number_mon++;
					win_m_idx = g_ptr->m_idx;
				}
			}
		}

		if (number_mon == 0)
		{
			msg_print(_("相打ちに終わりました。", "Nothing survived."));
			msg_print(NULL);
			player_ptr->energy_need = 0;
			update_gambling_monsters(player_ptr);
		}
		else if ((number_mon - 1) == 0)
		{
			GAME_TEXT m_name[MAX_NLEN];
			monster_type *wm_ptr;
			wm_ptr = &floor_ptr->m_list[win_m_idx];
			monster_desc(player_ptr, m_name, wm_ptr, 0);
			msg_format(_("%sが勝利した！", "%s won!"), m_name);
			msg_print(NULL);

			if (win_m_idx == (sel_monster + 1))
			{
				msg_print(_("おめでとうございます。", "Congratulations."));
				msg_format(_("%d＄を受け取った。", "You received %d gold."), battle_odds);
				player_ptr->au += battle_odds;
			}
			else
			{
				msg_print(_("残念でした。", "You lost gold."));
			}

			msg_print(NULL);
			player_ptr->energy_need = 0;
			update_gambling_monsters(player_ptr);
		}
		else if (current_world_ptr->game_turn - floor_ptr->generated_turn == 150 * TURNS_PER_TICK)
		{
			msg_print(_("申し分けありませんが、この勝負は引き分けとさせていただきます。", "This battle ended in a draw."));
			player_ptr->au += kakekin;
			msg_print(NULL);
			player_ptr->energy_need = 0;
			update_gambling_monsters(player_ptr);
		}
	}

	if (current_world_ptr->game_turn % TURNS_PER_TICK) return;

	if (autosave_t && autosave_freq && !player_ptr->phase_out)
	{
		if (!(current_world_ptr->game_turn % ((s32b)autosave_freq * TURNS_PER_TICK)))
			do_cmd_save_game(player_ptr, TRUE);
	}

	if (floor_ptr->monster_noise && !ignore_unview)
	{
		msg_print(_("何かが聞こえた。", "You hear noise."));
	}

	if (!floor_ptr->dun_level && !floor_ptr->inside_quest && !player_ptr->phase_out && !floor_ptr->inside_arena)
	{
		if (!(current_world_ptr->game_turn % ((TURNS_PER_TICK * TOWN_DAWN) / 2)))
		{
			bool dawn = (!(current_world_ptr->game_turn % (TURNS_PER_TICK * TOWN_DAWN)));
			if (dawn) day_break(player_ptr);
			else night_falls(player_ptr);

		}
	}
	else if ((vanilla_town || (lite_town && !floor_ptr->inside_quest && !player_ptr->phase_out && !floor_ptr->inside_arena)) && floor_ptr->dun_level)
	{
		if (!(current_world_ptr->game_turn % (TURNS_PER_TICK * STORE_TICKS)))
		{
			if (one_in_(STORE_SHUFFLE))
			{
				int n;
				do
				{
					n = randint0(MAX_STORES);
				} while ((n == STORE_HOME) || (n == STORE_MUSEUM));

				for (FEAT_IDX i = 1; i < max_f_idx; i++)
				{
					feature_type *f_ptr = &f_info[i];
					if (!f_ptr->name) continue;
					if (!have_flag(f_ptr->flags, FF_STORE)) continue;

					if (f_ptr->subtype == n)
					{
						if (cheat_xtra)
							msg_format(_("%sの店主をシャッフルします。", "Shuffle a Shopkeeper of %s."), f_name + f_ptr->name);

						store_shuffle(player_ptr, n);
						break;
					}
				}
			}
		}
	}

	if (one_in_(d_info[player_ptr->dungeon_idx].max_m_alloc_chance) &&
		!floor_ptr->inside_arena && !floor_ptr->inside_quest && !player_ptr->phase_out)
	{
		(void)alloc_monster(player_ptr, MAX_SIGHT + 5, 0);
	}

	if (!(current_world_ptr->game_turn % (TURNS_PER_TICK * 10)) && !player_ptr->phase_out)
		regenerate_monsters(player_ptr);
	if (!(current_world_ptr->game_turn % (TURNS_PER_TICK * 3)))
		regenerate_captured_monsters(player_ptr);

	if (!player_ptr->leaving)
	{
		for (int i = 0; i < MAX_MTIMED; i++)
		{
			if (floor_ptr->mproc_max[i] > 0) process_monsters_mtimed(player_ptr, i);
		}
	}

	if (!hour && !min)
	{
		if (min != prev_min)
		{
			exe_write_diary(player_ptr, DIARY_DIALY, 0, NULL);
			determine_daily_bounty(player_ptr, FALSE);
		}
	}

	/*
	 * Nightmare mode activates the TY_CURSE at midnight
	 * Require exact minute -- Don't activate multiple times in a minute
	 */
	if (ironman_nightmare && (min != prev_min))
	{
		if ((hour == 23) && !(min % 15))
		{
			disturb(player_ptr, FALSE, TRUE);
			switch (min / 15)
			{
			case 0:
				msg_print(_("遠くで不気味な鐘の音が鳴った。", "You hear a distant bell toll ominously."));
				break;

			case 1:
				msg_print(_("遠くで鐘が二回鳴った。", "A distant bell sounds twice."));
				break;

			case 2:
				msg_print(_("遠くで鐘が三回鳴った。", "A distant bell sounds three times."));
				break;

			case 3:
				msg_print(_("遠くで鐘が四回鳴った。", "A distant bell tolls four times."));
				break;
			}
		}

		if (!hour && !min)
		{
			disturb(player_ptr, TRUE, TRUE);
			msg_print(_("遠くで鐘が何回も鳴り、死んだような静けさの中へ消えていった。", "A distant bell tolls many times, fading into an deathly silence."));
			if (player_ptr->wild_mode)
			{
				player_ptr->oldpy = randint1(MAX_HGT - 2);
				player_ptr->oldpx = randint1(MAX_WID - 2);
				change_wild_mode(player_ptr, TRUE);
				take_turn(player_ptr, 100);

			}

			player_ptr->invoking_midnight_curse = TRUE;
		}
	}

	process_world_aux_digestion(player_ptr);
	process_world_aux_hp_and_sp(player_ptr);
	process_world_aux_timeout(player_ptr);
	process_world_aux_light(player_ptr);
	process_world_aux_mutation(player_ptr);
	process_world_aux_curse(player_ptr);
	process_world_aux_recharge(player_ptr);
	sense_inventory1(player_ptr);
	sense_inventory2(player_ptr);
	process_world_aux_movement(player_ptr);
}


/*!
 * @brief ウィザードモードへの導入処理
 * / Verify use of "wizard" mode
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 実際にウィザードモードへ移行したらTRUEを返す。
 */
static bool enter_wizard_mode(player_type *player_ptr)
{
	if (!current_world_ptr->noscore)
	{
		if (!allow_debug_opts || arg_wizard)
		{
			msg_print(_("ウィザードモードは許可されていません。 ", "Wizard mode is not permitted."));
			return FALSE;
		}

		msg_print(_("ウィザードモードはデバッグと実験のためのモードです。 ", "Wizard mode is for debugging and experimenting."));
		msg_print(_("一度ウィザードモードに入るとスコアは記録されません。", "The game will not be scored if you enter wizard mode."));
		msg_print(NULL);
		if (!get_check(_("本当にウィザードモードに入りたいのですか? ", "Are you sure you want to enter wizard mode? ")))
		{
			return FALSE;
		}

		exe_write_diary(player_ptr, DIARY_DESCRIPTION, 0, _("ウィザードモードに突入してスコアを残せなくなった。", "gave up recording score to enter wizard mode."));
		current_world_ptr->noscore |= 0x0002;
	}

	return TRUE;
}


/*!
 * @brief デバッグコマンドへの導入処理
 * / Verify use of "debug" commands
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 実際にデバッグコマンドへ移行したらTRUEを返す。
 */
static bool enter_debug_mode(player_type *player_ptr)
{
	if (!current_world_ptr->noscore)
	{
		if (!allow_debug_opts)
		{
			msg_print(_("デバッグコマンドは許可されていません。 ", "Use of debug command is not permitted."));
			return FALSE;
		}

		msg_print(_("デバッグ・コマンドはデバッグと実験のためのコマンドです。 ", "The debug commands are for debugging and experimenting."));
		msg_print(_("デバッグ・コマンドを使うとスコアは記録されません。", "The game will not be scored if you use debug commands."));
		msg_print(NULL);
		if (!get_check(_("本当にデバッグ・コマンドを使いますか? ", "Are you sure you want to use debug commands? ")))
		{
			return FALSE;
		}

		exe_write_diary(player_ptr, DIARY_DESCRIPTION, 0, _("デバッグモードに突入してスコアを残せなくなった。", "gave up sending score to use debug commands."));
		current_world_ptr->noscore |= 0x0008;
	}

	return TRUE;
}

/*
 * todo これが多重インクルード問題の原因 (の1つ)かもしれない、wizard2.cに同名の関数が存在する
 * Hack -- Declare the Debug Routines
 */
extern void do_cmd_debug(player_type *creature_ptr);

/*!
 * @brief プレイヤーから受けた入力コマンドの分岐処理。
 * / Parse and execute the current command Give "Warning" on illegal commands.
 * @todo Make some "blocks"
 * @return なし
 */
static void process_command(player_type *creature_ptr)
{
	COMMAND_CODE old_now_message = now_message;
	repeat_check();
	now_message = 0;
	if ((creature_ptr->pclass == CLASS_SNIPER) && (creature_ptr->concent))
		creature_ptr->reset_concent = TRUE;

	floor_type *floor_ptr = creature_ptr->current_floor_ptr;
	switch (command_cmd)
	{
	case ESCAPE:
	case ' ':
	{
		/* Ignore */
		break;
	}
	case '\r':
	case '\n':
	{
		/* todo 嘘。returnしていない
		 * Ignore return
		 */
		break;
	}
	case KTRL('W'):
	{
		if (current_world_ptr->wizard)
		{
			current_world_ptr->wizard = FALSE;
			msg_print(_("ウィザードモード解除。", "Wizard mode off."));
		}
		else if (enter_wizard_mode(creature_ptr))
		{
			current_world_ptr->wizard = TRUE;
			msg_print(_("ウィザードモード突入。", "Wizard mode on."));
		}
		creature_ptr->update |= (PU_MONSTERS);
		creature_ptr->redraw |= (PR_TITLE);

		break;
	}
	case KTRL('A'):
	{
		if (enter_debug_mode(creature_ptr))
		{
			do_cmd_debug(creature_ptr);
		}
		break;
	}
	case 'w':
	{
		if (!creature_ptr->wild_mode) do_cmd_wield(creature_ptr);
		break;
	}
	case 't':
	{
		if (!creature_ptr->wild_mode) do_cmd_takeoff(creature_ptr);
		break;
	}
	case 'd':
	{
		if (!creature_ptr->wild_mode) do_cmd_drop(creature_ptr);
		break;
	}
	case 'k':
	{
		do_cmd_destroy(creature_ptr);
		break;
	}
	case 'e':
	{
		do_cmd_equip(creature_ptr);
		break;
	}
	case 'i':
	{
		do_cmd_inven(creature_ptr);
		break;
	}
	case 'I':
	{
		do_cmd_observe(creature_ptr);
		break;
	}

	case KTRL('I'):
	{
		toggle_inventory_equipment(creature_ptr);
		break;
	}
	case '+':
	{
		if (!creature_ptr->wild_mode) do_cmd_alter(creature_ptr);
		break;
	}
	case 'T':
	{
		if (!creature_ptr->wild_mode) do_cmd_tunnel(creature_ptr);
		break;
	}
	case ';':
	{
		do_cmd_walk(creature_ptr, FALSE);
		break;
	}
	case '-':
	{
		do_cmd_walk(creature_ptr, TRUE);
		break;
	}
	case '.':
	{
		if (!creature_ptr->wild_mode) do_cmd_run(creature_ptr);
		break;
	}
	case ',':
	{
		do_cmd_stay(creature_ptr, always_pickup);
		break;
	}
	case 'g':
	{
		do_cmd_stay(creature_ptr, !always_pickup);
		break;
	}
	case 'R':
	{
		do_cmd_rest(creature_ptr);
		break;
	}
	case 's':
	{
		do_cmd_search(creature_ptr);
		break;
	}
	case 'S':
	{
		if (creature_ptr->action == ACTION_SEARCH) set_action(creature_ptr, ACTION_NONE);
		else set_action(creature_ptr, ACTION_SEARCH);
		break;
	}
	case SPECIAL_KEY_STORE:
	{
		do_cmd_store(creature_ptr);
		break;
	}
	case SPECIAL_KEY_BUILDING:
	{
		do_cmd_bldg(creature_ptr);
		break;
	}
	case SPECIAL_KEY_QUEST:
	{
		do_cmd_quest(creature_ptr);
		break;
	}
	case '<':
	{
		if (!creature_ptr->wild_mode && !floor_ptr->dun_level && !floor_ptr->inside_arena && !floor_ptr->inside_quest)
		{
			if (vanilla_town) break;

			if (creature_ptr->ambush_flag)
			{
				msg_print(_("襲撃から逃げるにはマップの端まで移動しなければならない。", "To flee the ambush you have to reach the edge of the map."));
				break;
			}

			if (creature_ptr->food < PY_FOOD_WEAK)
			{
				msg_print(_("その前に食事をとらないと。", "You must eat something here."));
				break;
			}

			change_wild_mode(creature_ptr, FALSE);
		}
		else
			do_cmd_go_up(creature_ptr);

		break;
	}
	case '>':
	{
		if (creature_ptr->wild_mode)
			change_wild_mode(creature_ptr, FALSE);
		else
			do_cmd_go_down(creature_ptr);
		break;
	}
	case 'o':
	{
		do_cmd_open(creature_ptr);
		break;
	}
	case 'c':
	{
		do_cmd_close(creature_ptr);
		break;
	}
	case 'j':
	{
		do_cmd_spike(creature_ptr);
		break;
	}
	case 'B':
	{
		do_cmd_bash(creature_ptr);
		break;
	}
	case 'D':
	{
		do_cmd_disarm(creature_ptr);
		break;
	}
	case 'G':
	{
		if ((creature_ptr->pclass == CLASS_SORCERER) || (creature_ptr->pclass == CLASS_RED_MAGE))
			msg_print(_("呪文を学習する必要はない！", "You don't have to learn spells!"));
		else if (creature_ptr->pclass == CLASS_SAMURAI)
			do_cmd_gain_hissatsu(creature_ptr);
		else if (creature_ptr->pclass == CLASS_MAGIC_EATER)
			import_magic_device(creature_ptr);
		else
			do_cmd_study(creature_ptr);
		break;
	}
	case 'b':
	{
		if ((creature_ptr->pclass == CLASS_MINDCRAFTER) ||
			(creature_ptr->pclass == CLASS_BERSERKER) ||
			(creature_ptr->pclass == CLASS_NINJA) ||
			(creature_ptr->pclass == CLASS_MIRROR_MASTER)
			) do_cmd_mind_browse(creature_ptr);
		else if (creature_ptr->pclass == CLASS_SMITH)
			do_cmd_kaji(creature_ptr, TRUE);
		else if (creature_ptr->pclass == CLASS_MAGIC_EATER)
			do_cmd_magic_eater(creature_ptr, TRUE, FALSE);
		else if (creature_ptr->pclass == CLASS_SNIPER)
			do_cmd_snipe_browse(creature_ptr);
		else do_cmd_browse(creature_ptr);
		break;
	}
	case 'm':
	{
		if (!creature_ptr->wild_mode)
		{
			if ((creature_ptr->pclass == CLASS_WARRIOR) || (creature_ptr->pclass == CLASS_ARCHER) || (creature_ptr->pclass == CLASS_CAVALRY))
			{
				msg_print(_("呪文を唱えられない！", "You cannot cast spells!"));
			}
			else if (floor_ptr->dun_level && (d_info[creature_ptr->dungeon_idx].flags1 & DF1_NO_MAGIC) && (creature_ptr->pclass != CLASS_BERSERKER) && (creature_ptr->pclass != CLASS_SMITH))
			{
				msg_print(_("ダンジョンが魔法を吸収した！", "The dungeon absorbs all attempted magic!"));
				msg_print(NULL);
			}
			else if (creature_ptr->anti_magic && (creature_ptr->pclass != CLASS_BERSERKER) && (creature_ptr->pclass != CLASS_SMITH))
			{
				concptr which_power = _("魔法", "magic");
				if (creature_ptr->pclass == CLASS_MINDCRAFTER)
					which_power = _("超能力", "psionic powers");
				else if (creature_ptr->pclass == CLASS_IMITATOR)
					which_power = _("ものまね", "imitation");
				else if (creature_ptr->pclass == CLASS_SAMURAI)
					which_power = _("必殺剣", "hissatsu");
				else if (creature_ptr->pclass == CLASS_MIRROR_MASTER)
					which_power = _("鏡魔法", "mirror magic");
				else if (creature_ptr->pclass == CLASS_NINJA)
					which_power = _("忍術", "ninjutsu");
				else if (mp_ptr->spell_book == TV_LIFE_BOOK)
					which_power = _("祈り", "prayer");

				msg_format(_("反魔法バリアが%sを邪魔した！", "An anti-magic shell disrupts your %s!"), which_power);
				free_turn(creature_ptr);
			}
			else if (creature_ptr->shero && (creature_ptr->pclass != CLASS_BERSERKER))
			{
				msg_format(_("狂戦士化していて頭が回らない！", "You cannot think directly!"));
				free_turn(creature_ptr);
			}
			else
			{
				if ((creature_ptr->pclass == CLASS_MINDCRAFTER) ||
					(creature_ptr->pclass == CLASS_BERSERKER) ||
					(creature_ptr->pclass == CLASS_NINJA) ||
					(creature_ptr->pclass == CLASS_MIRROR_MASTER)
					)
					do_cmd_mind(creature_ptr);
				else if (creature_ptr->pclass == CLASS_IMITATOR)
					do_cmd_mane(creature_ptr, FALSE);
				else if (creature_ptr->pclass == CLASS_MAGIC_EATER)
					do_cmd_magic_eater(creature_ptr, FALSE, FALSE);
				else if (creature_ptr->pclass == CLASS_SAMURAI)
					do_cmd_hissatsu(creature_ptr);
				else if (creature_ptr->pclass == CLASS_BLUE_MAGE)
					do_cmd_cast_learned(creature_ptr);
				else if (creature_ptr->pclass == CLASS_SMITH)
					do_cmd_kaji(creature_ptr, FALSE);
				else if (creature_ptr->pclass == CLASS_SNIPER)
					do_cmd_snipe(creature_ptr);
				else
					do_cmd_cast(creature_ptr);
			}
		}

		break;
	}
	case 'p':
	{
		do_cmd_pet(creature_ptr);
		break;
	}
	case '{':
	{
		do_cmd_inscribe(creature_ptr);
		break;
	}
	case '}':
	{
		do_cmd_uninscribe(creature_ptr);
		break;
	}
	case 'A':
	{
		do_cmd_activate(creature_ptr);
		break;
	}
	case 'E':
	{
		do_cmd_eat_food(creature_ptr);
		break;
	}
	case 'F':
	{
		do_cmd_refill(creature_ptr);
		break;
	}
	case 'f':
	{
		do_cmd_fire(creature_ptr, SP_NONE);
		break;
	}
	case 'v':
	{
		do_cmd_throw(creature_ptr, 1, FALSE, -1);
		break;
	}
	case 'a':
	{
		do_cmd_aim_wand(creature_ptr);
		break;
	}
	case 'z':
	{
		if (use_command && rogue_like_commands)
		{
			do_cmd_use(creature_ptr);
		}
		else
		{
			do_cmd_zap_rod(creature_ptr);
		}
		break;
	}
	case 'q':
	{
		do_cmd_quaff_potion(creature_ptr);
		break;
	}
	case 'r':
	{
		do_cmd_read_scroll(creature_ptr);
		break;
	}
	case 'u':
	{
		if (use_command && !rogue_like_commands)
			do_cmd_use(creature_ptr);
		else
			do_cmd_use_staff(creature_ptr);
		break;
	}
	case 'U':
	{
		do_cmd_racial_power(creature_ptr);
		break;
	}
	case 'M':
	{
		do_cmd_view_map(creature_ptr);
		break;
	}
	case 'L':
	{
		do_cmd_locate(creature_ptr);
		break;
	}
	case 'l':
	{
		do_cmd_look(creature_ptr);
		break;
	}
	case '*':
	{
		do_cmd_target(creature_ptr);
		break;
	}
	case '?':
	{
		do_cmd_help(creature_ptr);
		break;
	}
	case '/':
	{
		do_cmd_query_symbol(creature_ptr);
		break;
	}
	case 'C':
	{
		do_cmd_player_status(creature_ptr);
		break;
	}
	case '!':
	{
		(void)Term_user(0);
		break;
	}
	case '"':
	{
		do_cmd_pref(creature_ptr);
		break;
	}
	case '$':
	{
		do_cmd_reload_autopick(creature_ptr);
		break;
	}
	case '_':
	{
		do_cmd_edit_autopick(creature_ptr);
		break;
	}
	case '@':
	{
		do_cmd_macros(creature_ptr, process_autopick_file_command);
		break;
	}
	case '%':
	{
		do_cmd_visuals(creature_ptr, process_autopick_file_command);
		do_cmd_redraw(creature_ptr);
		break;
	}
	case '&':
	{
		do_cmd_colors(creature_ptr, process_autopick_file_command);
		do_cmd_redraw(creature_ptr);
		break;
	}
	case '=':
	{
		do_cmd_options();
		(void)combine_and_reorder_home(STORE_HOME);
		do_cmd_redraw(creature_ptr);
		break;
	}
	case ':':
	{
		do_cmd_note();
		break;
	}
	case 'V':
	{
		do_cmd_version();
		break;
	}
	case KTRL('F'):
	{
		do_cmd_feeling(creature_ptr);
		break;
	}
	case KTRL('O'):
	{
		do_cmd_message_one();
		break;
	}
	case KTRL('P'):
	{
		do_cmd_messages(old_now_message);
		break;
	}
	case KTRL('Q'):
	{
		do_cmd_checkquest(creature_ptr);
		break;
	}
	case KTRL('R'):
	{
		now_message = old_now_message;
		do_cmd_redraw(creature_ptr);
		break;
	}
	case KTRL('S'):
	{
		do_cmd_save_game(creature_ptr, FALSE);
		break;
	}
	case KTRL('T'):
	{
		do_cmd_time(creature_ptr);
		break;
	}
	case KTRL('X'):
	case SPECIAL_KEY_QUIT:
	{
		do_cmd_save_and_exit(creature_ptr);
		break;
	}
	case 'Q':
	{
		do_cmd_suicide(creature_ptr);
		break;
	}
	case '|':
	{
		do_cmd_diary(creature_ptr);
		break;
	}
	case '~':
	{
		do_cmd_knowledge(creature_ptr);
		break;
	}
	case '(':
	{
		do_cmd_load_screen();
		break;
	}
	case ')':
	{
		do_cmd_save_screen(creature_ptr, handle_stuff, process_autopick_file_command);
		break;
	}
	case ']':
	{
		prepare_movie_hooks();
		break;
	}
	case KTRL('V'):
	{
		spoil_random_artifact(creature_ptr, "randifact.txt");
		break;
	}
	case '`':
	{
		if (!creature_ptr->wild_mode) do_cmd_travel(creature_ptr);
		if (creature_ptr->special_defense & KATA_MUSOU)
		{
			set_action(creature_ptr, ACTION_NONE);
		}
		break;
	}
	default:
	{
		if (flush_failure) flush();
		if (one_in_(2))
		{
			char error_m[1024];
			sound(SOUND_ILLEGAL);
			if (!get_rnd_line(_("error_j.txt", "error.txt"), 0, error_m))
				msg_print(error_m);
		}
		else
		{
			prt(_(" '?' でヘルプが表示されます。", "Type '?' for help."), 0, 0);
		}

		break;
	}
	}

	if (!creature_ptr->energy_use && !now_message)
		now_message = old_now_message;
}


/*!
 * @brief アイテムの所持種類数が超えた場合にアイテムを床に落とす処理 / Hack -- Pack Overflow
 * @return なし
 */
static void pack_overflow(player_type *owner_ptr)
{
	if (owner_ptr->inventory_list[INVEN_PACK].k_idx == 0) return;

	GAME_TEXT o_name[MAX_NLEN];
	object_type *o_ptr;
	update_creature(owner_ptr);
	if (!owner_ptr->inventory_list[INVEN_PACK].k_idx) return;

	o_ptr = &owner_ptr->inventory_list[INVEN_PACK];
	disturb(owner_ptr, FALSE, TRUE);
	msg_print(_("ザックからアイテムがあふれた！", "Your pack overflows!"));

	object_desc(owner_ptr, o_name, o_ptr, 0);
	msg_format(_("%s(%c)を落とした。", "You drop %s (%c)."), o_name, index_to_label(INVEN_PACK));
	(void)drop_near(owner_ptr, o_ptr, 0, owner_ptr->y, owner_ptr->x);

	vary_item(owner_ptr, INVEN_PACK, -255);
	handle_stuff(owner_ptr);
}


/*!
 * @brief プレイヤーの行動エネルギーが充填される（＝プレイヤーのターンが回る）毎に行われる処理  / process the effects per 100 energy at player speed.
 * @return なし
 */
static void process_upkeep_with_speed(player_type *creature_ptr)
{
	if (!load && creature_ptr->enchant_energy_need > 0 && !creature_ptr->leaving)
	{
		creature_ptr->enchant_energy_need -= SPEED_TO_ENERGY(creature_ptr->pspeed);
	}

	if (creature_ptr->enchant_energy_need > 0) return;

	while (creature_ptr->enchant_energy_need <= 0)
	{
		if (!load) check_music(creature_ptr);
		if (!load) check_hex(creature_ptr);
		if (!load) revenge_spell(creature_ptr);

		creature_ptr->enchant_energy_need += ENERGY_NEED();
	}
}


static void process_fishing(player_type *creature_ptr)
{
	Term_xtra(TERM_XTRA_DELAY, 10);
	if (one_in_(1000))
	{
		MONRACE_IDX r_idx;
		bool success = FALSE;
		get_mon_num_prep(creature_ptr, monster_is_fishing_target, NULL);
		r_idx = get_mon_num(creature_ptr, creature_ptr->current_floor_ptr->dun_level ? creature_ptr->current_floor_ptr->dun_level : wilderness[creature_ptr->wilderness_y][creature_ptr->wilderness_x].level, 0);
		msg_print(NULL);
		if (r_idx && one_in_(2))
		{
			POSITION y, x;
			y = creature_ptr->y + ddy[creature_ptr->fishing_dir];
			x = creature_ptr->x + ddx[creature_ptr->fishing_dir];
			if (place_monster_aux(creature_ptr, 0, y, x, r_idx, PM_NO_KAGE))
			{
				GAME_TEXT m_name[MAX_NLEN];
				monster_desc(creature_ptr, m_name, &creature_ptr->current_floor_ptr->m_list[creature_ptr->current_floor_ptr->grid_array[y][x].m_idx], 0);
				msg_format(_("%sが釣れた！", "You have a good catch!"), m_name);
				success = TRUE;
			}
		}

		if (!success)
		{
			msg_print(_("餌だけ食われてしまった！くっそ～！", "Damn!  The fish stole your bait!"));
		}

		disturb(creature_ptr, FALSE, TRUE);
	}
}


/*!
 * @brief プレイヤーの行動処理 / Process the player
 * @return なし
 * @note
 * Notice the annoying code to handle "pack overflow", which\n
 * must come first just in case somebody manages to corrupt\n
 * the savefiles by clever use of menu commands or something.\n
 */
static void process_player(player_type *creature_ptr)
{
	if (creature_ptr->hack_mutation)
	{
		msg_print(_("何か変わった気がする！", "You feel different!"));

		(void)gain_mutation(creature_ptr, 0);
		creature_ptr->hack_mutation = FALSE;
	}

	if (creature_ptr->invoking_midnight_curse)
	{
		int count = 0;
		activate_ty_curse(creature_ptr, FALSE, &count);
		creature_ptr->invoking_midnight_curse = FALSE;
	}

	if (creature_ptr->phase_out)
	{
		for (MONSTER_IDX m_idx = 1; m_idx < creature_ptr->current_floor_ptr->m_max; m_idx++)
		{
			monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[m_idx];
			if (!monster_is_valid(m_ptr)) continue;

			m_ptr->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
			update_monster(creature_ptr, m_idx, FALSE);
		}

		print_time(creature_ptr);
	}
	else if (!(load && creature_ptr->energy_need <= 0))
	{
		creature_ptr->energy_need -= SPEED_TO_ENERGY(creature_ptr->pspeed);
	}

	if (creature_ptr->energy_need > 0) return;
	if (!command_rep) print_time(creature_ptr);

	if (creature_ptr->resting < 0)
	{
		if (creature_ptr->resting == COMMAND_ARG_REST_FULL_HEALING)
		{
			if ((creature_ptr->chp == creature_ptr->mhp) &&
				(creature_ptr->csp >= creature_ptr->msp))
			{
				set_action(creature_ptr, ACTION_NONE);
			}
		}
		else if (creature_ptr->resting == COMMAND_ARG_REST_UNTIL_DONE)
		{
			if ((creature_ptr->chp == creature_ptr->mhp) &&
				(creature_ptr->csp >= creature_ptr->msp) &&
				!creature_ptr->blind && !creature_ptr->confused &&
				!creature_ptr->poisoned && !creature_ptr->afraid &&
				!creature_ptr->stun && !creature_ptr->cut &&
				!creature_ptr->slow && !creature_ptr->paralyzed &&
				!creature_ptr->image && !creature_ptr->word_recall &&
				!creature_ptr->alter_reality)
			{
				set_action(creature_ptr, ACTION_NONE);
			}
		}
	}

	if (creature_ptr->action == ACTION_FISH) process_fishing(creature_ptr);

	if (check_abort)
	{
		if (creature_ptr->running || travel.run || command_rep || (creature_ptr->action == ACTION_REST) || (creature_ptr->action == ACTION_FISH))
		{
			inkey_scan = TRUE;
			if (inkey())
			{
				flush();
				disturb(creature_ptr, FALSE, TRUE);
				msg_print(_("中断しました。", "Canceled."));
			}
		}
	}

	if (creature_ptr->riding && !creature_ptr->confused && !creature_ptr->blind)
	{
		monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];
		if (MON_CSLEEP(m_ptr))
		{
			GAME_TEXT m_name[MAX_NLEN];
			(void)set_monster_csleep(creature_ptr, creature_ptr->riding, 0);
			monster_desc(creature_ptr, m_name, m_ptr, 0);
			msg_format(_("%^sを起こした。", "You have woken %s up."), m_name);
		}

		if (MON_STUNNED(m_ptr))
		{
			if (set_monster_stunned(creature_ptr, creature_ptr->riding,
				(randint0(r_ptr->level) < creature_ptr->skill_exp[GINOU_RIDING]) ? 0 : (MON_STUNNED(m_ptr) - 1)))
			{
				GAME_TEXT m_name[MAX_NLEN];
				monster_desc(creature_ptr, m_name, m_ptr, 0);
				msg_format(_("%^sを朦朧状態から立ち直らせた。", "%^s is no longer stunned."), m_name);
			}
		}

		if (MON_CONFUSED(m_ptr))
		{
			if (set_monster_confused(creature_ptr, creature_ptr->riding,
				(randint0(r_ptr->level) < creature_ptr->skill_exp[GINOU_RIDING]) ? 0 : (MON_CONFUSED(m_ptr) - 1)))
			{
				GAME_TEXT m_name[MAX_NLEN];
				monster_desc(creature_ptr, m_name, m_ptr, 0);
				msg_format(_("%^sを混乱状態から立ち直らせた。", "%^s is no longer confused."), m_name);
			}
		}

		if (MON_MONFEAR(m_ptr))
		{
			if (set_monster_monfear(creature_ptr, creature_ptr->riding,
				(randint0(r_ptr->level) < creature_ptr->skill_exp[GINOU_RIDING]) ? 0 : (MON_MONFEAR(m_ptr) - 1)))
			{
				GAME_TEXT m_name[MAX_NLEN];
				monster_desc(creature_ptr, m_name, m_ptr, 0);
				msg_format(_("%^sを恐怖から立ち直らせた。", "%^s is no longer afraid."), m_name);
			}
		}

		handle_stuff(creature_ptr);
	}

	load = FALSE;
	if (creature_ptr->lightspeed)
	{
		(void)set_lightspeed(creature_ptr, creature_ptr->lightspeed - 1, TRUE);
	}

	if ((creature_ptr->pclass == CLASS_FORCETRAINER) && P_PTR_KI)
	{
		if (P_PTR_KI < 40) P_PTR_KI = 0;
		else P_PTR_KI -= 40;
		creature_ptr->update |= (PU_BONUS);
	}

	if (creature_ptr->action == ACTION_LEARN)
	{
		s32b cost = 0L;
		u32b cost_frac = (creature_ptr->msp + 30L) * 256L;
		s64b_LSHIFT(cost, cost_frac, 16);
		if (s64b_cmp(creature_ptr->csp, creature_ptr->csp_frac, cost, cost_frac) < 0)
		{
			creature_ptr->csp = 0;
			creature_ptr->csp_frac = 0;
			set_action(creature_ptr, ACTION_NONE);
		}
		else
		{
			s64b_sub(&(creature_ptr->csp), &(creature_ptr->csp_frac), cost, cost_frac);
		}

		creature_ptr->redraw |= PR_MANA;
	}

	if (creature_ptr->special_defense & KATA_MASK)
	{
		if (creature_ptr->special_defense & KATA_MUSOU)
		{
			if (creature_ptr->csp < 3)
			{
				set_action(creature_ptr, ACTION_NONE);
			}
			else
			{
				creature_ptr->csp -= 2;
				creature_ptr->redraw |= (PR_MANA);
			}
		}
	}

	/*** Handle actual user input ***/
	while (creature_ptr->energy_need <= 0)
	{
		creature_ptr->window |= PW_PLAYER;
		creature_ptr->sutemi = FALSE;
		creature_ptr->counter = FALSE;
		creature_ptr->now_damaged = FALSE;

		handle_stuff(creature_ptr);
		move_cursor_relative(creature_ptr->y, creature_ptr->x);
		if (fresh_before) Term_fresh();

		pack_overflow(creature_ptr);
		if (!command_new) command_see = FALSE;

		free_turn(creature_ptr);
		if (creature_ptr->phase_out)
		{
			move_cursor_relative(creature_ptr->y, creature_ptr->x);
			command_cmd = SPECIAL_KEY_BUILDING;
			process_command(creature_ptr);
		}
		else if (creature_ptr->paralyzed || (creature_ptr->stun >= 100))
		{
			take_turn(creature_ptr, 100);
		}
		else if (creature_ptr->action == ACTION_REST)
		{
			if (creature_ptr->resting > 0)
			{
				creature_ptr->resting--;
				if (!creature_ptr->resting) set_action(creature_ptr, ACTION_NONE);
				creature_ptr->redraw |= (PR_STATE);
			}

			take_turn(creature_ptr, 100);
		}
		else if (creature_ptr->action == ACTION_FISH)
		{
			take_turn(creature_ptr, 100);
		}
		else if (creature_ptr->running)
		{
			run_step(creature_ptr, 0);
		}
		else if (travel.run)
		{
			travel_step(creature_ptr);
		}
		else if (command_rep)
		{
			command_rep--;
			creature_ptr->redraw |= (PR_STATE);
			handle_stuff(creature_ptr);
			msg_flag = FALSE;
			prt("", 0, 0);
			process_command(creature_ptr);
		}
		else
		{
			move_cursor_relative(creature_ptr->y, creature_ptr->x);
			can_save = TRUE;
			request_command(creature_ptr, FALSE);
			can_save = FALSE;
			process_command(creature_ptr);
		}

		pack_overflow(creature_ptr);
		if (creature_ptr->energy_use)
		{
			if (creature_ptr->timewalk || creature_ptr->energy_use > 400)
			{
				creature_ptr->energy_need += creature_ptr->energy_use * TURNS_PER_TICK / 10;
			}
			else
			{
				creature_ptr->energy_need += (s16b)((s32b)creature_ptr->energy_use * ENERGY_NEED() / 100L);
			}

			if (creature_ptr->image) creature_ptr->redraw |= (PR_MAP);

			for (MONSTER_IDX m_idx = 1; m_idx < creature_ptr->current_floor_ptr->m_max; m_idx++)
			{
				monster_type *m_ptr;
				monster_race *r_ptr;
				m_ptr = &creature_ptr->current_floor_ptr->m_list[m_idx];
				if (!monster_is_valid(m_ptr)) continue;
				if (!m_ptr->ml) continue;

				r_ptr = &r_info[m_ptr->ap_r_idx];
				if (!(r_ptr->flags1 & (RF1_ATTR_MULTI | RF1_SHAPECHANGER)))
					continue;

				lite_spot(creature_ptr, m_ptr->fy, m_ptr->fx);
			}

			if (repair_monsters)
			{
				repair_monsters = FALSE;
				for (MONSTER_IDX m_idx = 1; m_idx < creature_ptr->current_floor_ptr->m_max; m_idx++)
				{
					monster_type *m_ptr;
					m_ptr = &creature_ptr->current_floor_ptr->m_list[m_idx];
					if (!monster_is_valid(m_ptr)) continue;

					if (m_ptr->mflag & MFLAG_NICE)
					{
						m_ptr->mflag &= ~(MFLAG_NICE);
					}

					if (m_ptr->mflag2 & MFLAG2_MARK)
					{
						if (m_ptr->mflag2 & MFLAG2_SHOW)
						{
							m_ptr->mflag2 &= ~(MFLAG2_SHOW);
							repair_monsters = TRUE;
						}
						else
						{
							m_ptr->mflag2 &= ~(MFLAG2_MARK);
							m_ptr->ml = FALSE;
							update_monster(creature_ptr, m_idx, FALSE);
							if (creature_ptr->health_who == m_idx) creature_ptr->redraw |= (PR_HEALTH);
							if (creature_ptr->riding == m_idx) creature_ptr->redraw |= (PR_UHEALTH);

							lite_spot(creature_ptr, m_ptr->fy, m_ptr->fx);
						}
					}
				}
			}

			if (creature_ptr->pclass == CLASS_IMITATOR)
			{
				if (creature_ptr->mane_num > (creature_ptr->lev > 44 ? 3 : creature_ptr->lev > 29 ? 2 : 1))
				{
					creature_ptr->mane_num--;
					for (int j = 0; j < creature_ptr->mane_num; j++)
					{
						creature_ptr->mane_spell[j] = creature_ptr->mane_spell[j + 1];
						creature_ptr->mane_dam[j] = creature_ptr->mane_dam[j + 1];
					}
				}

				creature_ptr->new_mane = FALSE;
				creature_ptr->redraw |= (PR_IMITATION);
			}

			if (creature_ptr->action == ACTION_LEARN)
			{
				creature_ptr->new_mane = FALSE;
				creature_ptr->redraw |= (PR_STATE);
			}

			if (creature_ptr->timewalk && (creature_ptr->energy_need > -1000))
			{

				creature_ptr->redraw |= (PR_MAP);
				creature_ptr->update |= (PU_MONSTERS);
				creature_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

				msg_print(_("「時は動きだす…」", "You feel time flowing around you once more."));
				msg_print(NULL);
				creature_ptr->timewalk = FALSE;
				creature_ptr->energy_need = ENERGY_NEED();

				handle_stuff(creature_ptr);
			}
		}

		if (!creature_ptr->playing || creature_ptr->is_dead)
		{
			creature_ptr->timewalk = FALSE;
			break;
		}

		if (creature_ptr->energy_use && creature_ptr->reset_concent)
			reset_concentration(creature_ptr, TRUE);

		if (creature_ptr->leaving) break;
	}

	update_smell(creature_ptr->current_floor_ptr, creature_ptr);
}


/*!
 * @brief 現在プレイヤーがいるダンジョンの全体処理 / Interact with the current dungeon level.
 * @return なし
 * @details
 * <p>
 * この関数から現在の階層を出る、プレイヤーがキャラが死ぬ、
 * ゲームを終了するかのいずれかまでループする。
 * </p>
 * <p>
 * This function will not exit until the level is completed,\n
 * the user dies, or the game is terminated.\n
 * </p>
 */
static void dungeon(player_type *player_ptr, bool load_game)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	floor_ptr->base_level = floor_ptr->dun_level;
	current_world_ptr->is_loading_now = FALSE;
	player_ptr->leaving = FALSE;

	command_cmd = 0;
	command_rep = 0;
	command_arg = 0;
	command_dir = 0;

	target_who = 0;
	player_ptr->pet_t_m_idx = 0;
	player_ptr->riding_t_m_idx = 0;
	player_ptr->ambush_flag = FALSE;
	health_track(player_ptr, 0);
	repair_monsters = TRUE;
	repair_objects = TRUE;

	disturb(player_ptr, TRUE, TRUE);
	int quest_num = quest_num = quest_number(player_ptr, floor_ptr->dun_level);
	if (quest_num)
	{
		r_info[quest[quest_num].r_idx].flags1 |= RF1_QUESTOR;
	}

	if (player_ptr->max_plv < player_ptr->lev)
	{
		player_ptr->max_plv = player_ptr->lev;
	}

	if ((max_dlv[player_ptr->dungeon_idx] < floor_ptr->dun_level) && !floor_ptr->inside_quest)
	{
		max_dlv[player_ptr->dungeon_idx] = floor_ptr->dun_level;
		if (record_maxdepth) exe_write_diary(player_ptr, DIARY_MAXDEAPTH, floor_ptr->dun_level, NULL);
	}

	(void)calculate_upkeep(player_ptr);
	panel_bounds_center();
	verify_panel(player_ptr);
	msg_erase();

	current_world_ptr->character_xtra = TRUE;
	player_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER | PW_MONSTER | PW_OVERHEAD | PW_DUNGEON);
	player_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_EQUIPPY | PR_MAP);
	player_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS | PU_VIEW | PU_LITE | PU_MON_LITE | PU_TORCH | PU_MONSTERS | PU_DISTANCE | PU_FLOW);
	handle_stuff(player_ptr);

	current_world_ptr->character_xtra = FALSE;
	player_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
	player_ptr->update |= (PU_COMBINE | PU_REORDER);
	handle_stuff(player_ptr);
	Term_fresh();

	if (quest_num && (is_fixed_quest_idx(quest_num) &&
		!((quest_num == QUEST_OBERON) || (quest_num == QUEST_SERPENT) ||
			!(quest[quest_num].flags & QUEST_FLAG_PRESET))))
		do_cmd_feeling(player_ptr);

	if (player_ptr->phase_out)
	{
		if (load_game)
		{
			player_ptr->energy_need = 0;
			update_gambling_monsters(player_ptr);
		}
		else
		{
			msg_print(_("試合開始！", "Ready..Fight!"));
			msg_print(NULL);
		}
	}

	if ((player_ptr->pclass == CLASS_BARD) && (SINGING_SONG_EFFECT(player_ptr) > MUSIC_DETECT))
		SINGING_SONG_EFFECT(player_ptr) = MUSIC_DETECT;

	if (!player_ptr->playing || player_ptr->is_dead) return;

	if (!floor_ptr->inside_quest && (player_ptr->dungeon_idx == DUNGEON_ANGBAND))
	{
		quest_discovery(random_quest_number(player_ptr, floor_ptr->dun_level));
		floor_ptr->inside_quest = random_quest_number(player_ptr, floor_ptr->dun_level);
	}
	if ((floor_ptr->dun_level == d_info[player_ptr->dungeon_idx].maxdepth) && d_info[player_ptr->dungeon_idx].final_guardian)
	{
		if (r_info[d_info[player_ptr->dungeon_idx].final_guardian].max_num)
#ifdef JP
			msg_format("この階には%sの主である%sが棲んでいる。",
				d_name + d_info[player_ptr->dungeon_idx].name,
				r_name + r_info[d_info[player_ptr->dungeon_idx].final_guardian].name);
#else
			msg_format("%^s lives in this level as the keeper of %s.",
				r_name + r_info[d_info[player_ptr->dungeon_idx].final_guardian].name,
				d_name + d_info[player_ptr->dungeon_idx].name);
#endif
	}

	if (!load_game && (player_ptr->special_defense & NINJA_S_STEALTH)) set_superstealth(player_ptr, FALSE);

	floor_ptr->monster_level = floor_ptr->base_level;
	floor_ptr->object_level = floor_ptr->base_level;
	current_world_ptr->is_loading_now = TRUE;
	if (player_ptr->energy_need > 0 && !player_ptr->phase_out &&
		(floor_ptr->dun_level || player_ptr->leaving_dungeon || floor_ptr->inside_arena))
		player_ptr->energy_need = 0;

	player_ptr->leaving_dungeon = FALSE;
	mproc_init(floor_ptr);

	while (TRUE)
	{
		if ((floor_ptr->m_cnt + 32 > current_world_ptr->max_m_idx) && !player_ptr->phase_out)
			compact_monsters(player_ptr, 64);

		if ((floor_ptr->m_cnt + 32 < floor_ptr->m_max) && !player_ptr->phase_out)
			compact_monsters(player_ptr, 0);

		if (floor_ptr->o_cnt + 32 > current_world_ptr->max_o_idx)
			compact_objects(player_ptr, 64);

		if (floor_ptr->o_cnt + 32 < floor_ptr->o_max)
			compact_objects(player_ptr, 0);

		process_player(player_ptr);
		process_upkeep_with_speed(player_ptr);
		handle_stuff(player_ptr);

		move_cursor_relative(player_ptr->y, player_ptr->x);
		if (fresh_after) Term_fresh();

		if (!player_ptr->playing || player_ptr->is_dead) break;

		process_monsters(player_ptr);
		handle_stuff(player_ptr);

		move_cursor_relative(player_ptr->y, player_ptr->x);
		if (fresh_after) Term_fresh();

		if (!player_ptr->playing || player_ptr->is_dead) break;

		process_world(player_ptr);
		handle_stuff(player_ptr);

		move_cursor_relative(player_ptr->y, player_ptr->x);
		if (fresh_after) Term_fresh();

		if (!player_ptr->playing || player_ptr->is_dead) break;

		current_world_ptr->game_turn++;
		if (current_world_ptr->dungeon_turn < current_world_ptr->dungeon_turn_limit)
		{
			if (!player_ptr->wild_mode || wild_regen) current_world_ptr->dungeon_turn++;
			else if (player_ptr->wild_mode && !(current_world_ptr->game_turn % ((MAX_HGT + MAX_WID) / 2))) current_world_ptr->dungeon_turn++;
		}

		prevent_turn_overflow(player_ptr);

		if (player_ptr->leaving) break;

		if (wild_regen) wild_regen--;
	}

	if (quest_num && !(r_info[quest[quest_num].r_idx].flags1 & RF1_UNIQUE))
	{
		r_info[quest[quest_num].r_idx].flags1 &= ~RF1_QUESTOR;
	}

	if (player_ptr->playing && !player_ptr->is_dead)
	{
		/*
		 * Maintain Unique monsters and artifact, save current
		 * floor, then prepare next floor
		 */
		leave_floor(player_ptr);
		reinit_wilderness = FALSE;
	}

	write_level = TRUE;
}


/*!
 * @brief 全ユーザプロファイルをロードする / Load some "user pref files"
 * @paaram player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @note
 * Modified by Arcum Dagsson to support
 * separate macro files for different realms.
 */
static void load_all_pref_files(player_type *player_ptr)
{
	char buf[1024];
	sprintf(buf, "user.prf");
	process_pref_file(player_ptr, buf, process_autopick_file_command);
	sprintf(buf, "user-%s.prf", ANGBAND_SYS);
	process_pref_file(player_ptr, buf, process_autopick_file_command);
	sprintf(buf, "%s.prf", rp_ptr->title);
	process_pref_file(player_ptr, buf, process_autopick_file_command);
	sprintf(buf, "%s.prf", cp_ptr->title);
	process_pref_file(player_ptr, buf, process_autopick_file_command);
	sprintf(buf, "%s.prf", player_ptr->base_name);
	process_pref_file(player_ptr, buf, process_autopick_file_command);
	if (player_ptr->realm1 != REALM_NONE)
	{
		sprintf(buf, "%s.prf", realm_names[player_ptr->realm1]);
		process_pref_file(player_ptr, buf, process_autopick_file_command);
	}

	if (player_ptr->realm2 != REALM_NONE)
	{
		sprintf(buf, "%s.prf", realm_names[player_ptr->realm2]);
		process_pref_file(player_ptr, buf, process_autopick_file_command);
	}

	autopick_load_pref(player_ptr, FALSE);
}


/*!
 * @brief 1ゲームプレイの主要ルーチン / Actually play a game
 * @return なし
 * @note
 * If the "new_game" parameter is true, then, after loading the
 * savefile, we will commit suicide, if necessary, to allow the
 * player to start a new game.
 */
void play_game(player_type *player_ptr, bool new_game)
{
	bool load_game = TRUE;
	bool init_random_seed = FALSE;

#ifdef CHUUKEI
	if (chuukei_client)
	{
		reset_visuals(player_ptr, process_autopick_file_command);
		browse_chuukei();
		return;
	}

	else if (chuukei_server)
	{
		prepare_chuukei_hooks();
	}
#endif

	if (browsing_movie)
	{
		reset_visuals(player_ptr, process_autopick_file_command);
		browse_movie();
		return;
	}

	player_ptr->hack_mutation = FALSE;
	current_world_ptr->character_icky = TRUE;
	Term_activate(angband_term[0]);
	angband_term[0]->resize_hook = resize_map;
	for (MONSTER_IDX i = 1; i < 8; i++)
	{
		if (angband_term[i])
		{
			angband_term[i]->resize_hook = redraw_window;
		}
	}

	(void)Term_set_cursor(0);
	if (!load_player(player_ptr))
	{
		quit(_("セーブファイルが壊れています", "broken savefile"));
	}

	extract_option_vars();
	if (player_ptr->wait_report_score)
	{
		char buf[1024];
		bool success;

		if (!get_check_strict(_("待機していたスコア登録を今行ないますか？", "Do you register score now? "), CHECK_NO_HISTORY))
			quit(0);

		player_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
		update_creature(player_ptr);
		player_ptr->is_dead = TRUE;
		current_world_ptr->start_time = (u32b)time(NULL);
		signals_ignore_tstp();
		current_world_ptr->character_icky = TRUE;
		path_build(buf, sizeof(buf), ANGBAND_DIR_APEX, "scores.raw");
		highscore_fd = fd_open(buf, O_RDWR);

		/* 町名消失バグ対策(#38205)のためここで世界マップ情報を読み出す */
		process_dungeon_file(player_ptr, "w_info.txt", 0, 0, current_world_ptr->max_wild_y, current_world_ptr->max_wild_x);
		success = send_world_score(player_ptr, TRUE, update_playtime, display_player, map_name);

		if (!success && !get_check_strict(_("スコア登録を諦めますか？", "Do you give up score registration? "), CHECK_NO_HISTORY))
		{
			prt(_("引き続き待機します。", "standing by for future registration..."), 0, 0);
			(void)inkey();
		}
		else
		{
			player_ptr->wait_report_score = FALSE;
			top_twenty(player_ptr);
			if (!save_player(player_ptr)) msg_print(_("セーブ失敗！", "death save failed!"));
		}

		(void)fd_close(highscore_fd);
		highscore_fd = -1;
		signals_handle_tstp();

		quit(0);
	}

	current_world_ptr->creating_savefile = new_game;

	if (!current_world_ptr->character_loaded)
	{
		new_game = TRUE;
		current_world_ptr->character_dungeon = FALSE;
		init_random_seed = TRUE;
		init_saved_floors(player_ptr, FALSE);
	}
	else if (new_game)
	{
		init_saved_floors(player_ptr, TRUE);
	}

	if (!new_game)
	{
		process_player_name(player_ptr, FALSE);
	}

	if (init_random_seed)
	{
		Rand_state_init();
	}

	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	if (new_game)
	{
		current_world_ptr->character_dungeon = FALSE;

		floor_ptr->dun_level = 0;
		floor_ptr->inside_quest = 0;
		floor_ptr->inside_arena = FALSE;
		player_ptr->phase_out = FALSE;
		write_level = TRUE;

		current_world_ptr->seed_flavor = randint0(0x10000000);
		current_world_ptr->seed_town = randint0(0x10000000);

		player_birth(player_ptr, process_autopick_file_command);
		counts_write(player_ptr, 2, 0);
		player_ptr->count = 0;
		load = FALSE;
		determine_bounty_uniques(player_ptr);
		determine_daily_bounty(player_ptr, FALSE);
		wipe_o_list(floor_ptr);
	}
	else
	{
		write_level = FALSE;
		exe_write_diary(player_ptr, DIARY_GAMESTART, 1,
			_("                            ----ゲーム再開----",
				"                            --- Restarted Game ---"));

		/*
		 * todo もう2.2.Xなので互換性は打ち切ってもいいのでは？
		 * 1.0.9 以前はセーブ前に player_ptr->riding = -1 としていたので、再設定が必要だった。
		 * もう不要だが、以前のセーブファイルとの互換のために残しておく。
		 */
		if (player_ptr->riding == -1)
		{
			player_ptr->riding = 0;
			for (MONSTER_IDX i = floor_ptr->m_max; i > 0; i--)
			{
				if (player_bold(player_ptr, floor_ptr->m_list[i].fy, floor_ptr->m_list[i].fx))
				{
					player_ptr->riding = i;
					break;
				}
			}
		}
	}

	current_world_ptr->creating_savefile = FALSE;

	player_ptr->teleport_town = FALSE;
	player_ptr->sutemi = FALSE;
	current_world_ptr->timewalk_m_idx = 0;
	player_ptr->now_damaged = FALSE;
	now_message = 0;
	current_world_ptr->start_time = time(NULL) - 1;
	record_o_name[0] = '\0';

	panel_row_min = floor_ptr->height;
	panel_col_min = floor_ptr->width;
	if (player_ptr->pseikaku == SEIKAKU_SEXY)
		s_info[player_ptr->pclass].w_max[TV_HAFTED - TV_WEAPON_BEGIN][SV_WHIP] = WEAPON_EXP_MASTER;

	set_floor_and_wall(player_ptr->dungeon_idx);
	flavor_init();
	prt(_("お待ち下さい...", "Please wait..."), 0, 0);
	Term_fresh();

	if (arg_wizard)
	{
		if (enter_wizard_mode(player_ptr))
		{
			current_world_ptr->wizard = TRUE;

			if (player_ptr->is_dead || !player_ptr->y || !player_ptr->x)
			{
				init_saved_floors(player_ptr, TRUE);
				floor_ptr->inside_quest = 0;
				player_ptr->y = player_ptr->x = 10;
			}
		}
		else if (player_ptr->is_dead)
		{
			quit("Already dead.");
		}
	}

	if (!floor_ptr->dun_level && !floor_ptr->inside_quest)
	{
		process_dungeon_file(player_ptr, "w_info.txt", 0, 0, current_world_ptr->max_wild_y, current_world_ptr->max_wild_x);
		init_flags = INIT_ONLY_BUILDINGS;
		process_dungeon_file(player_ptr, "t_info.txt", 0, 0, MAX_HGT, MAX_WID);
		select_floor_music(player_ptr);
	}

	if (!current_world_ptr->character_dungeon)
	{
		change_floor(player_ptr);
	}
	else
	{
		if (player_ptr->panic_save)
		{
			if (!player_ptr->y || !player_ptr->x)
			{
				msg_print(_("プレイヤーの位置がおかしい。フロアを再生成します。", "What a strange player location, regenerate the dungeon floor."));
				change_floor(player_ptr);
			}

			if (!player_ptr->y || !player_ptr->x) player_ptr->y = player_ptr->x = 10;

			player_ptr->panic_save = 0;
		}
	}

	current_world_ptr->character_generated = TRUE;
	current_world_ptr->character_icky = FALSE;

	if (new_game)
	{
		char buf[80];
		sprintf(buf, _("%sに降り立った。", "arrived in %s."), map_name(player_ptr));
		exe_write_diary(player_ptr, DIARY_DESCRIPTION, 0, buf);
	}

	player_ptr->playing = TRUE;
	reset_visuals(player_ptr, process_autopick_file_command);
	load_all_pref_files(player_ptr);
	if (new_game)
	{
		player_outfit(player_ptr);
	}

	Term_xtra(TERM_XTRA_REACT, 0);

	player_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER);
	player_ptr->window |= (PW_MESSAGE | PW_OVERHEAD | PW_DUNGEON | PW_MONSTER | PW_OBJECT);
	handle_stuff(player_ptr);

	if (arg_force_original) rogue_like_commands = FALSE;
	if (arg_force_roguelike) rogue_like_commands = TRUE;

	if (player_ptr->chp < 0) player_ptr->is_dead = TRUE;

	if (player_ptr->prace == RACE_ANDROID) calc_android_exp(player_ptr);

	if (new_game && ((player_ptr->pclass == CLASS_CAVALRY) || (player_ptr->pclass == CLASS_BEASTMASTER)))
	{
		monster_type *m_ptr;
		MONRACE_IDX pet_r_idx = ((player_ptr->pclass == CLASS_CAVALRY) ? MON_HORSE : MON_YASE_HORSE);
		monster_race *r_ptr = &r_info[pet_r_idx];
		place_monster_aux(player_ptr, 0, player_ptr->y, player_ptr->x - 1, pet_r_idx,
			(PM_FORCE_PET | PM_NO_KAGE));
		m_ptr = &floor_ptr->m_list[hack_m_idx_ii];
		m_ptr->mspeed = r_ptr->speed;
		m_ptr->maxhp = r_ptr->hdice*(r_ptr->hside + 1) / 2;
		m_ptr->max_maxhp = m_ptr->maxhp;
		m_ptr->hp = r_ptr->hdice*(r_ptr->hside + 1) / 2;
		m_ptr->dealt_damage = 0;
		m_ptr->energy_need = ENERGY_NEED() + ENERGY_NEED();
	}

	(void)combine_and_reorder_home(STORE_HOME);
	(void)combine_and_reorder_home(STORE_MUSEUM);
	select_floor_music(player_ptr);

	while (TRUE)
	{
		dungeon(player_ptr, load_game);
		current_world_ptr->character_xtra = TRUE;
		handle_stuff(player_ptr);

		current_world_ptr->character_xtra = FALSE;
		target_who = 0;
		health_track(player_ptr, 0);
		forget_lite(floor_ptr);
		forget_view(floor_ptr);
		clear_mon_lite(floor_ptr);
		if (!player_ptr->playing && !player_ptr->is_dead) break;

		wipe_o_list(floor_ptr);
		if (!player_ptr->is_dead) wipe_monsters_list(player_ptr);

		msg_print(NULL);
		load_game = FALSE;
		if (player_ptr->playing && player_ptr->is_dead)
		{
			if (floor_ptr->inside_arena)
			{
				floor_ptr->inside_arena = FALSE;
				if (player_ptr->arena_number > MAX_ARENA_MONS)
					player_ptr->arena_number++;
				else
					player_ptr->arena_number = -1 - player_ptr->arena_number;
				player_ptr->is_dead = FALSE;
				player_ptr->chp = 0;
				player_ptr->chp_frac = 0;
				player_ptr->exit_bldg = TRUE;
				reset_tim_flags(player_ptr);
				prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS | CFM_RAND_CONNECT);
				leave_floor(player_ptr);
			}
			else
			{
				if ((current_world_ptr->wizard || cheat_live) && !get_check(_("死にますか? ", "Die? ")))
				{
					cheat_death(player_ptr);
				}
			}
		}

		if (player_ptr->is_dead) break;

		change_floor(player_ptr);
	}

	close_game(player_ptr);
	quit(NULL);
}


/*!
 * @brief ゲームターンからの実時間換算を行うための補正をかける
 * @param hoge ゲームターン
 * @details アンデッド種族は18:00からゲームを開始するので、この修正を予め行う。
 * @return 修正をかけた後のゲームターン
 */
s32b turn_real(player_type *player_ptr, s32b hoge)
{
	switch (player_ptr->start_race)
	{
	case RACE_VAMPIRE:
	case RACE_SKELETON:
	case RACE_ZOMBIE:
	case RACE_SPECTRE:
		return hoge - (TURNS_PER_TICK * TOWN_DAWN * 3 / 4);
	default:
		return hoge;
	}
}


/*!
 * @brief ターンのオーバーフローに対する対処
 * @param player_ptr プレーヤーへの参照ポインタ
 * @details ターン及びターンを記録する変数をターンの限界の1日前まで巻き戻す.
 * @return 修正をかけた後のゲームターン
 */
void prevent_turn_overflow(player_type *player_ptr)
{
	if (current_world_ptr->game_turn < current_world_ptr->game_turn_limit) return;

	int rollback_days = 1 + (current_world_ptr->game_turn - current_world_ptr->game_turn_limit) / (TURNS_PER_TICK * TOWN_DAWN);
	s32b rollback_turns = TURNS_PER_TICK * TOWN_DAWN * rollback_days;

	if (current_world_ptr->game_turn > rollback_turns) current_world_ptr->game_turn -= rollback_turns;
	else current_world_ptr->game_turn = 1;
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	if (floor_ptr->generated_turn > rollback_turns) floor_ptr->generated_turn -= rollback_turns;
	else floor_ptr->generated_turn = 1;
	if (current_world_ptr->arena_start_turn > rollback_turns) current_world_ptr->arena_start_turn -= rollback_turns;
	else current_world_ptr->arena_start_turn = 1;
	if (player_ptr->feeling_turn > rollback_turns) player_ptr->feeling_turn -= rollback_turns;
	else player_ptr->feeling_turn = 1;

	for (int i = 1; i < max_towns; i++)
	{
		for (int j = 0; j < MAX_STORES; j++)
		{
			store_type *store_ptr = &town_info[i].store[j];

			if (store_ptr->last_visit > -10L * TURNS_PER_TICK * STORE_TICKS)
			{
				store_ptr->last_visit -= rollback_turns;
				if (store_ptr->last_visit < -10L * TURNS_PER_TICK * STORE_TICKS) store_ptr->last_visit = -10L * TURNS_PER_TICK * STORE_TICKS;
			}

			if (store_ptr->store_open)
			{
				store_ptr->store_open -= rollback_turns;
				if (store_ptr->store_open < 1) store_ptr->store_open = 1;
			}
		}
	}
}


/*!
 * @brief ゲーム終了処理 /
 * Close up the current game (player may or may not be dead)
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * <pre>
 * This function is called only from "main.c" and "signals.c".
 * </pre>
 */
void close_game(player_type *player_ptr)
{
	char buf[1024];
	bool do_send = TRUE;
	handle_stuff(player_ptr);

	msg_print(NULL);
	flush();

	signals_ignore_tstp();

	current_world_ptr->character_icky = TRUE;
	path_build(buf, sizeof(buf), ANGBAND_DIR_APEX, "scores.raw");
	safe_setuid_grab();
	highscore_fd = fd_open(buf, O_RDWR);
	safe_setuid_drop();

	if (player_ptr->is_dead)
	{
		if (current_world_ptr->total_winner) kingly(player_ptr);

		if (!cheat_save || get_check(_("死んだデータをセーブしますか？ ", "Save death? ")))
		{
			if (!save_player(player_ptr)) msg_print(_("セーブ失敗！", "death save failed!"));
		}
		else do_send = FALSE;

		print_tomb(player_ptr);
		flush();

		show_info(player_ptr, handle_stuff, update_playtime, display_player, map_name);
		Term_clear();

		if (check_score(player_ptr))
		{
			if ((!send_world_score(player_ptr, do_send, update_playtime, display_player, map_name)))
			{
				if (get_check_strict(_("後でスコアを登録するために待機しますか？", "Stand by for later score registration? "),
					(CHECK_NO_ESCAPE | CHECK_NO_HISTORY)))
				{
					player_ptr->wait_report_score = TRUE;
					player_ptr->is_dead = FALSE;
					if (!save_player(player_ptr)) msg_print(_("セーブ失敗！", "death save failed!"));
				}
			}
			if (!player_ptr->wait_report_score)
				(void)top_twenty(player_ptr);
		}
		else if (highscore_fd >= 0)
		{
			display_scores_aux(0, 10, -1, NULL);
		}
	}
	else
	{
		do_cmd_save_game(player_ptr, FALSE);
		prt(_("リターンキーか ESC キーを押して下さい。", "Press Return (or Escape)."), 0, 40);
		play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_EXIT);
		if (inkey() != ESCAPE) predict_score(player_ptr);
	}

	(void)fd_close(highscore_fd);
	highscore_fd = -1;
	clear_saved_floor_files(player_ptr);
	signals_handle_tstp();
}


void update_output(player_type *player_ptr)
{
	if (player_ptr->redraw) redraw_stuff(player_ptr);
	if (player_ptr->window) window_stuff(player_ptr);
}
