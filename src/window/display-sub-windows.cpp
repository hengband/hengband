#include "window/display-sub-windows.h"
#include "flavor/flavor-describer.h"
#include "floor/cave.h"
#include "game-option/option-flags.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "grid/feature.h"
#include "inventory/inventory-describer.h"
#include "inventory/inventory-util.h"
#include "locale/japanese.h"
#include "main/sound-of-music.h"
#include "mind/mind-explanations-table.h"
#include "mind/mind-info.h"
#include "mind/mind-sniper.h"
#include "mind/mind-types.h"
#include "monster-race/monster-race.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "object/item-tester-hooker.h"
#include "object/object-info.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player/player-status-flags.h"
#include "player/player-status-table.h"
#include "player/player-status.h"
#include "realm/realm-names-table.h"
#include "spell/spells-execution.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/terrain-type-definition.h"
#include "target/target-preparation.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "util/int-char-converter.h"
#include "view/display-lore.h"
#include "view/display-map.h"
#include "view/display-messages.h"
#include "view/display-player.h"
#include "view/object-describer.h"
#include "window/main-window-equipments.h"
#include "window/main-window-util.h"
#include "world/world.h"
#include <algorithm>
#include <concepts>
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
 * @brief サブウィンドウの描画を行う
 *
 * pw_flag で指定したウィンドウフラグが設定されているサブウィンドウに対し描画を行う。
 * 描画は display_func で指定したコールバック関数で行う。
 *
 * @param pw_flag 描画を行うフラグ
 * @param display_func 描画を行う関数
 */
static void display_sub_windows(SubWindowRedrawingFlag pw_flag, std::invocable auto display_func)
{
    auto current_term = game_term;

    for (auto i = 0U; i < angband_terms.size(); ++i) {
        auto term = angband_terms[i];
        if (term == nullptr) {
            continue;
        }

        if (!g_window_flags[i].has(pw_flag)) {
            continue;
        }

        term_activate(term);
        display_func();
        term_fresh();
    }

    term_activate(current_term);
}

/*!
 * @brief サブウィンドウに所持品一覧を表示する / Hack -- display inventory in sub-windows
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void fix_inventory(PlayerType *player_ptr)
{
    display_sub_windows(SubWindowRedrawingFlag::INVENTORY,
        [player_ptr] {
            display_inventory(player_ptr, *fix_item_tester);
        });
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

    term_erase(0, y);
    term_gotoxy(x, y);
    if (!r_ptr) {
        return;
    }
    if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        buf = format(_("%3s(覚%2d)", "%3s(%2d)"), MonsterRace(r_idx).is_bounty(true) ? "  W" : "  U", n_awake);
    } else {
        buf = format(_("%3d(覚%2d)", "%3d(%2d)"), n_same, n_awake);
    }
    term_addstr(-1, TERM_WHITE, buf);

    term_addstr(-1, TERM_WHITE, " ");
    term_add_bigch(r_ptr->x_attr, r_ptr->x_char);

    if (r_ptr->r_tkills && m_ptr->mflag2.has_not(MonsterConstantFlagType::KAGE)) {
        buf = format(" %2d", (int)r_ptr->level);
    } else {
        buf = " ??";
    }

    term_addstr(-1, TERM_WHITE, buf);

    term_addstr(-1, TERM_WHITE, format(" %s ", r_ptr->name.data()));
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
        term_erase(0, line);
    }
}

static void print_pet_list_oneline(PlayerType *player_ptr, const MonsterEntity &monster, TERM_LEN x, TERM_LEN y, TERM_LEN width)
{
    const auto &monrace = monster.get_appearance_monrace();
    const auto name = monster_desc(player_ptr, &monster, MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE | MD_NO_OWNER);
    const auto &[bar_color, bar_len] = monster.get_hp_bar_data();
    const auto is_visible = monster.ml && !player_ptr->effects()->hallucination()->is_hallucinated();

    term_erase(0, y);
    if (is_visible) {
        term_putstr(x, y, -1, TERM_WHITE, "[----------]");
        term_putstr(x + 1, y, bar_len, bar_color, "**********");
    }

    term_gotoxy(x + 13, y);
    term_add_bigch(monrace.x_attr, monrace.x_char);
    term_addstr(-1, TERM_WHITE, " ");
    term_addstr(-1, TERM_WHITE, name);

    if (width >= 50) {
        const auto location = format(" (X:%3d Y:%3d)", monster.fx, monster.fy);
        prt(is_visible ? location : "", y, width - location.length());
    }
}

static void print_pet_list(PlayerType *player_ptr, const std::vector<MONSTER_IDX> &pets, TERM_LEN x, TERM_LEN y, TERM_LEN width, TERM_LEN height)
{
    for (auto n = 0U; n < pets.size(); ++n) {
        const auto &monster = player_ptr->current_floor_ptr->m_list[pets[n]];
        const int line = y + n;

        print_pet_list_oneline(player_ptr, monster, x, line, width);

        if ((line == height - 2) && (n < pets.size() - 2)) {
            term_erase(0, line + 1);
            term_putstr(x, line + 1, -1, TERM_WHITE, "-- and more --");
            break;
        }
    }

    for (int n = pets.size(); n < height; ++n) {
        term_erase(0, y + n);
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

    display_sub_windows(SubWindowRedrawingFlag::SIGHT_MONSTERS,
        [player_ptr, &once] {
            const auto &[wid, hgt] = term_get_size();
            std::call_once(once, target_sensing_monsters_prepare, player_ptr, monster_list);
            print_monster_list(player_ptr->current_floor_ptr, monster_list, 0, 0, hgt);
        });

    if (use_music && has_monster_music) {
        std::call_once(once, target_sensing_monsters_prepare, player_ptr, monster_list);
        select_monster_music(player_ptr, monster_list);
    }
}

/*!
 * @brief 視界内のペットのリストをサブウィンドウに表示する
 */
void fix_pet_list(PlayerType *player_ptr)
{
    display_sub_windows(SubWindowRedrawingFlag::PETS,
        [player_ptr] {
            const auto &[wid, hgt] = term_get_size();
            const auto pets = target_pets_prepare(player_ptr);
            print_pet_list(player_ptr, pets, 0, 0, wid, hgt);
        });
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

    const auto &[wid, hgt] = term_get_size();
    byte attr = TERM_WHITE;
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
        term_erase(cur_col, cur_row);
        term_putstr(0, cur_row, cur_col, TERM_WHITE, tmp_val);

        std::string item_name;
        auto is_two_handed = (i == INVEN_MAIN_HAND) && can_attack_with_sub_hand(player_ptr);
        is_two_handed |= (i == INVEN_SUB_HAND) && can_attack_with_main_hand(player_ptr);
        if (is_two_handed && has_two_handed_weapons(player_ptr)) {
            item_name = _("(武器を両手持ち)", "(wielding with two-hands)");
            attr = TERM_WHITE;
        } else {
            item_name = describe_flavor(player_ptr, o_ptr, 0);
            attr = tval_to_attr[enum2i(o_ptr->bi_key.tval()) % 128];
        }

        int n = item_name.length();
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

        term_putstr(cur_col, cur_row, n, attr, item_name);
        if (show_weights) {
            int wgt = o_ptr->weight * o_ptr->number;
            tmp_val = format(_("%3d.%1d kg", "%3d.%1d lb"), _(lb_to_kg_integer(wgt), wgt / 10), _(lb_to_kg_fraction(wgt), wgt % 10));
            prt(tmp_val, cur_row, wid - (show_labels ? 28 : 9));
        }

        if (show_labels) {
            term_putstr(wid - 20, cur_row, -1, TERM_WHITE, " <-- ");
            prt(mention_use(player_ptr, i), cur_row, wid - 15);
        }
    }

    for (int i = INVEN_TOTAL - INVEN_MAIN_HAND; i < hgt; i++) {
        term_erase(0, i);
    }
}

/*!
 * @brief 現在の装備品をサブウィンドウに表示する /
 * Hack -- display equipment in sub-windows
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void fix_equip(PlayerType *player_ptr)
{
    display_sub_windows(SubWindowRedrawingFlag::EQUIPMENT,
        [player_ptr] {
            display_equipment(player_ptr, *fix_item_tester);
        });
}

/*!
 * @brief 現在のプレイヤーステータスをサブウィンドウに表示する /
 * @param player_ptr プレイヤーへの参照ポインタ
 * Hack -- display character in sub-windows
 */
void fix_player(PlayerType *player_ptr)
{
    update_playtime();
    display_sub_windows(SubWindowRedrawingFlag::PLAYER,
        [player_ptr] {
            display_player(player_ptr, 0);
        });
}

/*!
 * @brief ゲームメッセージ履歴をサブウィンドウに表示する /
 * Hack -- display recent messages in sub-windows
 * Adjust for width and split messages
 */
void fix_message(void)
{
    display_sub_windows(SubWindowRedrawingFlag::MESSAGE,
        [] {
            const auto &[wid, hgt] = term_get_size();
            for (short i = 0; i < hgt; i++) {
                term_putstr(0, (hgt - 1) - i, -1, (byte)((i < now_message) ? TERM_WHITE : TERM_SLATE), *message_str(i));
                TERM_LEN x, y;
                term_locate(&x, &y);
                term_erase(x, y);
            }
        });
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
    display_sub_windows(SubWindowRedrawingFlag::OVERHEAD,
        [player_ptr] {
            const auto &[wid, hgt] = term_get_size();
            if (wid > COL_MAP + 2 && hgt > ROW_MAP + 2) {
                int cy, cx;
                display_map(player_ptr, &cy, &cx);
            }
        });
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
                const auto &terrain = TerrainList::get_instance()[feat_none];
                a = terrain.x_attr[F_LIT_STANDARD];
                c = terrain.x_char[F_LIT_STANDARD];
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
    display_sub_windows(SubWindowRedrawingFlag::DUNGEON,
        [player_ptr] {
            display_dungeon(player_ptr);
        });
}

/*!
 * @brief モンスターの思い出をサブウィンドウに表示する /
 * Hack -- display dungeon view in sub-windows
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void fix_monster(PlayerType *player_ptr)
{
    if (!MonsterRace(player_ptr->monster_race_idx).is_valid()) {
        return;
    }
    display_sub_windows(SubWindowRedrawingFlag::MONSTER_LORE,
        [player_ptr] {
            display_roff(player_ptr);
        });
}

/*!
 * @brief ベースアイテム情報をサブウィンドウに表示する /
 * Hack -- display object recall in sub-windows
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void fix_object(PlayerType *player_ptr)
{
    display_sub_windows(SubWindowRedrawingFlag::ITEM_KNOWLEDGE,
        [player_ptr] {
            display_koff(player_ptr);
        });
}

/*!
 * @brief 床上のモンスター情報を返す
 * @param floor_ptr 階の情報への参照ポインタ
 * @param grid_prt 座標グリッドの情報への参照ポインタ
 * @return モンスターが見える場合にはモンスター情報への参照ポインタ、それ以外はnullptr
 * @details
 * Lookコマンドでカーソルを合わせた場合に合わせてミミックは考慮しない。
 */
static const MonsterEntity *monster_on_floor_items(FloorType *floor_ptr, const Grid *g_ptr)
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
static void display_floor_item_list(PlayerType *player_ptr, const Pos2D &pos)
{
    const auto &[wid, hgt] = term_get_size();
    if (hgt <= 0) {
        return;
    }

    term_clear();
    term_gotoxy(0, 0);

    auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(pos);
    std::string line;

    // 先頭行を書く。
    auto is_hallucinated = player_ptr->effects()->hallucination()->is_hallucinated();
    if (player_ptr->is_located_at(pos)) {
        line = format(_("(X:%03d Y:%03d) あなたの足元のアイテム一覧", "Items at (%03d,%03d) under you"), pos.x, pos.y);
    } else if (const auto *m_ptr = monster_on_floor_items(&floor, &grid); m_ptr != nullptr) {
        if (is_hallucinated) {
            line = format(_("(X:%03d Y:%03d) 何か奇妙な物の足元の発見済みアイテム一覧", "Found items at (%03d,%03d) under something strange"), pos.x, pos.y);
        } else {
            const MonsterRaceInfo *const r_ptr = &m_ptr->get_appearance_monrace();
            line = format(_("(X:%03d Y:%03d) %sの足元の発見済みアイテム一覧", "Found items at (%03d,%03d) under %s"), pos.x, pos.y, r_ptr->name.data());
        }
    } else {
        const auto &terrain = grid.get_terrain();
        const auto fn = terrain.name.data();
        std::string buf;
        if (terrain.flags.has(TerrainCharacteristics::STORE) || (terrain.flags.has(TerrainCharacteristics::BLDG) && !floor.inside_arena)) {
            buf = format(_("%sの入口", "on the entrance of %s"), fn);
        } else if (terrain.flags.has(TerrainCharacteristics::WALL)) {
            buf = format(_("%sの中", "in %s"), fn);
        } else {
            buf = format(_("%s", "on %s"), fn);
        }

        line = format(_("(X:%03d Y:%03d) %sの上の発見済みアイテム一覧", "Found items at (X:%03d Y:%03d) %s"), pos.x, pos.y, buf.data());
    }
    term_addstr(-1, TERM_WHITE, line);

    // (y,x) のアイテムを1行に1個ずつ書く。
    TERM_LEN term_y = 1;
    for (const auto o_idx : grid.o_idx_list) {
        const auto &item = floor.o_list[o_idx];
        const auto tval = item.bi_key.tval();
        if (item.marked.has_not(OmType::FOUND) || tval == ItemKindType::GOLD) {
            continue;
        }

        // 途中で行数が足りなくなったら最終行にその旨追記して終了。
        if (term_y >= hgt) {
            term_addstr(-1, TERM_WHITE, "-- more --");
            break;
        }

        term_gotoxy(0, term_y);

        if (is_hallucinated) {
            term_addstr(-1, TERM_WHITE, _("何か奇妙な物", "something strange"));
        } else {
            const auto item_name = describe_flavor(player_ptr, &item, 0);
            TERM_COLOR attr = tval_to_attr[enum2i(tval) % 128];
            term_addstr(-1, attr, item_name);
        }

        ++term_y;
    }
}

/*!
 * @brief (y,x) のアイテム一覧をサブウィンドウに表示する / display item at (y,x) in sub-windows
 */
void fix_floor_item_list(PlayerType *player_ptr, const int y, const int x)
{
    display_sub_windows(SubWindowRedrawingFlag::FLOOR_ITEMS,
        [player_ptr, y, x] {
            display_floor_item_list(player_ptr, { y, x });
        });
}

/*!
 * @brief 発見済みのアイテム一覧を作成し、表示する
 * @param プレイヤー情報への参照ポインタ
 */
static void display_found_item_list(PlayerType *player_ptr)
{
    const auto &[wid, hgt] = term_get_size();
    if (hgt <= 0) {
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
        const auto is_item_to_display =
            item.is_valid() && (item.number > 0) &&
            item.marked.has(OmType::FOUND) && (item.bi_key.tval() != ItemKindType::GOLD);

        if (is_item_to_display) {
            found_item_list.push_back(&item);
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
        if (term_y >= hgt) {
            break;
        }

        term_gotoxy(0, term_y);

        // アイテムシンボル表示
        const auto symbol_code = item->get_symbol();
        const auto symbol = format(" %c ", symbol_code);
        const auto color_code_for_symbol = item->get_color();
        term_addstr(-1, color_code_for_symbol, symbol);

        const auto item_name = describe_flavor(player_ptr, item, 0);
        const auto color_code_for_item = tval_to_attr[enum2i(item->bi_key.tval()) % 128];
        term_addstr(-1, color_code_for_item, item_name);

        // アイテム座標表示
        const auto item_location = format("(X:%3d Y:%3d)", item->ix, item->iy);
        prt(item_location, term_y, wid - item_location.length() - 1);

        ++term_y;
    }
}

/*!
 * @brief 発見済みのアイテム一覧をサブウィンドウに表示する
 */
void fix_found_item_list(PlayerType *player_ptr)
{
    display_sub_windows(SubWindowRedrawingFlag::FOUND_ITEMS,
        [player_ptr] {
            display_found_item_list(player_ptr);
        });
}

/*!
 * @brief プレイヤーの全既知呪文を表示する / Display all known spells in a window
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * Need to analyze size of the window.
 * Need more color coding.
 */
static void display_spell_list(PlayerType *player_ptr)
{
    TERM_LEN y, x;
    int m[9]{};
    const magic_type *s_ptr;
    GAME_TEXT name[MAX_NLEN];

    clear_from(0);

    PlayerClass pc(player_ptr);
    if (pc.is_every_magic()) {
        return;
    }

    if (pc.equals(PlayerClassType::SNIPER)) {
        display_snipe_list(player_ptr);
        return;
    }

    if (pc.has_listed_magics()) {
        PERCENTAGE minfail = 0;
        PLAYER_LEVEL plev = player_ptr->lev;
        PERCENTAGE chance = 0;
        mind_type spell;
        MindKindType use_mind;
        bool use_hp = false;

        y = 1;
        x = 1;

        prt("", y, x);
        put_str(_("名前", "Name"), y, x + 5);
        put_str(_("Lv   MP 失率 効果", "Lv Mana Fail Info"), y, x + 35);

        switch (player_ptr->pclass) {
        case PlayerClassType::MINDCRAFTER:
            use_mind = MindKindType::MINDCRAFTER;
            break;
        case PlayerClassType::FORCETRAINER:
            use_mind = MindKindType::KI;
            break;
        case PlayerClassType::BERSERKER:
            use_mind = MindKindType::BERSERKER;
            use_hp = true;
            break;
        case PlayerClassType::MIRROR_MASTER:
            use_mind = MindKindType::MIRROR_MASTER;
            break;
        case PlayerClassType::NINJA:
            use_mind = MindKindType::NINJUTSU;
            use_hp = true;
            break;
        case PlayerClassType::ELEMENTALIST:
            use_mind = MindKindType::ELEMENTAL;
            break;
        default:
            use_mind = MindKindType::MINDCRAFTER;
            break;
        }

        for (int i = 0; i < MAX_MIND_POWERS; i++) {
            byte a = TERM_WHITE;
            spell = mind_powers[static_cast<int>(use_mind)].info[i];
            if (spell.min_lev > plev) {
                break;
            }

            chance = spell.fail;
            chance -= 3 * (player_ptr->lev - spell.min_lev);
            chance -= 3 * (adj_mag_stat[player_ptr->stat_index[mp_ptr->spell_stat]] - 1);
            if (!use_hp) {
                if (spell.mana_cost > player_ptr->csp) {
                    chance += 5 * (spell.mana_cost - player_ptr->csp);
                    a = TERM_ORANGE;
                }
            } else {
                if (spell.mana_cost > player_ptr->chp) {
                    chance += 100;
                    a = TERM_RED;
                }
            }

            minfail = adj_mag_fail[player_ptr->stat_index[mp_ptr->spell_stat]];
            if (chance < minfail) {
                chance = minfail;
            }

            auto player_stun = player_ptr->effects()->stun();
            chance += player_stun->get_magic_chance_penalty();
            if (chance > 95) {
                chance = 95;
            }

            const auto comment = mindcraft_info(player_ptr, use_mind, i);
            constexpr auto fmt = "  %c) %-30s%2d %4d %3d%%%s";
            term_putstr(x, y + i + 1, -1, a, format(fmt, I2A(i), spell.name, spell.min_lev, spell.mana_cost, chance, comment.data()));
        }

        return;
    }

    if (REALM_NONE == player_ptr->realm1) {
        return;
    }

    for (int j = 0; j < ((player_ptr->realm2 > REALM_NONE) ? 2 : 1); j++) {
        m[j] = 0;
        y = (j < 3) ? 0 : (m[j - 3] + 2);
        x = 27 * (j % 3);
        int n = 0;
        for (int i = 0; i < 32; i++) {
            byte a = TERM_WHITE;

            if (!is_magic((j < 1) ? player_ptr->realm1 : player_ptr->realm2)) {
                s_ptr = &technic_info[((j < 1) ? player_ptr->realm1 : player_ptr->realm2) - MIN_TECHNIC][i % 32];
            } else {
                s_ptr = &mp_ptr->info[((j < 1) ? player_ptr->realm1 : player_ptr->realm2) - 1][i % 32];
            }

            const auto realm = (j < 1) ? player_ptr->realm1 : player_ptr->realm2;
            const auto spell_name = exe_spell(player_ptr, realm, i % 32, SpellProcessType::NAME);
            strcpy(name, spell_name->data());

            if (s_ptr->slevel >= 99) {
                strcpy(name, _("(判読不能)", "(illegible)"));
                a = TERM_L_DARK;
            } else if ((j < 1) ? ((player_ptr->spell_forgotten1 & (1UL << i))) : ((player_ptr->spell_forgotten2 & (1UL << (i % 32))))) {
                a = TERM_ORANGE;
            } else if (!((j < 1) ? (player_ptr->spell_learned1 & (1UL << i)) : (player_ptr->spell_learned2 & (1UL << (i % 32))))) {
                a = TERM_RED;
            } else if (!((j < 1) ? (player_ptr->spell_worked1 & (1UL << i)) : (player_ptr->spell_worked2 & (1UL << (i % 32))))) {
                a = TERM_YELLOW;
            }

            m[j] = y + n;
            term_putstr(x, m[j], -1, a, format("%c/%c) %-20.20s", I2A(n / 8), I2A(n % 8), name));
            n++;
        }
    }
}

/*!
 * @brief 現在の習得済魔法をサブウィンドウに表示する /
 * @param player_ptr プレイヤーへの参照ポインタ
 * Hack -- display spells in sub-windows
 */
void fix_spell(PlayerType *player_ptr)
{
    display_sub_windows(SubWindowRedrawingFlag::SPELL,
        [player_ptr] {
            display_spell_list(player_ptr);
        });
}

/*!
 * @brief サブウィンドウに所持品、装備品リストの表示を行う
 */
void toggle_inventory_equipment()
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    for (auto i = 0U; i < angband_terms.size(); ++i) {
        if (!angband_terms[i]) {
            continue;
        }

        if (g_window_flags[i].has(SubWindowRedrawingFlag::INVENTORY)) {
            g_window_flags[i].reset(SubWindowRedrawingFlag::INVENTORY);
            g_window_flags[i].set(SubWindowRedrawingFlag::EQUIPMENT);
            rfu.set_flag(SubWindowRedrawingFlag::EQUIPMENT);
            continue;
        }

        if (g_window_flags[i].has(SubWindowRedrawingFlag::EQUIPMENT)) {
            g_window_flags[i].reset(SubWindowRedrawingFlag::EQUIPMENT);
            g_window_flags[i].set(SubWindowRedrawingFlag::INVENTORY);
            rfu.set_flag(SubWindowRedrawingFlag::INVENTORY);
        }
    }
}
