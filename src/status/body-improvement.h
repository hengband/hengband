#pragma once

class PlayerType;
class BodyImprovement {
public:
    BodyImprovement(PlayerType *player_ptr);

    bool has_effect() const;
    void mod_protection(short v, bool is_decrease = false);
    void set_protection(short v, bool is_decrease = false);

private:
    PlayerType *player_ptr;
    bool is_affected = false;
};

bool set_invuln(PlayerType *player_ptr, short v, bool do_dec);
bool set_tim_regen(PlayerType *player_ptr, short v, bool do_dec);
bool set_tim_reflect(PlayerType *player_ptr, short v, bool do_dec);
bool set_pass_wall(PlayerType *player_ptr, short v, bool do_dec);
bool set_tim_emission(PlayerType *player_ptr, short v, bool do_dec);
bool set_tim_exorcism(PlayerType *player_ptr, short v, bool do_dec);
