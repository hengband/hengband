#pragma once

#include "system/angband.h"

class PlayerType;
void wiz_cure_all(PlayerType *player_ptr);
void wiz_create_item(PlayerType *player_ptr);
void wiz_create_named_art(PlayerType *player_ptr);
void wiz_change_status(PlayerType *player_ptr);
void wiz_create_feature(PlayerType *player_ptr);
void wiz_jump_to_dungeon(PlayerType *player_ptr);
void wiz_learn_items_all(PlayerType *player_ptr);
void wiz_reset_race(PlayerType *player_ptr);
void wiz_reset_class(PlayerType *player_ptr);
void wiz_reset_realms(PlayerType *player_ptr);
void wiz_dump_options(void);
void set_gametime(void);
void wiz_zap_surrounding_monsters(PlayerType *player_ptr);
void wiz_zap_floor_monsters(PlayerType *player_ptr);
extern void cheat_death(PlayerType *player_ptr);
