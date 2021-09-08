#include "player-info/resistance-info.h"
#include "player-base/player-race.h"
#include "player-info/race-info.h"
#include "player-info/self-info-util.h"
#include "player/player-status-flags.h"
#include "status/element-resistance.h"
#include "system/player-type-definition.h"

void set_element_resistance_info(player_type *creature_ptr, self_info_type *self_ptr)
{
    if (has_immune_acid(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは酸に対する完全なる免疫を持っている。", "You are completely immune to acid.");
    else if (has_resist_acid(creature_ptr) && is_oppose_acid(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは酸への強力な耐性を持っている。", "You resist acid exceptionally well.");
    else if (has_resist_acid(creature_ptr) || is_oppose_acid(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは酸への耐性を持っている。", "You are resistant to acid.");

    if (player_race_has_flag(creature_ptr, TR_VUL_ACID) && !has_immune_acid(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは酸に弱い。", "You are susceptible to damage from acid.");

    if (has_immune_elec(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは電撃に対する完全なる免疫を持っている。", "You are completely immune to lightning.");
    else if (has_resist_elec(creature_ptr) && is_oppose_elec(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは電撃への強力な耐性を持っている。", "You resist lightning exceptionally well.");
    else if (has_resist_elec(creature_ptr) || is_oppose_elec(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは電撃への耐性を持っている。", "You are resistant to lightning.");

    if (player_race_has_flag(creature_ptr, TR_VUL_ELEC) && !has_immune_elec(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは電撃に弱い。", "You are susceptible to damage from lightning.");

    if (has_immune_fire(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは火に対する完全なる免疫を持っている。", "You are completely immune to fire.");
    else if (has_resist_fire(creature_ptr) && is_oppose_fire(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは火への強力な耐性を持っている。", "You resist fire exceptionally well.");
    else if (has_resist_fire(creature_ptr) || is_oppose_fire(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは火への耐性を持っている。", "You are resistant to fire.");

    if (player_race_has_flag(creature_ptr, TR_VUL_FIRE) && !has_immune_fire(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは火に弱い。", "You are susceptible to damage from fire.");

    if (has_immune_cold(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは冷気に対する完全なる免疫を持っている。", "You are completely immune to cold.");
    else if (has_resist_cold(creature_ptr) && is_oppose_cold(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは冷気への強力な耐性を持っている。", "You resist cold exceptionally well.");
    else if (has_resist_cold(creature_ptr) || is_oppose_cold(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは冷気への耐性を持っている。", "You are resistant to cold.");

    if (player_race_has_flag(creature_ptr, TR_VUL_COLD) && !has_immune_cold(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは冷気に弱い。", "You are susceptible to damage from cold.");

    if (has_resist_pois(creature_ptr) && is_oppose_pois(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは毒への強力な耐性を持っている。", "You resist poison exceptionally well.");
    else if (has_resist_pois(creature_ptr) || is_oppose_pois(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは毒への耐性を持っている。", "You are resistant to poison.");
}

void set_high_resistance_info(player_type *creature_ptr, self_info_type *self_ptr)
{
    if (has_resist_lite(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは閃光への耐性を持っている。", "You are resistant to bright light.");

    if (has_vuln_lite(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは閃光に弱い。", "You are susceptible to damage from bright light.");

    if (has_immune_dark(creature_ptr) || creature_ptr->wraith_form)
        self_ptr->info[self_ptr->line++] = _("あなたは暗黒に対する完全なる免疫を持っている。", "You are completely immune to darkness.");
    else if (has_resist_dark(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは暗黒への耐性を持っている。", "You are resistant to darkness.");

    if (has_resist_conf(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは混乱への耐性を持っている。", "You are resistant to confusion.");

    if (has_resist_sound(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは音波の衝撃への耐性を持っている。", "You are resistant to sonic attacks.");

    if (has_resist_disen(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは劣化への耐性を持っている。", "You are resistant to disenchantment.");

    if (has_resist_chaos(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたはカオスの力への耐性を持っている。", "You are resistant to chaos.");

    if (has_resist_shard(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは破片の攻撃への耐性を持っている。", "You are resistant to blasts of shards.");

    if (has_resist_nexus(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは因果混乱の攻撃への耐性を持っている。", "You are resistant to nexus attacks.");

    if (PlayerRace(creature_ptr).equals(player_race_type::SPECTRE))
        self_ptr->info[self_ptr->line++] = _("あなたは地獄の力を吸収できる。", "You can drain nether forces.");
    else if (has_resist_neth(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは地獄の力への耐性を持っている。", "You are resistant to nether forces.");
}
