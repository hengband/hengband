#include "system/angband.h"
#include "combat/martial-arts-table.h"
/*!
 * @brief マーシャルアーツ打撃テーブル
 */
const martial_arts ma_blows[MAX_MA] = {
#ifdef JP
    { "%sを殴った。", 1, 0, 1, 5, 0 },
    { "%sを蹴った。", 2, 0, 1, 7, 0 },
    { "%sに正拳突きをくらわした。", 3, 0, 1, 9, 0 },
    { "%sに膝蹴りをくらわした。", 5, 5, 2, 4, MA_KNEE },
    { "%sに肘打ちをくらわした。", 7, 5, 1, 12, 0 },
    { "%sに体当りした。", 9, 10, 2, 6, 0 },
    { "%sを蹴った。", 11, 10, 3, 6, MA_SLOW },
    { "%sにアッパーをくらわした。", 13, 12, 5, 5, 6 },
    { "%sに二段蹴りをくらわした。", 16, 15, 5, 6, 8 },
    { "%sに猫爪撃をくらわした。", 20, 20, 5, 8, 0 },
    { "%sに跳空脚をくらわした。", 24, 25, 6, 8, 10 },
    { "%sに鷲爪襲をくらわした。", 28, 25, 7, 9, 0 },
    { "%sに回し蹴りをくらわした。", 32, 30, 8, 10, 10 },
    { "%sに鉄拳撃をくらわした。", 35, 35, 8, 11, 10 },
    { "%sに飛空脚をくらわした。", 39, 35, 8, 12, 12 },
    { "%sに昇龍拳をくらわした。", 43, 35, 9, 12, 16 },
    { "%sに石破天驚拳をくらわした。", 48, 40, 10, 13, 18 },
#else
    { "You punch %s.", 1, 0, 1, 4, 0 },
    { "You kick %s.", 2, 0, 1, 6, 0 },
    { "You strike %s.", 3, 0, 1, 7, 0 },
    { "You hit %s with your knee.", 5, 5, 2, 3, MA_KNEE },
    { "You hit %s with your elbow.", 7, 5, 1, 8, 0 },
    { "You butt %s.", 9, 10, 2, 5, 0 },
    { "You kick %s.", 11, 10, 3, 4, MA_SLOW },
    { "You uppercut %s.", 13, 12, 4, 4, 6 },
    { "You double-kick %s.", 16, 15, 5, 4, 8 },
    { "You hit %s with a Cat's Claw.", 20, 20, 5, 5, 0 },
    { "You hit %s with a jump kick.", 25, 25, 5, 6, 10 },
    { "You hit %s with an Eagle's Claw.", 29, 25, 6, 6, 0 },
    { "You hit %s with a circle kick.", 33, 30, 6, 8, 10 },
    { "You hit %s with an Iron Fist.", 37, 35, 8, 8, 10 },
    { "You hit %s with a flying kick.", 41, 35, 8, 10, 12 },
    { "You hit %s with a Dragon Fist.", 45, 35, 10, 10, 16 },
    { "You hit %s with a Crushing Blow.", 48, 35, 10, 12, 18 },
#endif

};
