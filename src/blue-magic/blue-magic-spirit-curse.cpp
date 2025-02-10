/*!
 * @file blue-magic-spirit-curse.cpp
 * @brief 青魔法の呪い系処理定義
 */

#include "blue-magic/blue-magic-spirit-curse.h"
#include "blue-magic/blue-magic-util.h"
#include "effect/attribute-types.h"
#include "monster-race/race-ability-flags.h"
#include "mspell/mspell-damage-calculator.h"
#include "spell-kind/spells-launcher.h"
#include "system/angband-exceptions.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"
#include <fmt/format.h>
#include <unordered_map>

namespace {
const std::unordered_map<MonsterAbilityType, AttributeType> BLUE_MAGIC_SPIRIT_CURSE_TABLE = {
    { MonsterAbilityType::DRAIN_MANA, AttributeType::DRAIN_MANA },
    { MonsterAbilityType::MIND_BLAST, AttributeType::MIND_BLAST },
    { MonsterAbilityType::BRAIN_SMASH, AttributeType::BRAIN_SMASH },
    { MonsterAbilityType::CAUSE_1, AttributeType::CAUSE_1 },
    { MonsterAbilityType::CAUSE_2, AttributeType::CAUSE_2 },
    { MonsterAbilityType::CAUSE_3, AttributeType::CAUSE_3 },
    { MonsterAbilityType::CAUSE_4, AttributeType::CAUSE_4 },
};
}

bool cast_blue_magic_spirit_curse(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    int dir;
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    const auto magic = BLUE_MAGIC_SPIRIT_CURSE_TABLE.find(bmc_ptr->spell);
    if (magic == BLUE_MAGIC_SPIRIT_CURSE_TABLE.end()) {
        const auto message = fmt::format("Unknown blue magic spirit curse: {}", static_cast<int>(bmc_ptr->spell));
        THROW_EXCEPTION(std::logic_error, message);
    }

    const auto attribute_type = magic->second;
    const auto damage = monspell_bluemage_damage(player_ptr, bmc_ptr->spell, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(player_ptr, attribute_type, dir, damage, 0);
    return true;
}
