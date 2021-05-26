#pragma once

struct player_type;
class WorldTurnProcessor {
public:
    WorldTurnProcessor(player_type *player_ptr);
    virtual ~WorldTurnProcessor() = default;
    void process_world();
    void print_time();

private:
    player_type *player_ptr;
    int hour = 0;
    int min = 0;

    void process_downward();
    void process_monster_arena();
    void process_monster_arena_winner(int win_m_idx);
    void process_monster_arena_draw();
    void process_change_daytime_night();
    void shuffle_shopkeeper();
};
