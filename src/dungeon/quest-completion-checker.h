#pragma once

#include "system/angband.h"
#include "util/point-2d.h"
#include <tuple>

struct monster_type;
struct object_type;
class PlayerType;
struct quest_type;
class QuestCompletionChecker {
public:
    QuestCompletionChecker(PlayerType *player_ptr, monster_type *m_ptr);
    virtual ~QuestCompletionChecker() = default;

    void complete();

private:
    PlayerType *player_ptr;
    monster_type *m_ptr;
    short quest_idx;
    quest_type *q_ptr = nullptr;

    void set_quest_idx();
    std::tuple<bool, bool> switch_completion();
    void complete_kill_number();
    void complete_kill_all();
    std::tuple<bool, bool> complete_random();
    void complete_kill_any_level();
    void complete_tower();
    int count_all_hostile_monsters();
    Pos2D make_stairs(const bool create_stairs);
    void make_reward(const Pos2D pos);
    bool check_quality(object_type &item);
};
