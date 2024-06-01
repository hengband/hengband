/*!
 * @brief 内部ゲームデータ定義
 * @author Hourier
 * @date 2024/06/01
 */

#include "system/inner-game-data.h"

InnerGameData InnerGameData::instance{};

InnerGameData &InnerGameData::get_instance()
{
    return instance;
}
