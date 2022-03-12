/*
 * @brief 軽鎧に耐性等の追加効果を付与する処理
 * @date 2022/03/12
 * @author Hourier
 */

#include "object-enchant/protector/apply-magic-soft-armor.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "sv-definition/sv-armor-types.h"

/*
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
SoftArmorEnchanter::SoftArmorEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power)
    : ArmorEnchanter{ player_ptr, o_ptr, level, power }
{
}

/*!
 * @brief power > 2 はデバッグ専用.
 */
void SoftArmorEnchanter::apply_magic()
{
    switch (this->o_ptr->sval) {
    case SV_KUROSHOUZOKU:
        this->o_ptr->pval = randint1(4);
        break;
    case SV_ABUNAI_MIZUGI:
        if (this->player_ptr->ppersonality != PERSONALITY_SEXY) {
            break;
        }

        this->o_ptr->pval = 3;
        this->o_ptr->art_flags.set(TR_STR);
        this->o_ptr->art_flags.set(TR_INT);
        this->o_ptr->art_flags.set(TR_WIS);
        this->o_ptr->art_flags.set(TR_DEX);
        this->o_ptr->art_flags.set(TR_CON);
        this->o_ptr->art_flags.set(TR_CHR);
        break;
    default:
        break;
    }

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
