#pragma once

class PlayerType;
bool psychometry(PlayerType *player_ptr);

enum mind_mindcrafter_type : int;
bool cast_mindcrafter_spell(PlayerType *player_ptr, mind_mindcrafter_type spell);
