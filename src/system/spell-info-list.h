#pragma once

#include "external-lib/include-json.h"
#include "system/angband.h"
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
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

    void initialize();
    errr parse(nlohmann::json &spell_data);

    static SpellInfoList &get_instance();

    std::optional<short> get_spell_id(int realm, std::string_view spell_tag) const;
    const SpellInfo &get_spell_info(int realm, int spell_id) const;

private:
    SpellInfoList() = default;

    static SpellInfoList instance;

    std::vector<std::vector<SpellInfo>> spell_list{};
};
