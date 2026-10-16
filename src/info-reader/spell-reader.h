#pragma once

#include "info-reader/parse-error-types.h"
#include <nlohmann/json_fwd.hpp>
#include <tl/expected.hpp>
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
    tl::expected<RealmType, parse_error_type> read_realm() const;
    tl::expected<void, parse_error_type> set_spell_data(const nlohmann::json &spell_data, std::vector<SpellInfo> &spells) const;
    tl::expected<void, parse_error_type> set_book_data(std::vector<SpellInfo> &spells) const;

    const nlohmann::json &realm_data;
    SpellInfoList &spell_info_list;
};
