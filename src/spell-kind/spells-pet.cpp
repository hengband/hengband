#include "spell-kind/spells-pet.h"
#include "core/asking-player.h"
#include "effect/attribute-types.h"
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
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief ペット爆破処理 /
 */
void discharge_minion(PlayerType *player_ptr)
{
    bool okay = true;
    for (MONSTER_IDX i = 1; i < player_ptr->current_floor_ptr->m_max; i++) {
        auto *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
        if (!MonsterRace(m_ptr->r_idx).is_valid() || !m_ptr->is_pet()) {
            continue;
        }
        if (m_ptr->is_named()) {
            okay = false;
        }
    }

    if (!okay || player_ptr->riding) {
        if (!input_check(_("本当に全ペットを爆破しますか？", "You will blast all pets. Are you sure? "))) {
            return;
        }
    }

    for (MONSTER_IDX i = 1; i < player_ptr->current_floor_ptr->m_max; i++) {
        auto *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
        if (!MonsterRace(m_ptr->r_idx).is_valid() || !m_ptr->is_pet()) {
            continue;
        }

        MonsterRaceInfo *r_ptr;
        r_ptr = &m_ptr->get_monrace();
        if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
            const auto m_name = monster_desc(player_ptr, m_ptr, 0x00);
            msg_format(_("%sは爆破されるのを嫌がり、勝手に自分の世界へと帰った。", "%s^ resists being blasted and runs away."), m_name.data());
            delete_monster_idx(player_ptr, i);
            continue;
        }

        int dam = m_ptr->maxhp / 2;
        if (dam > 100) {
            dam = (dam - 100) / 2 + 100;
        }
        if (dam > 400) {
            dam = (dam - 400) / 2 + 400;
        }
        if (dam > 800) {
            dam = 800;
        }
        project(player_ptr, i, 2 + (r_ptr->level / 20), m_ptr->fy, m_ptr->fx, dam, AttributeType::PLASMA, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);

        if (record_named_pet && m_ptr->is_named()) {
            const auto m_name = monster_desc(player_ptr, m_ptr, MD_INDEF_VISIBLE);
            exe_write_diary(player_ptr, DiaryKind::NAMED_PET, RECORD_NAMED_PET_BLAST, m_name);
        }

        delete_monster_idx(player_ptr, i);
    }
}
