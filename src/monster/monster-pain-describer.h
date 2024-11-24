#pragma once

#include <optional>
#include <string>
#include <string_view>

class PlayerType;
class MonsterEntity;
enum class MonraceId : short;
class MonsterPainDescriber {
public:
    MonsterPainDescriber(MonraceId r_idx, char symbol, std::string_view m_name);

    std::optional<std::string> describe(int now_hp, int took_damage, bool visible);

private:
    MonraceId r_idx;
    char symbol;
    std::string m_name;
};
