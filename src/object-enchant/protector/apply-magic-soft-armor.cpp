/*
 * @brief 軽鎧に耐性等の追加効果を付与する処理
 * @date 2022/03/12
 * @author Hourier
 */

#include "object-enchant/protector/apply-magic-soft-armor.h"
#include "object/object-kind-hook.h"
#include "object/tval-types.h"
#include "sv-definition/sv-armor-types.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"

/*
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
SoftArmorEnchanter::SoftArmorEnchanter(PlayerType *player_ptr, ItemEntity *o_ptr, DEPTH level, int power)
    : ArmorEnchanter{ player_ptr, o_ptr, level, power }
{
}

/*!
 * @brief power > 2 はデバッグ専用.
 */
void SoftArmorEnchanter::apply_magic()
{
    this->sval_enchant();
    if (this->power > 1) {
        this->give_high_ego_index();
        if (this->is_high_ego_generated) {
            return;
        }

        this->give_ego_index();
        return;
    }

    if (this->power < -1) {
        this->give_cursed();
    }
}

void SoftArmorEnchanter::sval_enchant()
{
    const auto sval = this->o_ptr->bi_key.sval();
    if (!sval.has_value()) {
        return;
    }

    switch (sval.value()) {
    case SV_KUROSHOUZOKU:
        this->o_ptr->pval = randint1(4);
        return;
    case SV_ABUNAI_MIZUGI:
        if (this->player_ptr->ppersonality != PERSONALITY_SEXY) {
            return;
        }

        this->o_ptr->pval = 3;
        this->o_ptr->art_flags.set(TR_STR);
        this->o_ptr->art_flags.set(TR_INT);
        this->o_ptr->art_flags.set(TR_WIS);
        this->o_ptr->art_flags.set(TR_DEX);
        this->o_ptr->art_flags.set(TR_CON);
        this->o_ptr->art_flags.set(TR_CHR);
        return;
    default:
        return;
    }
}

/*
 * @brief ベースアイテムがローブの時、確率で永続か宵闇のローブを生成する.
 * @return 生成条件を満たしたらtrue、満たさなかったらfalse
 * @details 永続：12%、宵闇：3%
 */
void SoftArmorEnchanter::give_high_ego_index()
{
    const auto sval = this->o_ptr->bi_key.sval();
    if ((sval != SV_ROBE) || (randint0(100) >= 15)) {
        return;
    }

    this->is_high_ego_generated = true;
    auto ego_robe = one_in_(5);
    this->o_ptr->ego_idx = ego_robe ? EgoType::TWILIGHT : EgoType::PERMANENCE;
    if (!ego_robe) {
        return;
    }

    const BaseitemKey key(ItemKindType::SOFT_ARMOR, SV_TWILIGHT_ROBE);
    this->o_ptr->bi_id = lookup_baseitem_id(key);
    this->o_ptr->bi_key = key;
    this->o_ptr->ac = 0;
    this->o_ptr->to_a = 0;
    return;
}
