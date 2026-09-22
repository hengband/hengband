/*!
 * @file blue-magic-breath.cpp
 * @brief 青魔法のブレス系呪文定義
 */

#include "blue-magic/blue-magic-breath.h"
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

bool cast_blue_magic_breath(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    const auto dir = get_aim_dir(player_ptr);
    if (!dir) {
        return false;
    }

    const auto magic = find_mspell_projection(bmc_ptr->spell);
    if (!magic || (magic->type != MspellProjectionType::BREATH)) {
        const auto message = fmt::format("Unknown blue magic breath: {}", static_cast<int>(bmc_ptr->spell));
        THROW_EXCEPTION(std::logic_error, message);
    }

    msg_print(magic->message);
    const auto radius = (bmc_ptr->plev > 40 ? 3 : 2);
    const auto damage = monspell_bluemage_damage(player_ptr, bmc_ptr->spell, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, magic->attribute, dir, damage, radius);
    return true;
}
