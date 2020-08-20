#include "spell-kind/spells-pet.h"
#include "core/asking-player.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "game-option/play-record-options.h"
#include "io/write-diary.h"
#include "monster-floor/monster-remover.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/smart-learn-types.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief ペット爆破処理 /
 * @return なし
 */
void discharge_minion(player_type *caster_ptr)
{
    bool okay = TRUE;
    for (MONSTER_IDX i = 1; i < caster_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
        if (!m_ptr->r_idx || !is_pet(m_ptr))
            continue;
        if (m_ptr->nickname)
            okay = FALSE;
    }

    if (!okay || caster_ptr->riding) {
        if (!get_check(_("本当に全ペットを爆破しますか？", "You will blast all pets. Are you sure? ")))
            return;
    }

    for (MONSTER_IDX i = 1; i < caster_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
        if (!m_ptr->r_idx || !is_pet(m_ptr))
            continue;

        monster_race *r_ptr;
        r_ptr = &r_info[m_ptr->r_idx];
        if (r_ptr->flags1 & RF1_UNIQUE) {
            GAME_TEXT m_name[MAX_NLEN];
            monster_desc(caster_ptr, m_name, m_ptr, 0x00);
            msg_format(_("%sは爆破されるのを嫌がり、勝手に自分の世界へと帰った。", "%^s resists being blasted and runs away."), m_name);
            delete_monster_idx(caster_ptr, i);
            continue;
        }

        HIT_POINT dam = m_ptr->maxhp / 2;
        if (dam > 100)
            dam = (dam - 100) / 2 + 100;
        if (dam > 400)
            dam = (dam - 400) / 2 + 400;
        if (dam > 800)
            dam = 800;
        project(caster_ptr, i, 2 + (r_ptr->level / 20), m_ptr->fy, m_ptr->fx, dam, GF_PLASMA, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL, -1);

        if (record_named_pet && m_ptr->nickname) {
            GAME_TEXT m_name[MAX_NLEN];

            monster_desc(caster_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
            exe_write_diary(caster_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_BLAST, m_name);
        }

        delete_monster_idx(caster_ptr, i);
    }
}
