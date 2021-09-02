/*
 * @file object-type-definition.h
 * @brief アイテム定義の構造体とエンティティ処理実装
 * @author Hourier
 * @date 2021/05/02
 */

#include "system/object-type-definition.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object/object-kind.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/player-type-definition.h"

/*!
 * @brief オブジェクトを初期化する
 * Wipe an object clean.
 */
void object_type::wipe()
{
    (void)WIPE(this, object_type);
}

/*!
 * @brief オブジェクトを複製する
 * Wipe an object clean.
 * @param j_ptr 複製元のオブジェクトの構造体参照ポインタ
 */
void object_type::copy_from(object_type *j_ptr)
{
    (void)COPY(this, j_ptr, object_type);
}

/*!
 * @brief オブジェクト構造体にベースアイテムを作成する
 * Prepare an object based on an object kind.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param k_idx 新たに作成したいベースアイテム情報のID
 */
void object_type::prep(KIND_OBJECT_IDX ko_idx)
{
    object_kind *k_ptr = &k_info[ko_idx];
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

    if (k_ptr->act_idx > 0)
        this->xtra2 = (XTRA8)k_ptr->act_idx;
    if (k_info[this->k_idx].cost <= 0)
        this->ident |= (IDENT_BROKEN);

    if (k_ptr->gen_flags.has(TRG::CURSED))
        this->curse_flags.set(TRC::CURSED);
    if (k_ptr->gen_flags.has(TRG::HEAVY_CURSE))
        this->curse_flags.set(TRC::HEAVY_CURSE);
    if (k_ptr->gen_flags.has(TRG::PERMA_CURSE))
        this->curse_flags.set(TRC::PERMA_CURSE);
    if (k_ptr->gen_flags.has(TRG::RANDOM_CURSE0))
        this->curse_flags.set(get_curse(0, this));
    if (k_ptr->gen_flags.has(TRG::RANDOM_CURSE1))
        this->curse_flags.set(get_curse(1, this));
    if (k_ptr->gen_flags.has(TRG::RANDOM_CURSE2))
        this->curse_flags.set(get_curse(2, this));
}

bool object_type::is_lance() const
{
    auto is_lance = this->tval == TV_POLEARM;
    is_lance &= (this->sval == SV_LANCE) || (this->sval == SV_HEAVY_LANCE);
    return is_lance;
}
