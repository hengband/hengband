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
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class WildernessTerrain;
enum class DungeonId;
class WildernessGrid {
public:
    WildernessGrid() = default;
    WildernessTerrain terrain{};
    short town = 0;
    int road = 0;
    uint32_t seed = 0;
    int level = 0;
    DungeonId entrance{};
    std::string name = "";
};

extern std::vector<std::vector<WildernessGrid>> wilderness;
extern bool reinit_wilderness;

enum parse_error_type : int;
class PlayerType;
void wilderness_gen(PlayerType *player_ptr);
void wilderness_gen_small(PlayerType *player_ptr);
void init_wilderness_terrains();
void init_wilderness_encounter();
void seed_wilderness();
std::pair<parse_error_type, std::optional<Pos2D>> parse_line_wilderness(PlayerType *player_ptr, char *buf, int xmin, int xmax, const Pos2D &bottom_left);
bool change_wild_mode(PlayerType *player_ptr, bool encount);
