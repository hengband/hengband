﻿#include "view/display-monster-status.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/smart-learn-types.h"

/*
 * Monster health description
 */
concptr look_mon_desc(monster_type *m_ptr, BIT_FLAGS mode)
{
    bool living = monster_living(m_ptr->ap_r_idx);
    int perc = m_ptr->maxhp > 0 ? 100L * m_ptr->hp / m_ptr->maxhp : 0;

    concptr desc;
    if (m_ptr->hp >= m_ptr->maxhp) {
        desc = living ? _("無傷", "unhurt") : _("無ダメージ", "undamaged");
    } else if (perc >= 60) {
        desc = living ? _("軽傷", "somewhat wounded") : _("小ダメージ", "somewhat damaged");
    } else if (perc >= 25) {
        desc = living ? _("負傷", "wounded") : _("中ダメージ", "damaged");
    } else if (perc >= 10) {
        desc = living ? _("重傷", "badly wounded") : _("大ダメージ", "badly damaged");
    } else {
        desc = living ? _("半死半生", "almost dead") : _("倒れかけ", "almost destroyed");
    }

    concptr attitude;
    if (!(mode & 0x01)) {
        attitude = "";
    } else if (is_pet(m_ptr)) {
        attitude = _(", ペット", ", pet");
    } else if (is_friendly(m_ptr)) {
        attitude = _(", 友好的", ", friendly");
    } else {
        attitude = _("", "");
    }

    concptr clone = (m_ptr->smart & SM_CLONED) ? ", clone" : "";
    monster_race *ap_r_ptr = &r_info[m_ptr->ap_r_idx];
    if (ap_r_ptr->r_tkills && !(m_ptr->mflag2 & MFLAG2_KAGE)) {
        return format(_("レベル%d, %s%s%s", "Level %d, %s%s%s"), ap_r_ptr->level, desc, attitude, clone);
    }

    return format(_("レベル???, %s%s%s", "Level ???, %s%s%s"), desc, attitude, clone);
}
