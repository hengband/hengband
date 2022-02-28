#include "player/process-name.h"
#include "autopick/autopick-reader-writer.h"
#include "core/asking-player.h"
#include "game-option/birth-options.h"
#include "io/files-util.h"
#include "player/player-personality.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/string-processor.h"
#include "world/world.h"

/*!
 * @brief プレイヤーの名前をチェックして修正する
 * Process the player name.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sf セーブファイル名に合わせた修正を行うならばTRUE
 * @details
 * Extract a clean "base name".
 * Build the savefile name if needed.
 */
void process_player_name(PlayerType *player_ptr, bool is_new_savefile)
{
    char old_player_base[32] = "";
    if (w_ptr->character_generated) {
        strcpy(old_player_base, player_ptr->base_name);
    }

    for (int i = 0; player_ptr->name[i]; i++) {
#ifdef JP
        if (iskanji(player_ptr->name[i])) {
            i++;
            continue;
        }

        if (iscntrl((unsigned char)player_ptr->name[i]))
#else
        if (iscntrl(player_ptr->name[i]))
#endif
        {
            quit_fmt(_("'%s' という名前は不正なコントロールコードを含んでいます。", "The name '%s' contains control chars!"), player_ptr->name);
        }
    }

    int k = 0;
    for (int i = 0; player_ptr->name[i]; i++) {
#ifdef JP
        unsigned char c = player_ptr->name[i];
#else
        char c = player_ptr->name[i];
#endif

#ifdef JP
        if (iskanji(c)) {
            if (k + 2 >= (int)sizeof(player_ptr->base_name) || !player_ptr->name[i + 1]) {
                break;
            }

            player_ptr->base_name[k++] = c;
            i++;
            player_ptr->base_name[k++] = player_ptr->name[i];
        }
#ifdef SJIS
        else if (iskana(c))
            player_ptr->base_name[k++] = c;
#endif
        else
#endif
            if (!strncmp(PATH_SEP, player_ptr->name + i, strlen(PATH_SEP))) {
            player_ptr->base_name[k++] = '_';
            i += strlen(PATH_SEP);
        }
#if defined(WINDOWS)
        else if (angband_strchr("\"*,/:;<>?\\|", c))
            player_ptr->base_name[k++] = '_';
#endif
        else if (isprint(c)) {
            player_ptr->base_name[k++] = c;
        }
    }

    player_ptr->base_name[k] = '\0';
    if (!player_ptr->base_name[0]) {
        strcpy(player_ptr->base_name, "PLAYER");
    }

    auto is_modified = false;
    if (is_new_savefile && (!savefile[0] || !keep_savefile)) {
        char temp[128];

#ifdef SAVEFILE_USE_UID
        /* Rename the savefile, using the player_ptr->player_uid and player_ptr->base_name */
        (void)sprintf(temp, "%d.%s", player_ptr->player_uid, player_ptr->base_name);
#else
        /* Rename the savefile, using the player_ptr->base_name */
        (void)sprintf(temp, "%s", player_ptr->base_name);
#endif
        path_build(savefile, sizeof(savefile), ANGBAND_DIR_SAVE, temp);
        is_modified = true;
    }

    if (is_modified || !savefile_base[0]) {
        concptr s = savefile;
        while (true) {
            concptr t;
            t = angband_strstr(s, PATH_SEP);
            if (!t) {
                break;
            }
            s = t + 1;
        }

#ifdef SAVEFILE_USE_UID
        strcpy(savefile_base, angband_strstr(s, ".") + 1);
#else
        strcpy(savefile_base, s);
#endif
    }

    if (w_ptr->character_generated && !streq(old_player_base, player_ptr->base_name)) {
        autopick_load_pref(player_ptr, false);
    }
}

/*!
 * @brief プレイヤーの名前を変更するコマンドのメインルーチン
 * Gets a name for the character, reacting to name changes.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * <pre>
 * Assumes that "display_player()" has just been called
 * Perhaps we should NOT ask for a name (at "birth()") on
 * Unix machines?  XXX XXX
 * What a horrible name for a global function.
 * </pre>
 */
void get_name(PlayerType *player_ptr)
{
    char tmp[64];
    strcpy(tmp, player_ptr->name);

    if (get_string(_("キャラクターの名前を入力して下さい: ", "Enter a name for your character: "), tmp, 15)) {
        strcpy(player_ptr->name, tmp);
    }

    if (strlen(player_ptr->name) == 0) {
        strcpy(player_ptr->name, "PLAYER");
    }

    strcpy(tmp, ap_ptr->title);
#ifdef JP
    if (ap_ptr->no == 1) {
        strcat(tmp, "の");
    }
#else
    strcat(tmp, " ");
#endif
    strcat(tmp, player_ptr->name);

    term_erase(34, 1, 255);
    c_put_str(TERM_L_BLUE, tmp, 1, 34);
    clear_from(22);
}
