/*!
 * @file mspell-projection-table.cpp
 * @brief プレイヤーが使うモンスター魔法のうち、ブレス・ボール・ボルトの表
 */

#include "mspell/mspell-projection-table.h"
#include "effect/attribute-types.h"
#include "locale/language-switcher.h"
#include "monster-race/race-ability-flags.h"
#include "spell-kind/spells-launcher.h"
#include "view/display-messages.h"
#include <unordered_map>

namespace {
const std::unordered_map<MonsterAbilityType, MspellProjection> MSPELL_PROJECTION_TABLE = {
    { MonsterAbilityType::BR_ACID, { MspellProjectionType::BREATH, AttributeType::ACID, 0, _("酸のブレスを吐いた。", "You breathe acid.") } },
    { MonsterAbilityType::BR_ELEC, { MspellProjectionType::BREATH, AttributeType::ELEC, 0, _("稲妻のブレスを吐いた。", "You breathe lightning.") } },
    { MonsterAbilityType::BR_FIRE, { MspellProjectionType::BREATH, AttributeType::FIRE, 0, _("火炎のブレスを吐いた。", "You breathe fire.") } },
    { MonsterAbilityType::BR_COLD, { MspellProjectionType::BREATH, AttributeType::COLD, 0, _("冷気のブレスを吐いた。", "You breathe frost.") } },
    { MonsterAbilityType::BR_POIS, { MspellProjectionType::BREATH, AttributeType::POIS, 0, _("ガスのブレスを吐いた。", "You breathe gas.") } },
    { MonsterAbilityType::BR_NETH, { MspellProjectionType::BREATH, AttributeType::NETHER, 0, _("地獄のブレスを吐いた。", "You breathe nether.") } },
    { MonsterAbilityType::BR_LITE, { MspellProjectionType::BREATH, AttributeType::LITE, 0, _("閃光のブレスを吐いた。", "You breathe light.") } },
    { MonsterAbilityType::BR_DARK, { MspellProjectionType::BREATH, AttributeType::DARK, 0, _("暗黒のブレスを吐いた。", "You breathe darkness.") } },
    { MonsterAbilityType::BR_CONF, { MspellProjectionType::BREATH, AttributeType::CONFUSION, 0, _("混乱のブレスを吐いた。", "You breathe confusion.") } },
    { MonsterAbilityType::BR_SOUN, { MspellProjectionType::BREATH, AttributeType::SOUND, 0, _("轟音のブレスを吐いた。", "You breathe sound.") } },
    { MonsterAbilityType::BR_CHAO, { MspellProjectionType::BREATH, AttributeType::CHAOS, 0, _("カオスのブレスを吐いた。", "You breathe chaos.") } },
    { MonsterAbilityType::BR_DISE, { MspellProjectionType::BREATH, AttributeType::DISENCHANT, 0, _("劣化のブレスを吐いた。", "You breathe disenchantment.") } },
    { MonsterAbilityType::BR_NEXU, { MspellProjectionType::BREATH, AttributeType::NEXUS, 0, _("因果混乱のブレスを吐いた。", "You breathe nexus.") } },
    { MonsterAbilityType::BR_TIME, { MspellProjectionType::BREATH, AttributeType::TIME, 0, _("時間逆転のブレスを吐いた。", "You breathe time.") } },
    { MonsterAbilityType::BR_INER, { MspellProjectionType::BREATH, AttributeType::INERTIAL, 0, _("遅鈍のブレスを吐いた。", "You breathe inertia.") } },
    { MonsterAbilityType::BR_GRAV, { MspellProjectionType::BREATH, AttributeType::GRAVITY, 0, _("重力のブレスを吐いた。", "You breathe gravity.") } },
    { MonsterAbilityType::BR_SHAR, { MspellProjectionType::BREATH, AttributeType::SHARDS, 0, _("破片のブレスを吐いた。", "You breathe shards.") } },
    { MonsterAbilityType::BR_PLAS, { MspellProjectionType::BREATH, AttributeType::PLASMA, 0, _("プラズマのブレスを吐いた。", "You breathe plasma.") } },
    { MonsterAbilityType::BR_FORC, { MspellProjectionType::BREATH, AttributeType::FORCE, 0, _("フォースのブレスを吐いた。", "You breathe force.") } },
    { MonsterAbilityType::BR_MANA, { MspellProjectionType::BREATH, AttributeType::MANA, 0, _("魔力のブレスを吐いた。", "You breathe mana.") } },
    { MonsterAbilityType::BR_NUKE, { MspellProjectionType::BREATH, AttributeType::NUKE, 0, _("放射性廃棄物のブレスを吐いた。", "You breathe toxic waste.") } },
    { MonsterAbilityType::BR_DISI, { MspellProjectionType::BREATH, AttributeType::DISINTEGRATE, 0, _("分解のブレスを吐いた。", "You breathe disintegration.") } },
    { MonsterAbilityType::BR_VOID, { MspellProjectionType::BREATH, AttributeType::VOID_MAGIC, 0, _("虚無のブレスを吐いた。", "You breathe void.") } },
    { MonsterAbilityType::BR_ABYSS, { MspellProjectionType::BREATH, AttributeType::ABYSS, 0, _("深淵のブレスを吐いた。", "You breathe abyss.") } },
    { MonsterAbilityType::BA_ACID, { MspellProjectionType::BALL, AttributeType::ACID, 2, _("アシッド・ボールの呪文を唱えた。", "You cast an acid ball.") } },
    { MonsterAbilityType::BA_ELEC, { MspellProjectionType::BALL, AttributeType::ELEC, 2, _("サンダー・ボールの呪文を唱えた。", "You cast a lightning ball.") } },
    { MonsterAbilityType::BA_FIRE, { MspellProjectionType::BALL, AttributeType::FIRE, 2, _("ファイア・ボールの呪文を唱えた。", "You cast a fire ball.") } },
    { MonsterAbilityType::BA_COLD, { MspellProjectionType::BALL, AttributeType::COLD, 2, _("アイス・ボールの呪文を唱えた。", "You cast a frost ball.") } },
    { MonsterAbilityType::BA_POIS, { MspellProjectionType::BALL, AttributeType::POIS, 2, _("悪臭雲の呪文を唱えた。", "You cast a stinking cloud.") } },
    { MonsterAbilityType::BA_NUKE, { MspellProjectionType::BALL, AttributeType::NUKE, 2, _("放射能球を放った。", "You cast a ball of radiation.") } },
    { MonsterAbilityType::BA_NETH, { MspellProjectionType::BALL, AttributeType::NETHER, 2, _("地獄球の呪文を唱えた。", "You cast a nether ball.") } },
    { MonsterAbilityType::BA_CHAO, { MspellProjectionType::BALL, AttributeType::CHAOS, 4, _("純ログルスを放った。", "You invoke a raw Logrus.") } },
    { MonsterAbilityType::BA_WATE, { MspellProjectionType::BALL, AttributeType::WATER, 4, _("流れるような身振りをした。", "You gesture fluidly.") } },
    { MonsterAbilityType::BA_LITE, { MspellProjectionType::BALL, AttributeType::LITE, 4, _("スターバーストの呪文を念じた。", "You invoke a starburst.") } },
    { MonsterAbilityType::BA_DARK, { MspellProjectionType::BALL, AttributeType::DARK, 4, _("暗黒の嵐の呪文を念じた。", "You invoke a darkness storm.") } },
    { MonsterAbilityType::BA_MANA, { MspellProjectionType::BALL, AttributeType::MANA, 4, _("魔力の嵐の呪文を念じた。", "You invoke a mana storm.") } },
    { MonsterAbilityType::BA_VOID, { MspellProjectionType::BALL, AttributeType::VOID_MAGIC, 4, _("虚無の嵐の呪文を念じた。", "You invoke a void storm.") } },
    { MonsterAbilityType::BA_ABYSS, { MspellProjectionType::BALL, AttributeType::ABYSS, 4, _("深淵の嵐の呪文を念じた。", "You invoke a abyss storm.") } },
    { MonsterAbilityType::BA_METEOR, { MspellProjectionType::BALL, AttributeType::METEOR, 4, _("メテオスウォームの呪文を念じた。", "You invoke a meteor swarm.") } },
    { MonsterAbilityType::BO_ACID, { MspellProjectionType::BOLT, AttributeType::ACID, 0, _("アシッド・ボルトの呪文を唱えた。", "You cast an acid bolt.") } },
    { MonsterAbilityType::BO_ELEC, { MspellProjectionType::BOLT, AttributeType::ELEC, 0, _("サンダー・ボルトの呪文を唱えた。", "You cast a lightning bolt.") } },
    { MonsterAbilityType::BO_FIRE, { MspellProjectionType::BOLT, AttributeType::FIRE, 0, _("ファイア・ボルトの呪文を唱えた。", "You cast a fire bolt.") } },
    { MonsterAbilityType::BO_COLD, { MspellProjectionType::BOLT, AttributeType::COLD, 0, _("アイス・ボルトの呪文を唱えた。", "You cast a frost bolt.") } },
    { MonsterAbilityType::BO_NETH, { MspellProjectionType::BOLT, AttributeType::NETHER, 0, _("地獄の矢の呪文を唱えた。", "You cast a nether bolt.") } },
    { MonsterAbilityType::BO_WATE, { MspellProjectionType::BOLT, AttributeType::WATER, 0, _("ウォーター・ボルトの呪文を唱えた。", "You cast a water bolt.") } },
    { MonsterAbilityType::BO_MANA, { MspellProjectionType::BOLT, AttributeType::MANA, 0, _("魔力の矢の呪文を唱えた。", "You cast a mana bolt.") } },
    { MonsterAbilityType::BO_PLAS, { MspellProjectionType::BOLT, AttributeType::PLASMA, 0, _("プラズマ・ボルトの呪文を唱えた。", "You cast a plasma bolt.") } },
    { MonsterAbilityType::BO_ICEE, { MspellProjectionType::BOLT, AttributeType::ICE, 0, _("極寒の矢の呪文を唱えた。", "You cast a ice bolt.") } },
    { MonsterAbilityType::MISSILE, { MspellProjectionType::BOLT, AttributeType::MISSILE, 0, _("マジック・ミサイルの呪文を唱えた。", "You cast a magic missile.") } },
    { MonsterAbilityType::BO_ABYSS, { MspellProjectionType::BOLT, AttributeType::ABYSS, 0, _("アビス・ボルトの呪文を唱えた。", "You cast a abyss bolt.") } },
    { MonsterAbilityType::BO_VOID, { MspellProjectionType::BOLT, AttributeType::VOID_MAGIC, 0, _("ヴォイド・ボルトの呪文を唱えた。", "You cast a void bolt.") } },
    { MonsterAbilityType::BO_METEOR, { MspellProjectionType::BOLT, AttributeType::METEOR, 0, _("メテオストライクの呪文を唱えた。", "You cast a meteor strike.") } },
    { MonsterAbilityType::BO_LITE, { MspellProjectionType::BOLT, AttributeType::LITE, 0, _("スターライトアローの呪文を唱えた。", "You cast a starlight arrow.") } },
};
}

/*!
 * @brief モンスター魔法のブレス・ボール・ボルトの定義を探す
 * @param ability モンスター魔法の種類
 * @return ブレス・ボール・ボルトならその定義、それ以外なら nullopt
 */
tl::optional<const MspellProjection &> find_mspell_projection(MonsterAbilityType ability)
{
    const auto it = MSPELL_PROJECTION_TABLE.find(ability);
    if (it == MSPELL_PROJECTION_TABLE.end()) {
        return tl::nullopt;
    }

    return it->second;
}

/*!
 * @brief モンスター魔法のブレス・ボール・ボルトを、メッセージを出してから放つ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param projection 放つ魔法の定義
 * @param dir 放つ方向
 * @param damage 威力
 * @param breath_radius ブレスの半径 (職業によって求め方が違うので、呼び出し側で決める)
 */
void fire_mspell_projection(PlayerType *player_ptr, const MspellProjection &projection, const Direction &dir, int damage, int breath_radius)
{
    msg_print(projection.message);
    switch (projection.type) {
    case MspellProjectionType::BREATH:
        fire_breath(player_ptr, projection.attribute, dir, damage, breath_radius);
        return;
    case MspellProjectionType::BALL:
        fire_ball(player_ptr, projection.attribute, dir, damage, projection.radius);
        return;
    case MspellProjectionType::BOLT:
        fire_bolt(player_ptr, projection.attribute, dir, damage);
        return;
    }
}
