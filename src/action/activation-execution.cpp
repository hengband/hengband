/*!
 * @file activation-execution.cpp
 * @brief アイテムの発動実行定義
 */

#include "action/activation-execution.h"
#include "action/action-limited.h"
#include "artifact/random-art-effects.h"
#include "core/window-redrawer.h"
#include "effect/attribute-types.h"
#include "effect/spells-effect-util.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/geometry.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-info.h"
#include "monster/monster-util.h"
#include "object-activation/activation-switcher.h"
#include "object-activation/activation-util.h"
#include "object-enchant/activation-info-table.h"
#include "object-enchant/object-ego.h"
#include "object/object-info.h"
#include "player-base/player-class.h"
#include "player-status/player-energy.h"
#include "racial/racial-android.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-ring-types.h"
#include "system/baseitem/baseitem-key.h"
#include "system/floor/floor-info.h"
#include "system/item/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"
#include "world/world.h"

static void decide_chance_fail(PlayerType *player_ptr, ae_type *ae_ptr)
{
    ae_ptr->chance = player_ptr->skill_dev;
    if (player_ptr->effects()->confusion().is_confused()) {
        ae_ptr->chance = ae_ptr->chance / 2;
    }

    ae_ptr->fail = ae_ptr->lev + 5;
    if (ae_ptr->chance > ae_ptr->fail) {
        ae_ptr->fail -= (ae_ptr->chance - ae_ptr->fail) * 2;
    } else {
        ae_ptr->chance -= (ae_ptr->fail - ae_ptr->chance) * 2;
    }

    if (ae_ptr->fail < USE_DEVICE) {
        ae_ptr->fail = USE_DEVICE;
    }

    if (ae_ptr->chance < USE_DEVICE) {
        ae_ptr->chance = USE_DEVICE;
    }
}

static void decide_activation_success(PlayerType *player_ptr, ae_type *ae_ptr)
{
    if (PlayerClass(player_ptr).equals(PlayerClassType::BERSERKER)) {
        ae_ptr->success = false;
        return;
    }

    if (ae_ptr->chance > ae_ptr->fail) {
        ae_ptr->success = randint0(ae_ptr->chance * 2) >= ae_ptr->fail;
        return;
    }

    ae_ptr->success = randint0(ae_ptr->fail * 2) < ae_ptr->chance;
}

static bool check_activation_success(ae_type *ae_ptr)
{
    if (ae_ptr->success) {
        return true;
    }

    if (flush_failure) {
        flush();
    }

    msg_print(_("うまく始動させることができなかった。", "You failed to activate it properly."));
    sound(SoundKind::FAIL);
    return false;
}

static bool check_activation_conditions(PlayerType *player_ptr, ae_type *ae_ptr)
{
    if (!check_activation_success(ae_ptr)) {
        return false;
    }

    if (ae_ptr->item->timeout) {
        msg_print(_("それは微かに音を立て、輝き、消えた...", "It whines, glows and fades..."));
        return false;
    }

    if (ae_ptr->item->is_fuel() && (ae_ptr->item->fuel == 0)) {
        msg_print(_("燃料がない。", "It has no fuel."));
        PlayerEnergy(player_ptr).reset_player_turn();
        return false;
    }

    return true;
}

/*!
 * @brief アイテムの発動効果を処理する。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param item 対象アイテムへの参照
 */
static void activate_artifact(PlayerType *player_ptr, std::shared_ptr<ItemEntity> &item)
{
    const auto it_activation = item->find_activation_info();
    if (it_activation == activation_info.end()) {
        msg_print("Activation information is not found.");
        return;
    }

    const auto item_name = describe_flavor(player_ptr, *item, OD_NAME_ONLY | OD_OMIT_PREFIX | OD_BASE_NAME);
    const auto &[has_activated, item_activated] = switch_activation(player_ptr, *item, it_activation->index, item_name);
    if (!has_activated) {
        return;
    }

    if (item_activated) {
        item = item_activated;
    }

    if (it_activation->constant) {
        item->timeout = static_cast<short>(*it_activation->constant);
        if (it_activation->dice > 0) {
            item->timeout += static_cast<short>(randint1(it_activation->dice));
        }

        return;
    }

    switch (it_activation->index) {
    case RandomArtActType::BR_FIRE:
        item->timeout = item->bi_key == BaseitemKey(ItemKindType::RING, SV_RING_FLAMES) ? 200 : 250;
        return;
    case RandomArtActType::BR_COLD:
        item->timeout = item->bi_key == BaseitemKey(ItemKindType::RING, SV_RING_ICE) ? 200 : 250;
        return;
    case RandomArtActType::TERROR:
        item->timeout = 3 * (player_ptr->lev + 10);
        return;
    case RandomArtActType::MURAMASA:
        return;
    default:
        msg_format("Special timeout is not implemented: %d.", enum2i(it_activation->index));
        return;
    }
}

/*!
 * @brief 装備を発動するコマンドのサブルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param i_idx 発動するオブジェクトの所持品ID
 */
void exe_activate(PlayerType *player_ptr, INVENTORY_IDX i_idx)
{
    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    ae_type ae(player_ptr, i_idx);
    ae.decide_activation_level();
    decide_chance_fail(player_ptr, &ae);
    if (cmd_limit_time_walk(player_ptr)) {
        return;
    }

    decide_activation_success(player_ptr, &ae);
    if (!check_activation_conditions(player_ptr, &ae)) {
        return;
    }

    msg_print(_("始動させた...", "You activate it..."));
    sound(SoundKind::ZAP);
    if (ae.item->has_activation()) {
        activate_artifact(player_ptr, ae.item);
        static constexpr auto flags = {
            SubWindowRedrawingFlag::INVENTORY,
            SubWindowRedrawingFlag::EQUIPMENT,
        };
        RedrawingFlagsUpdater::get_instance().set_flags(flags);
        return;
    }

    msg_print(_("おっと、このアイテムは始動できない。", "Oops.  That object cannot be activated."));
}
