#pragma once

class PlayerType;
class QuaffEffects {
public:
    QuaffEffects(PlayerType *player_ptr);

    // @todo switch/case文を移してくるまでの一時的なpublicメソッド.
    bool booze();
    bool detonation();

private:
    PlayerType *player_ptr;
};
