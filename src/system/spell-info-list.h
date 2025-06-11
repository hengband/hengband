#pragma once

#include "external-lib/include-json.h"
#include "realm/realm-types.h"
#include "system/angband.h"
#include <string>
#include <string_view>
#include <tl/optional.hpp>
#include <unordered_map>
#include <vector>

constexpr int SPELLS_IN_REALM = 32;
/*!
 * @brief 魔法領域名とintの対応表
 */
inline const std::unordered_map<std::string_view, RealmType> realms_list = {
    { "LIFE", RealmType::LIFE },
    { "SORCERY", RealmType::SORCERY },
    { "NATURE", RealmType::NATURE },
    { "CHAOS", RealmType::CHAOS },
    { "DEATH", RealmType::DEATH },
    { "TRUMP", RealmType::TRUMP },
    { "ARCANE", RealmType::ARCANE },
    { "CRAFT", RealmType::CRAFT },
    { "DEMON", RealmType::DAEMON },
    { "CRUSADE", RealmType::CRUSADE },
    { "MUSIC", RealmType::MUSIC },
    { "HISSATSU", RealmType::HISSATSU },
    { "HEX", RealmType::HEX },
};

enum class RealmType;

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

    tl::optional<short> get_spell_id(RealmType realm, std::string_view spell_tag) const;
    const SpellInfo &get_spell_info(RealmType realm, int spell_id) const;

private:
    SpellInfoList() = default;

    static SpellInfoList instance;

    std::vector<std::vector<SpellInfo>> spell_list{};
};
