#pragma once

#include "system/angband.h"

struct player_type;
void wiz_cure_all(player_type *player_ptr);
void wiz_create_item(player_type *player_ptr);
void wiz_create_named_art(player_type *player_ptr);
void wiz_change_status(player_type *player_ptr);
void wiz_create_feature(player_type *player_ptr);
void wiz_jump_to_dungeon(player_type *player_ptr);
void wiz_learn_items_all(player_type *player_ptr);
void wiz_reset_race(player_type *player_ptr);
void wiz_reset_class(player_type *player_ptr);
void wiz_reset_realms(player_type *player_ptr);
void wiz_dump_options(void);
void set_gametime(void);
void wiz_zap_surrounding_monsters(player_type *player_ptr);
void wiz_zap_floor_monsters(player_type *player_ptr);
extern void cheat_death(player_type *player_ptr);
