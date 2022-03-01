/*
 * @brief 読むことができるアイテム群の内、一つの指輪に刻印された文字を読んだ時の効果や処理を記述する.
 * @date 2022/02/26
 * @author Hourier
 */

#include "object-use/read/ring-power-read-executor.h"
#include "locale/language-switcher.h"
#include "view/display-messages.h"

bool RingOfPowerReadExecutor::is_identified() const
{
    return false;
}

bool RingOfPowerReadExecutor::read()
{
    msg_print(_("「一つの指輪は全てを統べ、", "'One Ring to rule them all, "));
    msg_print(nullptr);
    msg_print(_("一つの指輪は全てを見つけ、", "One Ring to find them, "));
    msg_print(nullptr);
    msg_print(_("一つの指輪は全てを捕らえて", "One Ring to bring them all "));
    msg_print(nullptr);
    msg_print(_("暗闇の中に繋ぎとめる。」", "and in the darkness bind them.'"));
    return false;
}
