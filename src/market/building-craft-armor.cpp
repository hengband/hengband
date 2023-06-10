#include "market/building-craft-armor.h"
#include "io/input-key-acceptor.h"
#include "market/building-util.h"
#include "term/screen-processor.h"
#include "view/display-util.h"

/*!
 * @brief ACから回避率、ダメージ減少率を計算し表示する。 / Evaluate AC
 * @details
 * Calculate and display the dodge-rate and the protection-rate
 * based on AC
 * @param ac プレイヤーのAC。
 * @return 常にTRUEを返す。
 */
bool eval_ac(short ac)
{
    constexpr auto memo = _("ダメージ軽減率とは、敵の攻撃が当たった時そのダメージを\n"
                            "何パーセント軽減するかを示します。\n"
                            "ダメージ軽減は通常の直接攻撃(種類が「攻撃する」と「粉砕する」の物)\n"
                            "に対してのみ効果があります。\n \n"
                            "敵のレベルとは、その敵が通常何階に現れるかを示します。\n \n"
                            "回避率は敵の直接攻撃を何パーセントの確率で避けるかを示し、\n"
                            "敵のレベルとあなたのACによって決定されます。\n \n"
                            "ダメージ期待値とは、敵の１００ポイントの通常攻撃に対し、\n"
                            "回避率とダメージ軽減率を考慮したダメージの期待値を示します。\n",
        "'Protection Rate' means how much damage is reduced by your armor.\n"
        "Note that the Protection rate is effective only against normal "
        "'attack' and 'shatter' type melee attacks, "
        "and has no effect against any other types such as 'poison'.\n \n"
        "'Dodge Rate' indicates the success rate on dodging the "
        "monster's melee attacks.  "
        "It is depend on the level of the monster and your AC.\n \n"
        "'Average Damage' indicates the expected amount of damage "
        "when you are attacked by normal melee attacks with power=100.");

    if (ac < 0) {
        ac = 0;
    }

    const auto protection = 100 * std::min<short>(ac, 150) / 250;
    screen_save();
    clear_bldg(0, 22);

    auto row = 2;
    put_str(format(_("あなたの現在のAC: %3d", "Your current AC : %3d"), ac), row++, 0);
    put_str(format(_("ダメージ軽減率  : %3d%%", "Protection rate : %3d%%"), protection), row++, 0);
    row++;

    put_str(_("敵のレベル      :", "Level of Monster:"), row + 0, 0);
    put_str(_("回避率          :", "Dodge Rate      :"), row + 1, 0);
    put_str(_("ダメージ期待値  :", "Average Damage  :"), row + 2, 0);

    for (auto col = 17 + 1, lvl = 0; lvl <= 100; lvl += 10, col += 5) {
        auto quality = 60 + lvl * 3; /* attack quality with power 60 */
        put_str(format("%3d", lvl), row + 0, col);

        /* 回避率を計算 */
        auto dodge = 5 + (std::min(100, 100 * (ac * 3 / 4) / quality) * 9 + 5) / 10;
        put_str(format("%3d%%", dodge), row + 1, col);

        /* 100点の攻撃に対してのダメージ期待値を計算 */
        auto average = (100 - dodge) * (100 - protection) / 100;
        put_str(format("%3d", average), row + 2, col);
    }

    display_wrap_around(memo, 70, row + 4, 4);

    prt(_("現在のあなたの装備からすると、あなたの防御力はこれくらいです:", "Defense abilities from your current Armor Class are evaluated below."), 0, 0);

    flush();
    (void)inkey();
    screen_load();

    return true;
}
