#pragma once

#include <vector>

struct HighScore;
void display_scores(const std::vector<HighScore> &scores, int from, int to, int note, const HighScore *score);

#ifndef _WIN32
void display_scores(int from, int to);
#endif
