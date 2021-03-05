﻿#include "cmd-io/cmd-diary.h"
#include "cmd-io/diary-subtitle-table.h"
#include "core/asking-player.h"
#include "core/show-file.h"
#include "game-option/play-record-options.h"
#include "io/record-play-movie.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "io/write-diary.h"
#include "main/sound-of-music.h"
#include "player/player-personality.h"
#include "player/player-realm.h"
#include "term/screen-processor.h"
#include "util/angband-files.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 日記のタイトル表記と内容出力
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void display_diary(player_type *creature_ptr)
{
    char diary_title[256];
    GAME_TEXT file_name[MAX_NLEN];
    char buf[1024];
    char tmp[80];
    sprintf(file_name, _("playrecord-%s.txt", "playrec-%s.txt"), savefile_base);
    path_build(buf, sizeof(buf), ANGBAND_DIR_USER, file_name);

    if (creature_ptr->pclass == CLASS_WARRIOR || creature_ptr->pclass == CLASS_MONK || creature_ptr->pclass == CLASS_SAMURAI
        || creature_ptr->pclass == CLASS_BERSERKER)
        strcpy(tmp, subtitle[randint0(MAX_SUBTITLE - 1)]);
    else if (is_wizard_class(creature_ptr))
        strcpy(tmp, subtitle[randint0(MAX_SUBTITLE - 1) + 1]);
    else
        strcpy(tmp, subtitle[randint0(MAX_SUBTITLE - 2) + 1]);

#ifdef JP
    sprintf(diary_title, "「%s%s%sの伝説 -%s-」", ap_ptr->title, ap_ptr->no ? "の" : "", creature_ptr->name, tmp);
#else
    sprintf(diary_title, "Legend of %s %s '%s'", ap_ptr->title, creature_ptr->name, tmp);
#endif

    (void)show_file(creature_ptr, FALSE, buf, diary_title, -1, 0);
}

/*!
 * @brief 日記に任意の内容を表記するコマンドのメインルーチン /
 * @return なし
 */
static void add_diary_note(player_type *creature_ptr)
{
    char tmp[80] = "\0";
    char bunshou[80] = "\0";
    if (get_string(_("内容: ", "diary note: "), tmp, 79)) {
        strcpy(bunshou, tmp);
        exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 0, bunshou);
    }
}

/*!
 * @brief 最後に取得したアイテムの情報を日記に追加するメインルーチン /
 * @return なし
 */
static void do_cmd_last_get(player_type *creaute_ptr)
{
    if (record_o_name[0] == '\0')
        return;

    char buf[256];
    sprintf(buf, _("%sの入手を記録します。", "Do you really want to record getting %s? "), record_o_name);
    if (!get_check(buf))
        return;

    GAME_TURN turn_tmp = current_world_ptr->game_turn;
    current_world_ptr->game_turn = record_turn;
    sprintf(buf, _("%sを手に入れた。", "discover %s."), record_o_name);
    exe_write_diary(creaute_ptr, DIARY_DESCRIPTION, 0, buf);
    current_world_ptr->game_turn = turn_tmp;
}

/*!
 * @brief ファイル中の全日記記録を消去する /
 * @return なし
 */
static void do_cmd_erase_diary(void)
{
    GAME_TEXT file_name[MAX_NLEN];
    char buf[256];
    FILE *fff = NULL;

    if (!get_check(_("本当に記録を消去しますか？", "Do you really want to delete all your records? ")))
        return;
    sprintf(file_name, _("playrecord-%s.txt", "playrec-%s.txt"), savefile_base);
    path_build(buf, sizeof(buf), ANGBAND_DIR_USER, file_name);
    fd_kill(buf);

    fff = angband_fopen(buf, "w");
    if (fff) {
        angband_fclose(fff);
        msg_format(_("記録を消去しました。", "deleted record."));
    } else {
        msg_format(_("%s の消去に失敗しました。", "failed to delete %s."), buf);
    }

    msg_print(NULL);
}

/*!
 * @brief 日記コマンド
 * @param crerature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_diary(player_type *creature_ptr)
{
    screen_save();
    while (TRUE) {
        term_clear();
        prt(_("[ 記録の設定 ]", "[ Play Record ]"), 2, 0);
        prt(_("(1) 記録を見る", "(1) Display your record"), 4, 5);
        prt(_("(2) 文章を記録する", "(2) Add record"), 5, 5);
        prt(_("(3) 直前に入手又は鑑定したものを記録する", "(3) Record the last item you got or identified"), 6, 5);
        prt(_("(4) 記録を消去する", "(4) Delete your record"), 7, 5);
        prt(_("(R) プレイ動画を記録する/中止する", "(R) Record playing movie / or stop it"), 9, 5);
        prt(_("コマンド:", "Command: "), 18, 0);
        int i = inkey();
        if (i == ESCAPE)
            break;

        switch (i) {
        case '1':
            display_diary(creature_ptr);
            break;
        case '2':
            add_diary_note(creature_ptr);
            break;
        case '3':
            do_cmd_last_get(creature_ptr);
            break;
        case '4':
            do_cmd_erase_diary();
            break;
        case 'r':
        case 'R':
            screen_load();
            prepare_movie_hooks(creature_ptr);
            return;
        default:
            bell();
        }

        msg_erase();
    }

    screen_load();
}
