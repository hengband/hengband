#pragma once

class PlayerType;
struct ProjectResult;
class SpellsMirrorMaster {
public:
    SpellsMirrorMaster(PlayerType *player_ptr);
    void remove_all_mirrors(bool explode);
    void remove_mirror(int y, int x);
    bool mirror_tunnel();
    bool place_mirror();
    bool mirror_concentration();
    void seal_of_mirror(const int dam);
    void seeker_ray(int dir, int dam);
    void super_ray(int dir, int dam);

private:
    PlayerType *player_ptr;
    void next_mirror(int *next_y, int *next_x, int cury, int curx);
    void project_seeker_ray(int target_x, int target_y, int dam);
    void project_super_ray(int target_x, int target_y, int dam);
};
