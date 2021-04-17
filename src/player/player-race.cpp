#include "player/player-race.h"
#include "core/player-redraw-types.h"
#include "inventory/inventory-slot-types.h"
#include "player/player-race-types.h"
#include "system/object-type-definition.h"
#include "mimic-info-table.h"
#include "player/race-info-table.h"

const player_race *rp_ptr;

/*!
 * @brief 救援召喚時のモンスターシンボルを返す
 * @param creature_ptr プレイヤー情報への参照ポインタ
 * @return シンボル文字
 */
SYMBOL_CODE get_summon_symbol_from_player(player_type *creature_ptr)
{
    SYMBOL_CODE symbol = 'N';
    const player_race *mmc_ptr;

    switch (creature_ptr->mimic_form) {
    case MIMIC_DEMON:
    case MIMIC_DEMON_LORD:
    case MIMIC_VAMPIRE:
        mmc_ptr = &mimic_info[creature_ptr->mimic_form];
        break;
    default: //MIMIC_NONE or undefined
        mmc_ptr = &race_info[creature_ptr->prace];
        break;
    }

    auto l = strlen(mmc_ptr->symbol);
    auto mul = 1;
    for (size_t i = 0; i < l; i++) {
        if (one_in_(mul))
            symbol = mmc_ptr->symbol[i];
        mul *= 13;
    }
    return symbol;
}

bool is_specific_player_race(player_type *creature_ptr, player_race_type prace) { return (!creature_ptr->mimic_form && (creature_ptr->prace == prace)); }
