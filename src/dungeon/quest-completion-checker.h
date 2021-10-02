#pragma once

#include "system/angband.h"
#include "util/point-2d.h"
#include <tuple>

struct monster_type;
struct player_type;
struct quest_type;
class QuestCompletionChecker {
public:
    QuestCompletionChecker(player_type *player_ptr, monster_type *m_ptr);
    virtual ~QuestCompletionChecker() = default;

    void complete();

private:
    player_type *player_ptr;
    monster_type *m_ptr;
    short quest_idx;
    quest_type *q_ptr = nullptr;

    int count_all_hostile_monsters();
    void set_quest_idx();
    std::tuple<bool, bool> switch_completion();
    Pos2D make_stairs(const bool create_stairs);
};
