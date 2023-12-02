/*
 * @file item-entity.cpp
 * @brief アイテム実体とそれにまつわる判定処理群
 * @author Hourier
 * @date 2022/11/25
 * @details オブジェクトの状態を変更するメソッドは道半ば
 */

#include "system/item-entity.h"
#include "artifact/fixed-art-types.h"
#include "artifact/random-art-effects.h"
#include "monster-race/monster-race.h"
#include "object-enchant/activation-info-table.h"
#include "object-enchant/dragon-breaths-table.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-value.h"
#include "object/tval-types.h"
#include "smith/object-smith.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-ring-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info.h"
#include "system/monster-race-info.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include <sstream>

ItemEntity::ItemEntity()
    : bi_key(BaseitemKey(ItemKindType::NONE))
    , fixed_artifact_idx(FixedArtifactId::NONE)
{
}

/*!
 * @brief アイテムを初期化する
 */
void ItemEntity::wipe()
{
    *this = {};
}

/*!
 * @brief アイテムを複製する
 * @param j_ptr 複製元アイテムへの参照ポインタ
 */
void ItemEntity::copy_from(const ItemEntity *j_ptr)
{
    *this = *j_ptr;
}

/*!
 * @brief アイテム構造体にベースアイテムを作成する
 * @param bi_id 新たに作成したいベースアイテム情報のID
 */
void ItemEntity::prep(short new_bi_id)
{
    const auto &baseitem = baseitems_info[new_bi_id];
    auto old_stack_idx = this->stack_idx;
    this->wipe();
    this->stack_idx = old_stack_idx;
    this->bi_id = new_bi_id;
    this->bi_key = baseitem.bi_key;
    this->pval = baseitem.pval;
    this->number = 1;
    this->weight = baseitem.weight;
    this->to_h = baseitem.to_h;
    this->to_d = baseitem.to_d;
    this->to_a = baseitem.to_a;
    this->ac = baseitem.ac;
    this->dd = baseitem.dd;
    this->ds = baseitem.ds;

    if (baseitem.act_idx > RandomArtActType::NONE) {
        this->activation_id = baseitem.act_idx;
    }

    if (this->get_baseitem().cost <= 0) {
        this->ident |= (IDENT_BROKEN);
    }

    if (baseitem.gen_flags.has(ItemGenerationTraitType::CURSED)) {
        this->curse_flags.set(CurseTraitType::CURSED);
    }

    if (baseitem.gen_flags.has(ItemGenerationTraitType::HEAVY_CURSE)) {
        this->curse_flags.set(CurseTraitType::HEAVY_CURSE);
    }

    if (baseitem.gen_flags.has(ItemGenerationTraitType::PERMA_CURSE)) {
        this->curse_flags.set(CurseTraitType::PERMA_CURSE);
    }

    if (baseitem.gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE0)) {
        this->curse_flags.set(get_curse(0, this));
    }

    if (baseitem.gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE1)) {
        this->curse_flags.set(get_curse(1, this));
    }

    if (baseitem.gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE2)) {
        this->curse_flags.set(get_curse(2, this));
    }
}

bool ItemEntity::is(ItemKindType tval) const
{
    return this->bi_key.is(tval);
}

/*!
 * @brief 武器か否かを判定する
 * @return 武器か否か
 */
bool ItemEntity::is_weapon() const
{
    return this->bi_key.is_weapon();
}

/*!
 * @brief 武器や矢弾として使用できるかを判定する
 * @return 武器や矢弾として使えるか否か
 */
bool ItemEntity::is_weapon_ammo() const
{
    return this->is_weapon() || this->is_ammo();
}

/*!
 * @brief 武器、防具、矢弾として使用できるかを判定する
 * @return 武器、防具、矢弾として使えるか否か
 */
bool ItemEntity::is_weapon_armour_ammo() const
{
    return this->is_weapon_ammo() || this->is_protector();
}

/*!
 * @brief 近接武器として装備できるかを判定する
 * @return 近接武器として使えるか否か
 */
bool ItemEntity::is_melee_weapon() const
{
    return this->bi_key.is_melee_weapon();
}

/*!
 * @brief エッセンスの付加可能な武器や矢弾かを判定する
 * @return エッセンスの付加可能な武器か矢弾か否か
 */
bool ItemEntity::is_melee_ammo() const
{
    return this->bi_key.is_melee_ammo();
}

/*!
 * @brief 装備可能であるかを判定する
 * @return 装備可能か否か
 */
bool ItemEntity::is_wearable() const
{
    return this->bi_key.is_wearable();
}

/*!
 * @brief 装備品であるかを判定する
 * @return 装備品か否か
 */
bool ItemEntity::is_equipment() const
{
    return this->bi_key.is_equipement();
}

/*!
 * @brief 武器匠の「武器」鑑定対象になるかを判定する
 * @return 鑑定対象か否か
 */
bool ItemEntity::is_orthodox_melee_weapons() const
{
    return this->bi_key.is_orthodox_melee_weapon();
}

/*!
 * @brief 修復対象となる壊れた武器かを判定する
 * @return 修復対象か否か
 */
bool ItemEntity::is_broken_weapon() const
{
    return this->bi_key.is_broken_weapon();
}

/*!
 * @brief 投射可能な武器かどうかを判定する
 * @return 投射可能か否か
 */
bool ItemEntity::is_throwable() const
{
    return this->bi_key.is_throwable();
}

/*!
 * @brief どちらの手にも装備できる武器かを判定する
 * @return 左右両方の手で装備可能か否か
 */
bool ItemEntity::is_wieldable_in_etheir_hand() const
{
    return this->bi_key.is_wieldable_in_etheir_hand();
}

/*!
 * @brief 強化不能武器であるかを判定する
 * @return 強化不能か否か
 */
bool ItemEntity::refuse_enchant_weapon() const
{
    return this->bi_key.refuse_enchant_weapon();
}

/*!
 * @brief 強化可能武器であるかを判定する
 * @return 強化可能か否か
 */
bool ItemEntity::allow_enchant_weapon() const
{
    return this->is_weapon_ammo() && !this->refuse_enchant_weapon();
}

/*!
 * @brief 強化可能な近接武器であるかを判定する
 * @return 強化可能な近接武器か否か
 */
bool ItemEntity::allow_enchant_melee_weapon() const
{
    return this->is_melee_weapon() && !this->refuse_enchant_weapon();
}

/*!
 * @brief 両手持ち可能な武器かを判定する
 * @return 両手持ち可能か否か
 */
bool ItemEntity::allow_two_hands_wielding() const
{
    return this->is_melee_weapon() && ((this->weight > 99) || this->is(ItemKindType::POLEARM));
}

/*!
 * @brief 矢弾として使用できるかどうかを判定する
 * @return 矢弾として使えるか否か
 */
bool ItemEntity::is_ammo() const
{
    return this->bi_key.is_ammo();
}

/*!
 * @brief 対象の矢やクロスボウの矢の材料になるかを判定する
 * @return 材料にできるか否か
 */
bool ItemEntity::is_convertible() const
{
    return this->bi_key.is_convertible();
}

bool ItemEntity::is_lance() const
{
    auto is_lance = this->bi_key == BaseitemKey(ItemKindType::POLEARM, SV_LANCE);
    is_lance |= this->bi_key == BaseitemKey(ItemKindType::POLEARM, SV_HEAVY_LANCE);
    return is_lance;
}

/*!
 * @brief 防具かを判定する
 * @return 防具か否か
 */
bool ItemEntity::is_protector() const
{
    return this->bi_key.is_protector();
}

/*!
 * @brief オーラを纏える防具かどうかを判定する
 * @return オーラを纏えるか否か
 */
bool ItemEntity::can_be_aura_protector() const
{
    return this->bi_key.can_be_aura_protector();
}

/*!
 * @brief レアアイテムかどうかを判定する
 * @return レアアイテムか否か
 */
bool ItemEntity::is_rare() const
{
    return this->bi_key.is_rare();
}

/*!
 * @brief エゴアイテムかどうかを判定する
 * @return エゴアイテムか否か
 */
bool ItemEntity::is_ego() const
{
    return this->ego_idx != EgoType::NONE;
}

/*!
 * @brief 鍛冶師のエッセンス付加済かを判定する
 * @return エッセンス付加済か否か
 */
bool ItemEntity::is_smith() const
{
    return Smith::object_effect(this) || Smith::object_activation(this);
}

/*!
 * @brief 固定アーティファクトもしくはランダムアーティファクトであるかを判定する
 * @return 固定アーティファクトもしくはランダムアーティファクトか否か
 */
bool ItemEntity::is_fixed_or_random_artifact() const
{
    return this->is_fixed_artifact() || this->randart_name;
}

/*!
 * @brief 固定アーティファクトかを判定する
 * @return 固定アーティファクトか否か
 */
bool ItemEntity::is_fixed_artifact() const
{
    return !this->is_specific_artifact(FixedArtifactId::NONE);
}

/*!
 * @brief ランダムアーティファクトかを判定する
 * @return ランダムアーティファクトか否か
 */
bool ItemEntity::is_random_artifact() const
{
    return this->is_fixed_or_random_artifact() && !this->is_fixed_artifact();
}

/*!
 * @brief 通常のアイテムかを返す
 * @return アーティファクト、エゴ、鍛冶師エッセンス付加いずれでもないか、どれか1つであるか
 */
bool ItemEntity::is_nameless() const
{
    return !this->is_fixed_or_random_artifact() && !this->is_ego() && !this->is_smith();
}

bool ItemEntity::is_valid() const
{
    return this->bi_id != 0;
}

bool ItemEntity::is_broken() const
{
    return any_bits(this->ident, IDENT_BROKEN);
}

bool ItemEntity::is_cursed() const
{
    return this->curse_flags.any();
}

bool ItemEntity::is_held_by_monster() const
{
    return this->held_m_idx != 0;
}

/*
 * @brief 鑑定済かを判定する
 * @return 鑑定済か否か
 */
bool ItemEntity::is_known() const
{
    const auto &baseitem = this->get_baseitem();
    return any_bits(this->ident, IDENT_KNOWN) || (baseitem.easy_know && baseitem.aware);
}

bool ItemEntity::is_fully_known() const
{
    return any_bits(this->ident, IDENT_FULL_KNOWN);
}

/*!
 * @brief ベースアイテムが鑑定済かを判定する
 * @return 鑑定済か否か
 */
bool ItemEntity::is_aware() const
{
    return this->get_baseitem().aware;
}

/*
 * @brief ベースアイテムが試行済かを判定する
 * @return 試行済か否か
 */
bool ItemEntity::is_tried() const
{
    return this->get_baseitem().tried;
}

/*!
 * @brief 薬であるかを判定する
 * @return 薬か否か
 */
bool ItemEntity::is_potion() const
{
    return this->bi_key.is(ItemKindType::POTION);
}

/*!
 * @brief 読めるアイテムかを判定する
 * @return 読めるか否か
 */
bool ItemEntity::is_readable() const
{
    auto can_read = this->is(ItemKindType::SCROLL);
    can_read |= this->is(ItemKindType::PARCHMENT);
    can_read |= this->is_specific_artifact(FixedArtifactId::GHB);
    can_read |= this->is_specific_artifact(FixedArtifactId::POWER);
    return can_read;
}

/*!
 * @brief ランタンの燃料になるかを判定する
 * @return ランタンの燃料になるか否か
 */
bool ItemEntity::can_refill_lantern() const
{
    return this->is(ItemKindType::FLASK) || (this->bi_key == BaseitemKey(ItemKindType::LITE, SV_LITE_LANTERN));
}

/*!
 * @brief 松明に束ねられるかどうかを判定する
 * @return 松明に束ねられるか否か
 */
bool ItemEntity::can_refill_torch() const
{
    return this->bi_key == BaseitemKey(ItemKindType::LITE, SV_LITE_TORCH);
}

/*!
 * @brief 魔力充填が可能なアイテムかどうか判定する
 * @return 魔力充填が可能か否か
 */
bool ItemEntity::can_recharge() const
{
    return this->bi_key.can_recharge();
}

/*!
 * @brief 悪魔領域のグレーターデーモン召喚に利用可能な死体かどうかを判定する
 * @return 生贄に使用可能な死体か否か
 */
bool ItemEntity::is_offerable() const
{
    if (this->bi_key != BaseitemKey(ItemKindType::CORPSE, SV_CORPSE)) {
        return false;
    }

    return angband_strchr("pht", monraces_info[i2enum<MonsterRaceId>(this->pval)].d_char) != nullptr;
}

/*!
 * @brief 魔道具として発動できるかを判定する
 * @return 発動可能か否か
 */
bool ItemEntity::is_activatable() const
{
    if (!this->is_known()) {
        return false;
    }

    const auto flags = this->get_flags();
    return flags.has(TR_ACTIVATE);
}

/*!
 * @brief 燃料として使えるかを判定する
 * @return 燃料か否か
 */
bool ItemEntity::is_fuel() const
{
    return this->bi_key.is_fuel();
}

/*!
 * @brief 魔法書かどうかを判定する
 * @return 魔法書か否か
 */
bool ItemEntity::is_spell_book() const
{
    return this->bi_key.is_spell_book();
}

/*!
 * @brief 同一の命中値上昇及びダメージ上昇があるかを判定する
 * @return 同一修正か
 * @details 鍛冶師が篭手に殺戮エッセンスを付加した場合のみこの判定に意味がある
 */
bool ItemEntity::is_glove_same_temper(const ItemEntity *j_ptr) const
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
 * @brief 「2つの～～」と重ねられるかを一般的に判定する
 * @return 重ねられるか
 * @details 個別のアイテムによっては別途条件があるが、それはこの関数では判定しない
 */
bool ItemEntity::can_pile(const ItemEntity *j_ptr) const
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

    if (this->is_fixed_or_random_artifact() || j_ptr->is_fixed_or_random_artifact()) {
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
TERM_COLOR ItemEntity::get_color() const
{
    const auto &baseitem = this->get_baseitem();
    const auto flavor = baseitem.flavor;
    if (flavor != 0) {
        return baseitems_info[flavor].x_attr;
    }

    auto has_attr = !this->is_valid();
    has_attr |= this->bi_key != BaseitemKey(ItemKindType::CORPSE, SV_CORPSE);
    has_attr |= baseitem.x_attr != TERM_DARK;
    if (has_attr) {
        return baseitem.x_attr;
    }

    return monraces_info[i2enum<MonsterRaceId>(this->pval)].x_attr;
}

/*
 * @brief アイテムシンボルを取得する
 * @details 未鑑定名のないアイテム (魔法書等)はベースアイテム定義そのまま
 * 未鑑定名のあるアイテム (薬等)は、未鑑定名の割り当てられたシンボル
 * 以上について、設定で変更しているシンボルならばそれを、していないならば定義シンボルを返す
 */
char ItemEntity::get_symbol() const
{
    const auto &baseitem = this->get_baseitem();
    const auto flavor = baseitem.flavor;
    return flavor ? baseitems_info[flavor].x_char : baseitem.x_char;
}

/*!
 * @brief アイテム価格算出のメインルーチン
 * @return 判明している現価格
 */
int ItemEntity::get_price() const
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
int ItemEntity::get_baseitem_price() const
{
    if (this->is_aware()) {
        return this->get_baseitem().cost;
    }

    switch (this->bi_key.tval()) {
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

int ItemEntity::calc_figurine_value() const
{
    const auto r_idx = i2enum<MonsterRaceId>(this->pval);
    return MonraceList::get_instance().calc_figurine_value(r_idx);
}

int ItemEntity::calc_capture_value() const
{
    const auto r_idx = i2enum<MonsterRaceId>(this->pval);
    return MonraceList::get_instance().calc_capture_value(r_idx);
}

bool ItemEntity::is_specific_artifact(FixedArtifactId id) const
{
    return this->fixed_artifact_idx == id;
}

bool ItemEntity::has_unidentified_name() const
{
    return this->bi_key.has_unidentified_name();
}

ItemKindType ItemEntity::get_arrow_kind() const
{
    return this->bi_key.get_arrow_kind();
}

bool ItemEntity::is_wand_rod() const
{
    return this->bi_key.is_wand_rod();
}

bool ItemEntity::is_wand_staff() const
{
    return this->bi_key.is_wand_staff();
}

short ItemEntity::get_bow_energy() const
{
    return this->bi_key.get_bow_energy();
}

int ItemEntity::get_arrow_magnification() const
{
    return this->bi_key.get_arrow_magnification();
}

bool ItemEntity::is_aiming_rod() const
{
    return this->bi_key.is_aiming_rod();
}

bool ItemEntity::is_lite_requiring_fuel() const
{
    return this->bi_key.is_lite_requiring_fuel();
}

bool ItemEntity::is_junk() const
{
    return this->bi_key.is_junk();
}

bool ItemEntity::is_armour() const
{
    return this->bi_key.is_armour();
}

bool ItemEntity::is_cross_bow() const
{
    return this->bi_key.is_cross_bow();
}

bool ItemEntity::is_inscribed() const
{
    return this->inscription != std::nullopt;
}

/*!
 * @brief オブジェクトから発動効果構造体を取得する。
 * @return 発動効果構造体 (なかったら無効イテレータ)
 */
std::vector<ActivationType>::const_iterator ItemEntity::find_activation_info() const
{
    const auto index = this->get_activation_index();
    return std::find_if(activation_info.begin(), activation_info.end(), [index](const auto &x) { return x.index == index; });
}

bool ItemEntity::has_activation() const
{
    return this->get_activation_index() != RandomArtActType::NONE;
}

BaseitemInfo &ItemEntity::get_baseitem() const
{
    return baseitems_info[this->bi_id];
}

EgoItemDefinition &ItemEntity::get_ego() const
{
    return egos_info.at(this->ego_idx);
}

ArtifactType &ItemEntity::get_fixed_artifact() const
{
    return ArtifactsInfo::get_instance().get_artifact(this->fixed_artifact_idx);
}

TrFlags ItemEntity::get_flags() const
{
    const auto &baseitem = this->get_baseitem();
    auto flags = baseitem.flags;

    if (this->is_fixed_artifact()) {
        flags = this->get_fixed_artifact().flags;
    }

    if (this->is_ego()) {
        const auto &ego = this->get_ego();
        flags.set(ego.flags);
        this->modify_ego_lite_flags(flags);
    }

    flags.set(this->art_flags);
    if (auto effect = Smith::object_effect(this); effect) {
        auto tr_flags = Smith::get_effect_tr_flags(*effect);
        flags.set(tr_flags);
    }

    if (Smith::object_activation(this)) {
        flags.set(TR_ACTIVATE);
    }

    return flags;
}

TrFlags ItemEntity::get_flags_known() const
{
    TrFlags flags{};
    if (!this->is_aware()) {
        return flags;
    }

    const auto &baseitem = this->get_baseitem();
    flags = baseitem.flags;
    if (!this->is_known()) {
        return flags;
    }

    if (this->is_ego()) {
        const auto &ego = this->get_ego();
        flags.set(ego.flags);
        this->modify_ego_lite_flags(flags);
    }

    if (this->is_fully_known()) {
        if (this->is_fixed_artifact()) {
            flags = this->get_fixed_artifact().flags;
        }

        flags.set(this->art_flags);
    }

    if (auto effect = Smith::object_effect(this); effect) {
        auto tr_flags = Smith::get_effect_tr_flags(*effect);
        flags.set(tr_flags);
    }

    if (Smith::object_activation(this)) {
        flags.set(TR_ACTIVATE);
    }

    return flags;
}

/*!
 * @brief 発動効果の記述を生成する (メインルーチン)
 * @return 発動効果
 */
std::string ItemEntity::explain_activation() const
{
    const auto flags = this->get_flags();
    if (flags.has_not(TR_ACTIVATE)) {
        return _("なし", "nothing");
    }

    if (this->has_activation()) {
        return this->build_activation_description();
    }

    return this->bi_key.explain_activation();
}

std::string ItemEntity::build_timeout_description(const ActivationType &act) const
{
    const auto constant = act.constant;
    const auto dice = act.dice;
    if (constant == 0 && dice == 0) {
        return _("いつでも", "every turn");
    }

    if (constant) {
        std::stringstream ss;
        ss << _("", "every ");
        if (constant > 0) {
            ss << *constant;
            if (dice > 0) {
                ss << '+';
            }
        }

        if (dice > 0) {
            ss << 'd' << dice;
        }

        ss << _(" ターン毎", " turns");
        return ss.str();
    }

    std::stringstream ss;
    switch (act.index) {
    case RandomArtActType::BR_FIRE:
        ss << _("", "every ") << (this->bi_key == BaseitemKey(ItemKindType::RING, SV_RING_FLAMES) ? 200 : 250) << _(" ターン毎", " turns");
        return ss.str();
    case RandomArtActType::BR_COLD:
        ss << _("", "every ") << (this->bi_key == BaseitemKey(ItemKindType::RING, SV_RING_ICE) ? 200 : 250) << _(" ターン毎", " turns");
        return ss.str();
    case RandomArtActType::TERROR:
        return _("3*(レベル+10) ターン毎", "every 3 * (level+10) turns");
    case RandomArtActType::MURAMASA:
        return _("確率50%で壊れる", "(destroyed 50%)");
    default:
        return "undefined";
    }
}

/*!
 * @brief 発動効果の記述を生成する（サブルーチン/汎用）
 * @return 発動効果
 */
std::string ItemEntity::build_activation_description() const
{
    const auto it = this->find_activation_info();
    if (it == activation_info.end()) {
        return _("未定義", "something undefined");
    }

    const auto activation_description = this->build_activation_description(*it);
    const auto timeout_description = this->build_timeout_description(*it);
    std::stringstream ss;
    ss << activation_description << _(" : ", " ") << timeout_description;
    return ss.str();
}

/*!
 * @brief 鑑定済にする
 */
void ItemEntity::mark_as_known()
{
    this->feeling = FEEL_NONE;
    this->ident &= ~(IDENT_SENSE);
    this->ident &= ~(IDENT_EMPTY);
    this->ident |= (IDENT_KNOWN);
}

/*!
 * @brief 試行済にする
 */
void ItemEntity::mark_as_tried() const
{
    this->get_baseitem().mark_as_tried();
}

/*!
 * @brief エゴ光源のフラグを修正する
 *
 * 寿命のある光源で寿命が0ターンの時、光源エゴアイテムに起因するフラグは
 * 灼熱エゴの火炎耐性を除き付与されないようにする。
 *
 * @param flags フラグ情報を受け取る配列
 */
void ItemEntity::modify_ego_lite_flags(TrFlags &flags) const
{
    if (!this->bi_key.is(ItemKindType::LITE)) {
        return;
    }

    if (!this->is_lite_requiring_fuel() || this->fuel != 0) {
        return;
    }

    switch (this->ego_idx) {
    case EgoType::LITE_AURA_FIRE:
        flags.reset(TR_SH_FIRE);
        return;
    case EgoType::LITE_INFRA:
        flags.reset(TR_INFRA);
        return;
    case EgoType::LITE_EYE:
        flags.reset({ TR_RES_BLIND, TR_SEE_INVIS });
        return;
    default:
        return;
    }
}

/*!
 * @brief 発動効果IDを取得する
 * @details いくつかのケースで定義されている発動効果から、
 * 鍛冶師による付与＞固定アーティファクト＞エゴ＞ランダムアーティファクト＞ベースアイテムの優先順位で走査する
 * @return 発動効果ID
 */
RandomArtActType ItemEntity::get_activation_index() const
{
    if (auto act_idx = Smith::object_activation(this); act_idx) {
        return *act_idx;
    }

    if (this->is_fixed_artifact()) {
        const auto &artifact = this->get_fixed_artifact();
        if (artifact.flags.has(TR_ACTIVATE)) {
            return artifact.act_idx;
        }
    }

    if (this->is_ego()) {
        const auto &ego = this->get_ego();
        if (ego.flags.has(TR_ACTIVATE)) {
            return ego.act_idx;
        }
    }

    if (!this->is_random_artifact()) {
        const auto &baseitem = this->get_baseitem();
        if (baseitem.flags.has(TR_ACTIVATE)) {
            return baseitem.act_idx;
        }
    }

    return this->activation_id;
}

std::string ItemEntity::build_activation_description(const ActivationType &act) const
{
    switch (act.index) {
    case RandomArtActType::NONE:
        return act.desc;
    case RandomArtActType::BR_FIRE:
        if (this->bi_key == BaseitemKey(ItemKindType::RING, SV_RING_FLAMES)) {
            return _("火炎のブレス (200) と火への耐性", "breathe fire (200) and resist fire");
        }

        return act.desc;
    case RandomArtActType::BR_COLD:
        if (this->bi_key == BaseitemKey(ItemKindType::RING, SV_RING_ICE)) {
            return _("冷気のブレス (200) と冷気への耐性", "breathe cold (200) and resist cold");
        }

        return act.desc;
    case RandomArtActType::BR_DRAGON:
        return this->build_activation_description_dragon_breath();
    case RandomArtActType::AGGRAVATE:
        if (this->is_specific_artifact(FixedArtifactId::HYOUSIGI)) {
            return _("拍子木を打ちならす", "beat wooden clappers");
        }

        return act.desc;
    case RandomArtActType::ACID_BALL_AND_RESISTANCE:
        return _("アシッド・ボール (100) と酸への耐性", "ball of acid (100) and resist acid");
    case RandomArtActType::FIRE_BALL_AND_RESISTANCE:
        return _("ファイア・ボール (100) と火への耐性", "ball of fire (100) and resist fire");
    case RandomArtActType::COLD_BALL_AND_RESISTANCE:
        return _("アイス・ボール (100) と冷気への耐性", "ball of cold (100) and resist cold");
    case RandomArtActType::ELEC_BALL_AND_RESISTANCE:
        return _("サンダー・ボール (100) と電撃への耐性", "ball of elec (100) and resist elec");
    case RandomArtActType::POIS_BALL_AND_RESISTANCE:
        return _("ポイズン・ボール (100) と毒への耐性", "ball of poison (100) and resist elec");
    case RandomArtActType::RESIST_ACID:
        return _("一時的な酸への耐性", "temporary resist acid");
    case RandomArtActType::RESIST_FIRE:
        return _("一時的な火への耐性", "temporary resist fire");
    case RandomArtActType::RESIST_COLD:
        return _("一時的な冷気への耐性", "temporary resist cold");
    case RandomArtActType::RESIST_ELEC:
        return _("一時的な電撃への耐性", "temporary resist elec");
    case RandomArtActType::RESIST_POIS:
        return _("一時的な毒への耐性", "temporary resist elec");
    default:
        return act.desc;
    }
}

/*!
 * @brief 発動効果の記述を返す (ドラゴンブレス)
 * @return 発動効果
 */
std::string ItemEntity::build_activation_description_dragon_breath() const
{
    std::stringstream ss;
    ss << _("", "breathe ");
    auto n = 0;
    const auto flags = this->get_flags();
    for (auto i = 0; dragonbreath_info[i].flag != 0; i++) {
        if (flags.has(dragonbreath_info[i].flag)) {
            if (n > 0) {
                ss << _("、", ", ");
            }

            ss << dragonbreath_info[i].name;
            n++;
        }
    }

    ss << _("のブレス(250)", " (250)");
    return ss.str();
}
