#pragma once

class PlayerType;
bool psychometry(PlayerType *player_ptr);

enum class MindMindcrafterType : int;
bool cast_mindcrafter_spell(PlayerType *player_ptr, MindMindcrafterType spell);
