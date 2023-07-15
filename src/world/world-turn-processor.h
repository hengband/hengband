#pragma once

class PlayerType;
class WorldTurnProcessor {
public:
    WorldTurnProcessor(PlayerType *player_ptr);
    virtual ~WorldTurnProcessor() = default;
    void process_world();
    void print_time();

private:
    PlayerType *player_ptr;
    int hour = 0;
    int min = 0;

    void process_downward();
    void process_monster_arena();
    void process_monster_arena_winner(int win_m_idx);
    void process_monster_arena_draw();
    void decide_auto_save();
    void process_change_daytime_night();
    void process_world_monsters();
    void decide_alloc_monster();
    void shuffle_shopkeeper();
    void ring_nightmare_bell(int prev_min);
};
