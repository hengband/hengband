/*
 * @brief 兜に耐性等の追加効果を付与する処理
 * @date 2021/08/01
 * @author Hourier
 * @details ドラゴンヘルムは必ず付与する. それ以外は確率的に付与する.
 */

#include "object-enchant/protector/apply-magic-helm.h"
#include "artifact/random-art-generator.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-ego.h"
#include "sv-definition/sv-protector-types.h"
#include "system/object-type-definition.h"

/*
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
HelmEnchanter::HelmEnchanter(PlayerType *player_ptr, object_type *o_ptr, DEPTH level, int power)
    : AbstractProtectorEnchanter{ o_ptr, level, power }
    , player_ptr(player_ptr)
{
}

void HelmEnchanter::apply_magic()
{
    if (this->o_ptr->sval == SV_DRAGON_HELM) {
        dragon_resist(this->o_ptr);
        if (!one_in_(3)) {
            return;
        }
    }

    if (this->power > 1) {
        this->give_ego_index();
        return;
    }

    if (this->power < -1) {
        this->give_cursed();
    }
}

/*
 * @details power > 2はデバッグ専用.
 */
void HelmEnchanter::give_ego_index()
{
    if ((this->power > 2) || one_in_(20)) {
        become_random_artifact(this->player_ptr, this->o_ptr, false);
        return;
    }

    while (true) {
        this->o_ptr->name2 = get_random_ego(INVEN_HEAD, true);
        switch (this->o_ptr->name2) {
        case EGO_BRILLIANCE:
        case EGO_DARK:
        case EGO_INFRAVISION:
        case EGO_H_PROTECTION:
        case EGO_LITE:
            return;
        case EGO_SEEING:
            if (one_in_(7)) {
                add_low_telepathy(this->o_ptr);
            }

            return;
        default:
            continue;
        }
    }
}

void HelmEnchanter::give_cursed()
{
    while (true) {
        this->o_ptr->name2 = get_random_ego(INVEN_HEAD, false);
        switch (this->o_ptr->name2) {
        case EGO_ANCIENT_CURSE:
            return;
        default:
            continue;
        }
    }
}
