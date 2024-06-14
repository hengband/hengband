#include "combat/martial-arts-table.h"
/*!
 * @brief マーシャルアーツ打撃テーブル
 */
const martial_arts ma_blows[MAX_MA] = {
    { _("%sを殴った。", "You punch %s."), 1, 0, 1, 5, 0 },
    { _("%sを蹴った。", "You kick %s."), 2, 0, 1, 7, 0 },
    { _("%sに正拳突きをくらわした。", "You strike %s."), 3, 0, 1, 9, 0 },
    { _("%sに膝蹴りをくらわした。", "You hit %s with your knee."), 5, 5, 2, 4, MA_KNEE },
    { _("%sに肘打ちをくらわした。", "You hit %s with your elbow."), 7, 5, 1, 12, 0 },
    { _("%sに体当りした。", "You butt %s."), 9, 10, 2, 6, 0 },
    { _("%sを蹴った。", "You kick %s."), 11, 10, 3, 6, MA_SLOW },
    { _("%sにアッパーをくらわした。", "You uppercut %s."), 13, 12, 5, 5, 6 },
    { _("%sに二段蹴りをくらわした。", "You double-kick %s."), 16, 15, 5, 6, 8 },
    { _("%sに猫爪撃をくらわした。", "You hit %s with a Cat's Claw."), 20, 20, 5, 8, 0 },
    { _("%sに跳空脚をくらわした。", "You hit %s with a jump kick."), 24, 25, 6, 8, 10 },
    { _("%sに鷲爪襲をくらわした。", "You hit %s with an Eagle's Claw."), 28, 25, 7, 9, 0 },
    { _("%sに回し蹴りをくらわした。", "You hit %s with a circle kick."), 32, 30, 8, 10, 10 },
    { _("%sに鉄拳撃をくらわした。", "You hit %s with an Iron Fist."), 35, 35, 8, 11, 10 },
    { _("%sに飛空脚をくらわした。", "You hit %s with a flying kick."), 39, 35, 8, 12, 12 },
    { _("%sに昇龍拳をくらわした。", "You hit %s with a Dragon Fist."), 43, 35, 9, 12, 16 },
    { _("%sに石破天驚拳をくらわした。", "You hit %s with a Crushing Blow."), 48, 40, 10, 13, 18 },
};
