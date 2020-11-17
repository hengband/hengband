/*!
 * @brief キャラクタの特性を表示する
 * @date 2020/02/25
 * @author Hourier
 */

#include "display-characteristic.h"
#include "flavor/flavor-util.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object/object-flags.h"
#include "player/permanent-resistances.h"
#include "player/race-resistances.h"
#include "player/temporary-resistances.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"

typedef struct {
	BIT_FLAGS player_flags[TR_FLAG_SIZE];
	BIT_FLAGS tim_player_flags[TR_FLAG_SIZE];
	BIT_FLAGS player_imm[TR_FLAG_SIZE];
	BIT_FLAGS tim_player_imm[TR_FLAG_SIZE];
	BIT_FLAGS player_vuln[TR_FLAG_SIZE];
	BIT_FLAGS known_obj_imm[TR_FLAG_SIZE];
} all_player_flags;


/*!
 * @brief 呪われた装備の表示色を変更する
 * @param mode 表示オプション
 * @param row 行数
 * @param col 列数
 * @param flags 装備品へのフラグ群
 * @param header_color 耐性等のパラメータ名 の色
 * @param o_ptr 装備品への参照ポインタ
 * @return 与えられた装備が呪われていればTRUE
 */
static bool decide_cursed_equipment_color(u16b mode, TERM_LEN row, TERM_LEN *col, BIT_FLAGS *flags, byte *header_color, object_type *o_ptr)
{
	if ((mode & DP_CURSE) == 0) return FALSE;

	if (has_flag(flags, TR_ADD_L_CURSE) || has_flag(flags, TR_ADD_H_CURSE))
	{
		c_put_str(TERM_L_DARK, "+", row, *col);
		*header_color = TERM_WHITE;
	}

	if (o_ptr->curse_flags & (TRC_CURSED | TRC_HEAVY_CURSE))
	{
		c_put_str(TERM_WHITE, "+", row, *col);
		*header_color = TERM_WHITE;
	}

	if (o_ptr->curse_flags & TRC_PERMA_CURSE)
	{
		c_put_str(TERM_WHITE, "*", row, *col);
		*header_color = TERM_WHITE;
	}

	(*col)++;
	return TRUE;
}


/*!
 * @brief 装備品の光源範囲増加/減少で表示色を変える
 * @param row 行数
 * @param col 列数
 * @param flag1 参照する特性ID
 * @param flags 装備品へのフラグ群
 * @param header_color 耐性等のパラメータ名 の色
 * @return 装備品が光源範囲に影響を及ぼすならばTRUE、そうでないならFALSE
 */
static bool decide_light_equipment_color(TERM_LEN row, TERM_LEN *col, int flag1, BIT_FLAGS *flags, byte *header_color)
{
	if (flag1 != TR_LITE_1) return FALSE;

	if (has_dark_flag(flags))
	{
		c_put_str(TERM_L_DARK, "+", row, *col);
		*header_color = TERM_WHITE;
	}
	else if (has_lite_flag(flags))
	{
		c_put_str(TERM_WHITE, "+", row, *col);
		*header_color = TERM_WHITE;
	}

	(*col)++;
	return TRUE;
}


/*!
 * @brief プレーヤーの弱点に応じて表示色を変える
 * @param mode 表示オプション
 * @param row 行数
 * @param col 列数
 * @param flag1 参照する特性ID
 * @param flags 装備品へのフラグ群
 * @param header_color 耐性等のパラメータ名 の色
 * @param vuln プレーヤーの弱点
 * @return なし
 */
static void decide_vulnerability_color(u16b mode, TERM_LEN row, TERM_LEN *col, int flag1, BIT_FLAGS *flags, byte *header_color, bool vuln)
{
	if (has_flag(flags, flag1))
	{
		c_put_str((byte)(vuln ? TERM_L_RED : TERM_WHITE),
			(mode & DP_IMM) ? "*" : "+", row, *col);
		*header_color = TERM_WHITE;
	}

	(*col)++;
}


/*!
 * @brief 装備品を走査し、状況に応じて耐性等の表示色を変える
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param mode 表示オプション
 * @param row 行数
 * @param col 列数
 * @param flag1 参照する特性ID
 * @param header_color 耐性等のパラメータ名 の色
 * @param vuln プレーヤーの弱点
 * @return なし
 * @details
 * max_i changes only when weapon flags need only two column
 */
static void decide_colors(player_type *creature_ptr, u16b mode, TERM_LEN row, TERM_LEN *col, int flag1, byte *header_color, bool vuln)
{
	int max_i = (mode & DP_WP) ? INVEN_LARM + 1 : INVEN_TOTAL;
	for (inventory_slot_type i = INVEN_RARM; i < max_i; i++)
	{
		BIT_FLAGS flags[TR_FLAG_SIZE];
		object_type *o_ptr;
		o_ptr = &creature_ptr->inventory_list[i];
                object_flags_known(creature_ptr, o_ptr, flags);
		if (!(mode & DP_IMM))
			c_put_str((byte)(vuln ? TERM_RED : TERM_SLATE), ".", row, *col);

		if (decide_cursed_equipment_color(mode, row, col, flags, header_color, o_ptr)) continue;
		if (decide_light_equipment_color(row, col, flag1, flags, header_color)) continue;

		decide_vulnerability_color(mode, row, col, flag1, flags, header_color, vuln);
	}
}


/*!
 * @brief プレイヤーの特性フラグ一種を表示する
 * @param row コンソール表示位置の左上行
 * @param col コンソール表示位置の左上列
 * @param header コンソール上で表示する特性名
 * @param header_color 耐性等のパラメータ名 の色
 * @param header_col 「耐性等のパラメータ名 の色」の元々の位置
 * @param flag1 参照する特性ID
 * @param vuln プレーヤーの弱点
 * @param f プレイヤーの特性情報構造体
 * @return なし
 */
static void display_one_characteristic(TERM_LEN row, TERM_LEN col, concptr header, byte header_color, int header_col, int flag1, bool vuln, all_player_flags *f)
{
	c_put_str((byte)(vuln ? TERM_RED : TERM_SLATE), ".", row, col);
	if (has_flag(f->player_flags, flag1))
	{
		c_put_str((byte)(vuln ? TERM_L_RED : TERM_WHITE), "+", row, col);
		header_color = TERM_WHITE;
	}

	if (has_flag(f->tim_player_flags, flag1))
	{
		c_put_str((byte)(vuln ? TERM_ORANGE : TERM_YELLOW), "#", row, col);
		header_color = TERM_WHITE;
	}

	if (has_flag(f->tim_player_imm, flag1))
	{
		c_put_str(TERM_YELLOW, "*", row, col);
		header_color = TERM_WHITE;
	}

	if (has_flag(f->player_imm, flag1))
	{
		c_put_str(TERM_WHITE, "*", row, col);
		header_color = TERM_WHITE;
	}

	if (vuln)
		c_put_str(TERM_RED, "v", row, col + 1);

	c_put_str(header_color, header, row, header_col);
}


/*!
 * @brief プレイヤーの特性フラグ一種表示を処理するメインルーチン
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param row コンソール表示位置の左上行
 * @param col コンソール表示位置の左上列
 * @param header コンソール上で表示する特性名
 * @param flag1 参照する特性ID
 * @param f プレイヤーの特性情報構造体
 * @param mode 表示オプション
 * @return なし
 */
static void process_one_characteristic(player_type *creature_ptr, TERM_LEN row, TERM_LEN col, concptr header, int flag1, all_player_flags *f, u16b mode)
{
	byte header_color = TERM_L_DARK;
	int header_col = col;
	bool vuln = FALSE;
	if (has_flag(f->player_vuln, flag1) &&
		!(has_flag(f->known_obj_imm, flag1) || has_flag(f->player_imm, flag1) || has_flag(f->tim_player_imm, flag1)))
		vuln = TRUE;

	col += strlen(header) + 1;
	decide_colors(creature_ptr, mode, row, &col, flag1, &header_color, vuln);
	if (mode & DP_IMM)
	{
		if (header_color != TERM_L_DARK)
		{
			c_put_str(header_color, header, row, header_col);
		}

		return;
	}

	display_one_characteristic(row, col, header, header_color, header_col, flag1, vuln, f);
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

	process_one_characteristic(creature_ptr, row + 0, col, _("耐酸  :", "Acid  :"), TR_RES_ACID, f, 0);
	process_one_characteristic(creature_ptr, row + 0, col, _("耐酸  :", "Acid  :"), TR_IM_ACID, f, DP_IMM);
	process_one_characteristic(creature_ptr, row + 1, col, _("耐電撃:", "Elec  :"), TR_RES_ELEC, f, 0);
	process_one_characteristic(creature_ptr, row + 1, col, _("耐電撃:", "Elec  :"), TR_IM_ELEC, f, DP_IMM);
	process_one_characteristic(creature_ptr, row + 2, col, _("耐火炎:", "Fire  :"), TR_RES_FIRE, f, 0);
	process_one_characteristic(creature_ptr, row + 2, col, _("耐火炎:", "Fire  :"), TR_IM_FIRE, f, DP_IMM);
	process_one_characteristic(creature_ptr, row + 3, col, _("耐冷気:", "Cold  :"), TR_RES_COLD, f, 0);
	process_one_characteristic(creature_ptr, row + 3, col, _("耐冷気:", "Cold  :"), TR_IM_COLD, f, DP_IMM);
	process_one_characteristic(creature_ptr, row + 4, col, _("耐毒  :", "Poison:"), TR_RES_POIS, f, 0);
	process_one_characteristic(creature_ptr, row + 5, col, _("耐閃光:", "Light :"), TR_RES_LITE, f, 0);
	process_one_characteristic(creature_ptr, row + 6, col, _("耐暗黒:", "Dark  :"), TR_RES_DARK, f, 0);
	process_one_characteristic(creature_ptr, row + 7, col, _("耐破片:", "Shard :"), TR_RES_SHARDS, f, 0);
	process_one_characteristic(creature_ptr, row + 8, col, _("耐盲目:", "Blind :"), TR_RES_BLIND, f, 0);
	process_one_characteristic(creature_ptr, row + 9, col, _("耐混乱:", "Conf  :"), TR_RES_CONF, f, 0);
}


/*!
 * @brief プレーヤーの上位耐性を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param display_player_equippy 表示へのコールバック
 * @param f 特性フラグへの参照ポインタ
 * @return なし
 */
static void display_advanced_resistance_info(player_type *creature_ptr, void(*display_player_equippy)(player_type*, TERM_LEN, TERM_LEN, BIT_FLAGS16), all_player_flags *f)
{
	TERM_LEN row = 12;
	TERM_LEN col = 26;
	(*display_player_equippy)(creature_ptr, row - 2, col + 8, 0);
	c_put_str(TERM_WHITE, "abcdefghijkl@", row - 1, col + 8);

	process_one_characteristic(creature_ptr, row + 0, col, _("耐轟音:", "Sound :"), TR_RES_SOUND, f, 0);
	process_one_characteristic(creature_ptr, row + 1, col, _("耐地獄:", "Nether:"), TR_RES_NETHER, f, 0);
	process_one_characteristic(creature_ptr, row + 2, col, _("耐因混:", "Nexus :"), TR_RES_NEXUS, f, 0);
	process_one_characteristic(creature_ptr, row + 3, col, _("耐カオ:", "Chaos :"), TR_RES_CHAOS, f, 0);
	process_one_characteristic(creature_ptr, row + 4, col, _("耐劣化:", "Disnch:"), TR_RES_DISEN, f, 0);
	process_one_characteristic(creature_ptr, row + 5, col, _("耐恐怖:", "Fear  :"), TR_RES_FEAR, f, 0);
	process_one_characteristic(creature_ptr, row + 6, col, _("反射  :", "Reflct:"), TR_REFLECT, f, 0);
	process_one_characteristic(creature_ptr, row + 7, col, _("火炎オ:", "AuFire:"), TR_SH_FIRE, f, 0);
	process_one_characteristic(creature_ptr, row + 8, col, _("電気オ:", "AuElec:"), TR_SH_ELEC, f, 0);
	process_one_characteristic(creature_ptr, row + 9, col, _("冷気オ:", "AuCold:"), TR_SH_COLD, f, 0);
}


/*!
 * @brief プレーヤーのその他耐性を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param display_player_equippy 表示へのコールバック
 * @param f 特性フラグへの参照ポインタ
 * @return なし
 */
static void display_other_resistance_info(player_type *creature_ptr, void(*display_player_equippy)(player_type*, TERM_LEN, TERM_LEN, BIT_FLAGS16), all_player_flags *f)
{
	TERM_LEN row = 12;
	TERM_LEN col = 51;
	(*display_player_equippy)(creature_ptr, row - 2, col + 12, 0);
	c_put_str(TERM_WHITE, "abcdefghijkl@", row - 1, col + 12);

	process_one_characteristic(creature_ptr, row + 0, col, _("加速      :", "Speed     :"), TR_SPEED, f, 0);
	process_one_characteristic(creature_ptr, row + 1, col, _("耐麻痺    :", "FreeAction:"), TR_FREE_ACT, f, 0);
	process_one_characteristic(creature_ptr, row + 2, col, _("透明体視認:", "SeeInvisi.:"), TR_SEE_INVIS, f, 0);
	process_one_characteristic(creature_ptr, row + 3, col, _("経験値保持:", "Hold Exp  :"), TR_HOLD_EXP, f, 0);
	process_one_characteristic(creature_ptr, row + 4, col, _("警告      :", "Warning   :"), TR_WARNING, f, 0);
	process_one_characteristic(creature_ptr, row + 5, col, _("遅消化    :", "SlowDigest:"), TR_SLOW_DIGEST, f, 0);
	process_one_characteristic(creature_ptr, row + 6, col, _("急回復    :", "Regene.   :"), TR_REGEN, f, 0);
	process_one_characteristic(creature_ptr, row + 7, col, _("浮遊      :", "Levitation:"), TR_LEVITATION, f, 0);
	process_one_characteristic(creature_ptr, row + 8, col, _("永遠光源  :", "Perm Lite :"), TR_LITE_1, f, 0);
	process_one_characteristic(creature_ptr, row + 9, col, _("呪い      :", "Cursed    :"), 0, f, DP_CURSE);
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
	display_other_resistance_info(creature_ptr, display_player_equippy, &f);
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

	process_one_characteristic(creature_ptr, row + 0, col, _("邪悪 倍打 :", "Slay Evil :"), TR_SLAY_EVIL, f, DP_WP);
	process_one_characteristic(creature_ptr, row + 0, col, _("邪悪 倍打 :", "Slay Evil :"), TR_KILL_EVIL, f, (DP_WP | DP_IMM));
	process_one_characteristic(creature_ptr, row + 1, col, _("不死 倍打 :", "Slay Und. :"), TR_SLAY_UNDEAD, f, DP_WP);
	process_one_characteristic(creature_ptr, row + 1, col, _("不死 倍打 :", "Slay Und. :"), TR_KILL_UNDEAD, f, (DP_WP | DP_IMM));
	process_one_characteristic(creature_ptr, row + 2, col, _("悪魔 倍打 :", "Slay Demon:"), TR_SLAY_DEMON, f, DP_WP);
	process_one_characteristic(creature_ptr, row + 2, col, _("悪魔 倍打 :", "Slay Demon:"), TR_KILL_DEMON, f, (DP_WP | DP_IMM));
	process_one_characteristic(creature_ptr, row + 3, col, _("龍 倍打   :", "Slay Drag.:"), TR_SLAY_DRAGON, f, DP_WP);
	process_one_characteristic(creature_ptr, row + 3, col, _("龍 倍打   :", "Slay Drag.:"), TR_KILL_DRAGON, f, (DP_WP | DP_IMM));
	process_one_characteristic(creature_ptr, row + 4, col, _("人間 倍打 :", "Slay Human:"), TR_SLAY_HUMAN, f, DP_WP);
	process_one_characteristic(creature_ptr, row + 4, col, _("人間 倍打 :", "Slay Human:"), TR_KILL_HUMAN, f, (DP_WP | DP_IMM));
	process_one_characteristic(creature_ptr, row + 5, col, _("動物 倍打 :", "Slay Anim.:"), TR_SLAY_ANIMAL, f, DP_WP);
	process_one_characteristic(creature_ptr, row + 5, col, _("動物 倍打 :", "Slay Anim.:"), TR_KILL_ANIMAL, f, (DP_WP | DP_IMM));
	process_one_characteristic(creature_ptr, row + 6, col, _("オーク倍打:", "Slay Orc  :"), TR_SLAY_ORC, f, DP_WP);
	process_one_characteristic(creature_ptr, row + 6, col, _("オーク倍打:", "Slay Orc  :"), TR_KILL_ORC, f, (DP_WP | DP_IMM));
	process_one_characteristic(creature_ptr, row + 7, col, _("トロル倍打:", "Slay Troll:"), TR_SLAY_TROLL, f, DP_WP);
	process_one_characteristic(creature_ptr, row + 7, col, _("トロル倍打:", "Slay Troll:"), TR_KILL_TROLL, f, (DP_WP | DP_IMM));
	process_one_characteristic(creature_ptr, row + 8, col, _("巨人 倍打 :", "Slay Giant:"), TR_SLAY_GIANT, f, DP_WP);
	process_one_characteristic(creature_ptr, row + 8, col, _("巨人 倍打 :", "Slay Giant:"), TR_KILL_GIANT, f, (DP_WP | DP_IMM));
	process_one_characteristic(creature_ptr, row + 9, col, _("溶解      :", "Acid Brand:"), TR_BRAND_ACID, f, DP_WP);
	process_one_characteristic(creature_ptr, row + 10, col, _("電撃      :", "Elec Brand:"), TR_BRAND_ELEC, f, DP_WP);
	process_one_characteristic(creature_ptr, row + 11, col, _("焼棄      :", "Fire Brand:"), TR_BRAND_FIRE, f, DP_WP);
	process_one_characteristic(creature_ptr, row + 12, col, _("凍結      :", "Cold Brand:"), TR_BRAND_COLD, f, DP_WP);
	process_one_characteristic(creature_ptr, row + 13, col, _("毒殺      :", "Poison Brd:"), TR_BRAND_POIS, f, DP_WP);
	process_one_characteristic(creature_ptr, row + 14, col, _("切れ味    :", "Sharpness :"), TR_VORPAL, f, DP_WP);
	process_one_characteristic(creature_ptr, row + 15, col, _("地震      :", "Quake     :"), TR_IMPACT, f, DP_WP);
	process_one_characteristic(creature_ptr, row + 16, col, _("吸血      :", "Vampiric  :"), TR_VAMPIRIC, f, DP_WP);
	process_one_characteristic(creature_ptr, row + 17, col, _("カオス効果:", "Chaotic   :"), TR_CHAOTIC, f, DP_WP);
	process_one_characteristic(creature_ptr, row + 18, col, _("理力      :", "Force Wep.:"), TR_FORCE_WEAPON, f, DP_WP);
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

	process_one_characteristic(creature_ptr, row + 0, col, _("テレパシー :", "Telepathy  :"), TR_TELEPATHY, f, 0);
	process_one_characteristic(creature_ptr, row + 1, col, _("邪悪ESP    :", "ESP Evil   :"), TR_ESP_EVIL, f, 0);
	process_one_characteristic(creature_ptr, row + 2, col, _("無生物ESP  :", "ESP Noliv. :"), TR_ESP_NONLIVING, f, 0);
	process_one_characteristic(creature_ptr, row + 3, col, _("善良ESP    :", "ESP Good   :"), TR_ESP_GOOD, f, 0);
	process_one_characteristic(creature_ptr, row + 4, col, _("不死ESP    :", "ESP Undead :"), TR_ESP_UNDEAD, f, 0);
	process_one_characteristic(creature_ptr, row + 5, col, _("悪魔ESP    :", "ESP Demon  :"), TR_ESP_DEMON, f, 0);
	process_one_characteristic(creature_ptr, row + 6, col, _("龍ESP      :", "ESP Dragon :"), TR_ESP_DRAGON, f, 0);
	process_one_characteristic(creature_ptr, row + 7, col, _("人間ESP    :", "ESP Human  :"), TR_ESP_HUMAN, f, 0);
	process_one_characteristic(creature_ptr, row + 8, col, _("動物ESP    :", "ESP Animal :"), TR_ESP_ANIMAL, f, 0);
	process_one_characteristic(creature_ptr, row + 9, col, _("オークESP  :", "ESP Orc    :"), TR_ESP_ORC, f, 0);
	process_one_characteristic(creature_ptr, row + 10, col, _("トロルESP  :", "ESP Troll  :"), TR_ESP_TROLL, f, 0);
	process_one_characteristic(creature_ptr, row + 11, col, _("巨人ESP    :", "ESP Giant  :"), TR_ESP_GIANT, f, 0);
	process_one_characteristic(creature_ptr, row + 12, col, _("ユニークESP:", "ESP Unique :"), TR_ESP_UNIQUE, f, 0);
	process_one_characteristic(creature_ptr, row + 13, col, _("腕力維持   :", "Sust Str   :"), TR_SUST_STR, f, 0);
	process_one_characteristic(creature_ptr, row + 14, col, _("知力維持   :", "Sust Int   :"), TR_SUST_INT, f, 0);
	process_one_characteristic(creature_ptr, row + 15, col, _("賢さ維持   :", "Sust Wis   :"), TR_SUST_WIS, f, 0);
	process_one_characteristic(creature_ptr, row + 16, col, _("器用維持   :", "Sust Dex   :"), TR_SUST_DEX, f, 0);
	process_one_characteristic(creature_ptr, row + 17, col, _("耐久維持   :", "Sust Con   :"), TR_SUST_CON, f, 0);
	process_one_characteristic(creature_ptr, row + 18, col, _("魅力維持   :", "Sust Chr   :"), TR_SUST_CHR, f, 0);
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

	process_one_characteristic(creature_ptr, row + 0, col, _("追加攻撃    :", "Add Blows   :"), TR_BLOWS, f, 0);
	process_one_characteristic(creature_ptr, row + 1, col, _("採掘        :", "Add Tunnel  :"), TR_TUNNEL, f, 0);
	process_one_characteristic(creature_ptr, row + 2, col, _("赤外線視力  :", "Add Infra   :"), TR_INFRA, f, 0);
	process_one_characteristic(creature_ptr, row + 3, col, _("魔法道具支配:", "Add Device  :"), TR_MAGIC_MASTERY, f, 0);
	process_one_characteristic(creature_ptr, row + 4, col, _("隠密        :", "Add Stealth :"), TR_STEALTH, f, 0);
	process_one_characteristic(creature_ptr, row + 5, col, _("探索        :", "Add Search  :"), TR_SEARCH, f, 0);

	process_one_characteristic(creature_ptr, row + 7, col, _("乗馬        :", "Riding      :"), TR_RIDING, f, 0);
	process_one_characteristic(creature_ptr, row + 8, col, _("投擲        :", "Throw       :"), TR_THROW, f, 0);
	process_one_characteristic(creature_ptr, row + 9, col, _("祝福        :", "Blessed     :"), TR_BLESSED, f, 0);
	process_one_characteristic(creature_ptr, row + 10, col, _("反テレポート:", "No Teleport :"), TR_NO_TELE, f, 0);
	process_one_characteristic(creature_ptr, row + 11, col, _("反魔法      :", "Anti Magic  :"), TR_NO_MAGIC, f, 0);
	process_one_characteristic(creature_ptr, row + 12, col, _("消費魔力減少:", "Econom. Mana:"), TR_DEC_MANA, f, 0);

	process_one_characteristic(creature_ptr, row + 14, col, _("経験値減少  :", "Drain Exp   :"), TR_DRAIN_EXP, f, 0);
	process_one_characteristic(creature_ptr, row + 15, col, _("乱テレポート:", "Rnd.Teleport:"), TR_TELEPORT, f, 0);
	process_one_characteristic(creature_ptr, row + 16, col, _("反感        :", "Aggravate   :"), TR_AGGRAVATE, f, 0);
	process_one_characteristic(creature_ptr, row + 17, col, _("太古の怨念  :", "TY Curse    :"), TR_TY_CURSE, f, 0);
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
