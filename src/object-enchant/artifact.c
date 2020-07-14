/*!
 * @brief アーティファクトの生成と管理 / Artifact code
 * @date 2013/12/11
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2013 Deskull rearranged comment for Doxygen.
 */

#include "object-enchant/artifact.h"
#include "artifact/random-art-activation.h"
#include "artifact/random-art-bias-types.h"
#include "artifact/random-art-characteristics.h"
#include "artifact/random-art-pval-investor.h"
#include "art-definition/art-armor-types.h"
#include "art-definition/art-protector-types.h"
#include "art-definition/art-sword-types.h"
#include "art-definition/art-weapon-types.h"
#include "art-definition/random-art-effects.h"
#include "cmd-item/cmd-smith.h"
#include "core/asking-player.h"
#include "core/window-redrawer.h"
#include "flavor/object-flavor.h"
#include "floor/floor-object.h"
#include "floor/floor.h"
#include "game-option/cheat-types.h"
#include "grid/grid.h"
#include "io/files-util.h"
#include "mind/mind-weaponsmith.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object-hook/hook-weapon.h"
#include "object/object-flags.h"
#include "object/object-generator.h"
#include "object/object-kind-hook.h"
#include "object/object-kind.h"
#include "object/object-value-calc.h"
#include "perception/identification.h"
#include "perception/object-perception.h"
#include "player/avatar.h"
#include "player/player-class.h"
#include "player/player-personalities-types.h"
#include "spell/spells-object.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/system-variables.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "view/display-messages.h"
#include "wizard/artifact-bias-table.h"
#include "wizard/wizard-messages.h"
#include "world/world.h"

/*
 * The artifact arrays
 */
artifact_type *a_info;
char *a_name;
char *a_text;

/*
 * Maximum number of artifacts in a_info.txt
 */
ARTIFACT_IDX max_a_idx;

/*!
 * @brief オブジェクトから能力発動IDを取得する。
 * @details いくつかのケースで定義されている発動効果から、
 * 鍛冶師による付与＞固定アーティファクト＞エゴ＞ランダムアーティファクト＞ベースアイテムの優先順位で走査していく。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動効果のIDを返す
 */
int activation_index(player_type *player_ptr, object_type *o_ptr)
{
    if (object_is_smith(player_ptr, o_ptr)) {
        switch (o_ptr->xtra3 - 1) {
        case ESSENCE_TMP_RES_ACID:
            return ACT_RESIST_ACID;
        case ESSENCE_TMP_RES_ELEC:
            return ACT_RESIST_ELEC;
        case ESSENCE_TMP_RES_FIRE:
            return ACT_RESIST_FIRE;
        case ESSENCE_TMP_RES_COLD:
            return ACT_RESIST_COLD;
        case TR_IMPACT:
            return ACT_QUAKE;
        }
    }

    if (object_is_fixed_artifact(o_ptr)) {
        if (have_flag(a_info[o_ptr->name1].flags, TR_ACTIVATE)) {
            return a_info[o_ptr->name1].act_idx;
        }
    }

    if (object_is_ego(o_ptr)) {
        if (have_flag(e_info[o_ptr->name2].flags, TR_ACTIVATE)) {
            return e_info[o_ptr->name2].act_idx;
        }
    }

    if (!object_is_random_artifact(o_ptr)) {
        if (have_flag(k_info[o_ptr->k_idx].flags, TR_ACTIVATE)) {
            return k_info[o_ptr->k_idx].act_idx;
        }
    }

    return o_ptr->xtra2;
}

/*!
 * @brief オブジェクトから発動効果構造体のポインタを取得する。
 * @details activation_index() 関数の結果から参照する。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動効果構造体のポインタを返す
 */
const activation_type *find_activation_info(player_type *player_ptr, object_type *o_ptr)
{
    const int index = activation_index(player_ptr, o_ptr);
    const activation_type *p;
    for (p = activation_info; p->flag != NULL; ++p) {
        if (p->index == index) {
            return p;
        }
    }

    return NULL;
}
