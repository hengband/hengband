#pragma once

#include "system/angband.h"

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
    std::string name; /*!< 自動拾い/破壊定義の名称一致基準 / Items which have 'name' as part of its name match */
    std::string insc; /*!< 対象となったアイテムに自動で刻む内容 / Items will be auto-inscribed as 'insc' */
    BIT_FLAGS flag[2]; /*!< キーワードに関する汎用的な条件フラグ / Misc. keyword to be matched */
    byte action; /*!< 対象のアイテムを拾う/破壊/放置するかの指定フラグ / Auto-pickup or Destroy or Leave items */
    byte dice; /*!< 武器のダイス値基準値 / Weapons which have more than 'dice' dice match */
    byte bonus; /*!< アイテムのボーナス基準値 / Items which have more than 'bonus' magical bonus match */
};

/*
 * Struct for yank buffer
 */
struct chain_str_type {
    struct chain_str_type *next;
    char s[1];
};

/*
 * Data struct for text editor
 */
class ItemEntity;
;
struct text_body_type {
    int wid, hgt;
    int cx, cy;
    int upper, left;
    int old_wid, old_hgt;
    int old_cy;
    int old_upper, old_left;
    int mx, my;
    byte mark;

    ItemEntity *search_o_ptr;
    concptr search_str;
    concptr last_destroyed;

    chain_str_type *yank;
    bool yank_eol;

    std::vector<concptr> lines_list;
    byte states[MAX_LINES];

    uint16_t dirty_flags;
    int dirty_line;
    int filename_mode;
    int old_com_id;

    bool changed;
};

/*
 *  List for auto-picker/destroyer entries
 */
extern std::vector<autopick_type> autopick_list;
extern ItemEntity autopick_last_destroyed_object;

class PlayerType;
void free_text_lines(std::vector<concptr> &lines_list);
int get_com_id(char key);
void auto_inscribe_item(PlayerType *player_ptr, ItemEntity *o_ptr, int idx);
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
