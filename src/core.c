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
#include "util.h"
#include "core.h"
#include "inet.h"
#include "term.h"

#include "creature.h"

#include "birth.h"
#include "bldg.h"
#include "cmd-activate.h"
#include "cmd-dump.h"
#include "cmd-eat.h"
#include "cmd-hissatsu.h"
#include "cmd-item.h"
#include "cmd-magiceat.h"
#include "cmd-mane.h"
#include "cmd-quaff.h"
#include "cmd-read.h"
#include "cmd-smith.h"
#include "cmd-usestaff.h"
#include "cmd-zaprod.h"
#include "cmd-zapwand.h"
#include "cmd-pet.h"
#include "cmd-basic.h"
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
#include "store.h"
#include "spells.h"
#include "spells-summon.h"
#include "spells-object.h"
#include "spells-status.h"
#include "spells-floor.h"
#include "monster-spell.h"
#include "mind.h"
#include "world.h"
#include "mutation.h"
#include "quest.h"
#include "artifact.h"
#include "avatar.h"
#include "player-move.h"
#include "player-status.h"
#include "player-class.h"
#include "player-race.h"
#include "player-personality.h"
#include "player-damage.h"
#include "player-effects.h"
#include "cmd-spell.h"
#include "realm-hex.h"
#include "objectkind.h"
#include "object-hook.h"
#include "wild.h"
#include "monster-process.h"
#include "monster-status.h"
#include "monsterrace-hook.h"
#include "floor-save.h"
#include "feature.h"
#include "player-skill.h"
#include "player-inventory.h"

#include "view-mainwindow.h"
#include "dungeon-file.h"
#include "files.h"
#include "scores.h"
#include "autopick.h"
#include "save.h"
#include "realm.h"
#include "realm-song.h"
#include "targeting.h"

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

bool can_save = FALSE;        /* Game can be saved */

COMMAND_CODE now_message;

bool repair_monsters;	/* Hack -- optimize detect monsters */
bool repair_objects;	/* Hack -- optimize detect objects */

concptr ANGBAND_SYS = "xxx"; //!< Hack -- The special Angband "System Suffix" This variable is used to choose an appropriate "pref-xxx" file

#ifdef JP
concptr ANGBAND_KEYBOARD = "JAPAN"; //!< Hack -- The special Angband "Keyboard Suffix" This variable is used to choose an appropriate macro-trigger definition
#else
concptr ANGBAND_KEYBOARD = "0";
#endif

concptr ANGBAND_GRAF = "ascii"; //!< Hack -- The special Angband "Graphics Suffix" This variable is used to choose an appropriate "graf-xxx" file

static bool load = TRUE; /*!<ロード処理中の分岐フラグ*/
static int wild_regen = 20; /*!<広域マップ移動時の自然回復処理カウンタ（広域マップ1マス毎に20回処理を基本とする）*/

/*
 * Flags for initialization
 */
int init_flags;

/*!
 * @brief 擬似鑑定を実際に行い判定を反映する
 * @param slot 擬似鑑定を行うプレイヤーの所持リストID
 * @param heavy 重度の擬似鑑定を行うならばTRUE
 * @return なし
 */
static void sense_inventory_aux(INVENTORY_IDX slot, bool heavy)
{
	byte feel;
	object_type *o_ptr = &p_ptr->inventory_list[slot];
	GAME_TEXT o_name[MAX_NLEN];

	/* We know about it already, do not tell us again */
	if (o_ptr->ident & (IDENT_SENSE))return;

	/* It is fully known, no information needed */
	if (object_is_known(o_ptr)) return;

	/* Check for a feeling */
	feel = (heavy ? value_check_aux1(o_ptr) : value_check_aux2(o_ptr));

	/* Skip non-feelings */
	if (!feel) return;

	/* Bad luck */
	if ((p_ptr->muta3 & MUT3_BAD_LUCK) && !randint0(13))
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

	/* Stop everything */
	if (disturb_minor) disturb(p_ptr, FALSE, FALSE);

	/* Get an object description */
	object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

	/* Message (equipment) */
	if (slot >= INVEN_RARM)
	{
#ifdef JP
		msg_format("%s%s(%c)は%sという感じがする...",
			describe_use(slot),o_name, index_to_label(slot),game_inscriptions[feel]);
#else
		msg_format("You feel the %s (%c) you are %s %s %s...",
			   o_name, index_to_label(slot), describe_use(slot),
			   ((o_ptr->number == 1) ? "is" : "are"),
				   game_inscriptions[feel]);
#endif

	}
	else
	{
#ifdef JP
		msg_format("ザックの中の%s(%c)は%sという感じがする...",
			o_name, index_to_label(slot),game_inscriptions[feel]);
#else
		msg_format("You feel the %s (%c) in your pack %s %s...",
			   o_name, index_to_label(slot),
			   ((o_ptr->number == 1) ? "is" : "are"),
				   game_inscriptions[feel]);
#endif

	}

	o_ptr->ident |= (IDENT_SENSE);

	/* Set the "inscription" */
	o_ptr->feeling = feel;

	/* Auto-inscription/destroy */
	autopick_alter_item(slot, destroy_feeling);
	p_ptr->update |= (PU_COMBINE | PU_REORDER);

	p_ptr->window |= (PW_INVEN | PW_EQUIP);
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
	INVENTORY_IDX i;
	PLAYER_LEVEL plev = creature_ptr->lev;
	bool heavy = FALSE;
	object_type *o_ptr;


	/*** Check for "sensing" ***/

	/* No sensing when confused */
	if (creature_ptr->confused) return;

	/* Analyze the class */
	switch (creature_ptr->pclass)
	{
		case CLASS_WARRIOR:
		case CLASS_ARCHER:
		case CLASS_SAMURAI:
		case CLASS_CAVALRY:
		{
			/* Good sensing */
			if (0 != randint0(9000L / (plev * plev + 40))) return;

			/* Heavy sensing */
			heavy = TRUE;

			break;
		}

		case CLASS_SMITH:
		{
			/* Good sensing */
			if (0 != randint0(6000L / (plev * plev + 50))) return;

			/* Heavy sensing */
			heavy = TRUE;

			break;
		}

		case CLASS_MAGE:
		case CLASS_HIGH_MAGE:
		case CLASS_SORCERER:
		case CLASS_MAGIC_EATER:
		{
			/* Very bad (light) sensing */
			if (0 != randint0(240000L / (plev + 5))) return;

			break;
		}

		case CLASS_PRIEST:
		case CLASS_BARD:
		{
			/* Good (light) sensing */
			if (0 != randint0(10000L / (plev * plev + 40))) return;

			break;
		}

		case CLASS_ROGUE:
		case CLASS_NINJA:
		{
			/* Okay sensing */
			if (0 != randint0(20000L / (plev * plev + 40))) return;

			/* Heavy sensing */
			heavy = TRUE;

			break;
		}

		case CLASS_RANGER:
		{
			/* Bad sensing */
			if (0 != randint0(95000L / (plev * plev + 40))) return;

			/* Changed! */
			heavy = TRUE;

			break;
		}

		case CLASS_PALADIN:
		case CLASS_SNIPER:
		{
			/* Bad sensing */
			if (0 != randint0(77777L / (plev * plev + 40))) return;

			/* Heavy sensing */
			heavy = TRUE;

			break;
		}

		case CLASS_WARRIOR_MAGE:
		case CLASS_RED_MAGE:
		{
			/* Bad sensing */
			if (0 != randint0(75000L / (plev * plev + 40))) return;

			break;
		}

		case CLASS_MINDCRAFTER:
		case CLASS_IMITATOR:
		case CLASS_BLUE_MAGE:
		case CLASS_MIRROR_MASTER:
		{
			/* Bad sensing */
			if (0 != randint0(55000L / (plev * plev + 40))) return;

			break;
		}

		case CLASS_CHAOS_WARRIOR:
		{
			/* Bad sensing */
			if (0 != randint0(80000L / (plev * plev + 40))) return;

			/* Changed! */
			heavy = TRUE;

			break;
		}

		case CLASS_MONK:
		case CLASS_FORCETRAINER:
		{
			/* Okay sensing */
			if (0 != randint0(20000L / (plev * plev + 40))) return;

			break;
		}

		case CLASS_TOURIST:
		{
			/* Good sensing */
			if (0 != randint0(20000L / ((plev+50)*(plev+50)))) return;

			/* Heavy sensing */
			heavy = TRUE;

			break;
		}

		case CLASS_BEASTMASTER:
		{
			/* Bad sensing */
			if (0 != randint0(65000L / (plev * plev + 40))) return;

			break;
		}
		case CLASS_BERSERKER:
		{
			/* Heavy sensing */
			heavy = TRUE;

			break;
		}
	}

	if (compare_virtue(creature_ptr, V_KNOWLEDGE, 100, VIRTUE_LARGE)) heavy = TRUE;

	/*** Sense everything ***/

	/* Check everything */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		bool okay = FALSE;

		o_ptr = &creature_ptr->inventory_list[i];

		/* Skip empty slots */
		if (!o_ptr->k_idx) continue;

		/* Valid "tval" codes */
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

		/* Skip non-sense machines */
		if (!okay) continue;

		/* Occasional failure on creature_ptr->inventory_list items */
		if ((i < INVEN_RARM) && (0 != randint0(5))) continue;

		/* Good luck */
		if ((creature_ptr->muta3 & MUT3_GOOD_LUCK) && !randint0(13))
		{
			heavy = TRUE;
		}

		sense_inventory_aux(i, heavy);
	}
}

/*!
 * @brief 1プレイヤーターン毎に武器、防具以外の擬似鑑定が行われるかを判定する。
 * @return なし
 */
static void sense_inventory2(player_type *creature_ptr)
{
	INVENTORY_IDX i;
	PLAYER_LEVEL plev = creature_ptr->lev;
	object_type *o_ptr;


	/*** Check for "sensing" ***/

	/* No sensing when confused */
	if (creature_ptr->confused) return;

	/* Analyze the class */
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
			/* Very bad (light) sensing */
			if (0 != randint0(240000L / (plev + 5))) return;

			break;
		}

		case CLASS_RANGER:
		case CLASS_WARRIOR_MAGE:
		case CLASS_RED_MAGE:
		case CLASS_MONK:
		{
			/* Bad sensing */
			if (0 != randint0(95000L / (plev * plev + 40))) return;

			break;
		}

		case CLASS_PRIEST:
		case CLASS_BARD:
		case CLASS_ROGUE:
		case CLASS_FORCETRAINER:
		case CLASS_MINDCRAFTER:
		{
			/* Good sensing */
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
			/* Good sensing */
			if (0 != randint0(9000L / (plev * plev + 40))) return;

			break;
		}

		case CLASS_TOURIST:
		{
			/* Good sensing */
			if (0 != randint0(20000L / ((plev+50)*(plev+50)))) return;

			break;
		}
	}

	/*** Sense everything ***/

	/* Check everything */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		bool okay = FALSE;

		o_ptr = &creature_ptr->inventory_list[i];

		/* Skip empty slots */
		if (!o_ptr->k_idx) continue;

		/* Valid "tval" codes */
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

		/* Skip non-sense machines */
		if (!okay) continue;

		/* Occasional failure on creature_ptr->inventory_list items */
		if ((i < INVEN_RARM) && (0 != randint0(5))) continue;

		sense_inventory_aux(i, TRUE);
	}
}

/*!
 * @brief パターン終点到達時のテレポート処理を行う
 * @return なし
 */
static void pattern_teleport(player_type *creature_ptr)
{
	DEPTH min_level = 0;
	DEPTH max_level = 99;

	/* Ask for level */
	if (get_check(_("他の階にテレポートしますか？", "Teleport level? ")))
	{
		char ppp[80];
		char tmp_val[160];

		/* Only downward in ironman mode */
		if (ironman_downward)
			min_level = creature_ptr->current_floor_ptr->dun_level;

		/* Maximum level */
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

		/* Prompt */
		sprintf(ppp, _("テレポート先:(%d-%d)", "Teleport to level (%d-%d): "), (int)min_level, (int)max_level);

		/* Default */
		sprintf(tmp_val, "%d", (int)creature_ptr->current_floor_ptr->dun_level);

		/* Ask for a level */
		if (!get_string(ppp, tmp_val, 10)) return;

		/* Extract request */
		command_arg = (COMMAND_ARG)atoi(tmp_val);
	}
	else if (get_check(_("通常テレポート？", "Normal teleport? ")))
	{
		teleport_player(creature_ptr, 200, 0L);
		return;
	}
	else
	{
		return;
	}
	if (command_arg < min_level) command_arg = (COMMAND_ARG)min_level;
	if (command_arg > max_level) command_arg = (COMMAND_ARG)max_level;

	/* Accept request */
	msg_format(_("%d 階にテレポートしました。", "You teleport to dungeon level %d."), command_arg);

	if (autosave_l) do_cmd_save_game(TRUE);

	/* Change level */
	creature_ptr->current_floor_ptr->dun_level = command_arg;

	leave_quest_check();

	if (record_stair) exe_write_diary(creature_ptr, NIKKI_PAT_TELE, 0, NULL);

	creature_ptr->current_floor_ptr->inside_quest = 0;
	free_turn(creature_ptr);

	/*
	 * Clear all saved floors
	 * and create a first saved floor
	 */
	prepare_change_floor_mode(CFM_FIRST_FLOOR);
	creature_ptr->leaving = TRUE;
}

/*!
 * @brief 各種パターン地形上の特別な処理 / Returns TRUE if we are on the Pattern...
 * @return 実際にパターン地形上にプレイヤーが居た場合はTRUEを返す。
 */
static bool pattern_effect(player_type *creature_ptr)
{
	int pattern_type;

	if (!pattern_tile(creature_ptr->y, creature_ptr->x)) return FALSE;

	if ((PRACE_IS_(creature_ptr, RACE_AMBERITE)) &&
	    (creature_ptr->cut > 0) && one_in_(10))
	{
		wreck_the_pattern(creature_ptr);
	}

	pattern_type = f_info[p_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].feat].subtype;

	switch (pattern_type)
	{
	case PATTERN_TILE_END:
		(void)set_image(creature_ptr, 0);
		(void)restore_all_status(creature_ptr);
		(void)restore_level(creature_ptr);
		(void)cure_critical_wounds(creature_ptr, 1000);

		cave_set_feat(creature_ptr->current_floor_ptr, creature_ptr->y, creature_ptr->x, feat_pattern_old);
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
	HIT_POINT new_chp;
	u32b new_chp_frac;
	HIT_POINT old_chp;

	if (creature_ptr->special_defense & KATA_KOUKIJIN) return;
	if (creature_ptr->action == ACTION_HAYAGAKE) return;

	/* Save the old hitpoints */
	old_chp = creature_ptr->chp;

	/*
	 * Extract the new hitpoints
	 *
	 * 'percent' is the Regen factor in unit (1/2^16)
	 */
	new_chp = 0;
	new_chp_frac = (creature_ptr->mhp * percent + PY_REGEN_HPBASE);

	/* Convert the unit (1/2^16) to (1/2^32) */
	s64b_LSHIFT(new_chp, new_chp_frac, 16);

	/* Regenerating */
	s64b_add(&(creature_ptr->chp), &(creature_ptr->chp_frac), new_chp, new_chp_frac);


	/* Fully healed */
	if (0 < s64b_cmp(creature_ptr->chp, creature_ptr->chp_frac, creature_ptr->mhp, 0))
	{
		creature_ptr->chp = creature_ptr->mhp;
		creature_ptr->chp_frac = 0;
	}

	/* Notice changes */
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
		/* PY_REGEN_NORMAL is the Regen factor in unit (1/2^16) */
		s32b decay = 0;
		u32b decay_frac = (creature_ptr->msp * 32 * PY_REGEN_NORMAL + PY_REGEN_MNBASE);

		/* Convert the unit (1/2^16) to (1/2^32) */
		s64b_LSHIFT(decay, decay_frac, 16);

		/* Decay */
		s64b_sub(&(creature_ptr->csp), &(creature_ptr->csp_frac), decay, decay_frac);

		/* Stop decaying */
		if (creature_ptr->csp < creature_ptr->msp)
		{
			creature_ptr->csp = creature_ptr->msp;
			creature_ptr->csp_frac = 0;
		}
	}

	/* Regenerating mana (unless the player has excess mana) */
	else if (regen_rate > 0)
	{
		/* (percent/100) is the Regen factor in unit (1/2^16) */
		MANA_POINT new_mana = 0;
		u32b new_mana_frac = (creature_ptr->msp * regen_rate / 100 + PY_REGEN_MNBASE);

		/* Convert the unit (1/2^16) to (1/2^32) */
		s64b_LSHIFT(new_mana, new_mana_frac, 16);

		/* Regenerate */
		s64b_add(&(creature_ptr->csp), &(creature_ptr->csp_frac), new_mana, new_mana_frac);

		/* Must set frac to zero even if equal */
		if (creature_ptr->csp >= creature_ptr->msp)
		{
			creature_ptr->csp = creature_ptr->msp;
			creature_ptr->csp_frac = 0;
		}
	}


	/* Reduce mana (even when the player has excess mana) */
	if (regen_rate < 0)
	{
		/* PY_REGEN_NORMAL is the Regen factor in unit (1/2^16) */
		s32b reduce_mana = 0;
		u32b reduce_mana_frac = (creature_ptr->msp * (-1) * regen_rate / 100 + PY_REGEN_MNBASE);

		/* Convert the unit (1/2^16) to (1/2^32) */
		s64b_LSHIFT(reduce_mana, reduce_mana_frac, 16);

		/* Reduce mana */
		s64b_sub(&(creature_ptr->csp), &(creature_ptr->csp_frac), reduce_mana, reduce_mana_frac);

		/* Check overflow */
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
	int i;
	int dev = 30;
	int mult = (dev + adj_mag_mana[creature_ptr->stat_ind[A_INT]]); /* x1 to x2 speed bonus for recharging */

	for (i = 0; i < EATER_EXT*2; i++)
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
	for (i = EATER_EXT*2; i < EATER_EXT*3; i++)
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
 * @return なし
 * @note Should probably be done during monster turns.
 */
static void regen_monsters(void)
{
	int i, frac;


	/* Regenerate everyone */
	for (i = 1; i < p_ptr->current_floor_ptr->m_max; i++)
	{
		/* Check the i'th monster */
		monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		if (!monster_is_valid(m_ptr)) continue;

		/* Allow regeneration (if needed) */
		if (m_ptr->hp < m_ptr->maxhp)
		{
			/* Hack -- Base regeneration */
			frac = m_ptr->maxhp / 100;

			/* Hack -- Minimal regeneration rate */
			if (!frac) if (one_in_(2)) frac = 1;

			/* Hack -- Some monsters regenerate quickly */
			if (r_ptr->flags2 & RF2_REGENERATE) frac *= 2;

			/* Hack -- Regenerate */
			m_ptr->hp += frac;

			/* Do not over-regenerate */
			if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

			/* Redraw (later) if needed */
			if (p_ptr->health_who == i) p_ptr->redraw |= (PR_HEALTH);
			if (p_ptr->riding == i) p_ptr->redraw |= (PR_UHEALTH);
		}
	}
}


/*!
 * @brief 30ゲームターン毎のボール中モンスターのHP自然回復処理 / Regenerate the captured monsters (once per 30 game turns)
 * @return なし
 * @note Should probably be done during monster turns.
 */
static void regen_captured_monsters(void)
{
	int i, frac;
	bool heal = FALSE;

	/* Regenerate everyone */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		monster_race *r_ptr;
		object_type *o_ptr = &p_ptr->inventory_list[i];

		if (!o_ptr->k_idx) continue;
		if (o_ptr->tval != TV_CAPTURE) continue;
		if (!o_ptr->pval) continue;

		heal = TRUE;

		r_ptr = &r_info[o_ptr->pval];

		/* Allow regeneration (if needed) */
		if (o_ptr->xtra4 < o_ptr->xtra5)
		{
			/* Hack -- Base regeneration */
			frac = o_ptr->xtra5 / 100;

			/* Hack -- Minimal regeneration rate */
			if (!frac) if (one_in_(2)) frac = 1;

			/* Hack -- Some monsters regenerate quickly */
			if (r_ptr->flags2 & RF2_REGENERATE) frac *= 2;

			/* Hack -- Regenerate */
			o_ptr->xtra4 += (XTRA16)frac;

			/* Do not over-regenerate */
			if (o_ptr->xtra4 > o_ptr->xtra5) o_ptr->xtra4 = o_ptr->xtra5;
		}
	}

	if (heal)
	{
		/* Combine pack */
		p_ptr->update |= (PU_COMBINE);
		p_ptr->window |= (PW_INVEN);
		p_ptr->window |= (PW_EQUIP);
		wild_regen = 20;
	}
}

/*!
 * @brief 寿命つき光源の警告メッセージ処理
 * @param o_ptr 現在光源として使っているオブジェクトの構造体参照ポインタ
 * @return なし
 */
static void notice_lite_change(player_type *creature_ptr, object_type *o_ptr)
{
	/* Hack -- notice interesting fuel steps */
	if ((o_ptr->xtra4 < 100) || (!(o_ptr->xtra4 % 100)))
	{
		creature_ptr->window |= (PW_EQUIP);
	}

	/* Hack -- Special treatment when blind */
	if (creature_ptr->blind)
	{
		/* Hack -- save some light for later */
		if (o_ptr->xtra4 == 0) o_ptr->xtra4++;
	}

	/* The light is now out */
	else if (o_ptr->xtra4 == 0)
	{
		disturb(creature_ptr, FALSE, TRUE);
		msg_print(_("明かりが消えてしまった！", "Your light has gone out!"));

		/* Recalculate torch radius */
		creature_ptr->update |= (PU_TORCH);

		/* Some ego light lose its effects without fuel */
		creature_ptr->update |= (PU_BONUS);
	}

	/* The light is getting dim */
	else if (o_ptr->name2 == EGO_LITE_LONG)
	{
		if ((o_ptr->xtra4 < 50) && (!(o_ptr->xtra4 % 5))
		    && (current_world_ptr->game_turn % (TURNS_PER_TICK*2)))
		{
			if (disturb_minor) disturb(creature_ptr, FALSE, TRUE);
			msg_print(_("明かりが微かになってきている。", "Your light is growing faint."));
		}
	}

	/* The light is getting dim */
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
static void recharged_notice(object_type *o_ptr)
{
	GAME_TEXT o_name[MAX_NLEN];

	concptr s;

	/* No inscription */
	if (!o_ptr->inscription) return;

	/* Find a '!' */
	s = my_strchr(quark_str(o_ptr->inscription), '!');

	/* Process notification request. */
	while (s)
	{
		/* Find another '!' */
		if (s[1] == '!')
		{
			/* Describe (briefly) */
			object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

			/* Notify the player */
#ifdef JP
			msg_format("%sは再充填された。", o_name);
#else
			if (o_ptr->number > 1)
				msg_format("Your %s are recharged.", o_name);
			else
				msg_format("Your %s is recharged.", o_name);
#endif

			disturb(p_ptr, FALSE, FALSE);

			/* Done. */
			return;
		}

		/* Keep looking for '!'s */
		s = my_strchr(s + 1, '!');
	}
}

/*!
 * @brief プレイヤーの歌に関する継続処理
 * @return なし
 */
static void check_music(player_type *caster_ptr)
{
	const magic_type *s_ptr;
	int spell;
	MANA_POINT need_mana;
	u32b need_mana_frac;

	/* Music singed by player */
	if (caster_ptr->pclass != CLASS_BARD) return;
	if (!SINGING_SONG_EFFECT(caster_ptr) && !INTERUPTING_SONG_EFFECT(caster_ptr)) return;

	if (caster_ptr->anti_magic)
	{
		stop_singing(caster_ptr);
		return;
	}

	spell = SINGING_SONG_ID(caster_ptr);
	s_ptr = &technic_info[REALM_MUSIC - MIN_TECHNIC][spell];

	need_mana = mod_need_mana(s_ptr->smana, spell, REALM_MUSIC);
	need_mana_frac = 0;

	/* Divide by 2 */
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
	else if(caster_ptr->spell_exp[spell] < SPELL_EXP_SKILLED)
	{ if (one_in_(2) && (caster_ptr->current_floor_ptr->dun_level > 4) && ((caster_ptr->current_floor_ptr->dun_level + 10) > caster_ptr->lev)) caster_ptr->spell_exp[spell] += 1; }
	else if(caster_ptr->spell_exp[spell] < SPELL_EXP_EXPERT)
	{ if (one_in_(5) && ((caster_ptr->current_floor_ptr->dun_level + 5) > caster_ptr->lev) && ((caster_ptr->current_floor_ptr->dun_level + 5) > s_ptr->slevel)) caster_ptr->spell_exp[spell] += 1; }
	else if(caster_ptr->spell_exp[spell] < SPELL_EXP_MASTER)
	{ if (one_in_(5) && ((caster_ptr->current_floor_ptr->dun_level + 5) > caster_ptr->lev) && (caster_ptr->current_floor_ptr->dun_level > s_ptr->slevel)) caster_ptr->spell_exp[spell] += 1; }

	/* Do any effects of continual song */
	exe_spell(caster_ptr, REALM_MUSIC, spell, SPELL_CONT);
}

/*!
 * @brief 現在呪いを保持している装備品を一つランダムに探し出す / Choose one of items that have cursed flag
 * @param flag 探し出したい呪いフラグ配列
 * @return 該当の呪いが一つでもあった場合にランダムに選ばれた装備品のオブジェクト構造体参照ポインタを返す。\n
 * 呪いがない場合NULLを返す。
 */
static object_type *choose_cursed_obj_name(BIT_FLAGS flag)
{
	int i;
	int choices[INVEN_TOTAL-INVEN_RARM];
	int number = 0;

	/* Paranoia -- Player has no warning-item */
	if (!(p_ptr->cursed & flag)) return NULL;

	/* Search Inventry */
	for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr = &p_ptr->inventory_list[i];

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
					(flag == TRC_SLOW_REGEN) )
		{
			u32b cf = 0L;
			BIT_FLAGS flgs[TR_FLAG_SIZE];
			object_flags(o_ptr, flgs);
			switch (flag)
			{
			  case TRC_ADD_L_CURSE	: cf = TR_ADD_L_CURSE; break;
			  case TRC_ADD_H_CURSE	: cf = TR_ADD_H_CURSE; break;
			  case TRC_DRAIN_HP		: cf = TR_DRAIN_HP; break;
			  case TRC_DRAIN_MANA	: cf = TR_DRAIN_MANA; break;
			  case TRC_CALL_ANIMAL	: cf = TR_CALL_ANIMAL; break;
			  case TRC_CALL_DEMON	: cf = TR_CALL_DEMON; break;
			  case TRC_CALL_DRAGON	: cf = TR_CALL_DRAGON; break;
			  case TRC_CALL_UNDEAD	: cf = TR_CALL_UNDEAD; break;
			  case TRC_COWARDICE	: cf = TR_COWARDICE; break;
			  case TRC_LOW_MELEE	: cf = TR_LOW_MELEE; break;
			  case TRC_LOW_AC		: cf = TR_LOW_AC; break;
			  case TRC_LOW_MAGIC	: cf = TR_LOW_MAGIC; break;
			  case TRC_FAST_DIGEST	: cf = TR_FAST_DIGEST; break;
			  case TRC_SLOW_REGEN	: cf = TR_SLOW_REGEN; break;
			  default 				: break;
			}
			if (have_flag(flgs, cf))
			{
				choices[number] = i;
				number++;
			}
		}
	}

	/* Choice one of them */
	return (&p_ptr->inventory_list[choices[randint0(number)]]);
}

static void process_world_aux_digestion(player_type *creature_ptr)
{
	if (!creature_ptr->phase_out)
	{
		/* Digest quickly when gorged */
		if (creature_ptr->food >= PY_FOOD_MAX)
		{
			/* Digest a lot of food */
			(void)set_food(creature_ptr, creature_ptr->food - 100);
		}

		/* Digest normally -- Every 50 game turns */
		else if (!(current_world_ptr->game_turn % (TURNS_PER_TICK * 5)))
		{
			/* Basic digestion rate based on speed */
			int digestion = SPEED_TO_ENERGY(creature_ptr->pspeed);

			/* Regeneration takes more food */
			if (creature_ptr->regenerate)
				digestion += 20;
			if (creature_ptr->special_defense & (KAMAE_MASK | KATA_MASK))
				digestion += 20;
			if (creature_ptr->cursed & TRC_FAST_DIGEST)
				digestion += 30;

			/* Slow digestion takes less food */
			if (creature_ptr->slow_digest)
				digestion -= 5;

			/* Minimal digestion */
			if (digestion < 1) digestion = 1;
			/* Maximal digestion */
			if (digestion > 100) digestion = 100;

			/* Digest some food */
			(void)set_food(creature_ptr, creature_ptr->food - digestion);
		}


		/* Getting Faint */
		if ((creature_ptr->food < PY_FOOD_FAINT))
		{
			/* Faint occasionally */
			if (!creature_ptr->paralyzed && (randint0(100) < 10))
			{
				msg_print(_("あまりにも空腹で気絶してしまった。", "You faint from the lack of food."));
				disturb(creature_ptr, TRUE, TRUE);

				/* Hack -- faint (bypass free action) */
				(void)set_paralyzed(creature_ptr, creature_ptr->paralyzed + 1 + randint0(5));
			}

			/* Starve to death (slowly) */
			if (creature_ptr->food < PY_FOOD_STARVE)
			{
				/* Calculate damage */
				HIT_POINT dam = (PY_FOOD_STARVE - creature_ptr->food) / 10;

				if (!IS_INVULN(creature_ptr)) take_hit(creature_ptr, DAMAGE_LOSELIFE, dam, _("空腹", "starvation"), -1);
			}
		}
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

	/* Default regeneration */
	int regen_amount = PY_REGEN_NORMAL;


	/*** Damage over Time ***/

	/* Take damage from poison */
	if (creature_ptr->poisoned && !IS_INVULN(creature_ptr))
	{
		take_hit(creature_ptr, DAMAGE_NOESCAPE, 1, _("毒", "poison"), -1);
	}

	/* Take damage from cuts */
	if (creature_ptr->cut && !IS_INVULN(creature_ptr))
	{
		HIT_POINT dam;

		/* Mortal wound or Deep Gash */
		if (creature_ptr->cut > 1000)
		{
			dam = 200;
		}

		else if (creature_ptr->cut > 200)
		{
			dam = 80;
		}

		/* Severe cut */
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

		/* Other cuts */
		else
		{
			dam = 1;
		}

		take_hit(creature_ptr, DAMAGE_NOESCAPE, dam, _("致命傷", "a fatal wound"), -1);
	}

	/* (Vampires) Take damage from sunlight */
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
			object_type * o_ptr = &creature_ptr->inventory_list[INVEN_LITE];
			GAME_TEXT o_name [MAX_NLEN];
			char ouch [MAX_NLEN+40];

			/* Get an object description */
			object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
			msg_format(_("%sがあなたのアンデッドの肉体を焼き焦がした！", "The %s scorches your undead flesh!"), o_name);

			cave_no_regen = TRUE;

			/* Get an object description */
			object_desc(o_name, o_ptr, OD_NAME_ONLY);
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
			if(PRACE_IS_(creature_ptr, RACE_ENT)) damage += damage / 3;
			if(creature_ptr->resist_fire) damage = damage / 3;
			if(IS_OPPOSE_FIRE()) damage = damage / 3;
			if(creature_ptr->levitation) damage = damage / 5;

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
			if (IS_OPPOSE_COLD()) damage = damage / 3;
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
			if (IS_OPPOSE_ELEC()) damage = damage / 3;
			if (creature_ptr->levitation) damage = damage / 5;

			damage = damage / 100 + (randint0(100) < (damage % 100));

			if (creature_ptr->levitation)
			{
				msg_print(_("電撃を受けた！", "The electric shocks you!"));
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
			if (IS_OPPOSE_ACID()) damage = damage / 3;
			if (creature_ptr->levitation) damage = damage / 5;

			damage = damage / 100 + (randint0(100) < (damage % 100));

			if (creature_ptr->levitation)
			{
				msg_print(_("酸が飛び散った！", "The acid melt you!"));
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
			if (IS_OPPOSE_POIS()) damage = damage / 3;
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
			if (IS_OPPOSE_FIRE()) damage = damage / 3;
			msg_print(_("熱い！", "It's hot!"));
			take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, _("炎のオーラ", "Fire aura"), -1);
		}
		if ((r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].flags2 & RF2_AURA_ELEC) && !creature_ptr->immune_elec)
		{
			damage = r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].level / 2;
			if (PRACE_IS_(creature_ptr, RACE_ANDROID)) damage += damage / 3;
			if (creature_ptr->resist_elec) damage = damage / 3;
			if (IS_OPPOSE_ELEC()) damage = damage / 3;
			msg_print(_("痛い！", "It hurts!"));
			take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, _("電気のオーラ", "Elec aura"), -1);
		}
		if ((r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].flags3 & RF3_AURA_COLD) && !creature_ptr->immune_cold)
		{
			damage = r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].level / 2;
			if (creature_ptr->resist_cold) damage = damage / 3;
			if (IS_OPPOSE_COLD()) damage = damage / 3;
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


	/*** handle regeneration ***/

	/* Getting Weak */
	if (creature_ptr->food < PY_FOOD_WEAK)
	{
		/* Lower regeneration */
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

	/* Are we walking the pattern? */
	if (pattern_effect(creature_ptr))
	{
		cave_no_regen = TRUE;
	}
	else
	{
		/* Regeneration ability */
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


	/* Searching or Resting */
	if ((creature_ptr->action == ACTION_SEARCH) || (creature_ptr->action == ACTION_REST))
	{
		regen_amount = regen_amount * 2;
	}

	upkeep_factor = calculate_upkeep(creature_ptr);

	/* No regeneration while special action */
	if ((creature_ptr->action == ACTION_LEARN) ||
	    (creature_ptr->action == ACTION_HAYAGAKE) ||
	    (creature_ptr->special_defense & KATA_KOUKIJIN))
	{
		upkeep_factor += 100;
	}

	/* Regenerate the mana */
	regenmana(creature_ptr, upkeep_factor, regen_amount);


	/* Recharge magic eater's power */
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

	/* Poisoned or cut yields no healing */
	if (creature_ptr->poisoned) regen_amount = 0;
	if (creature_ptr->cut) regen_amount = 0;

	/* Special floor -- Pattern, in a wall -- yields no healing */
	if (cave_no_regen) regen_amount = 0;

	regen_amount = (regen_amount * creature_ptr->mutant_regenerate_mod) / 100;

	/* Regenerate Hit Points if needed */
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

	/*** Timeout Various Things ***/

	/* Mimic */
	if (creature_ptr->tim_mimic)
	{
		(void)set_mimic(creature_ptr, creature_ptr->tim_mimic - 1, creature_ptr->mimic_form, TRUE);
	}

	/* Hack -- Hallucinating */
	if (creature_ptr->image)
	{
		(void)set_image(creature_ptr, creature_ptr->image - dec_count);
	}

	/* Blindness */
	if (creature_ptr->blind)
	{
		(void)set_blind(creature_ptr, creature_ptr->blind - dec_count);
	}

	/* Times see-invisible */
	if (creature_ptr->tim_invis)
	{
		(void)set_tim_invis(creature_ptr, creature_ptr->tim_invis - 1, TRUE);
	}

	if (creature_ptr->suppress_multi_reward)
	{
		creature_ptr->suppress_multi_reward = FALSE;
	}

	/* Timed esp */
	if (creature_ptr->tim_esp)
	{
		(void)set_tim_esp(creature_ptr, creature_ptr->tim_esp - 1, TRUE);
	}

	/* Timed temporary elemental brands. -LM- */
	if (creature_ptr->ele_attack)
	{
		creature_ptr->ele_attack--;

		/* Clear all temporary elemental brands. */
		if (!creature_ptr->ele_attack) set_ele_attack(creature_ptr, 0, 0);
	}

	/* Timed temporary elemental immune. -LM- */
	if (creature_ptr->ele_immune)
	{
		creature_ptr->ele_immune--;

		/* Clear all temporary elemental brands. */
		if (!creature_ptr->ele_immune) set_ele_immune(creature_ptr, 0, 0);
	}

	/* Timed infra-vision */
	if (creature_ptr->tim_infra)
	{
		(void)set_tim_infra(creature_ptr, creature_ptr->tim_infra - 1, TRUE);
	}

	/* Timed stealth */
	if (creature_ptr->tim_stealth)
	{
		(void)set_tim_stealth(creature_ptr, creature_ptr->tim_stealth - 1, TRUE);
	}

	/* Timed levitation */
	if (creature_ptr->tim_levitation)
	{
		(void)set_tim_levitation(creature_ptr, creature_ptr->tim_levitation - 1, TRUE);
	}

	/* Timed sh_touki */
	if (creature_ptr->tim_sh_touki)
	{
		(void)set_tim_sh_touki(creature_ptr, creature_ptr->tim_sh_touki - 1, TRUE);
	}

	/* Timed sh_fire */
	if (creature_ptr->tim_sh_fire)
	{
		(void)set_tim_sh_fire(creature_ptr, creature_ptr->tim_sh_fire - 1, TRUE);
	}

	/* Timed sh_holy */
	if (creature_ptr->tim_sh_holy)
	{
		(void)set_tim_sh_holy(creature_ptr, creature_ptr->tim_sh_holy - 1, TRUE);
	}

	/* Timed eyeeye */
	if (creature_ptr->tim_eyeeye)
	{
		(void)set_tim_eyeeye(creature_ptr, creature_ptr->tim_eyeeye - 1, TRUE);
	}

	/* Timed resist-magic */
	if (creature_ptr->resist_magic)
	{
		(void)set_resist_magic(creature_ptr, creature_ptr->resist_magic - 1, TRUE);
	}

	/* Timed regeneration */
	if (creature_ptr->tim_regen)
	{
		(void)set_tim_regen(creature_ptr, creature_ptr->tim_regen - 1, TRUE);
	}

	/* Timed resist nether */
	if (creature_ptr->tim_res_nether)
	{
		(void)set_tim_res_nether(creature_ptr, creature_ptr->tim_res_nether - 1, TRUE);
	}

	/* Timed resist time */
	if (creature_ptr->tim_res_time)
	{
		(void)set_tim_res_time(creature_ptr, creature_ptr->tim_res_time - 1, TRUE);
	}

	/* Timed reflect */
	if (creature_ptr->tim_reflect)
	{
		(void)set_tim_reflect(creature_ptr, creature_ptr->tim_reflect - 1, TRUE);
	}

	/* Multi-shadow */
	if (creature_ptr->multishadow)
	{
		(void)set_multishadow(creature_ptr, creature_ptr->multishadow - 1, TRUE);
	}

	/* Timed Robe of dust */
	if (creature_ptr->dustrobe)
	{
		(void)set_dustrobe(creature_ptr, creature_ptr->dustrobe - 1, TRUE);
	}

	/* Timed infra-vision */
	if (creature_ptr->kabenuke)
	{
		(void)set_kabenuke(creature_ptr, creature_ptr->kabenuke - 1, TRUE);
	}

	/* Paralysis */
	if (creature_ptr->paralyzed)
	{
		(void)set_paralyzed(creature_ptr, creature_ptr->paralyzed - dec_count);
	}

	/* Confusion */
	if (creature_ptr->confused)
	{
		(void)set_confused(creature_ptr, creature_ptr->confused - dec_count);
	}

	/* Afraid */
	if (creature_ptr->afraid)
	{
		(void)set_afraid(creature_ptr, creature_ptr->afraid - dec_count);
	}

	/* Fast */
	if (creature_ptr->fast)
	{
		(void)set_fast(creature_ptr, creature_ptr->fast - 1, TRUE);
	}

	/* Slow */
	if (creature_ptr->slow)
	{
		(void)set_slow(creature_ptr, creature_ptr->slow - dec_count, TRUE);
	}

	/* Protection from evil */
	if (creature_ptr->protevil)
	{
		(void)set_protevil(creature_ptr, creature_ptr->protevil - 1, TRUE);
	}

	/* Invulnerability */
	if (creature_ptr->invuln)
	{
		(void)set_invuln(creature_ptr, creature_ptr->invuln - 1, TRUE);
	}

	/* Wraith form */
	if (creature_ptr->wraith_form)
	{
		(void)set_wraith_form(creature_ptr, creature_ptr->wraith_form - 1, TRUE);
	}

	/* Heroism */
	if (creature_ptr->hero)
	{
		(void)set_hero(creature_ptr, creature_ptr->hero - 1, TRUE);
	}

	/* Super Heroism */
	if (creature_ptr->shero)
	{
		(void)set_shero(creature_ptr, creature_ptr->shero - 1, TRUE);
	}

	/* Blessed */
	if (creature_ptr->blessed)
	{
		(void)set_blessed(creature_ptr, creature_ptr->blessed - 1, TRUE);
	}

	/* Shield */
	if (creature_ptr->shield)
	{
		(void)set_shield(creature_ptr, creature_ptr->shield - 1, TRUE);
	}

	/* Tsubureru */
	if (creature_ptr->tsubureru)
	{
		(void)set_tsubureru(creature_ptr, creature_ptr->tsubureru - 1, TRUE);
	}

	/* Magicdef */
	if (creature_ptr->magicdef)
	{
		(void)set_magicdef(creature_ptr, creature_ptr->magicdef - 1, TRUE);
	}

	/* Tsuyoshi */
	if (creature_ptr->tsuyoshi)
	{
		(void)set_tsuyoshi(creature_ptr, creature_ptr->tsuyoshi - 1, TRUE);
	}

	/* Oppose Acid */
	if (creature_ptr->oppose_acid)
	{
		(void)set_oppose_acid(creature_ptr, creature_ptr->oppose_acid - 1, TRUE);
	}

	/* Oppose Lightning */
	if (creature_ptr->oppose_elec)
	{
		(void)set_oppose_elec(creature_ptr, creature_ptr->oppose_elec - 1, TRUE);
	}

	/* Oppose Fire */
	if (creature_ptr->oppose_fire)
	{
		(void)set_oppose_fire(creature_ptr, creature_ptr->oppose_fire - 1, TRUE);
	}

	/* Oppose Cold */
	if (creature_ptr->oppose_cold)
	{
		(void)set_oppose_cold(creature_ptr, creature_ptr->oppose_cold - 1, TRUE);
	}

	/* Oppose Poison */
	if (creature_ptr->oppose_pois)
	{
		(void)set_oppose_pois(creature_ptr, creature_ptr->oppose_pois - 1, TRUE);
	}

	if (creature_ptr->ult_res)
	{
		(void)set_ultimate_res(creature_ptr, creature_ptr->ult_res - 1, TRUE);
	}

	/*** Poison and Stun and Cut ***/

	/* Poison */
	if (creature_ptr->poisoned)
	{
		int adjust = adj_con_fix[creature_ptr->stat_ind[A_CON]] + 1;

		/* Apply some healing */
		(void)set_poisoned(creature_ptr, creature_ptr->poisoned - adjust);
	}

	/* Stun */
	if (creature_ptr->stun)
	{
		int adjust = adj_con_fix[creature_ptr->stat_ind[A_CON]] + 1;

		/* Apply some healing */
		(void)set_stun(creature_ptr, creature_ptr->stun - adjust);
	}

	/* Cut */
	if (creature_ptr->cut)
	{
		int adjust = adj_con_fix[creature_ptr->stat_ind[A_CON]] + 1;

		/* Hack -- Truly "mortal" wound */
		if (creature_ptr->cut > 1000) adjust = 0;

		/* Apply some healing */
		(void)set_cut(creature_ptr,creature_ptr->cut - adjust);
	}
}


/*!
 * @brief 10ゲームターンが進行する毎に光源の寿命を減らす処理
 * / Handle burning fuel every 10 game turns
 * @return なし
 */
static void process_world_aux_light(player_type *creature_ptr)
{
	/* Check for light being wielded */
	object_type *o_ptr = &creature_ptr->inventory_list[INVEN_LITE];

	/* Burn some fuel in the current lite */
	if (o_ptr->tval == TV_LITE)
	{
		/* Hack -- Use some fuel (except on artifacts) */
		if (!(object_is_fixed_artifact(o_ptr) || o_ptr->sval == SV_LITE_FEANOR) && (o_ptr->xtra4 > 0))
		{
			/* Decrease life-span */
			if (o_ptr->name2 == EGO_LITE_LONG)
			{
				if (current_world_ptr->game_turn % (TURNS_PER_TICK*2)) o_ptr->xtra4--;
			}
			else o_ptr->xtra4--;

			/* Notice interesting fuel steps */
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
	/* No mutation with effects */
	if (!creature_ptr->muta2) return;

	/* No effect on monster arena */
	if (creature_ptr->phase_out) return;

	/* No effect on the global map */
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
				(void)teleport_player_aux(creature_ptr,100, TELEPORT_NONMAGICAL | TELEPORT_PASSIVE);
				wiz_dark(creature_ptr);
				msg_print(_("あなたは見知らぬ場所で目が醒めた...頭が痛い。", "You wake up somewhere with a sore head..."));
				msg_print(_("何も覚えていない。どうやってここに来たかも分からない！", "You can't remember a thing, or how you got here!"));
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
		(void)get_hack_dir(&dire);
		fire_ball(creature_ptr, GF_MANA, dire, creature_ptr->lev * 2, 3);
	}

	if ((creature_ptr->muta2 & MUT2_ATT_DEMON) && !creature_ptr->anti_magic && (randint1(6666) == 666))
	{
		bool pet = one_in_(6);
		BIT_FLAGS mode = PM_ALLOW_GROUP;

		if (pet) mode |= PM_FORCE_PET;
		else mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);

		if (summon_specific((pet ? -1 : 0), creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_DEMON, mode))
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

		banish_monsters(100);
		if (!creature_ptr->current_floor_ptr->dun_level && creature_ptr->town_num)
		{
			int n;

			/* Pick a random shop (except home) */
			do
			{
				n = randint0(MAX_STORES);
			}
			while ((n == STORE_HOME) || (n == STORE_MUSEUM));

			msg_print(_("店の主人が丘に向かって走っている！", "You see one of the shopkeepers running for the hills!"));
			store_shuffle(n);
		}
		msg_print(NULL);
	}

	if ((creature_ptr->muta2 & MUT2_EAT_LIGHT) && one_in_(3000))
	{
		object_type *o_ptr;

		msg_print(_("影につつまれた。", "A shadow passes over you."));
		msg_print(NULL);

		/* Absorb light from the current possition */
		if ((creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW)
		{
			hp_player(creature_ptr, 10);
		}

		o_ptr = &creature_ptr->inventory_list[INVEN_LITE];

		/* Absorb some fuel in the current lite */
		if (o_ptr->tval == TV_LITE)
		{
			/* Use some fuel (except on artifacts) */
			if (!object_is_fixed_artifact(o_ptr) && (o_ptr->xtra4 > 0))
			{
				/* Heal the player a bit */
				hp_player(creature_ptr, o_ptr->xtra4 / 20);

				/* Decrease life-span of lite */
				o_ptr->xtra4 /= 2;
				msg_print(_("光源からエネルギーを吸収した！", "You absorb energy from your light!"));

				/* Notice interesting fuel steps */
				notice_lite_change(creature_ptr, o_ptr);
			}
		}

		/*
		 * Unlite the area (radius 10) around player and
		 * do 50 points damage to every affected monster
		 */
		unlite_area(50, 10);
	}

	if ((creature_ptr->muta2 & MUT2_ATT_ANIMAL) && !creature_ptr->anti_magic && one_in_(7000))
	{
		bool pet = one_in_(3);
		BIT_FLAGS mode = PM_ALLOW_GROUP;

		if (pet) mode |= PM_FORCE_PET;
		else mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);

		if (summon_specific((pet ? -1 : 0), creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_ANIMAL, mode))
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

		if (summon_specific((pet ? -1 : 0), creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_DRAGON, mode))
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
		MONSTER_IDX monster;

		for (monster = 0; monster < creature_ptr->current_floor_ptr->m_max; monster++)
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
		INVENTORY_IDX slot = 0;
		object_type *o_ptr = NULL;

		disturb(creature_ptr, FALSE, TRUE);
		msg_print(_("足がもつれて転んだ！", "You trip over your own feet!"));
		take_hit(creature_ptr, DAMAGE_NOESCAPE, randint1(creature_ptr->wt / 6), _("転倒", "tripping"), -1);

		msg_print(NULL);
		if (has_melee_weapon(creature_ptr, INVEN_RARM))
		{
			slot = INVEN_RARM;
			o_ptr = &creature_ptr->inventory_list[INVEN_RARM];

			if (has_melee_weapon(creature_ptr, INVEN_LARM) && one_in_(2))
			{
				o_ptr = &creature_ptr->inventory_list[INVEN_LARM];
				slot = INVEN_LARM;
			}
		}
		else if (has_melee_weapon(creature_ptr, INVEN_LARM))
		{
			o_ptr = &creature_ptr->inventory_list[INVEN_LARM];
			slot = INVEN_LARM;
		}
		if (slot && !object_is_cursed(o_ptr))
		{
			msg_print(_("武器を落としてしまった！", "You drop your weapon!"));
			inven_drop(slot, 1);
		}
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
			int i, i_keep = 0, count = 0;

			/* Scan the equipment with random teleport ability */
			for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
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
			object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
			msg_format(_("%sがテレポートの能力を発動させようとしている。", "Your %s is activating teleportation."), o_name);
			if (get_check_strict(_("テレポートしますか？", "Teleport? "), CHECK_OKAY_CANCEL))
			{
				disturb(creature_ptr, FALSE, TRUE);
				teleport_player(creature_ptr, 50, 0L);
			}
			else
			{
				msg_format(_("%sに{.}(ピリオド)と銘を刻むと発動を抑制できます。", 
							 "You can inscribe {.} on your %s to disable random teleportation. "), o_name);
				disturb(creature_ptr, TRUE, TRUE);
			}
		}
		/* Make a chainsword noise */
		if ((creature_ptr->cursed & TRC_CHAINSWORD) && one_in_(CHAINSWORD_NOISE))
		{
			char noise[1024];
			if (!get_rnd_line(_("chainswd_j.txt", "chainswd.txt"), 0, noise))
				msg_print(noise);
			disturb(creature_ptr, FALSE, FALSE);
		}
		/* TY Curse */
		if ((creature_ptr->cursed & TRC_TY_CURSE) && one_in_(TY_CURSE_CHANCE))
		{
			int count = 0;
			(void)activate_ty_curse(creature_ptr, FALSE, &count);
		}
		/* Handle experience draining */
		if (creature_ptr->prace != RACE_ANDROID && ((creature_ptr->cursed & TRC_DRAIN_EXP) && one_in_(4)))
		{
			creature_ptr->exp -= (creature_ptr->lev + 1) / 2;
			if (creature_ptr->exp < 0) creature_ptr->exp = 0;
			creature_ptr->max_exp -= (creature_ptr->lev + 1) / 2;
			if (creature_ptr->max_exp < 0) creature_ptr->max_exp = 0;
			check_experience(creature_ptr);
		}
		/* Add light curse (Later) */
		if ((creature_ptr->cursed & TRC_ADD_L_CURSE) && one_in_(2000))
		{
			BIT_FLAGS new_curse;
			object_type *o_ptr;

			o_ptr = choose_cursed_obj_name(TRC_ADD_L_CURSE);

			new_curse = get_curse(0, o_ptr);
			if (!(o_ptr->curse_flags & new_curse))
			{
				GAME_TEXT o_name[MAX_NLEN];

				object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

				o_ptr->curse_flags |= new_curse;
				msg_format(_("悪意に満ちた黒いオーラが%sをとりまいた...", "There is a malignant black aura surrounding your %s..."), o_name);

				o_ptr->feeling = FEEL_NONE;

				creature_ptr->update |= (PU_BONUS);
			}
		}
		/* Add heavy curse (Later) */
		if ((creature_ptr->cursed & TRC_ADD_H_CURSE) && one_in_(2000))
		{
			BIT_FLAGS new_curse;
			object_type *o_ptr;

			o_ptr = choose_cursed_obj_name(TRC_ADD_H_CURSE);

			new_curse = get_curse(1, o_ptr);
			if (!(o_ptr->curse_flags & new_curse))
			{
				GAME_TEXT o_name[MAX_NLEN];

				object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

				o_ptr->curse_flags |= new_curse;
				msg_format(_("悪意に満ちた黒いオーラが%sをとりまいた...", "There is a malignant black aura surrounding your %s..."), o_name);
				o_ptr->feeling = FEEL_NONE;

				creature_ptr->update |= (PU_BONUS);
			}
		}
		/* Call animal */
		if ((creature_ptr->cursed & TRC_CALL_ANIMAL) && one_in_(2500))
		{
			if (summon_specific(0, creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_ANIMAL, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)))
			{
				GAME_TEXT o_name[MAX_NLEN];

				object_desc(o_name, choose_cursed_obj_name(TRC_CALL_ANIMAL), (OD_OMIT_PREFIX | OD_NAME_ONLY));
				msg_format(_("%sが動物を引き寄せた！", "Your %s have attracted an animal!"), o_name);
				disturb(creature_ptr, FALSE, TRUE);
			}
		}
		/* Call demon */
		if ((creature_ptr->cursed & TRC_CALL_DEMON) && one_in_(1111))
		{
			if (summon_specific(0, creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_DEMON, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)))
			{
				GAME_TEXT o_name[MAX_NLEN];

				object_desc(o_name, choose_cursed_obj_name(TRC_CALL_DEMON), (OD_OMIT_PREFIX | OD_NAME_ONLY));
				msg_format(_("%sが悪魔を引き寄せた！", "Your %s have attracted a demon!"), o_name);
				disturb(creature_ptr, FALSE, TRUE);
			}
		}
		/* Call dragon */
		if ((creature_ptr->cursed & TRC_CALL_DRAGON) && one_in_(800))
		{
			if (summon_specific(0, creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_DRAGON,
			    (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)))
			{
				GAME_TEXT o_name[MAX_NLEN];

				object_desc(o_name, choose_cursed_obj_name(TRC_CALL_DRAGON), (OD_OMIT_PREFIX | OD_NAME_ONLY));
				msg_format(_("%sがドラゴンを引き寄せた！", "Your %s have attracted an dragon!"), o_name);
				disturb(creature_ptr, FALSE, TRUE);
			}
		}
		/* Call undead */
		if ((creature_ptr->cursed & TRC_CALL_UNDEAD) && one_in_(1111))
		{
			if (summon_specific(0, creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_UNDEAD,
			    (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)))
			{
				GAME_TEXT o_name[MAX_NLEN];

				object_desc(o_name, choose_cursed_obj_name(TRC_CALL_UNDEAD), (OD_OMIT_PREFIX | OD_NAME_ONLY));
				msg_format(_("%sが死霊を引き寄せた！", "Your %s have attracted an undead!"), o_name);
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
		/* Teleport player */
		if ((creature_ptr->cursed & TRC_TELEPORT) && one_in_(200) && !creature_ptr->anti_tele)
		{
			disturb(creature_ptr, FALSE, TRUE);

			/* Teleport player */
			teleport_player(creature_ptr, 40, TELEPORT_PASSIVE);
		}
		/* Handle HP draining */
		if ((creature_ptr->cursed & TRC_DRAIN_HP) && one_in_(666))
		{
			GAME_TEXT o_name[MAX_NLEN];

			object_desc(o_name, choose_cursed_obj_name(TRC_DRAIN_HP), (OD_OMIT_PREFIX | OD_NAME_ONLY));
			msg_format(_("%sはあなたの体力を吸収した！", "Your %s drains HP from you!"), o_name);
			take_hit(creature_ptr, DAMAGE_LOSELIFE, MIN(creature_ptr->lev*2, 100), o_name, -1);
		}
		/* Handle mana draining */
		if ((creature_ptr->cursed & TRC_DRAIN_MANA) && creature_ptr->csp && one_in_(666))
		{
			GAME_TEXT o_name[MAX_NLEN];

			object_desc(o_name, choose_cursed_obj_name(TRC_DRAIN_MANA), (OD_OMIT_PREFIX | OD_NAME_ONLY));
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

	/* Rarely, take damage from the Jewel of Judgement */
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

	/* Process equipment */
	for (changed = FALSE, i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		/* Get the object */
		object_type *o_ptr = &creature_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;

		/* Recharge activatable objects */
		if (o_ptr->timeout > 0)
		{
			/* Recharge */
			o_ptr->timeout--;

			/* Notice changes */
			if (!o_ptr->timeout)
			{
				recharged_notice(o_ptr);
				changed = TRUE;
			}
		}
	}

	/* Notice changes */
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

		/* Examine all charging rods or stacks of charging rods. */
		if ((o_ptr->tval == TV_ROD) && (o_ptr->timeout))
		{
			/* Determine how many rods are charging. */
			TIME_EFFECT temp = (o_ptr->timeout + (k_ptr->pval - 1)) / k_ptr->pval;
			if (temp > o_ptr->number) temp = (TIME_EFFECT)o_ptr->number;

			/* Decrease timeout by that number. */
			o_ptr->timeout -= temp;

			/* Boundary control. */
			if (o_ptr->timeout < 0) o_ptr->timeout = 0;

			/* Notice changes, provide message if object is inscribed. */
			if (!(o_ptr->timeout))
			{
				recharged_notice(o_ptr);
				changed = TRUE;
			}

			/* One of the stack of rod is charged */
			else if (o_ptr->timeout % k_ptr->pval)
			{
				changed = TRUE;
			}
		}
	}

	/* Notice changes */
	if (changed)
	{
		creature_ptr->window |= (PW_INVEN);
		wild_regen = 20;
	}

	/* Process objects on floor */
	for (i = 1; i < p_ptr->current_floor_ptr->o_max; i++)
	{
		object_type *o_ptr = &p_ptr->current_floor_ptr->o_list[i];

		if (!OBJECT_IS_VALID(o_ptr)) continue;

		/* Recharge rods on the ground.  No messages. */
		if ((o_ptr->tval == TV_ROD) && (o_ptr->timeout))
		{
			/* Charge it */
			o_ptr->timeout -= (TIME_EFFECT)o_ptr->number;

			/* Boundary control. */
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
	/* Delayed Word-of-Recall */
	if (creature_ptr->word_recall)
	{
		/*
		 * HACK: Autosave BEFORE resetting the recall counter (rr9)
		 * The player is yanked up/down as soon as
		 * he loads the autosaved game.
		 */
		if (autosave_l && (creature_ptr->word_recall == 1) && !creature_ptr->phase_out)
			do_cmd_save_game(TRUE);

		/* Count down towards recall */
		creature_ptr->word_recall--;

		creature_ptr->redraw |= (PR_STATUS);

		/* Activate the recall */
		if (!creature_ptr->word_recall)
		{
			/* Disturbing! */
			disturb(creature_ptr, FALSE, TRUE);

			/* Determine the level */
			if (floor_ptr->dun_level || creature_ptr->current_floor_ptr->inside_quest || creature_ptr->enter_dungeon)
			{
				msg_print(_("上に引っ張りあげられる感じがする！", "You feel yourself yanked upwards!"));

				if (creature_ptr->dungeon_idx) creature_ptr->recall_dungeon = creature_ptr->dungeon_idx;
				if (record_stair)
					exe_write_diary(creature_ptr, NIKKI_RECALL, floor_ptr->dun_level, NULL);

				floor_ptr->dun_level = 0;
				creature_ptr->dungeon_idx = 0;

				leave_quest_check();
				leave_tower_check();

				creature_ptr->current_floor_ptr->inside_quest = 0;

				creature_ptr->leaving = TRUE;
			}
			else
			{
				msg_print(_("下に引きずり降ろされる感じがする！", "You feel yourself yanked downwards!"));

				creature_ptr->dungeon_idx = creature_ptr->recall_dungeon;

				if (record_stair)
					exe_write_diary(creature_ptr, NIKKI_RECALL, floor_ptr->dun_level, NULL);

				/* New depth */
				floor_ptr->dun_level = max_dlv[creature_ptr->dungeon_idx];
				if (floor_ptr->dun_level < 1) floor_ptr->dun_level = 1;

				/* Nightmare mode makes recall more dangerous */
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
					/* Save player position */
					creature_ptr->oldpx = creature_ptr->x;
					creature_ptr->oldpy = creature_ptr->y;
				}
				creature_ptr->wild_mode = FALSE;

				/*
				 * Clear all saved floors
				 * and create a first saved floor
				 */
				prepare_change_floor_mode(CFM_FIRST_FLOOR);
				creature_ptr->leaving = TRUE;

				if (creature_ptr->dungeon_idx == DUNGEON_ANGBAND)
				{
					int i;

					for (i = MIN_RANDOM_QUEST; i < MAX_RANDOM_QUEST + 1; i++)
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


	/* Delayed Alter reality */
	if (creature_ptr->alter_reality)
	{
		if (autosave_l && (creature_ptr->alter_reality == 1) && !creature_ptr->phase_out)
			do_cmd_save_game(TRUE);

		/* Count down towards alter */
		creature_ptr->alter_reality--;

		creature_ptr->redraw |= (PR_STATUS);

		/* Activate the alter reality */
		if (!creature_ptr->alter_reality)
		{
			/* Disturbing! */
			disturb(creature_ptr, FALSE, TRUE);

			/* Determine the level */
			if (!quest_number(floor_ptr->dun_level) && floor_ptr->dun_level)
			{
				msg_print(_("世界が変わった！", "The world changes!"));

				/*
				 * Clear all saved floors
				 * and create a first saved floor
				 */
				prepare_change_floor_mode(CFM_FIRST_FLOOR);
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
	int day, hour, min;

	const s32b A_DAY = TURNS_PER_TICK * TOWN_DAWN;
	s32b prev_turn_in_today = ((current_world_ptr->game_turn - TURNS_PER_TICK) % A_DAY + A_DAY / 4) % A_DAY;
	int prev_min = (1440 * prev_turn_in_today / A_DAY) % 60;
	
	extract_day_hour_min(&day, &hour, &min);

	/* Update dungeon feeling, and announce it if changed */
	update_dungeon_feeling(player_ptr, player_ptr->current_floor_ptr);

	/* 帰還無しモード時のレベルテレポバグ対策 / Fix for level teleport bugs on ironman_downward.*/
	if (ironman_downward && (player_ptr->dungeon_idx != DUNGEON_ANGBAND && player_ptr->dungeon_idx != 0))
	{
		player_ptr->current_floor_ptr->dun_level = 0;
		player_ptr->dungeon_idx = 0;
		prepare_change_floor_mode(CFM_FIRST_FLOOR | CFM_RAND_PLACE);
		player_ptr->current_floor_ptr->inside_arena = FALSE;
		player_ptr->wild_mode = FALSE;
		player_ptr->leaving = TRUE;
	}

	/*** Check monster arena ***/
	if (player_ptr->phase_out && !player_ptr->leaving)
	{
		int i2, j2;
		int win_m_idx = 0;
		int number_mon = 0;

		/* Count all hostile monsters */
		for (i2 = 0; i2 < player_ptr->current_floor_ptr->width; ++i2)
			for (j2 = 0; j2 < player_ptr->current_floor_ptr->height; j2++)
			{
				grid_type *g_ptr = &player_ptr->current_floor_ptr->grid_array[j2][i2];

				if ((g_ptr->m_idx > 0) && (g_ptr->m_idx != player_ptr->riding))
				{
					number_mon++;
					win_m_idx = g_ptr->m_idx;
				}
			}

		if (number_mon == 0)
		{
			msg_print(_("相打ちに終わりました。", "They have kill each other at the same time."));
			msg_print(NULL);
			player_ptr->energy_need = 0;
			update_gambling_monsters();
		}
		else if ((number_mon-1) == 0)
		{
			GAME_TEXT m_name[MAX_NLEN];
			monster_type *wm_ptr;

			wm_ptr = &player_ptr->current_floor_ptr->m_list[win_m_idx];

			monster_desc(m_name, wm_ptr, 0);
			msg_format(_("%sが勝利した！", "%s is winner!"), m_name);
			msg_print(NULL);

			if (win_m_idx == (sel_monster+1))
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
			update_gambling_monsters();
		}
		else if (current_world_ptr->game_turn - player_ptr->current_floor_ptr->generated_turn == 150 * TURNS_PER_TICK)
		{
			msg_print(_("申し分けありませんが、この勝負は引き分けとさせていただきます。", "This battle have ended in a draw."));
			player_ptr->au += kakekin;
			msg_print(NULL);
			player_ptr->energy_need = 0;
			update_gambling_monsters();
		}
	}

	/* Every 10 game turns */
	if (current_world_ptr->game_turn % TURNS_PER_TICK) return;

	/*** Attempt timed autosave ***/
	if (autosave_t && autosave_freq && !player_ptr->phase_out)
	{
		if (!(current_world_ptr->game_turn % ((s32b)autosave_freq * TURNS_PER_TICK)))
			do_cmd_save_game(TRUE);
	}

	if (player_ptr->current_floor_ptr->monster_noise && !ignore_unview)
	{
		msg_print(_("何かが聞こえた。", "You hear noise."));
	}

	/*** Handle the wilderness/town (sunshine) ***/

	/* While in town/wilderness */
	if (!player_ptr->current_floor_ptr->dun_level && !player_ptr->current_floor_ptr->inside_quest && !player_ptr->phase_out && !player_ptr->current_floor_ptr->inside_arena)
	{
		/* Hack -- Daybreak/Nighfall in town */
		if (!(current_world_ptr->game_turn % ((TURNS_PER_TICK * TOWN_DAWN) / 2)))
		{
			bool dawn;

			/* Check for dawn */
			dawn = (!(current_world_ptr->game_turn % (TURNS_PER_TICK * TOWN_DAWN)));

			if (dawn) day_break(player_ptr->current_floor_ptr);
			else night_falls(player_ptr->current_floor_ptr);

		}
	}

	/* While in the dungeon (vanilla_town or lite_town mode only) */
	else if ((vanilla_town || (lite_town && !player_ptr->current_floor_ptr->inside_quest && !player_ptr->phase_out && !player_ptr->current_floor_ptr->inside_arena)) && player_ptr->current_floor_ptr->dun_level)
	{
		/*** Shuffle the Storekeepers ***/

		/* Chance is only once a day (while in dungeon) */
		if (!(current_world_ptr->game_turn % (TURNS_PER_TICK * STORE_TICKS)))
		{
			/* Sometimes, shuffle the shop-keepers */
			if (one_in_(STORE_SHUFFLE))
			{
				int n;
				FEAT_IDX i;

				/* Pick a random shop (except home and museum) */
				do
				{
					n = randint0(MAX_STORES);
				}
				while ((n == STORE_HOME) || (n == STORE_MUSEUM));

				/* Check every feature */
				for (i = 1; i < max_f_idx; i++)
				{
					feature_type *f_ptr = &f_info[i];

					/* Skip empty index */
					if (!f_ptr->name) continue;

					/* Skip non-store features */
					if (!have_flag(f_ptr->flags, FF_STORE)) continue;

					/* Verify store type */
					if (f_ptr->subtype == n)
					{
						if (cheat_xtra) msg_format(_("%sの店主をシャッフルします。", "Shuffle a Shopkeeper of %s."), f_name + f_ptr->name);

						/* Shuffle it */
						store_shuffle(n);

						break;
					}
				}
			}
		}
	}


	/*** Process the monsters ***/

	/* Check for creature generation. */
	if (one_in_(d_info[player_ptr->dungeon_idx].max_m_alloc_chance) &&
	    !player_ptr->current_floor_ptr->inside_arena && !player_ptr->current_floor_ptr->inside_quest && !player_ptr->phase_out)
	{
		/* Make a new monster */
		(void)alloc_monster(MAX_SIGHT + 5, 0);
	}

	/* Hack -- Check for creature regeneration */
	if (!(current_world_ptr->game_turn % (TURNS_PER_TICK * 10)) && !player_ptr->phase_out) regen_monsters();
	if (!(current_world_ptr->game_turn % (TURNS_PER_TICK * 3))) regen_captured_monsters();

	if (!player_ptr->leaving)
	{
		int i;

		/* Hack -- Process the counters of monsters if needed */
		for (i = 0; i < MAX_MTIMED; i++)
		{
			if (player_ptr->current_floor_ptr->mproc_max[i] > 0) process_monsters_mtimed(i);
		}
	}


	/* Date changes */
	if (!hour && !min)
	{
		if (min != prev_min)
		{
			exe_write_diary(player_ptr, NIKKI_HIGAWARI, 0, NULL);
			determine_today_mon(FALSE);
		}
	}

	/*
	 * Nightmare mode activates the TY_CURSE at midnight
	 * Require exact minute -- Don't activate multiple times in a minute
	 */

	if (ironman_nightmare && (min != prev_min))
	{

		/* Every 15 minutes after 11:00 pm */
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

		/* TY_CURSE activates at midnight! */
		if (!hour && !min)
		{

			disturb(player_ptr, TRUE, TRUE);
			msg_print(_("遠くで鐘が何回も鳴り、死んだような静けさの中へ消えていった。", "A distant bell tolls many times, fading into an deathly silence."));

			if (player_ptr->wild_mode)
			{
				/* Go into large wilderness view */
				player_ptr->oldpy = randint1(MAX_HGT - 2);
				player_ptr->oldpx = randint1(MAX_WID - 2);
				change_wild_mode(player_ptr, TRUE);

				/* Give first move to monsters */
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
 * @return 実際にウィザードモードへ移行したらTRUEを返す。
 */
static bool enter_wizard_mode(void)
{
	/* Ask first time */
	if (!current_world_ptr->noscore)
	{
		/* Wizard mode is not permitted */
		if (!allow_debug_opts || arg_wizard)
		{
			msg_print(_("ウィザードモードは許可されていません。 ", "Wizard mode is not permitted."));
			return FALSE;
		}

		/* Mention effects */
		msg_print(_("ウィザードモードはデバッグと実験のためのモードです。 ", "Wizard mode is for debugging and experimenting."));
		msg_print(_("一度ウィザードモードに入るとスコアは記録されません。", "The game will not be scored if you enter wizard mode."));
		msg_print(NULL);

		/* Verify request */
		if (!get_check(_("本当にウィザードモードに入りたいのですか? ", "Are you sure you want to enter wizard mode? ")))
		{
			return (FALSE);
		}

		exe_write_diary(p_ptr, NIKKI_BUNSHOU, 0, _("ウィザードモードに突入してスコアを残せなくなった。", "give up recording score to enter wizard mode."));
		/* Mark savefile */
		current_world_ptr->noscore |= 0x0002;
	}

	/* Success */
	return (TRUE);
}


#ifdef ALLOW_WIZARD

/*!
 * @brief デバッグコマンドへの導入処理
 * / Verify use of "debug" commands
 * @return 実際にデバッグコマンドへ移行したらTRUEを返す。
 */
static bool enter_debug_mode(void)
{
	/* Ask first time */
	if (!current_world_ptr->noscore)
	{
		/* Debug mode is not permitted */
		if (!allow_debug_opts)
		{
			msg_print(_("デバッグコマンドは許可されていません。 ", "Use of debug command is not permitted."));
			return FALSE;
		}

		/* Mention effects */
		msg_print(_("デバッグ・コマンドはデバッグと実験のためのコマンドです。 ", "The debug commands are for debugging and experimenting."));
		msg_print(_("デバッグ・コマンドを使うとスコアは記録されません。", "The game will not be scored if you use debug commands."));

		msg_print(NULL);

		/* Verify request */
		if (!get_check(_("本当にデバッグ・コマンドを使いますか? ", "Are you sure you want to use debug commands? ")))
		{
			return (FALSE);
		}

		exe_write_diary(p_ptr, NIKKI_BUNSHOU, 0, _("デバッグモードに突入してスコアを残せなくなった。", "give up sending score to use debug commands."));
		/* Mark savefile */
		current_world_ptr->noscore |= 0x0008;
	}

	/* Success */
	return (TRUE);
}

/*
 * Hack -- Declare the Debug Routines
 */
extern void do_cmd_debug(player_type *creature_ptr);

#endif /* ALLOW_WIZARD */


#ifdef ALLOW_BORG

/*!
 * @brief ボーグコマンドへの導入処理
 * / Verify use of "borg" commands
 * @return 実際にボーグコマンドへ移行したらTRUEを返す。
 */
static bool enter_borg_mode(void)
{
	/* Ask first time */
	if (!(current_world_ptr->noscore & 0x0010))
	{
		/* Mention effects */
		msg_print(_("ボーグ・コマンドはデバッグと実験のためのコマンドです。 ", "The borg commands are for debugging and experimenting."));
		msg_print(_("ボーグ・コマンドを使うとスコアは記録されません。", "The game will not be scored if you use borg commands."));

		msg_print(NULL);

		/* Verify request */
		if (!get_check(_("本当にボーグ・コマンドを使いますか? ", "Are you sure you want to use borg commands? ")))
		{
			return (FALSE);
		}

		exe_write_diary(p_ptr, NIKKI_BUNSHOU, 0, _("ボーグ・コマンドを使用してスコアを残せなくなった。", "give up recording score to use borg commands."));
		/* Mark savefile */
		current_world_ptr->noscore |= 0x0010;
	}

	/* Success */
	return (TRUE);
}

/*
 * Hack -- Declare the Ben Borg
 */
extern void do_cmd_borg(void);

#endif /* ALLOW_BORG */


/*!
 * @brief プレイヤーから受けた入力コマンドの分岐処理。
 * / Parse and execute the current command Give "Warning" on illegal commands.
 * @todo Make some "blocks"
 * @return なし
 */
static void process_command(player_type *creature_ptr)
{
	COMMAND_CODE old_now_message = now_message;

	/* Handle repeating the last command */
	repeat_check();

	now_message = 0;

	/* Sniper */
	if ((creature_ptr->pclass == CLASS_SNIPER) && (creature_ptr->concent))
		creature_ptr->reset_concent = TRUE;

	/* Parse the command */
	switch (command_cmd)
	{
		/* Ignore */
		case ESCAPE:
		case ' ':
		{
			break;
		}

		/* Ignore return */
		case '\r':
		case '\n':
		{
			break;
		}

		/*** Wizard Commands ***/
		case KTRL('W'):
		{
			if (current_world_ptr->wizard)
			{
				current_world_ptr->wizard = FALSE;
				msg_print(_("ウィザードモード解除。", "Wizard mode off."));
			}
			else if (enter_wizard_mode())
			{
				current_world_ptr->wizard = TRUE;
				msg_print(_("ウィザードモード突入。", "Wizard mode on."));
			}
			creature_ptr->update |= (PU_MONSTERS);
			creature_ptr->redraw |= (PR_TITLE);

			break;
		}


#ifdef ALLOW_WIZARD

		/* Special "debug" commands */
		case KTRL('A'):
		{
			if (enter_debug_mode())
			{
				do_cmd_debug(creature_ptr);
			}
			break;
		}

#endif /* ALLOW_WIZARD */


#ifdef ALLOW_BORG

		/* Special "borg" commands */
		case KTRL('Z'):
		{
			if (enter_borg_mode())
			{
				if (!creature_ptr->wild_mode) do_cmd_borg();
			}
			break;
		}

#endif /* ALLOW_BORG */



		/*** Inventory Commands ***/

		/* Wear/wield equipment */
		case 'w':
		{
			if (!creature_ptr->wild_mode) do_cmd_wield(creature_ptr);
			break;
		}

		/* Take off equipment */
		case 't':
		{
			if (!creature_ptr->wild_mode) do_cmd_takeoff(creature_ptr);
			break;
		}

		/* Drop an item */
		case 'd':
		{
			if (!creature_ptr->wild_mode) do_cmd_drop(creature_ptr);
			break;
		}

		/* Destroy an item */
		case 'k':
		{
			do_cmd_destroy(creature_ptr);
			break;
		}

		/* Equipment list */
		case 'e':
		{
			do_cmd_equip(creature_ptr);
			break;
		}

		/* Inventory list */
		case 'i':
		{
			do_cmd_inven(creature_ptr);
			break;
		}


		/*** Various commands ***/

		/* Identify an object */
		case 'I':
		{
			do_cmd_observe(creature_ptr);
			break;
		}

		case KTRL('I'):
		{
			toggle_inven_equip(creature_ptr);
			break;
		}


		/*** Standard "Movement" Commands ***/

		/* Alter a grid */
		case '+':
		{
			if (!creature_ptr->wild_mode) do_cmd_alter(creature_ptr);
			break;
		}

		/* Dig a tunnel */
		case 'T':
		{
			if (!creature_ptr->wild_mode) do_cmd_tunnel(creature_ptr);
			break;
		}

		/* Move (usually pick up things) */
		case ';':
		{
			do_cmd_walk(creature_ptr, FALSE);
			break;
		}

		/* Move (usually do not pick up) */
		case '-':
		{
			do_cmd_walk(creature_ptr, TRUE);
			break;
		}


		/*** Running, Resting, Searching, Staying */

		/* Begin Running -- Arg is Max Distance */
		case '.':
		{
			if (!creature_ptr->wild_mode) do_cmd_run(creature_ptr);
			break;
		}

		/* Stay still (usually pick things up) */
		case ',':
		{
			do_cmd_stay(creature_ptr, always_pickup);
			break;
		}

		/* Stay still (usually do not pick up) */
		case 'g':
		{
			do_cmd_stay(creature_ptr, !always_pickup);
			break;
		}

		/* Rest -- Arg is time */
		case 'R':
		{
			do_cmd_rest(creature_ptr);
			break;
		}

		/* Search for traps/doors */
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


		/*** Stairs and Doors and Chests and Traps ***/

		/* Enter store */
		case SPECIAL_KEY_STORE:
		{
			do_cmd_store();
			break;
		}

		/* Enter building -KMW- */
		case SPECIAL_KEY_BUILDING:
		{
			do_cmd_bldg(creature_ptr);
			break;
		}

		/* Enter quest level -KMW- */
		case SPECIAL_KEY_QUEST:
		{
			do_cmd_quest();
			break;
		}

		/* Go up staircase */
		case '<':
		{
			if (!creature_ptr->wild_mode && !p_ptr->current_floor_ptr->dun_level && !creature_ptr->current_floor_ptr->inside_arena && !creature_ptr->current_floor_ptr->inside_quest)
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

		/* Go down staircase */
		case '>':
		{
			if (creature_ptr->wild_mode)
				change_wild_mode(creature_ptr, FALSE);
			else
				do_cmd_go_down(creature_ptr);
			break;
		}

		/* Open a door or chest */
		case 'o':
		{
			do_cmd_open(creature_ptr);
			break;
		}

		/* Close a door */
		case 'c':
		{
			do_cmd_close(creature_ptr);
			break;
		}

		/* Jam a door with spikes */
		case 'j':
		{
			do_cmd_spike(creature_ptr);
			break;
		}

		/* Bash a door */
		case 'B':
		{
			do_cmd_bash(creature_ptr);
			break;
		}

		/* Disarm a trap or chest */
		case 'D':
		{
			do_cmd_disarm(creature_ptr);
			break;
		}


		/*** Magic and Prayers ***/

		/* Gain new spells/prayers */
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

		/* Browse a book */
		case 'b':
		{
			if ( (creature_ptr->pclass == CLASS_MINDCRAFTER) ||
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

		/* Cast a spell */
		case 'm':
		{
			/* -KMW- */
			if (!creature_ptr->wild_mode)
			{
				if ((creature_ptr->pclass == CLASS_WARRIOR) || (creature_ptr->pclass == CLASS_ARCHER) || (creature_ptr->pclass == CLASS_CAVALRY))
				{
					msg_print(_("呪文を唱えられない！", "You cannot cast spells!"));
				}
				else if (p_ptr->current_floor_ptr->dun_level && (d_info[creature_ptr->dungeon_idx].flags1 & DF1_NO_MAGIC) && (creature_ptr->pclass != CLASS_BERSERKER) && (creature_ptr->pclass != CLASS_SMITH))
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
						do_cmd_cast_learned();
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

		/* Issue a pet command */
		case 'p':
		{
			do_cmd_pet(creature_ptr);
			break;
		}

		/*** Use various objects ***/

		/* Inscribe an object */
		case '{':
		{
			do_cmd_inscribe(creature_ptr);
			break;
		}

		/* Uninscribe an object */
		case '}':
		{
			do_cmd_uninscribe(creature_ptr);
			break;
		}

		/* Activate an artifact */
		case 'A':
		{
			do_cmd_activate(creature_ptr);
			break;
		}

		/* Eat some food */
		case 'E':
		{
			do_cmd_eat_food(creature_ptr);
			break;
		}

		/* Fuel your lantern/torch */
		case 'F':
		{
			do_cmd_refill(creature_ptr);
			break;
		}

		/* Fire an item */
		case 'f':
		{
			do_cmd_fire(creature_ptr, SP_NONE);
			break;
		}

		/* Throw an item */
		case 'v':
		{
			do_cmd_throw(creature_ptr, 1, FALSE, -1);
			break;
		}

		/* Aim a wand */
		case 'a':
		{
			do_cmd_aim_wand(creature_ptr);
			break;
		}

		/* Zap a rod */
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

		/* Quaff a potion */
		case 'q':
		{
			do_cmd_quaff_potion(creature_ptr);
			break;
		}

		/* Read a scroll */
		case 'r':
		{
			do_cmd_read_scroll(creature_ptr);
			break;
		}

		/* Use a staff */
		case 'u':
		{
			if (use_command && !rogue_like_commands)
				do_cmd_use(creature_ptr);
			else
				do_cmd_use_staff(creature_ptr);
			break;
		}

		/* Use racial power */
		case 'U':
		{
			do_cmd_racial_power(creature_ptr);
			break;
		}


		/*** Looking at Things (nearby or on map) ***/

		/* Full dungeon map */
		case 'M':
		{
			do_cmd_view_map();
			break;
		}

		/* Locate player on map */
		case 'L':
		{
			do_cmd_locate(creature_ptr);
			break;
		}

		/* Look around */
		case 'l':
		{
			do_cmd_look(creature_ptr);
			break;
		}

		/* Target monster or location */
		case '*':
		{
			do_cmd_target(creature_ptr);
			break;
		}



		/*** Help and Such ***/

		/* Help */
		case '?':
		{
			do_cmd_help();
			break;
		}

		/* Identify symbol */
		case '/':
		{
			do_cmd_query_symbol();
			break;
		}

		/* Character description */
		case 'C':
		{
			do_cmd_player_status(creature_ptr);
			break;
		}


		/*** System Commands ***/

		/* Hack -- User interface */
		case '!':
		{
			(void)Term_user(0);
			break;
		}

		/* Single line from a pref file */
		case '"':
		{
			do_cmd_pref();
			break;
		}

		case '$':
		{
			do_cmd_reload_autopick();
			break;
		}

		case '_':
		{
			do_cmd_edit_autopick();
			break;
		}

		/* Interact with macros */
		case '@':
		{
			do_cmd_macros(creature_ptr);
			break;
		}

		/* Interact with visuals */
		case '%':
		{
			do_cmd_visuals(creature_ptr);
			do_cmd_redraw(creature_ptr);
			break;
		}

		/* Interact with colors */
		case '&':
		{
			do_cmd_colors(creature_ptr);
			do_cmd_redraw(creature_ptr);
			break;
		}

		/* Interact with options */
		case '=':
		{
			do_cmd_options();
			(void)combine_and_reorder_home(STORE_HOME);
			do_cmd_redraw(creature_ptr);
			break;
		}

		/*** Misc Commands ***/

		/* Take notes */
		case ':':
		{
			do_cmd_note();
			break;
		}

		/* Version info */
		case 'V':
		{
			do_cmd_version();
			break;
		}

		/* Repeat level feeling */
		case KTRL('F'):
		{
			do_cmd_feeling(creature_ptr);
			break;
		}

		/* Show previous message */
		case KTRL('O'):
		{
			do_cmd_message_one();
			break;
		}

		/* Show previous messages */
		case KTRL('P'):
		{
			do_cmd_messages(old_now_message);
			break;
		}

		/* Show quest status -KMW- */
		case KTRL('Q'):
		{
			do_cmd_checkquest();
			break;
		}

		/* Redraw the screen */
		case KTRL('R'):
		{
			now_message = old_now_message;
			do_cmd_redraw(creature_ptr);
			break;
		}

#ifndef VERIFY_SAVEFILE

		/* Hack -- Save and don't quit */
		case KTRL('S'):
		{
			do_cmd_save_game(FALSE);
			break;
		}

#endif /* VERIFY_SAVEFILE */

		case KTRL('T'):
		{
			do_cmd_time();
			break;
		}

		/* Save and quit */
		case KTRL('X'):
		case SPECIAL_KEY_QUIT:
		{
			do_cmd_save_and_exit();
			break;
		}

		/* Quit (commit suicide) */
		case 'Q':
		{
			do_cmd_suicide(creature_ptr);
			break;
		}

		case '|':
		{
			do_cmd_nikki(creature_ptr);
			break;
		}

		/* Check artifacts, uniques, objects */
		case '~':
		{
			do_cmd_knowledge(creature_ptr);
			break;
		}

		/* Load "screen dump" */
		case '(':
		{
			do_cmd_load_screen();
			break;
		}

		/* Save "screen dump" */
		case ')':
		{
			do_cmd_save_screen();
			break;
		}

		/* Record/stop "Movie" */
		case ']':
		{
			prepare_movie_hooks();
			break;
		}

		/* Make random artifact list */
		case KTRL('V'):
		{
			spoil_random_artifact(creature_ptr, "randifact.txt");
			break;
		}

#ifdef TRAVEL
		case '`':
		{
			if (!creature_ptr->wild_mode) do_cmd_travel(creature_ptr);
			if (creature_ptr->special_defense & KATA_MUSOU)
			{
				set_action(creature_ptr, ACTION_NONE);
			}
			break;
		}
#endif

		/* Hack -- Unknown command */
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
	if (owner_ptr->inventory_list[INVEN_PACK].k_idx)
	{
		GAME_TEXT o_name[MAX_NLEN];
		object_type *o_ptr;

		/* Is auto-destroy done? */
		update_creature(owner_ptr);
		if (!owner_ptr->inventory_list[INVEN_PACK].k_idx) return;

		/* Access the slot to be dropped */
		o_ptr = &owner_ptr->inventory_list[INVEN_PACK];

		disturb(owner_ptr, FALSE, TRUE);

		/* Warning */
		msg_print(_("ザックからアイテムがあふれた！", "Your pack overflows!"));
		object_desc(o_name, o_ptr, 0);

		msg_format(_("%s(%c)を落とした。", "You drop %s (%c)."), o_name, index_to_label(INVEN_PACK));

		/* Drop it (carefully) near the player */
		(void)drop_near(o_ptr, 0, owner_ptr->y, owner_ptr->x);

		vary_item(INVEN_PACK, -255);
		handle_stuff();
	}
}

/*!
 * @brief プレイヤーの行動エネルギーが充填される（＝プレイヤーのターンが回る）毎に行われる処理  / process the effects per 100 energy at player speed.
 * @return なし
 */
static void process_upkeep_with_speed(player_type *creature_ptr)
{
	/* Give the player some energy */
	if (!load && creature_ptr->enchant_energy_need > 0 && !creature_ptr->leaving)
	{
		creature_ptr->enchant_energy_need -= SPEED_TO_ENERGY(creature_ptr->pspeed);
	}
	
	/* No turn yet */
	if (creature_ptr->enchant_energy_need > 0) return;
	
	while (creature_ptr->enchant_energy_need <= 0)
	{
		/* Handle the player song */
		if (!load) check_music(creature_ptr);

		/* Hex - Handle the hex spells */
		if (!load) check_hex(creature_ptr);
		if (!load) revenge_spell(creature_ptr);
		
		/* There is some randomness of needed energy */
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
		get_mon_num_prep(monster_is_fishing_target, NULL);
		r_idx = get_mon_num(creature_ptr->current_floor_ptr->dun_level ? creature_ptr->current_floor_ptr->dun_level : wilderness[creature_ptr->wilderness_y][creature_ptr->wilderness_x].level);
		msg_print(NULL);
		if (r_idx && one_in_(2))
		{
			POSITION y, x;
			y = creature_ptr->y + ddy[creature_ptr->fishing_dir];
			x = creature_ptr->x + ddx[creature_ptr->fishing_dir];
			if (place_monster_aux(0, y, x, r_idx, PM_NO_KAGE))
			{
				GAME_TEXT m_name[MAX_NLEN];
				monster_desc(m_name, &creature_ptr->current_floor_ptr->m_list[creature_ptr->current_floor_ptr->grid_array[y][x].m_idx], 0);
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
	MONSTER_IDX m_idx;

	/*** Apply energy ***/

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
		for(m_idx = 1; m_idx < creature_ptr->current_floor_ptr->m_max; m_idx++)
		{
			monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[m_idx];

			if (!monster_is_valid(m_ptr)) continue;

			m_ptr->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
			update_monster(creature_ptr, m_idx, FALSE);
		}
		prt_time();
	}

	/* Give the player some energy */
	else if (!(load && creature_ptr->energy_need <= 0))
	{
		creature_ptr->energy_need -= SPEED_TO_ENERGY(creature_ptr->pspeed);
	}

	/* No turn yet */
	if (creature_ptr->energy_need > 0) return;
	if (!command_rep) prt_time();

	/*** Check for interupts ***/

	/* Complete resting */
	if (creature_ptr->resting < 0)
	{
		/* Basic resting */
		if (creature_ptr->resting == COMMAND_ARG_REST_FULL_HEALING)
		{
			/* Stop resting */
			if ((creature_ptr->chp == creature_ptr->mhp) &&
			    (creature_ptr->csp >= creature_ptr->msp))
			{
				set_action(creature_ptr, ACTION_NONE);
			}
		}

		/* Complete resting */
		else if (creature_ptr->resting == COMMAND_ARG_REST_UNTIL_DONE)
		{
			/* Stop resting */
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

	/* Handle "abort" */
	if (check_abort)
	{
		/* Check for "player abort" (semi-efficiently for resting) */
		if (creature_ptr->running || travel.run || command_rep || (creature_ptr->action == ACTION_REST) || (creature_ptr->action == ACTION_FISH))
		{
			/* Do not wait */
			inkey_scan = TRUE;

			/* Check for a key */
			if (inkey())
			{
				flush(); /* Flush input */

				disturb(creature_ptr, FALSE, TRUE);

				/* Hack -- Show a Message */
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

			/* Recover fully */
			(void)set_monster_csleep(creature_ptr->riding, 0);
			monster_desc(m_name, m_ptr, 0);
			msg_format(_("%^sを起こした。", "You have waked %s up."), m_name);
		}

		if (MON_STUNNED(m_ptr))
		{
			/* Hack -- Recover from stun */
			if (set_monster_stunned(creature_ptr->riding,
				(randint0(r_ptr->level) < creature_ptr->skill_exp[GINOU_RIDING]) ? 0 : (MON_STUNNED(m_ptr) - 1)))
			{
				GAME_TEXT m_name[MAX_NLEN];
				monster_desc(m_name, m_ptr, 0);
				msg_format(_("%^sを朦朧状態から立ち直らせた。", "%^s is no longer stunned."), m_name);
			}
		}

		if (MON_CONFUSED(m_ptr))
		{
			/* Hack -- Recover from confusion */
			if (set_monster_confused(creature_ptr->riding,
				(randint0(r_ptr->level) < creature_ptr->skill_exp[GINOU_RIDING]) ? 0 : (MON_CONFUSED(m_ptr) - 1)))
			{
				GAME_TEXT m_name[MAX_NLEN];
				monster_desc(m_name, m_ptr, 0);
				msg_format(_("%^sを混乱状態から立ち直らせた。", "%^s is no longer confused."), m_name);
			}
		}

		if (MON_MONFEAR(m_ptr))
		{
			/* Hack -- Recover from fear */
			if(set_monster_monfear(creature_ptr->riding,
				(randint0(r_ptr->level) < creature_ptr->skill_exp[GINOU_RIDING]) ? 0 : (MON_MONFEAR(m_ptr) - 1)))
			{
				GAME_TEXT m_name[MAX_NLEN];
				monster_desc(m_name, m_ptr, 0);
				msg_format(_("%^sを恐怖から立ち直らせた。", "%^s is no longer fear."), m_name);
			}
		}

		handle_stuff();
	}
	
	load = FALSE;

	/* Fast */
	if (creature_ptr->lightspeed)
	{
		(void)set_lightspeed(creature_ptr, creature_ptr->lightspeed - 1, TRUE);
	}
	if ((creature_ptr->pclass == CLASS_FORCETRAINER) && P_PTR_KI)
	{
		if(P_PTR_KI < 40) P_PTR_KI = 0;
		else P_PTR_KI -= 40;
		creature_ptr->update |= (PU_BONUS);
	}
	if (creature_ptr->action == ACTION_LEARN)
	{
		s32b cost = 0L;
		u32b cost_frac = (creature_ptr->msp + 30L) * 256L;

		/* Convert the unit (1/2^16) to (1/2^32) */
		s64b_LSHIFT(cost, cost_frac, 16);
 
		if (s64b_cmp(creature_ptr->csp, creature_ptr->csp_frac, cost, cost_frac) < 0)
		{
			/* Mana run out */
			creature_ptr->csp = 0;
			creature_ptr->csp_frac = 0;
			set_action(creature_ptr, ACTION_NONE);
		}
		else
		{
			/* Reduce mana */
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

	/* Repeat until out of energy */
	while (creature_ptr->energy_need <= 0)
	{
		creature_ptr->window |= PW_PLAYER;
		creature_ptr->sutemi = FALSE;
		creature_ptr->counter = FALSE;
		creature_ptr->now_damaged = FALSE;

		handle_stuff();

		/* Place the cursor on the player */
		move_cursor_relative(creature_ptr->y, creature_ptr->x);

		/* Refresh (optional) */
		if (fresh_before) Term_fresh();

		/* Hack -- Pack Overflow */
		pack_overflow(creature_ptr);

		/* Hack -- cancel "lurking browse mode" */
		if (!command_new) command_see = FALSE;

		/* Assume free turn */
		free_turn(creature_ptr);

		if (creature_ptr->phase_out)
		{
			/* Place the cursor on the player */
			move_cursor_relative(creature_ptr->y, creature_ptr->x);

			command_cmd = SPECIAL_KEY_BUILDING;

			/* Process the command */
			process_command(creature_ptr);
		}

		/* Paralyzed or Knocked Out */
		else if (creature_ptr->paralyzed || (creature_ptr->stun >= 100))
		{
			take_turn(creature_ptr, 100);
		}

		/* Resting */
		else if (creature_ptr->action == ACTION_REST)
		{
			/* Timed rest */
			if (creature_ptr->resting > 0)
			{
				/* Reduce rest count */
				creature_ptr->resting--;

				if (!creature_ptr->resting) set_action(creature_ptr, ACTION_NONE);
				creature_ptr->redraw |= (PR_STATE);
			}

			take_turn(creature_ptr, 100);
		}

		/* Fishing */
		else if (creature_ptr->action == ACTION_FISH)
		{
			take_turn(creature_ptr, 100);
		}

		/* Running */
		else if (creature_ptr->running)
		{
			/* Take a step */
			run_step(0);
		}

#ifdef TRAVEL
		/* Traveling */
		else if (travel.run)
		{
			/* Take a step */
			travel_step(creature_ptr);
		}
#endif

		/* Repeated command */
		else if (command_rep)
		{
			/* Count this execution */
			command_rep--;

			creature_ptr->redraw |= (PR_STATE);
			handle_stuff();

			/* Hack -- Assume messages were seen */
			msg_flag = FALSE;

			/* Clear the top line */
			prt("", 0, 0);

			/* Process the command */
			process_command(creature_ptr);
		}

		/* Normal command */
		else
		{
			/* Place the cursor on the player */
			move_cursor_relative(creature_ptr->y, creature_ptr->x);

			can_save = TRUE;
			/* Get a command (normal) */
			request_command(FALSE);
			can_save = FALSE;

			/* Process the command */
			process_command(creature_ptr);
		}

		/* Hack -- Pack Overflow */
		pack_overflow(creature_ptr);

		/*** Clean up ***/

		/* Significant */
		if (creature_ptr->energy_use)
		{
			/* Use some energy */
			if (creature_ptr->timewalk || creature_ptr->energy_use > 400)
			{
				/* The Randomness is irrelevant */
				creature_ptr->energy_need += creature_ptr->energy_use * TURNS_PER_TICK / 10;
			}
			else
			{
				/* There is some randomness of needed energy */
				creature_ptr->energy_need += (s16b)((s32b)creature_ptr->energy_use * ENERGY_NEED() / 100L);
			}

			/* Hack -- constant hallucination */
			if (creature_ptr->image) creature_ptr->redraw |= (PR_MAP);

			/* Shimmer multi-hued monsters */
			for (m_idx = 1; m_idx < creature_ptr->current_floor_ptr->m_max; m_idx++)
			{
				monster_type *m_ptr;
				monster_race *r_ptr;

				m_ptr = &creature_ptr->current_floor_ptr->m_list[m_idx];
				if (!monster_is_valid(m_ptr)) continue;

				/* Skip unseen monsters */
				if (!m_ptr->ml) continue;

				/* Access the monster race */
				r_ptr = &r_info[m_ptr->ap_r_idx];

				/* Skip non-multi-hued monsters */
				if (!(r_ptr->flags1 & (RF1_ATTR_MULTI | RF1_SHAPECHANGER)))
					continue;

				/* Redraw regardless */
				lite_spot(m_ptr->fy, m_ptr->fx);
			}


			/* Handle monster detection */
			if (repair_monsters)
			{
				/* Reset the flag */
				repair_monsters = FALSE;

				/* Rotate detection flags */
				for (m_idx = 1; m_idx < creature_ptr->current_floor_ptr->m_max; m_idx++)
				{
					monster_type *m_ptr;
					m_ptr = &creature_ptr->current_floor_ptr->m_list[m_idx];
					if (!monster_is_valid(m_ptr)) continue;

					/* Nice monsters get mean */
					if (m_ptr->mflag & MFLAG_NICE)
					{
						/* Nice monsters get mean */
						m_ptr->mflag &= ~(MFLAG_NICE);
					}

					/* Handle memorized monsters */
					if (m_ptr->mflag2 & MFLAG2_MARK)
					{
						/* Maintain detection */
						if (m_ptr->mflag2 & MFLAG2_SHOW)
						{
							/* Forget flag */
							m_ptr->mflag2 &= ~(MFLAG2_SHOW);

							/* Still need repairs */
							repair_monsters = TRUE;
						}

						/* Remove detection */
						else
						{
							/* Forget flag */
							m_ptr->mflag2 &= ~(MFLAG2_MARK);

							/* Assume invisible */
							m_ptr->ml = FALSE;
							update_monster(creature_ptr, m_idx, FALSE);

							if (creature_ptr->health_who == m_idx) creature_ptr->redraw |= (PR_HEALTH);
							if (creature_ptr->riding == m_idx) creature_ptr->redraw |= (PR_UHEALTH);

							/* Redraw regardless */
							lite_spot(m_ptr->fy, m_ptr->fx);
						}
					}
				}
			}
			if (creature_ptr->pclass == CLASS_IMITATOR)
			{
				int j;
				if (creature_ptr->mane_num > (creature_ptr->lev > 44 ? 3 : creature_ptr->lev > 29 ? 2 : 1))
				{
					creature_ptr->mane_num--;
					for (j = 0; j < creature_ptr->mane_num; j++)
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

			if (creature_ptr->timewalk && (creature_ptr->energy_need > - 1000))
			{

				creature_ptr->redraw |= (PR_MAP);
				creature_ptr->update |= (PU_MONSTERS);
				creature_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

				msg_print(_("「時は動きだす…」", "You feel time flowing around you once more."));
				msg_print(NULL);
				creature_ptr->timewalk = FALSE;
				creature_ptr->energy_need = ENERGY_NEED();

				handle_stuff();
			}
		}

		/* Hack -- notice death */
		if (!creature_ptr->playing || creature_ptr->is_dead)
		{
			creature_ptr->timewalk = FALSE;
			break;
		}

		/* Sniper */
		if (creature_ptr->energy_use && creature_ptr->reset_concent) reset_concentration(creature_ptr, TRUE);

		/* Handle "leaving" */
		if (creature_ptr->leaving) break;
	}

	/* Update scent trail */
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
	int quest_num = 0;

	/* Set the base level */
	player_ptr->current_floor_ptr->base_level = player_ptr->current_floor_ptr->dun_level;

	/* Reset various flags */
	current_world_ptr->is_loading_now = FALSE;

	/* Not leaving */
	player_ptr->leaving = FALSE;

	/* Reset the "command" vars */
	command_cmd = 0;

#if 0 /* Don't reset here --- It's used for Arena */
	command_new = 0;
#endif

	command_rep = 0;
	command_arg = 0;
	command_dir = 0;


	/* Cancel the target */
	target_who = 0;
	player_ptr->pet_t_m_idx = 0;
	player_ptr->riding_t_m_idx = 0;
	player_ptr->ambush_flag = FALSE;

	/* Cancel the health bar */
	health_track(0);

	/* Check visual effects */
	repair_monsters = TRUE;
	repair_objects = TRUE;


	disturb(player_ptr, TRUE, TRUE);

	/* Get index of current quest (if any) */
	quest_num = quest_number(player_ptr->current_floor_ptr->dun_level);

	/* Inside a quest? */
	if (quest_num)
	{
		/* Mark the quest monster */
		r_info[quest[quest_num].r_idx].flags1 |= RF1_QUESTOR;
	}

	/* Track maximum player level */
	if (player_ptr->max_plv < player_ptr->lev)
	{
		player_ptr->max_plv = player_ptr->lev;
	}


	/* Track maximum dungeon level (if not in quest -KMW-) */
	if ((max_dlv[player_ptr->dungeon_idx] < player_ptr->current_floor_ptr->dun_level) && !player_ptr->current_floor_ptr->inside_quest)
	{
		max_dlv[player_ptr->dungeon_idx] = player_ptr->current_floor_ptr->dun_level;
		if (record_maxdepth) exe_write_diary(player_ptr, NIKKI_MAXDEAPTH, player_ptr->current_floor_ptr->dun_level, NULL);
	}

	(void)calculate_upkeep(player_ptr);

	/* Validate the panel */
	panel_bounds_center();

	/* Verify the panel */
	verify_panel();

	msg_erase();


	/* Enter "xtra" mode */
	current_world_ptr->character_xtra = TRUE;

	player_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER | PW_MONSTER | PW_OVERHEAD | PW_DUNGEON);
	player_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_EQUIPPY | PR_MAP);
	player_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS | PU_VIEW | PU_LITE | PU_MON_LITE | PU_TORCH | PU_MONSTERS | PU_DISTANCE | PU_FLOW);

	handle_stuff();

	/* Leave "xtra" mode */
	current_world_ptr->character_xtra = FALSE;

	player_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
	player_ptr->update |= (PU_COMBINE | PU_REORDER);
	handle_stuff();
	Term_fresh();

	if (quest_num && (is_fixed_quest_idx(quest_num) &&
	    !((quest_num == QUEST_OBERON) || (quest_num == QUEST_SERPENT) ||
	    !(quest[quest_num].flags & QUEST_FLAG_PRESET)))) do_cmd_feeling(player_ptr);

	if (player_ptr->phase_out)
	{
		if (load_game)
		{
			player_ptr->energy_need = 0;
			update_gambling_monsters();
		}
		else
		{
			msg_print(_("試合開始！", "Ready..Fight!"));
			msg_print(NULL);
		}
	}

	if ((player_ptr->pclass == CLASS_BARD) && (SINGING_SONG_EFFECT(player_ptr) > MUSIC_DETECT))
		SINGING_SONG_EFFECT(player_ptr) = MUSIC_DETECT;

	/* Hack -- notice death or departure */
	if (!player_ptr->playing || player_ptr->is_dead) return;

	/* Print quest message if appropriate */
	if (!player_ptr->current_floor_ptr->inside_quest && (player_ptr->dungeon_idx == DUNGEON_ANGBAND))
	{
		quest_discovery(random_quest_number(player_ptr->current_floor_ptr->dun_level));
		player_ptr->current_floor_ptr->inside_quest = random_quest_number(player_ptr->current_floor_ptr->dun_level);
	}
	if ((player_ptr->current_floor_ptr->dun_level == d_info[player_ptr->dungeon_idx].maxdepth) && d_info[player_ptr->dungeon_idx].final_guardian)
	{
		if (r_info[d_info[player_ptr->dungeon_idx].final_guardian].max_num)
#ifdef JP
			msg_format("この階には%sの主である%sが棲んでいる。",
				   d_name+d_info[player_ptr->dungeon_idx].name, 
				   r_name+r_info[d_info[player_ptr->dungeon_idx].final_guardian].name);
#else
			msg_format("%^s lives in this level as the keeper of %s.",
					   r_name+r_info[d_info[player_ptr->dungeon_idx].final_guardian].name, 
					   d_name+d_info[player_ptr->dungeon_idx].name);
#endif
	}

	if (!load_game && (player_ptr->special_defense & NINJA_S_STEALTH)) set_superstealth(player_ptr, FALSE);

	/*** Process this dungeon level ***/

	/* Reset the monster generation level */
	player_ptr->current_floor_ptr->monster_level = player_ptr->current_floor_ptr->base_level;

	/* Reset the object generation level */
	player_ptr->current_floor_ptr->object_level = player_ptr->current_floor_ptr->base_level;

	current_world_ptr->is_loading_now = TRUE;

	if (player_ptr->energy_need > 0 && !player_ptr->phase_out &&
	    (player_ptr->current_floor_ptr->dun_level || player_ptr->leaving_dungeon || player_ptr->current_floor_ptr->inside_arena))
		player_ptr->energy_need = 0;

	/* Not leaving dungeon */
	player_ptr->leaving_dungeon = FALSE;

	/* Initialize monster process */
	mproc_init();

	/* Main loop */
	while (TRUE)
	{
		/* Hack -- Compact the monster list occasionally */
		if ((player_ptr->current_floor_ptr->m_cnt + 32 > current_world_ptr->max_m_idx) && !player_ptr->phase_out) compact_monsters(64);

		/* Hack -- Compress the monster list occasionally */
		if ((player_ptr->current_floor_ptr->m_cnt + 32 < player_ptr->current_floor_ptr->m_max) && !player_ptr->phase_out) compact_monsters(0);


		/* Hack -- Compact the object list occasionally */
		if (player_ptr->current_floor_ptr->o_cnt + 32 > current_world_ptr->max_o_idx) compact_objects(player_ptr->current_floor_ptr, 64);

		/* Hack -- Compress the object list occasionally */
		if (player_ptr->current_floor_ptr->o_cnt + 32 < player_ptr->current_floor_ptr->o_max) compact_objects(player_ptr->current_floor_ptr, 0);

		/* Process the player */
		process_player(player_ptr);
		process_upkeep_with_speed(player_ptr);

		handle_stuff();

		/* Hack -- Hilite the player */
		move_cursor_relative(player_ptr->y, player_ptr->x);

		/* Optional fresh */
		if (fresh_after) Term_fresh();

		/* Hack -- Notice death or departure */
		if (!player_ptr->playing || player_ptr->is_dead) break;

		/* Process all of the monsters */
		process_monsters();

		handle_stuff();

		/* Hack -- Hilite the player */
		move_cursor_relative(player_ptr->y, player_ptr->x);

		/* Optional fresh */
		if (fresh_after) Term_fresh();

		/* Hack -- Notice death or departure */
		if (!player_ptr->playing || player_ptr->is_dead) break;

		/* Process the world */
		process_world(player_ptr);

		handle_stuff();

		/* Hack -- Hilite the player */
		move_cursor_relative(player_ptr->y, player_ptr->x);

		/* Optional fresh */
		if (fresh_after) Term_fresh();

		/* Hack -- Notice death or departure */
		if (!player_ptr->playing || player_ptr->is_dead) break;

		/* Count game turns */
		current_world_ptr->game_turn++;

		if (current_world_ptr->dungeon_turn < current_world_ptr->dungeon_turn_limit)
		{
			if (!player_ptr->wild_mode || wild_regen) current_world_ptr->dungeon_turn++;
			else if (player_ptr->wild_mode && !(current_world_ptr->game_turn % ((MAX_HGT + MAX_WID) / 2))) current_world_ptr->dungeon_turn++;
		}

		prevent_turn_overflow();

		/* Handle "leaving" */
		if (player_ptr->leaving) break;

		if (wild_regen) wild_regen--;
	}

	/* Inside a quest and non-unique questor? */
	if (quest_num && !(r_info[quest[quest_num].r_idx].flags1 & RF1_UNIQUE))
	{
		/* Un-mark the quest monster */
		r_info[quest[quest_num].r_idx].flags1 &= ~RF1_QUESTOR;
	}

	/* Not save-and-quit and not dead? */
	if (player_ptr->playing && !player_ptr->is_dead)
	{
		/*
		 * Maintain Unique monsters and artifact, save current
		 * floor, then prepare next floor
		 */
		leave_floor(player_ptr);

		/* Forget the flag */
		reinit_wilderness = FALSE;
	}

	/* Write about current level on the play record once per level */
	write_level = TRUE;
}


/*!
 * @brief 全ユーザプロファイルをロードする / Load some "user pref files"
 * @return なし
 * @note
 * Modified by Arcum Dagsson to support
 * separate macro files for different realms.
 */
static void load_all_pref_files(void)
{
	char buf[1024];

	/* Access the "user" pref file */
	sprintf(buf, "user.prf");

	/* Process that file */
	process_pref_file(buf);

	/* Access the "user" system pref file */
	sprintf(buf, "user-%s.prf", ANGBAND_SYS);

	/* Process that file */
	process_pref_file(buf);

	/* Access the "race" pref file */
	sprintf(buf, "%s.prf", rp_ptr->title);

	/* Process that file */
	process_pref_file(buf);

	/* Access the "class" pref file */
	sprintf(buf, "%s.prf", cp_ptr->title);

	/* Process that file */
	process_pref_file(buf);

	/* Access the "character" pref file */
	sprintf(buf, "%s.prf", p_ptr->base_name);

	/* Process that file */
	process_pref_file(buf);

	/* Access the "realm 1" pref file */
	if (p_ptr->realm1 != REALM_NONE)
	{
		sprintf(buf, "%s.prf", realm_names[p_ptr->realm1]);

		/* Process that file */
		process_pref_file(buf);
	}

	/* Access the "realm 2" pref file */
	if (p_ptr->realm2 != REALM_NONE)
	{
		sprintf(buf, "%s.prf", realm_names[p_ptr->realm2]);

		/* Process that file */
		process_pref_file(buf);
	}


	/* Load an autopick preference file */
	autopick_load_pref(FALSE);
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
	MONSTER_IDX i;
	bool load_game = TRUE;
	bool init_random_seed = FALSE;

#ifdef CHUUKEI
	if (chuukei_client)
	{
		reset_visuals();
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
		reset_visuals();
		browse_movie();
		return;
	}

	player_ptr->hack_mutation = FALSE;

	/* Hack -- Character is "icky" */
	current_world_ptr->character_icky = TRUE;

	/* Make sure main term is active */
	Term_activate(angband_term[0]);

	/* Initialise the resize hooks */
	angband_term[0]->resize_hook = resize_map;

	for (i = 1; i < 8; i++)
	{
		/* Does the term exist? */
		if (angband_term[i])
		{
			/* Add the redraw on resize hook */
			angband_term[i]->resize_hook = redraw_window;
		}
	}

	/* Hack -- turn off the cursor */
	(void)Term_set_cursor(0);


	/* Attempt to load */
	if (!load_player())
	{
		quit(_("セーブファイルが壊れています", "broken savefile"));
	}

	/* Extract the options */
	extract_option_vars();

	/* Report waited score */
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

		/* No suspending now */
		signals_ignore_tstp();
		
		/* Hack -- Character is now "icky" */
		current_world_ptr->character_icky = TRUE;
		path_build(buf, sizeof(buf), ANGBAND_DIR_APEX, "scores.raw");

		/* Open the high score file, for reading/writing */
		highscore_fd = fd_open(buf, O_RDWR);

		/* 町名消失バグ対策(#38205)のためここで世界マップ情報を読み出す */
		process_dungeon_file("w_info.txt", 0, 0, current_world_ptr->max_wild_y, current_world_ptr->max_wild_x);

		/* Handle score, show Top scores */
		success = send_world_score(TRUE);

		if (!success && !get_check_strict(_("スコア登録を諦めますか？", "Do you give up score registration? "), CHECK_NO_HISTORY))
		{
			prt(_("引き続き待機します。", "standing by for future registration..."), 0, 0);
			(void)inkey();
		}
		else
		{
			player_ptr->wait_report_score = FALSE;
			top_twenty(player_ptr);
			if (!save_player()) msg_print(_("セーブ失敗！", "death save failed!"));
		}
		/* Shut the high score file */
		(void)fd_close(highscore_fd);

		/* Forget the high score fd */
		highscore_fd = -1;
		
		/* Allow suspending now */
		signals_handle_tstp();

		quit(0);
	}

	current_world_ptr->creating_savefile = new_game;

	/* Nothing loaded */
	if (!current_world_ptr->character_loaded)
	{
		/* Make new player */
		new_game = TRUE;

		/* The dungeon is not ready */
		current_world_ptr->character_dungeon = FALSE;

		/* Prepare to init the RNG */
		init_random_seed = TRUE;

		/* Initialize the saved floors data */
		init_saved_floors(FALSE);
	}

	/* Old game is loaded.  But new game is requested. */
	else if (new_game)
	{
		/* Initialize the saved floors data */
		init_saved_floors(TRUE);
	}

	/* Process old character */
	if (!new_game)
	{
		/* Process the player name */
		process_player_name(FALSE);
	}

	/* Init the RNG */
	if (init_random_seed)
	{
		Rand_state_init();
	}

	/* Roll new character */
	if (new_game)
	{
		/* The dungeon is not ready */
		current_world_ptr->character_dungeon = FALSE;

		/* Start in town */
		player_ptr->current_floor_ptr->dun_level = 0;
		player_ptr->current_floor_ptr->inside_quest = 0;
		player_ptr->current_floor_ptr->inside_arena = FALSE;
		player_ptr->phase_out = FALSE;

		write_level = TRUE;

		/* Hack -- seed for flavors */
		current_world_ptr->seed_flavor = randint0(0x10000000);

		/* Hack -- seed for town layout */
		current_world_ptr->seed_town = randint0(0x10000000);

		/* Roll up a new character */
		player_birth(player_ptr);

		counts_write(2,0);
		player_ptr->count = 0;

		load = FALSE;

		determine_bounty_uniques();
		determine_today_mon(FALSE);

		/* Initialize object array */
		wipe_o_list(player_ptr->current_floor_ptr);
	}
	else
	{
		write_level = FALSE;

		exe_write_diary(player_ptr, NIKKI_GAMESTART, 1, 
					  _("                            ----ゲーム再開----",
						"                            ---- Restart Game ----"));

/*
 * 1.0.9 以前はセーブ前に player_ptr->riding = -1 としていたので、再設定が必要だった。
 * もう不要だが、以前のセーブファイルとの互換のために残しておく。
 */
		if (player_ptr->riding == -1)
		{
			player_ptr->riding = 0;
			for (i = player_ptr->current_floor_ptr->m_max; i > 0; i--)
			{
				if (player_bold(player_ptr, player_ptr->current_floor_ptr->m_list[i].fy, player_ptr->current_floor_ptr->m_list[i].fx))
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

	/* Reset map panel */
	panel_row_min = player_ptr->current_floor_ptr->height;
	panel_col_min = player_ptr->current_floor_ptr->width;

	/* Sexy gal gets bonus to maximum weapon skill of whip */
	if (player_ptr->pseikaku == SEIKAKU_SEXY)
		s_info[player_ptr->pclass].w_max[TV_HAFTED-TV_WEAPON_BEGIN][SV_WHIP] = WEAPON_EXP_MASTER;

	/* Fill the arrays of floors and walls in the good proportions */
	set_floor_and_wall(player_ptr->dungeon_idx);

	/* Flavor the objects */
	flavor_init();

	/* Flash a message */
	prt(_("お待ち下さい...", "Please wait..."), 0, 0);

	/* Flush the message */
	Term_fresh();


	/* Hack -- Enter wizard mode */
	if (arg_wizard)
	{
		if (enter_wizard_mode())
		{
			current_world_ptr->wizard = TRUE;

			if (player_ptr->is_dead || !player_ptr->y || !player_ptr->x)
			{
				/* Initialize the saved floors data */
				init_saved_floors(TRUE);

				/* Avoid crash */
				player_ptr->current_floor_ptr->inside_quest = 0;

				/* Avoid crash in update_view() */
				player_ptr->y = player_ptr->x = 10;
			}
		}
		else if (player_ptr->is_dead)
		{
			quit("Already dead.");
		}
	}

	/* Initialize the town-buildings if necessary */
	if (!player_ptr->current_floor_ptr->dun_level && !player_ptr->current_floor_ptr->inside_quest)
	{
		process_dungeon_file("w_info.txt", 0, 0, current_world_ptr->max_wild_y, current_world_ptr->max_wild_x);
		init_flags = INIT_ONLY_BUILDINGS;
		process_dungeon_file("t_info.txt", 0, 0, MAX_HGT, MAX_WID);
		select_floor_music(player_ptr);
	}

	/* Generate a dungeon level if needed */
	if (!current_world_ptr->character_dungeon)
	{
		change_floor(player_ptr);
	}
	else
	{
		/* HACK -- Restore from panic-save */
		if (player_ptr->panic_save)
		{
			/* No player?  -- Try to regenerate floor */
			if (!player_ptr->y || !player_ptr->x)
			{
				msg_print(_("プレイヤーの位置がおかしい。フロアを再生成します。", "What a strange player location.  Regenerate the dungeon floor."));
				change_floor(player_ptr);
			}

			/* Still no player?  -- Try to locate random place */
			if (!player_ptr->y || !player_ptr->x) player_ptr->y = player_ptr->x = 10;

			/* No longer in panic */
			player_ptr->panic_save = 0;
		}
	}

	/* Character is now "complete" */
	current_world_ptr->character_generated = TRUE;


	/* Hack -- Character is no longer "icky" */
	current_world_ptr->character_icky = FALSE;


	if (new_game)
	{
		char buf[80];
		sprintf(buf, _("%sに降り立った。", "You are standing in the %s."), map_name());
		exe_write_diary(player_ptr, NIKKI_BUNSHOU, 0, buf);
	}


	/* Start game */
	player_ptr->playing = TRUE;

	/* Reset the visual mappings */
	reset_visuals();

	/* Load the "pref" files */
	load_all_pref_files();

	/* Give startup outfit (after loading pref files) */
	if (new_game)
	{
		player_outfit(player_ptr);
	}

	/* React to changes */
	Term_xtra(TERM_XTRA_REACT, 0);

	player_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER);
	player_ptr->window |= (PW_MESSAGE | PW_OVERHEAD | PW_DUNGEON | PW_MONSTER | PW_OBJECT);
	handle_stuff();

	/* Set or clear "rogue_like_commands" if requested */
	if (arg_force_original) rogue_like_commands = FALSE;
	if (arg_force_roguelike) rogue_like_commands = TRUE;

	/* Hack -- Enforce "delayed death" */
	if (player_ptr->chp < 0) player_ptr->is_dead = TRUE;

	if (player_ptr->prace == RACE_ANDROID) calc_android_exp(player_ptr);

	if (new_game && ((player_ptr->pclass == CLASS_CAVALRY) || (player_ptr->pclass == CLASS_BEASTMASTER)))
	{
		monster_type *m_ptr;
		MONRACE_IDX pet_r_idx = ((player_ptr->pclass == CLASS_CAVALRY) ? MON_HORSE : MON_YASE_HORSE);
		monster_race *r_ptr = &r_info[pet_r_idx];
		place_monster_aux(0, player_ptr->y, player_ptr->x - 1, pet_r_idx,
				  (PM_FORCE_PET | PM_NO_KAGE));
		m_ptr = &player_ptr->current_floor_ptr->m_list[hack_m_idx_ii];
		m_ptr->mspeed = r_ptr->speed;
		m_ptr->maxhp = r_ptr->hdice*(r_ptr->hside+1)/2;
		m_ptr->max_maxhp = m_ptr->maxhp;
		m_ptr->hp = r_ptr->hdice*(r_ptr->hside+1)/2;
		m_ptr->dealt_damage = 0;
		m_ptr->energy_need = ENERGY_NEED() + ENERGY_NEED();
	}

	(void)combine_and_reorder_home(STORE_HOME);
	(void)combine_and_reorder_home(STORE_MUSEUM);

	select_floor_music(player_ptr);

	/* Process */
	while (TRUE)
	{
		/* Process the level */
		dungeon(player_ptr, load_game);

		/* Hack -- prevent "icky" message */
		current_world_ptr->character_xtra = TRUE;

		handle_stuff();

		current_world_ptr->character_xtra = FALSE;

		/* Cancel the target */
		target_who = 0;

		/* Cancel the health bar */
		health_track(0);

		forget_lite(player_ptr->current_floor_ptr);
		forget_view(player_ptr->current_floor_ptr);
		clear_mon_lite(player_ptr->current_floor_ptr);

		/* Handle "quit and save" */
		if (!player_ptr->playing && !player_ptr->is_dead) break;

		wipe_o_list(player_ptr->current_floor_ptr);
		if (!player_ptr->is_dead) wipe_m_list();


		msg_print(NULL);

		load_game = FALSE;

		/* Accidental Death */
		if (player_ptr->playing && player_ptr->is_dead)
		{
			if (player_ptr->current_floor_ptr->inside_arena)
			{
				player_ptr->current_floor_ptr->inside_arena = FALSE;
				if (player_ptr->arena_number > MAX_ARENA_MONS)
					player_ptr->arena_number++;
				else
					player_ptr->arena_number = -1 - player_ptr->arena_number;
				player_ptr->is_dead = FALSE;
				player_ptr->chp = 0;
				player_ptr->chp_frac = 0;
				player_ptr->exit_bldg = TRUE;
				reset_tim_flags(player_ptr);

				/* Leave through the exit */
				prepare_change_floor_mode(CFM_SAVE_FLOORS | CFM_RAND_CONNECT);

				/* prepare next floor */
				leave_floor(player_ptr);
			}
			else
			{
				/* Mega-Hack -- Allow player to cheat death */
				if ((current_world_ptr->wizard || cheat_live) && !get_check(_("死にますか? ", "Die? ")))
				{
					cheat_death(player_ptr);
				}
			}
		}

		/* Handle "death" */
		if (player_ptr->is_dead) break;

		/* Make a new level */
		change_floor(player_ptr);
	}

	/* Close stuff */
	close_game();

	/* Quit */
	quit(NULL);
}

/*!
 * @brief ゲームターンからの実時間換算を行うための補正をかける
 * @param hoge ゲームターン
 * @details アンデッド種族は18:00からゲームを開始するので、この修正を予め行う。
 * @return 修正をかけた後のゲームターン
 */
s32b turn_real(s32b hoge)
{
	switch (p_ptr->start_race)
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
 * @details ターン及びターンを記録する変数をターンの限界の1日前まで巻き戻す.
 * @return 修正をかけた後のゲームターン
 */
void prevent_turn_overflow(void)
{
	int rollback_days, i, j;
	s32b rollback_turns;

	if (current_world_ptr->game_turn < current_world_ptr->game_turn_limit) return;

	rollback_days = 1 + (current_world_ptr->game_turn - current_world_ptr->game_turn_limit) / (TURNS_PER_TICK * TOWN_DAWN);
	rollback_turns = TURNS_PER_TICK * TOWN_DAWN * rollback_days;

	if (current_world_ptr->game_turn > rollback_turns) current_world_ptr->game_turn -= rollback_turns;
	else current_world_ptr->game_turn = 1;
	if (p_ptr->current_floor_ptr->generated_turn > rollback_turns) p_ptr->current_floor_ptr->generated_turn -= rollback_turns;
	else p_ptr->current_floor_ptr->generated_turn = 1;
	if (current_world_ptr->arena_start_turn > rollback_turns) current_world_ptr->arena_start_turn -= rollback_turns;
	else current_world_ptr->arena_start_turn = 1;
	if (p_ptr->feeling_turn > rollback_turns) p_ptr->feeling_turn -= rollback_turns;
	else p_ptr->feeling_turn = 1;

	for (i = 1; i < max_towns; i++)
	{
		for (j = 0; j < MAX_STORES; j++)
		{
			store_type *st_ptr = &town_info[i].store[j];

			if (st_ptr->last_visit > -10L * TURNS_PER_TICK * STORE_TICKS)
			{
				st_ptr->last_visit -= rollback_turns;
				if (st_ptr->last_visit < -10L * TURNS_PER_TICK * STORE_TICKS) st_ptr->last_visit = -10L * TURNS_PER_TICK * STORE_TICKS;
			}

			if (st_ptr->store_open)
			{
				st_ptr->store_open -= rollback_turns;
				if (st_ptr->store_open < 1) st_ptr->store_open = 1;
			}
		}
	}
}

/*!
 * @brief ゲーム終了処理 /
 * Close up the current game (player may or may not be dead)
 * @return なし
 * @details
 * <pre>
 * This function is called only from "main.c" and "signals.c".
 * </pre>
 */
void close_game(void)
{
	char buf[1024];
	bool do_send = TRUE;

	/*	concptr p = "[i:キャラクタの情報, f:ファイル書き出し, t:スコア, x:*鑑定*, ESC:ゲーム終了]"; */
	handle_stuff();

	/* Flush the messages */
	msg_print(NULL);

	/* Flush the input */
	flush();


	/* No suspending now */
	signals_ignore_tstp();


	/* Hack -- Character is now "icky" */
	current_world_ptr->character_icky = TRUE;

	path_build(buf, sizeof(buf), ANGBAND_DIR_APEX, "scores.raw");

	/* Grab permissions */
	safe_setuid_grab();

	/* Open the high score file, for reading/writing */
	highscore_fd = fd_open(buf, O_RDWR);

	/* Drop permissions */
	safe_setuid_drop();

	/* Handle death */
	if (p_ptr->is_dead)
	{
		/* Handle retirement */
		if (current_world_ptr->total_winner) kingly(p_ptr);

		/* Save memories */
		if (!cheat_save || get_check(_("死んだデータをセーブしますか？ ", "Save death? ")))
		{
			if (!save_player()) msg_print(_("セーブ失敗！", "death save failed!"));
		}
		else do_send = FALSE;

		/* You are dead */
		print_tomb();

		flush();

		/* Show more info */
		show_info();
		Term_clear();

		if (check_score())
		{
			if ((!send_world_score(do_send)))
			{
				if (get_check_strict(_("後でスコアを登録するために待機しますか？", "Stand by for later score registration? "),
					(CHECK_NO_ESCAPE | CHECK_NO_HISTORY)))
				{
					p_ptr->wait_report_score = TRUE;
					p_ptr->is_dead = FALSE;
					if (!save_player()) msg_print(_("セーブ失敗！", "death save failed!"));
				}
			}
			if (!p_ptr->wait_report_score)
				(void)top_twenty(p_ptr);
		}
		else if (highscore_fd >= 0)
		{
			display_scores_aux(0, 10, -1, NULL);
		}
#if 0
		/* Dump bones file */
		make_bones();
#endif
	}

	/* Still alive */
	else
	{
		/* Save the game */
		do_cmd_save_game(FALSE);

		/* Prompt for scores */
		prt(_("リターンキーか ESC キーを押して下さい。", "Press Return (or Escape)."), 0, 40);
		play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_EXIT);

		/* Predict score (or ESCAPE) */
		if (inkey() != ESCAPE) predict_score(p_ptr);
	}


	/* Shut the high score file */
	(void)fd_close(highscore_fd);

	/* Forget the high score fd */
	highscore_fd = -1;

	/* Kill all temporal files */
	clear_saved_floor_files();

	/* Allow suspending now */
	signals_handle_tstp();
}


/*!
 * @brief 全更新処理をチェックして処理していく
 * Handle "p_ptr->update" and "p_ptr->redraw" and "p_ptr->window"
 * @return なし
 */
void handle_stuff(void)
{
	if (p_ptr->update) update_creature(p_ptr);
	if (p_ptr->redraw) redraw_stuff(p_ptr);
	if (p_ptr->window) window_stuff();
}

void update_output(void)
{
	if (p_ptr->redraw) redraw_stuff(p_ptr);
	if (p_ptr->window) window_stuff();
}

