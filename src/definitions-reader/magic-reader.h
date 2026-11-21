#pragma once

#include "definitions-reader/definition-id-checker.h"
#include "definitions-reader/parse-error-types.h"
#include "realm/realm-types.h"
#include "spell/magic-aptitude.h"
#include "system/spell-info-list.h"
#include "util/enum-converter.h"
#include <array>
#include <nlohmann/json_fwd.hpp>
#include <tl/expected.hpp>
#include <tl/optional.hpp>

enum class PlayerClassType : short;
struct player_magic;

class MagicReader {
public:
    using Session = DefinitionIdChecker;
    static Session create_session(tl::optional<int> last_id = tl::nullopt);
    static tl::expected<AptitudeSteps, ParseErrorType> read_aptitude_steps(const nlohmann::json &root);

    MagicReader(const nlohmann::json &class_data, Session &session, const AptitudeSteps &steps = {});
    MagicReader(nlohmann::json &&, Session &, const AptitudeSteps & = {}) = delete;
    MagicReader(const MagicReader &) = delete;
    MagicReader(MagicReader &&) = delete;
    MagicReader &operator=(const MagicReader &) = delete;
    MagicReader &operator=(MagicReader &&) = delete;

    tl::expected<void, ParseErrorType> read() const;

private:
    //! 領域の魔法ごとに、unlearnable か overrides に書かれたかどうか
    using SpellListedFlags = std::array<bool, SPELLS_IN_REALM>;
    //! 領域の魔法ごとの、上書きの段階 (上書きの無い魔法は値なし)
    using SpellAptitudes = std::array<tl::optional<int>, SPELLS_IN_REALM>;
    //! 領域ごとに、定義に書かれたかどうか
    using RealmListedFlags = std::array<bool, enum2i(RealmType::MAX)>;

    static tl::expected<int, ParseErrorType> read_listed_spell(const nlohmann::json &spell_tag_obj, int max_book, RealmType realm, SpellListedFlags &listed);
    static tl::expected<void, ParseErrorType> read_unlearnable_spells(const nlohmann::json &unlearnable_obj, int max_book, RealmType realm, SpellListedFlags &listed, SpellListedFlags &unlearnable);
    static tl::expected<void, ParseErrorType> check_realm_choices(PlayerClassType pclass, const RealmListedFlags &listed, const RealmListedFlags &has_second);
    static tl::expected<void, ParseErrorType> read_overrides(const nlohmann::json &overrides_obj, int max_book, RealmType realm, int aptitude, int aptitude_as_second, SpellListedFlags &listed, SpellAptitudes &overrides);

    tl::expected<int, ParseErrorType> read_class_id() const;
    tl::expected<void, ParseErrorType> set_spell_type(player_magic &magics_info) const;
    tl::expected<void, ParseErrorType> set_magic_status(player_magic &magics_info) const;
    tl::expected<void, ParseErrorType> set_realm_data(player_magic &magics_info, PlayerClassType pclass) const;
    tl::expected<void, ParseErrorType> set_realm_spells(const nlohmann::json &realm_data, player_magic &magics_info, RealmType realm, bool &has_second) const;

    const nlohmann::json &class_data;
    Session &session;
    AptitudeSteps steps;
};
