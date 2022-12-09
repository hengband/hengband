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
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-bow-types.h"
#include "sv-definition/sv-food-types.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-rod-types.h"
#include "sv-definition/sv-weapon-types.h"
#include <set>
#include <stdexcept>
#include <unordered_map>

namespace {
constexpr auto ITEM_NOT_BOW = "This item is not a bow!";
constexpr auto ITEM_NOT_ROD = "This item is not a rod!";
constexpr auto ITEM_NOT_LITE = "This item is not a lite!";
}

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
    if ((this->type_value != ItemKindType::BOW) || !this->subtype_value.has_value()) {
        throw std::logic_error(ITEM_NOT_BOW);
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

bool BaseitemKey::can_recharge() const
{
    switch (this->type_value) {
    case ItemKindType::STAFF:
    case ItemKindType::WAND:
    case ItemKindType::ROD:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_wand_rod() const
{
    switch (this->type_value) {
    case ItemKindType::WAND:
    case ItemKindType::ROD:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_wand_staff() const
{
    switch (this->type_value) {
    case ItemKindType::WAND:
    case ItemKindType::STAFF:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_protector() const
{
    switch (this->type_value) {
    case ItemKindType::BOOTS:
    case ItemKindType::GLOVES:
    case ItemKindType::HELM:
    case ItemKindType::CROWN:
    case ItemKindType::SHIELD:
    case ItemKindType::CLOAK:
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::DRAG_ARMOR:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::can_be_aura_protector() const
{
    switch (this->type_value) {
    case ItemKindType::CLOAK:
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_wearable() const
{
    switch (this->type_value) {
    case ItemKindType::BOW:
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
    case ItemKindType::BOOTS:
    case ItemKindType::GLOVES:
    case ItemKindType::HELM:
    case ItemKindType::CROWN:
    case ItemKindType::SHIELD:
    case ItemKindType::CLOAK:
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::DRAG_ARMOR:
    case ItemKindType::LITE:
    case ItemKindType::AMULET:
    case ItemKindType::RING:
    case ItemKindType::CARD:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_weapon() const
{
    switch (this->type_value) {
    case ItemKindType::BOW:
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_equipement() const
{
    switch (this->type_value) {
    case ItemKindType::SHOT:
    case ItemKindType::ARROW:
    case ItemKindType::BOLT:
    case ItemKindType::BOW:
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
    case ItemKindType::BOOTS:
    case ItemKindType::GLOVES:
    case ItemKindType::HELM:
    case ItemKindType::CROWN:
    case ItemKindType::SHIELD:
    case ItemKindType::CLOAK:
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::DRAG_ARMOR:
    case ItemKindType::LITE:
    case ItemKindType::AMULET:
    case ItemKindType::RING:
    case ItemKindType::CARD:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_melee_ammo() const
{
    switch (this->type_value) {
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::DIGGING:
    case ItemKindType::BOLT:
    case ItemKindType::ARROW:
    case ItemKindType::SHOT:
        return true;
    case ItemKindType::SWORD:
        return this->subtype_value != SV_POISON_NEEDLE;
    default:
        return false;
    }
}

bool BaseitemKey::is_orthodox_melee_weapon() const
{
    switch (this->type_value) {
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::DIGGING:
        return true;
    case ItemKindType::SWORD:
        return this->subtype_value != SV_POISON_NEEDLE;
    default:
        return false;
    }
}

bool BaseitemKey::is_broken_weapon() const
{
    if (this->type_value != ItemKindType::SWORD) {
        return false;
    }

    if (!this->subtype_value.has_value()) {
        return false;
    }

    switch (this->subtype_value.value()) {
    case SV_BROKEN_DAGGER:
    case SV_BROKEN_SWORD:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_throwable() const
{
    switch (this->type_value) {
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_wieldable_in_etheir_hand() const
{
    switch (this->type_value) {
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
    case ItemKindType::SHIELD:
    case ItemKindType::CAPTURE:
    case ItemKindType::CARD:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_rare() const
{
    static const std::unordered_map<ItemKindType, const std::set<int>> rare_table = {
        { ItemKindType::HAFTED, { SV_MACE_OF_DISRUPTION, SV_WIZSTAFF } },
        { ItemKindType::POLEARM, { SV_SCYTHE_OF_SLICING, SV_DEATH_SCYTHE } },
        { ItemKindType::SWORD, { SV_BLADE_OF_CHAOS, SV_DIAMOND_EDGE, SV_POISON_NEEDLE, SV_HAYABUSA } },
        { ItemKindType::SHIELD, { SV_DRAGON_SHIELD, SV_MIRROR_SHIELD } },
        { ItemKindType::HELM, { SV_DRAGON_HELM } },
        { ItemKindType::BOOTS, { SV_PAIR_OF_DRAGON_GREAVE } },
        { ItemKindType::CLOAK, { SV_ELVEN_CLOAK, SV_ETHEREAL_CLOAK, SV_SHADOW_CLOAK, SV_MAGIC_RESISTANCE_CLOAK } },
        { ItemKindType::GLOVES, { SV_SET_OF_DRAGON_GLOVES } },
        { ItemKindType::SOFT_ARMOR, { SV_KUROSHOUZOKU, SV_ABUNAI_MIZUGI } },
        { ItemKindType::HARD_ARMOR, { SV_MITHRIL_CHAIN_MAIL, SV_MITHRIL_PLATE_MAIL, SV_ADAMANTITE_PLATE_MAIL } },
        { ItemKindType::DRAG_ARMOR, { /* Any */ } },
    };

    if (!this->subtype_value.has_value()) {
        return false;
    }

    if (auto it = rare_table.find(this->type_value); it != rare_table.end()) {
        const auto &svals = it->second;
        return svals.empty() || (svals.find(this->subtype_value.value()) != svals.end());
    }

    return false;
}

short BaseitemKey::get_bow_energy() const
{
    if ((this->type_value != ItemKindType::BOW) || !this->subtype_value.has_value()) {
        throw std::logic_error(ITEM_NOT_BOW);
    }

    switch (this->subtype_value.value()) {
    case SV_SLING:
        return 8000;
    case SV_NAMAKE_BOW:
        return 7777;
    case SV_LIGHT_XBOW:
        return 12000;
    case SV_HEAVY_XBOW:
        return 13333;
    default:
        return 10000;
    }
}

int BaseitemKey::get_arrow_magnification() const
{
    if ((this->type_value != ItemKindType::BOW) || !this->subtype_value.has_value()) {
        throw std::logic_error(ITEM_NOT_BOW);
    }

    switch (this->subtype_value.value()) {
    case SV_SLING:
    case SV_SHORT_BOW:
        return 2;
    case SV_LONG_BOW:
    case SV_NAMAKE_BOW:
    case SV_LIGHT_XBOW:
        return 3;
    case SV_HEAVY_XBOW:
        return 4;
    default:
        return 0;
    }
}

bool BaseitemKey::is_aiming_rod() const
{
    if ((this->type_value != ItemKindType::ROD) || !this->subtype_value.has_value()) {
        throw std::logic_error(ITEM_NOT_ROD);
    }

    switch (this->subtype_value.value()) {
    case SV_ROD_TELEPORT_AWAY:
    case SV_ROD_DISARMING:
    case SV_ROD_LITE:
    case SV_ROD_SLEEP_MONSTER:
    case SV_ROD_SLOW_MONSTER:
    case SV_ROD_HYPODYNAMIA:
    case SV_ROD_POLYMORPH:
    case SV_ROD_ACID_BOLT:
    case SV_ROD_ELEC_BOLT:
    case SV_ROD_FIRE_BOLT:
    case SV_ROD_COLD_BOLT:
    case SV_ROD_ACID_BALL:
    case SV_ROD_ELEC_BALL:
    case SV_ROD_FIRE_BALL:
    case SV_ROD_COLD_BALL:
    case SV_ROD_STONE_TO_MUD:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_lite_requiring_fuel() const
{
    if ((this->type_value != ItemKindType::LITE) || !this->subtype_value.has_value()) {
        throw std::logic_error(ITEM_NOT_LITE);
    }

    switch (this->subtype_value.value()) {
    case SV_LITE_TORCH:
    case SV_LITE_LANTERN:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_junk() const
{
    switch (this->type_value) {
    case ItemKindType::SKELETON:
    case ItemKindType::BOTTLE:
    case ItemKindType::JUNK:
    case ItemKindType::STATUE:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_armour() const
{
    switch (this->type_value) {
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::DRAG_ARMOR:
        return true;
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
