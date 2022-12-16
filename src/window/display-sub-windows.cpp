﻿#include "window/display-sub-windows.h"
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
#include "locale/japanese.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "object/item-tester-hooker.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "spell-kind/magic-item-recharger.h"
#include "system/baseitem-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "target/target-describer.h"
#include "target/target-preparation.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-lore.h"
#include "view/display-map.h"
#include "view/display-messages.h"
#include "view/display-player.h"
#include "view/object-describer.h"
#include "window/main-window-equipments.h"
#include "window/main-window-util.h"
#include "world/world.h"
#include <algorithm>
#include <mutex>
#include <sstream>
#include <string>
#include <util/object-sort.h>

/*! サブウィンドウ表示用の ItemTester オブジェクト */
static std::unique_ptr<ItemTester> fix_item_tester = std::make_unique<AllMatchItemTester>();

FixItemTesterSetter::FixItemTesterSetter(const ItemTester &item_tester)
{
    fix_item_tester = item_tester.clone();
}

FixItemTesterSetter::~FixItemTesterSetter()
{
    fix_item_tester = std::make_unique<AllMatchItemTester>();
}

/*!
 * @brief サブウィンドウに所持品一覧を表示する / Hack -- display inventory in sub-windows
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void fix_inventory(PlayerType *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term_type *old = game_term;
        if (!angband_terms[j]) {
            continue;
        }

        if (!(window_flag[j] & (PW_INVEN))) {
            continue;
        }

        term_activate(angband_terms[j]);
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
 * @param n_same モンスター数の現在数
 * @param n_awake 起きている数
 * @details
 * <pre>
 * nnn X LV name
 *  nnn : number or unique(U) or wanted unique(W)
 *  X   : symbol of monster
 *  LV  : monster lv if known
 *  name: name of monster
 * </pre>
 */
static void print_monster_line(TERM_LEN x, TERM_LEN y, MonsterEntity *m_ptr, int n_same, int n_awake)
{
    std::string buf;
    MonsterRaceId r_idx = m_ptr->ap_r_idx;
    auto *r_ptr = &monraces_info[r_idx];

    term_erase(0, y, 255);
    term_gotoxy(x, y);
    if (!r_ptr) {
        return;
    }
    if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        buf = format(_("%3s(覚%2d)", "%3s(%2d)"), MonsterRace(r_idx).is_bounty(true) ? "  W" : "  U", n_awake);
    } else {
        buf = format(_("%3d(覚%2d)", "%3d(%2d)"), n_same, n_awake);
    }
    term_addstr(-1, TERM_WHITE, buf.data());

    term_addstr(-1, TERM_WHITE, " ");
    term_add_bigch(r_ptr->x_attr, r_ptr->x_char);

    if (r_ptr->r_tkills && m_ptr->mflag2.has_not(MonsterConstantFlagType::KAGE)) {
        buf = format(" %2d", (int)r_ptr->level);
    } else {
        buf = " ??";
    }

    term_addstr(-1, TERM_WHITE, buf.data());

    buf = format(" %s ", r_ptr->name.data());
    term_addstr(-1, TERM_WHITE, buf.data());
}

/*!
 * @brief モンスターの出現リストを表示する / Print monster info in line
 * @param x 表示列
 * @param y 表示行
 * @param max_lines 最大何行描画するか
 */
void print_monster_list(FloorType *floor_ptr, const std::vector<MONSTER_IDX> &monster_list, TERM_LEN x, TERM_LEN y, TERM_LEN max_lines)
{
    TERM_LEN line = y;
    struct info {
        MonsterEntity *monster_entity;
        int visible_count; // 現在数
        int awake_count; // 起きている数
    };

    // 出現リスト表示用のデータ
    std::vector<info> monster_list_info;

    // 描画に必要なデータを集める
    for (auto monster_index : monster_list) {
        auto m_ptr = &floor_ptr->m_list[monster_index];

        if (m_ptr->is_pet()) {
            continue;
        } // pet
        if (!MonsterRace(m_ptr->r_idx).is_valid()) {
            continue;
        } // dead?

        // ソート済みなので同じモンスターは連続する．これを利用して同じモンスターをカウント，まとめて表示する．
        if (monster_list_info.empty() || (monster_list_info.back().monster_entity->ap_r_idx != m_ptr->ap_r_idx)) {
            monster_list_info.push_back({ m_ptr, 0, 0 });
        }

        // 出現数をカウント
        monster_list_info.back().visible_count++;

        // 起きているモンスターをカウント
        if (!m_ptr->is_asleep()) {
            monster_list_info.back().awake_count++;
        }
    }

    // 集めたデータを元にリストを表示する
    for (const auto &info : monster_list_info) {
        print_monster_line(x, line++, info.monster_entity, info.visible_count, info.awake_count);

        // 行数が足りなくなったら中断。
        if (line - y == max_lines) {
            // 行数が足りなかった場合、最終行にその旨表示。
            term_addstr(-1, TERM_WHITE, "-- and more --");
            break;
        }
    }

    for (; line < max_lines; line++) {
        term_erase(0, line, 255);
    }
}

/*!
 * @brief 出現中モンスターのリストをサブウィンドウに表示する / Hack -- display monster list in sub-windows
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void fix_monster_list(PlayerType *player_ptr)
{
    static std::vector<MONSTER_IDX> monster_list;
    std::once_flag once;

    for (int j = 0; j < 8; j++) {
        term_type *old = game_term;
        if (!angband_terms[j]) {
            continue;
        }
        if (!(window_flag[j] & PW_MONSTER_LIST)) {
            continue;
        }
        if (angband_terms[j]->never_fresh) {
            continue;
        }

        term_activate(angband_terms[j]);
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
static void display_equipment(PlayerType *player_ptr, const ItemTester &item_tester)
{
    if (!player_ptr || !player_ptr->inventory_list) {
        return;
    }

    TERM_LEN wid, hgt;
    term_get_size(&wid, &hgt);

    TERM_COLOR attr = TERM_WHITE;
    GAME_TEXT o_name[MAX_NLEN];
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        int cur_row = i - INVEN_MAIN_HAND;
        if (cur_row >= hgt) {
            break;
        }

        auto o_ptr = &player_ptr->inventory_list[i];
        auto do_disp = player_ptr->select_ring_slot ? is_ring_slot(i) : item_tester.okay(o_ptr);
        std::string tmp_val = "   ";

        if (do_disp) {
            tmp_val[0] = index_to_label(i);
            tmp_val[1] = ')';
        }

        int cur_col = 3;
        term_erase(cur_col, cur_row, 255);
        term_putstr(0, cur_row, cur_col, TERM_WHITE, tmp_val.data());

        if ((((i == INVEN_MAIN_HAND) && can_attack_with_sub_hand(player_ptr)) || ((i == INVEN_SUB_HAND) && can_attack_with_main_hand(player_ptr))) && has_two_handed_weapons(player_ptr)) {
            strcpy(o_name, _("(武器を両手持ち)", "(wielding with two-hands)"));
            attr = TERM_WHITE;
        } else {
            describe_flavor(player_ptr, o_name, o_ptr, 0);
            attr = tval_to_attr[enum2i(o_ptr->bi_key.tval()) % 128];
        }

        int n = strlen(o_name);
        if (o_ptr->timeout) {
            attr = TERM_L_DARK;
        }

        if (show_item_graph) {
            const auto a = o_ptr->get_color();
            const auto c = o_ptr->get_symbol();
            term_queue_bigchar(cur_col, cur_row, a, c, 0, 0);
            if (use_bigtile) {
                cur_col++;
            }

            cur_col += 2;
        }

        term_putstr(cur_col, cur_row, n, attr, o_name);
        if (show_weights) {
            int wgt = o_ptr->weight * o_ptr->number;
            tmp_val = format(_("%3d.%1d kg", "%3d.%1d lb"), _(lb_to_kg_integer(wgt), wgt / 10), _(lb_to_kg_fraction(wgt), wgt % 10));
            prt(tmp_val.data(), cur_row, wid - (show_labels ? 28 : 9));
        }

        if (show_labels) {
            term_putstr(wid - 20, cur_row, -1, TERM_WHITE, " <-- ");
            prt(mention_use(player_ptr, i), cur_row, wid - 15);
        }
    }

    for (int i = INVEN_TOTAL - INVEN_MAIN_HAND; i < hgt; i++) {
        term_erase(0, i, 255);
    }
}

/*!
 * @brief 現在の装備品をサブウィンドウに表示する /
 * Hack -- display equipment in sub-windows
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void fix_equip(PlayerType *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term_type *old = game_term;
        if (!angband_terms[j]) {
            continue;
        }
        if (!(window_flag[j] & (PW_EQUIP))) {
            continue;
        }

        term_activate(angband_terms[j]);
        display_equipment(player_ptr, *fix_item_tester);
        term_fresh();
        term_activate(old);
    }
}

/*!
 * @brief 現在のプレイヤーステータスをサブウィンドウに表示する /
 * @param player_ptr プレイヤーへの参照ポインタ
 * Hack -- display character in sub-windows
 */
void fix_player(PlayerType *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term_type *old = game_term;
        if (!angband_terms[j]) {
            continue;
        }

        if (!(window_flag[j] & (PW_PLAYER))) {
            continue;
        }

        term_activate(angband_terms[j]);
        update_playtime();
        (void)display_player(player_ptr, 0);
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
        term_type *old = game_term;
        if (!angband_terms[j]) {
            continue;
        }

        if (!(window_flag[j] & (PW_MESSAGE))) {
            continue;
        }

        term_activate(angband_terms[j]);
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
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * Note that the "player" symbol does NOT appear on the map.
 */
void fix_overhead(PlayerType *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term_type *old = game_term;
        TERM_LEN wid, hgt;
        if (!angband_terms[j]) {
            continue;
        }

        if (!(window_flag[j] & (PW_OVERHEAD))) {
            continue;
        }

        term_activate(angband_terms[j]);
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
static void display_dungeon(PlayerType *player_ptr)
{
    TERM_COLOR ta = 0;
    auto tc = '\0';

    for (TERM_LEN x = player_ptr->x - game_term->wid / 2 + 1; x <= player_ptr->x + game_term->wid / 2; x++) {
        for (TERM_LEN y = player_ptr->y - game_term->hgt / 2 + 1; y <= player_ptr->y + game_term->hgt / 2; y++) {
            TERM_COLOR a;
            char c;
            if (!in_bounds2(player_ptr->current_floor_ptr, y, x)) {
                auto *f_ptr = &terrains_info[feat_none];
                a = f_ptr->x_attr[F_LIT_STANDARD];
                c = f_ptr->x_char[F_LIT_STANDARD];
                term_queue_char(x - player_ptr->x + game_term->wid / 2 - 1, y - player_ptr->y + game_term->hgt / 2 - 1, a, c, ta, tc);
                continue;
            }

            map_info(player_ptr, y, x, &a, &c, &ta, &tc);

            if (!use_graphics) {
                if (w_ptr->timewalk_m_idx) {
                    a = TERM_DARK;
                } else if (is_invuln(player_ptr) || player_ptr->timewalk) {
                    a = TERM_WHITE;
                } else if (player_ptr->wraith_form) {
                    a = TERM_L_DARK;
                }
            }

            term_queue_char(x - player_ptr->x + game_term->wid / 2 - 1, y - player_ptr->y + game_term->hgt / 2 - 1, a, c, ta, tc);
        }
    }
}

/*!
 * @brief 自分の周辺のダンジョンの地形をサブウィンドウに表示する / display dungeon view around player in a sub window
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void fix_dungeon(PlayerType *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term_type *old = game_term;
        if (!angband_terms[j]) {
            continue;
        }

        if (!(window_flag[j] & (PW_DUNGEON))) {
            continue;
        }

        term_activate(angband_terms[j]);
        display_dungeon(player_ptr);
        term_fresh();
        term_activate(old);
    }
}

/*!
 * @brief モンスターの思い出をサブウィンドウに表示する /
 * Hack -- display dungeon view in sub-windows
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void fix_monster(PlayerType *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term_type *old = game_term;
        if (!angband_terms[j]) {
            continue;
        }

        if (!(window_flag[j] & (PW_MONSTER))) {
            continue;
        }

        term_activate(angband_terms[j]);
        if (MonsterRace(player_ptr->monster_race_idx).is_valid()) {
            display_roff(player_ptr);
        }

        term_fresh();
        term_activate(old);
    }
}

/*!
 * @brief ベースアイテム情報をサブウィンドウに表示する /
 * Hack -- display object recall in sub-windows
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void fix_object(PlayerType *player_ptr)
{
    for (auto i = 0; i < 8; i++) {
        auto *old = game_term;
        if (angband_terms[i] == nullptr) {
            continue;
        }

        if (none_bits(window_flag[i], PW_OBJECT)) {
            continue;
        }

        term_activate(angband_terms[i]);
        display_koff(player_ptr);
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
static const MonsterEntity *monster_on_floor_items(FloorType *floor_ptr, const grid_type *g_ptr)
{
    if (g_ptr->m_idx == 0) {
        return nullptr;
    }

    auto m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
    if (!m_ptr->is_valid() || !m_ptr->ml) {
        return nullptr;
    }

    return m_ptr;
}

/*!
 * @brief 床上のアイテム一覧を作成し、表示する
 * @param プレイヤー情報への参照ポインタ
 * @param y 参照する座標グリッドのy座標
 * @param x 参照する座標グリッドのx座標
 */
static void display_floor_item_list(PlayerType *player_ptr, const int y, const int x)
{
    // Term の行数を取得。
    TERM_LEN term_h;
    {
        TERM_LEN term_w;
        term_get_size(&term_w, &term_h);
    }
    if (term_h <= 0) {
        return;
    }

    term_clear();
    term_gotoxy(0, 0);

    auto *floor_ptr = player_ptr->current_floor_ptr;
    const auto *g_ptr = &floor_ptr->grid_array[y][x];
    std::string line;

    // 先頭行を書く。
    auto is_hallucinated = player_ptr->effects()->hallucination()->is_hallucinated();
    if (player_bold(player_ptr, y, x)) {
        line = format(_("(X:%03d Y:%03d) あなたの足元のアイテム一覧", "Items at (%03d,%03d) under you"), x, y);
    } else if (const auto *m_ptr = monster_on_floor_items(floor_ptr, g_ptr); m_ptr != nullptr) {
        if (is_hallucinated) {
            line = format(_("(X:%03d Y:%03d) 何か奇妙な物の足元の発見済みアイテム一覧", "Found items at (%03d,%03d) under something strange"), x, y);
        } else {
            const MonsterRaceInfo *const r_ptr = &monraces_info[m_ptr->ap_r_idx];
            line = format(_("(X:%03d Y:%03d) %sの足元の発見済みアイテム一覧", "Found items at (%03d,%03d) under %s"), x, y, r_ptr->name.data());
        }
    } else {
        const TerrainType *const f_ptr = &terrains_info[g_ptr->feat];
        concptr fn = f_ptr->name.data();
        std::string buf;

        if (f_ptr->flags.has(TerrainCharacteristics::STORE) || (f_ptr->flags.has(TerrainCharacteristics::BLDG) && !floor_ptr->inside_arena)) {
            buf = format(_("%sの入口", "on the entrance of %s"), fn);
        } else if (f_ptr->flags.has(TerrainCharacteristics::WALL)) {
            buf = format(_("%sの中", "in %s"), fn);
        } else {
            buf = format(_("%s", "on %s"), fn);
        }
        line = format(_("(X:%03d Y:%03d) %sの上の発見済みアイテム一覧", "Found items at (X:%03d Y:%03d) %s"), x, y, buf.data());
    }
    term_addstr(-1, TERM_WHITE, line.data());

    // (y,x) のアイテムを1行に1個ずつ書く。
    TERM_LEN term_y = 1;
    for (const auto o_idx : g_ptr->o_idx_list) {
        auto *const o_ptr = &floor_ptr->o_list[o_idx];
        const auto tval = o_ptr->bi_key.tval();
        if (o_ptr->marked.has_not(OmType::FOUND) || tval == ItemKindType::GOLD) {
            continue;
        }

        // 途中で行数が足りなくなったら最終行にその旨追記して終了。
        if (term_y >= term_h) {
            term_addstr(-1, TERM_WHITE, "-- more --");
            break;
        }

        term_gotoxy(0, term_y);

        if (is_hallucinated) {
            term_addstr(-1, TERM_WHITE, _("何か奇妙な物", "something strange"));
        } else {
            char buf[1024];
            describe_flavor(player_ptr, buf, o_ptr, 0);
            TERM_COLOR attr = tval_to_attr[enum2i(tval) % 128];
            term_addstr(-1, attr, buf);
        }

        ++term_y;
    }
}

/*!
 * @brief (y,x) のアイテム一覧をサブウィンドウに表示する / display item at (y,x) in sub-windows
 */
void fix_floor_item_list(PlayerType *player_ptr, const int y, const int x)
{
    for (int j = 0; j < 8; j++) {
        if (!angband_terms[j]) {
            continue;
        }
        if (angband_terms[j]->never_fresh) {
            continue;
        }
        if (!(window_flag[j] & PW_FLOOR_ITEM_LIST)) {
            continue;
        }

        term_type *old = game_term;
        term_activate(angband_terms[j]);

        display_floor_item_list(player_ptr, y, x);
        term_fresh();

        term_activate(old);
    }
}

/*!
 * @brief 発見済みのアイテム一覧を作成し、表示する
 * @param プレイヤー情報への参照ポインタ
 */
static void display_found_item_list(PlayerType *player_ptr)
{
    // Term の行数を取得。
    TERM_LEN term_h;
    TERM_LEN term_w;
    term_get_size(&term_w, &term_h);

    if (term_h <= 0) {
        return;
    }

    auto *floor_ptr = player_ptr->current_floor_ptr;

    // 所持品一覧と同じ順にソートする
    // あらかじめfloor_ptr->o_list から↓項目を取り除く
    // bi_idが0
    // OM_FOUNDフラグが立っていない
    // ItemKindTypeがGOLD
    std::vector<ItemEntity *> found_item_list;
    for (auto &item : floor_ptr->o_list) {
        auto item_entity_ptr = &item;
        if (item_entity_ptr->bi_id > 0 && item_entity_ptr->marked.has(OmType::FOUND) && item_entity_ptr->bi_key.tval() != ItemKindType::GOLD) {
            found_item_list.push_back(item_entity_ptr);
        }
    }

    std::sort(
        found_item_list.begin(), found_item_list.end(),
        [player_ptr](ItemEntity *left, ItemEntity *right) -> bool {
            return object_sort_comp(player_ptr, left, left->get_price(), right);
        });

    term_clear();
    term_gotoxy(0, 0);

    // 先頭行を書く。
    term_addstr(-1, TERM_WHITE, _("発見済みのアイテム一覧", "Found items"));

    // 発見済みのアイテムを表示
    TERM_LEN term_y = 1;
    for (auto item : found_item_list) {
        // 途中で行数が足りなくなったら終了。
        if (term_y >= term_h) {
            break;
        }

        term_gotoxy(0, term_y);

        // アイテムシンボル表示
        const auto symbol_code = item->get_symbol();
        const std::string symbol = format(" %c ", symbol_code);
        const auto color_code_for_symbol = item->get_color();
        term_addstr(-1, color_code_for_symbol, symbol.data());

        // アイテム名表示
        char temp[512];
        describe_flavor(player_ptr, temp, item, 0);
        const std::string item_description(temp);
        const auto color_code_for_item = tval_to_attr[enum2i(item->bi_key.tval()) % 128];
        term_addstr(-1, color_code_for_item, item_description.data());

        // アイテム座標表示
        const std::string item_location = format("(X:%3d Y:%3d)", item->ix, item->iy);
        prt(item_location.data(), term_y, term_w - item_location.length() - 1);

        ++term_y;
    }
}

/*!
 * @brief 発見済みのアイテム一覧をサブウィンドウに表示する
 */
void fix_found_item_list(PlayerType *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        if (!angband_terms[j]) {
            continue;
        }
        if (angband_terms[j]->never_fresh) {
            continue;
        }
        if (none_bits(window_flag[j], PW_FOUND_ITEM_LIST)) {
            continue;
        }

        term_type *old = game_term;
        term_activate(angband_terms[j]);

        display_found_item_list(player_ptr);
        term_fresh();

        term_activate(old);
    }
}

/*!
 * @brief サブウィンドウに所持品、装備品リストの表示を行う /
 * Flip "inven" and "equip" in any sub-windows
 */
void toggle_inventory_equipment(PlayerType *player_ptr)
{
    for (auto i = 0U; i < angband_terms.size(); ++i) {
        if (!angband_terms[i]) {
            continue;
        }

        if (window_flag[i] & (PW_INVEN)) {
            window_flag[i] &= ~(PW_INVEN);
            window_flag[i] |= (PW_EQUIP);
            player_ptr->window_flags |= (PW_EQUIP);
            continue;
        }

        if (window_flag[i] & PW_EQUIP) {
            window_flag[i] &= ~(PW_EQUIP);
            window_flag[i] |= PW_INVEN;
            player_ptr->window_flags |= PW_INVEN;
        }
    }
}
