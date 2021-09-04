#pragma once

struct player_type;
bool psychometry(player_type *caster_ptr);

enum mind_mindcrafter_type : int;
bool cast_mindcrafter_spell(player_type *caster_ptr, mind_mindcrafter_type spell);
