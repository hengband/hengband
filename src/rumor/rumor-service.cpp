#include "rumor/rumor-service.h"
#include "game-option/birth-options.h"
#include "io/files-util.h"
#include "rumor/rumor-definition.h"
#include "rumor/rumor-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/monrace/monrace-list.h"
#include "util/angband-files.h"
#include "world/world.h"

/*!
 * @brief 噂に関する初期化のファサード
 */
void RumorService::initialize()
{
    auto &rumors = RumorList::get_instance();
    const auto path = path_build(ANGBAND_DIR_FILE, _("rumors_j.txt", "rumors.txt"));
    rumors.read_rumors(path);
}

/*!
 * @brief 噂に関する初期後処理のファサード
 */
void RumorService::retouch()
{
    auto &rumors = RumorList::get_instance();
    rumors.add_towns();
    rumors.add_shallow_dungeons();
    rumors.add_normal_monsters();
    rumors.add_shallow_artifacts();
    rumors.add_deep_dungeons();
    rumors.add_unique_monsters();
    rumors.add_deep_artifacts();
    rumors.validate();
    rumors.make_table();
}

const RumorDefinition &RumorService::pick_rumor(tl::optional<RumorRarity> rarity)
{
    const auto &rumors = RumorList::get_instance();
    while (true) {
        const auto &rumor = rarity ? rumors.select_rumor(*rarity) : rumors.select_random_rumor();
        const auto rumor_type = rumor.get_type();
        if ((rumor_type == RumorType::TOWN) && (lite_town || vanilla_town)) {
            continue;
        }

        const auto &world = AngbandWorld::get_instance();
        if (!world.total_winner && (rumor_type == RumorType::DEEP_DUNGEON)) {
            const auto dungeon_id = std::get<DungeonId>(rumor.get_id());
            if ((dungeon_id == DungeonId::HEAVEN) || (dungeon_id == DungeonId::HELL)) {
                continue;
            }
        }

        return rumor;
    }
}
