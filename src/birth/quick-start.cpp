#include "birth/quick-start.h"
#include "birth/birth-stat.h"
#include "birth/birth-util.h"
#include "birth/game-play-initializer.h"
#include "core/player-update-types.h"
#include "io/input-key-acceptor.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/player-personality.h"
#include "player/player-sex.h"
#include "player/player-status.h"
#include "player/process-name.h"
#include "player/race-info-table.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/enum-converter.h"

/*
 * The last character rolled,
 * holded for quick start
 */
birther previous_char;

/*!
 * @brief クイックスタート処理の問い合わせと実行を行う。/Ask whether the player use Quick Start or not.
 */
bool ask_quick_start(player_type *player_ptr)
{
    if (!previous_char.quick_ok)
        return false;

    term_clear();
    put_str(_("クイック・スタートを使うと以前と全く同じキャラクターで始められます。",
                "Do you want to use the quick start function (same character as your last one)."),
        11, 2);
    while (true) {
        char c;
        put_str(_("クイック・スタートを使いますか？[y/N]", "Use quick start? [y/N]"), 14, 10);
        c = inkey();
        if (c == 'Q')
            quit(nullptr);
        else if (c == 'S')
            return false;
        else if (c == '?')
            show_help(player_ptr, _("jbirth.txt#QuickStart", "birth.txt#QuickStart"));
        else if ((c == 'y') || (c == 'Y'))
            break;
        else
            return false;
    }

    load_prev_data(player_ptr, false);
    init_turn(player_ptr);
    init_dungeon_quests(player_ptr);

    sp_ptr = &sex_info[player_ptr->psex];
    rp_ptr = &race_info[enum2i(player_ptr->prace)];
    cp_ptr = &class_info[player_ptr->pclass];
    mp_ptr = &m_info[player_ptr->pclass];
    ap_ptr = &personality_info[player_ptr->pseikaku];

    get_extra(player_ptr, false);
    player_ptr->update |= (PU_BONUS | PU_HP);
    update_creature(player_ptr);
    player_ptr->chp = player_ptr->mhp;
    player_ptr->csp = player_ptr->msp;
    process_player_name(player_ptr);
    return true;
}
/*!
 * @brief プレイヤーのクイックスタート情報をプレイヤー構造体から保存する / Save the current data for later
 * @param birther_ptr クイックスタート構造体の参照ポインタ
 * @return なし。
 */
void save_prev_data(player_type *player_ptr, birther *birther_ptr)
{
    birther_ptr->psex = player_ptr->psex;
    birther_ptr->prace = player_ptr->prace;
    birther_ptr->pclass = player_ptr->pclass;
    birther_ptr->pseikaku = player_ptr->pseikaku;

    if (player_ptr->pclass == CLASS_ELEMENTALIST)
        birther_ptr->realm1 = player_ptr->element;
    else
        birther_ptr->realm1 = player_ptr->realm1;

    birther_ptr->realm2 = player_ptr->realm2;
    birther_ptr->age = player_ptr->age;
    birther_ptr->ht = player_ptr->ht;
    birther_ptr->wt = player_ptr->wt;
    birther_ptr->sc = player_ptr->sc;
    birther_ptr->au = player_ptr->au;

    for (int i = 0; i < A_MAX; i++) {
        birther_ptr->stat_max[i] = player_ptr->stat_max[i];
        birther_ptr->stat_max_max[i] = player_ptr->stat_max_max[i];
    }

    for (int i = 0; i < PY_MAX_LEVEL; i++) {
        birther_ptr->player_hp[i] = player_ptr->player_hp[i];
    }

    birther_ptr->chaos_patron = player_ptr->chaos_patron;
    for (int i = 0; i < 8; i++) {
        birther_ptr->vir_types[i] = player_ptr->vir_types[i];
    }

    for (int i = 0; i < 4; i++) {
        strcpy(birther_ptr->history[i], player_ptr->history[i]);
    }
}

/*!
 * @brief プレイヤーのクイックスタート情報をプレイヤー構造体へ読み込む / Load the previous data
 * @param swap TRUEならば現在のプレイヤー構造体上との内容をスワップする形で読み込む。
 * @return なし。
 */
void load_prev_data(player_type *player_ptr, bool swap)
{
    birther temp;
    if (swap)
        save_prev_data(player_ptr, &temp);

    player_ptr->psex = previous_char.psex;
    player_ptr->prace = previous_char.prace;
    player_ptr->pclass = previous_char.pclass;
    player_ptr->pseikaku = previous_char.pseikaku;

    if (player_ptr->pclass == CLASS_ELEMENTALIST)
        player_ptr->element = previous_char.realm1;
    else
        player_ptr->realm1 = previous_char.realm1;

    player_ptr->realm2 = previous_char.realm2;
    player_ptr->age = previous_char.age;
    player_ptr->ht = previous_char.ht;
    player_ptr->wt = previous_char.wt;
    player_ptr->sc = previous_char.sc;
    player_ptr->au = previous_char.au;

    for (int i = 0; i < A_MAX; i++) {
        player_ptr->stat_cur[i] = player_ptr->stat_max[i] = previous_char.stat_max[i];
        player_ptr->stat_max_max[i] = previous_char.stat_max_max[i];
    }

    for (int i = 0; i < PY_MAX_LEVEL; i++) {
        player_ptr->player_hp[i] = previous_char.player_hp[i];
    }

    player_ptr->mhp = player_ptr->player_hp[0];
    player_ptr->chp = player_ptr->player_hp[0];
    player_ptr->chaos_patron = previous_char.chaos_patron;
    for (int i = 0; i < 8; i++) {
        player_ptr->vir_types[i] = previous_char.vir_types[i];
    }

    for (int i = 0; i < 4; i++) {
        strcpy(player_ptr->history[i], previous_char.history[i]);
    }

    if (swap) {
        previous_char = temp;
    }
}
