/*
 * @brief 読むことができるアイテム群の内、Tシャツ『★GHB』を読んだ時の効果や処理を記述する.
 * @date 2022/02/26
 * @author Hourier
 */

#include "object-use/read/gbh-shirt-read-executor.h"
#include "view/display-messages.h"

bool GbhShirtReadExecutor::is_identified() const
{
    return false;
}

bool GbhShirtReadExecutor::read()
{
    msg_print(_("私は苦労して『グレーター・ヘル=ビースト』を倒した。", "I had a very hard time to kill the Greater hell-beast, "));
    msg_print(_("しかし手に入ったのはこのきたないＴシャツだけだった。", "but all I got was this lousy t-shirt!"));
    return false;
}
