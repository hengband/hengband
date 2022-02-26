/*
 * @brief 読むことができるアイテム群の内、巻物を読んだ時の効果や処理を記述する.
 * @date 2022/02/26
 * @author Hourier
 */

#include "object-use/read/scroll-read-executor.h"

ScrollReadExecutor::ScrollReadExecutor(PlayerType *player_ptr, ObjectType *o_ptr, bool known)
    : player_ptr(player_ptr)
    , o_ptr(o_ptr)
    , known(known)
{
}

bool ScrollReadExecutor::is_identified()
{
    return this->ident;
}

/*
 * @todo 戻り値はコンパイルを通すためだけの暫定値
 */
bool ScrollReadExecutor::read()
{
    return true;
}
