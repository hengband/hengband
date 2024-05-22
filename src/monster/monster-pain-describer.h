#pragma once

#include "system/angband.h"

class PlayerType;
class MonsterEntity;
enum class MonsterRaceId : int16_t;

class MonsterPainDescriber {
public:
    MonsterPainDescriber(MonsterRaceId r_idx, char symbol, std::string_view m_name);

    std::optional<std::string> describe(int now_hp, int took_damage, bool visible);

private:
    MonsterRaceId r_idx;
    char symbol;
    std::string m_name;
};
