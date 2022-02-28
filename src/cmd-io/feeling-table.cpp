/*
 * @brief 雰囲気を表すテキストの配列群
 * @date 2020/03/08
 * @author Hourier
 */

#include "cmd-io/feeling-table.h"

concptr do_cmd_feeling_text[MAX_FEELING_TEXT] = {
    _("この階の雰囲気を感じとれなかった...", "Looks like any other level."),
    _("この階には何か特別なものがあるような気がする。", "You feel there is something special about this level."),
    _("恐ろしい死の幻が目に浮かび、気絶しそうになった！", "You nearly faint as horrible visions of death fill your mind!"),
    _("この階はとても危険なようだ。", "This level looks very dangerous."),
    _("とても悪い予感がする...", "You have a very bad feeling..."),
    _("悪い予感がする...", "You have a bad feeling..."),
    _("何か緊張する。", "You feel nervous."),
    _("少し不運な気がする...", "You feel your luck is turning..."),
    _("この場所は好きになれない。", "You don't like the look of this place."),
    _("この階はそれなりに安全なようだ。", "This level looks reasonably safe."),
    _("なんて退屈なところだ...", "What a boring place...")
};

concptr do_cmd_feeling_text_combat[MAX_FEELING_TEXT] = {
    _("この階の雰囲気を感じとれなかった...", "Looks like any other level."),
    _("この階には何か特別なものがあるような気がする。", "You feel there is something special about this level."),
    _("今夜もまた、誰かが命を落とす...", "You nearly faint as horrible visions of death fill your mind!"),
    _("この階はとても危険なようだ。", "This level looks very dangerous."),
    _("とても悪い予感がする...", "You have a very bad feeling..."),
    _("悪い予感がする...", "You have a bad feeling..."),
    _("何か緊張する。", "You feel nervous."),
    _("少し不運な気がする...", "You feel your luck is turning..."),
    _("この場所は好きになれない。", "You don't like the look of this place."),
    _("この階はそれなりに安全なようだ。", "This level looks reasonably safe."),
    _("なんて退屈なところだ...", "What a boring place...")
};

concptr do_cmd_feeling_text_lucky[MAX_FEELING_TEXT] = {
    _("この階の雰囲気を感じとれなかった...", "Looks like any other level."),
    _("この階には何か特別なものがあるような気がする。", "You feel there is something special about this level."),
    _("この階はこの上なく素晴らしい感じがする。", "You have a superb feeling about this level."),
    _("素晴らしい感じがする...", "You have an excellent feeling..."),
    _("とても良い感じがする...", "You have a very good feeling..."),
    _("良い感じがする...", "You have a good feeling..."),
    _("ちょっと幸運な感じがする...", "You feel strangely lucky..."),
    _("多少は運が向いてきたか...", "You feel your luck is turning..."),
    _("見た感じ悪くはない...", "You like the look of this place..."),
    _("全然駄目ということはないが...", "This level can't be all bad..."),
    _("なんて退屈なところだ...", "What a boring place...")
};
