#include "world/town-info-service.h"
#include "game-option/birth-options.h"
#include "system/floor/town-info.h"
#include "system/floor/town-list.h"

/*!
 * @brief 町名の上書きを行う
 *
 * 荒野なしオプションがONの時、辺境の地を「街」と差し替える.
 * 辺境の地にのみ別名が定義されており、辺境の地の街番号を直接指定するのはサービスクラスでのみ行うべきである.
 */
void TownInfoService::overwrite_town_name()
{
    if (vanilla_town || lite_town) {
        TownList::get_instance().get_town(1).overwrite_town_name();
    }
}
