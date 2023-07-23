#pragma once

#include "system/angband.h"

class MonsterEntity;
class PlayerType;
class AvatarChanger {
public:
    AvatarChanger(PlayerType *player_ptr, MonsterEntity *m_ptr);
    virtual ~AvatarChanger() = default;
    void change_virtue();

private:
    PlayerType *player_ptr;
    MonsterEntity *m_ptr;
    void change_virtue_non_beginner();
    void change_virtue_unique();
    void change_virtue_good_evil();
    void change_virtue_revenge();
    void change_virtue_good_animal();
    void change_virtue_wild_thief();
};
