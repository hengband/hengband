#include "player-info/race-info.h"
#include "core/player-redraw-types.h"
#include "inventory/inventory-slot-types.h"
#include "player-base/player-race.h"
#include "player-info/mimic-info-table.h"
#include "player-info/race-types.h"
#include "player/race-info-table.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"

const player_race_info *rp_ptr;

/*!
 * @brief 救援召喚時のモンスターシンボルを返す
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @return シンボル文字
 */
SYMBOL_CODE get_summon_symbol_from_player(player_type *player_ptr)
{
    SYMBOL_CODE symbol = 'N';
    auto mmc_ptr = PlayerRace(player_ptr).get_info();

    auto l = strlen(mmc_ptr->symbol);
    auto mul = 1;
    for (size_t i = 0; i < l; i++) {
        if (one_in_(mul))
            symbol = mmc_ptr->symbol[i];
        mul *= 13;
    }
    return symbol;
}
