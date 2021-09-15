#include "birth-loader.h"
#include "birth/quick-start.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "player-ability/player-ability-types.h"
#include "system/angband.h"
#include "system/system-variables.h"

/*!
 * @brief クイックスタート情報を読み込む / Load quick start data
 */
void load_quick_start(void)
{
    if (h_older_than(1, 0, 13)) {
        previous_char.quick_ok = false;
        return;
    }

    byte tmp8u;
    rd_byte(&tmp8u);
    previous_char.psex = i2enum<player_sex>(tmp8u);
    rd_byte(&tmp8u);
    previous_char.prace = (player_race_type)tmp8u;
    rd_byte(&tmp8u);
    previous_char.pclass = (player_class_type)tmp8u;
    rd_byte(&tmp8u);
    previous_char.pseikaku = (player_personality_type)tmp8u;
    rd_byte(&tmp8u);
    previous_char.realm1 = (int16_t)tmp8u;
    rd_byte(&tmp8u);
    previous_char.realm2 = (int16_t)tmp8u;

    rd_s16b(&previous_char.age);
    rd_s16b(&previous_char.ht);
    rd_s16b(&previous_char.wt);
    rd_s16b(&previous_char.sc);
    rd_s32b(&previous_char.au);

    for (int i = 0; i < A_MAX; i++)
        rd_s16b(&previous_char.stat_max[i]);
    for (int i = 0; i < A_MAX; i++)
        rd_s16b(&previous_char.stat_max_max[i]);

    for (int i = 0; i < PY_MAX_LEVEL; i++) {
        int16_t tmp16s;
        rd_s16b(&tmp16s);
        previous_char.player_hp[i] = (HIT_POINT)tmp16s;
    }

    rd_s16b(&previous_char.chaos_patron);

    for (int i = 0; i < 8; i++)
        rd_s16b(&previous_char.vir_types[i]);

    for (int i = 0; i < 4; i++)
        rd_string(previous_char.history[i], sizeof(previous_char.history[i]));

    rd_byte(&tmp8u);
    rd_byte(&tmp8u);
    previous_char.quick_ok = (bool)tmp8u;
}
