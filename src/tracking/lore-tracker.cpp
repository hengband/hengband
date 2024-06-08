/*!
 * @brief サブウィンドウに表示するモンスター種族IDを保持するクラス
 * @author Hourier
 * @date 2024/06/08
 */

#include "tracking/lore-tracker.h"

LoreTracker LoreTracker::instance{};

LoreTracker &LoreTracker::get_instance()
{
    return instance;
}
