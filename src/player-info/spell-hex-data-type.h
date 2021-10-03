#pragma once

#include "system/angband.h"

#include "realm/realm-hex-numbers.h"
#include "util/flag-group.h"

enum class SpellHexRevengeType : byte;

using HexSpellFlagGroup = FlagGroup<spell_hex_type, HEX_MAX>;

struct spell_hex_data_type {
    HexSpellFlagGroup casting_spells{};
    HexSpellFlagGroup interrupting_spells{};
    SpellHexRevengeType revenge_type{};
    int32_t revenge_power{};
    byte revenge_turn{};
};
