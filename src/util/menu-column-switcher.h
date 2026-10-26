/*!
 * @file menu-column-switcher.h
 * @brief 2 列に並べたメニューで、カーソルを反対の列に移す処理
 */

#pragma once

#include <concepts>

/*!
 * @brief 2 列に並べたメニューで、カーソルを反対の列の行に移す
 * @param menu_line 今のカーソルの行 (1 〜 size)
 * @param is_left_key 左への移動のキー (4・h など) が押されたか。左の列から移るときに探す向きだけを決め、
 * 右の列から移るときは押したキーによらず上へ探す
 * @param size メニューの行の数 (2 以上)。1 〜 size / 2 行目を左の列、残りを右の列に並べる
 * @param is_filled 0 から数えた行の番号を受け、その行に選べる項目があるかを返す関数
 * @return 移った後のカーソルの行 (1 〜 size)
 * @details 反対の列の同じ高さの行に移り、そこが空なら上下に進んで選べる項目のある行を探す。
 * 左の列から移るときは下へ (is_left_key なら上へ)、右の列から移るときは上へ進み、一覧の端に着いたら
 * 向きを変える。端に着いたら向きを変えてから進むので、一覧の外 (0 行目や size + 1 行目) は調べない。
 * 選べる項目のある行が 1 つも無いと止まらないので、呼ぶ側は項目が 1 つ以上ある状態で呼ぶ。
 */
template <std::predicate<int> Pred>
int switch_menu_column(int menu_line, bool is_left_key, int size, Pred is_filled)
{
    auto reverse = is_left_key;
    const auto half = size / 2;
    if (menu_line > half) {
        menu_line -= half;
        reverse = true;
    } else {
        menu_line += half;
    }

    while (!is_filled(menu_line - 1)) {
        if (reverse && (menu_line <= 1)) {
            reverse = false;
        } else if (!reverse && (menu_line >= size)) {
            reverse = true;
        }

        menu_line += reverse ? -1 : 1;
    }

    return menu_line;
}
