/*!
 * @file blue-magic-breath.cpp
 * @brief 青魔法のブレス系呪文定義
 */

#include "blue-magic/blue-magic-breath.h"
#include "blue-magic/blue-magic-util.h"
#include "effect/attribute-types.h"
#include "mind/mind-blue-mage.h"
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
struct blue_magic_breath_type {
    AttributeType attribute_type;
    std::string_view message;
};

const std::unordered_map<MonsterAbilityType, blue_magic_breath_type> BLUE_MAGIC_BREATH_TABLE = {
    { MonsterAbilityType::BR_ACID, { AttributeType::ACID, _("酸のブレスを吐いた。", "You breathe acid.") } },
    { MonsterAbilityType::BR_ELEC, { AttributeType::ELEC, _("稲妻のブレスを吐いた。", "You breathe lightning.") } },
    { MonsterAbilityType::BR_FIRE, { AttributeType::FIRE, _("火炎のブレスを吐いた。", "You breathe fire.") } },
    { MonsterAbilityType::BR_COLD, { AttributeType::COLD, _("冷気のブレスを吐いた。", "You breathe frost.") } },
    { MonsterAbilityType::BR_POIS, { AttributeType::POIS, _("ガスのブレスを吐いた。", "You breathe gas.") } },
    { MonsterAbilityType::BR_NETH, { AttributeType::NETHER, _("地獄のブレスを吐いた。", "You breathe nether.") } },
    { MonsterAbilityType::BR_LITE, { AttributeType::LITE, _("閃光のブレスを吐いた。", "You breathe light.") } },
    { MonsterAbilityType::BR_DARK, { AttributeType::DARK, _("暗黒のブレスを吐いた。", "You breathe darkness.") } },
    { MonsterAbilityType::BR_CONF, { AttributeType::CONFUSION, _("混乱のブレスを吐いた。", "You breathe confusion.") } },
    { MonsterAbilityType::BR_SOUN, { AttributeType::SOUND, _("轟音のブレスを吐いた。", "You breathe sound.") } },
    { MonsterAbilityType::BR_CHAO, { AttributeType::CHAOS, _("カオスのブレスを吐いた。", "You breathe chaos.") } },
    { MonsterAbilityType::BR_DISE, { AttributeType::DISENCHANT, _("劣化のブレスを吐いた。", "You breathe disenchantment.") } },
    { MonsterAbilityType::BR_NEXU, { AttributeType::NEXUS, _("因果混乱のブレスを吐いた。", "You breathe nexus.") } },
    { MonsterAbilityType::BR_TIME, { AttributeType::TIME, _("時間逆転のブレスを吐いた。", "You breathe time.") } },
    { MonsterAbilityType::BR_INER, { AttributeType::INERTIAL, _("遅鈍のブレスを吐いた。", "You breathe inertia.") } },
    { MonsterAbilityType::BR_GRAV, { AttributeType::GRAVITY, _("重力のブレスを吐いた。", "You breathe gravity.") } },
    { MonsterAbilityType::BR_SHAR, { AttributeType::SHARDS, _("破片のブレスを吐いた。", "You breathe shards.") } },
    { MonsterAbilityType::BR_PLAS, { AttributeType::PLASMA, _("プラズマのブレスを吐いた。", "You breathe plasma.") } },
    { MonsterAbilityType::BR_FORC, { AttributeType::FORCE, _("フォースのブレスを吐いた。", "You breathe force.") } },
    { MonsterAbilityType::BR_MANA, { AttributeType::MANA, _("魔力のブレスを吐いた。", "You breathe mana.") } },
    { MonsterAbilityType::BR_NUKE, { AttributeType::NUKE, _("放射性廃棄物のブレスを吐いた。", "You breathe toxic waste.") } },
    { MonsterAbilityType::BR_DISI, { AttributeType::DISINTEGRATE, _("分解のブレスを吐いた。", "You breathe disintegration.") } },
    { MonsterAbilityType::BR_VOID, { AttributeType::VOID_MAGIC, _("虚無のブレスを吐いた。", "You breathe void.") } },
    { MonsterAbilityType::BR_ABYSS, { AttributeType::ABYSS, _("深淵のブレスを吐いた。", "You breathe abyss.") } },

};
}

bool cast_blue_magic_breath(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    const auto dir = get_aim_dir(player_ptr);
    if (!dir) {
        return false;
    }

    const auto magic = BLUE_MAGIC_BREATH_TABLE.find(bmc_ptr->spell);
    if (magic == BLUE_MAGIC_BREATH_TABLE.end()) {
        const auto message = fmt::format("Unknown blue magic breath: {}", static_cast<int>(bmc_ptr->spell));
        THROW_EXCEPTION(std::logic_error, message);
    }

    const auto &[attribute_type, message] = magic->second;
    msg_print(message);
    const auto radius = (bmc_ptr->plev > 40 ? 3 : 2);
    const auto damage = monspell_bluemage_damage(player_ptr, bmc_ptr->spell, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, attribute_type, dir, damage, radius);
    return true;
}
