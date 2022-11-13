#pragma once

#include "system/angband.h"

constexpr auto BREAK_RUNE_PROTECTION = 550; /*!< 守りのルーンの強靭度 / Rune of protection resistance */
constexpr auto BREAK_RUNE_EXPLOSION = 299; /*!< 爆発のルーンの発動しやすさ / For explosive runes */

struct monster_type;
class PlayerType;
struct turn_flags;
bool process_monster_movement(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, DIRECTION *mm, POSITION oy, POSITION ox, int *count);
void process_speak_sound(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION oy, POSITION ox, bool aware);
void set_target(monster_type *m_ptr, POSITION y, POSITION x);
void reset_target(monster_type *m_ptr);
