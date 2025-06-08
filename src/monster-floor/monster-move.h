#pragma once

#include "system/angband.h"
#include "util/point-2d.h"

constexpr auto BREAK_RUNE_PROTECTION = 550; /*!< 守りのルーンの強靭度 / Rune of protection resistance */
constexpr auto BREAK_RUNE_EXPLOSION = 299; /*!< 爆発のルーンの発動しやすさ / For explosive runes */

class Direction;
class MonsterEntity;
class MonsterMovementDirectionList;
class MonraceDefinition;
class PlayerType;
struct turn_flags;
void activate_explosive_rune(PlayerType *player_ptr, const Pos2D &pos, const MonraceDefinition &monrace);
bool process_monster_movement(PlayerType *player_ptr, turn_flags *turn_flags_ptr, const MonsterMovementDirectionList &mmdl, const Pos2D &pos, int *count);
void process_sound(PlayerType *player_ptr, MONSTER_IDX m_idx);
void process_speak(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION oy, POSITION ox, bool aware);
