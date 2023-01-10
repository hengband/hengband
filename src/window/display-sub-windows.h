#pragma once

#include "system/angband.h"

#include <vector>

class FloorType;
class PlayerType;
class ItemTester;
void fix_inventory(PlayerType *player_ptr);
void print_monster_list(FloorType *floor_ptr, const std::vector<MONSTER_IDX> &monster_list, TERM_LEN x, TERM_LEN y, TERM_LEN max_lines);
void fix_monster_list(PlayerType *player_ptr);
void fix_equip(PlayerType *player_ptr);
void fix_player(PlayerType *player_ptr);
void fix_message(void);
void fix_overhead(PlayerType *player_ptr);
void fix_dungeon(PlayerType *player_ptr);
void fix_monster(PlayerType *player_ptr);
void fix_object(PlayerType *player_ptr);
void fix_floor_item_list(PlayerType *player_ptr, const int y, const int x);
void fix_found_item_list(PlayerType *player_ptr);
void fix_spell(PlayerType *player_ptr);
void toggle_inventory_equipment(PlayerType *player_ptr);

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
