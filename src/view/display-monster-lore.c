#include "view/display-monster-lore.h"
#include "monster-lore/monster-lore.h"
#include "term/term-color-types.h"
#include "world/world.h"

/*!
 * @brief モンスター情報のヘッダを記述する
 * Hack -- Display the "name" and "attr/chars" of a monster race
 * @param r_idx モンスターの種族ID
 * @return なし
 */
void roff_top(MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    char c1 = r_ptr->d_char;
    char c2 = r_ptr->x_char;

    TERM_COLOR a1 = r_ptr->d_attr;
    TERM_COLOR a2 = r_ptr->x_attr;

    Term_erase(0, 0, 255);
    Term_gotoxy(0, 0);

#ifdef JP
#else
    if (!(r_ptr->flags1 & RF1_UNIQUE)) {
        Term_addstr(-1, TERM_WHITE, "The ");
    }
#endif

    Term_addstr(-1, TERM_WHITE, (r_name + r_ptr->name));

    Term_addstr(-1, TERM_WHITE, " ('");
    Term_add_bigch(a1, c1);
    Term_addstr(-1, TERM_WHITE, "')");

    Term_addstr(-1, TERM_WHITE, "/('");
    Term_add_bigch(a2, c2);
    Term_addstr(-1, TERM_WHITE, "'):");

    if (!current_world_ptr->wizard)
        return;

    char buf[16];
    sprintf(buf, "%d", r_idx);
    Term_addstr(-1, TERM_WHITE, " (");
    Term_addstr(-1, TERM_L_BLUE, buf);
    Term_addch(TERM_WHITE, ')');
}

/*!
 * @brief  モンスター情報の表示と共に画面を一時消去するサブルーチン /
 * Hack -- describe the given monster race at the top of the screen
 * @param r_idx モンスターの種族ID
 * @param mode 表示オプション
 * @return なし
 */
void screen_roff(player_type *player_ptr, MONRACE_IDX r_idx, BIT_FLAGS mode)
{
    msg_erase();
    Term_erase(0, 1, 255);
    hook_c_roff = c_roff;
    process_monster_lore(player_ptr, r_idx, mode);
    roff_top(r_idx);
}

/*!
 * @brief モンスター情報の現在のウィンドウに表示する /
 * Hack -- describe the given monster race in the current "term" window
 * @param r_idx モンスターの種族ID
 * @return なし
 */
void display_roff(player_type *player_ptr)
{
    for (int y = 0; y < Term->hgt; y++) {
        Term_erase(0, y, 255);
    }

    Term_gotoxy(0, 1);
    hook_c_roff = c_roff;
    MONRACE_IDX r_idx = player_ptr->monster_race_idx;
    process_monster_lore(player_ptr, r_idx, 0);
    roff_top(r_idx);
}

/*!
 * @brief モンスター詳細情報を自動スポイラー向けに出力する /
 * Hack -- output description of the given monster race
 * @param r_idx モンスターの種族ID
 * @param roff_func 出力処理を行う関数ポインタ
 * @return なし
 */
void output_monster_spoiler(player_type *player_ptr, MONRACE_IDX r_idx, void (*roff_func)(TERM_COLOR attr, concptr str))
{
    hook_c_roff = roff_func;
    process_monster_lore(player_ptr, r_idx, 0x03);
}
