#include "dungeon/quest-monster-placer.h"
#include "dungeon/quest.h"
#include "floor/floor-generator-util.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-info.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief クエストに関わるモンスターの配置を行う / Place quest monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 成功したならばTRUEを返す
 */
bool place_quest_monsters(PlayerType *player_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &quests = QuestList::get_instance();
    for (const auto &[quest_id, quest] : quests) {
        auto no_quest_monsters = quest.status != QuestStatusType::TAKEN;
        no_quest_monsters |= (quest.type != QuestKindType::KILL_LEVEL && quest.type != QuestKindType::RANDOM);
        no_quest_monsters |= quest.level != floor.dun_level;
        no_quest_monsters |= floor.dungeon_id != quest.dungeon;
        no_quest_monsters |= any_bits(quest.flags, QUEST_FLAG_PRESET);

        if (no_quest_monsters) {
            continue;
        }

        const auto &monrace = quest.get_bounty();
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE) && (monrace.cur_num >= monrace.max_num)) {
            continue;
        }

        auto mode = PM_NO_KAGE | PM_NO_PET;
        if (monrace.misc_flags.has_not(MonsterMiscType::HAS_FRIENDS)) {
            mode |= PM_ALLOW_GROUP;
        }

        for (int j = 0; j < (quest.max_num - quest.cur_num); j++) {
            int k;
            for (k = 0; k < SAFE_MAX_ATTEMPTS; k++) {
                Pos2D pos(0, 0);
                int l;
                for (l = SAFE_MAX_ATTEMPTS; l > 0; l--) {
                    pos = Pos2D(randint0(floor.height), randint0(floor.width));
                    const auto &grid = floor.get_grid(pos);
                    const auto &terrain = grid.get_terrain();
                    if (terrain.flags.has_none_of({ TerrainCharacteristics::MOVE, TerrainCharacteristics::CAN_FLY })) {
                        continue;
                    }

                    if (!monster_can_enter(player_ptr, pos.y, pos.x, &monrace, 0)) {
                        continue;
                    }

                    if (Grid::calc_distance(pos, player_ptr->get_position()) < 10) {
                        continue;
                    }

                    if (grid.is_icky()) {
                        continue;
                    } else {
                        break;
                    }
                }

                if (l == 0) {
                    return false;
                }

                if (place_specific_monster(player_ptr, pos.y, pos.x, quest.r_idx, mode)) {
                    break;
                }

                continue;
            }

            if (k == SAFE_MAX_ATTEMPTS) {
                return false;
            }
        }
    }

    return true;
}
