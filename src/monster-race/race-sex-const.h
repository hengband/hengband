#pragma once

class MonsterRaceInfo;

enum class MonsterSex {
    NONE = 0,
    MALE = 1,
    FEMALE = 2,
};

bool is_male(const MonsterSex sex);
bool is_male(const MonsterRaceInfo &monrace);
bool is_female(const MonsterSex sex);
bool is_female(const MonsterRaceInfo &monrace);
