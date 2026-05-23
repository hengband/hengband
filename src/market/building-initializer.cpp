#include "market/building-initializer.h"
#include "player-info/class-types.h"
#include "player-info/race-types.h"
#include "realm/realm-types.h"
#include "system/building-type-definition.h"

/*!
 * @brief 店情報初期化のメインルーチン
 */
void init_buildings()
{
    for (auto i = 0; i < MAX_BUILDINGS; i++) {
        auto &building = buildings[i];
        building.name[0] = '\0';
        building.owner_name[0] = '\0';
        building.owner_race[0] = '\0';
        for (auto j = 0; j < 8; j++) {
            building.act_names[j][0] = '\0';
            building.member_costs[j] = 0;
            building.other_costs[j] = 0;
            building.letters[j] = 0;
            building.actions[j] = 0;
            building.action_restr[j] = 0;
        }

        building.member_class.assign(PLAYER_CLASS_TYPE_MAX, static_cast<short>(PlayerClassType::WARRIOR));
        building.member_race.assign(MAX_RACES, static_cast<short>(PlayerRaceType::HUMAN));
        building.member_realm.assign(MAX_MAGIC + 1, 0);
    }
}
