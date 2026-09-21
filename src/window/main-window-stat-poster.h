#pragma once

#include "term/term-color-types.h"
#include "view/status-bars-table.h"
#include <bitset>
#include <string>
#include <utility>

using StatusBarFlags = std::bitset<MAX_STAT_BARS>;

class PlayerType;
void print_stat(PlayerType *player_ptr, int stat);
void print_cut(PlayerType *player_ptr);
void print_stun(PlayerType *player_ptr);
void print_hunger(PlayerType *player_ptr);
void print_state(PlayerType *player_ptr);
std::pair<std::string, TERM_COLOR> describe_player_state(PlayerType *player_ptr);
void print_speed(PlayerType *player_ptr);
std::pair<std::string, TERM_COLOR> describe_player_speed(PlayerType *player_ptr);
void print_study(PlayerType *player_ptr);
void print_imitation(PlayerType *player_ptr);
StatusBarFlags collect_status_bar_flags(PlayerType *player_ptr);
void print_status(PlayerType *player_ptr);
void print_frame_extra(PlayerType *player_ptr);
