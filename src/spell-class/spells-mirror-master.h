#pragma once

class PlayerType;
class SpellsMirrorMaster {
public:
    SpellsMirrorMaster(PlayerType *player_ptr);
    void remove_all_mirrors(bool explode);
    void remove_mirror(int y, int x);
    bool mirror_tunnel();
    bool place_mirror();
    bool mirror_concentration();

private:
    PlayerType *player_ptr;
};
