#pragma once

#include <nlohmann/json_fwd.hpp>
#include <vector>

enum class RealmType;
class SpellInfo;
class SpellInfoList;

class SpellReader {
public:
    SpellReader(const nlohmann::json &realm_data, SpellInfoList &spell_info_list);
    SpellReader(nlohmann::json &&, SpellInfoList &) = delete;
    SpellReader(const SpellReader &) = delete;
    SpellReader(SpellReader &&) = delete;
    SpellReader &operator=(const SpellReader &) = delete;
    SpellReader &operator=(SpellReader &&) = delete;

    int read() const;

private:
    int set_realm(RealmType &realm) const;
    int set_spell_data(const nlohmann::json &spell_data, std::vector<SpellInfo> &spells) const;
    int set_book_data(std::vector<SpellInfo> &spells) const;

    const nlohmann::json &realm_data;
    SpellInfoList &spell_info_list;
};
