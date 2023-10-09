#include "view/display-monster-status.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/smart-learn-types.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"

/*
 * Monster health description
 */
std::string look_mon_desc(MonsterEntity *m_ptr, BIT_FLAGS mode)
{
    auto living = m_ptr->has_living_flag(true);
    auto perc = m_ptr->maxhp > 0 ? 100L * m_ptr->hp / m_ptr->maxhp : 0;

    concptr desc;
    if (!m_ptr->ml) {
        desc = _("損傷具合不明", "damage unknown");
    } else if (m_ptr->hp >= m_ptr->maxhp) {
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
    } else if (m_ptr->is_pet()) {
        attitude = _(", ペット", ", pet");
    } else if (m_ptr->is_friendly()) {
        attitude = _(", 友好的", ", friendly");
    } else {
        attitude = _("", "");
    }

    concptr clone = m_ptr->mflag2.has(MonsterConstantFlagType::CLONED) ? ", clone" : "";
    MonsterRaceInfo *ap_r_ptr = &m_ptr->get_real_monrace();
    if (ap_r_ptr->r_tkills && m_ptr->mflag2.has_not(MonsterConstantFlagType::KAGE)) {
        return format(_("レベル%d, %s%s%s", "Level %d, %s%s%s"), ap_r_ptr->level, desc, attitude, clone);
    }

    return format(_("レベル???, %s%s%s", "Level ???, %s%s%s"), desc, attitude, clone);
}
