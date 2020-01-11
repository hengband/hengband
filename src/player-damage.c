#include "angband.h"
#include "core.h"
#include "util.h"
#include "term.h"

#include "avatar.h"
#include "cmd-dump.h"
#include "realm-song.h"
#include "floor.h"
#include "artifact.h"
#include "object-flavor.h"
#include "object-hook.h"
#include "object-broken.h"
#include "player-move.h"
#include "player-damage.h"
#include "player-personality.h"
#include "player-status.h"
#include "player-effects.h"
#include "player-class.h"
#include "monster-spell.h"
#include "world.h"
#include "view-mainwindow.h"
#include "quest.h"
#include "report.h"
#include "wild.h"
#include "save.h"
#include "files.h"


/*!
* todo 元々「破損したアイテムの数」をreturnしていました
* しかし調査の結果、どの関数も戻り値を使用していませんでした
* よって戻りをvoidとし、破損したアイテム数を計上しているローカル変数を削除しました
* 確認後、問題がなければ本コメントを削除の上コミット願います
* そして英語のコメントにある「stealing」とは一体…詳細不明につき残しました
* なお「toryを省略する必要はないじゃろ」との独断により関数名を変更しました
* 気に入らなければ元に戻して下さい。。。 by Hourier
* ***
* @brief アイテムを指定確率で破損させる /
* Destroys a type of item on a given percent chance
* @param player_ptr プレーヤーへの参照ポインタ
* @param typ 破損判定関数ポインタ
* @param perc 基本確率
* @return なし
* @details
* Note that missiles are no longer necessarily all destroyed
* Destruction taken from "melee.c" code for "stealing".
* New-style wands and rods handled correctly. -LM-
*/
void inventory_damage(player_type *player_ptr, inven_func typ, int perc)
{
	INVENTORY_IDX i;
	int j, amt;
	object_type *o_ptr;
	GAME_TEXT o_name[MAX_NLEN];

	if (CHECK_MULTISHADOW(player_ptr) || player_ptr->current_floor_ptr->inside_arena) return;

	/* Scan through the slots backwards */
	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &player_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;

		/* Hack -- for now, skip artifacts */
		if (object_is_artifact(o_ptr)) continue;

		/* Give this item slot a shot at death */
		if (!(*typ)(o_ptr)) continue;
		
		/* Count the casualties */
		for (amt = j = 0; j < o_ptr->number; ++j)
		{
			if (randint0(100) < perc) amt++;
		}

		/* Some casualities */
		if (!amt) continue;
		
		object_desc(o_name, o_ptr, OD_OMIT_PREFIX);

		msg_format(_("%s(%c)が%s壊れてしまった！", "%sour %s (%c) %s destroyed!"),
#ifdef JP
			o_name, index_to_label(i), ((o_ptr->number > 1) ?
			((amt == o_ptr->number) ? "全部" : (amt > 1 ? "何個か" : "一個")) : ""));
#else
			((o_ptr->number > 1) ? ((amt == o_ptr->number) ? "All of y" :
			(amt > 1 ? "Some of y" : "One of y")) : "Y"), o_name, index_to_label(i), ((amt > 1) ? "were" : "was"));
#endif

#ifdef JP
		if ((player_ptr->pseikaku == SEIKAKU_COMBAT) || (player_ptr->inventory_list[INVEN_BOW].name1 == ART_CRIMSON))
			msg_print("やりやがったな！");
		else if ((player_ptr->pseikaku == SEIKAKU_CHARGEMAN))
		{
			if (randint0(2) == 0) msg_print(_("ジュラル星人め！", ""));
			else msg_print(_("弱い者いじめは止めるんだ！", ""));
		}
#endif

		/* Potions smash open */
		if (object_is_potion(o_ptr))
		{
			(void)potion_smash_effect(0, player_ptr->y, player_ptr->x, o_ptr->k_idx);
		}

		/* Reduce the charges of rods/wands */
		reduce_charges(o_ptr, amt);

		/* Destroy "amt" items */
		inven_item_increase(player_ptr, i, -amt);
		inven_item_optimize(player_ptr, i);
	}
}


/*!
* @brief 酸攻撃による装備のAC劣化処理 /
* Acid has hit the player, attempt to affect some armor.
* @param 酸を浴びたキャラクタへの参照ポインタ
* @return 装備による軽減があったならTRUEを返す
* @details
* Note that the "base armor" of an object never changes.
* If any armor is damaged (or resists), the player takes less damage.
*/
static bool acid_minus_ac(player_type *creature_ptr)
{
	object_type *o_ptr = NULL;
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	GAME_TEXT o_name[MAX_NLEN];

	/* Pick a (possibly empty) creature_ptr->inventory_list slot */
	switch (randint1(7))
	{
	case 1: o_ptr = &creature_ptr->inventory_list[INVEN_RARM]; break;
	case 2: o_ptr = &creature_ptr->inventory_list[INVEN_LARM]; break;
	case 3: o_ptr = &creature_ptr->inventory_list[INVEN_BODY]; break;
	case 4: o_ptr = &creature_ptr->inventory_list[INVEN_OUTER]; break;
	case 5: o_ptr = &creature_ptr->inventory_list[INVEN_HANDS]; break;
	case 6: o_ptr = &creature_ptr->inventory_list[INVEN_HEAD]; break;
	case 7: o_ptr = &creature_ptr->inventory_list[INVEN_FEET]; break;
	}

	if (!o_ptr->k_idx) return FALSE;
	if (!object_is_armour(o_ptr)) return FALSE;

	object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
	object_flags(o_ptr, flgs);
	/* No damage left to be done */
	if (o_ptr->ac + o_ptr->to_a <= 0)
	{
		msg_format(_("%sは既にボロボロだ！", "Your %s is already crumble!"), o_name);
		return FALSE;
	}

	/* Object resists */
	if (have_flag(flgs, TR_IGNORE_ACID))
	{
		msg_format(_("しかし%sには効果がなかった！", "Your %s is unaffected!"), o_name);
		return TRUE;
	}

	msg_format(_("%sが酸で腐食した！", "Your %s is corroded!"), o_name);

	/* Damage the item */
	o_ptr->to_a--;

	/* Calculate bonuses */
	creature_ptr->update |= (PU_BONUS);
	creature_ptr->window |= (PW_EQUIP | PW_PLAYER);

	calc_android_exp(creature_ptr);

	/* Item was damaged */
	return TRUE;
}


/*!
* @brief 酸属性によるプレイヤー損害処理 /
* Hurt the player with Acid
* @param creature_ptr 酸を浴びたキャラクタへの参照ポインタ
* @param dam 基本ダメージ量
* @param kb_str ダメージ原因記述
* @param monspell 原因となったモンスター特殊攻撃ID
* @param aura オーラよるダメージが原因ならばTRUE
* @return 修正HPダメージ量
*/
HIT_POINT acid_dam(player_type *creature_ptr, HIT_POINT dam, concptr kb_str, int monspell, bool aura)
{
	HIT_POINT get_damage;
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;
	bool double_resist = IS_OPPOSE_ACID();

	/* Total Immunity */
	if (creature_ptr->immune_acid || (dam <= 0))
	{
		learn_spell(creature_ptr, monspell);
		return 0;
	}

	/* Vulnerability (Ouch!) */
	if (creature_ptr->muta3 & MUT3_VULN_ELEM) dam *= 2;
	if (creature_ptr->special_defense & KATA_KOUKIJIN) dam += dam / 3;

	/* Resist the damage */
	if (creature_ptr->resist_acid) dam = (dam + 2) / 3;
	if (double_resist) dam = (dam + 2) / 3;

	if (aura || !CHECK_MULTISHADOW(creature_ptr))
	{
		if ((!(double_resist || creature_ptr->resist_acid)) &&
			one_in_(HURT_CHANCE))
			(void)do_dec_stat(creature_ptr, A_CHR);

		/* If any armor gets hit, defend the player */
		if (acid_minus_ac(creature_ptr)) dam = (dam + 1) / 2;
	}

	get_damage = take_hit(creature_ptr, aura ? DAMAGE_NOESCAPE : DAMAGE_ATTACK, dam, kb_str, monspell);

	/* Inventory damage */
	if (!aura && !(double_resist && creature_ptr->resist_acid))
		inventory_damage(creature_ptr, set_acid_destroy, inv);
	return get_damage;
}


/*!
* @brief 電撃属性によるプレイヤー損害処理 /
* Hurt the player with electricity
* @param creature_ptr 電撃を浴びたキャラクタへの参照ポインタ
* @param dam 基本ダメージ量
* @param kb_str ダメージ原因記述
* @param monspell 原因となったモンスター特殊攻撃ID
* @param aura オーラよるダメージが原因ならばTRUE
* @return 修正HPダメージ量
*/
HIT_POINT elec_dam(player_type *creature_ptr, HIT_POINT dam, concptr kb_str, int monspell, bool aura)
{
	HIT_POINT get_damage;
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;
	bool double_resist = IS_OPPOSE_ELEC();

	/* Total immunity */
	if (creature_ptr->immune_elec || (dam <= 0))
	{
		learn_spell(creature_ptr, monspell);
		return 0;
	}

	/* Vulnerability (Ouch!) */
	if (creature_ptr->muta3 & MUT3_VULN_ELEM) dam *= 2;
	if (creature_ptr->special_defense & KATA_KOUKIJIN) dam += dam / 3;
	if (PRACE_IS_(creature_ptr, RACE_ANDROID)) dam += dam / 3;

	/* Resist the damage */
	if (creature_ptr->resist_elec) dam = (dam + 2) / 3;
	if (double_resist) dam = (dam + 2) / 3;

	if (aura || !CHECK_MULTISHADOW(creature_ptr))
	{
		if ((!(double_resist || creature_ptr->resist_elec)) &&
			one_in_(HURT_CHANCE))
			(void)do_dec_stat(creature_ptr, A_DEX);
	}

	get_damage = take_hit(creature_ptr, aura ? DAMAGE_NOESCAPE : DAMAGE_ATTACK, dam, kb_str, monspell);

	/* Inventory damage */
	if (!aura && !(double_resist && creature_ptr->resist_elec))
		inventory_damage(creature_ptr, set_elec_destroy, inv);

	return get_damage;
}


/*!
* @brief 火炎属性によるプレイヤー損害処理 /
* Hurt the player with Fire
* @param creature_ptr 火炎を浴びたキャラクタへの参照ポインタ
* @param dam 基本ダメージ量
* @param kb_str ダメージ原因記述
* @param monspell 原因となったモンスター特殊攻撃ID
* @param aura オーラよるダメージが原因ならばTRUE
* @return 修正HPダメージ量
*/
HIT_POINT fire_dam(player_type *creature_ptr, HIT_POINT dam, concptr kb_str, int monspell, bool aura)
{
	HIT_POINT get_damage;
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;
	bool double_resist = IS_OPPOSE_FIRE();

	/* Totally immune */
	if (creature_ptr->immune_fire || (dam <= 0))
	{
		learn_spell(creature_ptr, monspell);
		return 0;
	}

	/* Vulnerability (Ouch!) */
	if (creature_ptr->muta3 & MUT3_VULN_ELEM) dam *= 2;
	if (PRACE_IS_(creature_ptr, RACE_ENT)) dam += dam / 3;
	if (creature_ptr->special_defense & KATA_KOUKIJIN) dam += dam / 3;

	/* Resist the damage */
	if (creature_ptr->resist_fire) dam = (dam + 2) / 3;
	if (double_resist) dam = (dam + 2) / 3;

	if (aura || !CHECK_MULTISHADOW(creature_ptr))
	{
		if ((!(double_resist || creature_ptr->resist_fire)) &&
			one_in_(HURT_CHANCE))
			(void)do_dec_stat(creature_ptr, A_STR);
	}

	get_damage = take_hit(creature_ptr, aura ? DAMAGE_NOESCAPE : DAMAGE_ATTACK, dam, kb_str, monspell);

	/* Inventory damage */
	if (!aura && !(double_resist && creature_ptr->resist_fire))
		inventory_damage(creature_ptr, set_fire_destroy, inv);

	return get_damage;
}


/*!
* @brief 冷気属性によるプレイヤー損害処理 /
* Hurt the player with Cold
* @param creature_ptr 冷気を浴びたキャラクタへの参照ポインタ
* @param dam 基本ダメージ量
* @param kb_str ダメージ原因記述
* @param monspell 原因となったモンスター特殊攻撃ID
* @param aura オーラよるダメージが原因ならばTRUE
* @return 修正HPダメージ量
*/
HIT_POINT cold_dam(player_type *creature_ptr, HIT_POINT dam, concptr kb_str, int monspell, bool aura)
{
	HIT_POINT get_damage;
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;
	bool double_resist = IS_OPPOSE_COLD();

	/* Total immunity */
	if (creature_ptr->immune_cold || (dam <= 0))
	{
		learn_spell(creature_ptr, monspell);
		return 0;
	}

	/* Vulnerability (Ouch!) */
	if (creature_ptr->muta3 & MUT3_VULN_ELEM) dam *= 2;
	if (creature_ptr->special_defense & KATA_KOUKIJIN) dam += dam / 3;

	/* Resist the damage */
	if (creature_ptr->resist_cold) dam = (dam + 2) / 3;
	if (double_resist) dam = (dam + 2) / 3;

	if (aura || !CHECK_MULTISHADOW(creature_ptr))
	{
		if ((!(double_resist || creature_ptr->resist_cold)) &&
			one_in_(HURT_CHANCE))
			(void)do_dec_stat(creature_ptr, A_STR);
	}

	get_damage = take_hit(creature_ptr, aura ? DAMAGE_NOESCAPE : DAMAGE_ATTACK, dam, kb_str, monspell);

	/* Inventory damage */
	if (!aura && !(double_resist && creature_ptr->resist_cold))
		inventory_damage(creature_ptr, set_cold_destroy, inv);

	return get_damage;
}


/*
 * Decreases players hit points and sets death flag if necessary
 *
 * Invulnerability needs to be changed into a "shield"
 *
 * Hack -- this function allows the user to save (or quit)
 * the game when he dies, since the "You die." message is shown before
 * setting the player to "dead".
 */

int take_hit(player_type *creature_ptr, int damage_type, HIT_POINT damage, concptr hit_from, int monspell)
{
	int old_chp = creature_ptr->chp;

	char death_message[1024];
	char tmp[1024];

	int warning = (creature_ptr->mhp * hitpoint_warn / 10);
	if (creature_ptr->is_dead) return 0;

	if (creature_ptr->sutemi) damage *= 2;
	if (creature_ptr->special_defense & KATA_IAI) damage += (damage + 4) / 5;

	if (easy_band) damage = (damage + 1) / 2;

	if (damage_type != DAMAGE_USELIFE)
	{
		disturb(creature_ptr, TRUE, TRUE);
		if (auto_more)
		{
			creature_ptr->now_damaged = TRUE;
		}
	}

	if (monspell >= 0) learn_spell(creature_ptr, monspell);

	/* Mega-Hack -- Apply "invulnerability" */
	if ((damage_type != DAMAGE_USELIFE) && (damage_type != DAMAGE_LOSELIFE))
	{
		if (IS_INVULN(creature_ptr) && (damage < 9000))
		{
			if (damage_type == DAMAGE_FORCE)
			{
				msg_print(_("バリアが切り裂かれた！", "The attack cuts your shield of invulnerability open!"));
			}
			else if (one_in_(PENETRATE_INVULNERABILITY))
			{
				msg_print(_("無敵のバリアを破って攻撃された！", "The attack penetrates your shield of invulnerability!"));
			}
			else
			{
				return 0;
			}
		}

		if (CHECK_MULTISHADOW(creature_ptr))
		{
			if (damage_type == DAMAGE_FORCE)
			{
				msg_print(_("幻影もろとも体が切り裂かれた！", "The attack hits Shadow together with you!"));
			}
			else if (damage_type == DAMAGE_ATTACK)
			{
				msg_print(_("攻撃は幻影に命中し、あなたには届かなかった。", "The attack hits Shadow, you are unharmed!"));
				return 0;
			}
		}

		if (creature_ptr->wraith_form)
		{
			if (damage_type == DAMAGE_FORCE)
			{
				msg_print(_("半物質の体が切り裂かれた！", "The attack cuts through your ethereal body!"));
			}
			else
			{
				damage /= 2;
				if ((damage == 0) && one_in_(2)) damage = 1;
			}
		}

		if (creature_ptr->special_defense & KATA_MUSOU)
		{
			damage /= 2;
			if ((damage == 0) && one_in_(2)) damage = 1;
		}
	} /* not if LOSELIFE USELIFE */

	/* Hurt the player */
	creature_ptr->chp -= damage;
	if (damage_type == DAMAGE_GENO && creature_ptr->chp < 0)
	{
		damage += creature_ptr->chp;
		creature_ptr->chp = 0;
	}

	/* Display the hitpoints */
	creature_ptr->redraw |= (PR_HP);

	creature_ptr->window |= (PW_PLAYER);

	if (damage_type != DAMAGE_GENO && creature_ptr->chp == 0)
	{
		chg_virtue(creature_ptr, V_SACRIFICE, 1);
		chg_virtue(creature_ptr, V_CHANCE, 2);
	}

	/* Dead player */
	if (creature_ptr->chp < 0)
	{
		bool android = (creature_ptr->prace == RACE_ANDROID ? TRUE : FALSE);

#ifdef JP       /* 死んだ時に強制終了して死を回避できなくしてみた by Habu */
		if (!cheat_save)
			if (!save_player(creature_ptr)) msg_print("セーブ失敗！");
#endif

		sound(SOUND_DEATH);

		chg_virtue(creature_ptr, V_SACRIFICE, 10);

		handle_stuff(creature_ptr);
		creature_ptr->leaving = TRUE;

		/* Note death */
		creature_ptr->is_dead = TRUE;

		if (creature_ptr->current_floor_ptr->inside_arena)
		{
			concptr m_name = r_name + r_info[arena_info[creature_ptr->arena_number].r_idx].name;
			msg_format(_("あなたは%sの前に敗れ去った。", "You are beaten by %s."), m_name);
			msg_print(NULL);
			if (record_arena) exe_write_diary(creature_ptr, NIKKI_ARENA, -1 - creature_ptr->arena_number, m_name);
		}
		else
		{
			QUEST_IDX q_idx = quest_number(creature_ptr->current_floor_ptr->dun_level);
			bool seppuku = streq(hit_from, "Seppuku");
			bool winning_seppuku = current_world_ptr->total_winner && seppuku;

			play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_GAMEOVER);

#ifdef WORLD_SCORE
			/* Make screen dump */
			screen_dump = make_screen_dump(creature_ptr);
#endif

			/* Note cause of death */
			if (seppuku)
			{
				strcpy(creature_ptr->died_from, hit_from);
#ifdef JP
				if (!winning_seppuku) strcpy(creature_ptr->died_from, "切腹");
#endif
			}
			else
			{
				char dummy[1024];
#ifdef JP
				sprintf(dummy, "%s%s%s", !creature_ptr->paralyzed ? "" : creature_ptr->free_act ? "彫像状態で" : "麻痺状態で", creature_ptr->image ? "幻覚に歪んだ" : "", hit_from);
#else
				sprintf(dummy, "%s%s", hit_from, !creature_ptr->paralyzed ? "" : " while helpless");
#endif
				my_strcpy(creature_ptr->died_from, dummy, sizeof creature_ptr->died_from);
			}

			/* No longer a winner */
			current_world_ptr->total_winner = FALSE;

			if (winning_seppuku)
			{
				exe_write_diary(creature_ptr, NIKKI_BUNSHOU, 0, _("勝利の後切腹した。", "did Seppuku after the winning."));
			}
			else
			{
				char buf[20];

				if (creature_ptr->current_floor_ptr->inside_arena)
					strcpy(buf, _("アリーナ", "in the Arena"));
				else if (!creature_ptr->current_floor_ptr->dun_level)
					strcpy(buf, _("地上", "on the surface"));
				else if (q_idx && (is_fixed_quest_idx(q_idx) &&
					!((q_idx == QUEST_OBERON) || (q_idx == QUEST_SERPENT))))
					strcpy(buf, _("クエスト", "in a quest"));
				else
					sprintf(buf, _("%d階", "level %d"), (int)creature_ptr->current_floor_ptr->dun_level);

				sprintf(tmp, _("%sで%sに殺された。", "killed by %s %s."), buf, creature_ptr->died_from);
				exe_write_diary(creature_ptr, NIKKI_BUNSHOU, 0, tmp);
			}

			exe_write_diary(creature_ptr, NIKKI_GAMESTART, 1, _("-------- ゲームオーバー --------", "--------   Game  Over   --------"));
			exe_write_diary(creature_ptr, NIKKI_BUNSHOU, 1, "\n\n\n\n");

			flush();

			if (get_check_strict(_("画面を保存しますか？", "Dump the screen? "), CHECK_NO_HISTORY))
			{
				do_cmd_save_screen(creature_ptr);
			}

			flush();

			/* Initialize "last message" buffer */
			if (creature_ptr->last_message) string_free(creature_ptr->last_message);
			creature_ptr->last_message = NULL;

			/* Hack -- Note death */
			if (!last_words)
			{
#ifdef JP
				msg_format("あなたは%sました。", android ? "壊れ" : "死に");
#else
				msg_print(android ? "You are broken." : "You die.");
#endif

				msg_print(NULL);
			}
			else
			{
				if (winning_seppuku)
				{
					get_rnd_line(_("seppuku_j.txt", "seppuku.txt"), 0, death_message);
				}
				else
				{
					get_rnd_line(_("death_j.txt", "death.txt"), 0, death_message);
				}

				do
				{
#ifdef JP
					while (!get_string(winning_seppuku ? "辞世の句: " : "断末魔の叫び: ", death_message, 1024));
#else
					while (!get_string("Last word: ", death_message, 1024));
#endif
				} while (winning_seppuku && !get_check_strict(_("よろしいですか？", "Are you sure? "), CHECK_NO_HISTORY));

				if (death_message[0] == '\0')
				{
#ifdef JP
					strcpy(death_message, format("あなたは%sました。", android ? "壊れ" : "死に"));
#else
					strcpy(death_message, android ? "You are broken." : "You die.");
#endif
				}
				else creature_ptr->last_message = string_make(death_message);

#ifdef JP
				if (winning_seppuku)
				{
					int i, len;
					int w = Term->wid;
					int h = Term->hgt;
					int msg_pos_x[9] = { 5,  7,  9, 12,  14,  17,  19,  21, 23 };
					int msg_pos_y[9] = { 3,  4,  5,  4,   5,   4,   5,   6,  4 };
					concptr str;
					char* str2;

					Term_clear();

					/* 桜散る */
					for (i = 0; i < 40; i++)
						Term_putstr(randint0(w / 2) * 2, randint0(h), 2, TERM_VIOLET, "υ");

					str = death_message;
					if (strncmp(str, "「", 2) == 0) str += 2;

					str2 = my_strstr(str, "」");
					if (str2 != NULL) *str2 = '\0';

					i = 0;
					while (i < 9)
					{
						str2 = my_strstr(str, " ");
						if (str2 == NULL) len = strlen(str);
						else len = str2 - str;

						if (len != 0)
						{
							Term_putstr_v(w * 3 / 4 - 2 - msg_pos_x[i] * 2, msg_pos_y[i], len,
								TERM_WHITE, str);
							if (str2 == NULL) break;
							i++;
						}
						str = str2 + 1;
						if (*str == 0) break;
					}

					/* Hide cursor */
					Term_putstr(w - 1, h - 1, 1, TERM_WHITE, " ");

					flush();
#ifdef WORLD_SCORE
					/* Make screen dump */
					screen_dump = make_screen_dump(creature_ptr);
#endif

					/* Wait a key press */
					(void)inkey();
				}
				else
#endif
					msg_print(death_message);
			}
		}

		/* Dead */
		return damage;
	}

	handle_stuff(creature_ptr);

	/* Hitpoint warning */
	if (creature_ptr->chp < warning)
	{
		/* Hack -- bell on first notice */
		if (old_chp > warning) bell();

		sound(SOUND_WARN);

		if (record_danger && (old_chp > warning))
		{
			if (creature_ptr->image && damage_type == DAMAGE_ATTACK)
				hit_from = _("何か", "something");

			sprintf(tmp, _("%sによってピンチに陥った。", "A critical situation because of %s."), hit_from);
			exe_write_diary(creature_ptr, NIKKI_BUNSHOU, 0, tmp);
		}

		if (auto_more)
		{
			/* stop auto_more even if DAMAGE_USELIFE */
			creature_ptr->now_damaged = TRUE;
		}

		msg_print(_("*** 警告:低ヒット・ポイント！ ***", "*** LOW HITPOINT WARNING! ***"));
		msg_print(NULL);
		flush();
	}
	if (creature_ptr->wild_mode && !creature_ptr->leaving && (creature_ptr->chp < MAX(warning, creature_ptr->mhp / 5)))
	{
		change_wild_mode(creature_ptr, FALSE);
	}
	return damage;
}
