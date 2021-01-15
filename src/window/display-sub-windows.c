#include "window/display-sub-windows.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "floor/cave.h"
#include "game-option/option-flags.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "inventory/inventory-describer.h"
#include "inventory/inventory-slot-types.h"
#include "inventory/inventory-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "object/item-tester-hooker.h"
#include "object/object-info.h"
#include "player/player-status-flags.h"
#include "spell-kind/magic-item-recharger.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "target/target-preparation.h"
#include "target/target-types.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "view/display-lore.h"
#include "view/display-map.h"
#include "view/display-messages.h"
#include "view/display-player.h"
#include "view/object-describer.h"
#include "window/main-window-equipments.h"
#include "window/main-window-util.h"
#include "world/world.h"

/*!
 * @brief サブウィンドウに所持品一覧を表示する / Hack -- display inventory in sub-windows
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void fix_inventory(player_type *player_ptr, tval_type item_tester_tval)
{
    for (int j = 0; j < 8; j++) {
        term_type *old = Term;
        if (!angband_term[j])
            continue;

        if (!(window_flag[j] & (PW_INVEN)))
            continue;

        term_activate(angband_term[j]);
        display_inventory(player_ptr, item_tester_tval);
        term_fresh();
        term_activate(old);
    }
}

/*!
 * @brief モンスターの現在数を一行で表現する / Print monster info in line
 * @param x 表示列
 * @param y 表示行
 * @param m_ptr 思い出を表示するモンスター情報の参照ポインタ
 * @param n_same モンスターの数の現在数
 * @details
 * <pre>
 * nnn X LV name
 *  nnn : number or unique(U) or wanted unique(W)
 *  X   : symbol of monster
 *  LV  : monster lv if known
 *  name: name of monster
 * </pre>
 * @return なし
 */
static void print_monster_line(TERM_LEN x, TERM_LEN y, monster_type *m_ptr, int n_same)
{
    char buf[256];
    MONRACE_IDX r_idx = m_ptr->ap_r_idx;
    monster_race *r_ptr = &r_info[r_idx];

    term_gotoxy(x, y);
    if (!r_ptr)
        return;
    if (r_ptr->flags1 & RF1_UNIQUE) {
        bool is_bounty = FALSE;
        for (int i = 0; i < MAX_BOUNTY; i++) {
            if (current_world_ptr->bounty_r_idx[i] == r_idx) {
                is_bounty = TRUE;
                break;
            }
        }

        term_addstr(-1, TERM_WHITE, is_bounty ? "  W" : "  U");
    } else {
        sprintf(buf, "%3d", n_same);
        term_addstr(-1, TERM_WHITE, buf);
    }

    term_addstr(-1, TERM_WHITE, " ");
    term_add_bigch(r_ptr->x_attr, r_ptr->x_char);

    if (r_ptr->r_tkills && !(m_ptr->mflag2 & MFLAG2_KAGE)) {
        sprintf(buf, " %2d", (int)r_ptr->level);
    } else {
        strcpy(buf, " ??");
    }

    term_addstr(-1, TERM_WHITE, buf);

    sprintf(buf, " %s ", r_name + r_ptr->name);
    term_addstr(-1, TERM_WHITE, buf);
}

/*!
 * @brief モンスターの出現リストを表示する / Print monster info in line
 * @param x 表示列
 * @param y 表示行
 * @param max_lines 最大何行描画するか
 */
void print_monster_list(floor_type *floor_ptr, TERM_LEN x, TERM_LEN y, TERM_LEN max_lines)
{
    TERM_LEN line = y;
    monster_type *last_mons = NULL;
    monster_type *m_ptr = NULL;
    int n_same = 0;
    int i;
    for (i = 0; i < tmp_pos.n; i++) {
        grid_type *g_ptr = &floor_ptr->grid_array[tmp_pos.y[i]][tmp_pos.x[i]];
        if (!g_ptr->m_idx || !floor_ptr->m_list[g_ptr->m_idx].ml)
            continue;
        m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
        if (is_pet(m_ptr))
            continue; // pet
        if (!m_ptr->r_idx)
            continue; // dead?

        //ソート済みなので同じモンスターは連続する．これを利用して同じモンスターをカウント，まとめて表示する．
        //先頭モンスター
        if (!last_mons) {
            last_mons = m_ptr;
            n_same = 1;
            continue;
        }

        // same race?
        if (last_mons->ap_r_idx == m_ptr->ap_r_idx) {
            n_same++;
            continue; //表示処理を次に回す
        }

        // print last mons info
        print_monster_line(x, line++, last_mons, n_same);
        n_same = 1;
        last_mons = m_ptr;
        if (line - y - 1 == max_lines)
            break;
    }

    if (line - y - 1 == max_lines && i != tmp_pos.n) {
        term_gotoxy(x, line);
        term_addstr(-1, TERM_WHITE, "-- and more --");
    } else {
        if (last_mons)
            print_monster_line(x, line++, last_mons, n_same);
    }
}

/*!
 * @brief 出現中モンスターのリストをサブウィンドウに表示する / Hack -- display monster list in sub-windows
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void fix_monster_list(player_type *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term_type *old = Term;
        if (!angband_term[j])
            continue;
        if (!(window_flag[j] & PW_MONSTER_LIST))
            continue;

        term_activate(angband_term[j]);
        int w, h;
        term_get_size(&w, &h);
        term_clear();
        target_set_prepare(player_ptr, TARGET_LOOK);
        print_monster_list(player_ptr->current_floor_ptr, 0, 0, h);
        term_fresh();
        term_activate(old);
    }
}

/*!
 * @brief 装備アイテム一覧を表示する /
 * Choice window "shadow" of the "show_equip()" function
 * @return なし
 */
static void display_equipment(player_type *owner_ptr, tval_type tval)
{
    if (!owner_ptr || !owner_ptr->inventory_list)
        return;

    TERM_LEN wid, hgt;
    term_get_size(&wid, &hgt);

    TERM_COLOR attr = TERM_WHITE;
    char tmp_val[80];
    GAME_TEXT o_name[MAX_NLEN];
    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        o_ptr = &owner_ptr->inventory_list[i];
        tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';
        if (owner_ptr->select_ring_slot ? is_ring_slot(i) : item_tester_okay(owner_ptr, o_ptr, tval)) {
            tmp_val[0] = index_to_label(i);
            tmp_val[1] = ')';
        }

        term_putstr(0, i - INVEN_RARM, 3, TERM_WHITE, tmp_val);
        if ((((i == INVEN_RARM) && has_left_hand_weapon(owner_ptr)) || ((i == INVEN_LARM) && has_right_hand_weapon(owner_ptr))) && has_two_handed_weapons(owner_ptr)) {
            strcpy(o_name, _("(武器を両手持ち)", "(wielding with two-hands)"));
            attr = TERM_WHITE;
        } else {
            describe_flavor(owner_ptr, o_name, o_ptr, 0);
            attr = tval_to_attr[o_ptr->tval % 128];
        }

        int n = strlen(o_name);
        if (o_ptr->timeout)
            attr = TERM_L_DARK;

        term_putstr(3, i - INVEN_RARM, n, attr, o_name);
        term_erase(3 + n, i - INVEN_RARM, 255);
        if (show_weights) {
            int wgt = o_ptr->weight * o_ptr->number;
            sprintf(tmp_val, _("%3d.%1d kg", "%3d.%1d lb"), _(lbtokg1(wgt), wgt / 10), _(lbtokg2(wgt), wgt % 10));
            prt(tmp_val, i - INVEN_RARM, wid - (show_labels ? 28 : 9));
        }

        if (show_labels) {
            term_putstr(wid - 20, i - INVEN_RARM, -1, TERM_WHITE, " <-- ");
            prt(mention_use(owner_ptr, i), i - INVEN_RARM, wid - 15);
        }
    }

    for (int i = INVEN_TOTAL - INVEN_RARM; i < hgt; i++)
        term_erase(0, i, 255);
}

/*!
 * @brief 現在の装備品をサブウィンドウに表示する /
 * Hack -- display equipment in sub-windows
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void fix_equip(player_type *player_ptr, tval_type item_tester_tval)
{
    for (int j = 0; j < 8; j++) {
        term_type *old = Term;
        if (!angband_term[j])
            continue;
        if (!(window_flag[j] & (PW_EQUIP)))
            continue;

        term_activate(angband_term[j]);
        display_equipment(player_ptr, item_tester_tval);
        term_fresh();
        term_activate(old);
    }
}

/*!
 * @brief 現在のプレイヤーステータスをサブウィンドウに表示する /
 * @param player_ptr プレーヤーへの参照ポインタ
 * Hack -- display character in sub-windows
 * @return なし
 */
void fix_player(player_type *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term_type *old = Term;
        if (!angband_term[j])
            continue;

        if (!(window_flag[j] & (PW_PLAYER)))
            continue;

        term_activate(angband_term[j]);
        update_playtime();
        display_player(player_ptr, 0);
        term_fresh();
        term_activate(old);
    }
}

/*!
 * @brief ゲームメッセージ履歴をサブウィンドウに表示する /
 * Hack -- display recent messages in sub-windows
 * Adjust for width and split messages
 * @return なし
 */
void fix_message(void)
{
    for (int j = 0; j < 8; j++) {
        term_type *old = Term;
        if (!angband_term[j])
            continue;

        if (!(window_flag[j] & (PW_MESSAGE)))
            continue;

        term_activate(angband_term[j]);
        TERM_LEN w, h;
        term_get_size(&w, &h);
        for (int i = 0; i < h; i++) {
            term_putstr(0, (h - 1) - i, -1, (byte)((i < now_message) ? TERM_WHITE : TERM_SLATE), message_str((s16b)i));
            TERM_LEN x, y;
            term_locate(&x, &y);
            term_erase(x, y, 255);
        }

        term_fresh();
        term_activate(old);
    }
}

/*!
 * @brief 簡易マップをサブウィンドウに表示する /
 * Hack -- display overhead view in sub-windows
 * Adjust for width and split messages
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * Note that the "player" symbol does NOT appear on the map.
 */
void fix_overhead(player_type *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term_type *old = Term;
        TERM_LEN wid, hgt;
        if (!angband_term[j])
            continue;

        if (!(window_flag[j] & (PW_OVERHEAD)))
            continue;

        term_activate(angband_term[j]);
        term_get_size(&wid, &hgt);
        if (wid > COL_MAP + 2 && hgt > ROW_MAP + 2) {
            int cy, cx;
            display_map(player_ptr, &cy, &cx);
            term_fresh();
        }

        term_activate(old);
    }
}

static void display_dungeon(player_type *player_ptr)
{
    TERM_COLOR ta = 0;
    SYMBOL_CODE tc = '\0';

    for (TERM_LEN x = player_ptr->x - Term->wid / 2 + 1; x <= player_ptr->x + Term->wid / 2; x++) {
        for (TERM_LEN y = player_ptr->y - Term->hgt / 2 + 1; y <= player_ptr->y + Term->hgt / 2; y++) {
            TERM_COLOR a;
            SYMBOL_CODE c;
            if (!in_bounds2(player_ptr->current_floor_ptr, y, x)) {
                feature_type *f_ptr = &f_info[feat_none];
                a = f_ptr->x_attr[F_LIT_STANDARD];
                c = f_ptr->x_char[F_LIT_STANDARD];
                term_queue_char(x - player_ptr->x + Term->wid / 2 - 1, y - player_ptr->y + Term->hgt / 2 - 1, a, c, ta, tc);
                continue;
            }

            map_info(player_ptr, y, x, &a, &c, &ta, &tc);

            if (!use_graphics) {
                if (current_world_ptr->timewalk_m_idx)
                    a = TERM_DARK;
                else if (is_invuln(player_ptr) || player_ptr->timewalk)
                    a = TERM_WHITE;
                else if (player_ptr->wraith_form)
                    a = TERM_L_DARK;
            }

            term_queue_char(x - player_ptr->x + Term->wid / 2 - 1, y - player_ptr->y + Term->hgt / 2 - 1, a, c, ta, tc);
        }
    }
}

/*!
 * @brief ダンジョンの地形をサブウィンドウに表示する /
 * Hack -- display dungeon view in sub-windows
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void fix_dungeon(player_type *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term_type *old = Term;
        if (!angband_term[j])
            continue;

        if (!(window_flag[j] & (PW_DUNGEON)))
            continue;

        term_activate(angband_term[j]);
        display_dungeon(player_ptr);
        term_fresh();
        term_activate(old);
    }
}

/*!
 * @brief モンスターの思い出をサブウィンドウに表示する /
 * Hack -- display dungeon view in sub-windows
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void fix_monster(player_type *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term_type *old = Term;
        if (!angband_term[j])
            continue;

        if (!(window_flag[j] & (PW_MONSTER)))
            continue;

        term_activate(angband_term[j]);
        if (player_ptr->monster_race_idx)
            display_roff(player_ptr);

        term_fresh();
        term_activate(old);
    }
}

/*!
 * @brief ベースアイテム情報をサブウィンドウに表示する /
 * Hack -- display object recall in sub-windows
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void fix_object(player_type *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term_type *old = Term;
        if (!angband_term[j])
            continue;

        if (!(window_flag[j] & (PW_OBJECT)))
            continue;

        term_activate(angband_term[j]);
        if (player_ptr->object_kind_idx)
            display_koff(player_ptr, player_ptr->object_kind_idx);

        term_fresh();
        term_activate(old);
    }
}

/*!
 * @brief サブウィンドウに所持品、装備品リストの表示を行う /
 * Flip "inven" and "equip" in any sub-windows
 * @return なし
 */
void toggle_inventory_equipment(player_type *owner_ptr)
{
    for (int j = 0; j < 8; j++) {
        if (!angband_term[j])
            continue;

        if (window_flag[j] & (PW_INVEN)) {
            window_flag[j] &= ~(PW_INVEN);
            window_flag[j] |= (PW_EQUIP);
            owner_ptr->window |= (PW_EQUIP);
            continue;
        }

        if (window_flag[j] & PW_EQUIP) {
            window_flag[j] &= ~(PW_EQUIP);
            window_flag[j] |= PW_INVEN;
            owner_ptr->window |= PW_INVEN;
        }
    }
}
