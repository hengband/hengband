#pragma once

/*!
 * @struct autopick_type
 * @brief 自動拾い/破壊設定データの構造体 / A structure type for entry of auto-picker/destroyer
 */
typedef struct {
	concptr name;          /*!< 自動拾い/破壊定義の名称一致基準 / Items which have 'name' as part of its name match */
	concptr insc;          /*!< 対象となったアイテムに自動で刻む内容 / Items will be auto-inscribed as 'insc' */
	BIT_FLAGS flag[2];       /*!< キーワードに関する汎用的な条件フラグ / Misc. keyword to be matched */
	byte action;        /*!< 対象のアイテムを拾う/破壊/放置するかの指定フラグ / Auto-pickup or Destroy or Leave items */
	byte dice;          /*!< 武器のダイス値基準値 / Weapons which have more than 'dice' dice match */
	byte bonus;         /*!< アイテムのボーナス基準値 / Items which have more than 'bonus' magical bonus match */
} autopick_type;

/*
 *  List for auto-picker/destroyer entries
 */
extern int max_autopick;
extern int max_max_autopick;
extern autopick_type *autopick_list;

/* autopick.c */
extern void autopick_load_pref(bool disp_mes);
extern errr process_autopick_file_command(char *buf);
extern concptr autopick_line_from_entry(autopick_type *entry);
extern int is_autopick(object_type *o_ptr);
extern void autopick_alter_item(INVENTORY_IDX item, bool destroy);
extern void autopick_delayed_alter(void);
extern void autopick_pickup_items(grid_type *g_ptr);
extern bool autopick_autoregister(object_type *o_ptr);
extern void do_cmd_edit_autopick(void);
