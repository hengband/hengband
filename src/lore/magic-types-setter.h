#pragma once

struct lore_type;
class PlayerType;
void set_breath_types(PlayerType *player_ptr, lore_type *lore_ptr);
void set_ball_types(PlayerType *player_ptr, lore_type *lore_ptr);
void set_particular_types(PlayerType *player_ptr, lore_type *lore_ptr);
void set_bolt_types(PlayerType *player_ptr, lore_type *lore_ptr);
void set_status_types(lore_type *lore_ptr);
void set_teleport_types(lore_type *lore_ptr);
void set_floor_types(PlayerType *player_ptr, lore_type *lore_ptr);
void set_summon_types(lore_type *lore_ptr);
