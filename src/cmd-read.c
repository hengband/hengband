/*!
 * @file cmd-read.c
 * @brief プレイヤーの読むコマンド実装
 * @date 2018/09/07
 * @details
 * cmd6.cより分離。
 */

#include "angband.h"
#include "object-flavor.h"
#include "object-hook.h"
#include "artifact.h"
#include "avatar.h"
#include "player-status.h"
#include "player-effects.h"
#include "rumor.h"
#include "realm-hex.h"

#include "spells.h"
#include "spells-object.h"
#include "spells-floor.h"
#include "spells-summon.h"
#include "spells-status.h"

#include "cmd-basic.h"
#include "files.h"

/*!
 * @brief 巻物を読むコマンドのサブルーチン
 * Read a scroll (from the pack or floor).
 * @param item 読むオブジェクトの所持品ID
 * @param known 判明済ならばTRUE
 * @return なし
 * @details
 * <pre>
 * Certain scrolls can be "aborted" without losing the scroll.  These
 * include scrolls with no effects but recharge or identify, which are
 * cancelled before use.  XXX Reading them still takes a current_world_ptr->game_turn, though.
 * </pre>
 */
void do_cmd_read_scroll_aux(INVENTORY_IDX item, bool known)
{
	int         k, used_up, ident, lev;
	object_type *o_ptr;


	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &current_floor_ptr->o_list[0 - item];
	}

	take_turn(p_ptr, 100);
	if (cmd_limit_time_walk(p_ptr)) return;

	if (p_ptr->pclass == CLASS_BERSERKER)
	{
		msg_print(_("巻物なんて読めない。", "You cannot read."));
		return;
	}

	if (music_singing_any()) stop_singing(p_ptr);

	/* Hex */
	if (hex_spelling_any() && ((p_ptr->lev < 35) || hex_spell_fully())) stop_hex_spell_all();

	/* Not identified yet */
	ident = FALSE;

	/* Object level */
	lev = k_info[o_ptr->k_idx].level;

	/* Assume the scroll will get used up */
	used_up = TRUE;

	if (o_ptr->tval == TV_SCROLL)
	{
	/* Analyze the scroll */
	switch (o_ptr->sval)
	{
		case SV_SCROLL_DARKNESS:
		{
			if (!(p_ptr->resist_blind) && !(p_ptr->resist_dark))
			{
				(void)set_blind(p_ptr->blind + 3 + randint1(5));
			}
			if (unlite_area(10, 3)) ident = TRUE;
			break;
		}

		case SV_SCROLL_AGGRAVATE_MONSTER:
		{
			msg_print(_("カン高くうなる様な音が辺りを覆った。", "There is a high pitched humming noise."));
			aggravate_monsters(0);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_CURSE_ARMOR:
		{
			if (curse_armor()) ident = TRUE;
			break;
		}

		case SV_SCROLL_CURSE_WEAPON:
		{
			k = 0;
			if (has_melee_weapon(INVEN_RARM))
			{
				k = INVEN_RARM;
				if (has_melee_weapon(INVEN_LARM) && one_in_(2)) k = INVEN_LARM;
			}
			else if (has_melee_weapon(INVEN_LARM)) k = INVEN_LARM;
			if (k && curse_weapon(FALSE, k)) ident = TRUE;
			break;
		}

		case SV_SCROLL_SUMMON_MONSTER:
		{
			for (k = 0; k < randint1(3); k++)
			{
				if (summon_specific(0, p_ptr->y, p_ptr->x, current_floor_ptr->dun_level, 0, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET), '\0'))
				{
					ident = TRUE;
				}
			}
			break;
		}

		case SV_SCROLL_SUMMON_UNDEAD:
		{
			for (k = 0; k < randint1(3); k++)
			{
				if (summon_specific(0, p_ptr->y, p_ptr->x, current_floor_ptr->dun_level, SUMMON_UNDEAD, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET), '\0'))
				{
					ident = TRUE;
				}
			}
			break;
		}

		case SV_SCROLL_SUMMON_PET:
		{
			if (summon_specific(-1, p_ptr->y, p_ptr->x, current_floor_ptr->dun_level, 0, (PM_ALLOW_GROUP | PM_FORCE_PET), '\0'))
			{
				ident = TRUE;
			}
			break;
		}

		case SV_SCROLL_SUMMON_KIN:
		{
			if (summon_kin_player(p_ptr->lev, p_ptr->y, p_ptr->x, (PM_FORCE_PET | PM_ALLOW_GROUP)))
			{
				ident = TRUE;
			}
			break;
		}

		case SV_SCROLL_TRAP_CREATION:
		{
			if (trap_creation(p_ptr->y, p_ptr->x)) ident = TRUE;
			break;
		}

		case SV_SCROLL_PHASE_DOOR:
		{
			teleport_player(10, 0L);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_TELEPORT:
		{
			teleport_player(100, 0L);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_TELEPORT_LEVEL:
		{
			(void)teleport_level(0);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_WORD_OF_RECALL:
		{
			if (!recall_player(p_ptr, randint0(21) + 15)) used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_IDENTIFY:
		{
			if (!ident_spell(FALSE)) used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_STAR_IDENTIFY:
		{
			if (!identify_fully(FALSE)) used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_REMOVE_CURSE:
		{
			if (remove_curse())
			{
				ident = TRUE;
			}
			break;
		}

		case SV_SCROLL_STAR_REMOVE_CURSE:
		{
			if (remove_all_curse())
			{
				ident = TRUE;
			}
			break;
		}

		case SV_SCROLL_ENCHANT_ARMOR:
		{
			ident = TRUE;
			if (!enchant_spell(0, 0, 1)) used_up = FALSE;
			break;
		}

		case SV_SCROLL_ENCHANT_WEAPON_TO_HIT:
		{
			if (!enchant_spell(1, 0, 0)) used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_ENCHANT_WEAPON_TO_DAM:
		{
			if (!enchant_spell(0, 1, 0)) used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_STAR_ENCHANT_ARMOR:
		{
			if (!enchant_spell(0, 0, randint1(3) + 2)) used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_STAR_ENCHANT_WEAPON:
		{
			if (!enchant_spell(randint1(3), randint1(3), 0)) used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_RECHARGING:
		{
			if (!recharge(130)) used_up = FALSE;
			ident = TRUE;
			break;
		}

		case SV_SCROLL_MUNDANITY:
		{
			ident = TRUE;
			if (!mundane_spell(FALSE)) used_up = FALSE;
			break;
		}

		case SV_SCROLL_LIGHT:
		{
			if (lite_area(damroll(2, 8), 2)) ident = TRUE;
			break;
		}

		case SV_SCROLL_MAPPING:
		{
			map_area(DETECT_RAD_MAP);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_DETECT_GOLD:
		{
			if (detect_treasure(DETECT_RAD_DEFAULT)) ident = TRUE;
			if (detect_objects_gold(DETECT_RAD_DEFAULT)) ident = TRUE;
			break;
		}

		case SV_SCROLL_DETECT_ITEM:
		{
			if (detect_objects_normal(DETECT_RAD_DEFAULT)) ident = TRUE;
			break;
		}

		case SV_SCROLL_DETECT_TRAP:
		{
			if (detect_traps(DETECT_RAD_DEFAULT, known)) ident = TRUE;
			break;
		}

		case SV_SCROLL_DETECT_DOOR:
		{
			if (detect_doors(DETECT_RAD_DEFAULT)) ident = TRUE;
			if (detect_stairs(DETECT_RAD_DEFAULT)) ident = TRUE;
			break;
		}

		case SV_SCROLL_DETECT_INVIS:
		{
			if (detect_monsters_invis(DETECT_RAD_DEFAULT)) ident = TRUE;
			break;
		}

		case SV_SCROLL_SATISFY_HUNGER:
		{
			if (set_food(PY_FOOD_MAX - 1)) ident = TRUE;
			break;
		}

		case SV_SCROLL_BLESSING:
		{
			if (set_blessed(p_ptr->blessed + randint1(12) + 6, FALSE)) ident = TRUE;
			break;
		}

		case SV_SCROLL_HOLY_CHANT:
		{
			if (set_blessed(p_ptr->blessed + randint1(24) + 12, FALSE)) ident = TRUE;
			break;
		}

		case SV_SCROLL_HOLY_PRAYER:
		{
			if (set_blessed(p_ptr->blessed + randint1(48) + 24, FALSE)) ident = TRUE;
			break;
		}

		case SV_SCROLL_MONSTER_CONFUSION:
		{
			if (!(p_ptr->special_attack & ATTACK_CONFUSE))
			{
				msg_print(_("手が輝き始めた。", "Your hands begin to glow."));
				p_ptr->special_attack |= ATTACK_CONFUSE;
				p_ptr->redraw |= (PR_STATUS);
				ident = TRUE;
			}
			break;
		}

		case SV_SCROLL_PROTECTION_FROM_EVIL:
		{
			k = 3 * p_ptr->lev;
			if (set_protevil(p_ptr->protevil + randint1(25) + k, FALSE)) ident = TRUE;
			break;
		}

		case SV_SCROLL_RUNE_OF_PROTECTION:
		{
			warding_glyph();
			ident = TRUE;
			break;
		}

		case SV_SCROLL_TRAP_DOOR_DESTRUCTION:
		{
			if (destroy_doors_touch()) ident = TRUE;
			break;
		}

		case SV_SCROLL_STAR_DESTRUCTION:
		{
			if (destroy_area(p_ptr->y, p_ptr->x, 13 + randint0(5), FALSE))
				ident = TRUE;
			else
				msg_print(_("ダンジョンが揺れた...", "The dungeon trembles..."));

			break;
		}

		case SV_SCROLL_DISPEL_UNDEAD:
		{
			if (dispel_undead(80)) ident = TRUE;
			break;
		}

		case SV_SCROLL_SPELL:
		{
			if ((p_ptr->pclass == CLASS_WARRIOR) ||
				(p_ptr->pclass == CLASS_IMITATOR) ||
				(p_ptr->pclass == CLASS_MINDCRAFTER) ||
				(p_ptr->pclass == CLASS_SORCERER) ||
				(p_ptr->pclass == CLASS_ARCHER) ||
				(p_ptr->pclass == CLASS_MAGIC_EATER) ||
				(p_ptr->pclass == CLASS_RED_MAGE) ||
				(p_ptr->pclass == CLASS_SAMURAI) ||
				(p_ptr->pclass == CLASS_BLUE_MAGE) ||
				(p_ptr->pclass == CLASS_CAVALRY) ||
				(p_ptr->pclass == CLASS_BERSERKER) ||
				(p_ptr->pclass == CLASS_SMITH) ||
				(p_ptr->pclass == CLASS_MIRROR_MASTER) ||
				(p_ptr->pclass == CLASS_NINJA) ||
				(p_ptr->pclass == CLASS_SNIPER)) break;
			p_ptr->add_spells++;
			p_ptr->update |= (PU_SPELLS);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_GENOCIDE:
		{
			(void)symbol_genocide(300, TRUE);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_MASS_GENOCIDE:
		{
			(void)mass_genocide(300, TRUE);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_ACQUIREMENT:
		{
			acquirement(p_ptr->y, p_ptr->x, 1, TRUE, FALSE, FALSE);
			ident = TRUE;
			break;
		}

		case SV_SCROLL_STAR_ACQUIREMENT:
		{
			acquirement(p_ptr->y, p_ptr->x, randint1(2) + 1, TRUE, FALSE, FALSE);
			ident = TRUE;
			break;
		}

		/* New Hengband scrolls */
		case SV_SCROLL_FIRE:
		{
			fire_ball(GF_FIRE, 0, 666, 4);
			/* Note: "Double" damage since it is centered on the player ... */
			if (!(IS_OPPOSE_FIRE() || p_ptr->resist_fire || p_ptr->immune_fire))
				take_hit(DAMAGE_NOESCAPE, 50+randint1(50), _("炎の巻物", "a Scroll of Fire"), -1);

			ident = TRUE;
			break;
		}


		case SV_SCROLL_ICE:
		{
			fire_ball(GF_ICE, 0, 777, 4);
			if (!(IS_OPPOSE_COLD() || p_ptr->resist_cold || p_ptr->immune_cold))
				take_hit(DAMAGE_NOESCAPE, 100+randint1(100), _("氷の巻物", "a Scroll of Ice"), -1);

			ident = TRUE;
			break;
		}

		case SV_SCROLL_CHAOS:
		{
			fire_ball(GF_CHAOS, 0, 1000, 4);
			if (!p_ptr->resist_chaos)
				take_hit(DAMAGE_NOESCAPE, 111+randint1(111), _("ログルスの巻物", "a Scroll of Logrus"), -1);

			ident = TRUE;
			break;
		}

		case SV_SCROLL_RUMOR:
		{
			msg_print(_("巻物にはメッセージが書かれている:", "There is message on the scroll. It says:"));
			msg_print(NULL);
			display_rumor(TRUE);
			msg_print(NULL);
			msg_print(_("巻物は煙を立てて消え去った！", "The scroll disappears in a puff of smoke!"));

			ident = TRUE;
			break;
		}

		case SV_SCROLL_ARTIFACT:
		{
			ident = TRUE;
			if (!artifact_scroll()) used_up = FALSE;
			break;
		}

		case SV_SCROLL_RESET_RECALL:
		{
			ident = TRUE;
			if (!reset_recall()) used_up = FALSE;
			break;
		}

		case SV_SCROLL_AMUSEMENT:
		{
			ident = TRUE;
			amusement(p_ptr->y, p_ptr->x, 1, FALSE);
			break;
		}

		case SV_SCROLL_STAR_AMUSEMENT:
		{
			ident = TRUE;
			amusement(p_ptr->y, p_ptr->x,  randint1(2) + 1, FALSE);
			break;
		}
	}
	}
	else if (o_ptr->name1 == ART_GHB)
	{
		msg_print(_("私は苦労して『グレーター・ヘル=ビースト』を倒した。", "I had a very hard time to kill the Greater hell-beast, "));
		msg_print(_("しかし手に入ったのはこのきたないＴシャツだけだった。", "but all I got was this lousy t-shirt!"));
		used_up = FALSE;
	}
	else if (o_ptr->name1 == ART_POWER)
	{
		msg_print(_("「一つの指輪は全てを統べ、", "'One Ring to rule them all, "));
		msg_print(NULL);
		msg_print(_("一つの指輪は全てを見つけ、", "One Ring to find them, "));
		msg_print(NULL);
		msg_print(_("一つの指輪は全てを捕らえて", "One Ring to bring them all "));
		msg_print(NULL);
		msg_print(_("暗闇の中に繋ぎとめる。」", "and in the darkness bind them.'"));
		used_up = FALSE;
	}
	else if (o_ptr->tval==TV_PARCHMENT)
	{
		concptr q;
		GAME_TEXT o_name[MAX_NLEN];
		char buf[1024];
		screen_save();

		q=format("book-%d_jp.txt",o_ptr->sval);

		/* Display object description */
		object_desc(o_name, o_ptr, OD_NAME_ONLY);

		/* Build the filename */
		path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, q);

		/* Peruse the help file */
		(void)show_file(TRUE, buf, o_name, 0, 0);
		screen_load();

		used_up=FALSE;
	}

	p_ptr->update |= (PU_COMBINE | PU_REORDER);

	if (!(object_is_aware(o_ptr)))
	{
		chg_virtue(V_PATIENCE, -1);
		chg_virtue(V_CHANCE, 1);
		chg_virtue(V_KNOWLEDGE, -1);
	}

	/* The item was tried */
	object_tried(o_ptr);

	/* An identification was made */
	if (ident && !object_is_aware(o_ptr))
	{
		object_aware(o_ptr);
		gain_exp((lev + (p_ptr->lev >> 1)) / p_ptr->lev);
	}

	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);


	/* Hack -- allow certain scrolls to be "preserved" */
	if (!used_up)
	{
		return;
	}

	sound(SOUND_SCROLL);

	/* Destroy a scroll in the pack */
	if (item >= 0)
	{
		inven_item_increase(item, -1);
		inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Destroy a scroll on the floor */
	else
	{
		floor_item_increase(0 - item, -1);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}
}

/*!
 * @brief 読むコマンドのメインルーチン /
 * Eat some food (from the pack or floor)
 * @return なし
 */
void do_cmd_read_scroll(void)
{
	object_type *o_ptr;
	OBJECT_IDX item;
	concptr q, s;

	if (p_ptr->wild_mode)
	{
		return;
	}

	if (cmd_limit_arena(p_ptr)) return;

	if (p_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
	{
		set_action(ACTION_NONE);
	}

	if (cmd_limit_blind(p_ptr)) return;
	if (cmd_limit_confused(p_ptr)) return;

	/* Restrict choices to scrolls */
	item_tester_hook = item_tester_hook_readable;

	q = _("どの巻物を読みますか? ", "Read which scroll? ");
	s = _("読める巻物がない。", "You have no scrolls to read.");

	o_ptr = choose_object(&item, q, s, (USE_INVEN | USE_FLOOR));
	if (!o_ptr) return;

	/* Read the scroll */
	do_cmd_read_scroll_aux(item, object_is_aware(o_ptr));
}
