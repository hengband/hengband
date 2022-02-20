#pragma once

struct building_type;
class PlayerType;
bool is_owner(PlayerType *player_ptr, building_type *bldg);
bool is_member(PlayerType *player_ptr, building_type *bldg);
void display_buikding_service(PlayerType *player_ptr, building_type *bldg);
