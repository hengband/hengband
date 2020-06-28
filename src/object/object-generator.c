#include "object/object-generator.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object/object-kind.h"

/*!
 * @brief オブジェクトを初期化する
 * Wipe an object clean.
 * @param o_ptr 初期化したいオブジェクトの構造体参照ポインタ
 * @return なし
 */
void object_wipe(object_type *o_ptr) { (void)WIPE(o_ptr, object_type); }

/*!
 * @brief オブジェクトを複製する
 * Wipe an object clean.
 * @param o_ptr 複製元のオブジェクトの構造体参照ポインタ
 * @param j_ptr 複製先のオブジェクトの構造体参照ポインタ
 * @return なし
 */
void object_copy(object_type *o_ptr, object_type *j_ptr) { (void)COPY(o_ptr, j_ptr, object_type); }

/*!
 * @brief オブジェクト構造体にベースアイテムを作成する
 * Prepare an object based on an object kind.
 * @param o_ptr 代入したいオブジェクトの構造体参照ポインタ
 * @param k_idx 新たに作成したいベースアイテム情報のID
 * @return なし
 */
void object_prep(player_type *player_ptr, object_type *o_ptr, KIND_OBJECT_IDX k_idx)
{
    object_kind *k_ptr = &k_info[k_idx];
    object_wipe(o_ptr);
    o_ptr->k_idx = k_idx;
    o_ptr->tval = k_ptr->tval;
    o_ptr->sval = k_ptr->sval;
    o_ptr->pval = k_ptr->pval;
    o_ptr->number = 1;
    o_ptr->weight = k_ptr->weight;
    o_ptr->to_h = k_ptr->to_h;
    o_ptr->to_d = k_ptr->to_d;
    o_ptr->to_a = k_ptr->to_a;
    o_ptr->ac = k_ptr->ac;
    o_ptr->dd = k_ptr->dd;
    o_ptr->ds = k_ptr->ds;

    if (k_ptr->act_idx > 0)
        o_ptr->xtra2 = (XTRA8)k_ptr->act_idx;
    if (k_info[o_ptr->k_idx].cost <= 0)
        o_ptr->ident |= (IDENT_BROKEN);

    if (k_ptr->gen_flags & (TRG_CURSED))
        o_ptr->curse_flags |= (TRC_CURSED);
    if (k_ptr->gen_flags & (TRG_HEAVY_CURSE))
        o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
    if (k_ptr->gen_flags & (TRG_PERMA_CURSE))
        o_ptr->curse_flags |= (TRC_PERMA_CURSE);
    if (k_ptr->gen_flags & (TRG_RANDOM_CURSE0))
        o_ptr->curse_flags |= get_curse(player_ptr, 0, o_ptr);
    if (k_ptr->gen_flags & (TRG_RANDOM_CURSE1))
        o_ptr->curse_flags |= get_curse(player_ptr, 1, o_ptr);
    if (k_ptr->gen_flags & (TRG_RANDOM_CURSE2))
        o_ptr->curse_flags |= get_curse(player_ptr, 2, o_ptr);
}
