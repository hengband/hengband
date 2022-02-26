#include "load/birth-loader.h"
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

    previous_char.psex = i2enum<player_sex>(rd_byte());
    previous_char.prace = i2enum<PlayerRaceType>(rd_byte());
    previous_char.pclass = i2enum<PlayerClassType>(rd_byte());
    previous_char.ppersonality = i2enum<player_personality_type>(rd_byte());
    previous_char.realm1 = rd_byte();
    previous_char.realm2 = rd_byte();

    previous_char.age = rd_s16b();
    previous_char.ht = rd_s16b();
    previous_char.wt = rd_s16b();
    previous_char.sc = rd_s16b();
    previous_char.au = rd_s32b();

    for (int i = 0; i < A_MAX; i++) {
        previous_char.stat_max[i] = rd_s16b();
    }
    for (int i = 0; i < A_MAX; i++) {
        previous_char.stat_max_max[i] = rd_s16b();
    }

    for (int i = 0; i < PY_MAX_LEVEL; i++) {
        previous_char.player_hp[i] = rd_s16b();
    }

    previous_char.chaos_patron = rd_s16b();

    for (int i = 0; i < 8; i++) {
        previous_char.vir_types[i] = rd_s16b();
    }

    for (int i = 0; i < 4; i++) {
        rd_string(previous_char.history[i], sizeof(previous_char.history[i]));
    }

    strip_bytes(1);
    previous_char.quick_ok = rd_byte() != 0;
}
