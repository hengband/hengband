#pragma once

enum class SaveType {
    CLOSE_GAME,
    CONTINUE_GAME
};

class PlayerType;
bool save_player(PlayerType *player_ptr, SaveType type);
