#include "combat/martial-arts-table.h"

/*!
 * @brief マーシャルアーツの技を定義する
 * @param desc_fmt 打撃の説明の書式 ({} に対象の名前が入る)
 * @param min_lev 使える最低のレベル
 * @param success_chance 成功しやすさ
 * @param dice ダメージのダイス
 * @param special_effect 特殊効果のID
 * @details 打撃の説明の書式 (fmt::format_string) は既定のコンストラクタを持たないので、
 * 集成体のままだと既定のコンストラクタが暗黙に削除され MSVC の警告 C4623 になる。
 */
martial_arts::martial_arts(fmt::format_string<std::string_view> desc_fmt, PLAYER_LEVEL min_lev, int success_chance, const Dice &dice, int special_effect)
    : desc(desc_fmt)
    , min_level(min_lev)
    , chance(success_chance)
    , damage_dice(dice)
    , effect(special_effect)
{
}

/*!
 * @brief マーシャルアーツ打撃テーブル
 */
const martial_arts ma_blows[MAX_MA] = {
    { _("{}を殴った。", "You punch {}."), 1, 0, Dice(1, 5), 0 },
    { _("{}を蹴った。", "You kick {}."), 2, 0, Dice(1, 7), 0 },
    { _("{}に正拳突きをくらわした。", "You strike {}."), 3, 0, Dice(1, 9), 0 },
    { _("{}に膝蹴りをくらわした。", "You hit {} with your knee."), 5, 5, Dice(2, 4), MA_KNEE },
    { _("{}に肘打ちをくらわした。", "You hit {} with your elbow."), 7, 5, Dice(1, 12), 0 },
    { _("{}に体当りした。", "You butt {}."), 9, 10, Dice(2, 6), 0 },
    { _("{}を蹴った。", "You kick {}."), 11, 10, Dice(3, 6), MA_SLOW },
    { _("{}にアッパーをくらわした。", "You uppercut {}."), 13, 12, Dice(5, 5), 6 },
    { _("{}に二段蹴りをくらわした。", "You double-kick {}."), 16, 15, Dice(5, 6), 8 },
    { _("{}に猫爪撃をくらわした。", "You hit {} with a Cat's Claw."), 20, 20, Dice(5, 8), 0 },
    { _("{}に跳空脚をくらわした。", "You hit {} with a jump kick."), 24, 25, Dice(6, 8), 10 },
    { _("{}に鷲爪襲をくらわした。", "You hit {} with an Eagle's Claw."), 28, 25, Dice(7, 9), 0 },
    { _("{}に回し蹴りをくらわした。", "You hit {} with a circle kick."), 32, 30, Dice(8, 10), 10 },
    { _("{}に鉄拳撃をくらわした。", "You hit {} with an Iron Fist."), 35, 35, Dice(8, 11), 10 },
    { _("{}に飛空脚をくらわした。", "You hit {} with a flying kick."), 39, 35, Dice(8, 12), 12 },
    { _("{}に昇龍拳をくらわした。", "You hit {} with a Dragon Fist."), 43, 35, Dice(9, 12), 16 },
    { _("{}に石破天驚拳をくらわした。", "You hit {} with a Crushing Blow."), 48, 40, Dice(10, 13), 18 },
};
