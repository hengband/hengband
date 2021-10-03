#pragma once

struct player_type;
class StoneOfLore {
public:
    StoneOfLore(player_type *player_ptr);
    virtual ~StoneOfLore() = default;
    bool perilous_secrets();

private:
    player_type *player_ptr;

    void consume_mp();
};
