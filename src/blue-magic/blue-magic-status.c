#include "blue-magic/blue-magic-status.h"
#include "blue-magic/blue-magic-util.h"
#include "spell/spells-status.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool cast_blue_scare(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("恐ろしげな幻覚を作り出した。", "You cast a fearful illusion."));
    fear_monster(caster_ptr, bmc_ptr->dir, bmc_ptr->plev + 10);
    return TRUE;
}

bool cast_blue_blind(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    confuse_monster(caster_ptr, bmc_ptr->dir, bmc_ptr->plev * 2);
    return TRUE;
}

bool cast_blue_confusion(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("誘惑的な幻覚をつくり出した。", "You cast a mesmerizing illusion."));
    confuse_monster(caster_ptr, bmc_ptr->dir, bmc_ptr->plev * 2);
    return TRUE;
}

bool cast_blue_slow(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    slow_monster(caster_ptr, bmc_ptr->dir, bmc_ptr->plev);
    return TRUE;
}

bool cast_blue_sleep(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    sleep_monster(caster_ptr, bmc_ptr->dir, bmc_ptr->plev);
    return TRUE;
}
