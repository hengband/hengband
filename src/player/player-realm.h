#pragma once

#include "realm/realm-types.h"
#include "system/angband.h"
#include "util/flag-group.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <tl/optional.hpp>
#include <vector>

using RealmChoices = EnumClassFlagGroup<RealmType>;

enum class ItemKindType : short;
enum class PlayerClassType : short;
class PlayerType;
class LocalizedString;
struct magic_type;
class PlayerRealm {
public:
    PlayerRealm(PlayerType *player_ptr);

    static const LocalizedString &get_name(RealmType realm);
    static std::string_view get_explanation(RealmType realm);
    static std::string_view get_subinfo(RealmType realm);
    static const magic_type &get_spell_info(RealmType realm, int spell_id, tl::optional<PlayerClassType> pclass = tl::nullopt);
    static const std::string &get_spell_name(RealmType realm, int spell_id);
    static const std::string &get_spell_description(RealmType realm, int spell_id);
    static ItemKindType get_book(RealmType realm);
    static RealmChoices get_realm1_choices(PlayerClassType pclass);
    static RealmChoices get_realm2_choices(PlayerClassType pclass);
    static RealmType get_realm_of_book(ItemKindType tval);
    static bool is_magic(RealmType realm);
    static bool is_technic(RealmType realm);

    class Realm {
    public:
        Realm(RealmType realm);
        const LocalizedString &get_name() const;
        std::string_view get_explanation() const;
        std::string_view get_subinfo() const;
        const magic_type &get_spell_info(int spell_id) const;
        const std::string &get_spell_name(int spell_id) const;
        const std::string &get_spell_description(int spell_id) const;
        ItemKindType get_book() const;
        bool is_available() const;
        bool is_good_attribute() const;
        bool equals(RealmType realm) const;
        RealmType to_enum() const;

    private:
        RealmType realm_;
    };

    const Realm &realm1() const;
    const Realm &realm2() const;
    bool is_realm_hex() const;
    void reset();
    void set(RealmType realm1, RealmType realm2 = RealmType::NONE);

private:
    void set_(RealmType realm1, RealmType realm2);

    PlayerType *player_ptr;
    Realm realm1_;
    Realm realm2_;
};
