#pragma once

class PlayerType;
class SpellsMirrorMaster {
public:
    SpellsMirrorMaster(PlayerType *player_ptr);

private:
    PlayerType *player_ptr;
};
