#pragma once

#include "system/angband.h"
#include <vector>

/* 職業ごとの選択可能な魔法領域 / Possible realms that can be chosen. */
enum choosable_realm {
    CH_NONE = 0x00,
    CH_LIFE = 0x01,
    CH_SORCERY = 0x02,
    CH_NATURE = 0x04,
    CH_CHAOS = 0x08,
    CH_DEATH = 0x10,
    CH_TRUMP = 0x20,
    CH_ARCANE = 0x40,
    CH_ENCHANT = 0x80,
    CH_DAEMON = 0x100,
    CH_CRUSADE = 0x200,

    CH_MUSIC = 0x8000,
    CH_HISSATSU = 0x10000,
    CH_HEX = 0x20000,
};

enum class ItemKindType : short;
class PlayerType;
struct magic_type;
class PlayerRealm {
public:
    PlayerRealm(PlayerType *player_ptr);

    static const magic_type &get_spell_info(int realm, int num);
    static ItemKindType get_book(int realm);

    class Realm {
    public:
        Realm(int realm);
        const magic_type &get_spell_info(int num) const;
        ItemKindType get_book() const;

    private:
        int realm;
    };

    const Realm &realm1() const;
    const Realm &realm2() const;

private:
    Realm realm1_;
    Realm realm2_;
};

extern const std::vector<BIT_FLAGS> realm_choices1;
extern const std::vector<BIT_FLAGS> realm_choices2;
