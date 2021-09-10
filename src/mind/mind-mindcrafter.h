#pragma once

struct player_type;
bool psychometry(player_type *player_ptr);

enum mind_mindcrafter_type : int;
bool cast_mindcrafter_spell(player_type *player_ptr, mind_mindcrafter_type spell);
