/*!
 * @brief 荒野マップの生成とルール管理定義
 * @date 2025/02/01
 * @author
 * Robert A. Koeneke, 1983
 * James E. Wilson, 1989
 * Deskull, 2013
 * Hourier, 2025
 */

#pragma once

#include "util/point-2d.h"
#include <tl/expected.hpp>

enum parse_error_type : int;
class PlayerType;
void wilderness_gen(PlayerType *player_ptr);
void wilderness_gen_small(PlayerType *player_ptr);
void init_wilderness_terrains();
tl::expected<Pos2D, parse_error_type> parse_line_wilderness(char *line, int xmin, int xmax, const Pos2D &pos_parsing);
bool change_wild_mode(PlayerType *player_ptr, bool encount);
