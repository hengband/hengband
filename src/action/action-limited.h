#pragma once
/*!
 * @file action-limited.h
 * @brief プレイヤーの行動制約判定ヘッダ
 */

class PlayerType;
bool cmd_limit_cast(PlayerType *player_ptr);
bool cmd_limit_arena(PlayerType *player_ptr);
bool cmd_limit_time_walk(PlayerType *player_ptr);
bool cmd_limit_blind(PlayerType *player_ptr);
bool cmd_limit_confused(PlayerType *player_ptr);
bool cmd_limit_image(PlayerType *player_ptr);
bool cmd_limit_stun(PlayerType *player_ptr);
