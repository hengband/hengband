#include "birth/birth-util.h"
#include "cmd-io/cmd-gameoption.h"
#include "core/show-file.h"
#include "main/sound-of-music.h"
#include "system/game-option-types.h"
#include "term/screen-processor.h"

/*!
 * @brief プレイヤー作成を中断して変愚蛮怒を終了する
 */
void birth_quit(void)
{
    quit(nullptr);
}

/*!
 * @brief 指定されたヘルプファイルを表示する / Show specific help file
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param helpfile ファイル名
 */
void show_help(PlayerType *player_ptr, concptr helpfile)
{
    screen_save();
    (void)show_file(player_ptr, true, helpfile, nullptr, 0, 0);
    screen_load();
}

void birth_help_option(PlayerType *player_ptr, char c, BirthKind bk)
{
    concptr help_file;
    switch (bk) {
    case BirthKind::RACE:
        help_file = _("jraceclas.txt#TheRaces", "raceclas.txt#TheRaces");
        break;
    case BirthKind::CLASS:
        help_file = _("jraceclas.txt#TheClasses", "raceclas.txt#TheClasses");
        break;
    case BirthKind::REALM:
        help_file = _("jmagic.txt#MagicRealms", "magic.txt#MagicRealms");
        break;
    case BirthKind::PERSONALITY:
        help_file = _("jraceclas.txt#ThePersonalities", "raceclas.txt#ThePersonalities");
        break;
    case BirthKind::AUTO_ROLLER:
        help_file = _("jbirth.txt#AutoRoller", "birth.txt#AutoRoller");
        break;
    default:
        help_file = "";
        break;
    }

    if (c == '?') {
        show_help(player_ptr, help_file);
    } else if (c == '=') {
        screen_save();
        do_cmd_options_aux(player_ptr, OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Options ((*)) affect score"));
        screen_load();
    } else if (c != '2' && c != '4' && c != '6' && c != '8') {
        bell();
    }
}
