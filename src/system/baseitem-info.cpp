/*!
 * @brief ベースアイテム情報の構造体 / Information about object "kinds", including player knowledge.
 * @date 2019/05/01
 * @author deskull
 * @details
 * ゲーム進行用のセーブファイル上では aware と tried のみ保存対象とすること。と英文ではあるが実際はもっとある様子である。 /
 * Only "aware" and "tried" are saved in the savefile
 */

#include "system/baseitem-info.h"
#include "object/tval-types.h"
#include "sv-definition/sv-bow-types.h"
#include "sv-definition/sv-food-types.h"

BaseitemKey::BaseitemKey(const ItemKindType type_value, const std::optional<int> &subtype_value)
    : type_value(type_value)
    , subtype_value(subtype_value)
{
}

bool BaseitemKey::operator==(const BaseitemKey &other) const
{
    return (this->type_value == other.type_value) && (this->subtype_value == other.subtype_value);
}

// @details type_valueに大小があればそれを判定し、同一ならばsubtype_valueの大小を判定する.
bool BaseitemKey::operator<(const BaseitemKey &other) const
{
    if (this->type_value < other.type_value) {
        return true;
    }

    if (this->type_value > other.type_value) {
        return false;
    }

    return this->subtype_value < other.subtype_value;
}

ItemKindType BaseitemKey::tval() const
{
    return this->type_value;
}

std::optional<int> BaseitemKey::sval() const
{
    return this->subtype_value;
}

/*!
 * @brief 射撃武器に対応する矢/弾薬のベースアイテムIDを返す
 * @return 対応する矢/弾薬のベースアイテムID
 */
ItemKindType BaseitemKey::get_arrow_kind() const
{
    if (this->type_value != ItemKindType::BOW) {
        return ItemKindType::NONE;
    }

    if (!this->subtype_value.has_value()) {
        return ItemKindType::NONE;
    }

    switch (this->subtype_value.value()) {
    case SV_SLING:
        return ItemKindType::SHOT;
    case SV_SHORT_BOW:
    case SV_LONG_BOW:
    case SV_NAMAKE_BOW:
        return ItemKindType::ARROW;
    case SV_LIGHT_XBOW:
    case SV_HEAVY_XBOW:
        return ItemKindType::BOLT;
    case SV_CRIMSON:
    case SV_HARP:
        return ItemKindType::NO_AMMO;
    default:
        return ItemKindType::NONE;
    }
}

bool BaseitemKey::is_spell_book() const
{
    switch (this->type_value) {
    case ItemKindType::LIFE_BOOK:
    case ItemKindType::SORCERY_BOOK:
    case ItemKindType::NATURE_BOOK:
    case ItemKindType::CHAOS_BOOK:
    case ItemKindType::DEATH_BOOK:
    case ItemKindType::TRUMP_BOOK:
    case ItemKindType::ARCANE_BOOK:
    case ItemKindType::CRAFT_BOOK:
    case ItemKindType::DEMON_BOOK:
    case ItemKindType::CRUSADE_BOOK:
    case ItemKindType::MUSIC_BOOK:
    case ItemKindType::HISSATSU_BOOK:
    case ItemKindType::HEX_BOOK:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_high_level_book() const
{
    if (!this->is_spell_book()) {
        return false;
    }

    if (this->type_value == ItemKindType::ARCANE_BOOK) {
        return false;
    }

    return this->subtype_value >= 2;
}

bool BaseitemKey::is_melee_weapon() const
{
    switch (this->type_value) {
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_ammo() const
{
    switch (this->type_value) {
    case ItemKindType::SHOT:
    case ItemKindType::ARROW:
    case ItemKindType::BOLT:
        return true;
    default:
        return false;
    }
}

/*
 * @brief 未鑑定名を持つか否かの判定
 * @details FOODはキノコが該当する
 */
bool BaseitemKey::has_unidentified_name() const
{
    switch (this->type_value) {
    case ItemKindType::AMULET:
    case ItemKindType::RING:
    case ItemKindType::STAFF:
    case ItemKindType::WAND:
    case ItemKindType::ROD:
    case ItemKindType::SCROLL:
    case ItemKindType::POTION:
        return true;
    case ItemKindType::FOOD:
        return this->is_mushrooms();
    default:
        return false;
    }
}

bool BaseitemKey::is_mushrooms() const
{
    if (!this->subtype_value.has_value()) {
        return false;
    }

    switch (this->subtype_value.value()) {
    case SV_FOOD_POISON:
    case SV_FOOD_BLINDNESS:
    case SV_FOOD_PARANOIA:
    case SV_FOOD_CONFUSION:
    case SV_FOOD_HALLUCINATION:
    case SV_FOOD_PARALYSIS:
    case SV_FOOD_WEAKNESS:
    case SV_FOOD_SICKNESS:
    case SV_FOOD_STUPIDITY:
    case SV_FOOD_NAIVETY:
    case SV_FOOD_UNHEALTH:
    case SV_FOOD_DISEASE:
    case SV_FOOD_CURE_POISON:
    case SV_FOOD_CURE_BLINDNESS:
    case SV_FOOD_CURE_PARANOIA:
    case SV_FOOD_CURE_CONFUSION:
    case SV_FOOD_CURE_SERIOUS:
    case SV_FOOD_RESTORE_STR:
    case SV_FOOD_RESTORE_CON:
    case SV_FOOD_RESTORING:
        return true;
    default:
        return false;
    }
}

BaseitemInfo::BaseitemInfo()
    : bi_key(ItemKindType::NONE)
{
}

std::vector<BaseitemInfo> baseitems_info;
