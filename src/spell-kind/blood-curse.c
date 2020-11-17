#include "spell-kind/blood-curse.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "grid/grid.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spell-types.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "status/base-status.h"
#include "status/experience.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "view/display-messages.h"

void blood_curse_to_enemy(player_type *caster_ptr, MONSTER_IDX m_idx)
{
    monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[m_idx];
    grid_type *g_ptr = &caster_ptr->current_floor_ptr->grid_array[m_ptr->fy][m_ptr->fx];
    BIT_FLAGS curse_flg = (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP);
    int count = 0;
    bool is_first_loop = TRUE;
    while (is_first_loop || one_in_(5)) {
        is_first_loop = FALSE;
        switch (randint1(28)) {
        case 1:
        case 2:
            if (!count) {
                msg_print(_("地面が揺れた...", "The ground trembles..."));
                earthquake(caster_ptr, m_ptr->fy, m_ptr->fx, 4 + randint0(4), 0);
                if (!one_in_(6))
                    break;
            }
            /* Fall through */
        case 3:
        case 4:
        case 5:
        case 6:
            if (!count) {
                int extra_dam = damroll(10, 10);
                msg_print(_("純粋な魔力の次元への扉が開いた！", "A portal opens to a plane of raw mana!"));
                project(caster_ptr, 0, 8, m_ptr->fy, m_ptr->fx, extra_dam, GF_MANA, curse_flg, -1);
                if (!one_in_(6))
                    break;
            }
            /* Fall through */
        case 7:
        case 8:
            if (!count) {
                msg_print(_("空間が歪んだ！", "Space warps about you!"));
                if (m_ptr->r_idx)
                    teleport_away(caster_ptr, g_ptr->m_idx, damroll(10, 10), TELEPORT_PASSIVE);
                if (one_in_(13))
                    count += activate_hi_summon(caster_ptr, m_ptr->fy, m_ptr->fx, TRUE);
                if (!one_in_(6))
                    break;
            }
            /* Fall through */
        case 9:
        case 10:
        case 11:
            msg_print(_("エネルギーのうねりを感じた！", "You feel a surge of energy!"));
            project(caster_ptr, 0, 7, m_ptr->fy, m_ptr->fx, 50, GF_DISINTEGRATE, curse_flg, -1);
            if (!one_in_(6))
                break;
            /* Fall through */
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
            aggravate_monsters(caster_ptr, 0);
            if (!one_in_(6))
                break;
            /* Fall through */
        case 17:
        case 18:
            count += activate_hi_summon(caster_ptr, m_ptr->fy, m_ptr->fx, TRUE);
            if (!one_in_(6))
                break;
            /* Fall through */
        case 19:
        case 20:
        case 21:
        case 22: {
            bool pet = !one_in_(3);
            BIT_FLAGS mode = PM_ALLOW_GROUP;

            if (pet)
                mode |= PM_FORCE_PET;
            else
                mode |= (PM_NO_PET | PM_FORCE_FRIENDLY);

            count += summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x,
                (pet ? caster_ptr->lev * 2 / 3 + randint1(caster_ptr->lev / 2) : caster_ptr->current_floor_ptr->dun_level), 0, mode);
            if (!one_in_(6))
                break;
        }
            /* Fall through */
        case 23:
        case 24:
        case 25:
            if (caster_ptr->hold_exp && (randint0(100) < 75))
                break;

            msg_print(_("経験値が体から吸い取られた気がする！", "You feel your experience draining away..."));
            if (caster_ptr->hold_exp)
                lose_exp(caster_ptr, caster_ptr->exp / 160);
            else
                lose_exp(caster_ptr, caster_ptr->exp / 16);
            if (!one_in_(6))
                break;
            /* Fall through */
        case 26:
        case 27:
        case 28: {
            if (one_in_(13)) {
                for (int i = 0; i < A_MAX; i++) {
                    bool is_first_dec_stat = TRUE;
                    while (is_first_dec_stat || one_in_(2)) {
                        (void)do_dec_stat(caster_ptr, i);
                    }
                }
            } else {
                (void)do_dec_stat(caster_ptr, randint0(6));
            }

            break;
        }
        }
    }
}
