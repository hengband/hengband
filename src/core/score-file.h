/*!
 * @file score-file.h
 * @brief スコアファイル (lib/apex/scores.json) の読み書き
 * @details
 * スコアファイルは、形式の版 ("version") とスコアの高い順に並べた記録の配列 ("scores") を持つ
 * UTF-8 の JSON である。記録の文字列は UTF-8 のまま読み書きし、内部の文字コードには表示するときに変換する。
 * 版は互換性の無い変更をしたときだけ上げ、知らない欄は読むときに無視する。
 */
#pragma once

#include "core/score-util.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <tl/optional.hpp>
#include <vector>

std::string scores_to_json(const std::vector<HighScore> &scores);
tl::optional<std::vector<HighScore>> scores_from_json(std::string_view text);
int find_score_rank(const std::vector<HighScore> &scores, int64_t points);
int insert_score(std::vector<HighScore> &scores, const HighScore &score);
HighScore migrate_raw_score(const high_score &raw);

tl::optional<std::vector<HighScore>> read_score_file();
bool can_add_score_to_file();
tl::optional<std::vector<HighScore>> add_score_to_file(const HighScore &score, int &rank);
bool prepare_score_file();
