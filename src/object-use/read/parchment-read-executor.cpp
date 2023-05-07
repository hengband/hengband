/*
 * @brief 読むことができるアイテム群の内、中つ国ガイドを読んだ時の効果や処理を記述する.
 * @date 2022/02/26
 * @author Hourier
 */

#include "object-use/read/parchment-read-executor.h"
#include "core/show-file.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "io/files-util.h"
#include "system/angband.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/angband-files.h"

ParchmentReadExecutor::ParchmentReadExecutor(PlayerType *player_ptr, ItemEntity *o_ptr)
    : player_ptr(player_ptr)
    , o_ptr(o_ptr)
{
}

bool ParchmentReadExecutor::is_identified() const
{
    return false;
}

bool ParchmentReadExecutor::read()
{
    screen_save();
    auto q = format("book-%d_jp.txt", this->o_ptr->bi_key.sval().value());
    const auto item_name = describe_flavor(this->player_ptr, this->o_ptr, OD_NAME_ONLY);
    const auto &path = path_build(ANGBAND_DIR_FILE, q);
    const auto &filename = path.string();
    (void)show_file(this->player_ptr, true, filename.data(), item_name.data(), 0, 0);
    screen_load();
    return false;
}
