#include "wizard/wizard-item-modifier.h"
#include "artifact/random-art-generator.h"
#include "core/asking-player.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "io/input-key-acceptor.h"
#include "object-enchant/apply-magic.h"
#include "object-enchant/item-apply-magic.h"
#include "object-hook/hook-enchant.h"
#include "object/item-use-flags.h"
#include "object/object-flags.h"
#include "object/object-generator.h"
#include "object/object-kind.h"
#include "object/object-value.h"
#include "system/alloc-entries.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "view/display-messages.h"

#define K_MAX_DEPTH 110 /*!< アイテムの階層毎生成率を表示する最大階 */

/*!
 * @brief アイテムの階層毎生成率を表示する / Output a rarity graph for a type of object.
 * @param tval ベースアイテムの大項目ID
 * @param sval ベースアイテムの小項目ID
 * @param row 表示列
 * @param col 表示行
 * @return なし
 */
static void prt_alloc(tval_type tval, OBJECT_SUBTYPE_VALUE sval, TERM_LEN row, TERM_LEN col)
{
    u32b rarity[K_MAX_DEPTH];
    (void)C_WIPE(rarity, K_MAX_DEPTH, u32b);
    u32b total[K_MAX_DEPTH];
    (void)C_WIPE(total, K_MAX_DEPTH, u32b);
    s32b display[22];
    (void)C_WIPE(display, 22, s32b);

    int home = 0;
    for (int i = 0; i < K_MAX_DEPTH; i++) {
        int total_frac = 0;
        object_kind *k_ptr;
        alloc_entry *table = alloc_kind_table;
        for (int j = 0; j < alloc_kind_size; j++) {
            PERCENTAGE prob = 0;

            if (table[j].level <= i) {
                prob = table[j].prob1 * GREAT_OBJ * K_MAX_DEPTH;
            } else if (table[j].level - 1 > 0) {
                prob = table[j].prob1 * i * K_MAX_DEPTH / (table[j].level - 1);
            }

            k_ptr = &k_info[table[j].index];

            total[i] += prob / (GREAT_OBJ * K_MAX_DEPTH);
            total_frac += prob % (GREAT_OBJ * K_MAX_DEPTH);

            if ((k_ptr->tval == tval) && (k_ptr->sval == sval)) {
                home = k_ptr->level;
                rarity[i] += prob / (GREAT_OBJ * K_MAX_DEPTH);
            }
        }

        total[i] += total_frac / (GREAT_OBJ * K_MAX_DEPTH);
    }

    for (int i = 0; i < 22; i++) {
        int possibility = 0;
        for (int j = i * K_MAX_DEPTH / 22; j < (i + 1) * K_MAX_DEPTH / 22; j++)
            possibility += rarity[j] * 100000 / total[j];

        display[i] = possibility / 5;
    }

    for (int i = 0; i < 22; i++) {
        term_putch(col, row + i + 1, TERM_WHITE, '|');
        prt(format("%2dF", (i * 5)), row + i + 1, col);
        if ((i * K_MAX_DEPTH / 22 <= home) && (home < (i + 1) * K_MAX_DEPTH / 22))
            c_prt(TERM_RED, format("%3d.%04d%%", display[i] / 1000, display[i] % 1000), row + i + 1, col + 3);
        else
            c_prt(TERM_WHITE, format("%3d.%04d%%", display[i] / 1000, display[i] % 1000), row + i + 1, col + 3);
    }

    concptr r = "+---Rate---+";
    prt(r, row, col);
}

/*!
 * @brief 32ビット変数のビット配列を並べて描画する / Output a long int in binary format.
 * @return なし
 */
static void prt_binary(BIT_FLAGS flags, const int row, int col)
{
    u32b bitmask;
    for (int i = bitmask = 1; i <= 32; i++, bitmask *= 2)
        if (flags & bitmask)
            term_putch(col++, row, TERM_BLUE, '*');
        else
            term_putch(col++, row, TERM_WHITE, '-');
}

/*!
 * @brief アイテムの詳細ステータスを表示する /
 * Change various "permanent" player variables.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 詳細を表示するアイテム情報の参照ポインタ
 * @return なし
 */
static void wiz_display_item(player_type *player_ptr, object_type *o_ptr)
{
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    object_flags(player_ptr, o_ptr, flgs);
    int j = 13;
    for (int i = 1; i <= 23; i++)
        prt("", i, j - 2);

    prt_alloc(o_ptr->tval, o_ptr->sval, 1, 0);
    char buf[256];
    describe_flavor(player_ptr, buf, o_ptr, OD_STORE);
    prt(buf, 2, j);
    prt(format("kind = %-5d  level = %-4d  tval = %-5d  sval = %-5d", o_ptr->k_idx, k_info[o_ptr->k_idx].level, o_ptr->tval, o_ptr->sval), 4, j);
    prt(format("number = %-3d  wgt = %-6d  ac = %-5d    damage = %dd%d", o_ptr->number, o_ptr->weight, o_ptr->ac, o_ptr->dd, o_ptr->ds), 5, j);
    prt(format("pval = %-5d  toac = %-5d  tohit = %-4d  todam = %-4d", o_ptr->pval, o_ptr->to_a, o_ptr->to_h, o_ptr->to_d), 6, j);
    prt(format("name1 = %-4d  name2 = %-4d  cost = %ld", o_ptr->name1, o_ptr->name2, (long)object_value_real(player_ptr, o_ptr)), 7, j);
    prt(format("ident = %04x  xtra1 = %-4d  xtra2 = %-4d  timeout = %-d", o_ptr->ident, o_ptr->xtra1, o_ptr->xtra2, o_ptr->timeout), 8, j);
    prt(format("xtra3 = %-4d  xtra4 = %-4d  xtra5 = %-4d  cursed  = %-d", o_ptr->xtra3, o_ptr->xtra4, o_ptr->xtra5, o_ptr->curse_flags), 9, j);

    prt("+------------FLAGS1------------+", 10, j);
    prt("AFFECT........SLAY........BRAND.", 11, j);
    prt("      mf      cvae      xsqpaefc", 12, j);
    prt("siwdccsossidsahanvudotgddhuoclio", 13, j);
    prt("tnieohtctrnipttmiinmrrnrrraiierl", 14, j);
    prt("rtsxnarelcfgdkcpmldncltggpksdced", 15, j);
    prt_binary(flgs[0], 16, j);

    prt("+------------FLAGS2------------+", 17, j);
    prt("SUST....IMMUN.RESIST............", 18, j);
    prt("      reaefctrpsaefcpfldbc sn   ", 19, j);
    prt("siwdcciaclioheatcliooeialoshtncd", 20, j);
    prt("tnieohdsierlrfraierliatrnnnrhehi", 21, j);
    prt("rtsxnaeydcedwlatdcedsrekdfddrxss", 22, j);
    prt_binary(flgs[1], 23, j);

    prt("+------------FLAGS3------------+", 10, j + 32);
    prt("fe cnn t      stdrmsiiii d ab   ", 11, j + 32);
    prt("aa aoomywhs lleeieihgggg rtgl   ", 12, j + 32);
    prt("uu utmacaih eielgggonnnnaaere   ", 13, j + 32);
    prt("rr reanurdo vtieeehtrrrrcilas   ", 14, j + 32);
    prt("aa algarnew ienpsntsaefctnevs   ", 15, j + 32);
    prt_binary(flgs[2], 16, j + 32);

    prt("+------------FLAGS4------------+", 17, j + 32);
    prt("KILL....ESP.........            ", 18, j + 32);
    prt("aeud tghaud tgdhegnu            ", 19, j + 32);
    prt("nvneoriunneoriruvoon            ", 20, j + 32);
    prt("iidmroamidmroagmionq            ", 21, j + 32);
    prt("mlenclnmmenclnnnldlu            ", 22, j + 32);
    prt_binary(flgs[3], 23, j + 32);
}

/*!
 * @brief 検査対象のアイテムを基準とした生成テストを行う /
 * Try to create an item again. Output some statistics.    -Bernd-
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 生成テストの基準となるアイテム情報の参照ポインタ
 * @return なし
 * The statistics are correct now.  We acquire a clean grid, and then
 * repeatedly place an object in this grid, copying it into an item
 * holder, and then deleting the object.  We fiddle with the artifact
 * counter flags to prevent weirdness.  We use the items to collect
 * statistics on item creation relative to the initial item.
 */
static void wiz_statistics(player_type *caster_ptr, object_type *o_ptr)
{
    concptr q = "Rolls: %ld  Correct: %ld  Matches: %ld  Better: %ld  Worse: %ld  Other: %ld";
    concptr p = "Enter number of items to roll: ";
    char tmp_val[80];

    if (object_is_fixed_artifact(o_ptr))
        a_info[o_ptr->name1].cur_num = 0;

    u32b i, matches, better, worse, other, correct;
    u32b test_roll = 1000000;
    char ch;
    concptr quality;
    BIT_FLAGS mode;
    while (TRUE) {
        concptr pmt = "Roll for [n]ormal, [g]ood, or [e]xcellent treasure? ";
        wiz_display_item(caster_ptr, o_ptr);
        if (!get_com(pmt, &ch, FALSE))
            break;

        if (ch == 'n' || ch == 'N') {
            mode = 0L;
            quality = "normal";
        } else if (ch == 'g' || ch == 'G') {
            mode = AM_GOOD;
            quality = "good";
        } else if (ch == 'e' || ch == 'E') {
            mode = AM_GOOD | AM_GREAT;
            quality = "excellent";
        } else {
            break;
        }

        sprintf(tmp_val, "%ld", (long int)test_roll);
        if (get_string(p, tmp_val, 10))
            test_roll = atol(tmp_val);
        test_roll = MAX(1, test_roll);
        msg_format("Creating a lot of %s items. Base level = %d.", quality, caster_ptr->current_floor_ptr->dun_level);
        msg_print(NULL);

        correct = matches = better = worse = other = 0;
        for (i = 0; i <= test_roll; i++) {
            if ((i < 100) || (i % 100 == 0)) {
                inkey_scan = TRUE;
                if (inkey()) {
                    flush();
                    break; // stop rolling
                }

                prt(format(q, i, correct, matches, better, worse, other), 0, 0);
                term_fresh();
            }

            object_type forge;
            object_type *q_ptr = &forge;
            object_wipe(q_ptr);
            make_object(caster_ptr, q_ptr, mode);
            if (object_is_fixed_artifact(q_ptr))
                a_info[q_ptr->name1].cur_num = 0;

            if ((o_ptr->tval != q_ptr->tval) || (o_ptr->sval != q_ptr->sval))
                continue;

            correct++;
            if ((q_ptr->pval == o_ptr->pval) && (q_ptr->to_a == o_ptr->to_a) && (q_ptr->to_h == o_ptr->to_h) && (q_ptr->to_d == o_ptr->to_d)
                && (q_ptr->name1 == o_ptr->name1)) {
                matches++;
            } else if ((q_ptr->pval >= o_ptr->pval) && (q_ptr->to_a >= o_ptr->to_a) && (q_ptr->to_h >= o_ptr->to_h) && (q_ptr->to_d >= o_ptr->to_d)) {
                better++;
            } else if ((q_ptr->pval <= o_ptr->pval) && (q_ptr->to_a <= o_ptr->to_a) && (q_ptr->to_h <= o_ptr->to_h) && (q_ptr->to_d <= o_ptr->to_d)) {
                worse++;
            } else {
                other++;
            }
        }

        msg_format(q, i, correct, matches, better, worse, other);
        msg_print(NULL);
    }

    if (object_is_fixed_artifact(o_ptr))
        a_info[o_ptr->name1].cur_num = 1;
}

/*!
 * @brief アイテムの質を選択して再生成する /
 * Apply magic to an item or turn it into an artifact. -Bernd-
 * @param o_ptr 再生成の対象となるアイテム情報の参照ポインタ
 * @return なし
 */
static void wiz_reroll_item(player_type *owner_ptr, object_type *o_ptr)
{
    if (object_is_artifact(o_ptr))
        return;

    object_type forge;
    object_type *q_ptr;
    q_ptr = &forge;
    object_copy(q_ptr, o_ptr);

    char ch;
    bool changed = FALSE;
    while (TRUE) {
        wiz_display_item(owner_ptr, q_ptr);
        if (!get_com("[a]ccept, [w]orthless, [c]ursed, [n]ormal, [g]ood, [e]xcellent, [s]pecial? ", &ch, FALSE)) {
            if (object_is_fixed_artifact(q_ptr)) {
                a_info[q_ptr->name1].cur_num = 0;
                q_ptr->name1 = 0;
            }

            changed = FALSE;
            break;
        }

        if (ch == 'A' || ch == 'a') {
            changed = TRUE;
            break;
        }

        if (object_is_fixed_artifact(q_ptr)) {
            a_info[q_ptr->name1].cur_num = 0;
            q_ptr->name1 = 0;
        }

        switch (tolower(ch)) {
        /* Apply bad magic, but first clear object */
        case 'w':
            object_prep(owner_ptr, q_ptr, o_ptr->k_idx);
            apply_magic(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART | AM_GOOD | AM_GREAT | AM_CURSED);
            break;
        /* Apply bad magic, but first clear object */
        case 'c':
            object_prep(owner_ptr, q_ptr, o_ptr->k_idx);
            apply_magic(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART | AM_GOOD | AM_CURSED);
            break;
        /* Apply normal magic, but first clear object */
        case 'n':
            object_prep(owner_ptr, q_ptr, o_ptr->k_idx);
            apply_magic(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART);
            break;
        /* Apply good magic, but first clear object */
        case 'g':
            object_prep(owner_ptr, q_ptr, o_ptr->k_idx);
            apply_magic(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART | AM_GOOD);
            break;
        /* Apply great magic, but first clear object */
        case 'e':
            object_prep(owner_ptr, q_ptr, o_ptr->k_idx);
            apply_magic(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART | AM_GOOD | AM_GREAT);
            break;
        /* Apply special magic, but first clear object */
        case 's':
            object_prep(owner_ptr, q_ptr, o_ptr->k_idx);
            apply_magic(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_GOOD | AM_GREAT | AM_SPECIAL);
            if (!object_is_artifact(q_ptr))
                become_random_artifact(owner_ptr, q_ptr, FALSE);

            break;
        default:
            break;
        }

        q_ptr->iy = o_ptr->iy;
        q_ptr->ix = o_ptr->ix;
        q_ptr->next_o_idx = o_ptr->next_o_idx;
        q_ptr->marked = o_ptr->marked;
    }

    if (!changed)
        return;

    object_copy(o_ptr, q_ptr);
    owner_ptr->update |= PU_BONUS;
    owner_ptr->update |= PU_COMBINE | PU_REORDER;
    owner_ptr->window |= PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER;
}

/*!
 * @briefアイテムの基礎能力値を調整する / Tweak an item
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 調整するアイテムの参照ポインタ
 * @return なし
 */
static void wiz_tweak_item(player_type *player_ptr, object_type *o_ptr)
{
    if (object_is_artifact(o_ptr))
        return;

    concptr p = "Enter new 'pval' setting: ";
    char tmp_val[80];
    sprintf(tmp_val, "%d", o_ptr->pval);
    if (!get_string(p, tmp_val, 5))
        return;

    o_ptr->pval = (s16b)atoi(tmp_val);
    wiz_display_item(player_ptr, o_ptr);
    p = "Enter new 'to_a' setting: ";
    sprintf(tmp_val, "%d", o_ptr->to_a);
    if (!get_string(p, tmp_val, 5))
        return;

    o_ptr->to_a = (s16b)atoi(tmp_val);
    wiz_display_item(player_ptr, o_ptr);
    p = "Enter new 'to_h' setting: ";
    sprintf(tmp_val, "%d", o_ptr->to_h);
    if (!get_string(p, tmp_val, 5))
        return;

    o_ptr->to_h = (s16b)atoi(tmp_val);
    wiz_display_item(player_ptr, o_ptr);
    p = "Enter new 'to_d' setting: ";
    sprintf(tmp_val, "%d", (int)o_ptr->to_d);
    if (!get_string(p, tmp_val, 5))
        return;

    o_ptr->to_d = (s16b)atoi(tmp_val);
    wiz_display_item(player_ptr, o_ptr);
}

/*!
 * @brief 検査対象のアイテムの数を変更する /
 * Change the quantity of a the item
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 変更するアイテム情報構造体の参照ポインタ
 * @return なし
 */
static void wiz_quantity_item(object_type *o_ptr)
{
    if (object_is_artifact(o_ptr))
        return;

    int tmp_qnt = o_ptr->number;
    char tmp_val[100];
    sprintf(tmp_val, "%d", (int)o_ptr->number);
    if (get_string("Quantity: ", tmp_val, 2)) {
        int tmp_int = atoi(tmp_val);
        if (tmp_int < 1)
            tmp_int = 1;

        if (tmp_int > 99)
            tmp_int = 99;

        o_ptr->number = (byte)tmp_int;
    }

    if (o_ptr->tval == TV_ROD)
        o_ptr->pval = o_ptr->pval * o_ptr->number / tmp_qnt;
}

/*!
 * @brief アイテムを弄るデバッグコマンド
 * Play with an item. Options include:
 * @return なし
 * @details
 *   - Output statistics (via wiz_roll_item)<br>
 *   - Reroll item (via wiz_reroll_item)<br>
 *   - Change properties (via wiz_tweak_item)<br>
 *   - Change the number of items (via wiz_quantity_item)<br>
 */
void wiz_modify_item(player_type *creature_ptr)
{
    concptr q = "Play with which object? ";
    concptr s = "You have nothing to play with.";
    OBJECT_IDX item;
    object_type *o_ptr;
    o_ptr = choose_object(creature_ptr, &item, q, s, USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT, 0);
    if (!o_ptr)
        return;

    screen_save(creature_ptr);

    object_type forge;
    object_type *q_ptr;
    q_ptr = &forge;
    object_copy(q_ptr, o_ptr);
    char ch;
    bool changed = FALSE;
    while (TRUE) {
        wiz_display_item(creature_ptr, q_ptr);
        if (!get_com("[a]ccept [s]tatistics [r]eroll [t]weak [q]uantity? ", &ch, FALSE)) {
            changed = FALSE;
            break;
        }

        if (ch == 'A' || ch == 'a') {
            changed = TRUE;
            break;
        }

        if (ch == 's' || ch == 'S') {
            wiz_statistics(creature_ptr, q_ptr);
        }

        if (ch == 'r' || ch == 'R') {
            wiz_reroll_item(creature_ptr, q_ptr);
        }

        if (ch == 't' || ch == 'T') {
            wiz_tweak_item(creature_ptr, q_ptr);
        }

        if (ch == 'q' || ch == 'Q') {
            wiz_quantity_item(q_ptr);
        }
    }

    screen_load(creature_ptr);
    if (changed) {
        msg_print("Changes accepted.");

        object_copy(o_ptr, q_ptr);
        creature_ptr->update |= PU_BONUS;
        creature_ptr->update |= PU_COMBINE | PU_REORDER;
        creature_ptr->window |= PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER;
    } else {
        msg_print("Changes ignored.");
    }
}
