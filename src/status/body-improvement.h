#pragma once

class PlayerType;
class BodyImprovement {
public:
    BodyImprovement(PlayerType *player_ptr);

    bool mod_protection(short v, bool do_dec);
    bool set_protection(short v, bool do_dec);

private:
    PlayerType *player_ptr;
};

bool set_invuln(PlayerType *player_ptr, short v, bool do_dec);
bool set_tim_regen(PlayerType *player_ptr, short v, bool do_dec);
bool set_tim_reflect(PlayerType *player_ptr, short v, bool do_dec);
bool set_pass_wall(PlayerType *player_ptr, short v, bool do_dec);
