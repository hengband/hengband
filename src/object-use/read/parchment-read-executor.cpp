/*
 * @brief 読むことができるアイテム群の内、中つ国ガイドを読んだ時の効果や処理を記述する.
 * @date 2022/02/26
 * @author Hourier
 */

#include "object-use/read/parchment-read-executor.h"

ParchmentReadExecutor::ParchmentReadExecutor(PlayerType *player_ptr, ObjectType *o_ptr)
    : player_ptr(player_ptr)
    , o_ptr(o_ptr)
{
}

bool ParchmentReadExecutor::is_identified()
{
    return false;
}

bool ParchmentReadExecutor::read()
{
    return false;
}
