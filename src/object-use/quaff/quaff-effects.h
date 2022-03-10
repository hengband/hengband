#pragma once

class ObjectType;
class PlayerType;
class QuaffEffects {
public:
    QuaffEffects(PlayerType *player_ptr);

    bool influence(const ObjectType &o_ref);

private:
    PlayerType *player_ptr;

    bool booze();
    bool detonation();
};
