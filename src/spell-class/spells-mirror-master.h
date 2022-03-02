#pragma once

class PlayerType;
class SpellsMirrorMaster {
public:
    SpellsMirrorMaster(PlayerType *player_ptr);
    void remove_all_mirrors(bool explode);
    void remove_mirror(int y, int x);
    bool mirror_tunnel();

private:
    PlayerType *player_ptr;
};
