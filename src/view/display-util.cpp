#include "view/display-util.h"
#include "term/term-color-types.h"
#include <vector>

namespace {
struct disp_player_line {
    int col;
    int row;
    int len;
    const char *header;
};

const std::vector<disp_player_line> disp_player_lines = {
    { 1, 10, 25, _("打撃修正(格闘)", "Barehanded") },
    { 1, 10, 25, _("打撃修正(両手)", "Two hands") },
    { 1, 10, 25, _("打撃修正(右手)", "Right hand") },
    { 1, 10, 25, _("打撃修正(左手)", "Left hand") },
    { 1, 11, 25, _("打撃修正(左手)", "Left hand") },
    { 1, 11, 25, _("打撃修正(右手)", "Right hand") },
    { 1, 11, 25, _("", "Posture") },
    { 1, 15, 25, _("射撃攻撃修正", "Shooting") },
    { 1, 16, 25, _("射撃武器倍率", "Multiplier") },
    { 1, 20, 25, _("加速", "Speed") },
    { 1, 19, 25, _("ＡＣ", "AC") },
    { 29, 13, 21, _("レベル", "Level") },
    { 29, 14, 21, _("経験値", "Experience") },
    { 29, 15, 21, _("最大経験", "Max Exp") },
    { 29, 16, 21, _("次レベル", "Exp to Adv") },
    { 29, 17, 21, _("所持金", "Gold") },
    { 29, 19, 21, _("日付", "Time") },
    { 29, 10, 21, _("ＨＰ", "Hit point") },
    { 29, 11, 21, _("ＭＰ", "SP (Mana)") },
    { 29, 20, 21, _("プレイ時間", "Play time") },
    { 53, 10, -1, _("打撃命中  :", "Fighting   : ") },
    { 53, 11, -1, _("射撃命中  :", "Bows/Throw : ") },
    { 53, 12, -1, _("魔法防御  :", "SavingThrow: ") },
    { 53, 13, -1, _("隠密行動  :", "Stealth    : ") },
    { 53, 15, -1, _("知覚      :", "Perception : ") },
    { 53, 16, -1, _("探索      :", "Searching  : ") },
    { 53, 17, -1, _("解除      :", "Disarming  : ") },
    { 53, 18, -1, _("魔法道具  :", "MagicDevice: ") },
    { 1, 12, 25, _("打撃回数", "Blows/Round") },
    { 1, 17, 25, _("射撃回数", "Shots/Round") },
    { 1, 13, 25, _("平均ダメージ", "AverageDmg/Rnd") },
    { 53, 20, -1, _("赤外線視力:", "Infravision: ") },
    { 26, 1, -1, _("名前  : ", "Name  : ") },
    { 1, 3, -1, _("性別     : ", "Sex      : ") },
    { 1, 4, -1, _("種族     : ", "Race     : ") },
    { 1, 5, -1, _("職業     : ", "Class    : ") },
    { 1, 6, -1, _("魔法     : ", "Magic    : ") },
    { 1, 7, -1, _("守護魔神 : ", "Patron   : ") },
    { 29, 3, 21, _("年齢", "Age") },
    { 29, 4, 21, _("身長", "Height") },
    { 29, 5, 21, _("体重", "Weight") },
    { 29, 6, 21, _("社会的地位", "Social Class") },
    { 29, 7, 21, _("属性", "Align") },
    { 29, 14, 21, _("強化度", "Construction") },
    { 29, 16, 21, _("次レベル", "Const to Adv") },
    { 53, 19, -1, _("掘削      :", "Digging    : ") },
};
}

/*!
 * @brief プレイヤーのステータス1種を出力する
 * @param entry 項目ID
 * @param val 値を保管した文字列ポインタ
 * @param attr 項目表示の色
 */
void display_player_one_line(int entry, concptr val, TERM_COLOR attr)
{
    auto head = disp_player_lines[entry].header;
    auto head_len = strlen(head);
    auto row = disp_player_lines[entry].row;
    auto col = disp_player_lines[entry].col;
    auto len = disp_player_lines[entry].len;
    term_putstr(col, row, -1, TERM_WHITE, head);

    if (!val) {
        return;
    }

    if (len <= 0) {
        term_putstr(col + head_len, row, -1, attr, val);
        return;
    }

    int val_len = len - head_len;
    char buf[40];
    sprintf(buf, "%*.*s", val_len, val_len, val);
    term_putstr(col + head_len, row, -1, attr, buf);
}
