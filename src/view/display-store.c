#include "view/display-store.h"
#include "flavor/flavor-describer.h"
#include "game-option/birth-options.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "store/store-util.h"
#include "store/store.h" // todo 相互依存している、こっちは残す？.
#include "system/object-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"

/*!
 * @brief 店の商品リストを再表示する /
 * Re-displays a single store entry
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param pos 表示行
 * @return なし
 */
void display_entry(player_type *player_ptr, int pos)
{
    object_type *o_ptr;
    o_ptr = &st_ptr->stock[pos];
    int i = (pos % store_bottom);

    /* Label it, clear the line --(-- */
    char out_val[160];
    (void)sprintf(out_val, "%c) ", ((i > 25) ? toupper(I2A(i - 26)) : I2A(i)));
    prt(out_val, i + 6, 0);

    int cur_col = 3;
    if (show_item_graph) {
        TERM_COLOR a = object_attr(o_ptr);
        SYMBOL_CODE c = object_char(o_ptr);

        term_queue_bigchar(cur_col, i + 6, a, c, 0, 0);
        if (use_bigtile)
            cur_col++;

        cur_col += 2;
    }

    /* Describe an item in the home */
    int maxwid = 75;
    if ((cur_store_num == STORE_HOME) || (cur_store_num == STORE_MUSEUM)) {
        maxwid = 75;
        if (show_weights)
            maxwid -= 10;

        GAME_TEXT o_name[MAX_NLEN];
        describe_flavor(player_ptr, o_name, o_ptr, 0);
        o_name[maxwid] = '\0';
        c_put_str(tval_to_attr[o_ptr->tval], o_name, i + 6, cur_col);
        if (show_weights) {
            WEIGHT wgt = o_ptr->weight;
            sprintf(out_val, _("%3d.%1d kg", "%3d.%d lb"), _(lbtokg1(wgt), wgt / 10), _(lbtokg2(wgt), wgt % 10));
            put_str(out_val, i + 6, _(67, 68));
        }

        return;
    }

    maxwid = 65;
    if (show_weights)
        maxwid -= 7;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, o_ptr, 0);
    o_name[maxwid] = '\0';
    c_put_str(tval_to_attr[o_ptr->tval], o_name, i + 6, cur_col);

    if (show_weights) {
        int wgt = o_ptr->weight;
        sprintf(out_val, "%3d.%1d", _(lbtokg1(wgt), wgt / 10), _(lbtokg2(wgt), wgt % 10));
        put_str(out_val, i + 6, _(60, 61));
    }

    s32b x;
    if (o_ptr->ident & IDENT_FIXED) {
        x = price_item(player_ptr, o_ptr, ot_ptr->min_inflate, FALSE);
        (void)sprintf(out_val, _("%9ld固", "%9ld F"), (long)x);
        put_str(out_val, i + 6, 68);
        return;
    }

    if (!manual_haggle) {
        x = price_item(player_ptr, o_ptr, ot_ptr->min_inflate, FALSE);
        if (!noneedtobargain(x))
            x += x / 10;

        (void)sprintf(out_val, "%9ld  ", (long)x);
        put_str(out_val, i + 6, 68);
        return;
    }

    x = price_item(player_ptr, o_ptr, ot_ptr->max_inflate, FALSE);
    (void)sprintf(out_val, "%9ld  ", (long)x);
    put_str(out_val, i + 6, 68);
}
