/*!
 * @file blue-magic-ball-bolt.cpp
 * @brief 青魔法のボール/ボルト系呪文定義
 */

#include "blue-magic/blue-magic-ball-bolt.h"
#include "blue-magic/blue-magic-util.h"
#include "effect/attribute-types.h"
#include "monster-race/race-ability-flags.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-projection-table.h"
#include "spell-kind/spells-launcher.h"
#include "system/angband-exceptions.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"
#include <fmt/format.h>

bool cast_blue_magic_ball(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    const auto dir = get_aim_dir(player_ptr);
    if (!dir) {
        return false;
    }

    const auto magic = find_mspell_projection(bmc_ptr->spell);
    if (!magic || (magic->type != MspellProjectionType::BALL)) {
        const auto message = fmt::format("Unknown blue magic ball: {}", static_cast<int>(bmc_ptr->spell));
        THROW_EXCEPTION(std::logic_error, message);
    }

    msg_print(magic->message);
    const auto damage = monspell_bluemage_damage(player_ptr, bmc_ptr->spell, bmc_ptr->plev, DAM_ROLL);
    fire_ball(player_ptr, magic->attribute, dir, damage, magic->radius);
    return true;
};

bool cast_blue_magic_bolt(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    const auto dir = get_aim_dir(player_ptr);
    if (!dir) {
        return false;
    }

    const auto magic = find_mspell_projection(bmc_ptr->spell);
    if (!magic || (magic->type != MspellProjectionType::BOLT)) {
        const auto message = fmt::format("Unknown blue magic bolt: {}", static_cast<int>(bmc_ptr->spell));
        THROW_EXCEPTION(std::logic_error, message);
    }

    msg_print(magic->message);
    const auto damage = monspell_bluemage_damage(player_ptr, bmc_ptr->spell, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(player_ptr, magic->attribute, dir, damage);
    return true;
};
