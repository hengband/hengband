#pragma once

#include "system/angband.h"
#include "util/flag-group.h"
#include <array>
#include <vector>

constexpr int SPELLS_IN_REALM = 32;
/*!
 * @brief 魔法領域名とintの対応表
 */
inline const std::unordered_map<std::string_view, int> realms_list = {
    { "LIFE", 0 },
    { "SORCERY", 1 },
    { "NATURE", 2 },
    { "CHAOS", 3 },
    { "DEATH", 4 },
    { "TRUMP", 5 },
    { "ARCANE", 6 },
    { "CRAFT", 7 },
    { "DEMON", 8 },
    { "CRUSADE", 9 },
    { "MUSIC", 15 },
    { "HISSATSU", 16 },
    { "HEX", 17 },
};

class SpellInfo {
public:
    short idx{};

    std::string name; /*!< 呪文名 */
    std::string description; /*!< 呪文説明 */
    std::string tag; /*!< 呪文タグ() */
};

class SpellInfoList {
public:
    SpellInfoList(SpellInfoList &&) = delete;
    SpellInfoList(const SpellInfoList &) = delete;
    SpellInfoList &operator=(const SpellInfoList &) = delete;
    SpellInfoList &operator=(SpellInfoList &&) = delete;
    ~SpellInfoList() = default;

    void initiallize();

    static SpellInfoList &get_instance();
    std::vector<std::vector<SpellInfo>> spell_list{};

private:
    SpellInfoList() = default;

    static SpellInfoList instance;
};
