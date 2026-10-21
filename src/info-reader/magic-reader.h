#pragma once

#include "info-reader/parse-error-types.h"
#include <nlohmann/json_fwd.hpp>
#include <tl/expected.hpp>

enum class RealmType;
struct player_magic;

class MagicReader {
public:
    explicit MagicReader(const nlohmann::json &class_data);
    MagicReader(nlohmann::json &&) = delete;
    MagicReader(const MagicReader &) = delete;
    MagicReader(MagicReader &&) = delete;
    MagicReader &operator=(const MagicReader &) = delete;
    MagicReader &operator=(MagicReader &&) = delete;

    tl::expected<void, ParseErrorType> read() const;

private:
    tl::expected<int, ParseErrorType> read_class_id() const;
    tl::expected<void, ParseErrorType> set_spell_type(player_magic &magics_info) const;
    tl::expected<void, ParseErrorType> set_magic_status(player_magic &magics_info) const;
    tl::expected<void, ParseErrorType> set_spell_data(const nlohmann::json &spell_data, player_magic &magics_info, RealmType realm) const;
    tl::expected<void, ParseErrorType> set_realm_data(player_magic &magics_info) const;

    const nlohmann::json &class_data;
};
