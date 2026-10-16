#pragma once

#include "system/angband.h"

/*!
 * @brief 感知のメッセージを出す条件
 */
enum class DetectionNotice {
    ALWAYS, //!< 対象があれば出す
    NEW_ONLY, //!< 新しく感知した対象があるときだけ出す (感知を続ける効果用)
};

class PlayerType;
bool detect_traps(PlayerType *player_ptr, POSITION range, bool known, DetectionNotice notice = DetectionNotice::ALWAYS);
bool detect_doors(PlayerType *player_ptr, POSITION range);
bool detect_stairs(PlayerType *player_ptr, POSITION range);
bool detect_treasure(PlayerType *player_ptr, POSITION range);
bool detect_objects_gold(PlayerType *player_ptr, POSITION range);
bool detect_objects_normal(PlayerType *player_ptr, POSITION range);
bool detect_objects_magic(PlayerType *player_ptr, POSITION range);
bool detect_monsters_normal(PlayerType *player_ptr, POSITION range);
bool detect_monsters_invis(PlayerType *player_ptr, POSITION range);
bool detect_monsters_evil(PlayerType *player_ptr, POSITION range, DetectionNotice notice = DetectionNotice::ALWAYS);
bool detect_monsters_xxx(PlayerType *player_ptr, POSITION range, uint32_t match_flag);
bool detect_monsters_string(PlayerType *player_ptr, POSITION range, concptr);
bool detect_monsters_nonliving(PlayerType *player_ptr, POSITION range);
bool detect_monsters_mind(PlayerType *player_ptr, POSITION range);
bool detect_all(PlayerType *player_ptr, POSITION range);
