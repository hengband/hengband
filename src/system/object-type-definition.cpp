/*
 * @file object-type-definition.h
 * @brief アイテム定義の構造体とエンティティ処理実装
 * @author Hourier
 * @date 2022/10/09
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
#include "object/object-value.h"
#include "object/tval-types.h"
#include "smith/object-smith.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/baseitem-info-definition.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include <set>
#include <unordered_map>

ObjectType::ObjectType()
    : fixed_artifact_idx(FixedArtifactId::NONE)
{
}

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
void ObjectType::copy_from(const ObjectType *j_ptr)
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
    auto *k_ptr = &baseitems_info[ko_idx];
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

    if (k_ptr->act_idx > RandomArtActType::NONE) {
        this->activation_id = k_ptr->act_idx;
    }
    if (baseitems_info[this->k_idx].cost <= 0) {
        this->ident |= (IDENT_BROKEN);
    }

    if (k_ptr->gen_flags.has(ItemGenerationTraitType::CURSED)) {
        this->curse_flags.set(CurseTraitType::CURSED);
    }
    if (k_ptr->gen_flags.has(ItemGenerationTraitType::HEAVY_CURSE)) {
        this->curse_flags.set(CurseTraitType::HEAVY_CURSE);
    }
    if (k_ptr->gen_flags.has(ItemGenerationTraitType::PERMA_CURSE)) {
        this->curse_flags.set(CurseTraitType::PERMA_CURSE);
    }
    if (k_ptr->gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE0)) {
        this->curse_flags.set(get_curse(0, this));
    }
    if (k_ptr->gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE1)) {
        this->curse_flags.set(get_curse(1, this));
    }
    if (k_ptr->gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE2)) {
        this->curse_flags.set(get_curse(2, this));
    }
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
    case ItemKindType::SHOT:
        return true;
    case ItemKindType::SWORD:
        return this->sval != SV_POISON_NEEDLE;
    default:
        return false;
    }
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
    case ItemKindType::DIGGING:
        return true;
    case ItemKindType::SWORD:
        return this->sval != SV_POISON_NEEDLE;
    default:
        return false;
    }
}

/*!
 * @brief 修復対象となる壊れた武器かを判定する。 / Hook to specify "broken weapon"
 * @return 修復対象になるならTRUEを返す。
 */
bool ObjectType::is_broken_weapon() const
{
    if (this->tval != ItemKindType::SWORD) {
        return false;
    }

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
    switch (this->tval) {
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
        return true;
    default:
        return false;
    }
}

/*!
 * @brief オブジェクトがどちらの手にも装備できる武器かどうかの判定
 * @return 左右両方の手で装備できるならばtrueを返す。
 */
bool ObjectType::is_wieldable_in_etheir_hand() const
{
    switch (this->tval) {
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
    return this->ego_idx != EgoType::NONE;
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
    return this->fixed_artifact_idx != FixedArtifactId::NONE;
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
    return any_bits(this->ident, IDENT_KNOWN) || (baseitems_info[this->k_idx].easy_know && baseitems_info[this->k_idx].aware);
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
    return baseitems_info[this->k_idx].aware;
}

/*
 * Determine if a given inventory item is "tried"
 */
bool ObjectType::is_tried() const
{
    return baseitems_info[this->k_idx].tried;
}

/*!
 * @brief オブジェクトが薬であるかを返す
 * @return オブジェクトが薬ならばtrueを返す
 */
bool ObjectType::is_potion() const
{
    return baseitems_info[this->k_idx].tval == ItemKindType::POTION;
}

/*!
 * @brief オブジェクトをプレイヤーが読むことができるかを判定する /
 * Hook to determine if an object is readable
 * @return 読むことが可能ならばtrueを返す
 */
bool ObjectType::is_readable() const
{
    const auto is_scroll = this->tval == ItemKindType::SCROLL;
    const auto is_parchment = this->tval == ItemKindType::PARCHMENT;
    const auto is_ghb = this->fixed_artifact_idx == FixedArtifactId::GHB;
    const auto is_ring_power = this->fixed_artifact_idx == FixedArtifactId::POWER;
    return is_scroll || is_parchment || is_ghb || is_ring_power;
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
    switch (this->tval) {
    case ItemKindType::STAFF:
    case ItemKindType::WAND:
    case ItemKindType::ROD:
        return true;
    default:
        return false;
    }
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

    return angband_strchr("pht", monraces_info[i2enum<MonsterRaceId>(this->pval)].d_char) != nullptr;
}

/*!
 * @brief オブジェクトをプレイヤーが魔道具として発動できるかを判定する /
 * Hook to determine if an object is activatable
 * @return 魔道具として発動可能ならばTRUEを返す
 */
bool ObjectType::is_activatable() const
{
    if (!this->is_known()) {
        return false;
    }

    auto flags = object_flags(this);
    return flags.has(TR_ACTIVATE);
}

/*!
 * @brief オブジェクトが燃料として使えるかを判定する
 * @return 燃料か否か
 */
bool ObjectType::is_fuel() const
{
    auto is_fuel = (this->tval == ItemKindType::LITE) && ((this->sval == SV_LITE_TORCH) || (this->sval == SV_LITE_LANTERN));
    is_fuel |= (this->tval == ItemKindType::FLASK) && (this->sval == SV_FLASK_OIL);
    return is_fuel;
}

/*!
 * @brief オブジェクトが同一の命中値上昇及びダメージ上昇があるかを判定する
 * @return 同一修正か
 * @details 鍛冶師が篭手に殺戮エッセンスを付加した場合のみこの判定に意味がある
 */
bool ObjectType::is_glove_same_temper(const ObjectType *j_ptr) const
{
    if (!this->is_smith() || !j_ptr->is_smith()) {
        return false;
    }

    if (this->smith_hit != j_ptr->smith_hit) {
        return true;
    }

    if (this->smith_damage != j_ptr->smith_damage) {
        return true;
    }

    return false;
}

/*!
 * @brief オブジェクトが「2つの～～」と重ねられるかを一般的に判定する
 * @return 重ねられるか
 * @details 個別のアイテムによっては別途条件があるが、それはこの関数では判定しない
 */
bool ObjectType::can_pile(const ObjectType *j_ptr) const
{
    if (this->is_known() != j_ptr->is_known()) {
        return false;
    }

    if (this->feeling != j_ptr->feeling) {
        return false;
    }

    if (this->to_h != j_ptr->to_h) {
        return false;
    }

    if (this->to_d != j_ptr->to_d) {
        return false;
    }

    if (this->to_a != j_ptr->to_a) {
        return false;
    }

    if (this->pval != j_ptr->pval) {
        return false;
    }

    if (this->is_artifact() || j_ptr->is_artifact()) {
        return false;
    }

    if (this->ego_idx != j_ptr->ego_idx) {
        return false;
    }

    if (this->timeout || j_ptr->timeout) {
        return false;
    }

    if (this->ac != j_ptr->ac) {
        return false;
    }

    if (this->dd != j_ptr->dd) {
        return false;
    }

    if (this->ds != j_ptr->ds) {
        return false;
    }

    if (Smith::object_effect(this) != Smith::object_effect(j_ptr)) {
        return false;
    }

    if (Smith::object_activation(this) != Smith::object_activation(j_ptr)) {
        return false;
    }

    return true;
}

/*
 * @brief アイテムの色を取得する
 * @details 未鑑定名のあるアイテム (薬等)は、未鑑定名の割り当てられた色を返す
 * 未鑑定名のないアイテム (魔法書等)はベースアイテム定義そのままを返す
 * その中でモンスターの死体以外は、ベースアイテムの色を返す
 * モンスターの死体は、元モンスターの色を返す
 * 異常アイテム (「何か」)の場合、ベースアイテム定義に基づき黒を返す
 */
TERM_COLOR ObjectType::get_color() const
{
    const auto &base_item = baseitems_info[this->k_idx];
    const auto flavor = base_item.flavor;
    if (flavor != 0) {
        return baseitems_info[flavor].x_attr;
    }

    auto has_attr = this->k_idx == 0;
    has_attr |= (this->tval != ItemKindType::CORPSE) || (this->sval != SV_CORPSE);
    has_attr |= base_item.x_attr != TERM_DARK;
    if (has_attr) {
        return base_item.x_attr;
    }

    return monraces_info[i2enum<MonsterRaceId>(this->pval)].x_attr;
}

/*
 * @brief アイテムシンボルを取得する
 * @details 未鑑定名のないアイテム (魔法書等)はベースアイテム定義そのまま
 * 未鑑定名のあるアイテム (薬等)は、未鑑定名の割り当てられたシンボル
 * @todo 色と違って変える必要はない……はず？
 */
char ObjectType::get_symbol() const
{
    const auto &base_item = baseitems_info[this->k_idx];
    const auto flavor = base_item.flavor;
    return flavor ? baseitems_info[flavor].x_char : base_item.x_char;
}

/*!
 * @brief オブジェクト価格算出のメインルーチン
 * @return オブジェクトの判明している現価格
 */
int ObjectType::get_price() const
{
    int value;
    const auto is_worthless = this->is_broken() || this->is_cursed();
    if (this->is_known()) {
        if (is_worthless) {
            return 0;
        }

        value = object_value_real(this);
    } else {
        if (any_bits(this->ident, IDENT_SENSE) && is_worthless) {
            return 0;
        }

        value = this->get_baseitem_price();
    }

    if (this->discount) {
        value -= (value * this->discount / 100L);
    }

    return value;
}

/*!
 * @brief 未鑑定なベースアイテムの基本価格を返す
 * @return オブジェクトの未鑑定価格
 */
int ObjectType::get_baseitem_price() const
{
    if (this->is_aware()) {
        return baseitems_info[this->k_idx].cost;
    }

    switch (this->tval) {
    case ItemKindType::FOOD:
        return 5;
    case ItemKindType::POTION:
    case ItemKindType::SCROLL:
        return 20;
    case ItemKindType::STAFF:
        return 70;
    case ItemKindType::WAND:
        return 50;
    case ItemKindType::ROD:
        return 90;
    case ItemKindType::RING:
    case ItemKindType::AMULET:
        return 45;
    case ItemKindType::FIGURINE:
        return this->calc_figurine_value();
    case ItemKindType::CAPTURE:
        return this->calc_capture_value();
    default:
        return 0;
    }
}

int ObjectType::calc_figurine_value() const
{
    auto figure_r_idx = i2enum<MonsterRaceId>(this->pval);
    auto level = monraces_info[figure_r_idx].level;
    if (level < 20) {
        return level * 50L;
    }

    if (level < 30) {
        return 1000 + (level - 20) * 150;
    }

    if (level < 40) {
        return 2500 + (level - 30) * 350;
    }

    if (level < 50) {
        return 6000 + (level - 40) * 800;
    }

    return 14000 + (level - 50) * 2000;
}

int ObjectType::calc_capture_value() const
{
    auto capture_r_idx = i2enum<MonsterRaceId>(this->pval);
    if (!MonsterRace(capture_r_idx).is_valid()) {
        return 1000;
    }

    return (monraces_info[capture_r_idx].level) * 50 + 1000;
}
