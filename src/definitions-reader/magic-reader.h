#pragma once

#include "definitions-reader/definition-id-checker.h"
#include "definitions-reader/parse-error-types.h"
#include "system/spell-info-list.h"
#include <array>
#include <nlohmann/json_fwd.hpp>
#include <tl/expected.hpp>

enum class RealmType;
struct player_magic;

class MagicReader {
public:
    using Session = DefinitionIdChecker;
    static Session create_session(tl::optional<int> last_id = tl::nullopt);

    MagicReader(const nlohmann::json &class_data, Session &session);
    MagicReader(nlohmann::json &&, Session &) = delete;
    MagicReader(const MagicReader &) = delete;
    MagicReader(MagicReader &&) = delete;
    MagicReader &operator=(const MagicReader &) = delete;
    MagicReader &operator=(MagicReader &&) = delete;

    tl::expected<void, ParseErrorType> read() const;

private:
    //! 領域の魔法ごとに、定義 (spells_info か unlearnable) に書かれたかどうか
    using SpellListedFlags = std::array<bool, SPELLS_IN_REALM>;

    static tl::expected<int, ParseErrorType> read_listed_spell(const nlohmann::json &spell_tag_obj, const player_magic &magics_info, RealmType realm, SpellListedFlags &listed);
    static tl::expected<void, ParseErrorType> set_unlearnable_spells(const nlohmann::json &unlearnable_obj, const player_magic &magics_info, RealmType realm, SpellListedFlags &listed);
    static tl::expected<void, ParseErrorType> check_all_spells_listed(RealmType realm, int max_book, const SpellListedFlags &listed);

    tl::expected<int, ParseErrorType> read_class_id() const;
    tl::expected<void, ParseErrorType> set_spell_type(player_magic &magics_info) const;
    tl::expected<void, ParseErrorType> set_magic_status(player_magic &magics_info) const;
    tl::expected<void, ParseErrorType> set_spell_data(const nlohmann::json &spell_data, player_magic &magics_info, RealmType realm, SpellListedFlags &listed) const;
    tl::expected<void, ParseErrorType> set_realm_data(player_magic &magics_info) const;

    const nlohmann::json &class_data;
    Session &session;
};
