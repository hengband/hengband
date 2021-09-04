#pragma once

typedef struct building_type building_type;
struct player_type;
bool is_owner(player_type *player_ptr, building_type *bldg);
bool is_member(player_type *player_ptr, building_type *bldg);
void display_buikding_service(player_type *player_ptr, building_type *bldg);
