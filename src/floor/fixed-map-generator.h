#pragma once

#include "system/angband.h"
#include <string_view>

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
qtwg_type *initialize_quest_generator_type(qtwg_type *qg_ptr, int ymin, int xmin, int ymax, int xmax, int *y, int *x);
parse_error_type generate_fixed_map_floor(PlayerType *player_ptr, qtwg_type *qg_ptr);
/*! @brief 検証済みの1行を既存の凡例と生成範囲で適用し、行カーソルを進める. */
void apply_fixed_map_row(PlayerType *player_ptr, qtwg_type *qg_ptr, std::string_view row);
/*! @brief 生成済みの行・列からパネルサイズを設定し、開始位置を適用する. */
void apply_fixed_map_start(PlayerType *player_ptr, qtwg_type *qg_ptr, int start_y, int start_x);

/*!
 * @brief JSONCから読み込んだ固定クエストのレイアウトでフロアを生成する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param quest 対象クエスト (報酬解決のため参照)
 * @param fixed_map 生成に用いるレイアウト情報
 * @return エラーコード
 */
parse_error_type generate_quest_floor_from_json(PlayerType *player_ptr, QuestType &quest, const QuestFixedMap &fixed_map);
