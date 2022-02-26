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
#include "floor/cave.h"
#include "hpmp/hp-mp-processor.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
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
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "view/display-messages.h"

static bool cast_blue_dispel(PlayerType *player_ptr)
{
    if (!target_set(player_ptr, TARGET_KILL)) {
        return false;
    }

    MONSTER_IDX m_idx = player_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx;
    if ((m_idx == 0) || !player_has_los_bold(player_ptr, target_row, target_col) || !projectable(player_ptr, player_ptr->y, player_ptr->x, target_row, target_col)) {
        return true;
    }

    dispel_monster_status(player_ptr, m_idx);
    return true;
}

static bool cast_blue_rocket(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("ロケットを発射した。", "You fire a rocket."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::ROCKET, bmc_ptr->plev, DAM_ROLL);
    fire_rocket(player_ptr, AttributeType::ROCKET, bmc_ptr->dir, bmc_ptr->damage, 2);
    return true;
}

static bool cast_blue_shoot(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("矢を放った。", "You fire an arrow."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::SHOOT, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(player_ptr, AttributeType::MONSTER_SHOOT, bmc_ptr->dir, bmc_ptr->damage);
    return true;
}

static bool cast_blue_hand_doom(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("<破滅の手>を放った！", "You invoke the Hand of Doom!"));
    fire_ball_hide(player_ptr, AttributeType::HAND_DOOM, bmc_ptr->dir, bmc_ptr->plev * 3, 0);
    return true;
}

static bool exe_blue_teleport_back(PlayerType *player_ptr, GAME_TEXT *m_name)
{
    monster_type *m_ptr;
    monster_race *r_ptr;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if ((floor_ptr->grid_array[target_row][target_col].m_idx == 0) || !player_has_los_bold(player_ptr, target_row, target_col) || !projectable(player_ptr, player_ptr->y, player_ptr->x, target_row, target_col)) {
        return true;
    }

    m_ptr = &floor_ptr->m_list[floor_ptr->grid_array[target_row][target_col].m_idx];
    r_ptr = &r_info[m_ptr->r_idx];
    monster_desc(player_ptr, m_name, m_ptr, 0);
    if (r_ptr->resistance_flags.has_not(MonsterResistanceType::RESIST_TELEPORT)) {
        return false;
    }

    if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE) || r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_ALL)) {
        if (is_original_ap_and_seen(player_ptr, m_ptr)) {
            r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
        }

        msg_format(_("%sには効果がなかった！", "%s is unaffected!"), m_name);
        return true;
    }

    if (r_ptr->level <= randint1(100)) {
        return false;
    }

    if (is_original_ap_and_seen(player_ptr, m_ptr)) {
        r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
    }

    msg_format(_("%sには耐性がある！", "%s resists!"), m_name);
    return true;
}

static bool cast_blue_teleport_back(PlayerType *player_ptr)
{
    if (!target_set(player_ptr, TARGET_KILL)) {
        return false;
    }

    GAME_TEXT m_name[MAX_NLEN];
    if (exe_blue_teleport_back(player_ptr, m_name)) {
        return true;
    }

    msg_format(_("%sを引き戻した。", "You command %s to return."), m_name);
    teleport_monster_to(
        player_ptr, player_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx, player_ptr->y, player_ptr->x, 100, TELEPORT_PASSIVE);
    return true;
}

static bool cast_blue_teleport_away(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    (void)fire_beam(player_ptr, AttributeType::AWAY_ALL, bmc_ptr->dir, 100);
    return true;
}

static bool cast_blue_psy_spear(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("光の剣を放った。", "You throw a psycho-spear."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::PSY_SPEAR, bmc_ptr->plev, DAM_ROLL);
    (void)fire_beam(player_ptr, AttributeType::PSY_SPEAR, bmc_ptr->dir, bmc_ptr->damage);
    return true;
}

static bool cast_blue_make_trap(PlayerType *player_ptr)
{
    if (!target_set(player_ptr, TARGET_KILL)) {
        return false;
    }

    msg_print(_("呪文を唱えて邪悪に微笑んだ。", "You cast a spell and cackle evilly."));
    trap_creation(player_ptr, target_row, target_col);
    return true;
}

static bool switch_cast_blue_magic(PlayerType *player_ptr, bmc_type *bmc_ptr, MonsterAbilityType spell)
{
    switch (spell) {
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
        return cast_blue_breath_acid(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_ELEC:
        return cast_blue_breath_elec(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_FIRE:
        return cast_blue_breath_fire(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_COLD:
        return cast_blue_breath_cold(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_POIS:
        return cast_blue_breath_pois(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_NETH:
        return cast_blue_breath_nether(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_LITE:
        return cast_blue_breath_lite(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_DARK:
        return cast_blue_breath_dark(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_CONF:
        return cast_blue_breath_conf(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_SOUN:
        return cast_blue_breath_sound(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_CHAO:
        return cast_blue_breath_chaos(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_DISE:
        return cast_blue_breath_disenchant(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_NEXU:
        return cast_blue_breath_nexus(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_TIME:
        return cast_blue_breath_time(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_INER:
        return cast_blue_breath_inertia(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_GRAV:
        return cast_blue_breath_gravity(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_SHAR:
        return cast_blue_breath_shards(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_PLAS:
        return cast_blue_breath_plasma(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_FORC:
        return cast_blue_breath_force(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_MANA:
        return cast_blue_breath_mana(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_NUKE:
        return cast_blue_breath_nuke(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_DISI:
        return cast_blue_breath_disintegration(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_VOID:
        return cast_blue_breath_void(player_ptr, bmc_ptr);
    case MonsterAbilityType::BR_ABYSS:
        return cast_blue_breath_abyss(player_ptr, bmc_ptr);
    case MonsterAbilityType::BA_ACID:
        return cast_blue_ball_acid(player_ptr, bmc_ptr);
    case MonsterAbilityType::BA_ELEC:
        return cast_blue_ball_elec(player_ptr, bmc_ptr);
    case MonsterAbilityType::BA_FIRE:
        return cast_blue_ball_fire(player_ptr, bmc_ptr);
    case MonsterAbilityType::BA_COLD:
        return cast_blue_ball_cold(player_ptr, bmc_ptr);
    case MonsterAbilityType::BA_POIS:
        return cast_blue_ball_pois(player_ptr, bmc_ptr);
    case MonsterAbilityType::BA_NUKE:
        return cast_blue_ball_nuke(player_ptr, bmc_ptr);
    case MonsterAbilityType::BA_NETH:
        return cast_blue_ball_nether(player_ptr, bmc_ptr);
    case MonsterAbilityType::BA_CHAO:
        return cast_blue_ball_chaos(player_ptr, bmc_ptr);
    case MonsterAbilityType::BA_WATE:
        return cast_blue_ball_water(player_ptr, bmc_ptr);
    case MonsterAbilityType::BA_LITE:
        return cast_blue_ball_star_burst(player_ptr, bmc_ptr);
    case MonsterAbilityType::BA_DARK:
        return cast_blue_ball_dark_storm(player_ptr, bmc_ptr);
    case MonsterAbilityType::BA_MANA:
        return cast_blue_ball_mana_storm(player_ptr, bmc_ptr);
    case MonsterAbilityType::BA_VOID:
        return cast_blue_ball_void(player_ptr, bmc_ptr);
    case MonsterAbilityType::BA_ABYSS:
        return cast_blue_ball_abyss(player_ptr, bmc_ptr);
    case MonsterAbilityType::DRAIN_MANA:
        return cast_blue_drain_mana(player_ptr, bmc_ptr);
    case MonsterAbilityType::MIND_BLAST:
        return cast_blue_mind_blast(player_ptr, bmc_ptr);
    case MonsterAbilityType::BRAIN_SMASH:
        return cast_blue_brain_smash(player_ptr, bmc_ptr);
    case MonsterAbilityType::CAUSE_1:
        return cast_blue_curse_1(player_ptr, bmc_ptr);
    case MonsterAbilityType::CAUSE_2:
        return cast_blue_curse_2(player_ptr, bmc_ptr);
    case MonsterAbilityType::CAUSE_3:
        return cast_blue_curse_3(player_ptr, bmc_ptr);
    case MonsterAbilityType::CAUSE_4:
        return cast_blue_curse_4(player_ptr, bmc_ptr);
    case MonsterAbilityType::BO_ACID:
        return cast_blue_bolt_acid(player_ptr, bmc_ptr);
    case MonsterAbilityType::BO_ELEC:
        return cast_blue_bolt_elec(player_ptr, bmc_ptr);
    case MonsterAbilityType::BO_FIRE:
        return cast_blue_bolt_fire(player_ptr, bmc_ptr);
    case MonsterAbilityType::BO_COLD:
        return cast_blue_bolt_cold(player_ptr, bmc_ptr);
    case MonsterAbilityType::BO_NETH:
        return cast_blue_bolt_nether(player_ptr, bmc_ptr);
    case MonsterAbilityType::BO_WATE:
        return cast_blue_bolt_water(player_ptr, bmc_ptr);
    case MonsterAbilityType::BO_MANA:
        return cast_blue_bolt_mana(player_ptr, bmc_ptr);
    case MonsterAbilityType::BO_PLAS:
        return cast_blue_bolt_plasma(player_ptr, bmc_ptr);
    case MonsterAbilityType::BO_ICEE:
        return cast_blue_bolt_icee(player_ptr, bmc_ptr);
    case MonsterAbilityType::BO_ABYSS:
        return cast_blue_bolt_abyss(player_ptr, bmc_ptr);
    case MonsterAbilityType::BO_VOID:
        return cast_blue_bolt_void(player_ptr, bmc_ptr);
    case MonsterAbilityType::MISSILE:
        return cast_blue_bolt_missile(player_ptr, bmc_ptr);
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
        (void)set_fast(player_ptr, randint1(20 + bmc_ptr->plev) + bmc_ptr->plev, false);
        return true;
    case MonsterAbilityType::HAND_DOOM:
        return cast_blue_hand_doom(player_ptr, bmc_ptr);
    case MonsterAbilityType::HEAL: {
        msg_print(_("自分の傷に念を集中した。", "You concentrate on your wounds!"));
        (void)hp_player(player_ptr, bmc_ptr->plev * 4);
        BadStatusSetter bss(player_ptr);
        (void)bss.stun(0);
        (void)bss.cut(0);
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
        return cast_blue_teleport_away(player_ptr, bmc_ptr);
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
    bmc_type *bmc_ptr = initialize_blue_magic_type(player_ptr, &tmp_bm, success, get_pseudo_monstetr_level);
    if (!switch_cast_blue_magic(player_ptr, bmc_ptr, spell)) {
        return false;
    }

    if (bmc_ptr->no_trump) {
        msg_print(_("何も現れなかった。", "No one appeared."));
    }

    return true;
}
