#pragma once

#include "realm/realm-types.h"
#include "system/angband.h"
#include "util/flag-group.h"
#include <cstdint>
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
    static const magic_type &get_spell_info(int realm, int num);
    static ItemKindType get_book(int realm);
    static RealmChoices get_realm1_choices(PlayerClassType pclass);
    static RealmChoices get_realm2_choices(PlayerClassType pclass);

    class Realm {
    public:
        Realm(int realm);
        const LocalizedString &get_name() const;
        const magic_type &get_spell_info(int num) const;
        ItemKindType get_book() const;
        bool is_good_attribute() const;

    private:
        int realm;
    };

    const Realm &realm1() const;
    const Realm &realm2() const;

private:
    Realm realm1_;
    Realm realm2_;
};
