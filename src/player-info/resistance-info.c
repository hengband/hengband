#include "player-info/resistance-info.h"
#include "player-info/self-info-util.h"
#include "player/player-race.h"
#include "status/element-resistance.h"

void set_element_resistance_info(player_type* creature_ptr, self_info_type* si_ptr)
{
    if (creature_ptr->immune_acid) {
        si_ptr->info[si_ptr->line++] = _("あなたは酸に対する完全なる免疫を持っている。", "You are completely immune to acid.");
    } else if (creature_ptr->resist_acid && is_oppose_acid(creature_ptr)) {
        si_ptr->info[si_ptr->line++] = _("あなたは酸への強力な耐性を持っている。", "You resist acid exceptionally well.");
    } else if (creature_ptr->resist_acid || is_oppose_acid(creature_ptr)) {
        si_ptr->info[si_ptr->line++] = _("あなたは酸への耐性を持っている。", "You are resistant to acid.");
    }

    if (creature_ptr->immune_elec) {
        si_ptr->info[si_ptr->line++] = _("あなたは電撃に対する完全なる免疫を持っている。", "You are completely immune to lightning.");
    } else if (creature_ptr->resist_elec && is_oppose_elec(creature_ptr)) {
        si_ptr->info[si_ptr->line++] = _("あなたは電撃への強力な耐性を持っている。", "You resist lightning exceptionally well.");
    } else if (creature_ptr->resist_elec || is_oppose_elec(creature_ptr)) {
        si_ptr->info[si_ptr->line++] = _("あなたは電撃への耐性を持っている。", "You are resistant to lightning.");
    }

    if (is_specific_player_race(creature_ptr, RACE_ANDROID) && !creature_ptr->immune_elec) {
        si_ptr->info[si_ptr->line++] = _("あなたは電撃に弱い。", "You are susceptible to damage from lightning.");
    }

    if (creature_ptr->immune_fire) {
        si_ptr->info[si_ptr->line++] = _("あなたは火に対する完全なる免疫を持っている。", "You are completely immune to fire.");
    } else if (creature_ptr->resist_fire && is_oppose_fire(creature_ptr)) {
        si_ptr->info[si_ptr->line++] = _("あなたは火への強力な耐性を持っている。", "You resist fire exceptionally well.");
    } else if (creature_ptr->resist_fire || is_oppose_fire(creature_ptr)) {
        si_ptr->info[si_ptr->line++] = _("あなたは火への耐性を持っている。", "You are resistant to fire.");
    }

    if (is_specific_player_race(creature_ptr, RACE_ENT) && !creature_ptr->immune_fire) {
        si_ptr->info[si_ptr->line++] = _("あなたは火に弱い。", "You are susceptible to damage from fire.");
    }

    if (creature_ptr->immune_cold) {
        si_ptr->info[si_ptr->line++] = _("あなたは冷気に対する完全なる免疫を持っている。", "You are completely immune to cold.");
    } else if (creature_ptr->resist_cold && is_oppose_cold(creature_ptr)) {
        si_ptr->info[si_ptr->line++] = _("あなたは冷気への強力な耐性を持っている。", "You resist cold exceptionally well.");
    } else if (creature_ptr->resist_cold || is_oppose_cold(creature_ptr)) {
        si_ptr->info[si_ptr->line++] = _("あなたは冷気への耐性を持っている。", "You are resistant to cold.");
    }

    if (creature_ptr->resist_pois && is_oppose_pois(creature_ptr)) {
        si_ptr->info[si_ptr->line++] = _("あなたは毒への強力な耐性を持っている。", "You resist poison exceptionally well.");
    } else if (creature_ptr->resist_pois || is_oppose_pois(creature_ptr)) {
        si_ptr->info[si_ptr->line++] = _("あなたは毒への耐性を持っている。", "You are resistant to poison.");
    }
}

void set_high_resistance_info(player_type *creature_ptr, self_info_type *si_ptr)
{
    if (creature_ptr->resist_lite)
        si_ptr->info[si_ptr->line++] = _("あなたは閃光への耐性を持っている。", "You are resistant to bright light.");

    if (is_specific_player_race(creature_ptr, RACE_VAMPIRE) || is_specific_player_race(creature_ptr, RACE_S_FAIRY)
        || (creature_ptr->mimic_form == MIMIC_VAMPIRE))
        si_ptr->info[si_ptr->line++] = _("あなたは閃光に弱い。", "You are susceptible to damage from bright light.");

    if (is_specific_player_race(creature_ptr, RACE_VAMPIRE) || (creature_ptr->mimic_form == MIMIC_VAMPIRE) || creature_ptr->wraith_form)
        si_ptr->info[si_ptr->line++] = _("あなたは暗黒に対する完全なる免疫を持っている。", "You are completely immune to darkness.");
    else if (creature_ptr->resist_dark)
        si_ptr->info[si_ptr->line++] = _("あなたは暗黒への耐性を持っている。", "You are resistant to darkness.");
    
    if (creature_ptr->resist_conf)
        si_ptr->info[si_ptr->line++] = _("あなたは混乱への耐性を持っている。", "You are resistant to confusion.");
    
    if (creature_ptr->resist_sound)
        si_ptr->info[si_ptr->line++] = _("あなたは音波の衝撃への耐性を持っている。", "You are resistant to sonic attacks.");
    
    if (creature_ptr->resist_disen)
        si_ptr->info[si_ptr->line++] = _("あなたは劣化への耐性を持っている。", "You are resistant to disenchantment.");
    
    if (creature_ptr->resist_chaos)
        si_ptr->info[si_ptr->line++] = _("あなたはカオスの力への耐性を持っている。", "You are resistant to chaos.");
    
    if (creature_ptr->resist_shard)
        si_ptr->info[si_ptr->line++] = _("あなたは破片の攻撃への耐性を持っている。", "You are resistant to blasts of shards.");
    
    if (creature_ptr->resist_nexus)
        si_ptr->info[si_ptr->line++] = _("あなたは因果混乱の攻撃への耐性を持っている。", "You are resistant to nexus attacks.");

    if (is_specific_player_race(creature_ptr, RACE_SPECTRE))
        si_ptr->info[si_ptr->line++] = _("あなたは地獄の力を吸収できる。", "You can drain nether forces.");
    else if (creature_ptr->resist_neth)
        si_ptr->info[si_ptr->line++] = _("あなたは地獄の力への耐性を持っている。", "You are resistant to nether forces.");
}
