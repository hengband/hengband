#pragma once

enum mind_berserker_type : int;
struct player_type;
bool cast_berserk_spell(player_type *player_ptr, mind_berserker_type spell);
