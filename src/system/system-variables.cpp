/*!
 * @brief グローバル変数の残骸
 * @date 2013/12/31
 * @todo 呼び出し関係を良く読んで消す方針で進めたい.
 */

#include "system/system-variables.h"

/*!
 * @todo どこからも呼ばれていない。main関数辺りに移設するか、そもそもコメントでいいと思われる
 * コピーライト情報 / Link a copyright message into the executable
 */
/*
const concptr copyright[5] =
{
        "Copyright (c) 1989 James E. Wilson, Robert A. Keoneke",
        "",
        "This software may be copied and distributed for educational, research,",
        "and not for profit purposes provided that this copyright and statement",
        "are included in all such copies."
};
*/

concptr ANGBAND_SYS = "xxx";
concptr ANGBAND_KEYBOARD = _("JAPAN", "0");
concptr ANGBAND_GRAF = "ascii";
init_flags_type init_flags; //!< @todo このグローバル変数何とかしたい

/*!
 * Function hook to restrict "get_obj_num_prep()" function
 */
bool (*get_obj_num_hook)(KIND_OBJECT_IDX k_idx);

OBJECT_SUBTYPE_VALUE coin_type;
