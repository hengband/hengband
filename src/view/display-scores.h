#pragma once

struct high_score;
void display_scores(int from, int to, int note, high_score *score);

#ifndef WINDOWS
void display_scores(int from, int to);
#endif
