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
	if (disturb_minor) disturb(FALSE, FALSE);

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

	/* Message (p_ptr->inventory_list) */
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
 * Sense the p_ptr->inventory_list\n
 *\n
 *   Class 0 = Warrior --> fast and heavy\n
 *   Class 1 = Mage    --> slow and light\n
 *   Class 2 = Priest  --> fast but light\n
 *   Class 3 = Rogue   --> okay and heavy\n
 *   Class 4 = Ranger  --> slow but heavy  (changed!)\n
 *   Class 5 = Paladin --> slow but heavy\n
 */
static void sense_inventory1(void)
{
	INVENTORY_IDX i;
	PLAYER_LEVEL plev = p_ptr->lev;
	bool heavy = FALSE;
	object_type *o_ptr;


	/*** Check for "sensing" ***/

	/* No sensing when confused */
	if (p_ptr->confused) return;

	/* Analyze the class */
	switch (p_ptr->pclass)
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

	if (compare_virtue(p_ptr, V_KNOWLEDGE, 100, VIRTUE_LARGE)) heavy = TRUE;

	/*** Sense everything ***/

	/* Check everything */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		bool okay = FALSE;

		o_ptr = &p_ptr->inventory_list[i];

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

		/* Occasional failure on p_ptr->inventory_list items */
		if ((i < INVEN_RARM) && (0 != randint0(5))) continue;

		/* Good luck */
		if ((p_ptr->muta3 & MUT3_GOOD_LUCK) && !randint0(13))
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
static void sense_inventory2(void)
{
	INVENTORY_IDX i;
	PLAYER_LEVEL plev = p_ptr->lev;
	object_type *o_ptr;


	/*** Check for "sensing" ***/

	/* No sensing when confused */
	if (p_ptr->confused) return;

	/* Analyze the class */
	switch (p_ptr->pclass)
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

		o_ptr = &p_ptr->inventory_list[i];

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

		/* Occasional failure on p_ptr->inventory_list items */
		if ((i < INVEN_RARM) && (0 != randint0(5))) continue;

		sense_inventory_aux(i, TRUE);
	}
}

/*!
 * @brief パターン終点到達時のテレポート処理を行う
 * @return なし
 */
static void pattern_teleport(void)
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
			min_level = current_floor_ptr->dun_level;

		/* Maximum level */
		if (p_ptr->dungeon_idx == DUNGEON_ANGBAND)
		{
			if (current_floor_ptr->dun_level > 100)
				max_level = MAX_DEPTH - 1;
			else if (current_floor_ptr->dun_level == 100)
				max_level = 100;
		}
		else
		{
			max_level = d_info[p_ptr->dungeon_idx].maxdepth;
			min_level = d_info[p_ptr->dungeon_idx].mindepth;
		}

		/* Prompt */
		sprintf(ppp, _("テレポート先:(%d-%d)", "Teleport to level (%d-%d): "), (int)min_level, (int)max_level);

		/* Default */
		sprintf(tmp_val, "%d", (int)current_floor_ptr->dun_level);

		/* Ask for a level */
		if (!get_string(ppp, tmp_val, 10)) return;

		/* Extract request */
		command_arg = (COMMAND_ARG)atoi(tmp_val);
	}
	else if (get_check(_("通常テレポート？", "Normal teleport? ")))
	{
		teleport_player(200, 0L);
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
	current_floor_ptr->dun_level = command_arg;

	leave_quest_check();

	if (record_stair) do_cmd_write_nikki(NIKKI_PAT_TELE, 0, NULL);

	p_ptr->inside_quest = 0;
	free_turn(p_ptr);

	/*
	 * Clear all saved floors
	 * and create a first saved floor
	 */
	prepare_change_floor_mode(CFM_FIRST_FLOOR);
	p_ptr->leaving = TRUE;
}

/*!
 * @brief 各種パターン地形上の特別な処理 / Returns TRUE if we are on the Pattern...
 * @return 実際にパターン地形上にプレイヤーが居た場合はTRUEを返す。
 */
static bool pattern_effect(void)
{
	int pattern_type;

	if (!pattern_tile(p_ptr->y, p_ptr->x)) return FALSE;

	if ((PRACE_IS_(p_ptr, RACE_AMBERITE)) &&
	    (p_ptr->cut > 0) && one_in_(10))
	{
		wreck_the_pattern(p_ptr);
	}

	pattern_type = f_info[current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].feat].subtype;

	switch (pattern_type)
	{
	case PATTERN_TILE_END:
		(void)set_image(p_ptr, 0);
		(void)restore_all_status();
		(void)restore_level(p_ptr);
		(void)cure_critical_wounds(1000);

		cave_set_feat(p_ptr->y, p_ptr->x, feat_pattern_old);
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
		pattern_teleport();
		break;

	case PATTERN_TILE_WRECKED:
		if (!IS_INVULN())
			take_hit(p_ptr, DAMAGE_NOESCAPE, 200, _("壊れた「パターン」を歩いたダメージ", "walking the corrupted Pattern"), -1);
		break;

	default:
		if (PRACE_IS_(p_ptr, RACE_AMBERITE) && !one_in_(2))
			return TRUE;
		else if (!IS_INVULN())
			take_hit(p_ptr, DAMAGE_NOESCAPE, damroll(1, 3), _("「パターン」を歩いたダメージ", "walking the Pattern"), -1);
		break;
	}

	return TRUE;
}


/*!
 * @brief プレイヤーのHP自然回復処理 / Regenerate hit points -RAK-
 * @param percent 回復比率
 * @return なし
 */
static void regenhp(int percent)
{
	HIT_POINT new_chp;
	u32b new_chp_frac;
	HIT_POINT old_chp;

	if (p_ptr->special_defense & KATA_KOUKIJIN) return;
	if (p_ptr->action == ACTION_HAYAGAKE) return;

	/* Save the old hitpoints */
	old_chp = p_ptr->chp;

	/*
	 * Extract the new hitpoints
	 *
	 * 'percent' is the Regen factor in unit (1/2^16)
	 */
	new_chp = 0;
	new_chp_frac = (p_ptr->mhp * percent + PY_REGEN_HPBASE);

	/* Convert the unit (1/2^16) to (1/2^32) */
	s64b_LSHIFT(new_chp, new_chp_frac, 16);

	/* Regenerating */
	s64b_add(&(p_ptr->chp), &(p_ptr->chp_frac), new_chp, new_chp_frac);


	/* Fully healed */
	if (0 < s64b_cmp(p_ptr->chp, p_ptr->chp_frac, p_ptr->mhp, 0))
	{
		p_ptr->chp = p_ptr->mhp;
		p_ptr->chp_frac = 0;
	}

	/* Notice changes */
	if (old_chp != p_ptr->chp)
	{
		p_ptr->redraw |= (PR_HP);
		p_ptr->window |= (PW_PLAYER);
		wild_regen = 20;
	}
}


/*!
 * @brief プレイヤーのMP自然回復処理(regen_magic()のサブセット) / Regenerate mana points
 * @param upkeep_factor ペット維持によるMPコスト量
 * @param regen_amount 回復量
 * @return なし
 */
static void regenmana(MANA_POINT upkeep_factor, MANA_POINT regen_amount)
{
	MANA_POINT old_csp = p_ptr->csp;
	s32b regen_rate = regen_amount * 100 - upkeep_factor * PY_REGEN_NORMAL;

	/*
	 * Excess mana will decay 32 times faster than normal
	 * regeneration rate.
	 */
	if (p_ptr->csp > p_ptr->msp)
	{
		/* PY_REGEN_NORMAL is the Regen factor in unit (1/2^16) */
		s32b decay = 0;
		u32b decay_frac = (p_ptr->msp * 32 * PY_REGEN_NORMAL + PY_REGEN_MNBASE);

		/* Convert the unit (1/2^16) to (1/2^32) */
		s64b_LSHIFT(decay, decay_frac, 16);

		/* Decay */
		s64b_sub(&(p_ptr->csp), &(p_ptr->csp_frac), decay, decay_frac);

		/* Stop decaying */
		if (p_ptr->csp < p_ptr->msp)
		{
			p_ptr->csp = p_ptr->msp;
			p_ptr->csp_frac = 0;
		}
	}

	/* Regenerating mana (unless the player has excess mana) */
	else if (regen_rate > 0)
	{
		/* (percent/100) is the Regen factor in unit (1/2^16) */
		MANA_POINT new_mana = 0;
		u32b new_mana_frac = (p_ptr->msp * regen_rate / 100 + PY_REGEN_MNBASE);

		/* Convert the unit (1/2^16) to (1/2^32) */
		s64b_LSHIFT(new_mana, new_mana_frac, 16);

		/* Regenerate */
		s64b_add(&(p_ptr->csp), &(p_ptr->csp_frac), new_mana, new_mana_frac);

		/* Must set frac to zero even if equal */
		if (p_ptr->csp >= p_ptr->msp)
		{
			p_ptr->csp = p_ptr->msp;
			p_ptr->csp_frac = 0;
		}
	}


	/* Reduce mana (even when the player has excess mana) */
	if (regen_rate < 0)
	{
		/* PY_REGEN_NORMAL is the Regen factor in unit (1/2^16) */
		s32b reduce_mana = 0;
		u32b reduce_mana_frac = (p_ptr->msp * (-1) * regen_rate / 100 + PY_REGEN_MNBASE);

		/* Convert the unit (1/2^16) to (1/2^32) */
		s64b_LSHIFT(reduce_mana, reduce_mana_frac, 16);

		/* Reduce mana */
		s64b_sub(&(p_ptr->csp), &(p_ptr->csp_frac), reduce_mana, reduce_mana_frac);

		/* Check overflow */
		if (p_ptr->csp < 0)
		{
			p_ptr->csp = 0;
			p_ptr->csp_frac = 0;
		}
	}

	if (old_csp != p_ptr->csp)
	{
		p_ptr->redraw |= (PR_MANA);
		p_ptr->window |= (PW_PLAYER);
		p_ptr->window |= (PW_SPELL);
		wild_regen = 20;
	}
}

/*!
 * @brief プレイヤーのMP自然回復処理 / Regenerate magic regen_amount: PY_REGEN_NORMAL * 2 (if resting) * 2 (if having regenarate)
 * @param regen_amount 回復量
 * @return なし
 */
static void regenmagic(int regen_amount)
{
	MANA_POINT new_mana;
	int i;
	int dev = 30;
	int mult = (dev + adj_mag_mana[p_ptr->stat_ind[A_INT]]); /* x1 to x2 speed bonus for recharging */

	for (i = 0; i < EATER_EXT*2; i++)
	{
		if (!p_ptr->magic_num2[i]) continue;
		if (p_ptr->magic_num1[i] == ((long)p_ptr->magic_num2[i] << 16)) continue;

		/* Increase remaining charge number like float value */
		new_mana = (regen_amount * mult * ((long)p_ptr->magic_num2[i] + 13)) / (dev * 8);
		p_ptr->magic_num1[i] += new_mana;

		/* Check maximum charge */
		if (p_ptr->magic_num1[i] > (p_ptr->magic_num2[i] << 16))
		{
			p_ptr->magic_num1[i] = ((long)p_ptr->magic_num2[i] << 16);
		}
		wild_regen = 20;
	}
	for (i = EATER_EXT*2; i < EATER_EXT*3; i++)
	{
		if (!p_ptr->magic_num1[i]) continue;
		if (!p_ptr->magic_num2[i]) continue;

		/* Decrease remaining period for charging */
		new_mana = (regen_amount * mult * ((long)p_ptr->magic_num2[i] + 10) * EATER_ROD_CHARGE) 
					/ (dev * 16 * PY_REGEN_NORMAL); 
		p_ptr->magic_num1[i] -= new_mana;

		/* Check minimum remaining period for charging */
		if (p_ptr->magic_num1[i] < 0) p_ptr->magic_num1[i] = 0;
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
	for (i = 1; i < current_floor_ptr->m_max; i++)
	{
		/* Check the i'th monster */
		monster_type *m_ptr = &current_floor_ptr->m_list[i];
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
static void notice_lite_change(object_type *o_ptr)
{
	/* Hack -- notice interesting fuel steps */
	if ((o_ptr->xtra4 < 100) || (!(o_ptr->xtra4 % 100)))
	{
		p_ptr->window |= (PW_EQUIP);
	}

	/* Hack -- Special treatment when blind */
	if (p_ptr->blind)
	{
		/* Hack -- save some light for later */
		if (o_ptr->xtra4 == 0) o_ptr->xtra4++;
	}

	/* The light is now out */
	else if (o_ptr->xtra4 == 0)
	{
		disturb(FALSE, TRUE);
		msg_print(_("明かりが消えてしまった！", "Your light has gone out!"));

		/* Recalculate torch radius */
		p_ptr->update |= (PU_TORCH);

		/* Some ego light lose its effects without fuel */
		p_ptr->update |= (PU_BONUS);
	}

	/* The light is getting dim */
	else if (o_ptr->name2 == EGO_LITE_LONG)
	{
		if ((o_ptr->xtra4 < 50) && (!(o_ptr->xtra4 % 5))
		    && (current_world_ptr->game_turn % (TURNS_PER_TICK*2)))
		{
			if (disturb_minor) disturb(FALSE, TRUE);
			msg_print(_("明かりが微かになってきている。", "Your light is growing faint."));
		}
	}

	/* The light is getting dim */
	else if ((o_ptr->xtra4 < 100) && (!(o_ptr->xtra4 % 10)))
	{
		if (disturb_minor) disturb(FALSE, TRUE);
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

			disturb(FALSE, FALSE);

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
static void check_music(void)
{
	const magic_type *s_ptr;
	int spell;
	MANA_POINT need_mana;
	u32b need_mana_frac;

	/* Music singed by player */
	if (p_ptr->pclass != CLASS_BARD) return;
	if (!SINGING_SONG_EFFECT(p_ptr) && !INTERUPTING_SONG_EFFECT(p_ptr)) return;

	if (p_ptr->anti_magic)
	{
		stop_singing(p_ptr);
		return;
	}

	spell = SINGING_SONG_ID(p_ptr);
	s_ptr = &technic_info[REALM_MUSIC - MIN_TECHNIC][spell];

	need_mana = mod_need_mana(s_ptr->smana, spell, REALM_MUSIC);
	need_mana_frac = 0;

	/* Divide by 2 */
	s64b_RSHIFT(need_mana, need_mana_frac, 1);

	if (s64b_cmp(p_ptr->csp, p_ptr->csp_frac, need_mana, need_mana_frac) < 0)
	{
		stop_singing(p_ptr);
		return;
	}
	else
	{
		s64b_sub(&(p_ptr->csp), &(p_ptr->csp_frac), need_mana, need_mana_frac);

		p_ptr->redraw |= PR_MANA;
		if (INTERUPTING_SONG_EFFECT(p_ptr))
		{
			SINGING_SONG_EFFECT(p_ptr) = INTERUPTING_SONG_EFFECT(p_ptr);
			INTERUPTING_SONG_EFFECT(p_ptr) = MUSIC_NONE;
			msg_print(_("歌を再開した。", "You restart singing."));
			p_ptr->action = ACTION_SING;
			p_ptr->update |= (PU_BONUS | PU_HP | PU_MONSTERS);
			p_ptr->redraw |= (PR_MAP | PR_STATUS | PR_STATE);
			p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
		}
	}
	if (p_ptr->spell_exp[spell] < SPELL_EXP_BEGINNER)
		p_ptr->spell_exp[spell] += 5;
	else if(p_ptr->spell_exp[spell] < SPELL_EXP_SKILLED)
	{ if (one_in_(2) && (current_floor_ptr->dun_level > 4) && ((current_floor_ptr->dun_level + 10) > p_ptr->lev)) p_ptr->spell_exp[spell] += 1; }
	else if(p_ptr->spell_exp[spell] < SPELL_EXP_EXPERT)
	{ if (one_in_(5) && ((current_floor_ptr->dun_level + 5) > p_ptr->lev) && ((current_floor_ptr->dun_level + 5) > s_ptr->slevel)) p_ptr->spell_exp[spell] += 1; }
	else if(p_ptr->spell_exp[spell] < SPELL_EXP_MASTER)
	{ if (one_in_(5) && ((current_floor_ptr->dun_level + 5) > p_ptr->lev) && (current_floor_ptr->dun_level > s_ptr->slevel)) p_ptr->spell_exp[spell] += 1; }

	/* Do any effects of continual song */
	do_spell(REALM_MUSIC, spell, SPELL_CONT);
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

static void process_world_aux_digestion(void)
{
	if (!p_ptr->phase_out)
	{
		/* Digest quickly when gorged */
		if (p_ptr->food >= PY_FOOD_MAX)
		{
			/* Digest a lot of food */
			(void)set_food(p_ptr, p_ptr->food - 100);
		}

		/* Digest normally -- Every 50 game turns */
		else if (!(current_world_ptr->game_turn % (TURNS_PER_TICK * 5)))
		{
			/* Basic digestion rate based on speed */
			int digestion = SPEED_TO_ENERGY(p_ptr->pspeed);

			/* Regeneration takes more food */
			if (p_ptr->regenerate)
				digestion += 20;
			if (p_ptr->special_defense & (KAMAE_MASK | KATA_MASK))
				digestion += 20;
			if (p_ptr->cursed & TRC_FAST_DIGEST)
				digestion += 30;

			/* Slow digestion takes less food */
			if (p_ptr->slow_digest)
				digestion -= 5;

			/* Minimal digestion */
			if (digestion < 1) digestion = 1;
			/* Maximal digestion */
			if (digestion > 100) digestion = 100;

			/* Digest some food */
			(void)set_food(p_ptr, p_ptr->food - digestion);
		}


		/* Getting Faint */
		if ((p_ptr->food < PY_FOOD_FAINT))
		{
			/* Faint occasionally */
			if (!p_ptr->paralyzed && (randint0(100) < 10))
			{
				msg_print(_("あまりにも空腹で気絶してしまった。", "You faint from the lack of food."));
				disturb(TRUE, TRUE);

				/* Hack -- faint (bypass free action) */
				(void)set_paralyzed(p_ptr, p_ptr->paralyzed + 1 + randint0(5));
			}

			/* Starve to death (slowly) */
			if (p_ptr->food < PY_FOOD_STARVE)
			{
				/* Calculate damage */
				HIT_POINT dam = (PY_FOOD_STARVE - p_ptr->food) / 10;

				if (!IS_INVULN()) take_hit(p_ptr, DAMAGE_LOSELIFE, dam, _("空腹", "starvation"), -1);
			}
		}
	}
}

/*!
 * @brief 10ゲームターンが進行するごとにプレイヤーのHPとMPの増減処理を行う。
 *  / Handle timed damage and regeneration every 10 game turns
 * @return なし
 */
static void process_world_aux_hp_and_sp(void)
{
	feature_type *f_ptr = &f_info[current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].feat];
	bool cave_no_regen = FALSE;
	int upkeep_factor = 0;

	/* Default regeneration */
	int regen_amount = PY_REGEN_NORMAL;


	/*** Damage over Time ***/

	/* Take damage from poison */
	if (p_ptr->poisoned && !IS_INVULN())
	{
		take_hit(p_ptr, DAMAGE_NOESCAPE, 1, _("毒", "poison"), -1);
	}

	/* Take damage from cuts */
	if (p_ptr->cut && !IS_INVULN())
	{
		HIT_POINT dam;

		/* Mortal wound or Deep Gash */
		if (p_ptr->cut > 1000)
		{
			dam = 200;
		}

		else if (p_ptr->cut > 200)
		{
			dam = 80;
		}

		/* Severe cut */
		else if (p_ptr->cut > 100)
		{
			dam = 32;
		}

		else if (p_ptr->cut > 50)
		{
			dam = 16;
		}

		else if (p_ptr->cut > 25)
		{
			dam = 7;
		}

		else if (p_ptr->cut > 10)
		{
			dam = 3;
		}

		/* Other cuts */
		else
		{
			dam = 1;
		}

		take_hit(p_ptr, DAMAGE_NOESCAPE, dam, _("致命傷", "a fatal wound"), -1);
	}

	/* (Vampires) Take damage from sunlight */
	if (PRACE_IS_(p_ptr, RACE_VAMPIRE) || (p_ptr->mimic_form == MIMIC_VAMPIRE))
	{
		if (!current_floor_ptr->dun_level && !p_ptr->resist_lite && !IS_INVULN() && is_daytime())
		{
			if ((current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW)
			{
				msg_print(_("日光があなたのアンデッドの肉体を焼き焦がした！", "The sun's rays scorch your undead flesh!"));
				take_hit(p_ptr, DAMAGE_NOESCAPE, 1, _("日光", "sunlight"), -1);
				cave_no_regen = TRUE;
			}
		}

		if (p_ptr->inventory_list[INVEN_LITE].tval && (p_ptr->inventory_list[INVEN_LITE].name2 != EGO_LITE_DARKNESS) &&
		    !p_ptr->resist_lite)
		{
			object_type * o_ptr = &p_ptr->inventory_list[INVEN_LITE];
			GAME_TEXT o_name [MAX_NLEN];
			char ouch [MAX_NLEN+40];

			/* Get an object description */
			object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
			msg_format(_("%sがあなたのアンデッドの肉体を焼き焦がした！", "The %s scorches your undead flesh!"), o_name);

			cave_no_regen = TRUE;

			/* Get an object description */
			object_desc(o_name, o_ptr, OD_NAME_ONLY);
			sprintf(ouch, _("%sを装備したダメージ", "wielding %s"), o_name);

			if (!IS_INVULN()) take_hit(p_ptr, DAMAGE_NOESCAPE, 1, ouch, -1);
		}
	}

	if (have_flag(f_ptr->flags, FF_LAVA) && !IS_INVULN() && !p_ptr->immune_fire)
	{
		int damage = 0;

		if (have_flag(f_ptr->flags, FF_DEEP))
		{
			damage = 6000 + randint0(4000);
		}
		else if (!p_ptr->levitation)
		{
			damage = 3000 + randint0(2000);
		}

		if (damage)
		{
			if(PRACE_IS_(p_ptr, RACE_ENT)) damage += damage / 3;
			if(p_ptr->resist_fire) damage = damage / 3;
			if(IS_OPPOSE_FIRE()) damage = damage / 3;
			if(p_ptr->levitation) damage = damage / 5;

			damage = damage / 100 + (randint0(100) < (damage % 100));

			if (p_ptr->levitation)
			{
				msg_print(_("熱で火傷した！", "The heat burns you!"));
				take_hit(p_ptr, DAMAGE_NOESCAPE, damage, format(_("%sの上に浮遊したダメージ", "flying over %s"), 
								f_name + f_info[get_feat_mimic(&current_floor_ptr->grid_array[p_ptr->y][p_ptr->x])].name), -1);
			}
			else
			{
				concptr name = f_name + f_info[get_feat_mimic(&current_floor_ptr->grid_array[p_ptr->y][p_ptr->x])].name;
				msg_format(_("%sで火傷した！", "The %s burns you!"), name);
				take_hit(p_ptr, DAMAGE_NOESCAPE, damage, name, -1);
			}

			cave_no_regen = TRUE;
		}
	}

	if (have_flag(f_ptr->flags, FF_COLD_PUDDLE) && !IS_INVULN() && !p_ptr->immune_cold)
	{
		int damage = 0;

		if (have_flag(f_ptr->flags, FF_DEEP))
		{
			damage = 6000 + randint0(4000);
		}
		else if (!p_ptr->levitation)
		{
			damage = 3000 + randint0(2000);
		}

		if (damage)
		{
			if (p_ptr->resist_cold) damage = damage / 3;
			if (IS_OPPOSE_COLD()) damage = damage / 3;
			if (p_ptr->levitation) damage = damage / 5;

			damage = damage / 100 + (randint0(100) < (damage % 100));

			if (p_ptr->levitation)
			{
				msg_print(_("冷気に覆われた！", "The cold engulfs you!"));
				take_hit(p_ptr, DAMAGE_NOESCAPE, damage, format(_("%sの上に浮遊したダメージ", "flying over %s"),
					f_name + f_info[get_feat_mimic(&current_floor_ptr->grid_array[p_ptr->y][p_ptr->x])].name), -1);
			}
			else
			{
				concptr name = f_name + f_info[get_feat_mimic(&current_floor_ptr->grid_array[p_ptr->y][p_ptr->x])].name;
				msg_format(_("%sに凍えた！", "The %s frostbites you!"), name);
				take_hit(p_ptr, DAMAGE_NOESCAPE, damage, name, -1);
			}

			cave_no_regen = TRUE;
		}
	}

	if (have_flag(f_ptr->flags, FF_ELEC_PUDDLE) && !IS_INVULN() && !p_ptr->immune_elec)
	{
		int damage = 0;

		if (have_flag(f_ptr->flags, FF_DEEP))
		{
			damage = 6000 + randint0(4000);
		}
		else if (!p_ptr->levitation)
		{
			damage = 3000 + randint0(2000);
		}

		if (damage)
		{
			if (p_ptr->resist_elec) damage = damage / 3;
			if (IS_OPPOSE_ELEC()) damage = damage / 3;
			if (p_ptr->levitation) damage = damage / 5;

			damage = damage / 100 + (randint0(100) < (damage % 100));

			if (p_ptr->levitation)
			{
				msg_print(_("電撃を受けた！", "The electric shocks you!"));
				take_hit(p_ptr, DAMAGE_NOESCAPE, damage, format(_("%sの上に浮遊したダメージ", "flying over %s"),
					f_name + f_info[get_feat_mimic(&current_floor_ptr->grid_array[p_ptr->y][p_ptr->x])].name), -1);
			}
			else
			{
				concptr name = f_name + f_info[get_feat_mimic(&current_floor_ptr->grid_array[p_ptr->y][p_ptr->x])].name;
				msg_format(_("%sに感電した！", "The %s shocks you!"), name);
				take_hit(p_ptr, DAMAGE_NOESCAPE, damage, name, -1);
			}

			cave_no_regen = TRUE;
		}
	}

	if (have_flag(f_ptr->flags, FF_ACID_PUDDLE) && !IS_INVULN() && !p_ptr->immune_acid)
	{
		int damage = 0;

		if (have_flag(f_ptr->flags, FF_DEEP))
		{
			damage = 6000 + randint0(4000);
		}
		else if (!p_ptr->levitation)
		{
			damage = 3000 + randint0(2000);
		}

		if (damage)
		{
			if (p_ptr->resist_acid) damage = damage / 3;
			if (IS_OPPOSE_ACID()) damage = damage / 3;
			if (p_ptr->levitation) damage = damage / 5;

			damage = damage / 100 + (randint0(100) < (damage % 100));

			if (p_ptr->levitation)
			{
				msg_print(_("酸が飛び散った！", "The acid melt you!"));
				take_hit(p_ptr, DAMAGE_NOESCAPE, damage, format(_("%sの上に浮遊したダメージ", "flying over %s"),
					f_name + f_info[get_feat_mimic(&current_floor_ptr->grid_array[p_ptr->y][p_ptr->x])].name), -1);
			}
			else
			{
				concptr name = f_name + f_info[get_feat_mimic(&current_floor_ptr->grid_array[p_ptr->y][p_ptr->x])].name;
				msg_format(_("%sに溶かされた！", "The %s melts you!"), name);
				take_hit(p_ptr, DAMAGE_NOESCAPE, damage, name, -1);
			}

			cave_no_regen = TRUE;
		}
	}

	if (have_flag(f_ptr->flags, FF_POISON_PUDDLE) && !IS_INVULN())
	{
		int damage = 0;

		if (have_flag(f_ptr->flags, FF_DEEP))
		{
			damage = 6000 + randint0(4000);
		}
		else if (!p_ptr->levitation)
		{
			damage = 3000 + randint0(2000);
		}

		if (damage)
		{
			if (p_ptr->resist_pois) damage = damage / 3;
			if (IS_OPPOSE_POIS()) damage = damage / 3;
			if (p_ptr->levitation) damage = damage / 5;

			damage = damage / 100 + (randint0(100) < (damage % 100));

			if (p_ptr->levitation)
			{
				msg_print(_("毒気を吸い込んだ！", "The gas poisons you!"));
				take_hit(p_ptr, DAMAGE_NOESCAPE, damage, format(_("%sの上に浮遊したダメージ", "flying over %s"),
					f_name + f_info[get_feat_mimic(&current_floor_ptr->grid_array[p_ptr->y][p_ptr->x])].name), -1);
				if (p_ptr->resist_pois) (void)set_poisoned(p_ptr, p_ptr->poisoned + 1);
			}
			else
			{
				concptr name = f_name + f_info[get_feat_mimic(&current_floor_ptr->grid_array[p_ptr->y][p_ptr->x])].name;
				msg_format(_("%sに毒された！", "The %s poisons you!"), name);
				take_hit(p_ptr, DAMAGE_NOESCAPE, damage, name, -1);
				if (p_ptr->resist_pois) (void)set_poisoned(p_ptr, p_ptr->poisoned + 3);
			}

			cave_no_regen = TRUE;
		}
	}

	if (have_flag(f_ptr->flags, FF_WATER) && have_flag(f_ptr->flags, FF_DEEP) &&
	    !p_ptr->levitation && !p_ptr->can_swim && !p_ptr->resist_water)
	{
		if (p_ptr->total_weight > weight_limit(p_ptr))
		{
			msg_print(_("溺れている！", "You are drowning!"));
			take_hit(p_ptr, DAMAGE_NOESCAPE, randint1(p_ptr->lev), _("溺れ", "drowning"), -1);
			cave_no_regen = TRUE;
		}
	}

	if (p_ptr->riding)
	{
		HIT_POINT damage;
		if ((r_info[current_floor_ptr->m_list[p_ptr->riding].r_idx].flags2 & RF2_AURA_FIRE) && !p_ptr->immune_fire)
		{
			damage = r_info[current_floor_ptr->m_list[p_ptr->riding].r_idx].level / 2;
			if (PRACE_IS_(p_ptr, RACE_ENT)) damage += damage / 3;
			if (p_ptr->resist_fire) damage = damage / 3;
			if (IS_OPPOSE_FIRE()) damage = damage / 3;
			msg_print(_("熱い！", "It's hot!"));
			take_hit(p_ptr, DAMAGE_NOESCAPE, damage, _("炎のオーラ", "Fire aura"), -1);
		}
		if ((r_info[current_floor_ptr->m_list[p_ptr->riding].r_idx].flags2 & RF2_AURA_ELEC) && !p_ptr->immune_elec)
		{
			damage = r_info[current_floor_ptr->m_list[p_ptr->riding].r_idx].level / 2;
			if (PRACE_IS_(p_ptr, RACE_ANDROID)) damage += damage / 3;
			if (p_ptr->resist_elec) damage = damage / 3;
			if (IS_OPPOSE_ELEC()) damage = damage / 3;
			msg_print(_("痛い！", "It hurts!"));
			take_hit(p_ptr, DAMAGE_NOESCAPE, damage, _("電気のオーラ", "Elec aura"), -1);
		}
		if ((r_info[current_floor_ptr->m_list[p_ptr->riding].r_idx].flags3 & RF3_AURA_COLD) && !p_ptr->immune_cold)
		{
			damage = r_info[current_floor_ptr->m_list[p_ptr->riding].r_idx].level / 2;
			if (p_ptr->resist_cold) damage = damage / 3;
			if (IS_OPPOSE_COLD()) damage = damage / 3;
			msg_print(_("冷たい！", "It's cold!"));
			take_hit(p_ptr, DAMAGE_NOESCAPE, damage, _("冷気のオーラ", "Cold aura"), -1);
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
		if (!IS_INVULN() && !p_ptr->wraith_form && !p_ptr->kabenuke && ((p_ptr->chp > (p_ptr->lev / 5)) || !p_ptr->pass_wall))
		{
			concptr dam_desc;
			cave_no_regen = TRUE;

			if (p_ptr->pass_wall)
			{
				msg_print(_("体の分子が分解した気がする！", "Your molecules feel disrupted!"));
				dam_desc = _("密度", "density");
			}
			else
			{
				msg_print(_("崩れた岩に押し潰された！", "You are being crushed!"));
				dam_desc = _("硬い岩", "solid rock");
			}

			take_hit(p_ptr, DAMAGE_NOESCAPE, 1 + (p_ptr->lev / 5), dam_desc, -1);
		}
	}


	/*** handle regeneration ***/

	/* Getting Weak */
	if (p_ptr->food < PY_FOOD_WEAK)
	{
		/* Lower regeneration */
		if (p_ptr->food < PY_FOOD_STARVE)
		{
			regen_amount = 0;
		}
		else if (p_ptr->food < PY_FOOD_FAINT)
		{
			regen_amount = PY_REGEN_FAINT;
		}
		else
		{
			regen_amount = PY_REGEN_WEAK;
		}
	}

	/* Are we walking the pattern? */
	if (pattern_effect())
	{
		cave_no_regen = TRUE;
	}
	else
	{
		/* Regeneration ability */
		if (p_ptr->regenerate)
		{
			regen_amount = regen_amount * 2;
		}
		if (p_ptr->special_defense & (KAMAE_MASK | KATA_MASK))
		{
			regen_amount /= 2;
		}
		if (p_ptr->cursed & TRC_SLOW_REGEN)
		{
			regen_amount /= 5;
		}
	}


	/* Searching or Resting */
	if ((p_ptr->action == ACTION_SEARCH) || (p_ptr->action == ACTION_REST))
	{
		regen_amount = regen_amount * 2;
	}

	upkeep_factor = calculate_upkeep();

	/* No regeneration while special action */
	if ((p_ptr->action == ACTION_LEARN) ||
	    (p_ptr->action == ACTION_HAYAGAKE) ||
	    (p_ptr->special_defense & KATA_KOUKIJIN))
	{
		upkeep_factor += 100;
	}

	/* Regenerate the mana */
	regenmana(upkeep_factor, regen_amount);


	/* Recharge magic eater's power */
	if (p_ptr->pclass == CLASS_MAGIC_EATER)
	{
		regenmagic(regen_amount);
	}

	if ((p_ptr->csp == 0) && (p_ptr->csp_frac == 0))
	{
		while (upkeep_factor > 100)
		{
			msg_print(_("こんなに多くのペットを制御できない！", "Too many pets to control at once!"));
			msg_print(NULL);
			do_cmd_pet_dismiss();

			upkeep_factor = calculate_upkeep();

			msg_format(_("維持ＭＰは %d%%", "Upkeep: %d%% mana."), upkeep_factor);
			msg_print(NULL);
		}
	}

	/* Poisoned or cut yields no healing */
	if (p_ptr->poisoned) regen_amount = 0;
	if (p_ptr->cut) regen_amount = 0;

	/* Special floor -- Pattern, in a wall -- yields no healing */
	if (cave_no_regen) regen_amount = 0;

	regen_amount = (regen_amount * p_ptr->mutant_regenerate_mod) / 100;

	/* Regenerate Hit Points if needed */
	if ((p_ptr->chp < p_ptr->mhp) && !cave_no_regen)
	{
		regenhp(regen_amount);
	}
}

/*!
 * @brief 10ゲームターンが進行するごとに魔法効果の残りターンを減らしていく処理
 * / Handle timeout every 10 game turns
 * @return なし
 */
static void process_world_aux_timeout(void)
{
	const int dec_count = (easy_band ? 2 : 1);

	/*** Timeout Various Things ***/

	/* Mimic */
	if (p_ptr->tim_mimic)
	{
		(void)set_mimic(p_ptr, p_ptr->tim_mimic - 1, p_ptr->mimic_form, TRUE);
	}

	/* Hack -- Hallucinating */
	if (p_ptr->image)
	{
		(void)set_image(p_ptr, p_ptr->image - dec_count);
	}

	/* Blindness */
	if (p_ptr->blind)
	{
		(void)set_blind(p_ptr, p_ptr->blind - dec_count);
	}

	/* Times see-invisible */
	if (p_ptr->tim_invis)
	{
		(void)set_tim_invis(p_ptr, p_ptr->tim_invis - 1, TRUE);
	}

	if (p_ptr->suppress_multi_reward)
	{
		p_ptr->suppress_multi_reward = FALSE;
	}

	/* Timed esp */
	if (p_ptr->tim_esp)
	{
		(void)set_tim_esp(p_ptr, p_ptr->tim_esp - 1, TRUE);
	}

	/* Timed temporary elemental brands. -LM- */
	if (p_ptr->ele_attack)
	{
		p_ptr->ele_attack--;

		/* Clear all temporary elemental brands. */
		if (!p_ptr->ele_attack) set_ele_attack(p_ptr, 0, 0);
	}

	/* Timed temporary elemental immune. -LM- */
	if (p_ptr->ele_immune)
	{
		p_ptr->ele_immune--;

		/* Clear all temporary elemental brands. */
		if (!p_ptr->ele_immune) set_ele_immune(p_ptr, 0, 0);
	}

	/* Timed infra-vision */
	if (p_ptr->tim_infra)
	{
		(void)set_tim_infra(p_ptr, p_ptr->tim_infra - 1, TRUE);
	}

	/* Timed stealth */
	if (p_ptr->tim_stealth)
	{
		(void)set_tim_stealth(p_ptr, p_ptr->tim_stealth - 1, TRUE);
	}

	/* Timed levitation */
	if (p_ptr->tim_levitation)
	{
		(void)set_tim_levitation(p_ptr, p_ptr->tim_levitation - 1, TRUE);
	}

	/* Timed sh_touki */
	if (p_ptr->tim_sh_touki)
	{
		(void)set_tim_sh_touki(p_ptr, p_ptr->tim_sh_touki - 1, TRUE);
	}

	/* Timed sh_fire */
	if (p_ptr->tim_sh_fire)
	{
		(void)set_tim_sh_fire(p_ptr, p_ptr->tim_sh_fire - 1, TRUE);
	}

	/* Timed sh_holy */
	if (p_ptr->tim_sh_holy)
	{
		(void)set_tim_sh_holy(p_ptr, p_ptr->tim_sh_holy - 1, TRUE);
	}

	/* Timed eyeeye */
	if (p_ptr->tim_eyeeye)
	{
		(void)set_tim_eyeeye(p_ptr, p_ptr->tim_eyeeye - 1, TRUE);
	}

	/* Timed resist-magic */
	if (p_ptr->resist_magic)
	{
		(void)set_resist_magic(p_ptr, p_ptr->resist_magic - 1, TRUE);
	}

	/* Timed regeneration */
	if (p_ptr->tim_regen)
	{
		(void)set_tim_regen(p_ptr, p_ptr->tim_regen - 1, TRUE);
	}

	/* Timed resist nether */
	if (p_ptr->tim_res_nether)
	{
		(void)set_tim_res_nether(p_ptr, p_ptr->tim_res_nether - 1, TRUE);
	}

	/* Timed resist time */
	if (p_ptr->tim_res_time)
	{
		(void)set_tim_res_time(p_ptr, p_ptr->tim_res_time - 1, TRUE);
	}

	/* Timed reflect */
	if (p_ptr->tim_reflect)
	{
		(void)set_tim_reflect(p_ptr, p_ptr->tim_reflect - 1, TRUE);
	}

	/* Multi-shadow */
	if (p_ptr->multishadow)
	{
		(void)set_multishadow(p_ptr, p_ptr->multishadow - 1, TRUE);
	}

	/* Timed Robe of dust */
	if (p_ptr->dustrobe)
	{
		(void)set_dustrobe(p_ptr, p_ptr->dustrobe - 1, TRUE);
	}

	/* Timed infra-vision */
	if (p_ptr->kabenuke)
	{
		(void)set_kabenuke(p_ptr, p_ptr->kabenuke - 1, TRUE);
	}

	/* Paralysis */
	if (p_ptr->paralyzed)
	{
		(void)set_paralyzed(p_ptr, p_ptr->paralyzed - dec_count);
	}

	/* Confusion */
	if (p_ptr->confused)
	{
		(void)set_confused(p_ptr, p_ptr->confused - dec_count);
	}

	/* Afraid */
	if (p_ptr->afraid)
	{
		(void)set_afraid(p_ptr, p_ptr->afraid - dec_count);
	}

	/* Fast */
	if (p_ptr->fast)
	{
		(void)set_fast(p_ptr, p_ptr->fast - 1, TRUE);
	}

	/* Slow */
	if (p_ptr->slow)
	{
		(void)set_slow(p_ptr, p_ptr->slow - dec_count, TRUE);
	}

	/* Protection from evil */
	if (p_ptr->protevil)
	{
		(void)set_protevil(p_ptr, p_ptr->protevil - 1, TRUE);
	}

	/* Invulnerability */
	if (p_ptr->invuln)
	{
		(void)set_invuln(p_ptr, p_ptr->invuln - 1, TRUE);
	}

	/* Wraith form */
	if (p_ptr->wraith_form)
	{
		(void)set_wraith_form(p_ptr, p_ptr->wraith_form - 1, TRUE);
	}

	/* Heroism */
	if (p_ptr->hero)
	{
		(void)set_hero(p_ptr, p_ptr->hero - 1, TRUE);
	}

	/* Super Heroism */
	if (p_ptr->shero)
	{
		(void)set_shero(p_ptr, p_ptr->shero - 1, TRUE);
	}

	/* Blessed */
	if (p_ptr->blessed)
	{
		(void)set_blessed(p_ptr, p_ptr->blessed - 1, TRUE);
	}

	/* Shield */
	if (p_ptr->shield)
	{
		(void)set_shield(p_ptr, p_ptr->shield - 1, TRUE);
	}

	/* Tsubureru */
	if (p_ptr->tsubureru)
	{
		(void)set_tsubureru(p_ptr, p_ptr->tsubureru - 1, TRUE);
	}

	/* Magicdef */
	if (p_ptr->magicdef)
	{
		(void)set_magicdef(p_ptr, p_ptr->magicdef - 1, TRUE);
	}

	/* Tsuyoshi */
	if (p_ptr->tsuyoshi)
	{
		(void)set_tsuyoshi(p_ptr, p_ptr->tsuyoshi - 1, TRUE);
	}

	/* Oppose Acid */
	if (p_ptr->oppose_acid)
	{
		(void)set_oppose_acid(p_ptr, p_ptr->oppose_acid - 1, TRUE);
	}

	/* Oppose Lightning */
	if (p_ptr->oppose_elec)
	{
		(void)set_oppose_elec(p_ptr, p_ptr->oppose_elec - 1, TRUE);
	}

	/* Oppose Fire */
	if (p_ptr->oppose_fire)
	{
		(void)set_oppose_fire(p_ptr, p_ptr->oppose_fire - 1, TRUE);
	}

	/* Oppose Cold */
	if (p_ptr->oppose_cold)
	{
		(void)set_oppose_cold(p_ptr, p_ptr->oppose_cold - 1, TRUE);
	}

	/* Oppose Poison */
	if (p_ptr->oppose_pois)
	{
		(void)set_oppose_pois(p_ptr, p_ptr->oppose_pois - 1, TRUE);
	}

	if (p_ptr->ult_res)
	{
		(void)set_ultimate_res(p_ptr, p_ptr->ult_res - 1, TRUE);
	}

	/*** Poison and Stun and Cut ***/

	/* Poison */
	if (p_ptr->poisoned)
	{
		int adjust = adj_con_fix[p_ptr->stat_ind[A_CON]] + 1;

		/* Apply some healing */
		(void)set_poisoned(p_ptr, p_ptr->poisoned - adjust);
	}

	/* Stun */
	if (p_ptr->stun)
	{
		int adjust = adj_con_fix[p_ptr->stat_ind[A_CON]] + 1;

		/* Apply some healing */
		(void)set_stun(p_ptr, p_ptr->stun - adjust);
	}

	/* Cut */
	if (p_ptr->cut)
	{
		int adjust = adj_con_fix[p_ptr->stat_ind[A_CON]] + 1;

		/* Hack -- Truly "mortal" wound */
		if (p_ptr->cut > 1000) adjust = 0;

		/* Apply some healing */
		(void)set_cut(p_ptr,p_ptr->cut - adjust);
	}
}


/*!
 * @brief 10ゲームターンが進行する毎に光源の寿命を減らす処理
 * / Handle burning fuel every 10 game turns
 * @return なし
 */
static void process_world_aux_light(void)
{
	/* Check for light being wielded */
	object_type *o_ptr = &p_ptr->inventory_list[INVEN_LITE];

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
			notice_lite_change(o_ptr);
		}
	}
}


/*!
 * @brief 10ゲームターンが進行するごとに突然変異の発動判定を行う処理
 * / Handle mutation effects once every 10 game turns
 * @return なし
 */
static void process_world_aux_mutation(void)
{
	/* No mutation with effects */
	if (!p_ptr->muta2) return;

	/* No effect on monster arena */
	if (p_ptr->phase_out) return;

	/* No effect on the global map */
	if (p_ptr->wild_mode) return;

	if ((p_ptr->muta2 & MUT2_BERS_RAGE) && one_in_(3000))
	{
		disturb(FALSE, TRUE);
		msg_print(_("ウガァァア！", "RAAAAGHH!"));
		msg_print(_("激怒の発作に襲われた！", "You feel a fit of rage coming over you!"));
		(void)set_shero(p_ptr, 10 + randint1(p_ptr->lev), FALSE);
		(void)set_afraid(p_ptr, 0);
	}

	if ((p_ptr->muta2 & MUT2_COWARDICE) && (randint1(3000) == 13))
	{
		if (!p_ptr->resist_fear)
		{
			disturb(FALSE, TRUE);
			msg_print(_("とても暗い... とても恐い！", "It's so dark... so scary!"));
			set_afraid(p_ptr, p_ptr->afraid + 13 + randint1(26));
		}
	}

	if ((p_ptr->muta2 & MUT2_RTELEPORT) && (randint1(5000) == 88))
	{
		if (!p_ptr->resist_nexus && !(p_ptr->muta1 & MUT1_VTELEPORT) && !p_ptr->anti_tele)
		{
			disturb(FALSE, TRUE);
			msg_print(_("あなたの位置は突然ひじょうに不確定になった...", "Your position suddenly seems very uncertain..."));
			msg_print(NULL);
			teleport_player(40, TELEPORT_PASSIVE);
		}
	}

	if ((p_ptr->muta2 & MUT2_ALCOHOL) && (randint1(6400) == 321))
	{
		if (!p_ptr->resist_conf && !p_ptr->resist_chaos)
		{
			disturb(FALSE, TRUE);
			p_ptr->redraw |= PR_EXTRA;
			msg_print(_("いひきがもーろーとひてきたきがふる...ヒック！", "You feel a SSSCHtupor cOmINg over yOu... *HIC*!"));
		}

		if (!p_ptr->resist_conf)
		{
			(void)set_confused(p_ptr, p_ptr->confused + randint0(20) + 15);
		}

		if (!p_ptr->resist_chaos)
		{
			if (one_in_(20))
			{
				msg_print(NULL);
				if (one_in_(3)) lose_all_info(p_ptr);
				else wiz_dark();
				(void)teleport_player_aux(100, TELEPORT_NONMAGICAL | TELEPORT_PASSIVE);
				wiz_dark();
				msg_print(_("あなたは見知らぬ場所で目が醒めた...頭が痛い。", "You wake up somewhere with a sore head..."));
				msg_print(_("何も覚えていない。どうやってここに来たかも分からない！", "You can't remember a thing, or how you got here!"));
			}
			else
			{
				if (one_in_(3))
				{
					msg_print(_("き～れいなちょおちょらとんれいる～", "Thishcischs GooDSChtuff!"));
					(void)set_image(p_ptr, p_ptr->image + randint0(150) + 150);
				}
			}
		}
	}

	if ((p_ptr->muta2 & MUT2_HALLU) && (randint1(6400) == 42))
	{
		if (!p_ptr->resist_chaos)
		{
			disturb(FALSE, TRUE);
			p_ptr->redraw |= PR_EXTRA;
			(void)set_image(p_ptr, p_ptr->image + randint0(50) + 20);
		}
	}

	if ((p_ptr->muta2 & MUT2_FLATULENT) && (randint1(3000) == 13))
	{
		disturb(FALSE, TRUE);
		msg_print(_("ブゥーーッ！おっと。", "BRRAAAP! Oops."));
		msg_print(NULL);
		fire_ball(GF_POIS, 0, p_ptr->lev, 3);
	}

	if ((p_ptr->muta2 & MUT2_PROD_MANA) &&
	    !p_ptr->anti_magic && one_in_(9000))
	{
		int dire = 0;
		disturb(FALSE, TRUE);
		msg_print(_("魔法のエネルギーが突然あなたの中に流れ込んできた！エネルギーを解放しなければならない！", 
						"Magical energy flows through you! You must release it!"));

		flush();
		msg_print(NULL);
		(void)get_hack_dir(&dire);
		fire_ball(GF_MANA, dire, p_ptr->lev * 2, 3);
	}

	if ((p_ptr->muta2 & MUT2_ATT_DEMON) && !p_ptr->anti_magic && (randint1(6666) == 666))
	{
		bool pet = one_in_(6);
		BIT_FLAGS mode = PM_ALLOW_GROUP;

		if (pet) mode |= PM_FORCE_PET;
		else mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);

		if (summon_specific((pet ? -1 : 0), p_ptr->y, p_ptr->x, current_floor_ptr->dun_level, SUMMON_DEMON, mode))
		{
			msg_print(_("あなたはデーモンを引き寄せた！", "You have attracted a demon!"));
			disturb(FALSE, TRUE);
		}
	}

	if ((p_ptr->muta2 & MUT2_SPEED_FLUX) && one_in_(6000))
	{
		disturb(FALSE, TRUE);
		if (one_in_(2))
		{
			msg_print(_("精力的でなくなった気がする。", "You feel less energetic."));

			if (p_ptr->fast > 0)
			{
				set_fast(p_ptr, 0, TRUE);
			}
			else
			{
				set_slow(p_ptr, randint1(30) + 10, FALSE);
			}
		}
		else
		{
			msg_print(_("精力的になった気がする。", "You feel more energetic."));

			if (p_ptr->slow > 0)
			{
				set_slow(p_ptr, 0, TRUE);
			}
			else
			{
				set_fast(p_ptr, randint1(30) + 10, FALSE);
			}
		}
		msg_print(NULL);
	}
	if ((p_ptr->muta2 & MUT2_BANISH_ALL) && one_in_(9000))
	{
		disturb(FALSE, TRUE);
		msg_print(_("突然ほとんど孤独になった気がする。", "You suddenly feel almost lonely."));

		banish_monsters(100);
		if (!current_floor_ptr->dun_level && p_ptr->town_num)
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

	if ((p_ptr->muta2 & MUT2_EAT_LIGHT) && one_in_(3000))
	{
		object_type *o_ptr;

		msg_print(_("影につつまれた。", "A shadow passes over you."));
		msg_print(NULL);

		/* Absorb light from the current possition */
		if ((current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW)
		{
			hp_player(p_ptr, 10);
		}

		o_ptr = &p_ptr->inventory_list[INVEN_LITE];

		/* Absorb some fuel in the current lite */
		if (o_ptr->tval == TV_LITE)
		{
			/* Use some fuel (except on artifacts) */
			if (!object_is_fixed_artifact(o_ptr) && (o_ptr->xtra4 > 0))
			{
				/* Heal the player a bit */
				hp_player(p_ptr, o_ptr->xtra4 / 20);

				/* Decrease life-span of lite */
				o_ptr->xtra4 /= 2;
				msg_print(_("光源からエネルギーを吸収した！", "You absorb energy from your light!"));

				/* Notice interesting fuel steps */
				notice_lite_change(o_ptr);
			}
		}

		/*
		 * Unlite the area (radius 10) around player and
		 * do 50 points damage to every affected monster
		 */
		unlite_area(50, 10);
	}

	if ((p_ptr->muta2 & MUT2_ATT_ANIMAL) && !p_ptr->anti_magic && one_in_(7000))
	{
		bool pet = one_in_(3);
		BIT_FLAGS mode = PM_ALLOW_GROUP;

		if (pet) mode |= PM_FORCE_PET;
		else mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);

		if (summon_specific((pet ? -1 : 0), p_ptr->y, p_ptr->x, current_floor_ptr->dun_level, SUMMON_ANIMAL, mode))
		{
			msg_print(_("動物を引き寄せた！", "You have attracted an animal!"));
			disturb(FALSE, TRUE);
		}
	}

	if ((p_ptr->muta2 & MUT2_RAW_CHAOS) && !p_ptr->anti_magic && one_in_(8000))
	{
		disturb(FALSE, TRUE);
		msg_print(_("周りの空間が歪んでいる気がする！", "You feel the world warping around you!"));

		msg_print(NULL);
		fire_ball(GF_CHAOS, 0, p_ptr->lev, 8);
	}
	if ((p_ptr->muta2 & MUT2_NORMALITY) && one_in_(5000))
	{
		if (!lose_mutation(p_ptr, 0))
			msg_print(_("奇妙なくらい普通になった気がする。", "You feel oddly normal."));
	}
	if ((p_ptr->muta2 & MUT2_WRAITH) && !p_ptr->anti_magic && one_in_(3000))
	{
		disturb(FALSE, TRUE);
		msg_print(_("非物質化した！", "You feel insubstantial!"));

		msg_print(NULL);
		set_wraith_form(p_ptr, randint1(p_ptr->lev / 2) + (p_ptr->lev / 2), FALSE);
	}
	if ((p_ptr->muta2 & MUT2_POLY_WOUND) && one_in_(3000))
	{
		do_poly_wounds(p_ptr);
	}
	if ((p_ptr->muta2 & MUT2_WASTING) && one_in_(3000))
	{
		int which_stat = randint0(A_MAX);
		int sustained = FALSE;

		switch (which_stat)
		{
		case A_STR:
			if (p_ptr->sustain_str) sustained = TRUE;
			break;
		case A_INT:
			if (p_ptr->sustain_int) sustained = TRUE;
			break;
		case A_WIS:
			if (p_ptr->sustain_wis) sustained = TRUE;
			break;
		case A_DEX:
			if (p_ptr->sustain_dex) sustained = TRUE;
			break;
		case A_CON:
			if (p_ptr->sustain_con) sustained = TRUE;
			break;
		case A_CHR:
			if (p_ptr->sustain_chr) sustained = TRUE;
			break;
		default:
			msg_print(_("不正な状態！", "Invalid stat chosen!"));
			sustained = TRUE;
		}

		if (!sustained)
		{
			disturb(FALSE, TRUE);
			msg_print(_("自分が衰弱していくのが分かる！", "You can feel yourself wasting away!"));
			msg_print(NULL);
			(void)dec_stat(p_ptr, which_stat, randint1(6) + 6, one_in_(3));
		}
	}
	if ((p_ptr->muta2 & MUT2_ATT_DRAGON) && !p_ptr->anti_magic && one_in_(3000))
	{
		bool pet = one_in_(5);
		BIT_FLAGS mode = PM_ALLOW_GROUP;

		if (pet) mode |= PM_FORCE_PET;
		else mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);

		if (summon_specific((pet ? -1 : 0), p_ptr->y, p_ptr->x, current_floor_ptr->dun_level, SUMMON_DRAGON, mode))
		{
			msg_print(_("ドラゴンを引き寄せた！", "You have attracted a dragon!"));
			disturb(FALSE, TRUE);
		}
	}
	if ((p_ptr->muta2 & MUT2_WEIRD_MIND) && !p_ptr->anti_magic && one_in_(3000))
	{
		if (p_ptr->tim_esp > 0)
		{
			msg_print(_("精神にもやがかかった！", "Your mind feels cloudy!"));
			set_tim_esp(p_ptr, 0, TRUE);
		}
		else
		{
			msg_print(_("精神が広がった！", "Your mind expands!"));
			set_tim_esp(p_ptr, p_ptr->lev, FALSE);
		}
	}
	if ((p_ptr->muta2 & MUT2_NAUSEA) && !p_ptr->slow_digest && one_in_(9000))
	{
		disturb(FALSE, TRUE);
		msg_print(_("胃が痙攣し、食事を失った！", "Your stomach roils, and you lose your lunch!"));
		msg_print(NULL);
		set_food(p_ptr, PY_FOOD_WEAK);
		if (music_singing_any(p_ptr)) stop_singing(p_ptr);
		if (hex_spelling_any(p_ptr)) stop_hex_spell_all();
	}

	if ((p_ptr->muta2 & MUT2_WALK_SHAD) && !p_ptr->anti_magic && one_in_(12000) && !p_ptr->inside_arena)
	{
		alter_reality();
	}

	if ((p_ptr->muta2 & MUT2_WARNING) && one_in_(1000))
	{
		int danger_amount = 0;
		MONSTER_IDX monster;

		for (monster = 0; monster < current_floor_ptr->m_max; monster++)
		{
			monster_type *m_ptr = &current_floor_ptr->m_list[monster];
			monster_race *r_ptr = &r_info[m_ptr->r_idx];
			if (!monster_is_valid(m_ptr)) continue;

			if (r_ptr->level >= p_ptr->lev)
			{
				danger_amount += r_ptr->level - p_ptr->lev + 1;
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

	if ((p_ptr->muta2 & MUT2_INVULN) && !p_ptr->anti_magic && one_in_(5000))
	{
		disturb(FALSE, TRUE);
		msg_print(_("無敵な気がする！", "You feel invincible!"));
		msg_print(NULL);
		(void)set_invuln(p_ptr, randint1(8) + 8, FALSE);
	}

	if ((p_ptr->muta2 & MUT2_SP_TO_HP) && one_in_(2000))
	{
		MANA_POINT wounds = (MANA_POINT)(p_ptr->mhp - p_ptr->chp);

		if (wounds > 0)
		{
			HIT_POINT healing = p_ptr->csp;
			if (healing > wounds) healing = wounds;

			hp_player(p_ptr, healing);
			p_ptr->csp -= healing;
			p_ptr->redraw |= (PR_HP | PR_MANA);
		}
	}

	if ((p_ptr->muta2 & MUT2_HP_TO_SP) && !p_ptr->anti_magic && one_in_(4000))
	{
		HIT_POINT wounds = (HIT_POINT)(p_ptr->msp - p_ptr->csp);

		if (wounds > 0)
		{
			HIT_POINT healing = p_ptr->chp;
			if (healing > wounds) healing = wounds;

			p_ptr->csp += healing;
			p_ptr->redraw |= (PR_HP | PR_MANA);
			take_hit(p_ptr, DAMAGE_LOSELIFE, healing, _("頭に昇った血", "blood rushing to the head"), -1);
		}
	}

	if ((p_ptr->muta2 & MUT2_DISARM) && one_in_(10000))
	{
		INVENTORY_IDX slot = 0;
		object_type *o_ptr = NULL;

		disturb(FALSE, TRUE);
		msg_print(_("足がもつれて転んだ！", "You trip over your own feet!"));
		take_hit(p_ptr, DAMAGE_NOESCAPE, randint1(p_ptr->wt / 6), _("転倒", "tripping"), -1);

		msg_print(NULL);
		if (has_melee_weapon(p_ptr, INVEN_RARM))
		{
			slot = INVEN_RARM;
			o_ptr = &p_ptr->inventory_list[INVEN_RARM];

			if (has_melee_weapon(p_ptr, INVEN_LARM) && one_in_(2))
			{
				o_ptr = &p_ptr->inventory_list[INVEN_LARM];
				slot = INVEN_LARM;
			}
		}
		else if (has_melee_weapon(p_ptr, INVEN_LARM))
		{
			o_ptr = &p_ptr->inventory_list[INVEN_LARM];
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
static void process_world_aux_curse(void)
{
	if ((p_ptr->cursed & TRC_P_FLAG_MASK) && !p_ptr->phase_out && !p_ptr->wild_mode)
	{
		/*
		 * Hack: Uncursed teleporting items (e.g. Trump Weapons)
		 * can actually be useful!
		 */
		if ((p_ptr->cursed & TRC_TELEPORT_SELF) && one_in_(200))
		{
			GAME_TEXT o_name[MAX_NLEN];
			object_type *o_ptr;
			int i, i_keep = 0, count = 0;

			/* Scan the equipment with random teleport ability */
			for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
			{
				BIT_FLAGS flgs[TR_FLAG_SIZE];
				o_ptr = &p_ptr->inventory_list[i];
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

			o_ptr = &p_ptr->inventory_list[i_keep];
			object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
			msg_format(_("%sがテレポートの能力を発動させようとしている。", "Your %s is activating teleportation."), o_name);
			if (get_check_strict(_("テレポートしますか？", "Teleport? "), CHECK_OKAY_CANCEL))
			{
				disturb(FALSE, TRUE);
				teleport_player(50, 0L);
			}
			else
			{
				msg_format(_("%sに{.}(ピリオド)と銘を刻むと発動を抑制できます。", 
							 "You can inscribe {.} on your %s to disable random teleportation. "), o_name);
				disturb(TRUE, TRUE);
			}
		}
		/* Make a chainsword noise */
		if ((p_ptr->cursed & TRC_CHAINSWORD) && one_in_(CHAINSWORD_NOISE))
		{
			char noise[1024];
			if (!get_rnd_line(_("chainswd_j.txt", "chainswd.txt"), 0, noise))
				msg_print(noise);
			disturb(FALSE, FALSE);
		}
		/* TY Curse */
		if ((p_ptr->cursed & TRC_TY_CURSE) && one_in_(TY_CURSE_CHANCE))
		{
			int count = 0;
			(void)activate_ty_curse(FALSE, &count);
		}
		/* Handle experience draining */
		if (p_ptr->prace != RACE_ANDROID && ((p_ptr->cursed & TRC_DRAIN_EXP) && one_in_(4)))
		{
			p_ptr->exp -= (p_ptr->lev + 1) / 2;
			if (p_ptr->exp < 0) p_ptr->exp = 0;
			p_ptr->max_exp -= (p_ptr->lev + 1) / 2;
			if (p_ptr->max_exp < 0) p_ptr->max_exp = 0;
			check_experience(p_ptr);
		}
		/* Add light curse (Later) */
		if ((p_ptr->cursed & TRC_ADD_L_CURSE) && one_in_(2000))
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

				p_ptr->update |= (PU_BONUS);
			}
		}
		/* Add heavy curse (Later) */
		if ((p_ptr->cursed & TRC_ADD_H_CURSE) && one_in_(2000))
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

				p_ptr->update |= (PU_BONUS);
			}
		}
		/* Call animal */
		if ((p_ptr->cursed & TRC_CALL_ANIMAL) && one_in_(2500))
		{
			if (summon_specific(0, p_ptr->y, p_ptr->x, current_floor_ptr->dun_level, SUMMON_ANIMAL, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)))
			{
				GAME_TEXT o_name[MAX_NLEN];

				object_desc(o_name, choose_cursed_obj_name(TRC_CALL_ANIMAL), (OD_OMIT_PREFIX | OD_NAME_ONLY));
				msg_format(_("%sが動物を引き寄せた！", "Your %s have attracted an animal!"), o_name);
				disturb(FALSE, TRUE);
			}
		}
		/* Call demon */
		if ((p_ptr->cursed & TRC_CALL_DEMON) && one_in_(1111))
		{
			if (summon_specific(0, p_ptr->y, p_ptr->x, current_floor_ptr->dun_level, SUMMON_DEMON, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)))
			{
				GAME_TEXT o_name[MAX_NLEN];

				object_desc(o_name, choose_cursed_obj_name(TRC_CALL_DEMON), (OD_OMIT_PREFIX | OD_NAME_ONLY));
				msg_format(_("%sが悪魔を引き寄せた！", "Your %s have attracted a demon!"), o_name);
				disturb(FALSE, TRUE);
			}
		}
		/* Call dragon */
		if ((p_ptr->cursed & TRC_CALL_DRAGON) && one_in_(800))
		{
			if (summon_specific(0, p_ptr->y, p_ptr->x, current_floor_ptr->dun_level, SUMMON_DRAGON,
			    (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)))
			{
				GAME_TEXT o_name[MAX_NLEN];

				object_desc(o_name, choose_cursed_obj_name(TRC_CALL_DRAGON), (OD_OMIT_PREFIX | OD_NAME_ONLY));
				msg_format(_("%sがドラゴンを引き寄せた！", "Your %s have attracted an dragon!"), o_name);
				disturb(FALSE, TRUE);
			}
		}
		/* Call undead */
		if ((p_ptr->cursed & TRC_CALL_UNDEAD) && one_in_(1111))
		{
			if (summon_specific(0, p_ptr->y, p_ptr->x, current_floor_ptr->dun_level, SUMMON_UNDEAD,
			    (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)))
			{
				GAME_TEXT o_name[MAX_NLEN];

				object_desc(o_name, choose_cursed_obj_name(TRC_CALL_UNDEAD), (OD_OMIT_PREFIX | OD_NAME_ONLY));
				msg_format(_("%sが死霊を引き寄せた！", "Your %s have attracted an undead!"), o_name);
				disturb(FALSE, TRUE);
			}
		}
		if ((p_ptr->cursed & TRC_COWARDICE) && one_in_(1500))
		{
			if (!p_ptr->resist_fear)
			{
				disturb(FALSE, TRUE);
				msg_print(_("とても暗い... とても恐い！", "It's so dark... so scary!"));
				set_afraid(p_ptr, p_ptr->afraid + 13 + randint1(26));
			}
		}
		/* Teleport player */
		if ((p_ptr->cursed & TRC_TELEPORT) && one_in_(200) && !p_ptr->anti_tele)
		{
			disturb(FALSE, TRUE);

			/* Teleport player */
			teleport_player(40, TELEPORT_PASSIVE);
		}
		/* Handle HP draining */
		if ((p_ptr->cursed & TRC_DRAIN_HP) && one_in_(666))
		{
			GAME_TEXT o_name[MAX_NLEN];

			object_desc(o_name, choose_cursed_obj_name(TRC_DRAIN_HP), (OD_OMIT_PREFIX | OD_NAME_ONLY));
			msg_format(_("%sはあなたの体力を吸収した！", "Your %s drains HP from you!"), o_name);
			take_hit(p_ptr, DAMAGE_LOSELIFE, MIN(p_ptr->lev*2, 100), o_name, -1);
		}
		/* Handle mana draining */
		if ((p_ptr->cursed & TRC_DRAIN_MANA) && p_ptr->csp && one_in_(666))
		{
			GAME_TEXT o_name[MAX_NLEN];

			object_desc(o_name, choose_cursed_obj_name(TRC_DRAIN_MANA), (OD_OMIT_PREFIX | OD_NAME_ONLY));
			msg_format(_("%sはあなたの魔力を吸収した！", "Your %s drains mana from you!"), o_name);
			p_ptr->csp -= MIN(p_ptr->lev, 50);
			if (p_ptr->csp < 0)
			{
				p_ptr->csp = 0;
				p_ptr->csp_frac = 0;
			}
			p_ptr->redraw |= PR_MANA;
		}
	}

	/* Rarely, take damage from the Jewel of Judgement */
	if (one_in_(999) && !p_ptr->anti_magic)
	{
		object_type *o_ptr = &p_ptr->inventory_list[INVEN_LITE];

		if (o_ptr->name1 == ART_JUDGE)
		{
			if (object_is_known(o_ptr))
				msg_print(_("『審判の宝石』はあなたの体力を吸収した！", "The Jewel of Judgement drains life from you!"));
			else
				msg_print(_("なにかがあなたの体力を吸収した！", "Something drains life from you!"));
			take_hit(p_ptr, DAMAGE_LOSELIFE, MIN(p_ptr->lev, 50), _("審判の宝石", "the Jewel of Judgement"), -1);
		}
	}
}


/*!
 * @brief 10ゲームターンが進行するごとに魔道具の自然充填を行う処理
 * / Handle recharging objects once every 10 game turns
 * @return なし
 */
static void process_world_aux_recharge(void)
{
	int i;
	bool changed;

	/* Process equipment */
	for (changed = FALSE, i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		/* Get the object */
		object_type *o_ptr = &p_ptr->inventory_list[i];
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
		p_ptr->window |= (PW_EQUIP);
		wild_regen = 20;
	}

	/*
	 * Recharge rods.  Rods now use timeout to control charging status,
	 * and each charging rod in a stack decreases the stack's timeout by
	 * one per current_world_ptr->game_turn. -LM-
	 */
	for (changed = FALSE, i = 0; i < INVEN_PACK; i++)
	{
		object_type *o_ptr = &p_ptr->inventory_list[i];
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
		p_ptr->window |= (PW_INVEN);
		wild_regen = 20;
	}

	/* Process objects on floor */
	for (i = 1; i < current_floor_ptr->o_max; i++)
	{
		object_type *o_ptr = &current_floor_ptr->o_list[i];

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
static void process_world_aux_movement(void)
{
	/* Delayed Word-of-Recall */
	if (p_ptr->word_recall)
	{
		/*
		 * HACK: Autosave BEFORE resetting the recall counter (rr9)
		 * The player is yanked up/down as soon as
		 * he loads the autosaved game.
		 */
		if (autosave_l && (p_ptr->word_recall == 1) && !p_ptr->phase_out)
			do_cmd_save_game(TRUE);

		/* Count down towards recall */
		p_ptr->word_recall--;

		p_ptr->redraw |= (PR_STATUS);

		/* Activate the recall */
		if (!p_ptr->word_recall)
		{
			/* Disturbing! */
			disturb(FALSE, TRUE);

			/* Determine the level */
			if (current_floor_ptr->dun_level || p_ptr->inside_quest || p_ptr->enter_dungeon)
			{
				msg_print(_("上に引っ張りあげられる感じがする！", "You feel yourself yanked upwards!"));

				if (p_ptr->dungeon_idx) p_ptr->recall_dungeon = p_ptr->dungeon_idx;
				if (record_stair)
					do_cmd_write_nikki(NIKKI_RECALL, current_floor_ptr->dun_level, NULL);

				current_floor_ptr->dun_level = 0;
				p_ptr->dungeon_idx = 0;

				leave_quest_check();
				leave_tower_check();

				p_ptr->inside_quest = 0;

				p_ptr->leaving = TRUE;
			}
			else
			{
				msg_print(_("下に引きずり降ろされる感じがする！", "You feel yourself yanked downwards!"));

				p_ptr->dungeon_idx = p_ptr->recall_dungeon;

				if (record_stair)
					do_cmd_write_nikki(NIKKI_RECALL, current_floor_ptr->dun_level, NULL);

				/* New depth */
				current_floor_ptr->dun_level = max_dlv[p_ptr->dungeon_idx];
				if (current_floor_ptr->dun_level < 1) current_floor_ptr->dun_level = 1;

				/* Nightmare mode makes recall more dangerous */
				if (ironman_nightmare && !randint0(666) && (p_ptr->dungeon_idx == DUNGEON_ANGBAND))
				{
					if (current_floor_ptr->dun_level < 50)
					{
						current_floor_ptr->dun_level *= 2;
					}
					else if (current_floor_ptr->dun_level < 99)
					{
						current_floor_ptr->dun_level = (current_floor_ptr->dun_level + 99) / 2;
					}
					else if (current_floor_ptr->dun_level > 100)
					{
						current_floor_ptr->dun_level = d_info[p_ptr->dungeon_idx].maxdepth - 1;
					}
				}

				if (p_ptr->wild_mode)
				{
					p_ptr->wilderness_y = p_ptr->y;
					p_ptr->wilderness_x = p_ptr->x;
				}
				else
				{
					/* Save player position */
					p_ptr->oldpx = p_ptr->x;
					p_ptr->oldpy = p_ptr->y;
				}
				p_ptr->wild_mode = FALSE;

				/*
				 * Clear all saved floors
				 * and create a first saved floor
				 */
				prepare_change_floor_mode(CFM_FIRST_FLOOR);
				p_ptr->leaving = TRUE;

				if (p_ptr->dungeon_idx == DUNGEON_ANGBAND)
				{
					int i;

					for (i = MIN_RANDOM_QUEST; i < MAX_RANDOM_QUEST + 1; i++)
					{
						quest_type* const q_ptr = &quest[i];

						
						if ((q_ptr->type == QUEST_TYPE_RANDOM) &&
						    (q_ptr->status == QUEST_STATUS_TAKEN) &&
						    (q_ptr->level < current_floor_ptr->dun_level))
						{
							q_ptr->status = QUEST_STATUS_FAILED;
							q_ptr->complev = (byte)p_ptr->lev;
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
	if (p_ptr->alter_reality)
	{
		if (autosave_l && (p_ptr->alter_reality == 1) && !p_ptr->phase_out)
			do_cmd_save_game(TRUE);

		/* Count down towards alter */
		p_ptr->alter_reality--;

		p_ptr->redraw |= (PR_STATUS);

		/* Activate the alter reality */
		if (!p_ptr->alter_reality)
		{
			/* Disturbing! */
			disturb(FALSE, TRUE);

			/* Determine the level */
			if (!quest_number(current_floor_ptr->dun_level) && current_floor_ptr->dun_level)
			{
				msg_print(_("世界が変わった！", "The world changes!"));

				/*
				 * Clear all saved floors
				 * and create a first saved floor
				 */
				prepare_change_floor_mode(CFM_FIRST_FLOOR);
				p_ptr->leaving = TRUE;
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
static void process_world(void)
{
	int day, hour, min;

	const s32b A_DAY = TURNS_PER_TICK * TOWN_DAWN;
	s32b prev_turn_in_today = ((current_world_ptr->game_turn - TURNS_PER_TICK) % A_DAY + A_DAY / 4) % A_DAY;
	int prev_min = (1440 * prev_turn_in_today / A_DAY) % 60;
	
	extract_day_hour_min(&day, &hour, &min);

	/* Update dungeon feeling, and announce it if changed */
	update_dungeon_feeling();

	/* 帰還無しモード時のレベルテレポバグ対策 / Fix for level teleport bugs on ironman_downward.*/
	if (ironman_downward && (p_ptr->dungeon_idx != DUNGEON_ANGBAND && p_ptr->dungeon_idx != 0))
	{
		current_floor_ptr->dun_level = 0;
		p_ptr->dungeon_idx = 0;
		prepare_change_floor_mode(CFM_FIRST_FLOOR | CFM_RAND_PLACE);
		p_ptr->inside_arena = FALSE;
		p_ptr->wild_mode = FALSE;
		p_ptr->leaving = TRUE;
	}

	/*** Check monster arena ***/
	if (p_ptr->phase_out && !p_ptr->leaving)
	{
		int i2, j2;
		int win_m_idx = 0;
		int number_mon = 0;

		/* Count all hostile monsters */
		for (i2 = 0; i2 < current_floor_ptr->width; ++i2)
			for (j2 = 0; j2 < current_floor_ptr->height; j2++)
			{
				grid_type *g_ptr = &current_floor_ptr->grid_array[j2][i2];

				if ((g_ptr->m_idx > 0) && (g_ptr->m_idx != p_ptr->riding))
				{
					number_mon++;
					win_m_idx = g_ptr->m_idx;
				}
			}

		if (number_mon == 0)
		{
			msg_print(_("相打ちに終わりました。", "They have kill each other at the same time."));
			msg_print(NULL);
			p_ptr->energy_need = 0;
			update_gambling_monsters();
		}
		else if ((number_mon-1) == 0)
		{
			GAME_TEXT m_name[MAX_NLEN];
			monster_type *wm_ptr;

			wm_ptr = &current_floor_ptr->m_list[win_m_idx];

			monster_desc(m_name, wm_ptr, 0);
			msg_format(_("%sが勝利した！", "%s is winner!"), m_name);
			msg_print(NULL);

			if (win_m_idx == (sel_monster+1))
			{
				msg_print(_("おめでとうございます。", "Congratulations."));
				msg_format(_("%d＄を受け取った。", "You received %d gold."), battle_odds);
				p_ptr->au += battle_odds;
			}
			else
			{
				msg_print(_("残念でした。", "You lost gold."));
			}
			msg_print(NULL);
			p_ptr->energy_need = 0;
			update_gambling_monsters();
		}
		else if (current_world_ptr->game_turn - current_floor_ptr->generated_turn == 150 * TURNS_PER_TICK)
		{
			msg_print(_("申し分けありませんが、この勝負は引き分けとさせていただきます。", "This battle have ended in a draw."));
			p_ptr->au += kakekin;
			msg_print(NULL);
			p_ptr->energy_need = 0;
			update_gambling_monsters();
		}
	}

	/* Every 10 game turns */
	if (current_world_ptr->game_turn % TURNS_PER_TICK) return;

	/*** Attempt timed autosave ***/
	if (autosave_t && autosave_freq && !p_ptr->phase_out)
	{
		if (!(current_world_ptr->game_turn % ((s32b)autosave_freq * TURNS_PER_TICK)))
			do_cmd_save_game(TRUE);
	}

	if (current_floor_ptr->monster_noise && !ignore_unview)
	{
		msg_print(_("何かが聞こえた。", "You hear noise."));
	}

	/*** Handle the wilderness/town (sunshine) ***/

	/* While in town/wilderness */
	if (!current_floor_ptr->dun_level && !p_ptr->inside_quest && !p_ptr->phase_out && !p_ptr->inside_arena)
	{
		/* Hack -- Daybreak/Nighfall in town */
		if (!(current_world_ptr->game_turn % ((TURNS_PER_TICK * TOWN_DAWN) / 2)))
		{
			bool dawn;

			/* Check for dawn */
			dawn = (!(current_world_ptr->game_turn % (TURNS_PER_TICK * TOWN_DAWN)));

			if (dawn) day_break();
			else night_falls();

		}
	}

	/* While in the dungeon (vanilla_town or lite_town mode only) */
	else if ((vanilla_town || (lite_town && !p_ptr->inside_quest && !p_ptr->phase_out && !p_ptr->inside_arena)) && current_floor_ptr->dun_level)
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
	if (one_in_(d_info[p_ptr->dungeon_idx].max_m_alloc_chance) &&
	    !p_ptr->inside_arena && !p_ptr->inside_quest && !p_ptr->phase_out)
	{
		/* Make a new monster */
		(void)alloc_monster(MAX_SIGHT + 5, 0);
	}

	/* Hack -- Check for creature regeneration */
	if (!(current_world_ptr->game_turn % (TURNS_PER_TICK * 10)) && !p_ptr->phase_out) regen_monsters();
	if (!(current_world_ptr->game_turn % (TURNS_PER_TICK * 3))) regen_captured_monsters();

	if (!p_ptr->leaving)
	{
		int i;

		/* Hack -- Process the counters of monsters if needed */
		for (i = 0; i < MAX_MTIMED; i++)
		{
			if (current_floor_ptr->mproc_max[i] > 0) process_monsters_mtimed(i);
		}
	}


	/* Date changes */
	if (!hour && !min)
	{
		if (min != prev_min)
		{
			do_cmd_write_nikki(NIKKI_HIGAWARI, 0, NULL);
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
			disturb(FALSE, TRUE);

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

			disturb(TRUE, TRUE);
			msg_print(_("遠くで鐘が何回も鳴り、死んだような静けさの中へ消えていった。", "A distant bell tolls many times, fading into an deathly silence."));

			if (p_ptr->wild_mode)
			{
				/* Go into large wilderness view */
				p_ptr->oldpy = randint1(MAX_HGT - 2);
				p_ptr->oldpx = randint1(MAX_WID - 2);
				change_wild_mode(TRUE);

				/* Give first move to monsters */
				take_turn(p_ptr, 100);

			}

			p_ptr->invoking_midnight_curse = TRUE;
		}
	}

	process_world_aux_digestion();
	process_world_aux_hp_and_sp();
	process_world_aux_timeout();
	process_world_aux_light();
	process_world_aux_mutation();
	process_world_aux_curse();
	process_world_aux_recharge();
	sense_inventory1();
	sense_inventory2();
	process_world_aux_movement();
}

/*!
 * @brief ウィザードモードへの導入処理
 * / Verify use of "wizard" mode
 * @return 実際にウィザードモードへ移行したらTRUEを返す。
 */
static bool enter_wizard_mode(void)
{
	/* Ask first time */
	if (!p_ptr->noscore)
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

		do_cmd_write_nikki(NIKKI_BUNSHOU, 0, _("ウィザードモードに突入してスコアを残せなくなった。", "give up recording score to enter wizard mode."));
		/* Mark savefile */
		p_ptr->noscore |= 0x0002;
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
	if (!p_ptr->noscore)
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

		do_cmd_write_nikki(NIKKI_BUNSHOU, 0, _("デバッグモードに突入してスコアを残せなくなった。", "give up sending score to use debug commands."));
		/* Mark savefile */
		p_ptr->noscore |= 0x0008;
	}

	/* Success */
	return (TRUE);
}

/*
 * Hack -- Declare the Debug Routines
 */
extern void do_cmd_debug(void);

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
	if (!(p_ptr->noscore & 0x0010))
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

		do_cmd_write_nikki(NIKKI_BUNSHOU, 0, _("ボーグ・コマンドを使用してスコアを残せなくなった。", "give up recording score to use borg commands."));
		/* Mark savefile */
		p_ptr->noscore |= 0x0010;
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
static void process_command(void)
{
	COMMAND_CODE old_now_message = now_message;

	/* Handle repeating the last command */
	repeat_check();

	now_message = 0;

	/* Sniper */
	if ((p_ptr->pclass == CLASS_SNIPER) && (p_ptr->concent))
		p_ptr->reset_concent = TRUE;

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
			if (p_ptr->wizard)
			{
				p_ptr->wizard = FALSE;
				msg_print(_("ウィザードモード解除。", "Wizard mode off."));
			}
			else if (enter_wizard_mode())
			{
				p_ptr->wizard = TRUE;
				msg_print(_("ウィザードモード突入。", "Wizard mode on."));
			}
			p_ptr->update |= (PU_MONSTERS);
			p_ptr->redraw |= (PR_TITLE);

			break;
		}


#ifdef ALLOW_WIZARD

		/* Special "debug" commands */
		case KTRL('A'):
		{
			if (enter_debug_mode())
			{
				do_cmd_debug();
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
				if (!p_ptr->wild_mode) do_cmd_borg();
			}
			break;
		}

#endif /* ALLOW_BORG */



		/*** Inventory Commands ***/

		/* Wear/wield equipment */
		case 'w':
		{
			if (!p_ptr->wild_mode) do_cmd_wield();
			break;
		}

		/* Take off equipment */
		case 't':
		{
			if (!p_ptr->wild_mode) do_cmd_takeoff();
			break;
		}

		/* Drop an item */
		case 'd':
		{
			if (!p_ptr->wild_mode) do_cmd_drop();
			break;
		}

		/* Destroy an item */
		case 'k':
		{
			do_cmd_destroy();
			break;
		}

		/* Equipment list */
		case 'e':
		{
			do_cmd_equip();
			break;
		}

		/* Inventory list */
		case 'i':
		{
			do_cmd_inven();
			break;
		}


		/*** Various commands ***/

		/* Identify an object */
		case 'I':
		{
			do_cmd_observe();
			break;
		}

		case KTRL('I'):
		{
			toggle_inven_equip(p_ptr);
			break;
		}


		/*** Standard "Movement" Commands ***/

		/* Alter a grid */
		case '+':
		{
			if (!p_ptr->wild_mode) do_cmd_alter();
			break;
		}

		/* Dig a tunnel */
		case 'T':
		{
			if (!p_ptr->wild_mode) do_cmd_tunnel(p_ptr);
			break;
		}

		/* Move (usually pick up things) */
		case ';':
		{
			do_cmd_walk(FALSE);
			break;
		}

		/* Move (usually do not pick up) */
		case '-':
		{
			do_cmd_walk(TRUE);
			break;
		}


		/*** Running, Resting, Searching, Staying */

		/* Begin Running -- Arg is Max Distance */
		case '.':
		{
			if (!p_ptr->wild_mode) do_cmd_run();
			break;
		}

		/* Stay still (usually pick things up) */
		case ',':
		{
			do_cmd_stay(always_pickup);
			break;
		}

		/* Stay still (usually do not pick up) */
		case 'g':
		{
			do_cmd_stay(!always_pickup);
			break;
		}

		/* Rest -- Arg is time */
		case 'R':
		{
			do_cmd_rest();
			break;
		}

		/* Search for traps/doors */
		case 's':
		{
			do_cmd_search(p_ptr);
			break;
		}

		case 'S':
		{
			if (p_ptr->action == ACTION_SEARCH) set_action(p_ptr, ACTION_NONE);
			else set_action(p_ptr, ACTION_SEARCH);
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
			do_cmd_bldg();
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
			if (!p_ptr->wild_mode && !current_floor_ptr->dun_level && !p_ptr->inside_arena && !p_ptr->inside_quest)
			{
				if (vanilla_town) break;

				if (p_ptr->ambush_flag)
				{
					msg_print(_("襲撃から逃げるにはマップの端まで移動しなければならない。", "To flee the ambush you have to reach the edge of the map."));
					break;
				}

				if (p_ptr->food < PY_FOOD_WEAK)
				{
					msg_print(_("その前に食事をとらないと。", "You must eat something here."));
					break;
				}

				change_wild_mode(FALSE);
			}
			else
				do_cmd_go_up(p_ptr);
			break;
		}

		/* Go down staircase */
		case '>':
		{
			if (p_ptr->wild_mode)
				change_wild_mode(FALSE);
			else
				do_cmd_go_down(p_ptr);
			break;
		}

		/* Open a door or chest */
		case 'o':
		{
			do_cmd_open(p_ptr);
			break;
		}

		/* Close a door */
		case 'c':
		{
			do_cmd_close(p_ptr);
			break;
		}

		/* Jam a door with spikes */
		case 'j':
		{
			do_cmd_spike(p_ptr);
			break;
		}

		/* Bash a door */
		case 'B':
		{
			do_cmd_bash();
			break;
		}

		/* Disarm a trap or chest */
		case 'D':
		{
			do_cmd_disarm();
			break;
		}


		/*** Magic and Prayers ***/

		/* Gain new spells/prayers */
		case 'G':
		{
			if ((p_ptr->pclass == CLASS_SORCERER) || (p_ptr->pclass == CLASS_RED_MAGE))
				msg_print(_("呪文を学習する必要はない！", "You don't have to learn spells!"));
			else if (p_ptr->pclass == CLASS_SAMURAI)
				do_cmd_gain_hissatsu();
			else if (p_ptr->pclass == CLASS_MAGIC_EATER)
				import_magic_device();
			else
				do_cmd_study();
			break;
		}

		/* Browse a book */
		case 'b':
		{
			if ( (p_ptr->pclass == CLASS_MINDCRAFTER) ||
			     (p_ptr->pclass == CLASS_BERSERKER) ||
			     (p_ptr->pclass == CLASS_NINJA) ||
			     (p_ptr->pclass == CLASS_MIRROR_MASTER) 
			     ) do_cmd_mind_browse();
			else if (p_ptr->pclass == CLASS_SMITH)
				do_cmd_kaji(TRUE);
			else if (p_ptr->pclass == CLASS_MAGIC_EATER)
				do_cmd_magic_eater(TRUE, FALSE);
			else if (p_ptr->pclass == CLASS_SNIPER)
				do_cmd_snipe_browse();
			else do_cmd_browse();
			break;
		}

		/* Cast a spell */
		case 'm':
		{
			/* -KMW- */
			if (!p_ptr->wild_mode)
			{
				if ((p_ptr->pclass == CLASS_WARRIOR) || (p_ptr->pclass == CLASS_ARCHER) || (p_ptr->pclass == CLASS_CAVALRY))
				{
					msg_print(_("呪文を唱えられない！", "You cannot cast spells!"));
				}
				else if (current_floor_ptr->dun_level && (d_info[p_ptr->dungeon_idx].flags1 & DF1_NO_MAGIC) && (p_ptr->pclass != CLASS_BERSERKER) && (p_ptr->pclass != CLASS_SMITH))
				{
					msg_print(_("ダンジョンが魔法を吸収した！", "The dungeon absorbs all attempted magic!"));
					msg_print(NULL);
				}
				else if (p_ptr->anti_magic && (p_ptr->pclass != CLASS_BERSERKER) && (p_ptr->pclass != CLASS_SMITH))
				{
					concptr which_power = _("魔法", "magic");
					if (p_ptr->pclass == CLASS_MINDCRAFTER)
						which_power = _("超能力", "psionic powers");
					else if (p_ptr->pclass == CLASS_IMITATOR)
						which_power = _("ものまね", "imitation");
					else if (p_ptr->pclass == CLASS_SAMURAI)
						which_power = _("必殺剣", "hissatsu");
					else if (p_ptr->pclass == CLASS_MIRROR_MASTER)
						which_power = _("鏡魔法", "mirror magic");
					else if (p_ptr->pclass == CLASS_NINJA)
						which_power = _("忍術", "ninjutsu");
					else if (mp_ptr->spell_book == TV_LIFE_BOOK)
						which_power = _("祈り", "prayer");

					msg_format(_("反魔法バリアが%sを邪魔した！", "An anti-magic shell disrupts your %s!"), which_power);
					free_turn(p_ptr);
				}
				else if (p_ptr->shero && (p_ptr->pclass != CLASS_BERSERKER))
				{
					msg_format(_("狂戦士化していて頭が回らない！", "You cannot think directly!"));
					free_turn(p_ptr);
				}
				else
				{
					if ((p_ptr->pclass == CLASS_MINDCRAFTER) ||
					    (p_ptr->pclass == CLASS_BERSERKER) ||
					    (p_ptr->pclass == CLASS_NINJA) ||
					    (p_ptr->pclass == CLASS_MIRROR_MASTER)
					    )
						do_cmd_mind();
					else if (p_ptr->pclass == CLASS_IMITATOR)
						do_cmd_mane(FALSE);
					else if (p_ptr->pclass == CLASS_MAGIC_EATER)
						do_cmd_magic_eater(FALSE, FALSE);
					else if (p_ptr->pclass == CLASS_SAMURAI)
						do_cmd_hissatsu();
					else if (p_ptr->pclass == CLASS_BLUE_MAGE)
						do_cmd_cast_learned();
					else if (p_ptr->pclass == CLASS_SMITH)
						do_cmd_kaji(FALSE);
					else if (p_ptr->pclass == CLASS_SNIPER)
						do_cmd_snipe();
					else
						do_cmd_cast();
				}
			}
			break;
		}

		/* Issue a pet command */
		case 'p':
		{
			do_cmd_pet();
			break;
		}

		/*** Use various objects ***/

		/* Inscribe an object */
		case '{':
		{
			do_cmd_inscribe();
			break;
		}

		/* Uninscribe an object */
		case '}':
		{
			do_cmd_uninscribe();
			break;
		}

		/* Activate an artifact */
		case 'A':
		{
			do_cmd_activate(p_ptr);
			break;
		}

		/* Eat some food */
		case 'E':
		{
			do_cmd_eat_food();
			break;
		}

		/* Fuel your lantern/torch */
		case 'F':
		{
			do_cmd_refill();
			break;
		}

		/* Fire an item */
		case 'f':
		{
			do_cmd_fire(SP_NONE);
			break;
		}

		/* Throw an item */
		case 'v':
		{
			do_cmd_throw(1, FALSE, -1);
			break;
		}

		/* Aim a wand */
		case 'a':
		{
			do_cmd_aim_wand();
			break;
		}

		/* Zap a rod */
		case 'z':
		{
			if (use_command && rogue_like_commands)
			{
				do_cmd_use();
			}
			else
			{
				do_cmd_zap_rod();
			}
			break;
		}

		/* Quaff a potion */
		case 'q':
		{
			do_cmd_quaff_potion();
			break;
		}

		/* Read a scroll */
		case 'r':
		{
			do_cmd_read_scroll();
			break;
		}

		/* Use a staff */
		case 'u':
		{
			if (use_command && !rogue_like_commands)
				do_cmd_use();
			else
				do_cmd_use_staff();
			break;
		}

		/* Use racial power */
		case 'U':
		{
			do_cmd_racial_power();
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
			do_cmd_locate();
			break;
		}

		/* Look around */
		case 'l':
		{
			do_cmd_look();
			break;
		}

		/* Target monster or location */
		case '*':
		{
			do_cmd_target();
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
			do_cmd_player_status();
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
			do_cmd_macros();
			break;
		}

		/* Interact with visuals */
		case '%':
		{
			do_cmd_visuals();
			do_cmd_redraw();
			break;
		}

		/* Interact with colors */
		case '&':
		{
			do_cmd_colors();
			do_cmd_redraw();
			break;
		}

		/* Interact with options */
		case '=':
		{
			do_cmd_options();
			(void)combine_and_reorder_home(STORE_HOME);
			do_cmd_redraw();
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
			do_cmd_feeling();
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
			do_cmd_redraw();
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
			do_cmd_suicide();
			break;
		}

		case '|':
		{
			do_cmd_nikki();
			break;
		}

		/* Check artifacts, uniques, objects */
		case '~':
		{
			do_cmd_knowledge();
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
			spoil_random_artifact("randifact.txt");
			break;
		}

#ifdef TRAVEL
		case '`':
		{
			if (!p_ptr->wild_mode) do_cmd_travel();
			if (p_ptr->special_defense & KATA_MUSOU)
			{
				set_action(p_ptr, ACTION_NONE);
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
	if (!p_ptr->energy_use && !now_message)
		now_message = old_now_message;
}

/*!
 * @brief アイテムの所持種類数が超えた場合にアイテムを床に落とす処理 / Hack -- Pack Overflow
 * @return なし
 */
static void pack_overflow(void)
{
	if (p_ptr->inventory_list[INVEN_PACK].k_idx)
	{
		GAME_TEXT o_name[MAX_NLEN];
		object_type *o_ptr;

		/* Is auto-destroy done? */
		update_creature(p_ptr);
		if (!p_ptr->inventory_list[INVEN_PACK].k_idx) return;

		/* Access the slot to be dropped */
		o_ptr = &p_ptr->inventory_list[INVEN_PACK];

		disturb(FALSE, TRUE);

		/* Warning */
		msg_print(_("ザックからアイテムがあふれた！", "Your pack overflows!"));
		object_desc(o_name, o_ptr, 0);

		msg_format(_("%s(%c)を落とした。", "You drop %s (%c)."), o_name, index_to_label(INVEN_PACK));

		/* Drop it (carefully) near the player */
		(void)drop_near(o_ptr, 0, p_ptr->y, p_ptr->x);

		/* Modify, Describe, Optimize */
		inven_item_increase(INVEN_PACK, -255);
		inven_item_describe(INVEN_PACK);
		inven_item_optimize(INVEN_PACK);

		handle_stuff();
	}
}

/*!
 * @brief プレイヤーの行動エネルギーが充填される（＝プレイヤーのターンが回る）毎に行われる処理  / process the effects per 100 energy at player speed.
 * @return なし
 */
static void process_upkeep_with_speed(void)
{
	/* Give the player some energy */
	if (!load && p_ptr->enchant_energy_need > 0 && !p_ptr->leaving)
	{
		p_ptr->enchant_energy_need -= SPEED_TO_ENERGY(p_ptr->pspeed);
	}
	
	/* No current_world_ptr->game_turn yet */
	if (p_ptr->enchant_energy_need > 0) return;
	
	while (p_ptr->enchant_energy_need <= 0)
	{
		/* Handle the player song */
		if (!load) check_music();

		/* Hex - Handle the hex spells */
		if (!load) check_hex();
		if (!load) revenge_spell();
		
		/* There is some randomness of needed energy */
		p_ptr->enchant_energy_need += ENERGY_NEED();
	}
}

static void process_fishing(void)
{
	Term_xtra(TERM_XTRA_DELAY, 10);
	if (one_in_(1000))
	{
		MONRACE_IDX r_idx;
		bool success = FALSE;
		get_mon_num_prep(monster_is_fishing_target, NULL);
		r_idx = get_mon_num(current_floor_ptr->dun_level ? current_floor_ptr->dun_level : wilderness[p_ptr->wilderness_y][p_ptr->wilderness_x].level);
		msg_print(NULL);
		if (r_idx && one_in_(2))
		{
			POSITION y, x;
			y = p_ptr->y + ddy[p_ptr->fishing_dir];
			x = p_ptr->x + ddx[p_ptr->fishing_dir];
			if (place_monster_aux(0, y, x, r_idx, PM_NO_KAGE))
			{
				GAME_TEXT m_name[MAX_NLEN];
				monster_desc(m_name, &current_floor_ptr->m_list[current_floor_ptr->grid_array[y][x].m_idx], 0);
				msg_format(_("%sが釣れた！", "You have a good catch!"), m_name);
				success = TRUE;
			}
		}
		if (!success)
		{
			msg_print(_("餌だけ食われてしまった！くっそ～！", "Damn!  The fish stole your bait!"));
		}
		disturb(FALSE, TRUE);
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
static void process_player(void)
{
	MONSTER_IDX m_idx;

	/*** Apply energy ***/

	if (p_ptr->hack_mutation)
	{
		msg_print(_("何か変わった気がする！", "You feel different!"));

		(void)gain_mutation(p_ptr, 0);
		p_ptr->hack_mutation = FALSE;
	}

	if (p_ptr->invoking_midnight_curse)
	{
		int count = 0;
		activate_ty_curse(FALSE, &count);
		p_ptr->invoking_midnight_curse = FALSE;
	}

	if (p_ptr->phase_out)
	{
		for(m_idx = 1; m_idx < current_floor_ptr->m_max; m_idx++)
		{
			monster_type *m_ptr = &current_floor_ptr->m_list[m_idx];

			if (!monster_is_valid(m_ptr)) continue;

			m_ptr->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
			update_monster(m_idx, FALSE);
		}
		prt_time();
	}

	/* Give the player some energy */
	else if (!(load && p_ptr->energy_need <= 0))
	{
		p_ptr->energy_need -= SPEED_TO_ENERGY(p_ptr->pspeed);
	}

	/* No current_world_ptr->game_turn yet */
	if (p_ptr->energy_need > 0) return;
	if (!command_rep) prt_time();

	/*** Check for interupts ***/

	/* Complete resting */
	if (p_ptr->resting < 0)
	{
		/* Basic resting */
		if (p_ptr->resting == COMMAND_ARG_REST_FULL_HEALING)
		{
			/* Stop resting */
			if ((p_ptr->chp == p_ptr->mhp) &&
			    (p_ptr->csp >= p_ptr->msp))
			{
				set_action(p_ptr, ACTION_NONE);
			}
		}

		/* Complete resting */
		else if (p_ptr->resting == COMMAND_ARG_REST_UNTIL_DONE)
		{
			/* Stop resting */
			if ((p_ptr->chp == p_ptr->mhp) &&
			    (p_ptr->csp >= p_ptr->msp) &&
			    !p_ptr->blind && !p_ptr->confused &&
			    !p_ptr->poisoned && !p_ptr->afraid &&
			    !p_ptr->stun && !p_ptr->cut &&
			    !p_ptr->slow && !p_ptr->paralyzed &&
			    !p_ptr->image && !p_ptr->word_recall &&
			    !p_ptr->alter_reality)
			{
				set_action(p_ptr, ACTION_NONE);
			}
		}
	}

	if (p_ptr->action == ACTION_FISH) process_fishing();

	/* Handle "abort" */
	if (check_abort)
	{
		/* Check for "player abort" (semi-efficiently for resting) */
		if (p_ptr->running || travel.run || command_rep || (p_ptr->action == ACTION_REST) || (p_ptr->action == ACTION_FISH))
		{
			/* Do not wait */
			inkey_scan = TRUE;

			/* Check for a key */
			if (inkey())
			{
				flush(); /* Flush input */

				disturb(FALSE, TRUE);

				/* Hack -- Show a Message */
				msg_print(_("中断しました。", "Canceled."));
			}
		}
	}

	if (p_ptr->riding && !p_ptr->confused && !p_ptr->blind)
	{
		monster_type *m_ptr = &current_floor_ptr->m_list[p_ptr->riding];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		if (MON_CSLEEP(m_ptr))
		{
			GAME_TEXT m_name[MAX_NLEN];

			/* Recover fully */
			(void)set_monster_csleep(p_ptr->riding, 0);
			monster_desc(m_name, m_ptr, 0);
			msg_format(_("%^sを起こした。", "You have waked %s up."), m_name);
		}

		if (MON_STUNNED(m_ptr))
		{
			/* Hack -- Recover from stun */
			if (set_monster_stunned(p_ptr->riding,
				(randint0(r_ptr->level) < p_ptr->skill_exp[GINOU_RIDING]) ? 0 : (MON_STUNNED(m_ptr) - 1)))
			{
				GAME_TEXT m_name[MAX_NLEN];
				monster_desc(m_name, m_ptr, 0);
				msg_format(_("%^sを朦朧状態から立ち直らせた。", "%^s is no longer stunned."), m_name);
			}
		}

		if (MON_CONFUSED(m_ptr))
		{
			/* Hack -- Recover from confusion */
			if (set_monster_confused(p_ptr->riding,
				(randint0(r_ptr->level) < p_ptr->skill_exp[GINOU_RIDING]) ? 0 : (MON_CONFUSED(m_ptr) - 1)))
			{
				GAME_TEXT m_name[MAX_NLEN];
				monster_desc(m_name, m_ptr, 0);
				msg_format(_("%^sを混乱状態から立ち直らせた。", "%^s is no longer confused."), m_name);
			}
		}

		if (MON_MONFEAR(m_ptr))
		{
			/* Hack -- Recover from fear */
			if(set_monster_monfear(p_ptr->riding,
				(randint0(r_ptr->level) < p_ptr->skill_exp[GINOU_RIDING]) ? 0 : (MON_MONFEAR(m_ptr) - 1)))
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
	if (p_ptr->lightspeed)
	{
		(void)set_lightspeed(p_ptr, p_ptr->lightspeed - 1, TRUE);
	}
	if ((p_ptr->pclass == CLASS_FORCETRAINER) && P_PTR_KI)
	{
		if(P_PTR_KI < 40) P_PTR_KI = 0;
		else P_PTR_KI -= 40;
		p_ptr->update |= (PU_BONUS);
	}
	if (p_ptr->action == ACTION_LEARN)
	{
		s32b cost = 0L;
		u32b cost_frac = (p_ptr->msp + 30L) * 256L;

		/* Convert the unit (1/2^16) to (1/2^32) */
		s64b_LSHIFT(cost, cost_frac, 16);
 
		if (s64b_cmp(p_ptr->csp, p_ptr->csp_frac, cost, cost_frac) < 0)
		{
			/* Mana run out */
			p_ptr->csp = 0;
			p_ptr->csp_frac = 0;
			set_action(p_ptr, ACTION_NONE);
		}
		else
		{
			/* Reduce mana */
			s64b_sub(&(p_ptr->csp), &(p_ptr->csp_frac), cost, cost_frac);
		}
		p_ptr->redraw |= PR_MANA;
	}

	if (p_ptr->special_defense & KATA_MASK)
	{
		if (p_ptr->special_defense & KATA_MUSOU)
		{
			if (p_ptr->csp < 3)
			{
				set_action(p_ptr, ACTION_NONE);
			}
			else
			{
				p_ptr->csp -= 2;
				p_ptr->redraw |= (PR_MANA);
			}
		}
	}

	/*** Handle actual user input ***/

	/* Repeat until out of energy */
	while (p_ptr->energy_need <= 0)
	{
		p_ptr->window |= PW_PLAYER;
		p_ptr->sutemi = FALSE;
		p_ptr->counter = FALSE;
		p_ptr->now_damaged = FALSE;

		handle_stuff();

		/* Place the cursor on the player */
		move_cursor_relative(p_ptr->y, p_ptr->x);

		/* Refresh (optional) */
		if (fresh_before) Term_fresh();

		/* Hack -- Pack Overflow */
		pack_overflow();

		/* Hack -- cancel "lurking browse mode" */
		if (!command_new) command_see = FALSE;

		/* Assume free current_world_ptr->game_turn */
		free_turn(p_ptr);

		if (p_ptr->phase_out)
		{
			/* Place the cursor on the player */
			move_cursor_relative(p_ptr->y, p_ptr->x);

			command_cmd = SPECIAL_KEY_BUILDING;

			/* Process the command */
			process_command();
		}

		/* Paralyzed or Knocked Out */
		else if (p_ptr->paralyzed || (p_ptr->stun >= 100))
		{
			take_turn(p_ptr, 100);
		}

		/* Resting */
		else if (p_ptr->action == ACTION_REST)
		{
			/* Timed rest */
			if (p_ptr->resting > 0)
			{
				/* Reduce rest count */
				p_ptr->resting--;

				if (!p_ptr->resting) set_action(p_ptr, ACTION_NONE);
				p_ptr->redraw |= (PR_STATE);
			}

			take_turn(p_ptr, 100);
		}

		/* Fishing */
		else if (p_ptr->action == ACTION_FISH)
		{
			take_turn(p_ptr, 100);
		}

		/* Running */
		else if (p_ptr->running)
		{
			/* Take a step */
			run_step(0);
		}

#ifdef TRAVEL
		/* Traveling */
		else if (travel.run)
		{
			/* Take a step */
			travel_step();
		}
#endif

		/* Repeated command */
		else if (command_rep)
		{
			/* Count this execution */
			command_rep--;

			p_ptr->redraw |= (PR_STATE);
			handle_stuff();

			/* Hack -- Assume messages were seen */
			msg_flag = FALSE;

			/* Clear the top line */
			prt("", 0, 0);

			/* Process the command */
			process_command();
		}

		/* Normal command */
		else
		{
			/* Place the cursor on the player */
			move_cursor_relative(p_ptr->y, p_ptr->x);

			can_save = TRUE;
			/* Get a command (normal) */
			request_command(FALSE);
			can_save = FALSE;

			/* Process the command */
			process_command();
		}

		/* Hack -- Pack Overflow */
		pack_overflow();

		/*** Clean up ***/

		/* Significant */
		if (p_ptr->energy_use)
		{
			/* Use some energy */
			if (p_ptr->timewalk || p_ptr->energy_use > 400)
			{
				/* The Randomness is irrelevant */
				p_ptr->energy_need += p_ptr->energy_use * TURNS_PER_TICK / 10;
			}
			else
			{
				/* There is some randomness of needed energy */
				p_ptr->energy_need += (s16b)((s32b)p_ptr->energy_use * ENERGY_NEED() / 100L);
			}

			/* Hack -- constant hallucination */
			if (p_ptr->image) p_ptr->redraw |= (PR_MAP);

			/* Shimmer multi-hued monsters */
			for (m_idx = 1; m_idx < current_floor_ptr->m_max; m_idx++)
			{
				monster_type *m_ptr;
				monster_race *r_ptr;

				m_ptr = &current_floor_ptr->m_list[m_idx];
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
				for (m_idx = 1; m_idx < current_floor_ptr->m_max; m_idx++)
				{
					monster_type *m_ptr;
					m_ptr = &current_floor_ptr->m_list[m_idx];
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
							update_monster(m_idx, FALSE);

							if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
							if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);

							/* Redraw regardless */
							lite_spot(m_ptr->fy, m_ptr->fx);
						}
					}
				}
			}
			if (p_ptr->pclass == CLASS_IMITATOR)
			{
				int j;
				if (p_ptr->mane_num > (p_ptr->lev > 44 ? 3 : p_ptr->lev > 29 ? 2 : 1))
				{
					p_ptr->mane_num--;
					for (j = 0; j < p_ptr->mane_num; j++)
					{
						p_ptr->mane_spell[j] = p_ptr->mane_spell[j + 1];
						p_ptr->mane_dam[j] = p_ptr->mane_dam[j + 1];
					}
				}
				p_ptr->new_mane = FALSE;
				p_ptr->redraw |= (PR_IMITATION);
			}
			if (p_ptr->action == ACTION_LEARN)
			{
				p_ptr->new_mane = FALSE;
				p_ptr->redraw |= (PR_STATE);
			}

			if (p_ptr->timewalk && (p_ptr->energy_need > - 1000))
			{

				p_ptr->redraw |= (PR_MAP);
				p_ptr->update |= (PU_MONSTERS);
				p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

				msg_print(_("「時は動きだす…」", "You feel time flowing around you once more."));
				msg_print(NULL);
				p_ptr->timewalk = FALSE;
				p_ptr->energy_need = ENERGY_NEED();

				handle_stuff();
			}
		}

		/* Hack -- notice death */
		if (!p_ptr->playing || p_ptr->is_dead)
		{
			p_ptr->timewalk = FALSE;
			break;
		}

		/* Sniper */
		if (p_ptr->energy_use && p_ptr->reset_concent) reset_concentration(TRUE);

		/* Handle "leaving" */
		if (p_ptr->leaving) break;
	}

	/* Update scent trail */
	update_smell();
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
static void dungeon(bool load_game)
{
	int quest_num = 0;

	/* Set the base level */
	current_floor_ptr->base_level = current_floor_ptr->dun_level;

	/* Reset various flags */
	current_world_ptr->is_loading_now = FALSE;

	/* Not leaving */
	p_ptr->leaving = FALSE;

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
	p_ptr->pet_t_m_idx = 0;
	p_ptr->riding_t_m_idx = 0;
	p_ptr->ambush_flag = FALSE;

	/* Cancel the health bar */
	health_track(0);

	/* Check visual effects */
	repair_monsters = TRUE;
	repair_objects = TRUE;


	disturb(TRUE, TRUE);

	/* Get index of current quest (if any) */
	quest_num = quest_number(current_floor_ptr->dun_level);

	/* Inside a quest? */
	if (quest_num)
	{
		/* Mark the quest monster */
		r_info[quest[quest_num].r_idx].flags1 |= RF1_QUESTOR;
	}

	/* Track maximum player level */
	if (p_ptr->max_plv < p_ptr->lev)
	{
		p_ptr->max_plv = p_ptr->lev;
	}


	/* Track maximum dungeon level (if not in quest -KMW-) */
	if ((max_dlv[p_ptr->dungeon_idx] < current_floor_ptr->dun_level) && !p_ptr->inside_quest)
	{
		max_dlv[p_ptr->dungeon_idx] = current_floor_ptr->dun_level;
		if (record_maxdepth) do_cmd_write_nikki(NIKKI_MAXDEAPTH, current_floor_ptr->dun_level, NULL);
	}

	(void)calculate_upkeep();

	/* Validate the panel */
	panel_bounds_center();

	/* Verify the panel */
	verify_panel();

	msg_erase();


	/* Enter "xtra" mode */
	current_world_ptr->character_xtra = TRUE;

	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER | PW_MONSTER | PW_OVERHEAD | PW_DUNGEON);
	p_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_EQUIPPY | PR_MAP);
	p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS | PU_VIEW | PU_LITE | PU_MON_LITE | PU_TORCH | PU_MONSTERS | PU_DISTANCE | PU_FLOW);

	handle_stuff();

	/* Leave "xtra" mode */
	current_world_ptr->character_xtra = FALSE;

	p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
	p_ptr->update |= (PU_COMBINE | PU_REORDER);
	handle_stuff();
	Term_fresh();

	if (quest_num && (is_fixed_quest_idx(quest_num) &&
	    !((quest_num == QUEST_OBERON) || (quest_num == QUEST_SERPENT) ||
	    !(quest[quest_num].flags & QUEST_FLAG_PRESET)))) do_cmd_feeling();

	if (p_ptr->phase_out)
	{
		if (load_game)
		{
			p_ptr->energy_need = 0;
			update_gambling_monsters();
		}
		else
		{
			msg_print(_("試合開始！", "Ready..Fight!"));
			msg_print(NULL);
		}
	}

	if ((p_ptr->pclass == CLASS_BARD) && (SINGING_SONG_EFFECT(p_ptr) > MUSIC_DETECT))
		SINGING_SONG_EFFECT(p_ptr) = MUSIC_DETECT;

	/* Hack -- notice death or departure */
	if (!p_ptr->playing || p_ptr->is_dead) return;

	/* Print quest message if appropriate */
	if (!p_ptr->inside_quest && (p_ptr->dungeon_idx == DUNGEON_ANGBAND))
	{
		quest_discovery(random_quest_number(current_floor_ptr->dun_level));
		p_ptr->inside_quest = random_quest_number(current_floor_ptr->dun_level);
	}
	if ((current_floor_ptr->dun_level == d_info[p_ptr->dungeon_idx].maxdepth) && d_info[p_ptr->dungeon_idx].final_guardian)
	{
		if (r_info[d_info[p_ptr->dungeon_idx].final_guardian].max_num)
#ifdef JP
			msg_format("この階には%sの主である%sが棲んでいる。",
				   d_name+d_info[p_ptr->dungeon_idx].name, 
				   r_name+r_info[d_info[p_ptr->dungeon_idx].final_guardian].name);
#else
			msg_format("%^s lives in this level as the keeper of %s.",
					   r_name+r_info[d_info[p_ptr->dungeon_idx].final_guardian].name, 
					   d_name+d_info[p_ptr->dungeon_idx].name);
#endif
	}

	if (!load_game && (p_ptr->special_defense & NINJA_S_STEALTH)) set_superstealth(p_ptr, FALSE);

	/*** Process this dungeon level ***/

	/* Reset the monster generation level */
	current_floor_ptr->monster_level = current_floor_ptr->base_level;

	/* Reset the object generation level */
	current_floor_ptr->object_level = current_floor_ptr->base_level;

	current_world_ptr->is_loading_now = TRUE;

	if (p_ptr->energy_need > 0 && !p_ptr->phase_out &&
	    (current_floor_ptr->dun_level || p_ptr->leaving_dungeon || p_ptr->inside_arena))
		p_ptr->energy_need = 0;

	/* Not leaving dungeon */
	p_ptr->leaving_dungeon = FALSE;

	/* Initialize monster process */
	mproc_init();

	/* Main loop */
	while (TRUE)
	{
		/* Hack -- Compact the monster list occasionally */
		if ((current_floor_ptr->m_cnt + 32 > current_floor_ptr->max_m_idx) && !p_ptr->phase_out) compact_monsters(64);

		/* Hack -- Compress the monster list occasionally */
		if ((current_floor_ptr->m_cnt + 32 < current_floor_ptr->m_max) && !p_ptr->phase_out) compact_monsters(0);


		/* Hack -- Compact the object list occasionally */
		if (current_floor_ptr->o_cnt + 32 > current_floor_ptr->max_o_idx) compact_objects(64);

		/* Hack -- Compress the object list occasionally */
		if (current_floor_ptr->o_cnt + 32 < current_floor_ptr->o_max) compact_objects(0);

		/* Process the player */
		process_player();
		process_upkeep_with_speed();

		handle_stuff();

		/* Hack -- Hilite the player */
		move_cursor_relative(p_ptr->y, p_ptr->x);

		/* Optional fresh */
		if (fresh_after) Term_fresh();

		/* Hack -- Notice death or departure */
		if (!p_ptr->playing || p_ptr->is_dead) break;

		/* Process all of the monsters */
		process_monsters();

		handle_stuff();

		/* Hack -- Hilite the player */
		move_cursor_relative(p_ptr->y, p_ptr->x);

		/* Optional fresh */
		if (fresh_after) Term_fresh();

		/* Hack -- Notice death or departure */
		if (!p_ptr->playing || p_ptr->is_dead) break;

		/* Process the world */
		process_world();

		handle_stuff();

		/* Hack -- Hilite the player */
		move_cursor_relative(p_ptr->y, p_ptr->x);

		/* Optional fresh */
		if (fresh_after) Term_fresh();

		/* Hack -- Notice death or departure */
		if (!p_ptr->playing || p_ptr->is_dead) break;

		/* Count game turns */
		current_world_ptr->game_turn++;

		if (current_world_ptr->dungeon_turn < current_world_ptr->dungeon_turn_limit)
		{
			if (!p_ptr->wild_mode || wild_regen) current_world_ptr->dungeon_turn++;
			else if (p_ptr->wild_mode && !(current_world_ptr->game_turn % ((MAX_HGT + MAX_WID) / 2))) current_world_ptr->dungeon_turn++;
		}

		prevent_turn_overflow();

		/* Handle "leaving" */
		if (p_ptr->leaving) break;

		if (wild_regen) wild_regen--;
	}

	/* Inside a quest and non-unique questor? */
	if (quest_num && !(r_info[quest[quest_num].r_idx].flags1 & RF1_UNIQUE))
	{
		/* Un-mark the quest monster */
		r_info[quest[quest_num].r_idx].flags1 &= ~RF1_QUESTOR;
	}

	/* Not save-and-quit and not dead? */
	if (p_ptr->playing && !p_ptr->is_dead)
	{
		/*
		 * Maintain Unique monsters and artifact, save current
		 * floor, then prepare next floor
		 */
		leave_floor(p_ptr->change_floor_mode);

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
void play_game(bool new_game)
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

	p_ptr->hack_mutation = FALSE;

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

	/* Hack -- current_world_ptr->game_turn off the cursor */
	(void)Term_set_cursor(0);


	/* Attempt to load */
	if (!load_player())
	{
		quit(_("セーブファイルが壊れています", "broken savefile"));
	}

	/* Extract the options */
	extract_option_vars();

	/* Report waited score */
	if (p_ptr->wait_report_score)
	{
		char buf[1024];
		bool success;

		if (!get_check_strict(_("待機していたスコア登録を今行ないますか？", "Do you register score now? "), CHECK_NO_HISTORY))
			quit(0);

		p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
		update_creature(p_ptr);

		p_ptr->is_dead = TRUE;

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
			p_ptr->wait_report_score = FALSE;
			top_twenty();
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
		current_floor_ptr->dun_level = 0;
		p_ptr->inside_quest = 0;
		p_ptr->inside_arena = FALSE;
		p_ptr->phase_out = FALSE;

		write_level = TRUE;

		/* Hack -- seed for flavors */
		current_world_ptr->seed_flavor = randint0(0x10000000);

		/* Hack -- seed for town layout */
		current_world_ptr->seed_town = randint0(0x10000000);

		/* Roll up a new character */
		player_birth(p_ptr);

		counts_write(2,0);
		p_ptr->count = 0;

		load = FALSE;

		determine_bounty_uniques();
		determine_today_mon(FALSE);

		/* Initialize object array */
		wipe_o_list();
	}
	else
	{
		write_level = FALSE;

		do_cmd_write_nikki(NIKKI_GAMESTART, 1, 
					  _("                            ----ゲーム再開----",
						"                            ---- Restart Game ----"));

/*
 * 1.0.9 以前はセーブ前に p_ptr->riding = -1 としていたので、再設定が必要だった。
 * もう不要だが、以前のセーブファイルとの互換のために残しておく。
 */
		if (p_ptr->riding == -1)
		{
			p_ptr->riding = 0;
			for (i = current_floor_ptr->m_max; i > 0; i--)
			{
				if (player_bold(current_floor_ptr->m_list[i].fy, current_floor_ptr->m_list[i].fx))
				{
					p_ptr->riding = i;
					break;
				}
			}
		}
	}

	current_world_ptr->creating_savefile = FALSE;

	p_ptr->teleport_town = FALSE;
	p_ptr->sutemi = FALSE;
	current_world_ptr->timewalk_m_idx = 0;
	p_ptr->now_damaged = FALSE;
	now_message = 0;
	current_world_ptr->start_time = time(NULL) - 1;
	record_o_name[0] = '\0';

	/* Reset map panel */
	panel_row_min = current_floor_ptr->height;
	panel_col_min = current_floor_ptr->width;

	/* Sexy gal gets bonus to maximum weapon skill of whip */
	if (p_ptr->pseikaku == SEIKAKU_SEXY)
		s_info[p_ptr->pclass].w_max[TV_HAFTED-TV_WEAPON_BEGIN][SV_WHIP] = WEAPON_EXP_MASTER;

	/* Fill the arrays of floors and walls in the good proportions */
	set_floor_and_wall(p_ptr->dungeon_idx);

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
			p_ptr->wizard = TRUE;

			if (p_ptr->is_dead || !p_ptr->y || !p_ptr->x)
			{
				/* Initialize the saved floors data */
				init_saved_floors(TRUE);

				/* Avoid crash */
				p_ptr->inside_quest = 0;

				/* Avoid crash in update_view() */
				p_ptr->y = p_ptr->x = 10;
			}
		}
		else if (p_ptr->is_dead)
		{
			quit("Already dead.");
		}
	}

	/* Initialize the town-buildings if necessary */
	if (!current_floor_ptr->dun_level && !p_ptr->inside_quest)
	{
		process_dungeon_file("w_info.txt", 0, 0, current_world_ptr->max_wild_y, current_world_ptr->max_wild_x);
		init_flags = INIT_ONLY_BUILDINGS;
		process_dungeon_file("t_info.txt", 0, 0, MAX_HGT, MAX_WID);
		select_floor_music();
	}

	/* Generate a dungeon level if needed */
	if (!current_world_ptr->character_dungeon)
	{
		change_floor(p_ptr->change_floor_mode);
	}
	else
	{
		/* HACK -- Restore from panic-save */
		if (p_ptr->panic_save)
		{
			/* No player?  -- Try to regenerate floor */
			if (!p_ptr->y || !p_ptr->x)
			{
				msg_print(_("プレイヤーの位置がおかしい。フロアを再生成します。", "What a strange player location.  Regenerate the dungeon floor."));
				change_floor(p_ptr->change_floor_mode);
			}

			/* Still no player?  -- Try to locate random place */
			if (!p_ptr->y || !p_ptr->x) p_ptr->y = p_ptr->x = 10;

			/* No longer in panic */
			p_ptr->panic_save = 0;
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
		do_cmd_write_nikki(NIKKI_BUNSHOU, 0, buf);
	}


	/* Start game */
	p_ptr->playing = TRUE;

	/* Reset the visual mappings */
	reset_visuals();

	/* Load the "pref" files */
	load_all_pref_files();

	/* Give startup outfit (after loading pref files) */
	if (new_game)
	{
		player_outfit(p_ptr);
	}

	/* React to changes */
	Term_xtra(TERM_XTRA_REACT, 0);

	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER);
	p_ptr->window |= (PW_MESSAGE | PW_OVERHEAD | PW_DUNGEON | PW_MONSTER | PW_OBJECT);
	handle_stuff();

	/* Set or clear "rogue_like_commands" if requested */
	if (arg_force_original) rogue_like_commands = FALSE;
	if (arg_force_roguelike) rogue_like_commands = TRUE;

	/* Hack -- Enforce "delayed death" */
	if (p_ptr->chp < 0) p_ptr->is_dead = TRUE;

	if (p_ptr->prace == RACE_ANDROID) calc_android_exp(p_ptr);

	if (new_game && ((p_ptr->pclass == CLASS_CAVALRY) || (p_ptr->pclass == CLASS_BEASTMASTER)))
	{
		monster_type *m_ptr;
		MONRACE_IDX pet_r_idx = ((p_ptr->pclass == CLASS_CAVALRY) ? MON_HORSE : MON_YASE_HORSE);
		monster_race *r_ptr = &r_info[pet_r_idx];
		place_monster_aux(0, p_ptr->y, p_ptr->x - 1, pet_r_idx,
				  (PM_FORCE_PET | PM_NO_KAGE));
		m_ptr = &current_floor_ptr->m_list[hack_m_idx_ii];
		m_ptr->mspeed = r_ptr->speed;
		m_ptr->maxhp = r_ptr->hdice*(r_ptr->hside+1)/2;
		m_ptr->max_maxhp = m_ptr->maxhp;
		m_ptr->hp = r_ptr->hdice*(r_ptr->hside+1)/2;
		m_ptr->dealt_damage = 0;
		m_ptr->energy_need = ENERGY_NEED() + ENERGY_NEED();
	}

	(void)combine_and_reorder_home(STORE_HOME);
	(void)combine_and_reorder_home(STORE_MUSEUM);

	select_floor_music();

	/* Process */
	while (TRUE)
	{
		/* Process the level */
		dungeon(load_game);

		/* Hack -- prevent "icky" message */
		current_world_ptr->character_xtra = TRUE;

		handle_stuff();

		current_world_ptr->character_xtra = FALSE;

		/* Cancel the target */
		target_who = 0;

		/* Cancel the health bar */
		health_track(0);

		forget_lite();
		forget_view();
		clear_mon_lite();

		/* Handle "quit and save" */
		if (!p_ptr->playing && !p_ptr->is_dead) break;

		/* Erase the old current_floor_ptr->grid_array */
		wipe_o_list();
		if (!p_ptr->is_dead) wipe_m_list();


		msg_print(NULL);

		load_game = FALSE;

		/* Accidental Death */
		if (p_ptr->playing && p_ptr->is_dead)
		{
			if (p_ptr->inside_arena)
			{
				p_ptr->inside_arena = FALSE;
				if (p_ptr->arena_number > MAX_ARENA_MONS)
					p_ptr->arena_number++;
				else
					p_ptr->arena_number = -1 - p_ptr->arena_number;
				p_ptr->is_dead = FALSE;
				p_ptr->chp = 0;
				p_ptr->chp_frac = 0;
				p_ptr->exit_bldg = TRUE;
				reset_tim_flags(p_ptr);

				/* Leave through the exit */
				prepare_change_floor_mode(CFM_SAVE_FLOORS | CFM_RAND_CONNECT);

				/* prepare next floor */
				leave_floor(p_ptr->change_floor_mode);
			}
			else
			{
				/* Mega-Hack -- Allow player to cheat death */
				if ((p_ptr->wizard || cheat_live) && !get_check(_("死にますか? ", "Die? ")))
				{
					cheat_death(p_ptr);
				}
			}
		}

		/* Handle "death" */
		if (p_ptr->is_dead) break;

		/* Make a new level */
		change_floor(p_ptr->change_floor_mode);
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
	if (current_floor_ptr->generated_turn > rollback_turns) current_floor_ptr->generated_turn -= rollback_turns;
	else current_floor_ptr->generated_turn = 1;
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
		if (p_ptr->total_winner) kingly();

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
				(void)top_twenty();
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
		if (inkey() != ESCAPE) predict_score();
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
	if (p_ptr->redraw) redraw_stuff();
	if (p_ptr->window) window_stuff();
}

void update_output(void)
{
	if (p_ptr->redraw) redraw_stuff();
	if (p_ptr->window) window_stuff();
}

