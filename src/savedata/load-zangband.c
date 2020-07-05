#include "savedata/load-zangband.h"
#include "game-option/option-flags.h"
#include "realm/realm-types.h"

void load_zangband_options(void)
{
    if (option_flag[5] & (0x00000001 << 4))
        option_flag[5] &= ~(0x00000001 << 4);
    else
        option_flag[5] |= (0x00000001 << 4);

    if (option_flag[2] & (0x00000001 << 5))
        option_flag[2] &= ~(0x00000001 << 5);
    else
        option_flag[2] |= (0x00000001 << 5);

    if (option_flag[4] & (0x00000001 << 5))
        option_flag[4] &= ~(0x00000001 << 5);
    else
        option_flag[4] |= (0x00000001 << 5);

    if (option_flag[5] & (0x00000001 << 0))
        option_flag[5] &= ~(0x00000001 << 0);
    else
        option_flag[5] |= (0x00000001 << 0);

    if (option_flag[5] & (0x00000001 << 12))
        option_flag[5] &= ~(0x00000001 << 12);
    else
        option_flag[5] |= (0x00000001 << 12);

    if (option_flag[1] & (0x00000001 << 0))
        option_flag[1] &= ~(0x00000001 << 0);
    else
        option_flag[1] |= (0x00000001 << 0);

    if (option_flag[1] & (0x00000001 << 18))
        option_flag[1] &= ~(0x00000001 << 18);
    else
        option_flag[1] |= (0x00000001 << 18);

    if (option_flag[1] & (0x00000001 << 19))
        option_flag[1] &= ~(0x00000001 << 19);
    else
        option_flag[1] |= (0x00000001 << 19);

    if (option_flag[5] & (0x00000001 << 3))
        option_flag[1] &= ~(0x00000001 << 3);
    else
        option_flag[5] |= (0x00000001 << 3);
}

void set_zangband_realm(player_type* creature_ptr)
{
    if (creature_ptr->realm1 == 9)
        creature_ptr->realm1 = REALM_MUSIC;

    if (creature_ptr->realm2 == 9)
        creature_ptr->realm2 = REALM_MUSIC;

    if (creature_ptr->realm1 == 10)
        creature_ptr->realm1 = REALM_HISSATSU;

    if (creature_ptr->realm2 == 10)
        creature_ptr->realm2 = REALM_HISSATSU;
}
