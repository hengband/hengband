#pragma once

class PlayerType;
class StoneOfLore {
public:
    StoneOfLore(PlayerType *player_ptr);
    virtual ~StoneOfLore() = default;
    bool perilous_secrets();

private:
    PlayerType *player_ptr;

    void consume_mp();
};
