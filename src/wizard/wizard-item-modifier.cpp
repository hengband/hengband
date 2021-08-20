#include "wizard/wizard-item-modifier.h"
#include "artifact/fixed-art-generator.h"
#include "artifact/random-art-effects.h"
#include "artifact/random-art-generator.h"
#include "core/asking-player.h"
#include "core/show-file.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "core/stuff-handler.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/cheat-options.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "object-enchant/apply-magic.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-enchant.h"
#include "object-hook/hook-checker.h"
#include "object/item-use-flags.h"
#include "object/object-flags.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "object/object-kind-hook.h"
#include "object/object-mark-types.h"
#include "object/object-value.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "spell-kind/spells-perception.h"
#include "spell/spells-object.h"
#include "system/alloc-entries.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "view/display-messages.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "wizard/wizard-special-process.h"
#include "world/world.h"
#include <algorithm>
#include <limits>
#include <sstream>
#include <vector>

#define K_MAX_DEPTH 110 /*!< アイテムの階層毎生成率を表示する最大階 */

namespace {
/*!
 * @brief アイテム設定コマンド一覧表
 */
std::vector<std::vector<std::string>> wizard_sub_menu_table = {
    { "a", _("アーティファクト出現フラグリセット", "Restore aware flag of fixed artifact") },
    { "A", _("アーティファクトを出現済みにする", "Make a fixed artifact awared") },
    { "e", _("高級品獲得ドロップ", "Drop excellent object") },
    { "f", _("*鑑定*", "*Idenfity*") },
    { "i", _("鑑定", "Idenfity") },
    { "I", _("インベントリ全*鑑定*", "Idenfity all objects fully in inventory") },
    { "l", _("指定アイテム番号まで一括鑑定", "Make objects awared to target object id") },
    { "g", _("上質なアイテムドロップ", "Drop good object") },
    { "s", _("特別品獲得ドロップ", "Drop special object") },
    { "w", _("願い", "Wishing") },
    { "U", _("発動を変更する", "Modify item activation") },
};

/*!
 * @brief ゲーム設定コマンドの一覧を表示する
 */
void display_wizard_sub_menu()
{
    for (size_t y = 1; y <= wizard_sub_menu_table.size(); y++)
        term_erase(14, y, 64);

    int r = 1;
    int c = 15;
    int sz = wizard_sub_menu_table.size();
    for (int i = 0; i < sz; i++) {
        std::stringstream ss;
        ss << wizard_sub_menu_table[i][0] << ") " << wizard_sub_menu_table[i][1];
        put_str(ss.str().c_str(), r++, c);
    }
}
}

/*!
 * @brief キャスト先の型の最小値、最大値でclampする。
 */
template <typename T>
T clamp_cast(int val)
{
    return static_cast<T>(std::clamp(val,
        static_cast<int>(std::numeric_limits<T>::min()),
        static_cast<int>(std::numeric_limits<T>::max())));
}

void wiz_restore_aware_flag_of_fixed_arfifact(ARTIFACT_IDX a_idx, bool aware = false);
void wiz_modify_item_activation(player_type *caster_ptr);
void wiz_identify_full_inventory(player_type *caster_ptr);

/*!
    * @brief ゲーム設定コマンドの入力を受け付ける
    * @param creature_ptr プレイヤーの情報へのポインタ
       */
void wizard_item_modifier(player_type *creature_ptr)
{
    screen_save();
    display_wizard_sub_menu();

    char cmd;
    get_com("Player Command: ", &cmd, false);
    screen_load();

    switch (cmd) {
    case ESCAPE:
    case ' ':
    case '\n':
    case '\r':
        break;
    case 'a':
        wiz_restore_aware_flag_of_fixed_arfifact(command_arg);
        break;
    case 'A':
        wiz_restore_aware_flag_of_fixed_arfifact(command_arg, true);
        break;
    case 'e':
        if (command_arg <= 0)
            command_arg = 1;

        acquirement(creature_ptr, creature_ptr->y, creature_ptr->x, command_arg, true, false, true);
        break;
    case 'f':
        identify_fully(creature_ptr, false, TV_NONE);
        break;
    case 'g':
        if (command_arg <= 0)
            command_arg = 1;

        acquirement(creature_ptr, creature_ptr->y, creature_ptr->x, command_arg, false, false, true);
        break;
    case 'i':
        (void)ident_spell(creature_ptr, false, TV_NONE);
        break;
    case 'I':
        wiz_identify_full_inventory(creature_ptr);
        break;
    case 'l':
        wiz_learn_items_all(creature_ptr);
        break;
    case 's':
        if (command_arg <= 0)
            command_arg = 1;

        acquirement(creature_ptr, creature_ptr->y, creature_ptr->x, command_arg, true, true, true);
        break;
    case 'U':
        wiz_modify_item_activation(creature_ptr);
        break;
    case 'w':
        do_cmd_wishing(creature_ptr, -1, true, true, true);
        break;
    }
}

/*!
 * @brief 固定アーティファクトの出現フラグをリセットする
 * @param a_idx 指定したアーティファクトID
 */
void wiz_restore_aware_flag_of_fixed_arfifact(ARTIFACT_IDX a_idx, bool aware)
{
    if (a_idx <= 0) {
        char tmp[80] = "";
        sprintf(tmp, "Artifact ID (1-%d): ", max_a_idx - 1);
        char tmp_val[10] = "";
        if (!get_string(tmp, tmp_val, 3))
            return;

        a_idx = (ARTIFACT_IDX)atoi(tmp_val);
    }

    if (a_idx <= 0 || a_idx >= max_a_idx) {
        msg_format(_("番号は1から%dの間で指定して下さい。", "ID must be between 1 to %d."), max_a_idx - 1);
        return;
    }

    auto *a_ptr = &a_info[a_idx];
    a_ptr->cur_num = aware ? 1 : 0;
    msg_print(aware ? "Modified." : "Restored.");
}

/*!
 * @brief オブジェクトに発動を追加する/変更する
 * @param catser_ptr プレイヤー情報への参照ポインタ
 */
void wiz_modify_item_activation(player_type *caster_ptr)
{
    concptr q = "Which object? ";
    concptr s = "Nothing to do with.";
    OBJECT_IDX item;
    auto *o_ptr = choose_object(caster_ptr, &item, q, s, USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT, TV_NONE);
    if (!o_ptr)
        return;

    XTRA16 act_idx;
    char tmp[80] = "";
    sprintf(tmp, "Artifact ID (1-%d): ", ACT_MAX);
    char tmp_val[10] = "";
    if (!get_string(tmp, tmp_val, 3))
        return;

    act_idx = (XTRA16)atoi(tmp_val);

    if (act_idx <= 0 || act_idx > ACT_MAX) {
        msg_format(_("番号は1から%dの間で指定して下さい。", "ID must be between 1 to %d."), ACT_MAX - 1);
        return;
    }

    add_flag(o_ptr->art_flags, TR_ACTIVATE);
    o_ptr->xtra2 = act_idx;
}

/*!
 * @brief インベントリ内のアイテムを全て*鑑定*済みにする
 * @param catser_ptr プレイヤー情報への参照ポインタ
 */
void wiz_identify_full_inventory(player_type *caster_ptr)
{
    for (int i = 0; i < INVEN_TOTAL; i++) {
        object_type *o_ptr = &caster_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        auto k_ptr = &k_info[o_ptr->k_idx];
        k_ptr->aware = true; //!< @note 記録には残さないためTRUEを立てるのみ
        set_bits(o_ptr->ident, IDENT_KNOWN | IDENT_FULL_KNOWN);
        set_bits(o_ptr->marked, OM_TOUCHED);
    }

    /* Refrect item informaiton onto subwindows without updating inventory */
    reset_bits(caster_ptr->update, PU_COMBINE | PU_REORDER);
    handle_stuff(caster_ptr);
    set_bits(caster_ptr->update, PU_COMBINE | PU_REORDER);
    set_bits(caster_ptr->window_flags, PW_INVEN | PW_EQUIP);
}

/*!
 * @brief アイテムの階層毎生成率を表示する / Output a rarity graph for a type of object.
 * @param tval ベースアイテムの大項目ID
 * @param sval ベースアイテムの小項目ID
 * @param row 表示列
 * @param col 表示行
 */
static void prt_alloc(tval_type tval, OBJECT_SUBTYPE_VALUE sval, TERM_LEN row, TERM_LEN col)
{
    uint32_t rarity[K_MAX_DEPTH];
    (void)C_WIPE(rarity, K_MAX_DEPTH, uint32_t);
    uint32_t total[K_MAX_DEPTH];
    (void)C_WIPE(total, K_MAX_DEPTH, uint32_t);
    int32_t display[22];
    (void)C_WIPE(display, 22, int32_t);

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
 */
static void prt_binary(BIT_FLAGS flags, const int row, int col)
{
    uint32_t bitmask;
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
 */
static void wiz_display_item(player_type *player_ptr, object_type *o_ptr)
{
    TrFlags flgs;
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

    uint32_t i, matches, better, worse, other, correct;
    uint32_t test_roll = 1000000;
    char ch;
    concptr quality;
    BIT_FLAGS mode;
    while (true) {
        concptr pmt = "Roll for [n]ormal, [g]ood, or [e]xcellent treasure? ";
        wiz_display_item(caster_ptr, o_ptr);
        if (!get_com(pmt, &ch, false))
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
                inkey_scan = true;
                if (inkey()) {
                    flush();
                    break; // stop rolling
                }

                prt(format(q, i, correct, matches, better, worse, other), 0, 0);
                term_fresh();
            }

            object_type forge;
            object_type *q_ptr = &forge;
            q_ptr->wipe();
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
 */
static void wiz_reroll_item(player_type *owner_ptr, object_type *o_ptr)
{
    if (object_is_artifact(o_ptr))
        return;

    object_type forge;
    object_type *q_ptr;
    q_ptr = &forge;
    q_ptr->copy_from(o_ptr);

    char ch;
    bool changed = false;
    while (true) {
        wiz_display_item(owner_ptr, q_ptr);
        if (!get_com("[a]ccept, [w]orthless, [c]ursed, [n]ormal, [g]ood, [e]xcellent, [s]pecial? ", &ch, false)) {
            if (object_is_fixed_artifact(q_ptr)) {
                a_info[q_ptr->name1].cur_num = 0;
                q_ptr->name1 = 0;
            }

            changed = false;
            break;
        }

        if (ch == 'A' || ch == 'a') {
            changed = true;
            break;
        }

        if (object_is_fixed_artifact(q_ptr)) {
            a_info[q_ptr->name1].cur_num = 0;
            q_ptr->name1 = 0;
        }

        switch (tolower(ch)) {
        /* Apply bad magic, but first clear object */
        case 'w':
            q_ptr->prep(owner_ptr, o_ptr->k_idx);
            apply_magic_to_object(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART | AM_GOOD | AM_GREAT | AM_CURSED);
            break;
        /* Apply bad magic, but first clear object */
        case 'c':
            q_ptr->prep(owner_ptr, o_ptr->k_idx);
            apply_magic_to_object(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART | AM_GOOD | AM_CURSED);
            break;
        /* Apply normal magic, but first clear object */
        case 'n':
            q_ptr->prep(owner_ptr, o_ptr->k_idx);
            apply_magic_to_object(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART);
            break;
        /* Apply good magic, but first clear object */
        case 'g':
            q_ptr->prep(owner_ptr, o_ptr->k_idx);
            apply_magic_to_object(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART | AM_GOOD);
            break;
        /* Apply great magic, but first clear object */
        case 'e':
            q_ptr->prep(owner_ptr, o_ptr->k_idx);
            apply_magic_to_object(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART | AM_GOOD | AM_GREAT);
            break;
        /* Apply special magic, but first clear object */
        case 's':
            q_ptr->prep(owner_ptr, o_ptr->k_idx);
            apply_magic_to_object(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_GOOD | AM_GREAT | AM_SPECIAL);
            if (!object_is_artifact(q_ptr))
                become_random_artifact(owner_ptr, q_ptr, false);

            break;
        default:
            break;
        }

        q_ptr->iy = o_ptr->iy;
        q_ptr->ix = o_ptr->ix;
        q_ptr->marked = o_ptr->marked;
    }

    if (!changed)
        return;

    o_ptr->copy_from(q_ptr);
    set_bits(owner_ptr->update, PU_BONUS | PU_COMBINE | PU_REORDER);
    set_bits(owner_ptr->window_flags, PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER | PW_FLOOR_ITEM_LIST);
}

/*!
 * @briefアイテムの基礎能力値を調整する / Tweak an item
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 調整するアイテムの参照ポインタ
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

    o_ptr->pval = clamp_cast<int16_t>(atoi(tmp_val));
    wiz_display_item(player_ptr, o_ptr);
    p = "Enter new 'to_a' setting: ";
    sprintf(tmp_val, "%d", o_ptr->to_a);
    if (!get_string(p, tmp_val, 5))
        return;

    o_ptr->to_a = clamp_cast<int16_t>(atoi(tmp_val));
    wiz_display_item(player_ptr, o_ptr);
    p = "Enter new 'to_h' setting: ";
    sprintf(tmp_val, "%d", o_ptr->to_h);
    if (!get_string(p, tmp_val, 5))
        return;

    o_ptr->to_h = clamp_cast<int16_t>(atoi(tmp_val));
    wiz_display_item(player_ptr, o_ptr);
    p = "Enter new 'to_d' setting: ";
    sprintf(tmp_val, "%d", (int)o_ptr->to_d);
    if (!get_string(p, tmp_val, 5))
        return;

    o_ptr->to_d = clamp_cast<int16_t>(atoi(tmp_val));
    wiz_display_item(player_ptr, o_ptr);
}

/*!
 * @brief 検査対象のアイテムの数を変更する /
 * Change the quantity of a the item
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 変更するアイテム情報構造体の参照ポインタ
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
    o_ptr = choose_object(creature_ptr, &item, q, s, USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT, TV_NONE);
    if (!o_ptr)
        return;

    screen_save();

    object_type forge;
    object_type *q_ptr;
    q_ptr = &forge;
    q_ptr->copy_from(o_ptr);
    char ch;
    bool changed = false;
    while (true) {
        wiz_display_item(creature_ptr, q_ptr);
        if (!get_com("[a]ccept [s]tatistics [r]eroll [t]weak [q]uantity? ", &ch, false)) {
            changed = false;
            break;
        }

        if (ch == 'A' || ch == 'a') {
            changed = true;
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

    screen_load();
    if (changed) {
        msg_print("Changes accepted.");

        o_ptr->copy_from(q_ptr);
        set_bits(creature_ptr->update, PU_BONUS | PU_COMBINE | PU_REORDER);
        set_bits(creature_ptr->window_flags, PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER | PW_FLOOR_ITEM_LIST);
    } else {
        msg_print("Changes ignored.");
    }
}

/*!
 * @brief オブジェクトの装備スロットがエゴが有効なスロットかどうか判定
 */
static int is_slot_able_to_be_ego(player_type *caster_ptr, object_type *o_ptr)
{
    int slot = wield_slot(caster_ptr, o_ptr);

    if (slot > -1)
        return slot;

    if ((o_ptr->tval == TV_SHOT) || (o_ptr->tval == TV_ARROW) || (o_ptr->tval == TV_BOLT))
        return (INVEN_AMMO);

    return (-1);
}

/*!
 * @brief 願ったが消えてしまった場合のメッセージ 
 */
static void wishing_puff_of_smoke(void)
{
    msg_print(_("何かが足下に転がってきたが、煙のように消えてしまった。",
        "You feel something roll beneath your feet, but it disappears in a puff of smoke!"));
}

/*!
 * @brief 願ったが消えてしまった場合のメッセージ
 * @param caster_ptr 願ったプレイヤー情報への参照ポインタ
 * @param prob ★などを願った場合の生成確率
 * @param art_ok アーティファクトの生成を許すならTRUE
 * @param ego_ok エゴの生成を許すならTRUE
 * @param confirm 願わない場合に確認するかどうか
 * @return 願った結果
 */
WishResult do_cmd_wishing(player_type *caster_ptr, int prob, bool allow_art, bool allow_ego, bool confirm)
{
    concptr fixed_str[] = {
#ifdef JP
        "燃えない",
        "錆びない",
        "腐食しない",
        "安定した",
#else
        "rotproof",
        "fireproof",
        "rustproof",
        "erodeproof",
        "corrodeproof",
        "fixed",
#endif
        NULL,
    };

    char buf[MAX_NLEN] = "\0";
    char *str = buf;
    object_type forge;
    object_type *o_ptr = &forge;
    char o_name[MAX_NLEN];

    bool wish_art = false;
    bool wish_randart = false;
    bool wish_ego = false;
    bool exam_base = true;
    bool ok_art = (randint0(100) < prob) ? true : false;
    bool ok_ego = (randint0(100) < 50 + prob) ? true : false;
    bool must = (prob < 0) ? true : false;
    bool blessed = false;
    bool fixed = true;

    while (1) {
        if (get_string(_("何をお望み？ ", "For what do you wish?"), buf, (MAX_NLEN - 1)))
            break;
        if (confirm) {
            if (!get_check(_("何も願いません。本当によろしいですか？", "Do you wish nothing, really? ")))
                continue;
        }
        return WishResult::NOTHING;
    }

#ifndef JP
    str_tolower(str);

    /* remove 'a' */
    if (!strncmp(buf, "a ", 2))
        str = ltrim(str + 1);
    else if (!strncmp(buf, "an ", 3))
        str = ltrim(str + 2);
#endif // !JP

    str = rtrim(str);

    if (!strncmp(str, _("祝福された", "blessed"), _(10, 7))) {
        str = ltrim(str + _(10, 7));
        blessed = true;
    }

    for (int i = 0; fixed_str[i] != NULL; i++) {
        int len = strlen(fixed_str[i]);
        if (!strncmp(str, fixed_str[i], len)) {
            str = ltrim(str + len);
            fixed = true;
            break;
        }
    }

#ifdef JP
    if (!strncmp(str, "★", 2)) {
        str = ltrim(str + 2);
        wish_art = true;
        exam_base = false;
    } else
#endif

    if (!strncmp(str, _("☆", "The "), _(2, 4))) {
        str = ltrim(str + _(2, 4));
        wish_art = true;
        wish_randart = true;
    }

    /* wishing random ego ? */
    else if (!strncmp(str, _("高級な", "excellent "), _(6, 9))) {
        str = ltrim(str + _(6, 9));
        wish_ego = true;
    }

    if (strlen(str) < 1) {
        msg_print(_("名前がない！", "What?"));
        return WishResult::NOTHING;
    }

    if (!allow_art && wish_art) {
        msg_print(_("アーティファクトは願えない!", "You can not wish artifacts!"));
        return WishResult::NOTHING;
    }

    if (cheat_xtra)
        msg_format("Wishing %s....", buf);

    std::vector<KIND_OBJECT_IDX> k_ids;
    std::vector<EGO_IDX> e_ids;
    if (exam_base) {
        int len;
        int max_len = 0;
        for (KIND_OBJECT_IDX k = 1; k < max_k_idx; k++) {
            object_kind *k_ptr = &k_info[k];
            if (k_ptr->name.empty())
                continue;

            o_ptr->prep(caster_ptr, k);
            describe_flavor(caster_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY | OD_STORE));
#ifndef JP
            str_tolower(o_name);
#endif
            if (cheat_xtra)
                msg_format("Matching object No.%d %s", k, o_name);

            len = strlen(o_name);

            if (_(!strrncmp(str, o_name, len), !strncmp(str, o_name, len))) {
                if (len > max_len) {
                    k_ids.push_back(k);
                    max_len = len;
                }
            }
        }

        if (allow_ego && k_ids.size() == 1) {
            KIND_OBJECT_IDX k_idx = k_ids.back();
            o_ptr->prep(caster_ptr, k_idx);

            for (EGO_IDX k = 1; k < max_e_idx; k++) {
                ego_item_type *e_ptr = &e_info[k];
                if (e_ptr->name.empty())
                    continue;

                strcpy(o_name, e_ptr->name.c_str());
#ifndef JP
                str_tolower(o_name);
#endif
                if (cheat_xtra)
                    msg_format("Mathcing ego No.%d %s...", k, o_name);

                if (_(!strncmp(str, o_name, strlen(o_name)), !strrncmp(str, o_name, strlen(o_name)))) {
                    if (is_slot_able_to_be_ego(caster_ptr, o_ptr) != e_ptr->slot)
                        continue;

                    e_ids.push_back(k);
                }
            }
        }
    }

    std::vector<ARTIFACT_IDX> a_ids;

    if (allow_art) {
        char a_desc[MAX_NLEN] = "\0";
        char *a_str = a_desc;

        int len;
        int mlen = 0;
        for (ARTIFACT_IDX i = 1; i < max_a_idx; i++) {
            artifact_type *a_ptr = &a_info[i];
            if (a_ptr->name.empty())
                continue;

            KIND_OBJECT_IDX k_idx = lookup_kind(a_ptr->tval, a_ptr->sval);
            if (!k_idx)
                continue;

            o_ptr->prep(caster_ptr, k_idx);
            o_ptr->name1 = i;

            describe_flavor(caster_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY | OD_STORE));
#ifndef JP
            str_tolower(o_name);
#endif
            a_str = a_desc;
            strcpy(a_desc, a_ptr->name.c_str());

            if (*a_str == '$')
                a_str++;
#ifdef JP
            /* remove quotes */
            if (!strncmp(a_str, "『", 2)) {
                a_str += 2;
                char *s = strstr(a_str, "』");
                *s = '\0';
            }
            /* remove 'of' */
            else {
                int l = strlen(a_str);
                if (!strrncmp(a_str, "の", 2)) {
                    a_str[l - 2] = '\0';
                }
            }
#else
            /* remove quotes */
            if (a_str[0] == '\'') {
                a_str += 1;
                char *s = strchr(a_desc, '\'');
                *s = '\0';
            }
            /* remove 'of ' */
            else if (!strncmp(a_str, (const char *)"of ", 3)) {
                a_str += 3;
            }

            str_tolower(a_str);
#endif

            if (cheat_xtra)
                msg_format("Matching artifact No.%d %s(%s)", i, a_desc, _(&o_name[2], o_name));

            std::vector<const char *> l = { a_str, a_ptr->name.c_str(), _(&o_name[2], o_name) };
            for (size_t c = 0; c < l.size(); c++) {
                if (!strcmp(str, l.at(c))) {
                    len = strlen(l.at(c));
                    if (len > mlen) {
                        a_ids.push_back(i);
                        mlen = len;
                    }
                }
            }
        }
    }

    if (current_world_ptr->wizard && (a_ids.size() > 1 || e_ids.size() > 1)) {
        msg_print(_("候補が多すぎる！", "Too many matches!"));
        return WishResult::FAIL;
    }
    
    if (a_ids.size() == 1) {
        ARTIFACT_IDX a_idx = a_ids.back();
        if (must || (ok_art && !a_info[a_idx].cur_num)) {
            create_named_art(caster_ptr, a_idx, caster_ptr->y, caster_ptr->x);
            if (!current_world_ptr->wizard)
                a_info[a_idx].cur_num = 1;
        }
        else
            wishing_puff_of_smoke();
        return WishResult::ARTIFACT;
    }
    
    if (!allow_ego && (wish_ego || e_ids.size() > 0)) {
        msg_print(_("エゴアイテムは願えない！", "Can not wish ego item."));
        return WishResult::NOTHING;
    }
    
    if (k_ids.size() == 1) {
        KIND_OBJECT_IDX k_idx = k_ids.back();
        object_kind *k_ptr = &k_info[k_idx];

        artifact_type *a_ptr;
        ARTIFACT_IDX a_idx = 0;
        if (k_ptr->gen_flags.has(TRG::INSTA_ART)) {
            for (ARTIFACT_IDX i = 1; i < max_a_idx; i++) {
                a_ptr = &a_info[i];
                if (a_ptr->tval != k_ptr->tval || a_ptr->sval != k_ptr->sval)
                    continue;
                a_idx = i;
                break;
            }
        }

        if (a_idx > 0) {
            a_ptr = &a_info[a_idx];
            if (must || (ok_art && !a_ptr->cur_num)) {
                create_named_art(caster_ptr, a_idx, caster_ptr->y, caster_ptr->x);
                if (!current_world_ptr->wizard)
                    a_info[a_idx].cur_num = 1;
            }
            else
                wishing_puff_of_smoke();
            return WishResult::ARTIFACT;
        }

        if (wish_randart) {
            if (must || ok_art) {
                do {
                    o_ptr->prep(caster_ptr, k_idx);
                    apply_magic_to_object(caster_ptr, o_ptr, k_ptr->level, (AM_SPECIAL | AM_NO_FIXED_ART));
                } while (!o_ptr->art_name || o_ptr->name1 || o_ptr->name2 || object_is_cursed(o_ptr));

                if (o_ptr->art_name)
                    drop_near(caster_ptr, o_ptr, -1, caster_ptr->y, caster_ptr->x);
            } else {
                wishing_puff_of_smoke();
            }
            return WishResult::ARTIFACT;
        }

        WishResult res = WishResult::NOTHING;
        if (allow_ego && (wish_ego || e_ids.size() > 0)) {
            if (must || ok_ego) {
                if (e_ids.size() > 0) {
                    o_ptr->prep(caster_ptr, k_idx);
                    o_ptr->name2 = e_ids[0];
                    apply_ego(caster_ptr, o_ptr, caster_ptr->current_floor_ptr->base_level);
                } else {
                    int max_roll = 1000;
                    int i = 0;
                    for (i = 0; i < max_roll; i++) {
                        o_ptr->prep(caster_ptr, k_idx);
                        (void)apply_magic_to_object(caster_ptr, o_ptr, k_ptr->level, (AM_GREAT | AM_NO_FIXED_ART));

                        if (o_ptr->name1 || o_ptr->art_name)
                            continue;

                        if (wish_ego)
                            break;

                        EGO_IDX e_idx = 0;
                        for (auto e : e_ids) {
                            if (o_ptr->name2 == e) {
                                e_idx = e;
                                break;
                            }
                        }

                        if (e_idx != 0)
                            break;
                    }

                    if (i == max_roll) {
                        msg_print(_("失敗！もう一度願ってみてください。", "Failed! Try again."));
                        return WishResult::FAIL;
                    }
                }
            } else {
                wishing_puff_of_smoke();
            }
            
            res = WishResult::EGO;
        } else {
            for (int i = 0; i < 100; i++) {
                o_ptr->prep(caster_ptr, k_idx);
                apply_magic_to_object(caster_ptr, o_ptr, 0, (AM_NO_FIXED_ART));
                if (!object_is_cursed(o_ptr))
                    break;
            }
            res = WishResult::NORMAL;
        }

        if (blessed && wield_slot(caster_ptr, o_ptr) != -1)
            add_flag(o_ptr->art_flags, TR_BLESSED);

        if (fixed && wield_slot(caster_ptr, o_ptr) != -1) {
            add_flag(o_ptr->art_flags, TR_IGNORE_ACID);
            add_flag(o_ptr->art_flags, TR_IGNORE_FIRE);
        }

        (void)drop_near(caster_ptr, o_ptr, -1, caster_ptr->y, caster_ptr->x);

        return res;
    }

    msg_print(_("うーん、そんなものは存在しないようだ。", "Ummmm, that is not existing..."));
    return WishResult::FAIL;
}
