/*!
 * @file blue-magic-ball-bolt.cpp
 * @brief 青魔法のボール/ボルト系呪文定義
 */

#include "blue-magic/blue-magic-ball-bolt.h"
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
struct blue_magic_ball_type {
    AttributeType attribute_type;
    int radius;
    std::string_view message;
};

const std::unordered_map<MonsterAbilityType, blue_magic_ball_type> BLUE_MAIGC_BALL_TABLE = {
    { MonsterAbilityType::BA_ACID, { AttributeType::ACID, 2, _("アシッド・ボールの呪文を唱えた。", "You cast an acid ball.") } },
    { MonsterAbilityType::BA_ELEC, { AttributeType::ELEC, 2, _("サンダー・ボールの呪文を唱えた。", "You cast a lightning ball.") } },
    { MonsterAbilityType::BA_FIRE, { AttributeType::FIRE, 2, _("ファイア・ボールの呪文を唱えた。", "You cast a fire ball.") } },
    { MonsterAbilityType::BA_COLD, { AttributeType::COLD, 2, _("アイス・ボールの呪文を唱えた。", "You cast a frost ball.") } },
    { MonsterAbilityType::BA_POIS, { AttributeType::POIS, 2, _("悪臭雲の呪文を唱えた。", "You cast a stinking cloud.") } },
    { MonsterAbilityType::BA_NUKE, { AttributeType::NUKE, 2, _("放射能球を放った。", "You cast a ball of radiation.") } },
    { MonsterAbilityType::BA_NETH, { AttributeType::NETHER, 2, _("地獄球の呪文を唱えた。", "You cast a nether ball.") } },
    { MonsterAbilityType::BA_CHAO, { AttributeType::CHAOS, 4, _("純ログルスを放った。", "You invoke a raw Logrus.") } },
    { MonsterAbilityType::BA_WATE, { AttributeType::WATER, 4, _("流れるような身振りをした。", "You gesture fluidly.") } },
    { MonsterAbilityType::BA_LITE, { AttributeType::LITE, 4, _("スターバーストの呪文を念じた。", "You invoke a starburst.") } },
    { MonsterAbilityType::BA_DARK, { AttributeType::DARK, 4, _("暗黒の嵐の呪文を念じた。", "You invoke a darkness storm.") } },
    { MonsterAbilityType::BA_MANA, { AttributeType::MANA, 4, _("魔力の嵐の呪文を念じた。", "You invoke a mana storm.") } },
    { MonsterAbilityType::BA_VOID, { AttributeType::VOID_MAGIC, 4, _("虚無の嵐の呪文を念じた。", "You invoke a void storm.") } },
    { MonsterAbilityType::BA_ABYSS, { AttributeType::ABYSS, 4, _("深淵の嵐の呪文を念じた。", "You invoke a abyss storm.") } },
    { MonsterAbilityType::BA_METEOR, { AttributeType::METEOR, 4, _("メテオスウォームの呪文を念じた。", "You invoke a meteor swarm.") } },
};

struct blue_magic_bolt_type {
    AttributeType attribute_type;
    std::string_view message;
};

const std::unordered_map<MonsterAbilityType, blue_magic_bolt_type> BLUE_MAGIC_BOLT_TABLE = {
    { MonsterAbilityType::BO_ACID, { AttributeType::ACID, _("アシッド・ボルトの呪文を唱えた。", "You cast an acid bolt.") } },
    { MonsterAbilityType::BO_ELEC, { AttributeType::ELEC, _("サンダー・ボルトの呪文を唱えた。", "You cast a lightning bolt.") } },
    { MonsterAbilityType::BO_FIRE, { AttributeType::FIRE, _("ファイア・ボルトの呪文を唱えた。", "You cast a fire bolt.") } },
    { MonsterAbilityType::BO_COLD, { AttributeType::COLD, _("アイス・ボルトの呪文を唱えた。", "You cast a frost bolt.") } },
    { MonsterAbilityType::BO_NETH, { AttributeType::NETHER, _("地獄の矢の呪文を唱えた。", "You cast a nether bolt.") } },
    { MonsterAbilityType::BO_WATE, { AttributeType::WATER, _("ウォーター・ボルトの呪文を唱えた。", "You cast a water bolt.") } },
    { MonsterAbilityType::BO_MANA, { AttributeType::MANA, _("魔力の矢の呪文を唱えた。", "You cast a mana bolt.") } },
    { MonsterAbilityType::BO_PLAS, { AttributeType::PLASMA, _("プラズマ・ボルトの呪文を唱えた。", "You cast a plasma bolt.") } },
    { MonsterAbilityType::BO_ICEE, { AttributeType::ICE, _("極寒の矢の呪文を唱えた。", "You cast a ice bolt.") } },
    { MonsterAbilityType::MISSILE, { AttributeType::MISSILE, _("マジック・ミサイルの呪文を唱えた。", "You cast a magic missile.") } },
    { MonsterAbilityType::BO_ABYSS, { AttributeType::ABYSS, _("アビス・ボルトの呪文を唱えた。", "You cast a abyss bolt.") } },
    { MonsterAbilityType::BO_VOID, { AttributeType::VOID_MAGIC, _("ヴォイド・ボルトの呪文を唱えた。", "You cast a void bolt.") } },
    { MonsterAbilityType::BO_METEOR, { AttributeType::METEOR, _("メテオストライクの呪文を唱えた。", "You cast a meteor strike.") } },
    { MonsterAbilityType::BO_LITE, { AttributeType::LITE, _("スターライトアローの呪文を唱えた。", "You cast a starlight arrow.") } },
};
}

bool cast_blue_magic_ball(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    int dir;
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    const auto magic = BLUE_MAIGC_BALL_TABLE.find(bmc_ptr->spell);
    if (magic == BLUE_MAIGC_BALL_TABLE.end()) {
        const auto message = fmt::format("Unknown blue magic ball: {}", static_cast<int>(bmc_ptr->spell));
        THROW_EXCEPTION(std::logic_error, message);
    }

    const auto &[attribute_type, radius, message] = magic->second;
    msg_print(message);
    const auto damage = monspell_bluemage_damage(player_ptr, bmc_ptr->spell, bmc_ptr->plev, DAM_ROLL);
    fire_ball(player_ptr, attribute_type, dir, damage, radius);
    return true;
};

bool cast_blue_magic_bolt(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    int dir;
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    const auto magic = BLUE_MAGIC_BOLT_TABLE.find(bmc_ptr->spell);
    if (magic == BLUE_MAGIC_BOLT_TABLE.end()) {
        const auto message = fmt::format("Unknown blue magic bolt: {}", static_cast<int>(bmc_ptr->spell));
        THROW_EXCEPTION(std::logic_error, message);
    }

    const auto &[attribute_type, message] = magic->second;
    msg_print(message);
    const auto damage = monspell_bluemage_damage(player_ptr, bmc_ptr->spell, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(player_ptr, attribute_type, dir, damage);
    return true;
};
