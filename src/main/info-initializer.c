#include "main/info-initializer.h"
#include "info-reader/fixed-map-parser.h"

/*!
 * @brief 基本情報読み込みのメインルーチン /
 * Initialize misc. values
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return エラーコード
 */
errr init_misc(player_type *player_ptr) { return parse_fixed_map(player_ptr, "misc.txt", 0, 0, 0, 0); }
