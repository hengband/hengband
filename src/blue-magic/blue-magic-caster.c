#include "blue-magic/blue-magic-caster.h"
#include "blue-magic/blue-magic-ball-bolt.h"
#include "blue-magic/blue-magic-breath.h"
#include "blue-magic/blue-magic-spirit-curse.h"
#include "blue-magic/blue-magic-status.h"
#include "blue-magic/blue-magic-summon.h"
#include "blue-magic/blue-magic-util.h"
#include "blue-magic/learnt-info.h"
#include "core/hp-mp-processor.h"
#include "floor/cave.h"
#include "grid/grid.h"
#include "monster-race/monster-race.h"
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
#include "spell/spell-types.h"
#include "spell/spells-status.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "system/floor-type-definition.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "view/display-messages.h"

static bool cast_blue_dispel(player_type *caster_ptr)
{
    if (!target_set(caster_ptr, TARGET_KILL))
        return FALSE;

    MONSTER_IDX m_idx = caster_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx;
    if ((m_idx == 0) || !player_has_los_bold(caster_ptr, target_row, target_col)
        || !projectable(caster_ptr, caster_ptr->y, caster_ptr->x, target_row, target_col))
        return TRUE;

    dispel_monster_status(caster_ptr, m_idx);
    return TRUE;
}

static bool cast_blue_rocket(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("ロケットを発射した。", "You fire a rocket."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_ROCKET), bmc_ptr->plev, DAM_ROLL);
    fire_rocket(caster_ptr, GF_ROCKET, bmc_ptr->dir, bmc_ptr->damage, 2);
    return TRUE;
}

static bool cast_blue_shoot(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("矢を放った。", "You fire an arrow."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_SHOOT), bmc_ptr->plev, DAM_ROLL);
    fire_bolt(caster_ptr, GF_ARROW, bmc_ptr->dir, bmc_ptr->damage);
    return TRUE;
}

static bool cast_blue_hand_doom(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("<破滅の手>を放った！", "You invoke the Hand of Doom!"));
    fire_ball_hide(caster_ptr, GF_HAND_DOOM, bmc_ptr->dir, bmc_ptr->plev * 3, 0);
    return TRUE;
}

static bool exe_blue_teleport_back(player_type *caster_ptr, GAME_TEXT *m_name)
{
    monster_type *m_ptr;
    monster_race *r_ptr;
    floor_type *floor_ptr = caster_ptr->current_floor_ptr;
    if ((floor_ptr->grid_array[target_row][target_col].m_idx == 0) || !player_has_los_bold(caster_ptr, target_row, target_col)
        || !projectable(caster_ptr, caster_ptr->y, caster_ptr->x, target_row, target_col))
        return TRUE;

    m_ptr = &floor_ptr->m_list[floor_ptr->grid_array[target_row][target_col].m_idx];
    r_ptr = &r_info[m_ptr->r_idx];
    monster_desc(caster_ptr, m_name, m_ptr, 0);
    if ((r_ptr->flagsr & RFR_RES_TELE) == 0)
        return FALSE;

    if ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flagsr & RFR_RES_ALL)) {
        if (is_original_ap_and_seen(caster_ptr, m_ptr))
            r_ptr->r_flagsr |= RFR_RES_TELE;

        msg_format(_("%sには効果がなかった！", "%s is unaffected!"), m_name);
        return TRUE;
    }
    
    if (r_ptr->level <= randint1(100))
        return FALSE;

    if (is_original_ap_and_seen(caster_ptr, m_ptr))
        r_ptr->r_flagsr |= RFR_RES_TELE;

    msg_format(_("%sには耐性がある！", "%s resists!"), m_name);
    return TRUE;
}

static bool cast_blue_teleport_back(player_type *caster_ptr)
{
    if (!target_set(caster_ptr, TARGET_KILL))
        return FALSE;

    GAME_TEXT m_name[MAX_NLEN];
    if (exe_blue_teleport_back(caster_ptr, m_name))
        return TRUE;

    msg_format(_("%sを引き戻した。", "You command %s to return."), m_name);
    teleport_monster_to(
        caster_ptr, caster_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx, caster_ptr->y, caster_ptr->x, 100, TELEPORT_PASSIVE);
    return TRUE;
}

static bool cast_blue_teleport_away(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    (void)fire_beam(caster_ptr, GF_AWAY_ALL, bmc_ptr->dir, 100);
    return TRUE;
}

static bool cast_blue_psy_spear(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("光の剣を放った。", "You throw a psycho-spear."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_PSY_SPEAR), bmc_ptr->plev, DAM_ROLL);
    (void)fire_beam(caster_ptr, GF_PSY_SPEAR, bmc_ptr->dir, bmc_ptr->damage);
    return TRUE;
}

static bool cast_blue_make_trap(player_type *caster_ptr)
{
    if (!target_set(caster_ptr, TARGET_KILL))
        return FALSE;

    msg_print(_("呪文を唱えて邪悪に微笑んだ。", "You cast a spell and cackle evilly."));
    trap_creation(caster_ptr, target_row, target_col);
    return TRUE;
}

bool switch_cast_blue_magic(player_type *caster_ptr, bmc_type *bmc_ptr, SPELL_IDX spell)
{
    switch (spell) {
    case MS_SHRIEK:
        msg_print(_("かん高い金切り声をあげた。", "You make a high pitched shriek."));
        aggravate_monsters(caster_ptr, 0);
        return TRUE;
    case MS_XXX1:
    case MS_XXX2:
    case MS_XXX3:
    case MS_XXX4:
        return TRUE;
    case MS_DISPEL:
        return cast_blue_dispel(caster_ptr);
    case MS_ROCKET:
        return cast_blue_rocket(caster_ptr, bmc_ptr);
    case MS_SHOOT:
        return cast_blue_shoot(caster_ptr, bmc_ptr);
    case MS_BR_ACID:
        return cast_blue_breath_acid(caster_ptr, bmc_ptr);
    case MS_BR_ELEC:
        return cast_blue_breath_elec(caster_ptr, bmc_ptr);
    case MS_BR_FIRE:
        return cast_blue_breath_fire(caster_ptr, bmc_ptr);
    case MS_BR_COLD:
        return cast_blue_breath_cold(caster_ptr, bmc_ptr);
    case MS_BR_POIS:
        return cast_blue_breath_pois(caster_ptr, bmc_ptr);
    case MS_BR_NETHER:
        return cast_blue_breath_nether(caster_ptr, bmc_ptr);
    case MS_BR_LITE:
        return cast_blue_breath_lite(caster_ptr, bmc_ptr);
    case MS_BR_DARK:
        return cast_blue_breath_dark(caster_ptr, bmc_ptr);
    case MS_BR_CONF:
        return cast_blue_breath_conf(caster_ptr, bmc_ptr);
    case MS_BR_SOUND:
        return cast_blue_breath_sound(caster_ptr, bmc_ptr);
    case MS_BR_CHAOS:
        return cast_blue_breath_chaos(caster_ptr, bmc_ptr);
    case MS_BR_DISEN:
        return cast_blue_breath_disenchant(caster_ptr, bmc_ptr);
    case MS_BR_NEXUS:
        return cast_blue_breath_nexus(caster_ptr, bmc_ptr);
    case MS_BR_TIME:
        return cast_blue_breath_time(caster_ptr, bmc_ptr);
    case MS_BR_INERTIA:
        return cast_blue_breath_inertia(caster_ptr, bmc_ptr);
    case MS_BR_GRAVITY:
        return cast_blue_breath_gravity(caster_ptr, bmc_ptr);
    case MS_BR_SHARDS:
        return cast_blue_breath_shards(caster_ptr, bmc_ptr);
    case MS_BR_PLASMA:
        return cast_blue_breath_plasma(caster_ptr, bmc_ptr);
    case MS_BR_FORCE:
        return cast_blue_breath_force(caster_ptr, bmc_ptr);
    case MS_BR_MANA:
        return cast_blue_breath_mana(caster_ptr, bmc_ptr);
    case MS_BR_NUKE:
        return cast_blue_breath_nuke(caster_ptr, bmc_ptr);
    case MS_BR_DISI:
        return cast_blue_breath_disintegration(caster_ptr, bmc_ptr);
    case MS_BALL_ACID:
        return cast_blue_ball_acid(caster_ptr, bmc_ptr);
    case MS_BALL_ELEC:
        return cast_blue_ball_elec(caster_ptr, bmc_ptr);
    case MS_BALL_FIRE:
        return cast_blue_ball_fire(caster_ptr, bmc_ptr);
    case MS_BALL_COLD:
        return cast_blue_ball_cold(caster_ptr, bmc_ptr);
    case MS_BALL_POIS:
        return cast_blue_ball_pois(caster_ptr, bmc_ptr);
    case MS_BALL_NUKE:
        return cast_blue_ball_nuke(caster_ptr, bmc_ptr);
    case MS_BALL_NETHER:
        return cast_blue_ball_nether(caster_ptr, bmc_ptr);
    case MS_BALL_CHAOS:
        return cast_blue_ball_chaos(caster_ptr, bmc_ptr);
    case MS_BALL_WATER:
        return cast_blue_ball_water(caster_ptr, bmc_ptr);
    case MS_STARBURST:
        return cast_blue_ball_star_burst(caster_ptr, bmc_ptr);
    case MS_BALL_DARK:
        return cast_blue_ball_dark_storm(caster_ptr, bmc_ptr);
    case MS_BALL_MANA:
        return cast_blue_ball_mana_storm(caster_ptr, bmc_ptr);
    case MS_DRAIN_MANA:
        return cast_blue_drain_mana(caster_ptr, bmc_ptr);
    case MS_MIND_BLAST:
        return cast_blue_mind_blast(caster_ptr, bmc_ptr);
    case MS_BRAIN_SMASH:
        return cast_blue_brain_smash(caster_ptr, bmc_ptr);
    case MS_CAUSE_1:
        return cast_blue_curse_1(caster_ptr, bmc_ptr);
    case MS_CAUSE_2:
        return cast_blue_curse_2(caster_ptr, bmc_ptr);
    case MS_CAUSE_3:
        return cast_blue_curse_3(caster_ptr, bmc_ptr);
    case MS_CAUSE_4:
        return cast_blue_curse_4(caster_ptr, bmc_ptr);
    case MS_BOLT_ACID:
        return cast_blue_bolt_acid(caster_ptr, bmc_ptr);
    case MS_BOLT_ELEC:
        return cast_blue_bolt_elec(caster_ptr, bmc_ptr);
    case MS_BOLT_FIRE:
        return !cast_blue_bolt_fire(caster_ptr, bmc_ptr);
    case MS_BOLT_COLD:
        return cast_blue_bolt_cold(caster_ptr, bmc_ptr);
    case MS_BOLT_NETHER:
        return cast_blue_bolt_nether(caster_ptr, bmc_ptr);
    case MS_BOLT_WATER:
        return cast_blue_bolt_water(caster_ptr, bmc_ptr);
    case MS_BOLT_MANA:
        return cast_blue_bolt_mana(caster_ptr, bmc_ptr);
    case MS_BOLT_PLASMA:
        return cast_blue_bolt_plasma(caster_ptr, bmc_ptr);
    case MS_BOLT_ICE:
        return cast_blue_bolt_icee(caster_ptr, bmc_ptr);
    case MS_MAGIC_MISSILE:
        return cast_blue_bolt_missile(caster_ptr, bmc_ptr);
    case MS_SCARE:
        return cast_blue_scare(caster_ptr, bmc_ptr);
    case MS_BLIND:
        return cast_blue_blind(caster_ptr, bmc_ptr);
    case MS_CONF:
        return cast_blue_confusion(caster_ptr, bmc_ptr);
    case MS_SLOW:
        return cast_blue_slow(caster_ptr, bmc_ptr);
    case MS_SLEEP:
        return cast_blue_sleep(caster_ptr, bmc_ptr);
    case MS_SPEED:
        (void)set_fast(caster_ptr, randint1(20 + bmc_ptr->plev) + bmc_ptr->plev, FALSE);
        return TRUE;
    case MS_HAND_DOOM:
        return cast_blue_hand_doom(caster_ptr, bmc_ptr);
    case MS_HEAL:
        msg_print(_("自分の傷に念を集中した。", "You concentrate on your wounds!"));
        (void)hp_player(caster_ptr, bmc_ptr->plev * 4);
        (void)set_stun(caster_ptr, 0);
        (void)set_cut(caster_ptr, 0);
        return TRUE;
    case MS_INVULNER:
        msg_print(_("無傷の球の呪文を唱えた。", "You cast a Globe of Invulnerability."));
        (void)set_invuln(caster_ptr, randint1(4) + 4, FALSE);
        return TRUE;
    case MS_BLINK:
        teleport_player(caster_ptr, 10, TELEPORT_SPONTANEOUS);
        return TRUE;
    case MS_TELEPORT:
        teleport_player(caster_ptr, bmc_ptr->plev * 5, TELEPORT_SPONTANEOUS);
        return TRUE;
    case MS_WORLD:
        (void)time_walk(caster_ptr);
        return TRUE;
    case MS_SPECIAL:
        return TRUE;
    case MS_TELE_TO:
        return cast_blue_teleport_back(caster_ptr);
    case MS_TELE_AWAY:
        return cast_blue_teleport_away(caster_ptr, bmc_ptr);
    case MS_TELE_LEVEL:
        return teleport_level_other(caster_ptr);
    case MS_PSY_SPEAR:
        return cast_blue_psy_spear(caster_ptr, bmc_ptr);
    case MS_DARKNESS:
        msg_print(_("暗闇の中で手を振った。", "You gesture in shadow."));
        (void)unlite_area(caster_ptr, 10, 3);
        return TRUE;
    case MS_MAKE_TRAP:
        return cast_blue_make_trap(caster_ptr);
    case MS_FORGET:
        msg_print(_("しかし何も起きなかった。", "Nothing happen."));
        return TRUE;
    case MS_RAISE_DEAD:
        msg_print(_("死者復活の呪文を唱えた。", "You animate the dead."));
        (void)animate_dead(caster_ptr, 0, caster_ptr->y, caster_ptr->x);
        return TRUE;
    case MS_S_KIN:
        return cast_blue_summon_kin(caster_ptr, bmc_ptr);
    case MS_S_CYBER:
        return cast_blue_summon_cyber(caster_ptr, bmc_ptr);
    case MS_S_MONSTER:
        return cast_blue_summon_monster(caster_ptr, bmc_ptr);
    case MS_S_MONSTERS:
        return cast_blue_summon_monsters(caster_ptr, bmc_ptr);
    case MS_S_ANT:
        return cast_blue_summon_ant(caster_ptr, bmc_ptr);
    case MS_S_SPIDER:
        return cast_blue_summon_spider(caster_ptr, bmc_ptr);
    case MS_S_HOUND:
        return cast_blue_summon_hound(caster_ptr, bmc_ptr);
    case MS_S_HYDRA:
        return cast_blue_summon_hydra(caster_ptr, bmc_ptr);
    case MS_S_ANGEL:
        return cast_blue_summon_angel(caster_ptr, bmc_ptr);
    case MS_S_DEMON:
        return cast_blue_summon_demon(caster_ptr, bmc_ptr);
    case MS_S_UNDEAD:
        return cast_blue_summon_undead(caster_ptr, bmc_ptr);
    case MS_S_DRAGON:
        return cast_blue_summon_dragon(caster_ptr, bmc_ptr);
    case MS_S_HI_UNDEAD:
        return cast_blue_summon_high_undead(caster_ptr, bmc_ptr);
    case MS_S_HI_DRAGON:
        return cast_blue_summon_high_dragon(caster_ptr, bmc_ptr);
    case MS_S_AMBERITE:
        return cast_blue_summon_amberite(caster_ptr, bmc_ptr);
    case MS_S_UNIQUE:
        return cast_blue_summon_unique(caster_ptr, bmc_ptr);
    default:
        msg_print("hoge?");
        return TRUE;
    }
}

/*!
 * @brief 青魔法の発動 /
 * do_cmd_cast calls this function if the player's class is 'blue-mage'.
 * @param spell 発動するモンスター攻撃のID
 * @param success TRUEは成功時、FALSEは失敗時の処理を行う
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
bool cast_learned_spell(player_type *caster_ptr, SPELL_IDX spell, const bool success)
{
    bmc_type tmp_bm;
    bmc_type *bmc_ptr = initialize_blue_magic_type(caster_ptr, &tmp_bm, success, get_pseudo_monstetr_level);
    if (switch_cast_blue_magic(caster_ptr, bmc_ptr, spell))
        return FALSE;

    if (bmc_ptr->no_trump)
        msg_print(_("何も現れなかった。", "No one appeared."));

    return TRUE;
}
