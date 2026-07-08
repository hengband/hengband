#pragma once

#include "system/angband.h"

// Quest/Town/World Generator
struct qtwg_type {
    char *buf;
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
enum parse_error_type : int;
typedef parse_error_type (*process_dungeon_file_pf)(PlayerType *, std::string_view, int, int, int, int);

qtwg_type *initialize_quest_generator_type(qtwg_type *qg_ptr, int ymin, int xmin, int ymax, int xmax, int *y, int *x);
parse_error_type generate_fixed_map_floor(PlayerType *player_ptr, qtwg_type *qg_ptr, process_dungeon_file_pf parse_fixed_map);

/*!
 * @brief JSONCから読み込んだ固定クエストのレイアウトでフロアを生成する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param quest 対象クエスト (報酬解決のため参照)
 * @param fixed_map 生成に用いるレイアウト情報
 * @return エラーコード
 */
parse_error_type generate_quest_floor_from_json(PlayerType *player_ptr, QuestType &quest, const QuestFixedMap &fixed_map);
