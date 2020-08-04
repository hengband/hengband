#include "floor/dungeon-tunnel-util.h"
#include "system/angband.h"

dt_type *initialize_dt_type(dt_type *dt_ptr)
{
    dt_ptr->dun_tun_rnd = rand_range(DUN_TUN_RND_MIN, DUN_TUN_RND_MAX);
    dt_ptr->dun_tun_chg = rand_range(DUN_TUN_CHG_MIN, DUN_TUN_CHG_MAX);
    dt_ptr->dun_tun_con = rand_range(DUN_TUN_CON_MIN, DUN_TUN_CON_MAX);
    dt_ptr->dun_tun_pen = rand_range(DUN_TUN_PEN_MIN, DUN_TUN_PEN_MAX);
    dt_ptr->dun_tun_jct = rand_range(DUN_TUN_JCT_MIN, DUN_TUN_JCT_MAX);
    return dt_ptr;
}
