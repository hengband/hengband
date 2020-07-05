#include "savedata/load-zangband.h"
#include "game-option/option-flags.h"

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
