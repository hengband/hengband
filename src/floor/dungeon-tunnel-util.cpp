#include "floor/dungeon-tunnel-util.h"
#include "system/angband.h"

/*
 * Dungeon tunnel generation values
 */
#define DUN_TUN_RND_MIN 5 /* Chance of random direction (was 10) */
#define DUN_TUN_RND_MAX 20
#define DUN_TUN_CHG_MIN 20 /* Chance of changing direction (was 30) */
#define DUN_TUN_CHG_MAX 60
#define DUN_TUN_CON_MIN 10 /* Chance of extra tunneling (was 15) */
#define DUN_TUN_CON_MAX 40
#define DUN_TUN_PEN_MIN 30 /* Chance of doors at room entrances (was 25) */
#define DUN_TUN_PEN_MAX 70
#define DUN_TUN_JCT_MIN 60 /* Chance of doors at tunnel junctions (was 90) */
#define DUN_TUN_JCT_MAX 90

dt_type *initialize_dt_type(dt_type *dt_ptr)
{
    dt_ptr->dun_tun_rnd = rand_range(DUN_TUN_RND_MIN, DUN_TUN_RND_MAX);
    dt_ptr->dun_tun_chg = rand_range(DUN_TUN_CHG_MIN, DUN_TUN_CHG_MAX);
    dt_ptr->dun_tun_con = rand_range(DUN_TUN_CON_MIN, DUN_TUN_CON_MAX);
    dt_ptr->dun_tun_pen = rand_range(DUN_TUN_PEN_MIN, DUN_TUN_PEN_MAX);
    dt_ptr->dun_tun_jct = rand_range(DUN_TUN_JCT_MIN, DUN_TUN_JCT_MAX);
    return dt_ptr;
}
