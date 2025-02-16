/*!
 * @file blue-magic-caster.cpp
 * @brief 青魔法のその他系統の呪文定義と詠唱時分岐処理
 */

#include "blue-magic/blue-magic-caster.h"
#include "blue-magic/blue-magic-ball-bolt.h"
#include "blue-magic/blue-magic-breath.h"
#include "blue-magic/blue-magic-spirit-curse.h"
#include "blue-magic/blue-magic-status.h"
#include "blue-magic/blue-magic-summon.h"
#include "blue-magic/blue-magic-util.h"
#include "blue-magic/learnt-info.h"
#include "hpmp/hp-mp-processor.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-flags-resistance.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "mspell/mspell-damage-calculator.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell/spells-status.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "view/display-messages.h"

static bool cast_blue_dispel(PlayerType *player_ptr)
{
    const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
    if (!pos) {
        return false;
    }

    const auto &grid = player_ptr->current_floor_ptr->get_grid(*pos);
    const auto m_idx = grid.m_idx;
    if ((m_idx == 0) || !grid.has_los() || !projectable(player_ptr, player_ptr->get_position(), *pos)) {
        return true;
    }

    dispel_monster_status(player_ptr, m_idx);
    return true;
}

static bool cast_blue_rocket(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    const auto dir = get_aim_dir(player_ptr);
    if (!dir) {
        return false;
    }

    msg_print(_("ロケットを発射した。", "You fire a rocket."));
    const auto damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::ROCKET, bmc_ptr->plev, DAM_ROLL);
    fire_rocket(player_ptr, AttributeType::ROCKET, dir, damage, 2);
    return true;
}

static bool cast_blue_shoot(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    const auto dir = get_aim_dir(player_ptr);
    if (!dir) {
        return false;
    }

    msg_print(_("矢を放った。", "You fire an arrow."));
    const auto damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::SHOOT, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(player_ptr, AttributeType::MONSTER_SHOOT, dir, damage);
    return true;
}

static bool cast_blue_hand_doom(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    const auto dir = get_aim_dir(player_ptr);
    if (!dir) {
        return false;
    }

    msg_print(_("<破滅の手>を放った！", "You invoke the Hand of Doom!"));
    fire_ball_hide(player_ptr, AttributeType::HAND_DOOM, dir, bmc_ptr->plev * 3, 0);
    return true;
}

/* 効果が抵抗された場合、返される std::optional には値がありません。*/
static std::optional<std::string> exe_blue_teleport_back(PlayerType *player_ptr, const Pos2D &pos)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(pos);
    if (!grid.has_monster() || !grid.has_los() || !projectable(player_ptr, player_ptr->get_position(), pos)) {
        return std::nullopt;
    }

    const auto &monster = floor.m_list[grid.m_idx];
    auto &monrace = monster.get_monrace();
    auto m_name = monster_desc(player_ptr, monster, 0);
    if (monrace.resistance_flags.has_not(MonsterResistanceType::RESIST_TELEPORT)) {
        return m_name;
    }

    if (monrace.kind_flags.has(MonsterKindType::UNIQUE) || monrace.resistance_flags.has(MonsterResistanceType::RESIST_ALL)) {
        if (is_original_ap_and_seen(player_ptr, monster)) {
            monrace.r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
        }

        msg_format(_("%sには効果がなかった！", "%s is unaffected!"), m_name.data());
        return std::nullopt;
    }

    if (monrace.level <= randint1(100)) {
        return m_name;
    }

    if (is_original_ap_and_seen(player_ptr, monster)) {
        monrace.r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
    }

    msg_format(_("%sには耐性がある！", "%s resists!"), m_name.data());
    return std::nullopt;
}

static bool cast_blue_teleport_back(PlayerType *player_ptr)
{
    const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
    if (!pos) {
        return false;
    }

    const auto m_name = exe_blue_teleport_back(player_ptr, *pos);
    if (!m_name) {
        return true;
    }

    msg_format(_("%sを引き戻した。", "You command %s to return."), m_name->data());
    teleport_monster_to(
        player_ptr, player_ptr->current_floor_ptr->get_grid(*pos).m_idx, player_ptr->y, player_ptr->x, 100, TELEPORT_PASSIVE);
    return true;
}

static bool cast_blue_teleport_away(PlayerType *player_ptr)
{
    const auto dir = get_aim_dir(player_ptr);
    if (!dir) {
        return false;
    }

    (void)fire_beam(player_ptr, AttributeType::AWAY_ALL, dir, 100);
    return true;
}

static bool cast_blue_psy_spear(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    const auto dir = get_aim_dir(player_ptr);
    if (!dir) {
        return false;
    }

    msg_print(_("光の剣を放った。", "You throw a psycho-spear."));
    const auto damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::PSY_SPEAR, bmc_ptr->plev, DAM_ROLL);
    (void)fire_beam(player_ptr, AttributeType::PSY_SPEAR, dir, damage);
    return true;
}

static bool cast_blue_make_trap(PlayerType *player_ptr)
{
    const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
    if (!pos) {
        return false;
    }

    msg_print(_("呪文を唱えて邪悪に微笑んだ。", "You cast a spell and cackle evilly."));
    trap_creation(player_ptr, pos->y, pos->x);
    return true;
}

static bool switch_cast_blue_magic(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    switch (bmc_ptr->spell) {
    case MonsterAbilityType::SHRIEK:
        msg_print(_("かん高い金切り声をあげた。", "You make a high pitched shriek."));
        aggravate_monsters(player_ptr, 0);
        return true;
    case MonsterAbilityType::XXX1:
    case MonsterAbilityType::XXX2:
    case MonsterAbilityType::XXX3:
    case MonsterAbilityType::XXX4:
        return true;
    case MonsterAbilityType::DISPEL:
        return cast_blue_dispel(player_ptr);
    case MonsterAbilityType::ROCKET:
        return cast_blue_rocket(player_ptr, bmc_ptr);
    case MonsterAbilityType::SHOOT:
        return cast_blue_shoot(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_ACID:
    case MonsterAbilityType::BR_ELEC:
    case MonsterAbilityType::BR_FIRE:
    case MonsterAbilityType::BR_COLD:
    case MonsterAbilityType::BR_POIS:
    case MonsterAbilityType::BR_NETH:
    case MonsterAbilityType::BR_LITE:
    case MonsterAbilityType::BR_DARK:
    case MonsterAbilityType::BR_CONF:
    case MonsterAbilityType::BR_SOUN:
    case MonsterAbilityType::BR_CHAO:
    case MonsterAbilityType::BR_DISE:
    case MonsterAbilityType::BR_NEXU:
    case MonsterAbilityType::BR_TIME:
    case MonsterAbilityType::BR_INER:
    case MonsterAbilityType::BR_GRAV:
    case MonsterAbilityType::BR_SHAR:
    case MonsterAbilityType::BR_PLAS:
    case MonsterAbilityType::BR_FORC:
    case MonsterAbilityType::BR_MANA:
    case MonsterAbilityType::BR_NUKE:
    case MonsterAbilityType::BR_DISI:
    case MonsterAbilityType::BR_VOID:
    case MonsterAbilityType::BR_ABYSS:
        return cast_blue_magic_breath(player_ptr, bmc_ptr);
    case MonsterAbilityType::BA_ACID:
    case MonsterAbilityType::BA_ELEC:
    case MonsterAbilityType::BA_FIRE:
    case MonsterAbilityType::BA_COLD:
    case MonsterAbilityType::BA_POIS:
    case MonsterAbilityType::BA_NUKE:
    case MonsterAbilityType::BA_NETH:
    case MonsterAbilityType::BA_CHAO:
    case MonsterAbilityType::BA_WATE:
    case MonsterAbilityType::BA_LITE:
    case MonsterAbilityType::BA_DARK:
    case MonsterAbilityType::BA_MANA:
    case MonsterAbilityType::BA_VOID:
    case MonsterAbilityType::BA_ABYSS:
    case MonsterAbilityType::BA_METEOR:
        return cast_blue_magic_ball(player_ptr, bmc_ptr);
    case MonsterAbilityType::DRAIN_MANA:
    case MonsterAbilityType::MIND_BLAST:
    case MonsterAbilityType::BRAIN_SMASH:
    case MonsterAbilityType::CAUSE_1:
    case MonsterAbilityType::CAUSE_2:
    case MonsterAbilityType::CAUSE_3:
    case MonsterAbilityType::CAUSE_4:
        return cast_blue_magic_spirit_curse(player_ptr, bmc_ptr);
    case MonsterAbilityType::BO_ACID:
    case MonsterAbilityType::BO_ELEC:
    case MonsterAbilityType::BO_FIRE:
    case MonsterAbilityType::BO_COLD:
    case MonsterAbilityType::BO_NETH:
    case MonsterAbilityType::BO_WATE:
    case MonsterAbilityType::BO_MANA:
    case MonsterAbilityType::BO_PLAS:
    case MonsterAbilityType::BO_ICEE:
    case MonsterAbilityType::BO_ABYSS:
    case MonsterAbilityType::BO_VOID:
    case MonsterAbilityType::BO_METEOR:
    case MonsterAbilityType::BO_LITE:
    case MonsterAbilityType::MISSILE:
        return cast_blue_magic_bolt(player_ptr, bmc_ptr);
    case MonsterAbilityType::SCARE:
        return cast_blue_scare(player_ptr, bmc_ptr);
    case MonsterAbilityType::BLIND:
        return cast_blue_blind(player_ptr, bmc_ptr);
    case MonsterAbilityType::CONF:
        return cast_blue_confusion(player_ptr, bmc_ptr);
    case MonsterAbilityType::SLOW:
        return cast_blue_slow(player_ptr, bmc_ptr);
    case MonsterAbilityType::HOLD:
        return cast_blue_sleep(player_ptr, bmc_ptr);
    case MonsterAbilityType::HASTE:
        (void)set_acceleration(player_ptr, randint1(20 + bmc_ptr->plev) + bmc_ptr->plev, false);
        return true;
    case MonsterAbilityType::HAND_DOOM:
        return cast_blue_hand_doom(player_ptr, bmc_ptr);
    case MonsterAbilityType::HEAL: {
        msg_print(_("自分の傷に念を集中した。", "You concentrate on your wounds!"));
        (void)hp_player(player_ptr, bmc_ptr->plev * 4);
        BadStatusSetter bss(player_ptr);
        (void)bss.set_stun(0);
        (void)bss.set_cut(0);
        return true;
    }
    case MonsterAbilityType::INVULNER:
        msg_print(_("無傷の球の呪文を唱えた。", "You cast a Globe of Invulnerability."));
        (void)set_invuln(player_ptr, randint1(4) + 4, false);
        return true;
    case MonsterAbilityType::BLINK:
        teleport_player(player_ptr, 10, TELEPORT_SPONTANEOUS);
        return true;
    case MonsterAbilityType::TPORT:
        teleport_player(player_ptr, bmc_ptr->plev * 5, TELEPORT_SPONTANEOUS);
        return true;
    case MonsterAbilityType::WORLD:
        (void)time_walk(player_ptr);
        return true;
    case MonsterAbilityType::SPECIAL:
        return true;
    case MonsterAbilityType::TELE_TO:
        return cast_blue_teleport_back(player_ptr);
    case MonsterAbilityType::TELE_AWAY:
        return cast_blue_teleport_away(player_ptr);
    case MonsterAbilityType::TELE_LEVEL:
        return teleport_level_other(player_ptr);
    case MonsterAbilityType::PSY_SPEAR:
        return cast_blue_psy_spear(player_ptr, bmc_ptr);
    case MonsterAbilityType::DARKNESS:
        msg_print(_("暗闇の中で手を振った。", "You gesture in shadow."));
        (void)unlite_area(player_ptr, 10, 3);
        return true;
    case MonsterAbilityType::TRAPS:
        return cast_blue_make_trap(player_ptr);
    case MonsterAbilityType::FORGET:
        msg_print(_("しかし何も起きなかった。", "Nothing happens."));
        return true;
    case MonsterAbilityType::RAISE_DEAD:
        msg_print(_("死者復活の呪文を唱えた。", "You animate the dead."));
        (void)animate_dead(player_ptr, 0, player_ptr->y, player_ptr->x);
        return true;
    case MonsterAbilityType::S_KIN:
        return cast_blue_summon_kin(player_ptr, bmc_ptr);
    case MonsterAbilityType::S_CYBER:
        return cast_blue_summon_cyber(player_ptr, bmc_ptr);
    case MonsterAbilityType::S_MONSTER:
        return cast_blue_summon_monster(player_ptr, bmc_ptr);
    case MonsterAbilityType::S_MONSTERS:
        return cast_blue_summon_monsters(player_ptr, bmc_ptr);
    case MonsterAbilityType::S_ANT:
        return cast_blue_summon_ant(player_ptr, bmc_ptr);
    case MonsterAbilityType::S_SPIDER:
        return cast_blue_summon_spider(player_ptr, bmc_ptr);
    case MonsterAbilityType::S_HOUND:
        return cast_blue_summon_hound(player_ptr, bmc_ptr);
    case MonsterAbilityType::S_HYDRA:
        return cast_blue_summon_hydra(player_ptr, bmc_ptr);
    case MonsterAbilityType::S_ANGEL:
        return cast_blue_summon_angel(player_ptr, bmc_ptr);
    case MonsterAbilityType::S_DEMON:
        return cast_blue_summon_demon(player_ptr, bmc_ptr);
    case MonsterAbilityType::S_UNDEAD:
        return cast_blue_summon_undead(player_ptr, bmc_ptr);
    case MonsterAbilityType::S_DRAGON:
        return cast_blue_summon_dragon(player_ptr, bmc_ptr);
    case MonsterAbilityType::S_HI_UNDEAD:
        return cast_blue_summon_high_undead(player_ptr, bmc_ptr);
    case MonsterAbilityType::S_HI_DRAGON:
        return cast_blue_summon_high_dragon(player_ptr, bmc_ptr);
    case MonsterAbilityType::S_AMBERITES:
        return cast_blue_summon_amberite(player_ptr, bmc_ptr);
    case MonsterAbilityType::S_UNIQUE:
        return cast_blue_summon_unique(player_ptr, bmc_ptr);
    case MonsterAbilityType::S_DEAD_UNIQUE:
        return cast_blue_summon_dead_unique(player_ptr, bmc_ptr);
    default:
        msg_print("hoge?");
        return true;
    }
}

/*!
 * @brief 青魔法の発動 /
 * do_cmd_cast calls this function if the player's class is 'blue-mage'.
 * @param spell 発動するモンスター攻撃のID
 * @param success TRUEは成功時、FALSEは失敗時の処理を行う
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
bool cast_learned_spell(PlayerType *player_ptr, MonsterAbilityType spell, const bool success)
{
    bmc_type tmp_bm;
    bmc_type *bmc_ptr = initialize_blue_magic_type(player_ptr, &tmp_bm, spell, success, get_pseudo_monstetr_level);
    if (!switch_cast_blue_magic(player_ptr, bmc_ptr)) {
        return false;
    }

    if (bmc_ptr->no_trump) {
        msg_print(_("何も現れなかった。", "No one appeared."));
    }

    return true;
}
