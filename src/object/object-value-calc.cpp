﻿#include "object/object-value-calc.h"
#include "artifact/artifact-info.h"
#include "object-enchant/activation-info-table.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object/object-flags.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief オブジェクトのフラグ類から価格を算出する /
 * Return the value of the flags the object has...
 * @param o_ptr フラグ価格を確認したいオブジェクトの構造体参照ポインタ
 * @param plusses フラグに与える価格の基本重み
 * @return オブジェクトのフラグ価格
 */
PRICE flag_cost(const ItemEntity *o_ptr, int plusses)
{
    PRICE total = 0;
    auto flgs = object_flags(o_ptr);
    const auto &baseitem = baseitems_info.at(o_ptr->bi_id);
    flgs.reset(baseitem.flags);

    if (o_ptr->is_fixed_artifact()) {
        const auto &a_ref = artifacts_info.at(o_ptr->fixed_artifact_idx);
        flgs.reset(a_ref.flags);
    } else if (o_ptr->is_ego()) {
        const auto &e_ref = egos_info[o_ptr->ego_idx];
        flgs.reset(e_ref.flags);
    }

    /*
     * Calucurate values of remaining flags
     */
    if (flgs.has(TR_STR)) {
        total += (1500 * plusses);
    }
    if (flgs.has(TR_INT)) {
        total += (1500 * plusses);
    }
    if (flgs.has(TR_WIS)) {
        total += (1500 * plusses);
    }
    if (flgs.has(TR_DEX)) {
        total += (1500 * plusses);
    }
    if (flgs.has(TR_CON)) {
        total += (1500 * plusses);
    }
    if (flgs.has(TR_CHR)) {
        total += (750 * plusses);
    }
    if (flgs.has(TR_MAGIC_MASTERY)) {
        total += (600 * plusses);
    }
    if (flgs.has(TR_STEALTH)) {
        total += (250 * plusses);
    }
    if (flgs.has(TR_SEARCH)) {
        total += (100 * plusses);
    }
    if (flgs.has(TR_INFRA)) {
        total += (150 * plusses);
    }
    if (flgs.has(TR_TUNNEL)) {
        total += (175 * plusses);
    }
    if ((flgs.has(TR_SPEED)) && (plusses > 0)) {
        total += (10000 + (2500 * plusses));
    }
    if ((flgs.has(TR_BLOWS)) && (plusses > 0)) {
        total += (10000 + (2500 * plusses));
    }

    PRICE tmp_cost = 0;
    int count = 0;
    if (flgs.has(TR_CHAOTIC)) {
        total += 5000;
        count++;
    }
    if (flgs.has(TR_BRAND_MAGIC)) {
        total += 1000;
        count++;
    }
    if (flgs.has(TR_VAMPIRIC)) {
        total += 6500;
        count++;
    }
    if (flgs.has(TR_FORCE_WEAPON)) {
        tmp_cost += 2500;
        count++;
    }
    if (flgs.has(TR_KILL_ANIMAL)) {
        tmp_cost += 2800;
        count++;
    } else if (flgs.has(TR_SLAY_ANIMAL)) {
        tmp_cost += 1800;
        count++;
    }
    if (flgs.has(TR_KILL_EVIL)) {
        tmp_cost += 3300;
        count++;
    } else if (flgs.has(TR_SLAY_EVIL)) {
        tmp_cost += 2300;
        count++;
    }
    if (flgs.has(TR_KILL_GOOD)) {
        tmp_cost += 2800;
        count++;
    } else if (flgs.has(TR_SLAY_GOOD)) {
        tmp_cost += 1800;
        count++;
    }
    if (flgs.has(TR_KILL_HUMAN)) {
        tmp_cost += 2800;
        count++;
    } else if (flgs.has(TR_SLAY_HUMAN)) {
        tmp_cost += 1800;
        count++;
    }
    if (flgs.has(TR_KILL_UNDEAD)) {
        tmp_cost += 2800;
        count++;
    } else if (flgs.has(TR_SLAY_UNDEAD)) {
        tmp_cost += 1800;
        count++;
    }
    if (flgs.has(TR_KILL_DEMON)) {
        tmp_cost += 2800;
        count++;
    } else if (flgs.has(TR_SLAY_DEMON)) {
        tmp_cost += 1800;
        count++;
    }
    if (flgs.has(TR_KILL_ORC)) {
        tmp_cost += 2500;
        count++;
    } else if (flgs.has(TR_SLAY_ORC)) {
        tmp_cost += 1500;
        count++;
    }
    if (flgs.has(TR_KILL_TROLL)) {
        tmp_cost += 2800;
        count++;
    } else if (flgs.has(TR_SLAY_TROLL)) {
        tmp_cost += 1800;
        count++;
    }
    if (flgs.has(TR_KILL_GIANT)) {
        tmp_cost += 2800;
        count++;
    } else if (flgs.has(TR_SLAY_GIANT)) {
        tmp_cost += 1800;
        count++;
    }
    if (flgs.has(TR_KILL_DRAGON)) {
        tmp_cost += 2800;
        count++;
    } else if (flgs.has(TR_SLAY_DRAGON)) {
        tmp_cost += 1800;
        count++;
    }

    if (flgs.has(TR_VORPAL)) {
        tmp_cost += 2500;
        count++;
    }
    if (flgs.has(TR_IMPACT)) {
        tmp_cost += 2500;
        count++;
    }
    if (flgs.has(TR_EARTHQUAKE)) {
        tmp_cost += 2500;
        count++;
    }
    if (flgs.has(TR_BRAND_POIS)) {
        tmp_cost += 3800;
        count++;
    }
    if (flgs.has(TR_BRAND_ACID)) {
        tmp_cost += 3800;
        count++;
    }
    if (flgs.has(TR_BRAND_ELEC)) {
        tmp_cost += 3800;
        count++;
    }
    if (flgs.has(TR_BRAND_FIRE)) {
        tmp_cost += 2500;
        count++;
    }
    if (flgs.has(TR_BRAND_COLD)) {
        tmp_cost += 2500;
        count++;
    }
    total += (tmp_cost * count);

    if (flgs.has(TR_SUST_STR)) {
        total += 850;
    }
    if (flgs.has(TR_SUST_INT)) {
        total += 850;
    }
    if (flgs.has(TR_SUST_WIS)) {
        total += 850;
    }
    if (flgs.has(TR_SUST_DEX)) {
        total += 850;
    }
    if (flgs.has(TR_SUST_CON)) {
        total += 850;
    }
    if (flgs.has(TR_SUST_CHR)) {
        total += 250;
    }
    if (flgs.has(TR_RIDING)) {
        total += 0;
    }
    if (flgs.has(TR_EASY_SPELL)) {
        total += 1500;
    }
    if (flgs.has(TR_THROW)) {
        total += 5000;
    }
    if (flgs.has(TR_FREE_ACT)) {
        total += 4500;
    }
    if (flgs.has(TR_HOLD_EXP)) {
        total += 8500;
    }

    tmp_cost = 0;
    count = 0;
    if (flgs.has(TR_IM_ACID)) {
        tmp_cost += 15000;
        count += 2;
    }
    if (flgs.has(TR_IM_ELEC)) {
        tmp_cost += 15000;
        count += 2;
    }
    if (flgs.has(TR_IM_FIRE)) {
        tmp_cost += 15000;
        count += 2;
    }
    if (flgs.has(TR_IM_COLD)) {
        tmp_cost += 15000;
        count += 2;
    }
    if (flgs.has(TR_REFLECT)) {
        tmp_cost += 5000;
        count += 2;
    }
    if (flgs.has(TR_RES_ACID)) {
        tmp_cost += 500;
        count++;
    }
    if (flgs.has(TR_RES_ELEC)) {
        tmp_cost += 500;
        count++;
    }
    if (flgs.has(TR_RES_FIRE)) {
        tmp_cost += 500;
        count++;
    }
    if (flgs.has(TR_RES_COLD)) {
        tmp_cost += 500;
        count++;
    }
    if (flgs.has(TR_RES_POIS)) {
        tmp_cost += 1000;
        count += 2;
    }
    if (flgs.has(TR_RES_FEAR)) {
        tmp_cost += 1000;
        count += 2;
    }
    if (flgs.has(TR_RES_LITE)) {
        tmp_cost += 800;
        count += 2;
    }
    if (flgs.has(TR_RES_DARK)) {
        tmp_cost += 800;
        count += 2;
    }
    if (flgs.has(TR_RES_BLIND)) {
        tmp_cost += 900;
        count += 2;
    }
    if (flgs.has(TR_RES_CONF)) {
        tmp_cost += 900;
        count += 2;
    }
    if (flgs.has(TR_RES_SOUND)) {
        tmp_cost += 900;
        count += 2;
    }
    if (flgs.has(TR_RES_SHARDS)) {
        tmp_cost += 900;
        count += 2;
    }
    if (flgs.has(TR_RES_NETHER)) {
        tmp_cost += 900;
        count += 2;
    }
    if (flgs.has(TR_RES_NEXUS)) {
        tmp_cost += 900;
        count += 2;
    }
    if (flgs.has(TR_RES_CHAOS)) {
        tmp_cost += 1000;
        count += 2;
    }
    if (flgs.has(TR_RES_DISEN)) {
        tmp_cost += 2000;
        count += 2;
    }
    if (flgs.has(TR_RES_TIME)) {
        tmp_cost += 2000;
        count += 2;
    }
    if (flgs.has(TR_RES_WATER)) {
        tmp_cost += 2000;
        count += 2;
    }
    total += (tmp_cost * count);

    if (flgs.has(TR_RES_CURSE)) {
        total += 7500;
    }
    if (flgs.has(TR_SH_FIRE)) {
        total += 5000;
    }
    if (flgs.has(TR_SH_ELEC)) {
        total += 5000;
    }
    if (flgs.has(TR_SH_COLD)) {
        total += 5000;
    }
    if (flgs.has(TR_NO_TELE)) {
        total -= 10000;
    }
    if (flgs.has(TR_NO_MAGIC)) {
        total += 2500;
    }
    if (flgs.has(TR_TY_CURSE)) {
        total -= 15000;
    }
    if (flgs.has(TR_HIDE_TYPE)) {
        total += 0;
    }
    if (flgs.has(TR_SHOW_MODS)) {
        total += 0;
    }
    if (flgs.has(TR_LEVITATION)) {
        total += 1250;
    }
    if (flgs.has(TR_LITE_1)) {
        total += 1500;
    }
    if (flgs.has(TR_LITE_2)) {
        total += 2500;
    }
    if (flgs.has(TR_LITE_3)) {
        total += 4000;
    }
    if (flgs.has(TR_LITE_M1)) {
        total -= 1500;
    }
    if (flgs.has(TR_LITE_M2)) {
        total -= 2500;
    }
    if (flgs.has(TR_LITE_M3)) {
        total -= 4000;
    }
    if (flgs.has(TR_SEE_INVIS)) {
        total += 2000;
    }
    if (flgs.has(TR_TELEPATHY)) {
        total += 20000;
    }
    if (flgs.has(TR_ESP_ANIMAL)) {
        total += 1000;
    }
    if (flgs.has(TR_ESP_UNDEAD)) {
        total += 1000;
    }
    if (flgs.has(TR_ESP_DEMON)) {
        total += 1000;
    }
    if (flgs.has(TR_ESP_ORC)) {
        total += 1000;
    }
    if (flgs.has(TR_ESP_TROLL)) {
        total += 1000;
    }
    if (flgs.has(TR_ESP_GIANT)) {
        total += 1000;
    }
    if (flgs.has(TR_ESP_DRAGON)) {
        total += 1000;
    }
    if (flgs.has(TR_ESP_HUMAN)) {
        total += 1000;
    }
    if (flgs.has(TR_ESP_EVIL)) {
        total += 15000;
    }
    if (flgs.has(TR_ESP_GOOD)) {
        total += 2000;
    }
    if (flgs.has(TR_ESP_NONLIVING)) {
        total += 2000;
    }
    if (flgs.has(TR_ESP_UNIQUE)) {
        total += 10000;
    }
    if (flgs.has(TR_SLOW_DIGEST)) {
        total += 750;
    }
    if (flgs.has(TR_REGEN)) {
        total += 2500;
    }
    if (flgs.has(TR_WARNING)) {
        total += 2000;
    }
    if (flgs.has(TR_DEC_MANA)) {
        total += 10000;
    }
    if (flgs.has(TR_XTRA_MIGHT)) {
        total += 2250;
    }
    if (flgs.has(TR_XTRA_SHOTS)) {
        total += 10000;
    }
    if (flgs.has(TR_IGNORE_ACID)) {
        total += 100;
    }
    if (flgs.has(TR_IGNORE_ELEC)) {
        total += 100;
    }
    if (flgs.has(TR_IGNORE_FIRE)) {
        total += 100;
    }
    if (flgs.has(TR_IGNORE_COLD)) {
        total += 100;
    }
    if (flgs.has(TR_ACTIVATE)) {
        total += 100;
    }
    if (flgs.has(TR_DRAIN_EXP)) {
        total -= 12500;
    }
    if (flgs.has(TR_DRAIN_HP)) {
        total -= 12500;
    }
    if (flgs.has(TR_DRAIN_MANA)) {
        total -= 12500;
    }
    if (flgs.has(TR_CALL_ANIMAL)) {
        total -= 12500;
    }
    if (flgs.has(TR_CALL_DEMON)) {
        total -= 10000;
    }
    if (flgs.has(TR_CALL_DRAGON)) {
        total -= 10000;
    }
    if (flgs.has(TR_CALL_UNDEAD)) {
        total -= 10000;
    }
    if (flgs.has(TR_COWARDICE)) {
        total -= 5000;
    }
    if (flgs.has(TR_LOW_MELEE)) {
        total -= 5000;
    }
    if (flgs.has(TR_LOW_AC)) {
        total -= 5000;
    }
    if (flgs.has(TR_HARD_SPELL)) {
        total -= 15000;
    }
    if (flgs.has(TR_FAST_DIGEST)) {
        total -= 10000;
    }
    if (flgs.has(TR_SLOW_REGEN)) {
        total -= 10000;
    }
    if (flgs.has(TR_TELEPORT)) {
        if (o_ptr->is_cursed()) {
            total -= 7500;
        } else {
            total += 250;
        }
    }
    if (flgs.has(TR_VUL_CURSE)) {
        total -= 7500;
    }

    if (flgs.has(TR_AGGRAVATE)) {
        total -= 10000;
    }
    if (flgs.has(TR_BLESSED)) {
        total += 750;
    }
    if (o_ptr->curse_flags.has(CurseTraitType::ADD_L_CURSE)) {
        total -= 5000;
    }
    if (o_ptr->curse_flags.has(CurseTraitType::ADD_H_CURSE)) {
        total -= 12500;
    }
    if (o_ptr->curse_flags.has(CurseTraitType::CURSED)) {
        total -= 5000;
    }
    if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
        total -= 12500;
    }
    if (o_ptr->curse_flags.has(CurseTraitType::PERSISTENT_CURSE)) {
        total -= 12500;
    }
    if (o_ptr->curse_flags.has(CurseTraitType::PERMA_CURSE)) {
        total -= 15000;
    }

    /* Also, give some extra for activatable powers... */
    if (o_ptr->art_name && (o_ptr->art_flags.has(TR_ACTIVATE))) {
        auto act_ptr = find_activation_info(o_ptr);
        if (act_ptr.has_value()) {
            total += act_ptr.value()->value;
        }
    }

    return total;
}
