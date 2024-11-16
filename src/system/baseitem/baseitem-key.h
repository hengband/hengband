/*!
 * @brief ベースアイテムキー (IDと1対1対応するアイテム種別)定義
 * @author Hourier
 * @date 2024/11/16
 */

#pragma once

#include "object/tval-types.h"
#include <optional>

enum class ItemKindType : short;
class BaseitemKey {
public:
    constexpr BaseitemKey()
        : type_value(ItemKindType::NONE)
        , subtype_value(std::nullopt)
    {
    }

    constexpr BaseitemKey(const ItemKindType type_value, const std::optional<int> &subtype_value = std::nullopt)
        : type_value(type_value)
        , subtype_value(subtype_value)
    {
    }

    bool operator==(const BaseitemKey &other) const;
    bool operator!=(const BaseitemKey &other) const
    {
        return !(*this == other);
    }

    bool operator<(const BaseitemKey &other) const;
    bool operator>(const BaseitemKey &other) const
    {
        return other < *this;
    }

    bool operator<=(const BaseitemKey &other) const
    {
        return !(*this > other);
    }

    bool operator>=(const BaseitemKey &other) const
    {
        return !(*this < other);
    }

    ItemKindType tval() const;
    std::optional<int> sval() const;
    bool is_valid() const;
    bool is(ItemKindType tval) const;
    ItemKindType get_arrow_kind() const;
    bool is_spell_book() const;
    bool is_high_level_book() const;
    bool is_melee_weapon() const;
    bool is_ammo() const;
    bool has_unidentified_name() const;
    bool can_recharge() const;
    bool is_wand_rod() const;
    bool is_wand_staff() const;
    bool is_protector() const;
    bool can_be_aura_protector() const;
    bool is_wearable() const;
    bool is_weapon() const;
    bool is_equipement() const;
    bool is_melee_ammo() const;
    bool is_orthodox_melee_weapon() const;
    bool is_broken_weapon() const;
    bool is_throwable() const;
    bool is_wieldable_in_etheir_hand() const;
    bool is_rare() const;
    short get_bow_energy() const;
    int get_arrow_magnification() const;
    bool is_aiming_rod() const;
    bool is_lite_requiring_fuel() const;
    bool is_junk() const;
    bool is_armour() const;
    bool is_cross_bow() const;
    bool should_refuse_enchant() const;
    bool is_convertible() const;
    bool is_fuel() const;
    bool is_lance() const;
    bool is_readable() const;
    bool is_corpse() const;
    bool is_monster() const;
    bool are_both_statue(const BaseitemKey &other) const;

private:
    ItemKindType type_value;
    std::optional<int> subtype_value;

    bool is_mushrooms() const;
};
