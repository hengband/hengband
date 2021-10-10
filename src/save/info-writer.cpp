#include "save/info-writer.h"
#include "birth/quick-start.h"
#include "game-option/cheat-options.h"
#include "game-option/option-flags.h"
#include "game-option/option-types-table.h"
#include "game-option/special-options.h"
#include "player-ability/player-ability-types.h"
#include "save/item-writer.h"
#include "save/save-util.h"
#include "store/store-util.h"
#include "system/angband.h"
#include "system/object-type-definition.h"
#include "world/world.h"

/*!
 * @brief セーブデータに店舗情報を書き込む / Write a "store" record
 * @param store_ptr 店舗情報の参照ポインタ
 */
void wr_store(store_type *store_ptr)
{
    wr_u32b(store_ptr->store_open);
    wr_s16b(store_ptr->insult_cur);
    wr_byte(store_ptr->owner);
    wr_s16b(store_ptr->stock_num);
    wr_s16b(store_ptr->good_buy);
    wr_s16b(store_ptr->bad_buy);
    wr_s32b(store_ptr->last_visit);
    for (int j = 0; j < store_ptr->stock_num; j++)
        wr_item(&store_ptr->stock[j]);
}

/*!
 * @brief セーブデータに乱数情報を書き込む / Write RNG state
 * @param なし
 */
void wr_randomizer(void)
{
    wr_u16b(0);
    wr_u16b(0);
    const auto &state = w_ptr->rng.get_state();
    for (const auto s : state) {
        wr_u32b(s);
    }
    for (int i = state.size(); i < RAND_DEG; i++) {
        wr_u32b(0);
    }
}

/*!
 * @brief ゲームオプション情報を書き込む / Write the "options"
 */
void wr_options(save_type type)
{
    for (int i = 0; i < 4; i++)
        wr_u32b(0L);

    wr_u32b(delay_factor);

    wr_byte(hitpoint_warn);
    wr_byte(mana_warn);

    /*** Cheating options ***/
    uint16_t c = 0;
    if (cheat_sight)
        c |= 0x0040;

    if (cheat_turn)
        c |= 0x0080;

    if (cheat_peek)
        c |= 0x0100;

    if (cheat_hear)
        c |= 0x0200;

    if (cheat_room)
        c |= 0x0400;

    if (cheat_xtra)
        c |= 0x0800;

    if (cheat_know)
        c |= 0x1000;

    if (cheat_live)
        c |= 0x2000;

    if (cheat_save)
        c |= 0x4000;

    if (cheat_diary_output)
        c |= 0x8000;

    if (cheat_immortal)
        c |= 0x0020;

    if (type == SAVE_TYPE_DEBUG)
        c |= 0xFFFF;

    wr_u16b(c);

    wr_byte(autosave_l);
    wr_byte(autosave_t);
    wr_s16b(autosave_freq);

    for (int i = 0; option_info[i].o_desc; i++) {
        int os = option_info[i].o_set;
        int ob = option_info[i].o_bit;
        if (!option_info[i].o_var)
            continue;

        if (*option_info[i].o_var)
            option_flag[os] |= (1UL << ob);
        else
            option_flag[os] &= ~(1UL << ob);
    }

    for (int i = 0; i < 8; i++)
        wr_u32b(option_flag[i]);

    for (int i = 0; i < 8; i++)
        wr_u32b(option_mask[i]);

    for (int i = 0; i < 8; i++)
        wr_u32b(window_flag[i]);

    for (int i = 0; i < 8; i++)
        wr_u32b(window_mask[i]);
}

/*!
 * @brief ダミー情報スキップを書き込む / Hack -- Write the "ghost" info
 */
void wr_ghost(void)
{
    wr_string(_("不正なゴースト", "Broken Ghost"));
    for (int i = 0; i < 60; i++)
        wr_byte(0);
}

/*!
 * @brief クイック・スタート情報を書き込む / Save quick start data
 */
void save_quick_start(void)
{
    wr_byte(previous_char.psex);
    wr_byte((byte)previous_char.prace);
    wr_byte((byte)previous_char.pclass);
    wr_byte((byte)previous_char.ppersonality);
    wr_byte((byte)previous_char.realm1);
    wr_byte((byte)previous_char.realm2);

    wr_s16b(previous_char.age);
    wr_s16b(previous_char.ht);
    wr_s16b(previous_char.wt);
    wr_s16b(previous_char.sc);
    wr_s32b(previous_char.au);

    for (int i = 0; i < A_MAX; i++)
        wr_s16b(previous_char.stat_max[i]);

    for (int i = 0; i < A_MAX; i++)
        wr_s16b(previous_char.stat_max_max[i]);

    for (int i = 0; i < PY_MAX_LEVEL; i++)
        wr_s16b((int16_t)previous_char.player_hp[i]);

    wr_s16b(previous_char.chaos_patron);
    for (int i = 0; i < 8; i++)
        wr_s16b(previous_char.vir_types[i]);

    for (int i = 0; i < 4; i++)
        wr_string(previous_char.history[i]);

    /* UNUSED : Was number of random quests */
    wr_byte(0);
    if (w_ptr->noscore)
        previous_char.quick_ok = false;

    wr_byte((byte)previous_char.quick_ok);
}
