#pragma once

#include "info-reader/parse-error-types.h"
#include "system/angband.h"
#include <string_view>
#include <tl/expected.hpp>

// Quest/Town/World Generator
struct qtwg_type {
    int ymin;
    int xmin;
    int ymax;
    int xmax;
    int *y;
    int *x;
};

class PlayerType;
class QuestType;
struct QuestFixedMap;
qtwg_type *initialize_quest_generator_type(qtwg_type *qg_ptr, int ymin, int xmin, int ymax, int xmax, int *y, int *x);
tl::expected<void, ParseErrorType> apply_fixed_map_row(PlayerType *player_ptr, qtwg_type *qg_ptr, std::string_view row);
void apply_fixed_map_start(PlayerType *player_ptr, qtwg_type *qg_ptr, int start_y, int start_x);
tl::expected<void, ParseErrorType> generate_quest_floor_from_json(PlayerType *player_ptr, QuestType &quest, const QuestFixedMap &fixed_map);
