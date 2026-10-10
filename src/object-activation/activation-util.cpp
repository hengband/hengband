#include "object-activation/activation-util.h"
#include "object-enchant/activation-info-table.h"
#include "object/object-info.h"
#include "system/artifact/artifact-definition.h"
#include "system/gamevalue.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include <algorithm>

ae_type::ae_type(PlayerType *player_ptr, short i_idx)
{
    this->item = ref_item(player_ptr, i_idx);
    if (this->item) {
        this->lev = this->item->get_baseitem_level();
    }
}

void ae_type::decide_activation_level()
{
    if (this->item->is_fixed_artifact()) {
        this->lev = this->item->get_fixed_artifact().level;
        return;
    }

    if (this->item->is_random_artifact()) {
        const auto it_activation = this->item->find_activation_info();
        if (it_activation != activation_info.end()) {
            this->lev = it_activation->level;
        }

        return;
    }

    const auto tval = this->item->bi_key.tval();
    if (((tval == ItemKindType::RING) || (tval == ItemKindType::AMULET)) && this->item->is_ego()) {
        this->lev = this->item->get_ego().level;
    }
}

/*!
 * @brief 発動の基本難易度と技能から抽選用の成功・失敗重みを求める
 * @param skill 混乱等の補正後の魔道具技能（符号付き16bit）
 * @details levは各定義readerで0～128に制限される。この範囲と技能型なら重みと抽選時の2倍もintに収まる。
 */
void ae_type::decide_chance_fail(ACTION_SKILL_POWER skill)
{
    auto success_weight = int{ skill };
    auto failure_weight = this->lev + 5;
    if (success_weight > failure_weight) {
        failure_weight -= (success_weight - failure_weight) * 2;
    } else {
        success_weight -= (failure_weight - success_weight) * 2;
    }
    this->chance = std::max(success_weight, USE_DEVICE);
    this->fail = std::max(failure_weight, USE_DEVICE);
}
