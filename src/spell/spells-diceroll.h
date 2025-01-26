#pragma once

class MonsterEntity;
class PlayerType;
bool common_saving_throw_control(PlayerType *player_ptr, int pow, MonsterEntity *m_ptr);
bool common_saving_throw_charm(PlayerType *player_ptr, int pow, MonsterEntity *m_ptr);
int beam_chance(PlayerType *player_ptr);
