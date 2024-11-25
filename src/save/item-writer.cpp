#include "save/item-writer.h"
#include "artifact/random-art-effects.h"
#include "load/old/item-flag-types-savefile50.h"
#include "save/save-util.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/item-entity.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"

static BIT_FLAGS write_item_flags(const ItemEntity &item)
{
    BIT_FLAGS flags = 0x00000000;
    if (item.pval) {
        set_bits(flags, SaveDataItemFlagType::PVAL);
    }

    if (item.discount) {
        set_bits(flags, SaveDataItemFlagType::DISCOUNT);
    }

    if (item.number != 1) {
        set_bits(flags, SaveDataItemFlagType::NUMBER);
    }

    if (item.is_fixed_artifact()) {
        set_bits(flags, SaveDataItemFlagType::FIXED_ARTIFACT_IDX);
    }

    if (item.is_ego()) {
        set_bits(flags, SaveDataItemFlagType::EGO_IDX);
    }

    if (item.timeout) {
        set_bits(flags, SaveDataItemFlagType::TIMEOUT);
    }

    if (item.to_h) {
        set_bits(flags, SaveDataItemFlagType::TO_H);
    }

    if (item.to_d) {
        set_bits(flags, SaveDataItemFlagType::TO_D);
    }

    if (item.to_a) {
        set_bits(flags, SaveDataItemFlagType::TO_A);
    }

    if (item.ac) {
        set_bits(flags, SaveDataItemFlagType::AC);
    }

    if (item.damage_dice.num != 0) {
        set_bits(flags, SaveDataItemFlagType::DD);
    }

    if (item.damage_dice.sides != 0) {
        set_bits(flags, SaveDataItemFlagType::DS);
    }

    if (item.ident) {
        set_bits(flags, SaveDataItemFlagType::IDENT);
    }

    if (item.marked.any()) {
        set_bits(flags, SaveDataItemFlagType::MARKED);
    }

    if (item.art_flags.any()) {
        set_bits(flags, SaveDataItemFlagType::ART_FLAGS);
    }

    if (item.curse_flags.any()) {
        set_bits(flags, SaveDataItemFlagType::CURSE_FLAGS);
    }

    if (item.held_m_idx) {
        set_bits(flags, SaveDataItemFlagType::HELD_M_IDX);
    }

    if (item.activation_id > RandomArtActType::NONE) {
        set_bits(flags, SaveDataItemFlagType::ACTIVATION_ID);
    }

    if (item.chest_level > 0) {
        set_bits(flags, SaveDataItemFlagType::CHEST_LEVEL);
    }

    if (item.captured_monster_speed > 0) {
        set_bits(flags, SaveDataItemFlagType::CAPTURED_MONSTER_SPEED);
    }

    if (item.fuel > 0) {
        set_bits(flags, SaveDataItemFlagType::FUEL);
    }

    if (item.captured_monster_current_hp > 0) {
        set_bits(flags, SaveDataItemFlagType::CAPTURED_MONSTER_CURRENT_HP);
    }

    if (item.captured_monster_max_hp) {
        set_bits(flags, SaveDataItemFlagType::XTRA5);
    }

    if (item.feeling) {
        set_bits(flags, SaveDataItemFlagType::FEELING);
    }

    if (item.is_inscribed()) {
        set_bits(flags, SaveDataItemFlagType::INSCRIPTION);
    }

    if (item.is_random_artifact()) {
        set_bits(flags, SaveDataItemFlagType::ART_NAME);
    }

    if (item.stack_idx) {
        set_bits(flags, SaveDataItemFlagType::STACK_IDX);
    }

    if (item.is_smith()) {
        set_bits(flags, SaveDataItemFlagType::SMITH);
    }

    wr_u32b(flags);
    return flags;
}

static void write_item_info(const ItemEntity &item, const BIT_FLAGS flags)
{
    wr_s16b((int16_t)item.weight);
    if (any_bits(flags, SaveDataItemFlagType::FIXED_ARTIFACT_IDX)) {
        wr_s16b(enum2i(item.fa_id));
    }

    if (any_bits(flags, SaveDataItemFlagType::EGO_IDX)) {
        wr_byte((byte)item.ego_idx);
    }

    if (any_bits(flags, SaveDataItemFlagType::TIMEOUT)) {
        wr_s16b(item.timeout);
    }

    if (any_bits(flags, SaveDataItemFlagType::TO_H)) {
        wr_s16b(item.to_h);
    }

    if (any_bits(flags, SaveDataItemFlagType::TO_D)) {
        wr_s16b((int16_t)item.to_d);
    }

    if (any_bits(flags, SaveDataItemFlagType::TO_A)) {
        wr_s16b(item.to_a);
    }

    if (any_bits(flags, SaveDataItemFlagType::AC)) {
        wr_s16b(item.ac);
    }

    if (any_bits(flags, SaveDataItemFlagType::DD)) {
        wr_byte((byte)item.damage_dice.num);
    }

    if (any_bits(flags, SaveDataItemFlagType::DS)) {
        wr_byte((byte)item.damage_dice.sides);
    }

    if (any_bits(flags, SaveDataItemFlagType::IDENT)) {
        wr_byte(item.ident);
    }

    if (any_bits(flags, SaveDataItemFlagType::MARKED)) {
        wr_FlagGroup_bytes(item.marked, wr_byte, 1);
    }

    if (any_bits(flags, SaveDataItemFlagType::ART_FLAGS)) {
        wr_FlagGroup(item.art_flags, wr_byte);
    }

    if (any_bits(flags, SaveDataItemFlagType::CURSE_FLAGS)) {
        wr_FlagGroup(item.curse_flags, wr_byte);
    }

    if (any_bits(flags, SaveDataItemFlagType::HELD_M_IDX)) {
        wr_s16b(item.held_m_idx);
    }

    if (any_bits(flags, SaveDataItemFlagType::ACTIVATION_ID)) {
        wr_s16b(enum2i(item.activation_id));
    }

    if (any_bits(flags, SaveDataItemFlagType::CHEST_LEVEL)) {
        wr_byte(item.chest_level);
    }

    if (any_bits(flags, SaveDataItemFlagType::CAPTURED_MONSTER_SPEED)) {
        wr_byte(item.captured_monster_speed);
    }

    if (any_bits(flags, SaveDataItemFlagType::FUEL)) {
        wr_s16b(item.fuel);
    }

    if (any_bits(flags, SaveDataItemFlagType::CAPTURED_MONSTER_CURRENT_HP)) {
        wr_s16b(item.captured_monster_current_hp);
    }

    if (any_bits(flags, SaveDataItemFlagType::XTRA5)) {
        wr_s16b(item.captured_monster_max_hp);
    }

    if (any_bits(flags, SaveDataItemFlagType::FEELING)) {
        wr_byte(item.feeling);
    }

    if (any_bits(flags, SaveDataItemFlagType::STACK_IDX)) {
        wr_s16b(item.stack_idx);
    }

    if (any_bits(flags, SaveDataItemFlagType::SMITH)) {
        if (item.smith_effect) {
            wr_s16b(enum2i(*item.smith_effect));
        } else {
            wr_s16b(0);
        }

        if (item.smith_act_idx) {
            wr_s16b(enum2i(*item.smith_act_idx));
        } else {
            wr_s16b(0);
        }

        wr_byte(item.smith_hit);
        wr_byte(item.smith_damage);
    }
}

/*!
 * @brief アイテム情報をセーブデータに書き込む
 * @param item アイテムへの参照
 */
void wr_item(const ItemEntity &item)
{
    const auto flags = write_item_flags(item);
    wr_s16b(item.bi_id);
    wr_byte((byte)item.iy);
    wr_byte((byte)item.ix);
    if (any_bits(flags, SaveDataItemFlagType::PVAL)) {
        wr_s16b(item.pval);
    }

    if (any_bits(flags, SaveDataItemFlagType::DISCOUNT)) {
        wr_byte(item.discount);
    }

    if (any_bits(flags, SaveDataItemFlagType::NUMBER)) {
        wr_byte((byte)item.number);
    }

    write_item_info(item, flags);
    if (any_bits(flags, SaveDataItemFlagType::INSCRIPTION)) {
        wr_string(*item.inscription);
    }

    if (any_bits(flags, SaveDataItemFlagType::ART_NAME)) {
        wr_string(*item.randart_name);
    }
}

/*!
 * @brief セーブデータにアイテムの鑑定情報を書き込む / Write an "perception" record
 * @param bi_id ベースアイテムのID
 */
void wr_perception(short bi_id)
{
    byte tmp8u = 0;
    const auto &baseitem = BaseitemList::get_instance().get_baseitem(bi_id);
    if (baseitem.aware) {
        tmp8u |= 0x01;
    }

    if (baseitem.tried) {
        tmp8u |= 0x02;
    }

    wr_byte(tmp8u);
}
