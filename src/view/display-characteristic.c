#include "display-characteristic.h"
#include "term.h"
#include "player/permanent-resistances.h"
#include "player/temporary-resistances.h"
#include "player/race-resistances.h"

typedef struct {
	BIT_FLAGS player_flags[TR_FLAG_SIZE];
	BIT_FLAGS tim_player_flags[TR_FLAG_SIZE];
	BIT_FLAGS player_imm[TR_FLAG_SIZE];
	BIT_FLAGS tim_player_imm[TR_FLAG_SIZE];
	BIT_FLAGS player_vuln[TR_FLAG_SIZE];
	BIT_FLAGS known_obj_imm[TR_FLAG_SIZE];
} all_player_flags;

/*!
 * @brief プレイヤーの特性フラグ一種を表示する
 * Helper function, see below
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param row コンソール表示位置の左上行
 * @param col コンソール表示位置の左上列
 * @param header コンソール上で表示する特性名
 * @param flag1 参照する特性ID
 * @param f プレイヤーの特性情報構造体
 * @param mode 表示オプション
 * @return なし
 */
static void display_one_characteristic_info(player_type *creature_ptr, TERM_LEN row, TERM_LEN col, concptr header, int flag1, all_player_flags *f, u16b mode)
{
	byte header_color = TERM_L_DARK;
	int header_col = col;

	bool vuln = FALSE;
	if (have_flag(f->player_vuln, flag1) &&
		!(have_flag(f->known_obj_imm, flag1) ||
			have_flag(f->player_imm, flag1) ||
			have_flag(f->tim_player_imm, flag1)))
		vuln = TRUE;

	col += strlen(header) + 1;

	/* Weapon flags need only two column */
	int max_i = (mode & DP_WP) ? INVEN_LARM + 1 : INVEN_TOTAL;

	for (int i = INVEN_RARM; i < max_i; i++)
	{
		BIT_FLAGS flgs[TR_FLAG_SIZE];
		object_type *o_ptr;
		o_ptr = &creature_ptr->inventory_list[i];
		object_flags_known(o_ptr, flgs);
		if (!(mode & DP_IMM))
			c_put_str((byte)(vuln ? TERM_RED : TERM_SLATE), ".", row, col);

		if (mode & DP_CURSE)
		{
			if (have_flag(flgs, TR_ADD_L_CURSE) || have_flag(flgs, TR_ADD_H_CURSE))
			{
				c_put_str(TERM_L_DARK, "+", row, col);
				header_color = TERM_WHITE;
			}

			if (o_ptr->curse_flags & (TRC_CURSED | TRC_HEAVY_CURSE))
			{
				c_put_str(TERM_WHITE, "+", row, col);
				header_color = TERM_WHITE;
			}

			if (o_ptr->curse_flags & TRC_PERMA_CURSE)
			{
				c_put_str(TERM_WHITE, "*", row, col);
				header_color = TERM_WHITE;
			}

			col++;
			continue;
		}

		if (flag1 == TR_LITE_1)
		{
			if (HAVE_DARK_FLAG(flgs))
			{
				c_put_str(TERM_L_DARK, "+", row, col);
				header_color = TERM_WHITE;
			}
			else if (HAVE_LITE_FLAG(flgs))
			{
				c_put_str(TERM_WHITE, "+", row, col);
				header_color = TERM_WHITE;
			}

			col++;
			continue;
		}

		if (have_flag(flgs, flag1))
		{
			c_put_str((byte)(vuln ? TERM_L_RED : TERM_WHITE),
				(mode & DP_IMM) ? "*" : "+", row, col);
			header_color = TERM_WHITE;
		}

		col++;
	}

	if (mode & DP_IMM)
	{
		if (header_color != TERM_L_DARK)
		{
			c_put_str(header_color, header, row, header_col);
		}

		return;
	}

	c_put_str((byte)(vuln ? TERM_RED : TERM_SLATE), ".", row, col);
	if (have_flag(f->player_flags, flag1))
	{
		c_put_str((byte)(vuln ? TERM_L_RED : TERM_WHITE), "+", row, col);
		header_color = TERM_WHITE;
	}

	if (have_flag(f->tim_player_flags, flag1))
	{
		c_put_str((byte)(vuln ? TERM_ORANGE : TERM_YELLOW), "#", row, col);
		header_color = TERM_WHITE;
	}

	if (have_flag(f->tim_player_imm, flag1))
	{
		c_put_str(TERM_YELLOW, "*", row, col);
		header_color = TERM_WHITE;
	}

	if (have_flag(f->player_imm, flag1))
	{
		c_put_str(TERM_WHITE, "*", row, col);
		header_color = TERM_WHITE;
	}

	if (vuln) c_put_str(TERM_RED, "v", row, col + 1);
	c_put_str(header_color, header, row, header_col);
}


/*!
 * @brief プレーヤーの基本耐性を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param display_player_equippy 表示へのコールバック
 * @param f 特性フラグへの参照ポインタ
 * @return なし
 */
static void display_basic_resistance_info(player_type *creature_ptr, void(*display_player_equippy)(player_type*, TERM_LEN, TERM_LEN, BIT_FLAGS16), all_player_flags *f)
{
	TERM_LEN row = 12;
	TERM_LEN col = 1;
	(*display_player_equippy)(creature_ptr, row - 2, col + 8, 0);
	c_put_str(TERM_WHITE, "abcdefghijkl@", row - 1, col + 8);

#ifdef JP
	display_one_characteristic_info(creature_ptr, row + 0, col, "耐酸  :", TR_RES_ACID, f, 0);
	display_one_characteristic_info(creature_ptr, row + 0, col, "耐酸  :", TR_IM_ACID, f, DP_IMM);
	display_one_characteristic_info(creature_ptr, row + 1, col, "耐電撃:", TR_RES_ELEC, f, 0);
	display_one_characteristic_info(creature_ptr, row + 1, col, "耐電撃:", TR_IM_ELEC, f, DP_IMM);
	display_one_characteristic_info(creature_ptr, row + 2, col, "耐火炎:", TR_RES_FIRE, f, 0);
	display_one_characteristic_info(creature_ptr, row + 2, col, "耐火炎:", TR_IM_FIRE, f, DP_IMM);
	display_one_characteristic_info(creature_ptr, row + 3, col, "耐冷気:", TR_RES_COLD, f, 0);
	display_one_characteristic_info(creature_ptr, row + 3, col, "耐冷気:", TR_IM_COLD, f, DP_IMM);
	display_one_characteristic_info(creature_ptr, row + 4, col, "耐毒  :", TR_RES_POIS, f, 0);
	display_one_characteristic_info(creature_ptr, row + 5, col, "耐閃光:", TR_RES_LITE, f, 0);
	display_one_characteristic_info(creature_ptr, row + 6, col, "耐暗黒:", TR_RES_DARK, f, 0);
	display_one_characteristic_info(creature_ptr, row + 7, col, "耐破片:", TR_RES_SHARDS, f, 0);
	display_one_characteristic_info(creature_ptr, row + 8, col, "耐盲目:", TR_RES_BLIND, f, 0);
	display_one_characteristic_info(creature_ptr, row + 9, col, "耐混乱:", TR_RES_CONF, f, 0);
#else
	display_flag_aux(creature_ptr, row + 0, col, "Acid  :", TR_RES_ACID, f, 0);
	display_flag_aux(creature_ptr, row + 0, col, "Acid  :", TR_IM_ACID, f, DP_IMM);
	display_flag_aux(creature_ptr, row + 1, col, "Elec  :", TR_RES_ELEC, f, 0);
	display_flag_aux(creature_ptr, row + 1, col, "Elec  :", TR_IM_ELEC, f, DP_IMM);
	display_flag_aux(creature_ptr, row + 2, col, "Fire  :", TR_RES_FIRE, f, 0);
	display_flag_aux(creature_ptr, row + 2, col, "Fire  :", TR_IM_FIRE, f, DP_IMM);
	display_flag_aux(creature_ptr, row + 3, col, "Cold  :", TR_RES_COLD, f, 0);
	display_flag_aux(creature_ptr, row + 3, col, "Cold  :", TR_IM_COLD, f, DP_IMM);
	display_flag_aux(creature_ptr, row + 4, col, "Poison:", TR_RES_POIS, f, 0);
	display_flag_aux(creature_ptr, row + 5, col, "Light :", TR_RES_LITE, f, 0);
	display_flag_aux(creature_ptr, row + 6, col, "Dark  :", TR_RES_DARK, f, 0);
	display_flag_aux(creature_ptr, row + 7, col, "Shard :", TR_RES_SHARDS, f, 0);
	display_flag_aux(creature_ptr, row + 8, col, "Blind :", TR_RES_BLIND, f, 0);
	display_flag_aux(creature_ptr, row + 9, col, "Conf  :", TR_RES_CONF, f, 0);
#endif
}


/*!
 * @brief プレーヤーの上位耐性を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param display_player_equippy 表示へのコールバック
 * @param f 特性フラグへの参照ポインタ
 * @return なし
 */
void display_advanced_resistance_info(player_type *creature_ptr, void(*display_player_equippy)(player_type*, TERM_LEN, TERM_LEN, BIT_FLAGS16), all_player_flags *f)
{
	TERM_LEN row = 12;
	TERM_LEN col = 26;
	(*display_player_equippy)(creature_ptr, row - 2, col + 8, 0);
	c_put_str(TERM_WHITE, "abcdefghijkl@", row - 1, col + 8);

#ifdef JP
	display_one_characteristic_info(creature_ptr, row + 0, col, "耐轟音:", TR_RES_SOUND, f, 0);
	display_one_characteristic_info(creature_ptr, row + 1, col, "耐地獄:", TR_RES_NETHER, f, 0);
	display_one_characteristic_info(creature_ptr, row + 2, col, "耐因混:", TR_RES_NEXUS, f, 0);
	display_one_characteristic_info(creature_ptr, row + 3, col, "耐カオ:", TR_RES_CHAOS, f, 0);
	display_one_characteristic_info(creature_ptr, row + 4, col, "耐劣化:", TR_RES_DISEN, f, 0);
	display_one_characteristic_info(creature_ptr, row + 5, col, "耐恐怖:", TR_RES_FEAR, f, 0);
	display_one_characteristic_info(creature_ptr, row + 6, col, "反射  :", TR_REFLECT, f, 0);
	display_one_characteristic_info(creature_ptr, row + 7, col, "火炎オ:", TR_SH_FIRE, f, 0);
	display_one_characteristic_info(creature_ptr, row + 8, col, "電気オ:", TR_SH_ELEC, f, 0);
	display_one_characteristic_info(creature_ptr, row + 9, col, "冷気オ:", TR_SH_COLD, f, 0);
#else
	display_flag_aux(creature_ptr, row + 0, col, "Sound :", TR_RES_SOUND, f, 0);
	display_flag_aux(creature_ptr, row + 1, col, "Nether:", TR_RES_NETHER, f, 0);
	display_flag_aux(creature_ptr, row + 2, col, "Nexus :", TR_RES_NEXUS, f, 0);
	display_flag_aux(creature_ptr, row + 3, col, "Chaos :", TR_RES_CHAOS, f, 0);
	display_flag_aux(creature_ptr, row + 4, col, "Disnch:", TR_RES_DISEN, f, 0);
	display_flag_aux(creature_ptr, row + 5, col, "Fear  :", TR_RES_FEAR, f, 0);
	display_flag_aux(creature_ptr, row + 6, col, "Reflct:", TR_REFLECT, f, 0);
	display_flag_aux(creature_ptr, row + 7, col, "AuFire:", TR_SH_FIRE, f, 0);
	display_flag_aux(creature_ptr, row + 8, col, "AuElec:", TR_SH_ELEC, f, 0);
	display_flag_aux(creature_ptr, row + 9, col, "AuCold:", TR_SH_COLD, f, 0);
#endif
}


/*!
 * @brief プレイヤーの特性フラグ一覧表示1
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param display_player_equippy 表示へのコールバック
 * Special display, part 1
 * @return なし
 */
void display_player_flag_info_1(player_type *creature_ptr, void(*display_player_equippy)(player_type*, TERM_LEN, TERM_LEN, BIT_FLAGS16))
{
	all_player_flags f;
	player_flags(creature_ptr, f.player_flags);
	tim_player_flags(creature_ptr, f.tim_player_flags);
	player_immunity(creature_ptr, f.player_imm);
	tim_player_immunity(creature_ptr, f.tim_player_imm);
	known_obj_immunity(creature_ptr, f.known_obj_imm);
	player_vulnerability_flags(creature_ptr, f.player_vuln);

	display_basic_resistance_info(creature_ptr, display_player_equippy, &f);
	display_advanced_resistance_info(creature_ptr, display_player_equippy, &f);
	
	/*** Set 3 ***/
	TERM_LEN row = 12;
	TERM_LEN col = 51;
	(*display_player_equippy)(creature_ptr, row - 2, col + 12, 0);
	c_put_str(TERM_WHITE, "abcdefghijkl@", row - 1, col + 12);

#ifdef JP
	display_one_characteristic_info(creature_ptr, row + 0, col, "加速      :", TR_SPEED, &f, 0);
	display_one_characteristic_info(creature_ptr, row + 1, col, "耐麻痺    :", TR_FREE_ACT, &f, 0);
	display_one_characteristic_info(creature_ptr, row + 2, col, "透明体視認:", TR_SEE_INVIS, &f, 0);
	display_one_characteristic_info(creature_ptr, row + 3, col, "経験値保持:", TR_HOLD_EXP, &f, 0);
	display_one_characteristic_info(creature_ptr, row + 4, col, "警告      :", TR_WARNING, &f, 0);
	display_one_characteristic_info(creature_ptr, row + 5, col, "遅消化    :", TR_SLOW_DIGEST, &f, 0);
	display_one_characteristic_info(creature_ptr, row + 6, col, "急回復    :", TR_REGEN, &f, 0);
	display_one_characteristic_info(creature_ptr, row + 7, col, "浮遊      :", TR_LEVITATION, &f, 0);
	display_one_characteristic_info(creature_ptr, row + 8, col, "永遠光源  :", TR_LITE_1, &f, 0);
	display_one_characteristic_info(creature_ptr, row + 9, col, "呪い      :", 0, &f, DP_CURSE);
#else
	display_flag_aux(creature_ptr, row + 0, col, "Speed     :", TR_SPEED, &f, 0);
	display_flag_aux(creature_ptr, row + 1, col, "FreeAction:", TR_FREE_ACT, &f, 0);
	display_flag_aux(creature_ptr, row + 2, col, "SeeInvisi.:", TR_SEE_INVIS, &f, 0);
	display_flag_aux(creature_ptr, row + 3, col, "Hold Exp  :", TR_HOLD_EXP, &f, 0);
	display_flag_aux(creature_ptr, row + 4, col, "Warning   :", TR_WARNING, &f, 0);
	display_flag_aux(creature_ptr, row + 5, col, "SlowDigest:", TR_SLOW_DIGEST, &f, 0);
	display_flag_aux(creature_ptr, row + 6, col, "Regene.   :", TR_REGEN, &f, 0);
	display_flag_aux(creature_ptr, row + 7, col, "Levitation:", TR_LEVITATION, &f, 0);
	display_flag_aux(creature_ptr, row + 8, col, "Perm Lite :", TR_LITE_1, &f, 0);
	display_flag_aux(creature_ptr, row + 9, col, "Cursed    :", 0, &f, DP_CURSE);
#endif
}


/*!
 * @brief スレイ系の特性フラグを表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param display_player_equippy 表示へのコールバック
 * @param f 特性フラグへの参照ポインタ
 * @return なし
 */
static void display_slay_info(player_type *creature_ptr, void(*display_player_equippy)(player_type*, TERM_LEN, TERM_LEN, BIT_FLAGS16), all_player_flags *f)
{
	TERM_LEN row = 3;
	TERM_LEN col = 1;
	(*display_player_equippy)(creature_ptr, row - 2, col + 12, DP_WP);
	c_put_str(TERM_WHITE, "ab@", row - 1, col + 12);

#ifdef JP
	display_one_characteristic_info(creature_ptr, row + 0, col, "邪悪 倍打 :", TR_SLAY_EVIL, f, DP_WP);
	display_one_characteristic_info(creature_ptr, row + 0, col, "邪悪 倍打 :", TR_KILL_EVIL, f, (DP_WP | DP_IMM));
	display_one_characteristic_info(creature_ptr, row + 1, col, "不死 倍打 :", TR_SLAY_UNDEAD, f, DP_WP);
	display_one_characteristic_info(creature_ptr, row + 1, col, "不死 倍打 :", TR_KILL_UNDEAD, f, (DP_WP | DP_IMM));
	display_one_characteristic_info(creature_ptr, row + 2, col, "悪魔 倍打 :", TR_SLAY_DEMON, f, DP_WP);
	display_one_characteristic_info(creature_ptr, row + 2, col, "悪魔 倍打 :", TR_KILL_DEMON, f, (DP_WP | DP_IMM));
	display_one_characteristic_info(creature_ptr, row + 3, col, "龍 倍打   :", TR_SLAY_DRAGON, f, DP_WP);
	display_one_characteristic_info(creature_ptr, row + 3, col, "龍 倍打   :", TR_KILL_DRAGON, f, (DP_WP | DP_IMM));
	display_one_characteristic_info(creature_ptr, row + 4, col, "人間 倍打 :", TR_SLAY_HUMAN, f, DP_WP);
	display_one_characteristic_info(creature_ptr, row + 4, col, "人間 倍打 :", TR_KILL_HUMAN, f, (DP_WP | DP_IMM));
	display_one_characteristic_info(creature_ptr, row + 5, col, "動物 倍打 :", TR_SLAY_ANIMAL, f, DP_WP);
	display_one_characteristic_info(creature_ptr, row + 5, col, "動物 倍打 :", TR_KILL_ANIMAL, f, (DP_WP | DP_IMM));
	display_one_characteristic_info(creature_ptr, row + 6, col, "オーク倍打:", TR_SLAY_ORC, f, DP_WP);
	display_one_characteristic_info(creature_ptr, row + 6, col, "オーク倍打:", TR_KILL_ORC, f, (DP_WP | DP_IMM));
	display_one_characteristic_info(creature_ptr, row + 7, col, "トロル倍打:", TR_SLAY_TROLL, f, DP_WP);
	display_one_characteristic_info(creature_ptr, row + 7, col, "トロル倍打:", TR_KILL_TROLL, f, (DP_WP | DP_IMM));
	display_one_characteristic_info(creature_ptr, row + 8, col, "巨人 倍打 :", TR_SLAY_GIANT, f, DP_WP);
	display_one_characteristic_info(creature_ptr, row + 8, col, "巨人 倍打 :", TR_KILL_GIANT, f, (DP_WP | DP_IMM));
	display_one_characteristic_info(creature_ptr, row + 9, col, "溶解      :", TR_BRAND_ACID, f, DP_WP);
	display_one_characteristic_info(creature_ptr, row + 10, col, "電撃      :", TR_BRAND_ELEC, f, DP_WP);
	display_one_characteristic_info(creature_ptr, row + 11, col, "焼棄      :", TR_BRAND_FIRE, f, DP_WP);
	display_one_characteristic_info(creature_ptr, row + 12, col, "凍結      :", TR_BRAND_COLD, f, DP_WP);
	display_one_characteristic_info(creature_ptr, row + 13, col, "毒殺      :", TR_BRAND_POIS, f, DP_WP);
	display_one_characteristic_info(creature_ptr, row + 14, col, "切れ味    :", TR_VORPAL, f, DP_WP);
	display_one_characteristic_info(creature_ptr, row + 15, col, "地震      :", TR_IMPACT, f, DP_WP);
	display_one_characteristic_info(creature_ptr, row + 16, col, "吸血      :", TR_VAMPIRIC, f, DP_WP);
	display_one_characteristic_info(creature_ptr, row + 17, col, "カオス効果:", TR_CHAOTIC, f, DP_WP);
	display_one_characteristic_info(creature_ptr, row + 18, col, "理力      :", TR_FORCE_WEAPON, f, DP_WP);
#else
	display_flag_aux(creature_ptr, row + 0, col, "Slay Evil :", TR_SLAY_EVIL, f, DP_WP);
	display_flag_aux(creature_ptr, row + 0, col, "Slay Evil :", TR_KILL_EVIL, f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 1, col, "Slay Und. :", TR_SLAY_UNDEAD, f, DP_WP);
	display_flag_aux(creature_ptr, row + 1, col, "Slay Und. :", TR_KILL_UNDEAD, f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 2, col, "Slay Demon:", TR_SLAY_DEMON, f, DP_WP);
	display_flag_aux(creature_ptr, row + 2, col, "Slay Demon:", TR_KILL_DEMON, f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 3, col, "Slay Drag.:", TR_SLAY_DRAGON, f, DP_WP);
	display_flag_aux(creature_ptr, row + 3, col, "Slay Drag.:", TR_KILL_DRAGON, f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 4, col, "Slay Human:", TR_SLAY_HUMAN, f, DP_WP);
	display_flag_aux(creature_ptr, row + 4, col, "Slay Human:", TR_KILL_HUMAN, f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 5, col, "Slay Anim.:", TR_SLAY_ANIMAL, f, DP_WP);
	display_flag_aux(creature_ptr, row + 5, col, "Slay Anim.:", TR_KILL_ANIMAL, f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 6, col, "Slay Orc  :", TR_SLAY_ORC, f, DP_WP);
	display_flag_aux(creature_ptr, row + 6, col, "Slay Orc  :", TR_KILL_ORC, f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 7, col, "Slay Troll:", TR_SLAY_TROLL, f, DP_WP);
	display_flag_aux(creature_ptr, row + 7, col, "Slay Troll:", TR_KILL_TROLL, f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 8, col, "Slay Giant:", TR_SLAY_GIANT, f, DP_WP);
	display_flag_aux(creature_ptr, row + 8, col, "Slay Giant:", TR_KILL_GIANT, f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 9, col, "Acid Brand:", TR_BRAND_ACID, f, DP_WP);
	display_flag_aux(creature_ptr, row + 10, col, "Elec Brand:", TR_BRAND_ELEC, f, DP_WP);
	display_flag_aux(creature_ptr, row + 11, col, "Fire Brand:", TR_BRAND_FIRE, f, DP_WP);
	display_flag_aux(creature_ptr, row + 12, col, "Cold Brand:", TR_BRAND_COLD, f, DP_WP);
	display_flag_aux(creature_ptr, row + 13, col, "Poison Brd:", TR_BRAND_POIS, f, DP_WP);
	display_flag_aux(creature_ptr, row + 14, col, "Sharpness :", TR_VORPAL, f, DP_WP);
	display_flag_aux(creature_ptr, row + 15, col, "Quake     :", TR_IMPACT, f, DP_WP);
	display_flag_aux(creature_ptr, row + 16, col, "Vampiric  :", TR_VAMPIRIC, f, DP_WP);
	display_flag_aux(creature_ptr, row + 17, col, "Chaotic   :", TR_CHAOTIC, f, DP_WP);
	display_flag_aux(creature_ptr, row + 18, col, "Force Wep.:", TR_FORCE_WEAPON, f, DP_WP);
#endif
}


/*!
 * @brief ESP/能力維持の特性フラグを表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param display_player_equippy 表示へのコールバック
 * @param f 特性フラグへの参照ポインタ
 * @return なし
 */
static void display_esp_sustenance_info(player_type *creature_ptr, void(*display_player_equippy)(player_type*, TERM_LEN, TERM_LEN, BIT_FLAGS16), all_player_flags *f)
{
	TERM_LEN row = 3;
	TERM_LEN col = 1 + 12 + 7; // 20
	(*display_player_equippy)(creature_ptr, row - 2, col + 13, 0);
	c_put_str(TERM_WHITE, "abcdefghijkl@", row - 1, col + 13);

#ifdef JP
	display_one_characteristic_info(creature_ptr, row + 0, col, "テレパシー :", TR_TELEPATHY, f, 0);
	display_one_characteristic_info(creature_ptr, row + 1, col, "邪悪ESP    :", TR_ESP_EVIL, f, 0);
	display_one_characteristic_info(creature_ptr, row + 2, col, "無生物ESP  :", TR_ESP_NONLIVING, f, 0);
	display_one_characteristic_info(creature_ptr, row + 3, col, "善良ESP    :", TR_ESP_GOOD, f, 0);
	display_one_characteristic_info(creature_ptr, row + 4, col, "不死ESP    :", TR_ESP_UNDEAD, f, 0);
	display_one_characteristic_info(creature_ptr, row + 5, col, "悪魔ESP    :", TR_ESP_DEMON, f, 0);
	display_one_characteristic_info(creature_ptr, row + 6, col, "龍ESP      :", TR_ESP_DRAGON, f, 0);
	display_one_characteristic_info(creature_ptr, row + 7, col, "人間ESP    :", TR_ESP_HUMAN, f, 0);
	display_one_characteristic_info(creature_ptr, row + 8, col, "動物ESP    :", TR_ESP_ANIMAL, f, 0);
	display_one_characteristic_info(creature_ptr, row + 9, col, "オークESP  :", TR_ESP_ORC, f, 0);
	display_one_characteristic_info(creature_ptr, row + 10, col, "トロルESP  :", TR_ESP_TROLL, f, 0);
	display_one_characteristic_info(creature_ptr, row + 11, col, "巨人ESP    :", TR_ESP_GIANT, f, 0);
	display_one_characteristic_info(creature_ptr, row + 12, col, "ユニークESP:", TR_ESP_UNIQUE, f, 0);
	display_one_characteristic_info(creature_ptr, row + 13, col, "腕力維持   :", TR_SUST_STR, f, 0);
	display_one_characteristic_info(creature_ptr, row + 14, col, "知力維持   :", TR_SUST_INT, f, 0);
	display_one_characteristic_info(creature_ptr, row + 15, col, "賢さ維持   :", TR_SUST_WIS, f, 0);
	display_one_characteristic_info(creature_ptr, row + 16, col, "器用維持   :", TR_SUST_DEX, f, 0);
	display_one_characteristic_info(creature_ptr, row + 17, col, "耐久維持   :", TR_SUST_CON, f, 0);
	display_one_characteristic_info(creature_ptr, row + 18, col, "魅力維持   :", TR_SUST_CHR, f, 0);
#else
	display_flag_aux(creature_ptr, row + 0, col, "Telepathy  :", TR_TELEPATHY, f, 0);
	display_flag_aux(creature_ptr, row + 1, col, "ESP Evil   :", TR_ESP_EVIL, f, 0);
	display_flag_aux(creature_ptr, row + 2, col, "ESP Noliv. :", TR_ESP_NONLIVING, f, 0);
	display_flag_aux(creature_ptr, row + 3, col, "ESP Good   :", TR_ESP_GOOD, f, 0);
	display_flag_aux(creature_ptr, row + 4, col, "ESP Undead :", TR_ESP_UNDEAD, f, 0);
	display_flag_aux(creature_ptr, row + 5, col, "ESP Demon  :", TR_ESP_DEMON, f, 0);
	display_flag_aux(creature_ptr, row + 6, col, "ESP Dragon :", TR_ESP_DRAGON, f, 0);
	display_flag_aux(creature_ptr, row + 7, col, "ESP Human  :", TR_ESP_HUMAN, f, 0);
	display_flag_aux(creature_ptr, row + 8, col, "ESP Animal :", TR_ESP_ANIMAL, f, 0);
	display_flag_aux(creature_ptr, row + 9, col, "ESP Orc    :", TR_ESP_ORC, f, 0);
	display_flag_aux(creature_ptr, row + 10, col, "ESP Troll  :", TR_ESP_TROLL, f, 0);
	display_flag_aux(creature_ptr, row + 11, col, "ESP Giant  :", TR_ESP_GIANT, f, 0);
	display_flag_aux(creature_ptr, row + 12, col, "ESP Unique :", TR_ESP_UNIQUE, f, 0);
	display_flag_aux(creature_ptr, row + 13, col, "Sust Str   :", TR_SUST_STR, f, 0);
	display_flag_aux(creature_ptr, row + 14, col, "Sust Int   :", TR_SUST_INT, f, 0);
	display_flag_aux(creature_ptr, row + 15, col, "Sust Wis   :", TR_SUST_WIS, f, 0);
	display_flag_aux(creature_ptr, row + 16, col, "Sust Dex   :", TR_SUST_DEX, f, 0);
	display_flag_aux(creature_ptr, row + 17, col, "Sust Con   :", TR_SUST_CON, f, 0);
	display_flag_aux(creature_ptr, row + 18, col, "Sust Chr   :", TR_SUST_CHR, f, 0);
#endif
}


/*!
 * @brief その他の特性フラグを表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param display_player_equippy 表示へのコールバック
 * @param f 特性フラグへの参照ポインタ
 * @return なし
 */
static void display_other_info(player_type *creature_ptr, void(*display_player_equippy)(player_type*, TERM_LEN, TERM_LEN, BIT_FLAGS16), all_player_flags *f)
{
	TERM_LEN row = 3;
	TERM_LEN col = 20 + 12 + 17;
	(*display_player_equippy)(creature_ptr, row - 2, col + 14, 0);
	c_put_str(TERM_WHITE, "abcdefghijkl@", row - 1, col + 14);

#ifdef JP
	display_one_characteristic_info(creature_ptr, row + 0, col, "追加攻撃    :", TR_BLOWS, f, 0);
	display_one_characteristic_info(creature_ptr, row + 1, col, "採掘        :", TR_TUNNEL, f, 0);
	display_one_characteristic_info(creature_ptr, row + 2, col, "赤外線視力  :", TR_INFRA, f, 0);
	display_one_characteristic_info(creature_ptr, row + 3, col, "魔法道具支配:", TR_MAGIC_MASTERY, f, 0);
	display_one_characteristic_info(creature_ptr, row + 4, col, "隠密        :", TR_STEALTH, f, 0);
	display_one_characteristic_info(creature_ptr, row + 5, col, "探索        :", TR_SEARCH, f, 0);

	display_one_characteristic_info(creature_ptr, row + 7, col, "乗馬        :", TR_RIDING, f, 0);
	display_one_characteristic_info(creature_ptr, row + 8, col, "投擲        :", TR_THROW, f, 0);
	display_one_characteristic_info(creature_ptr, row + 9, col, "祝福        :", TR_BLESSED, f, 0);
	display_one_characteristic_info(creature_ptr, row + 10, col, "反テレポート:", TR_NO_TELE, f, 0);
	display_one_characteristic_info(creature_ptr, row + 11, col, "反魔法      :", TR_NO_MAGIC, f, 0);
	display_one_characteristic_info(creature_ptr, row + 12, col, "消費魔力減少:", TR_DEC_MANA, f, 0);

	display_one_characteristic_info(creature_ptr, row + 14, col, "経験値減少  :", TR_DRAIN_EXP, f, 0);
	display_one_characteristic_info(creature_ptr, row + 15, col, "乱テレポート:", TR_TELEPORT, f, 0);
	display_one_characteristic_info(creature_ptr, row + 16, col, "反感        :", TR_AGGRAVATE, f, 0);
	display_one_characteristic_info(creature_ptr, row + 17, col, "太古の怨念  :", TR_TY_CURSE, f, 0);
#else
	display_flag_aux(creature_ptr, row + 0, col, "Add Blows   :", TR_BLOWS, f, 0);
	display_flag_aux(creature_ptr, row + 1, col, "Add Tunnel  :", TR_TUNNEL, f, 0);
	display_flag_aux(creature_ptr, row + 2, col, "Add Infra   :", TR_INFRA, f, 0);
	display_flag_aux(creature_ptr, row + 3, col, "Add Device  :", TR_MAGIC_MASTERY, f, 0);
	display_flag_aux(creature_ptr, row + 4, col, "Add Stealth :", TR_STEALTH, f, 0);
	display_flag_aux(creature_ptr, row + 5, col, "Add Search  :", TR_SEARCH, f, 0);

	display_flag_aux(creature_ptr, row + 7, col, "Riding      :", TR_RIDING, f, 0);
	display_flag_aux(creature_ptr, row + 8, col, "Throw       :", TR_THROW, f, 0);
	display_flag_aux(creature_ptr, row + 9, col, "Blessed     :", TR_BLESSED, f, 0);
	display_flag_aux(creature_ptr, row + 10, col, "No Teleport :", TR_NO_TELE, f, 0);
	display_flag_aux(creature_ptr, row + 11, col, "Anti Magic  :", TR_NO_MAGIC, f, 0);
	display_flag_aux(creature_ptr, row + 12, col, "Econom. Mana:", TR_DEC_MANA, f, 0);

	display_flag_aux(creature_ptr, row + 14, col, "Drain Exp   :", TR_DRAIN_EXP, f, 0);
	display_flag_aux(creature_ptr, row + 15, col, "Rnd.Teleport:", TR_TELEPORT, f, 0);
	display_flag_aux(creature_ptr, row + 16, col, "Aggravate   :", TR_AGGRAVATE, f, 0);
	display_flag_aux(creature_ptr, row + 17, col, "TY Curse    :", TR_TY_CURSE, f, 0);
#endif
}


/*!
 * @brief プレイヤーの特性フラグ一覧表示2
 * @param creature_ptr プレーヤーへの参照ポインタ
 * Special display, part 2
 * @return なし
 */
void display_player_flag_info_2(player_type *creature_ptr, void(*display_player_equippy)(player_type*, TERM_LEN, TERM_LEN, BIT_FLAGS16))
{
	/* Extract flags and store */
	all_player_flags f;
	player_flags(creature_ptr, f.player_flags);
	tim_player_flags(creature_ptr, f.tim_player_flags);
	player_immunity(creature_ptr, f.player_imm);
	tim_player_immunity(creature_ptr, f.tim_player_imm);
	known_obj_immunity(creature_ptr, f.known_obj_imm);
	player_vulnerability_flags(creature_ptr, f.player_vuln);

	display_slay_info(creature_ptr, display_player_equippy, &f);
	display_esp_sustenance_info(creature_ptr, display_player_equippy, &f);
	display_other_info(creature_ptr, display_player_equippy, &f);
}
