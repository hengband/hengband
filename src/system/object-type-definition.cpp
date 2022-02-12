/*
 * @file object-type-definition.h
 * @brief アイテム定義の構造体とエンティティ処理実装
 * @author Hourier
 * @date 2021/05/02
 */

#include "system/object-type-definition.h"
#include "artifact/fixed-art-types.h"
#include "artifact/random-art-effects.h"
#include "monster-race/monster-race.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object/object-flags.h"
#include "object/object-kind.h"
#include "smith/object-smith.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"

#include <set>
#include <unordered_map>

/*!
 * @brief オブジェクトを初期化する
 * Wipe an object clean.
 */
void ObjectType::wipe()
{
    *this = {};
}

/*!
 * @brief オブジェクトを複製する
 * Wipe an object clean.
 * @param j_ptr 複製元のオブジェクトの構造体参照ポインタ
 */
void ObjectType::copy_from(ObjectType *j_ptr)
{
    *this = *j_ptr;
}

/*!
 * @brief オブジェクト構造体にベースアイテムを作成する
 * Prepare an object based on an object kind.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param k_idx 新たに作成したいベースアイテム情報のID
 */
void ObjectType::prep(KIND_OBJECT_IDX ko_idx)
{
    auto *k_ptr = &k_info[ko_idx];
    auto old_stack_idx = this->stack_idx;
    wipe();
    this->stack_idx = old_stack_idx;
    this->k_idx = ko_idx;
    this->tval = k_ptr->tval;
    this->sval = k_ptr->sval;
    this->pval = k_ptr->pval;
    this->number = 1;
    this->weight = k_ptr->weight;
    this->to_h = k_ptr->to_h;
    this->to_d = k_ptr->to_d;
    this->to_a = k_ptr->to_a;
    this->ac = k_ptr->ac;
    this->dd = k_ptr->dd;
    this->ds = k_ptr->ds;

    if (k_ptr->act_idx > RandomArtActType::NONE)
        this->activation_id = k_ptr->act_idx;
    if (k_info[this->k_idx].cost <= 0)
        this->ident |= (IDENT_BROKEN);

    if (k_ptr->gen_flags.has(ItemGenerationTraitType::CURSED))
        this->curse_flags.set(CurseTraitType::CURSED);
    if (k_ptr->gen_flags.has(ItemGenerationTraitType::HEAVY_CURSE))
        this->curse_flags.set(CurseTraitType::HEAVY_CURSE);
    if (k_ptr->gen_flags.has(ItemGenerationTraitType::PERMA_CURSE))
        this->curse_flags.set(CurseTraitType::PERMA_CURSE);
    if (k_ptr->gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE0))
        this->curse_flags.set(get_curse(0, this));
    if (k_ptr->gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE1))
        this->curse_flags.set(get_curse(1, this));
    if (k_ptr->gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE2))
        this->curse_flags.set(get_curse(2, this));
}

/*!
 * @brief オブジェクトが武器として装備できるかどうかを返す / Check if an object is weapon (including bows)
 * @return 武器として使えるならばtrueを返す
 */
bool ObjectType::is_weapon() const
{
    return (TV_WEAPON_BEGIN <= this->tval) && (this->tval <= TV_WEAPON_END);
}

/*!
 * @brief オブジェクトが武器や矢弾として使用できるかを返す / Check if an object is weapon (including bows and ammo)
 * Rare weapons/aromors including Blade of Chaos, Dragon armors, etc.
 * @return 武器や矢弾として使えるならばtrueを返す
 */
bool ObjectType::is_weapon_ammo() const
{
    return (TV_MISSILE_BEGIN <= this->tval) && (this->tval <= TV_WEAPON_END);
}

/*!
 * @brief オブジェクトが武器、防具、矢弾として使用できるかを返す / Check if an object is weapon, armour or ammo
 * @return 武器、防具、矢弾として使えるならばtrueを返す
 */
bool ObjectType::is_weapon_armour_ammo() const
{
    return this->is_weapon_ammo() || this->is_armour();
}

/*!
 * @brief オブジェクトが近接武器として装備できるかを返す / Melee weapons
 * @return 近接武器として使えるならばtrueを返す
 */
bool ObjectType::is_melee_weapon() const
{
    return (ItemKindType::DIGGING <= this->tval) && (this->tval <= ItemKindType::SWORD);
}

/*!
 * @brief エッセンスの付加可能な武器や矢弾かを返す
 * @return エッセンスの付加可能な武器か矢弾ならばtrueを返す。
 */
bool ObjectType::is_melee_ammo() const
{
    switch (this->tval) {
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::DIGGING:
    case ItemKindType::BOLT:
    case ItemKindType::ARROW:
    case ItemKindType::SHOT: {
        return true;
    }
    case ItemKindType::SWORD: {
        if (this->sval != SV_POISON_NEEDLE)
            return true;
    }

    default:
        break;
    }

    return false;
}

/*!
 * @brief オブジェクトが装備可能であるかを返す / Wearable including all weapon, all armour, bow, light source, amulet, and ring
 * @return 装備可能ならばTRUEを返す
 */
bool ObjectType::is_wearable() const
{
    return (TV_WEARABLE_BEGIN <= this->tval) && (this->tval <= TV_WEARABLE_END);
}

/*!
 * @brief オブジェクトが装備品であるかを返す(ObjectType::is_wearableに矢弾を含む) / Equipment including all wearable objects and ammo
 * @return 装備品ならばtrueを返す
 */
bool ObjectType::is_equipment() const
{
    return (TV_EQUIP_BEGIN <= this->tval) && (this->tval <= TV_EQUIP_END);
}

/*!
 * @brief 武器匠の「武器」鑑定対象になるかを判定する。/ Hook to specify "weapon"
 * @return 対象になるならtrueを返す。
 */
bool ObjectType::is_orthodox_melee_weapons() const
{
    switch (this->tval) {
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::DIGGING: {
        return true;
    }
    case ItemKindType::SWORD: {
        if (this->sval != SV_POISON_NEEDLE)
            return true;
    }

    default:
        break;
    }

    return false;
}

/*!
 * @brief 修復対象となる壊れた武器かを判定する。 / Hook to specify "broken weapon"
 * @return 修復対象になるならTRUEを返す。
 */
bool ObjectType::is_broken_weapon() const
{
    if (this->tval != ItemKindType::SWORD)
        return false;

    switch (this->sval) {
    case SV_BROKEN_DAGGER:
    case SV_BROKEN_SWORD:
        return true;
    }

    return false;
}

/*!
 * @brief オブジェクトが投射可能な武器かどうかを返す。
 * @return 投射可能な武器ならばtrue
 */
bool ObjectType::is_throwable() const
{
    return (this->tval == ItemKindType::DIGGING) || (this->tval == ItemKindType::SWORD) || (this->tval == ItemKindType::POLEARM) || (this->tval == ItemKindType::HAFTED);
}

/*!
 * @brief オブジェクトがどちらの手にも装備できる武器かどうかの判定
 * @return 左右両方の手で装備できるならばtrueを返す。
 */
bool ObjectType::is_wieldable_in_etheir_hand() const
{
    return ((ItemKindType::DIGGING <= this->tval) && (this->tval <= ItemKindType::SWORD)) || (this->tval == ItemKindType::SHIELD) || (this->tval == ItemKindType::CAPTURE) || (this->tval == ItemKindType::CARD);
}

/*!
 * @brief オブジェクトが強化不能武器であるかを返す / Poison needle can not be enchanted
 * @return 強化不能ならばtrueを返す
 */
bool ObjectType::refuse_enchant_weapon() const
{
    return (this->tval == ItemKindType::SWORD) && (this->sval == SV_POISON_NEEDLE);
}

/*!
 * @brief オブジェクトが強化可能武器であるかを返す /
 * Check if an object is weapon (including bows and ammo) and allows enchantment
 * @return 強化可能ならばtrueを返す
 */
bool ObjectType::allow_enchant_weapon() const
{
    return this->is_weapon_ammo() && !this->refuse_enchant_weapon();
}

/*!
 * @brief オブジェクトが強化可能な近接武器であるかを返す /
 * Check if an object is melee weapon and allows enchantment
 * @return 強化可能な近接武器ならばtrueを返す
 */
bool ObjectType::allow_enchant_melee_weapon() const
{
    return this->is_melee_weapon() && !this->refuse_enchant_weapon();
}

/*!
 * @brief オブジェクトが両手持ち可能な武器かを返す /
 * Check if an object is melee weapon and allows wielding with two-hands
 * @return 両手持ち可能ならばTRUEを返す
 */
bool ObjectType::allow_two_hands_wielding() const
{
    return this->is_melee_weapon() && ((this->weight > 99) || (this->tval == ItemKindType::POLEARM));
}

/*!
 * @brief オブジェクトが矢弾として使用できるかどうかを返す / Check if an object is ammo
 * @return 矢弾として使えるならばtrueを返す
 */
bool ObjectType::is_ammo() const
{
    return (TV_MISSILE_BEGIN <= this->tval) && (this->tval <= TV_MISSILE_END);
}

/*!
 * @brief 対象のアイテムが矢やクロスボウの矢の材料になるかを返す。/
 * Hook to determine if an object is contertible in an arrow/bolt
 * @return 材料にできるならtrueを返す
 */
bool ObjectType::is_convertible() const
{
    auto is_convertible = ((this->tval == ItemKindType::JUNK) || (this->tval == ItemKindType::SKELETON));
    is_convertible |= ((this->tval == ItemKindType::CORPSE) && (this->sval == SV_SKELETON));
    return is_convertible;
}

bool ObjectType::is_lance() const
{
    auto is_lance = this->tval == ItemKindType::POLEARM;
    is_lance &= (this->sval == SV_LANCE) || (this->sval == SV_HEAVY_LANCE);
    return is_lance;
}

/*!
 * @brief オブジェクトが防具として装備できるかどうかを返す / Check if an object is armour
 * @return 防具として装備できるならばtrueを返す
 */
bool ObjectType::is_armour() const
{
    return (TV_ARMOR_BEGIN <= this->tval) && (this->tval <= TV_ARMOR_END);
}

/*!
 * @brief オブジェクトがレアアイテムかどうかを返す /
 * Rare weapons/aromors including Blade of Chaos, Dragon armors, etc.
 * @return レアアイテムならばTRUEを返す
 */
bool ObjectType::is_rare() const
{
    static const std::unordered_map<ItemKindType, const std::set<OBJECT_SUBTYPE_VALUE>> rare_table = {
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

    if (auto it = rare_table.find(this->tval); it != rare_table.end()) {
        const auto &svals = it->second;
        return svals.empty() || (svals.find(this->sval) != svals.end());
    }

    return false;
}

/*!
 * @brief オブジェクトがエゴアイテムかどうかを返す
 * @return エゴアイテムならばtrueを返す
 */
bool ObjectType::is_ego() const
{
    return this->name2 != 0;
}

/*!
 * @brief オブジェクトが鍛冶師のエッセンス付加済みかを返す /
 * Check if an object is made by a smith's special ability
 * @return エッセンス付加済みならばTRUEを返す
 */
bool ObjectType::is_smith() const
{
    return Smith::object_effect(this).has_value() || Smith::object_activation(this).has_value();
}

/*!
 * @brief オブジェクトがアーティファクトかを返す /
 * Check if an object is artifact
 * @return アーティファクトならばtrueを返す
 */
bool ObjectType::is_artifact() const
{
    return this->is_fixed_artifact() || (this->art_name != 0);
}

/*!
 * @brief オブジェクトが固定アーティファクトかを返す /
 * Check if an object is fixed artifact
 * @return 固定アーティファクトならばtrueを返す
 */
bool ObjectType::is_fixed_artifact() const
{
    return this->name1 != 0;
}

/*!
 * @brief オブジェクトがランダムアーティファクトかを返す /
 * Check if an object is random artifact
 * @return ランダムアーティファクトならばtrueを返す
 */
bool ObjectType::is_random_artifact() const
{
    return this->is_artifact() && !this->is_fixed_artifact();
}

/*!
 * @brief オブジェクトが通常のアイテム(アーティファクト、エゴ、鍛冶師エッセンス付加いずれでもない)かを返す /
 * Check if an object is neither artifact, ego, nor 'smith' object
 * @return 通常のアイテムならばtrueを返す
 */
bool ObjectType::is_nameless() const
{
    return !this->is_artifact() && !this->is_ego() && !this->is_smith();
}

bool ObjectType::is_valid() const
{
    return this->k_idx != 0;
}

bool ObjectType::is_broken() const
{
    return any_bits(this->ident, IDENT_BROKEN);
}

bool ObjectType::is_cursed() const
{
    return this->curse_flags.any();
}

bool ObjectType::is_held_by_monster() const
{
    return this->held_m_idx != 0;
}

/*
 * Determine if a given inventory item is "known"
 * Test One -- Check for special "known" tag
 * Test Two -- Check for "Easy Know" + "Aware"
 */
bool ObjectType::is_known() const
{
    return any_bits(this->ident, IDENT_KNOWN) || (k_info[this->k_idx].easy_know && k_info[this->k_idx].aware);
}

bool ObjectType::is_fully_known() const
{
    return any_bits(this->ident, IDENT_FULL_KNOWN);
}

/*!
 * @brief 与えられたオブジェクトのベースアイテムが鑑定済かを返す / Determine if a given inventory item is "aware"
 * @return 鑑定済ならtrue
 */
bool ObjectType::is_aware() const
{
    return k_info[this->k_idx].aware;
}

/*
 * Determine if a given inventory item is "tried"
 */
bool ObjectType::is_tried() const
{
    return k_info[this->k_idx].tried;
}

/*!
 * @brief オブジェクトが薬であるかを返す
 * @return オブジェクトが薬ならばtrueを返す
 */
bool ObjectType::is_potion() const
{
    return k_info[this->k_idx].tval == ItemKindType::POTION;
}

/*!
 * @brief オブジェクトをプレイヤーが読むことができるかを判定する /
 * Hook to determine if an object is readable
 * @return 読むことが可能ならばtrueを返す
 */
bool ObjectType::is_readable() const
{
    return (this->tval == ItemKindType::SCROLL) || (this->tval == ItemKindType::PARCHMENT) || (this->name1 == ART_GHB) || (this->name1 == ART_POWER);
}

/*!
 * @brief オブジェクトがランタンの燃料になるかどうかを判定する
 * An "item_tester_hook" for refilling lanterns
 * @return オブジェクトがランタンの燃料になるならばTRUEを返す
 */
bool ObjectType::can_refill_lantern() const
{
    return (this->tval == ItemKindType::FLASK) || ((this->tval == ItemKindType::LITE) && (this->sval == SV_LITE_LANTERN));
}

/*!
 * @brief オブジェクトが松明に束ねられるかどうかを判定する
 * An "item_tester_hook" for refilling torches
 * @return オブジェクトが松明に束ねられるならばTRUEを返す
 */
bool ObjectType::can_refill_torch() const
{
    return (this->tval == ItemKindType::LITE) && (this->sval == SV_LITE_TORCH);
}

/*!
 * @brief 魔力充填が可能なアイテムかどうか判定する /
 * Hook for "get_item()".  Determine if something is rechargable.
 * @return 魔力充填が可能ならばTRUEを返す
 */
bool ObjectType::is_rechargeable() const
{
    return (this->tval == ItemKindType::STAFF) || (this->tval == ItemKindType::WAND) || (this->tval == ItemKindType::ROD);
}

/*!
 * @brief 悪魔領域のグレーターデーモン召喚に利用可能な死体かどうかを返す。 / An "item_tester_hook" for offer
 * @return 生贄に使用可能な死体ならばTRUEを返す。
 */
bool ObjectType::is_offerable() const
{
    if ((this->tval != ItemKindType::CORPSE) || (this->sval != SV_CORPSE)) {
        return false;
    }

    return angband_strchr("pht", r_info[this->pval].d_char) != nullptr;
}

/*!
 * @brief オブジェクトをプレイヤーが魔道具として発動できるかを判定する /
 * Hook to determine if an object is activatable
 * @return 魔道具として発動可能ならばTRUEを返す
 */
bool ObjectType::is_activatable() const
{
    if (!this->is_known())
        return false;

    auto flags = object_flags(this);
    return flags.has(TR_ACTIVATE);
}
