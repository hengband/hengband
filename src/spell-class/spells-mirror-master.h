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
    void seal_of_mirror(const int dam);

private:
    PlayerType *player_ptr;
};
