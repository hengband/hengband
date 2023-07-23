/*!
 * @file blue-magic-status.cpp
 * @brief 青魔法の状態異常系スペル定義
 */

#include "blue-magic/blue-magic-status.h"
#include "blue-magic/blue-magic-util.h"
#include "spell/spells-status.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool cast_blue_scare(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("恐ろしげな幻覚を作り出した。", "You cast a fearful illusion."));
    fear_monster(player_ptr, bmc_ptr->dir, bmc_ptr->plev + 10);
    return true;
}

bool cast_blue_blind(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    confuse_monster(player_ptr, bmc_ptr->dir, bmc_ptr->plev * 2);
    return true;
}

bool cast_blue_confusion(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("誘惑的な幻覚をつくり出した。", "You cast a mesmerizing illusion."));
    confuse_monster(player_ptr, bmc_ptr->dir, bmc_ptr->plev * 2);
    return true;
}

bool cast_blue_slow(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    slow_monster(player_ptr, bmc_ptr->dir, bmc_ptr->plev);
    return true;
}

bool cast_blue_sleep(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    sleep_monster(player_ptr, bmc_ptr->dir, bmc_ptr->plev);
    return true;
}
