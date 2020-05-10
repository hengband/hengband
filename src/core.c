/*!
 * @brief Angbandゲームエンジン / Angband game engine
 * @date 2013/12/31
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2013 Deskull rearranged comment for Doxygen.
 */

#include "angband.h"
#include "io/signal-handlers.h"
#include "util.h"
#include "main/sound-definitions-table.h"
#include "core.h"
#include "core/angband-version.h"
#include "core/stuff-handler.h"
#include "core/special-internal-keys.h"
#include "inet.h"
#include "gameterm.h"
#include "chuukei.h"

#include "creature.h"

#include "birth/birth.h"
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
#include "grid.h"
#include "object-flavor.h"
#include "knowledge/knowledge-autopick.h"
#include "knowledge/knowledge-quests.h"
#include "market/store.h"
#include "spell/technic-info-table.h"
#include "spells-object.h"
#include "spells-status.h"
#include "monster-spell.h"
#include "mind.h"
#include "world.h"
#include "mutation/mutation.h"
#include "market/arena-info-table.h"
#include "market/store-util.h"
#include "quest.h"
#include "view/display-player.h"
#include "player/process-name.h"
#include "player-move.h"
#include "player-class.h"
#include "player-race.h"
#include "player-personality.h"
#include "player-damage.h"
#include "player-effects.h"
#include "cmd-spell.h"
#include "realm/realm-hex.h"
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
#include "io/read-pref-file.h"
#include "scores.h"
#include "autopick/autopick-pref-processor.h"
#include "autopick/autopick-reader-writer.h"
#include "save.h"
#include "realm/realm.h"
#include "realm/realm-song.h"
#include "targeting.h"
#include "spell/spells-util.h"
#include "spell/spells-execution.h"
#include "spell/spells2.h"
#include "spell/spells3.h"
#include "core/output-updater.h"
#include "core/game-closer.h"
#include "core/turn-compensator.h"
#include "inventory/simple-appraiser.h"
#include "core/hp-mp-regenerator.h"
#include "core/hp-mp-processor.h"
#include "mutation/mutation-processor.h"
#include "object/lite-processor.h"
#include "core/magic-effects-timeout-reducer.h"
#include "inventory/inventory-curse.h"
#include "inventory/recharge-processor.h"
#include "wizard/wizard-spoiler.h"
#include "wizard/wizard-special-process.h"

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

/*
 * Flags for initialization
 */
int init_flags;

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
 * @brief 10ゲームターンが進行するごとにプレイヤーの腹を減らす
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
	process_player_hp_mp(player_ptr);
	reduce_magic_effects_timeout(player_ptr);
	reduce_lite_life(player_ptr);
	process_world_aux_mutation(player_ptr);
	execute_cursed_items_effect(player_ptr);
	recharge_magic_items(player_ptr);
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
	case '\r':
	case '\n':
	{
		/* Ignore */
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
		if (!creature_ptr->wild_mode)
			do_cmd_wield(creature_ptr);

		break;
	}
	case 't':
	{
		if (!creature_ptr->wild_mode)
			do_cmd_takeoff(creature_ptr);

		break;
	}
	case 'd':
	{
		if (!creature_ptr->wild_mode)
			do_cmd_drop(creature_ptr);

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
		if (!creature_ptr->wild_mode)
			do_cmd_alter(creature_ptr);

		break;
	}
	case 'T':
	{
		if (!creature_ptr->wild_mode)
			do_cmd_tunnel(creature_ptr);

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
		if (!creature_ptr->wild_mode)
			do_cmd_run(creature_ptr);

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
		if (creature_ptr->action == ACTION_SEARCH)
			set_action(creature_ptr, ACTION_NONE);
		else
			set_action(creature_ptr, ACTION_SEARCH);

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
			(creature_ptr->pclass == CLASS_MIRROR_MASTER))
			do_cmd_mind_browse(creature_ptr);
		else if (creature_ptr->pclass == CLASS_SMITH)
			do_cmd_kaji(creature_ptr, TRUE);
		else if (creature_ptr->pclass == CLASS_MAGIC_EATER)
			do_cmd_magic_eater(creature_ptr, TRUE, FALSE);
		else if (creature_ptr->pclass == CLASS_SNIPER)
			do_cmd_snipe_browse(creature_ptr);
		else
			do_cmd_browse(creature_ptr);

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
		do_cmd_save_screen(creature_ptr, process_autopick_file_command);
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
