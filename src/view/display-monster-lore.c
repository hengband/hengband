#include "view/display-monster-lore.h"
#include "lore/monster-lore.h"
#include "monster-race/race-flags1.h"
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

void display_kill_numbers(lore_type *lore_ptr)
{
    if ((lore_ptr->mode & 0x02) != 0)
        return;

    if (lore_ptr->flags1 & RF1_UNIQUE) {
        bool dead = (lore_ptr->r_ptr->max_num == 0) ? TRUE : FALSE;
        if (lore_ptr->r_ptr->r_deaths) {
            hooked_roff(
                format(_("%^sはあなたの先祖を %d 人葬っている", "%^s has slain %d of your ancestors"), wd_he[lore_ptr->msex], lore_ptr->r_ptr->r_deaths));

            if (dead) {
                hooked_roff(
                    _(format("が、すでに仇討ちは果たしている！"), format(", but you have avenged %s!  ", plural(lore_ptr->r_ptr->r_deaths, "him", "them"))));
            } else {
                hooked_roff(
                    _(format("のに、まだ仇討ちを果たしていない。"), format(", who %s unavenged.  ", plural(lore_ptr->r_ptr->r_deaths, "remains", "remain"))));
            }

            hooked_roff("\n");
        } else if (dead) {
            hooked_roff(_("あなたはこの仇敵をすでに葬り去っている。", "You have slain this foe.  "));
            hooked_roff("\n");
        }

        return;
    }

    if (lore_ptr->r_ptr->r_deaths) {
        hooked_roff(_(format("このモンスターはあなたの先祖を %d 人葬っている", lore_ptr->r_ptr->r_deaths),
            format("%d of your ancestors %s been killed by this creature, ", lore_ptr->r_ptr->r_deaths, plural(lore_ptr->r_ptr->r_deaths, "has", "have"))));

        if (lore_ptr->r_ptr->r_pkills) {
            hooked_roff(format(_("が、あなたはこのモンスターを少なくとも %d 体は倒している。", "and you have exterminated at least %d of the creatures.  "),
                lore_ptr->r_ptr->r_pkills));
        } else if (lore_ptr->r_ptr->r_tkills) {
            hooked_roff(format(
                _("が、あなたの先祖はこのモンスターを少なくとも %d 体は倒している。", "and your ancestors have exterminated at least %d of the creatures.  "),
                lore_ptr->r_ptr->r_tkills));
        } else {
            hooked_roff(format(_("が、まだ%sを倒したことはない。", "and %s is not ever known to have been defeated.  "), wd_he[lore_ptr->msex]));
        }

        hooked_roff("\n");
        return;
    }

    if (lore_ptr->r_ptr->r_pkills) {
        hooked_roff(format(
            _("あなたはこのモンスターを少なくとも %d 体は殺している。", "You have killed at least %d of these creatures.  "), lore_ptr->r_ptr->r_pkills));
    } else if (lore_ptr->r_ptr->r_tkills) {
        hooked_roff(format(_("あなたの先祖はこのモンスターを少なくとも %d 体は殺している。", "Your ancestors have killed at least %d of these creatures.  "),
            lore_ptr->r_ptr->r_tkills));
    } else {
        hooked_roff(_("このモンスターを倒したことはない。", "No battles to the death are recalled.  "));
    }

    hooked_roff("\n");
}
