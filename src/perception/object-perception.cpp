#include "perception/object-perception.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "game-option/play-record-options.h"
#include "io/write-diary.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"

/*!
 * @brief オブジェクトを＊鑑定＊済にする /
 * The player is now aware of the effects of the given object.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr ＊鑑定＊済にするオブジェクトの構造体参照ポインタ
 */
void object_aware(PlayerType *player_ptr, const ItemEntity *o_ptr)
{
    const bool is_already_awared = o_ptr->is_aware();
    auto &baseitem = o_ptr->get_baseitem();
    baseitem.aware = true;

    // 以下、playrecordに記録しない場合はreturnする
    if (!record_ident) {
        return;
    }

    if (is_already_awared || player_ptr->is_dead) {
        return;
    }

    // アーティファクト専用ベースアイテムは記録しない
    if (baseitem.gen_flags.has(ItemGenerationTraitType::INSTA_ART)) {
        return;
    }

    if (!o_ptr->has_unidentified_name()) {
        return;
    }

    // playrecordに識別したアイテムを記録
    ItemEntity forge;
    ItemEntity *q_ptr;
    q_ptr = &forge;
    q_ptr->copy_from(o_ptr);
    q_ptr->number = 1;
    const auto item_name = describe_flavor(player_ptr, q_ptr, OD_NAME_ONLY);
    exe_write_diary(player_ptr, DiaryKind::FOUND, 0, item_name);
}
