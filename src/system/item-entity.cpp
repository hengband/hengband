/*
 * @file item-entity.cpp
 * @brief アイテム実体とそれにまつわる判定処理群
 * @author Hourier
 * @date 2022/11/25
 * @details オブジェクトの状態を変更するメソッドは道半ば
 */

#include "system/item-entity.h"
#include "artifact/fixed-art-types.h"
#include "artifact/random-art-bias-types.h"
#include "artifact/random-art-effects.h"
#include "dungeon/quest.h"
#include "game-option/birth-options.h"
#include "game-option/game-play-options.h"
#include "object-enchant/activation-info-table.h"
#include "object-enchant/dragon-breaths-table.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-value.h"
#include "object/tval-types.h"
#include "smith/object-smith.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-ring-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monster-race-info.h"
#include "term/term-color-types.h"
#include "tracking/baseitem-tracker.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "world/world.h"
#include <algorithm>
#include <sstream>

ItemEntity::ItemEntity()
    : bi_key(BaseitemKey(ItemKindType::NONE))
    , fa_id(FixedArtifactId::NONE)
{
}

ItemEntity::ItemEntity(short bi_id)
{
    this->generate(bi_id);
}

ItemEntity::ItemEntity(const BaseitemKey &bi_key)
{
    const auto &baseitems = BaseitemList::get_instance();
    this->generate(baseitems.lookup_baseitem_id(bi_key));
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

void ItemEntity::generate(const BaseitemKey &new_bi_key)
{
    const auto new_bi_id = BaseitemList::get_instance().lookup_baseitem_id(new_bi_key);
    this->generate(new_bi_id);
}

/*!
 * @brief アイテム構造体にベースアイテムを作成する
 * @param bi_id 新たに作成したいベースアイテム情報のID
 */
void ItemEntity::generate(short new_bi_id)
{
    const auto &baseitem = BaseitemList::get_instance().get_baseitem(new_bi_id);
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
    this->damage_dice = baseitem.damage_dice;

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
 * @brief 強化可能武器であるかを判定する
 * @return 強化可能か否か
 */
bool ItemEntity::allow_enchant_weapon() const
{
    return this->is_weapon_ammo() && !this->should_refuse_enchant();
}

/*!
 * @brief 強化可能な近接武器であるかを判定する
 * @return 強化可能な近接武器か否か
 */
bool ItemEntity::allow_enchant_melee_weapon() const
{
    return this->is_melee_weapon() && !this->should_refuse_enchant();
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
    return this->bi_key.is_lance();
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
    auto can_read = this->bi_key.is_readable();
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
    if (!this->is_corpse()) {
        return false;
    }

    return this->get_monrace().symbol_char_is_any_of("pht");
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

    if (this->damage_dice != j_ptr->damage_dice) {
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

/*!
 * @brief アイテムのシンボルを取得する
 * @return シンボル
 * @details シンボル変更コマンドの影響を受けたくないので毎回ベースアイテムからシンボルを引っ張って返す.
 */
DisplaySymbol ItemEntity::get_symbol() const
{
    return { this->get_color(), this->get_character() };
}

/*!
 * @brief アイテム価格算出のメインルーチン
 * @return 判明している現価格
 */
int ItemEntity::calc_price() const
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
    return this->get_monrace().calc_figurine_value();
}

int ItemEntity::calc_capture_value() const
{
    return this->get_monrace().calc_capture_value();
}

/*!
 * @brief 強化不能武器であるかを判定する
 * @return 強化不能か否か
 */
bool ItemEntity::should_refuse_enchant() const
{
    return this->bi_key.should_refuse_enchant();
}

bool ItemEntity::is_specific_artifact(FixedArtifactId id) const
{
    return this->fa_id == id;
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

bool ItemEntity::is_corpse() const
{
    return this->bi_key.is_corpse();
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

bool ItemEntity::has_bias() const
{
    return this->artifact_bias != RandomArtifactBias::NONE;
}

bool ItemEntity::is_bounty() const
{
    if (this->bi_key.tval() != ItemKindType::MONSTER_REMAINS) {
        return false;
    }

    if (vanilla_town) {
        return false;
    }

    const auto monrace_id = this->get_monrace_id();
    if (MonraceList::is_tsuchinoko(monrace_id)) {
        return true;
    }

    const auto &world = AngbandWorld::get_instance();
    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    if (world.knows_daily_bounty && (monrace.name == world.get_today_bounty().name)) {
        return true;
    }

    return monrace.is_bounty(true);
}

bool ItemEntity::is_target_of(QuestId quest_id) const
{
    if (!inside_quest(quest_id)) {
        return false;
    }

    const auto &quest = QuestList::get_instance().get_quest(quest_id);
    if (!quest.has_reward()) {
        return false;
    }

    const auto &artifact = quest.get_reward();
    if (artifact.gen_flags.has(ItemGenerationTraitType::INSTA_ART)) {
        return false;
    }

    return this->bi_key == artifact.bi_key;
}

BaseitemInfo &ItemEntity::get_baseitem() const
{
    return BaseitemList::get_instance().get_baseitem(this->bi_id);
}

EgoItemDefinition &ItemEntity::get_ego() const
{
    return egos_info.at(this->ego_idx);
}

ArtifactType &ItemEntity::get_fixed_artifact() const
{
    return ArtifactList::get_instance().get_artifact(this->fa_id);
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

    return _("何も起きない", "Nothing");
}

/*!
 * @brief アイテムのpvalがモンスター種族IDを指しているかチェックする
 * @return モンスター種族IDの意図で使われているか否か
 */
bool ItemEntity::has_monrace() const
{
    if ((this->pval < 0) || !this->bi_key.is_monster()) {
        return false;
    }

    if (!this->bi_key.is(ItemKindType::CAPTURE) && (this->pval == 0)) {
        return false;
    }

    const auto &monraces = MonraceList::get_instance();
    return this->pval < static_cast<short>(monraces.size());
}

/*!
 * @brief アイテムのpvalからモンスター種族を引いて返す
 * @return モンスター種族定義
 * @details 死体/骨・モンスターボール・人形・像が該当する.
 */
const MonraceDefinition &ItemEntity::get_monrace() const
{
    if (!this->has_monrace()) {
        THROW_EXCEPTION(std::logic_error, "This item is not related to monrace!");
    }

    const auto monrace_id = this->get_monrace_id();
    return MonraceList::get_instance().get_monrace(monrace_id);
}

void ItemEntity::track_baseitem() const
{
    BaseitemTracker::get_instance().set_trackee(this->bi_id);
}

/*!
 * @brief アイテムを同一スロットへ重ねることができるかどうかを返す
 * @param other 検証したいアイテムへの参照
 * @return 重ね合わせ可能ならばTRUE
 */
bool ItemEntity::is_similar(const ItemEntity &other) const
{
    const auto total = this->number + other.number;
    const auto max_num = this->is_similar_part(other);
    return (max_num != 0) && (total <= max_num);
}

/*!
 * @brief 両オブジェクトをスロットに重ね合わせ可能な最大数を返す
 * @param other 検証したいアイテムへの参照
 * @return 重ね合わせ可能なアイテム数
 */
int ItemEntity::is_similar_part(const ItemEntity &other) const
{
    if (this->bi_id != other.bi_id) {
        return 0;
    }

    constexpr auto max_stack_size = 99;
    auto max_num = max_stack_size;
    switch (this->bi_key.tval()) {
    case ItemKindType::CHEST:
    case ItemKindType::CARD:
    case ItemKindType::CAPTURE:
        return 0;
    case ItemKindType::STATUE: {
        if (this->bi_key.are_both_statue(other.bi_key) || (this->pval != other.pval)) {
            return 0;
        }

        break;
    }
    case ItemKindType::FIGURINE:
    case ItemKindType::MONSTER_REMAINS:
        if (this->pval != other.pval) {
            return 0;
        }

        break;
    case ItemKindType::FOOD:
    case ItemKindType::POTION:
    case ItemKindType::SCROLL:
        break;
    case ItemKindType::STAFF:
        if ((none_bits(this->ident, IDENT_EMPTY) && !this->is_known()) || (none_bits(other.ident, IDENT_EMPTY) && !other.is_known())) {
            return 0;
        }

        if (this->pval != other.pval) {
            return 0;
        }

        break;
    case ItemKindType::WAND:
        if ((none_bits(this->ident, IDENT_EMPTY) && !this->is_known()) || (none_bits(other.ident, IDENT_EMPTY) && !other.is_known())) {
            return 0;
        }

        break;
    case ItemKindType::ROD:
        max_num = std::min(max_num, MAX_SHORT / this->get_baseitem().pval);
        break;
    case ItemKindType::GLOVES:
        if (this->is_glove_same_temper(&other)) {
            return 0;
        }

        if (!this->is_known() || !other.is_known()) {
            return 0;
        }

        if (!this->can_pile(&other)) {
            return 0;
        }

        break;
    case ItemKindType::LITE:
        if (this->fuel != other.fuel) {
            return 0;
        }

        if (!this->is_known() || !other.is_known()) {
            return 0;
        }

        if (!this->can_pile(&other)) {
            return 0;
        }

        break;
    case ItemKindType::BOW:
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
    case ItemKindType::BOOTS:
    case ItemKindType::HELM:
    case ItemKindType::CROWN:
    case ItemKindType::SHIELD:
    case ItemKindType::CLOAK:
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::DRAG_ARMOR:
    case ItemKindType::RING:
    case ItemKindType::AMULET:
    case ItemKindType::WHISTLE:
        if (!this->is_known() || !other.is_known()) {
            return 0;
        }

        if (!this->can_pile(&other)) {
            return 0;
        }

        break;
    case ItemKindType::BOLT:
    case ItemKindType::ARROW:
    case ItemKindType::SHOT:
        if (!this->can_pile(&other)) {
            return 0;
        }

        break;
    default:
        if (!this->is_known() || !other.is_known()) {
            return 0;
        }

        break;
    }

    if (this->art_flags != other.art_flags) {
        return 0;
    }

    if (this->curse_flags != other.curse_flags) {
        return 0;
    }

    if (any_bits(this->ident, IDENT_BROKEN) != any_bits(other.ident, IDENT_BROKEN)) {
        return 0;
    }

    if (this->is_inscribed() && other.is_inscribed() && (this->inscription != other.inscription)) {
        return 0;
    }

    if (!stack_force_notes && (this->inscription != other.inscription)) {
        return 0;
    }

    if (!stack_force_costs && (this->discount != other.discount)) {
        return 0;
    }

    return max_num;
}

/*!
 * @brief 店舗に並べた品を同一品であるかどうか判定する
 * @param other 比較対象アイテムへの参照
 * @return 同一扱いできるならTRUEを返す
 */
bool ItemEntity::is_similar_for_store(const ItemEntity &other) const
{
    if (this == &other) {
        return false;
    }

    if (this->bi_id != other.bi_id) {
        return false;
    }

    if ((this->pval != other.pval) && !this->is_wand_rod()) {
        return false;
    }

    if (this->to_h != other.to_h) {
        return false;
    }

    if (this->to_d != other.to_d) {
        return false;
    }

    if (this->to_a != other.to_a) {
        return false;
    }

    if (this->ego_idx != other.ego_idx) {
        return false;
    }

    if (this->is_fixed_or_random_artifact() || other.is_fixed_or_random_artifact()) {
        return false;
    }

    if (this->art_flags != other.art_flags) {
        return false;
    }

    if (this->timeout || other.timeout) {
        return false;
    }

    if (this->ac != other.ac) {
        return false;
    }

    if (this->damage_dice != other.damage_dice) {
        return false;
    }

    const auto tval = this->bi_key.tval();
    if (tval == ItemKindType::CHEST) {
        return false;
    }

    if (tval == ItemKindType::STATUE) {
        return false;
    }

    if (tval == ItemKindType::CAPTURE) {
        return false;
    }

    if ((tval == ItemKindType::LITE) && (this->fuel != other.fuel)) {
        return false;
    }

    if (this->discount != other.discount) {
        return false;
    }

    return true;
}

std::string ItemEntity::build_timeout_description(const ActivationType &act) const
{
    const auto description = act.build_timeout_description();
    if (description) {
        return *description;
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
 * @brief 非INSTA_ART型の固定アーティファクトの生成を確率に応じて試行する.
 * @param dungeon_level ダンジョンの階層
 * @return 生成に成功したか失敗したか
 */
bool ItemEntity::try_become_artifact(int dungeon_level)
{
    if (this->number != 1) {
        return false;
    }

    for (const auto &[a_idx, artifact] : ArtifactList::get_instance()) {
        if (!artifact.can_generate(this->bi_key)) {
            continue;
        }

        if ((artifact.level > dungeon_level) && !one_in_((artifact.level - dungeon_level) * 2)) {
            continue;
        }

        if (!one_in_(artifact.rarity)) {
            continue;
        }

        this->fa_id = a_idx;
        return true;
    }

    return false;
}

/*!
 * @brief 両オブジェクトをスロットに重ね合わせる
 * @param other 重ね合わせ元アイテムへの参照
 */
void ItemEntity::absorb(ItemEntity &other)
{
    int max_num = this->is_similar_part(other);
    int total = this->number + other.number;
    int diff = (total > max_num) ? total - max_num : 0;

    this->number = (total > max_num) ? max_num : total;
    if (other.is_known()) {
        this->mark_as_known();
    }

    if (((this->ident & IDENT_STORE) || (other.ident & IDENT_STORE)) && (!((this->ident & IDENT_STORE) && (other.ident & IDENT_STORE)))) {
        if (other.ident & IDENT_STORE) {
            other.ident &= 0xEF;
        }

        if (this->ident & IDENT_STORE) {
            this->ident &= 0xEF;
        }
    }

    if (other.is_fully_known()) {
        this->ident |= (IDENT_FULL_KNOWN);
    }

    if (other.is_inscribed()) {
        this->inscription = other.inscription;
    }

    if (other.feeling) {
        this->feeling = other.feeling;
    }

    if (this->discount < other.discount) {
        this->discount = other.discount;
    }

    const auto tval = this->bi_key.tval();
    if (tval == ItemKindType::ROD) {
        this->pval += other.pval * (other.number - diff) / other.number;
        this->timeout += other.timeout * (other.number - diff) / other.number;
    }

    if (tval == ItemKindType::WAND) {
        this->pval += other.pval * (other.number - diff) / other.number;
    }
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
    const auto flags = this->get_flags();
    return DragonBreaths::build_description(flags);
}

/*
 * @brief アイテムの色を取得する
 * @details 未鑑定名のあるアイテム (薬等)は、未鑑定名の割り当てられた色を返す
 * 未鑑定名のないアイテム (魔法書等)はベースアイテム定義そのままを返す
 * その中でモンスターの死体以外は、ベースアイテムの色を返す
 * モンスターの死体は、元モンスターの色を返す
 * 異常アイテム (「何か」)の場合、ベースアイテム定義に基づき黒を返す
 */
uint8_t ItemEntity::get_color() const
{
    const auto &baseitem = this->get_baseitem();
    const auto flavor = baseitem.flavor;
    if (flavor != 0) {
        return BaseitemList::get_instance().get_baseitem(flavor).symbol_config.color;
    }

    const auto &symbol_config = baseitem.symbol_config;
    auto has_attr = this->is_valid();
    has_attr &= this->is_corpse();
    has_attr &= symbol_config.color == TERM_DARK;
    if (!has_attr) {
        return symbol_config.color;
    }

    return this->get_monrace().symbol_config.color;
}

/*
 * @brief アイテムシンボルを取得する
 * @details 未鑑定名のないアイテム (魔法書等)はベースアイテム定義そのまま
 * 未鑑定名のあるアイテム (薬等)は、未鑑定名の割り当てられたシンボル
 * 以上について、設定で変更しているシンボルならばそれを、していないならば定義シンボルを返す
 */
char ItemEntity::get_character() const
{
    const auto &baseitem = this->get_baseitem();
    const auto flavor = baseitem.flavor;
    return flavor ? BaseitemList::get_instance().get_baseitem(flavor).symbol_config.character : baseitem.symbol_config.character;
}

MonraceId ItemEntity::get_monrace_id() const
{
    if (!this->has_monrace()) {
        THROW_EXCEPTION(std::logic_error, "This item is not related to monrace!");
    }

    return i2enum<MonraceId>(this->pval);
}
