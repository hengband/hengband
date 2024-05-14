#pragma once

#include "system/angband.h"

class PlayerType;
class MonsterEntity;

class MonsterPainDescriber {
public:
    MonsterPainDescriber(std::string m_name, const MonsterEntity *m_ptr);

    std::optional<std::string> describe(int dam);

private:
    std::string m_name;
    const MonsterEntity *m_ptr;
};
