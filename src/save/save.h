#pragma once

enum save_type {
    SAVE_TYPE_CLOSE_GAME = 0,
    SAVE_TYPE_CONTINUE_GAME = 1,
    SAVE_TYPE_DEBUG = 2
};

class PlayerType;
bool save_player(PlayerType *player_ptr, save_type type);
