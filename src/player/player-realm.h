#pragma once

#include "realm/realm-types.h"
#include "system/angband.h"
#include "util/flag-group.h"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using RealmChoices = FlagGroup<magic_realm_type, REALM_MAX>;

enum class ItemKindType : short;
enum class PlayerClassType : short;
class PlayerType;
class LocalizedString;
struct magic_type;
class PlayerRealm {
public:
    PlayerRealm(PlayerType *player_ptr);

    static const LocalizedString &get_name(int realm);
    static std::string_view get_explanation(int realm);
    static std::string_view get_subinfo(int realm);
    static const magic_type &get_spell_info(int realm, int spell_id, std::optional<PlayerClassType> pclass = std::nullopt);
    static const std::string &get_spell_name(int realm, int spell_id);
    static const std::string &get_spell_description(int realm, int spell_id);
    static ItemKindType get_book(int realm);
    static RealmChoices get_realm1_choices(PlayerClassType pclass);
    static RealmChoices get_realm2_choices(PlayerClassType pclass);
    static magic_realm_type get_realm_of_book(ItemKindType tval);

    class Realm {
    public:
        Realm(int realm);
        const LocalizedString &get_name() const;
        std::string_view get_explanation() const;
        std::string_view get_subinfo() const;
        const magic_type &get_spell_info(int spell_id) const;
        const std::string &get_spell_name(int spell_id) const;
        const std::string &get_spell_description(int spell_id) const;
        ItemKindType get_book() const;
        bool is_available() const;
        bool is_good_attribute() const;
        bool equals(int realm) const;

    private:
        int realm_;
    };

    const Realm &realm1() const;
    const Realm &realm2() const;
    bool is_realm_hex() const;
    void reset();
    void set(int realm1, int realm2 = REALM_NONE);

private:
    void set_(int realm1, int realm2);

    PlayerType *player_ptr;
    Realm realm1_;
    Realm realm2_;
};
