#include "system/angband.h"
#include "quick-start.h"

/*
 * The last character rolled,
 * holded for quick start
 */
birther previous_char;

/*!
 * @brief プレイヤーのクイックスタート情報をプレイヤー構造体から保存する / Save the current data for later
 * @param birther_ptr クイックスタート構造体の参照ポインタ
 * @return なし。
 */
void save_prev_data(player_type* creature_ptr, birther* birther_ptr)
{
    birther_ptr->psex = creature_ptr->psex;
    birther_ptr->prace = creature_ptr->prace;
    birther_ptr->pclass = creature_ptr->pclass;
    birther_ptr->pseikaku = creature_ptr->pseikaku;
    birther_ptr->realm1 = creature_ptr->realm1;
    birther_ptr->realm2 = creature_ptr->realm2;
    birther_ptr->age = creature_ptr->age;
    birther_ptr->ht = creature_ptr->ht;
    birther_ptr->wt = creature_ptr->wt;
    birther_ptr->sc = creature_ptr->sc;
    birther_ptr->au = creature_ptr->au;

    for (int i = 0; i < A_MAX; i++) {
        birther_ptr->stat_max[i] = creature_ptr->stat_max[i];
        birther_ptr->stat_max_max[i] = creature_ptr->stat_max_max[i];
    }

    for (int i = 0; i < PY_MAX_LEVEL; i++) {
        birther_ptr->player_hp[i] = creature_ptr->player_hp[i];
    }

    birther_ptr->chaos_patron = creature_ptr->chaos_patron;
    for (int i = 0; i < 8; i++) {
        birther_ptr->vir_types[i] = creature_ptr->vir_types[i];
    }

    for (int i = 0; i < 4; i++) {
        strcpy(birther_ptr->history[i], creature_ptr->history[i]);
    }
}

/*!
 * @brief プレイヤーのクイックスタート情報をプレイヤー構造体へ読み込む / Load the previous data
 * @param swap TRUEならば現在のプレイヤー構造体上との内容をスワップする形で読み込む。
 * @return なし。
 */
void load_prev_data(player_type* creature_ptr, bool swap)
{
    birther temp;
    if (swap)
        save_prev_data(creature_ptr, &temp);

    creature_ptr->psex = previous_char.psex;
    creature_ptr->prace = previous_char.prace;
    creature_ptr->pclass = previous_char.pclass;
    creature_ptr->pseikaku = previous_char.pseikaku;
    creature_ptr->realm1 = previous_char.realm1;
    creature_ptr->realm2 = previous_char.realm2;
    creature_ptr->age = previous_char.age;
    creature_ptr->ht = previous_char.ht;
    creature_ptr->wt = previous_char.wt;
    creature_ptr->sc = previous_char.sc;
    creature_ptr->au = previous_char.au;

    for (int i = 0; i < A_MAX; i++) {
        creature_ptr->stat_cur[i] = creature_ptr->stat_max[i] = previous_char.stat_max[i];
        creature_ptr->stat_max_max[i] = previous_char.stat_max_max[i];
    }

    for (int i = 0; i < PY_MAX_LEVEL; i++) {
        creature_ptr->player_hp[i] = previous_char.player_hp[i];
    }

    creature_ptr->mhp = creature_ptr->player_hp[0];
    creature_ptr->chp = creature_ptr->player_hp[0];
    creature_ptr->chaos_patron = previous_char.chaos_patron;
    for (int i = 0; i < 8; i++) {
        creature_ptr->vir_types[i] = previous_char.vir_types[i];
    }

    for (int i = 0; i < 4; i++) {
        strcpy(creature_ptr->history[i], previous_char.history[i]);
    }

    if (swap) {
        (void)COPY(&previous_char, &temp, birther);
    }
}
