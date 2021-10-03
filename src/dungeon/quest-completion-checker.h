#pragma once

struct monster_type;
struct player_type;
class QuestCompletionChecker {
public:
    QuestCompletionChecker(player_type *player_ptr, monster_type *m_ptr);
    virtual ~QuestCompletionChecker() = default;

    void complete();

private:
    player_type *player_ptr;
    monster_type *m_ptr;

    int count_all_hostile_monsters();
};
