#pragma once

class MonsterEntity;
class PlayerType;
bool common_saving_throw_control(PlayerType *player_ptr, int pow, const MonsterEntity &monster);
bool common_saving_throw_charm(PlayerType *player_ptr, int pow, const MonsterEntity &monster);
int beam_chance(PlayerType *player_ptr);
