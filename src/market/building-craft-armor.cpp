﻿#include "io/input-key-acceptor.h"
#include "market/building-craft-armor.h"
#include "market/building-util.h"
#include "term/screen-processor.h"
#include "util/buffer-shaper.h"

/*!
 * @brief ACから回避率、ダメージ減少率を計算し表示する。 / Evaluate AC
 * @details
 * Calculate and display the dodge-rate and the protection-rate
 * based on AC
 * @param iAC プレイヤーのAC。
 * @return 常にTRUEを返す。
 */
bool eval_ac(ARMOUR_CLASS iAC)
{
    const GAME_TEXT memo[] = _("ダメージ軽減率とは、敵の攻撃が当たった時そのダメージを\n"
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

    int protection;
    TERM_LEN col, row = 2;
    DEPTH lvl;
    char buf[80 * 20], *t;

    if (iAC < 0)
        iAC = 0;

    protection = 100 * MIN(iAC, 150) / 250;
    screen_save();
    clear_bldg(0, 22);

    put_str(format(_("あなたの現在のAC: %3d", "Your current AC : %3d"), iAC), row++, 0);
    put_str(format(_("ダメージ軽減率  : %3d%%", "Protection rate : %3d%%"), protection), row++, 0);
    row++;

    put_str(_("敵のレベル      :", "Level of Monster:"), row + 0, 0);
    put_str(_("回避率          :", "Dodge Rate      :"), row + 1, 0);
    put_str(_("ダメージ期待値  :", "Average Damage  :"), row + 2, 0);

    for (col = 17 + 1, lvl = 0; lvl <= 100; lvl += 10, col += 5) {
        int quality = 60 + lvl * 3; /* attack quality with power 60 */
        int dodge; /* 回避率(%) */
        int average; /* ダメージ期待値 */

        put_str(format("%3d", lvl), row + 0, col);

        /* 回避率を計算 */
        dodge = 5 + (MIN(100, 100 * (iAC * 3 / 4) / quality) * 9 + 5) / 10;
        put_str(format("%3d%%", dodge), row + 1, col);

        /* 100点の攻撃に対してのダメージ期待値を計算 */
        average = (100 - dodge) * (100 - protection) / 100;
        put_str(format("%3d", average), row + 2, col);
    }

    shape_buffer(memo, 70, buf, sizeof(buf));
    for (t = buf; t[0]; t += strlen(t) + 1)
        put_str(t, (row++) + 4, 4);

    prt(_("現在のあなたの装備からすると、あなたの防御力はこれくらいです:", "Defense abilities from your current Armor Class are evaluated below."), 0, 0);

    flush();
    (void)inkey();
    screen_load();

    return TRUE;
}
