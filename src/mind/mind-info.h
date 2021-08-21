#pragma once

typedef struct player_type player_type;
enum class mind_kind_type;

void mindcraft_info(player_type *caster_ptr, char *p, mind_kind_type use_mind, int power);
