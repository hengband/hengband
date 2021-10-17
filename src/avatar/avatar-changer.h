#pragma once

#include "system/angband.h"

struct monster_type;
struct player_type;
class AvatarChanger {
public:
    AvatarChanger(player_type *player_ptr, monster_type *m_ptr);
    virtual ~AvatarChanger() = default;
    void change_virtue();

private:
    player_type *player_ptr;
    monster_type *m_ptr;
    void change_virtue_non_beginner();
    void change_virtue_unique();
    void change_virtue_good_evil();
    void change_virtue_revenge();
    void change_virtue_good_animal();
    void change_virtue_wild_thief();
};
