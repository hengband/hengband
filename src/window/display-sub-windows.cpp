#include "window/display-sub-windows.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "floor/cave.h"
#include "game-option/option-flags.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "grid/feature.h"
#include "inventory/inventory-describer.h"
#include "inventory/inventory-slot-types.h"
#include "inventory/inventory-util.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "object/item-tester-hooker.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "object/object-mark-types.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "spell-kind/magic-item-recharger.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-describer.h"
#include "target/target-preparation.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "view/display-lore.h"
#include "view/display-map.h"
#include "view/display-messages.h"
#include "view/display-player.h"
#include "view/object-describer.h"
#include "window/main-window-equipments.h"
#include "window/main-window-util.h"
#include "world/world.h"
#include <mutex>
#include <sstream>
#include <string>

/*! サブウィンドウ表示用の ItemTester オブジェクト */
static std::unique_ptr<ItemTester> fix_item_tester = std::make_unique<AllMatchItemTester>();

FixItemTesterSetter::FixItemTesterSetter(const ItemTester& item_tester)
{
    fix_item_tester = item_tester.clone();
}

FixItemTesterSetter::~FixItemTesterSetter()
{
    fix_item_tester = std::make_unique<AllMatchItemTester>();
}

/*!
 * @brief サブウィンドウに所持品一覧を表示する / Hack -- display inventory in sub-windows
 * @param player_ptr プレーヤーへの参照ポインタ
 */
void fix_inventory(player_type *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term_type *old = Term;
        if (!angband_term[j])
            continue;

        if (!(window_flag[j] & (PW_INVEN)))
            continue;

        term_activate(angband_term[j]);
        display_inventory(player_ptr, *fix_item_tester);
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
 */
static void print_monster_line(TERM_LEN x, TERM_LEN y, monster_type *m_ptr, int n_same)
{
    char buf[256];
    MONRACE_IDX r_idx = m_ptr->ap_r_idx;
    monster_race *r_ptr = &r_info[r_idx];

    term_erase(0, y, 255);
    term_gotoxy(x, y);
    if (!r_ptr)
        return;
    if (r_ptr->flags1 & RF1_UNIQUE) {
        bool is_bounty = false;
        for (int i = 0; i < MAX_BOUNTY; i++) {
            if (current_world_ptr->bounty_r_idx[i] == r_idx) {
                is_bounty = true;
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

    if (r_ptr->r_tkills && m_ptr->mflag2.has_not(MFLAG2::KAGE)) {
        sprintf(buf, " %2d", (int)r_ptr->level);
    } else {
        strcpy(buf, " ??");
    }

    term_addstr(-1, TERM_WHITE, buf);

    sprintf(buf, " %s ", r_ptr->name.c_str());
    term_addstr(-1, TERM_WHITE, buf);
}

/*!
 * @brief モンスターの出現リストを表示する / Print monster info in line
 * @param x 表示列
 * @param y 表示行
 * @param max_lines 最大何行描画するか
 */
void print_monster_list(floor_type *floor_ptr, const std::vector<MONSTER_IDX> &monster_list, TERM_LEN x, TERM_LEN y, TERM_LEN max_lines)
{
    TERM_LEN line = y;
    monster_type *last_mons = nullptr;
    int n_same = 0;
    size_t i;
    for (i = 0; i < monster_list.size(); i++) {
        auto m_ptr = &floor_ptr->m_list[monster_list[i]];
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

        // last_mons と m_ptr が(見た目が)異なるなら、last_mons とその数を出力。
        // m_ptr を新たな last_mons とする。
        print_monster_line(x, line++, last_mons, n_same);
        n_same = 1;
        last_mons = m_ptr;

        // 行数が足りなくなったら中断。
        if (line - y == max_lines)
            break;
    }

    if (i != monster_list.size()) {
        // 行数が足りなかった場合、最終行にその旨表示。
        term_addstr(-1, TERM_WHITE, "-- and more --");
    } else {
        // 行数が足りていれば last_mons とその数を出力し、残りの行をクリア。
        if (last_mons)
            print_monster_line(x, line++, last_mons, n_same);
        for (; line < max_lines; line++)
            term_erase(0, line, 255);
    }
}

/*!
 * @brief 出現中モンスターのリストをサブウィンドウに表示する / Hack -- display monster list in sub-windows
 * @param player_ptr プレーヤーへの参照ポインタ
 */
void fix_monster_list(player_type *player_ptr)
{
    static std::vector<MONSTER_IDX> monster_list;
    std::once_flag once;

    for (int j = 0; j < 8; j++) {
        term_type *old = Term;
        if (!angband_term[j])
            continue;
        if (!(window_flag[j] & PW_MONSTER_LIST))
            continue;
        if (angband_term[j]->never_fresh)
            continue;

        term_activate(angband_term[j]);
        int w, h;
        term_get_size(&w, &h);
        std::call_once(once, target_sensing_monsters_prepare, player_ptr, monster_list);
        print_monster_list(player_ptr->current_floor_ptr, monster_list, 0, 0, h);
        term_fresh();
        term_activate(old);
    }

    if (use_music && has_monster_music) {
        std::call_once(once, target_sensing_monsters_prepare, player_ptr, monster_list);
        select_monster_music(player_ptr, monster_list);
    }
}

/*!
 * @brief 装備アイテム一覧を表示する /
 * Choice window "shadow" of the "show_equip()" function
 */
static void display_equipment(player_type *owner_ptr, const ItemTester& item_tester)
{
    if (!owner_ptr || !owner_ptr->inventory_list)
        return;

    TERM_LEN wid, hgt;
    term_get_size(&wid, &hgt);

    TERM_COLOR attr = TERM_WHITE;
    char tmp_val[80];
    GAME_TEXT o_name[MAX_NLEN];
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        int cur_row = i - INVEN_MAIN_HAND;
        if (cur_row >= hgt)
            break;

        auto o_ptr = &owner_ptr->inventory_list[i];
        auto do_disp = owner_ptr->select_ring_slot ? is_ring_slot(i) : item_tester.okay(o_ptr);
        strcpy(tmp_val, "   ");

        if (do_disp) {
            tmp_val[0] = index_to_label(i);
            tmp_val[1] = ')';
        }

        int cur_col = 3;
        term_erase(cur_col, cur_row, 255);
        term_putstr(0, cur_row, cur_col, TERM_WHITE, tmp_val);

        if ((((i == INVEN_MAIN_HAND) && can_attack_with_sub_hand(owner_ptr)) || ((i == INVEN_SUB_HAND) && can_attack_with_main_hand(owner_ptr)))
            && has_two_handed_weapons(owner_ptr)) {
            strcpy(o_name, _("(武器を両手持ち)", "(wielding with two-hands)"));
            attr = TERM_WHITE;
        } else {
            describe_flavor(owner_ptr, o_name, o_ptr, 0);
            attr = tval_to_attr[o_ptr->tval % 128];
        }

        int n = strlen(o_name);
        if (o_ptr->timeout)
            attr = TERM_L_DARK;

        if (show_item_graph) {
            TERM_COLOR a = object_attr(o_ptr);
            SYMBOL_CODE c = object_char(o_ptr);
            term_queue_bigchar(cur_col, cur_row, a, c, 0, 0);
            if (use_bigtile)
                cur_col++;

            cur_col += 2;
        }

        term_putstr(cur_col, cur_row, n, attr, o_name);
        if (show_weights) {
            int wgt = o_ptr->weight * o_ptr->number;
            sprintf(tmp_val, _("%3d.%1d kg", "%3d.%1d lb"), _(lbtokg1(wgt), wgt / 10), _(lbtokg2(wgt), wgt % 10));
            prt(tmp_val, cur_row, wid - (show_labels ? 28 : 9));
        }

        if (show_labels) {
            term_putstr(wid - 20, cur_row, -1, TERM_WHITE, " <-- ");
            prt(mention_use(owner_ptr, i), cur_row, wid - 15);
        }
    }

    for (int i = INVEN_TOTAL - INVEN_MAIN_HAND; i < hgt; i++)
        term_erase(0, i, 255);
}

/*!
 * @brief 現在の装備品をサブウィンドウに表示する /
 * Hack -- display equipment in sub-windows
 * @param player_ptr プレーヤーへの参照ポインタ
 */
void fix_equip(player_type *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term_type *old = Term;
        if (!angband_term[j])
            continue;
        if (!(window_flag[j] & (PW_EQUIP)))
            continue;

        term_activate(angband_term[j]);
        display_equipment(player_ptr, *fix_item_tester);
        term_fresh();
        term_activate(old);
    }
}

/*!
 * @brief 現在のプレイヤーステータスをサブウィンドウに表示する /
 * @param player_ptr プレーヤーへの参照ポインタ
 * Hack -- display character in sub-windows
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
            term_putstr(0, (h - 1) - i, -1, (byte)((i < now_message) ? TERM_WHITE : TERM_SLATE), message_str((int16_t)i));
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

/*!
 * @brief 自分の周辺の地形をTermに表示する
 * @param プレイヤー情報への参照ポインタ
 */
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
 * @brief 自分の周辺のダンジョンの地形をサブウィンドウに表示する / display dungeon view around player in a sub window
 * @param player_ptr プレーヤーへの参照ポインタ
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
 * @brief 床上のモンスター情報を返す
 * @param floor_ptr 階の情報への参照ポインタ
 * @param grid_prt 座標グリッドの情報への参照ポインタ
 * @return モンスターが見える場合にはモンスター情報への参照ポインタ、それ以外はnullptr
 * @details
 * Lookコマンドでカーソルを合わせた場合に合わせてミミックは考慮しない。
 */
static monster_type *monster_on_floor_items(const floor_type *floor_ptr, const grid_type *g_ptr)
{
    if (g_ptr->m_idx == 0)
        return nullptr;

    monster_type *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
    if (!monster_is_valid(m_ptr) || !m_ptr->ml)
        return nullptr;

    return m_ptr;
}

/*!
 * @brief 床上のアイテム一覧を作成し、表示する
 * @param プレイヤー情報への参照ポインタ
 * @param y 参照する座標グリッドのy座標
 * @param x 参照する座標グリッドのx座標
 */
static void display_floor_item_list(player_type *player_ptr, const int y, const int x)
{
    // Term の行数を取得。
    TERM_LEN term_h;
    {
        TERM_LEN term_w;
        term_get_size(&term_w, &term_h);
    }
    if (term_h <= 0)
        return;

    term_clear();
    term_gotoxy(0, 0);

    const auto *floor_ptr = player_ptr->current_floor_ptr;
    const auto *g_ptr = &floor_ptr->grid_array[y][x];
    char line[1024];

    // 先頭行を書く。
    if (player_bold(player_ptr, y, x))
        sprintf(line, _("(X:%03d Y:%03d) あなたの足元のアイテム一覧", "Items at (%03d,%03d) under you"), x, y);
    else if (const auto *m_ptr = monster_on_floor_items(floor_ptr, g_ptr); m_ptr != nullptr) {
        if (player_ptr->image) {
            sprintf(line, _("(X:%03d Y:%03d) 何か奇妙な物の足元の発見済みアイテム一覧", "Found items at (%03d,%03d) under something strange"), x, y);
        } else {
            const monster_race *const r_ptr = &r_info[m_ptr->ap_r_idx];
            sprintf(line, _("(X:%03d Y:%03d) %sの足元の発見済みアイテム一覧", "Found items at (%03d,%03d) under %s"), x, y, r_ptr->name.c_str());
        }
    } else {
        const feature_type *const f_ptr = &f_info[g_ptr->feat];
        concptr fn = f_ptr->name.c_str();
        char buf[512];

        if (f_ptr->flags.has(FF::STORE) || (f_ptr->flags.has(FF::BLDG) && !floor_ptr->inside_arena))
            sprintf(buf, _("%sの入口", "on the entrance of %s"), fn);
        else if (f_ptr->flags.has(FF::WALL))
            sprintf(buf, _("%sの中", "in %s"), fn);
        else
            sprintf(buf, _("%s", "on %s"), fn);
        sprintf(line, _("(X:%03d Y:%03d) %sの上の発見済みアイテム一覧", "Found items at (X:%03d Y:%03d) %s"), x, y, buf);
    }
    term_addstr(-1, TERM_WHITE, line);

    // (y,x) のアイテムを1行に1個ずつ書く。
    TERM_LEN term_y = 1;
    for (const auto o_idx : g_ptr->o_idx_list) {
        object_type *const o_ptr = &floor_ptr->o_list[o_idx];

        // 未発見アイテムおよび金は対象外。
        if (none_bits(o_ptr->marked, OM_FOUND) || o_ptr->tval == TV_GOLD) {
            continue;
        }

        // 途中で行数が足りなくなったら最終行にその旨追記して終了。
        if (term_y >= term_h) {
            term_addstr(-1, TERM_WHITE, "-- more --");
            break;
        }

        term_gotoxy(0, term_y);

        if (player_ptr->image) {
            term_addstr(-1, TERM_WHITE, _("何か奇妙な物", "something strange"));
        } else {
            describe_flavor(player_ptr, line, o_ptr, 0);
            TERM_COLOR attr = tval_to_attr[o_ptr->tval % 128];
            term_addstr(-1, attr, line);
        }

        ++term_y;
    }
}

/*!
 * @brief (y,x) のアイテム一覧をサブウィンドウに表示する / display item at (y,x) in sub-windows
 */
void fix_floor_item_list(player_type *player_ptr, const int y, const int x)
{
    for (int j = 0; j < 8; j++) {
        if (!angband_term[j])
            continue;
        if (angband_term[j]->never_fresh)
            continue;
        if (!(window_flag[j] & PW_FLOOR_ITEM_LIST))
            continue;

        term_type *old = Term;
        term_activate(angband_term[j]);

        display_floor_item_list(player_ptr, y, x);
        term_fresh();

        term_activate(old);
    }
}

/*!
 * @brief サブウィンドウに所持品、装備品リストの表示を行う /
 * Flip "inven" and "equip" in any sub-windows
 */
void toggle_inventory_equipment(player_type *owner_ptr)
{
    for (int j = 0; j < 8; j++) {
        if (!angband_term[j])
            continue;

        if (window_flag[j] & (PW_INVEN)) {
            window_flag[j] &= ~(PW_INVEN);
            window_flag[j] |= (PW_EQUIP);
            owner_ptr->window_flags |= (PW_EQUIP);
            continue;
        }

        if (window_flag[j] & PW_EQUIP) {
            window_flag[j] &= ~(PW_EQUIP);
            window_flag[j] |= PW_INVEN;
            owner_ptr->window_flags |= PW_INVEN;
        }
    }
}
