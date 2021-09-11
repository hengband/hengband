#pragma once

#include "system/angband.h"

#include <vector>

struct floor_type;
struct player_type;
class ItemTester;
void fix_inventory(player_type *player_ptr);
void print_monster_list(floor_type *floor_ptr, const std::vector<MONSTER_IDX> &monster_list, TERM_LEN x, TERM_LEN y, TERM_LEN max_lines);
void fix_monster_list(player_type *player_ptr);
void fix_equip(player_type *player_ptr);
void fix_player(player_type *player_ptr);
void fix_message(void);
void fix_overhead(player_type *player_ptr);
void fix_dungeon(player_type *player_ptr);
void fix_monster(player_type *player_ptr);
void fix_object(player_type *player_ptr);
void fix_floor_item_list(player_type *player_ptr, const int y, const int x);
void toggle_inventory_equipment(player_type *player_ptr);

/*!
 * @brief サブウィンドウ表示用の ItemTester オブジェクトを設定するクラス
 *
 * @details オブジェクトが生存している間コンストラクタで指定した ItemTester オブジェクトにより
 * アイテム表示が絞り込まれるようになる。
 * オブジェクトが破棄されるとデストラクタによりサブウィンドウ表示用 ItemTester オブジェクトは
 * AllMatchItemTester(全てのアイテムを表示)のインスタンスがセットされる。
 * なお、現状の仕様はアイテム表示の絞り込みとは、アイテムの先頭に表示されるアルファベットの
 * 選択記号が表示されるか否かの違いであり、アイテムそのものの表示が絞り込まれるわけではない。
 */
class FixItemTesterSetter {
public:
    explicit FixItemTesterSetter(const ItemTester &item_tester);
    ~FixItemTesterSetter();

    FixItemTesterSetter(const FixItemTesterSetter &) = delete;
    FixItemTesterSetter &operator=(const FixItemTesterSetter &) = delete;
    FixItemTesterSetter(FixItemTesterSetter &&) = delete;
    FixItemTesterSetter &operator=(FixItemTesterSetter &&) = delete;
};
