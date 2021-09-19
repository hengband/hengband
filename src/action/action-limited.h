#pragma once
/*!
 * @file action-limited.h
 * @brief プレイヤーの行動制約判定ヘッダ
 */

struct player_type;
bool cmd_limit_cast(player_type *player_ptr);
bool cmd_limit_arena(player_type *player_ptr);
bool cmd_limit_time_walk(player_type *player_ptr);
bool cmd_limit_blind(player_type *player_ptr);
bool cmd_limit_confused(player_type *player_ptr);
bool cmd_limit_image(player_type *player_ptr);
bool cmd_limit_stun(player_type *player_ptr);
