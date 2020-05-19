#include "system/angband.h"
#include "birth/birth-util.h"
#include "core/show-file.h"

/*!
 * @brief プレイヤー作成を中断して変愚蛮怒を終了する
 * @return なし
 */
void birth_quit(void)
{
    quit(NULL);
}

/*!
 * @brief 指定されたヘルプファイルを表示する / Show specific help file
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param helpfile ファイル名
 * @return なし
 */
void show_help(player_type* creature_ptr, concptr helpfile)
{
    screen_save();
    (void)show_file(creature_ptr, TRUE, helpfile, NULL, 0, 0);
    screen_load();
}

void birth_help_option(player_type *creature_ptr, char c, birth_kind bk)
{
    concptr help_file;
    switch (bk) {
    case BK_RACE:
        help_file = _("jraceclas.txt#TheRaces", "raceclas.txt#TheRaces");
        break;
    case BK_REALM:
        help_file = _("jmagic.txt#MagicRealms", "magic.txt#MagicRealms");
        break;
    case BK_AUTO_ROLLER:
        help_file = _("jbirth.txt#AutoRoller", "birth.txt#AutoRoller");
    default:
        help_file = "";
        break;
    }

    if (c == '?') {
        show_help(creature_ptr, help_file);
    } else if (c == '=') {
        screen_save();
        do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth option((*)s effect score)"));
        screen_load();
    } else if (c != '2' && c != '4' && c != '6' && c != '8')
        bell();
}
