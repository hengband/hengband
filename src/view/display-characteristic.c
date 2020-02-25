#include "display-characteristic.h"
#include "term.h"

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
void display_one_characteristic_info(player_type *creature_ptr, TERM_LEN row, TERM_LEN col, concptr header, int flag1, all_player_flags *f, u16b mode)
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
