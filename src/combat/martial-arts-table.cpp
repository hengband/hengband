#include "combat/martial-arts-table.h"
/*!
 * @brief マーシャルアーツ打撃テーブル
 */
const martial_arts ma_blows[MAX_MA] = {
    { _("%sを殴った。", "You punch %s."), 1, 0, 1, _(5, 4), 0 },
    { _("%sを蹴った。", "You kick %s."), 2, 0, 1, _(7, 6), 0 },
    { _("%sに正拳突きをくらわした。", "You strike %s."), 3, 0, 1, _(9, 7), 0 },
    { _("%sに膝蹴りをくらわした。", "You hit %s with your knee."), 5, 5, 2, _(4, 3), MA_KNEE },
    { _("%sに肘打ちをくらわした。", "You hit %s with your elbow."), 7, 5, 1, _(12, 8), 0 },
    { _("%sに体当りした。", "You butt %s."), 9, 10, 2, _(6, 5), 0 },
    { _("%sを蹴った。", "You kick %s."), 11, 10, 3, _(6, 4), MA_SLOW },
    { _("%sにアッパーをくらわした。", "You uppercut %s."), 13, 12, _(5, 4), _(5, 4), 6 },
    { _("%sに二段蹴りをくらわした。", "You double-kick %s."), 16, 15, 5, _(6, 4), 8 },
    { _("%sに猫爪撃をくらわした。", "You hit %s with a Cat's Claw."), 20, 20, 5, _(8, 5), 0 },
    { _("%sに跳空脚をくらわした。", "You hit %s with a jump kick."), _(24, 25), 25, _(6, 5), _(8, 6), 10 },
    { _("%sに鷲爪襲をくらわした。", "You hit %s with an Eagle's Claw."), _(28, 29), 25, _(7, 6), _(9, 6), 0 },
    { _("%sに回し蹴りをくらわした。", "You hit %s with a circle kick."), _(32, 33), 30, _(8, 6), _(10, 8), 10 },
    { _("%sに鉄拳撃をくらわした。", "You hit %s with an Iron Fist."), _(35, 37), 35, 8, _(11, 8), 10 },
    { _("%sに飛空脚をくらわした。", "You hit %s with a flying kick."), _(39, 41), 35, 8, _(12, 10), 12 },
    { _("%sに昇龍拳をくらわした。", "You hit %s with a Dragon Fist."), _(43, 45), 35, _(9, 10), _(12, 10), 16 },
    { _("%sに石破天驚拳をくらわした。", "You hit %s with a Crushing Blow."), 48, _(40, 35), 10, _(13, 12), 18 },
};
