#pragma once
/*!
 * @file movement-execution.h
 * @brief プレイヤーの歩行処理実行ヘッダ
 */

class Direction;
class PlayerType;
void exe_movement(PlayerType *player_ptr, const Direction &dir, bool do_pickup, bool break_trap);
