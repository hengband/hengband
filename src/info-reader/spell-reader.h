#pragma once

#include <nlohmann/json.hpp>

enum class RealmType;
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
    int set_spell_data(const nlohmann::json &spell_data, RealmType realm) const;
    int set_book_data(RealmType realm) const;

    const nlohmann::json &realm_data;
    SpellInfoList &spell_info_list;
};
