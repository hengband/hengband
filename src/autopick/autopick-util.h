#pragma once

#include "system/angband.h"
#include <memory>
#include <string>
#include <vector>

#define MAX_LINELEN 1024
#define MAX_AUTOPICK_DEFAULT 200
#define MAX_YANK MAX_LINELEN
#define MAX_LINES 6000

#define PT_DEFAULT 0
#define PT_WITH_PNAME 1

#define MARK_MARK 0x01
#define MARK_BY_SHIFT 0x02

#define LSTAT_BYPASS 0x01
#define LSTAT_EXPRESSION 0x02
#define LSTAT_AUTOREGISTER 0x04

/*!
 * @struct autopick_type
 * @brief 自動拾い/破壊設定データの構造体 / A structure type for entry of auto-picker/destroyer
 */
struct autopick_type {
    std::string name = ""; /*!< 自動拾い/破壊定義の名称一致基準 / Items which have 'name' as part of its name match */
    std::string insc = ""; /*!< 対象となったアイテムに自動で刻む内容 / Items will be auto-inscribed as 'insc' */
    BIT_FLAGS flags[2]{}; /*!< キーワードに関する汎用的な条件フラグ / Misc. keyword to be matched */
    byte action = 0; /*!< 対象のアイテムを拾う/破壊/放置するかの指定フラグ / Auto-pickup or Destroy or Leave items */
    byte dice = 0; /*!< 武器のダイス値基準値 / Weapons which have more than 'dice' dice match */
    byte bonus = 0; /*!< アイテムのボーナス基準値 / Items which have more than 'bonus' magical bonus match */
    bool has(int flag) const;
    void add(int flag);
    void remove(int flag);
};

/*
 * Data struct for text editor
 */
class ItemEntity;
struct text_body_type {
    text_body_type(int cx, int cy); //!< @details 画面表示はX→Yの順番であることが多いので一応揃える.

    void adjust_cursor_column();
    void update_cursor_column_record(int com_id);

    int cx;
    int cy;
    int rec_cx = 0;
    int wid = 0;
    int hgt = 0;
    int upper = 0;
    int left = 0;
    int old_wid = -1;
    int old_hgt = -1;
    int old_cy = -1;
    int old_upper = -1;
    int old_left = -1;
    int mx = 0;
    int my = 0;
    byte mark = 0;

    ItemEntity *search_o_ptr = nullptr;
    std::string search_str = "";
    std::string last_destroyed = "";

    std::vector<std::string> yank{};
    bool yank_eol = false;

    std::vector<std::unique_ptr<std::string>> lines_list;
    byte states[MAX_LINES]{};

    uint16_t dirty_flags = 0;
    int dirty_line = -1;
    int filename_mode = 0;
    int old_com_id = 0;

    bool changed = false;
};

/*
 *  List for auto-picker/destroyer entries
 */
extern std::vector<autopick_type> autopick_list;
extern ItemEntity autopick_last_destroyed_object;

int get_com_id(char key);
void auto_inscribe_item(ItemEntity *o_ptr, int idx);
int count_line(text_body_type *tb);

/*!
 * @brief 最大行数を超えるかチェックする
 * @param count 行数
 * @retval true 最大行数を超える
 * @retval false 最大行数を超えない
 */
inline bool is_greater_autopick_max_line(int count)
{
    return count > MAX_LINES - 3;
}
