#include "effect/effect-monster-lite-dark.h"
#include "effect/effect-monster-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-info.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"

ProcessResult effect_monster_lite_weak(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    if (!em_ptr->dam) {
        em_ptr->skipped = true;
        return ProcessResult::PROCESS_CONTINUE;
    }

    if (em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::HURT_LITE)) {
        em_ptr->dam = 0;
        return ProcessResult::PROCESS_CONTINUE;
    }

    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::HURT_LITE);
    }

    em_ptr->note = _("は光に身をすくめた！", " cringes from the light!");
    em_ptr->note_dies = _("は光を受けてしぼんでしまった！", " shrivels away in the light!");
    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_lite(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_LITE)) {
        em_ptr->note = _("には耐性がある！", " resists!");
        em_ptr->dam *= 2;
        em_ptr->dam /= (randint1(6) + 6);
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_LITE);
        }
    } else if (em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::HURT_LITE)) {
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::HURT_LITE);
        }

        em_ptr->note = _("は光に身をすくめた！", " cringes from the light!");
        em_ptr->note_dies = _("は光を受けてしぼんでしまった！", " shrivels away in the light!");
        em_ptr->dam *= 2;
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_dark(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::RESIST_DARK)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある！", " resists!");
    em_ptr->dam *= 2;
    em_ptr->dam /= (randint1(6) + 6);
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_DARK);
    }

    return ProcessResult::PROCESS_CONTINUE;
}
