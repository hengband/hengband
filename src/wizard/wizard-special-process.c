/*!
 * @brief ウィザードモードの処理(特別処理中心) / Wizard commands
 * @date 2014/09/07
 * @author
 * Copyright (c) 1997 Ben Harrison, and others<br>
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.<br>
 * 2014 Deskull rearranged comment for Doxygen.<br>
 */

#include "wizard/wizard-special-process.h"
#include "artifact/fixed-art-generator.h"
#include "artifact/random-art-generator.h"
#include "birth/inventory-initializer.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-io/cmd-help.h"
#include "cmd-io/cmd-save.h"
#include "cmd-visual/cmd-draw.h"
#include "core/asking-player.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "flavor/object-flavor.h"
#include "floor/floor-object.h"
#include "floor/floor-save.h"
#include "floor/floor.h"
#include "game-option/option-types-table.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
#include "grid/grid.h"
#include "info-reader/fixed-map-parser.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "io/targeting.h"
#include "io/write-diary.h"
#include "market/arena.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/monster-summon.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "mutation/mutation.h"
#include "object-enchant/apply-magic.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object-hook/hook-enchant.h"
#include "object/item-use-flags.h"
#include "object/object-flags.h"
#include "object/object-generator.h"
#include "object/object-kind.h"
#include "object/object-value.h"
#include "perception/object-perception.h"
#include "player/digestion-processor.h"
#include "player/patron.h"
#include "player/player-class.h"
#include "player/player-race-types.h"
#include "player/player-skill.h"
#include "player/player-status.h"
#include "player/selfinfo.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "status/experience.h"
#include "system/alloc-entries.h"
#include "system/angband-version.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "wizard/tval-descriptions-table.h"
#include "wizard/wizard-spells.h"
#include "wizard/wizard-spoiler.h"
#include "world/world.h"

#define K_MAX_DEPTH 110 /*!< アイテムの階層毎生成率を表示する最大階 */

#define NUM_O_SET 8
#define NUM_O_BIT 32

/*!
 * @brief 指定されたIDの固定アーティファクトを生成する / Create the artifact of the specified number
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void wiz_create_named_art(player_type *caster_ptr)
{
    char tmp_val[10] = "";
    if (!get_string("Artifact ID:", tmp_val, 3))
        return;

    ARTIFACT_IDX a_idx = (ARTIFACT_IDX)atoi(tmp_val);
    if ((a_idx < 0) || (a_idx >= max_a_idx))
        a_idx = 0;

    (void)create_named_art(caster_ptr, a_idx, caster_ptr->y, caster_ptr->x);
    msg_print("Allocated.");
}

/*!
 * @brief 32ビット変数のビット配列を並べて描画する / Output a long int in binary format.
 * @return なし
 */
static void prt_binary(BIT_FLAGS flags, int row, int col)
{
    u32b bitmask;
    for (int i = bitmask = 1; i <= 32; i++, bitmask *= 2) {
        if (flags & bitmask) {
            term_putch(col++, row, TERM_BLUE, '*');
        } else {
            term_putch(col++, row, TERM_WHITE, '-');
        }
    }
}

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
 * @brief プレイヤーの職業を変更する
 * @return なし
 * @todo 魔法領域の再選択などがまだ不完全、要実装。
 */
static void do_cmd_wiz_reset_class(player_type *creature_ptr)
{
    char ppp[80];
    sprintf(ppp, "Class (0-%d): ", MAX_CLASS - 1);

    char tmp_val[160];
    sprintf(tmp_val, "%d", creature_ptr->pclass);

    if (!get_string(ppp, tmp_val, 2))
        return;

    int tmp_int = atoi(tmp_val);
    if (tmp_int < 0 || tmp_int >= MAX_CLASS)
        return;

    creature_ptr->pclass = (byte)tmp_int;
    creature_ptr->window |= (PW_PLAYER);
    creature_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
    handle_stuff(creature_ptr);
}

/*!
 * @brief プレイヤーの現能力値を調整する
 * Aux function for "do_cmd_wiz_change()".	-RAK-
 * @return なし
 */
static void do_cmd_wiz_change_aux(player_type *creature_ptr)
{
    int tmp_int;
    long tmp_long;
    s16b tmp_s16b;
    char tmp_val[160];
    char ppp[80];

    for (int i = 0; i < A_MAX; i++) {
        sprintf(ppp, "%s (3-%d): ", stat_names[i], creature_ptr->stat_max_max[i]);
        sprintf(tmp_val, "%d", creature_ptr->stat_max[i]);
        if (!get_string(ppp, tmp_val, 3))
            return;

        tmp_int = atoi(tmp_val);
        if (tmp_int > creature_ptr->stat_max_max[i])
            tmp_int = creature_ptr->stat_max_max[i];
        else if (tmp_int < 3)
            tmp_int = 3;

        creature_ptr->stat_cur[i] = creature_ptr->stat_max[i] = (BASE_STATUS)tmp_int;
    }

    sprintf(tmp_val, "%d", WEAPON_EXP_MASTER);
    if (!get_string(_("熟練度: ", "Proficiency: "), tmp_val, 9))
        return;

    tmp_s16b = (s16b)atoi(tmp_val);
    if (tmp_s16b < WEAPON_EXP_UNSKILLED)
        tmp_s16b = WEAPON_EXP_UNSKILLED;
    if (tmp_s16b > WEAPON_EXP_MASTER)
        tmp_s16b = WEAPON_EXP_MASTER;

    for (int j = 0; j <= TV_WEAPON_END - TV_WEAPON_BEGIN; j++) {
        for (int i = 0; i < 64; i++) {
            creature_ptr->weapon_exp[j][i] = tmp_s16b;
            if (creature_ptr->weapon_exp[j][i] > s_info[creature_ptr->pclass].w_max[j][i])
                creature_ptr->weapon_exp[j][i] = s_info[creature_ptr->pclass].w_max[j][i];
        }
    }

    for (int j = 0; j < 10; j++) {
        creature_ptr->skill_exp[j] = tmp_s16b;
        if (creature_ptr->skill_exp[j] > s_info[creature_ptr->pclass].s_max[j])
            creature_ptr->skill_exp[j] = s_info[creature_ptr->pclass].s_max[j];
    }

    int k;
    for (k = 0; k < 32; k++)
        creature_ptr->spell_exp[k] = (tmp_s16b > SPELL_EXP_MASTER ? SPELL_EXP_MASTER : tmp_s16b);

    for (; k < 64; k++)
        creature_ptr->spell_exp[k] = (tmp_s16b > SPELL_EXP_EXPERT ? SPELL_EXP_EXPERT : tmp_s16b);

    sprintf(tmp_val, "%ld", (long)(creature_ptr->au));
    if (!get_string("Gold: ", tmp_val, 9))
        return;

    tmp_long = atol(tmp_val);
    if (tmp_long < 0)
        tmp_long = 0L;

    creature_ptr->au = tmp_long;
    sprintf(tmp_val, "%ld", (long)(creature_ptr->max_exp));
    if (!get_string("Experience: ", tmp_val, 9))
        return;

    tmp_long = atol(tmp_val);
    if (tmp_long < 0)
        tmp_long = 0L;

    if (creature_ptr->prace == RACE_ANDROID)
        return;

    creature_ptr->max_exp = tmp_long;
    creature_ptr->exp = tmp_long;
    check_experience(creature_ptr);
}

/*!
 * @brief プレイヤーの現能力値を調整する(メインルーチン)
 * Change various "permanent" player variables.
 * @return なし
 */
static void do_cmd_wiz_change(player_type *creature_ptr)
{
    do_cmd_wiz_change_aux(creature_ptr);
    do_cmd_redraw(creature_ptr);
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
 * @brief ベースアイテムのウィザード生成のために大項目IDと小項目IDを取得する /
 * Specify tval and sval (type and subtype of object) originally
 * @return ベースアイテムID
 * @details
 * by RAK, heavily modified by -Bernd-
 * This function returns the k_idx of an object type, or zero if failed
 * List up to 50 choices in three columns
 */
static KIND_OBJECT_IDX wiz_create_itemtype(void)
{
    KIND_OBJECT_IDX i;
    int num;
    TERM_LEN col, row;
    char ch;
    KIND_OBJECT_IDX choice[80];
    char buf[160];

    term_clear();
    for (num = 0; (num < 80) && tvals[num].tval; num++) {
        row = 2 + (num % 20);
        col = 20 * (num / 20);
        ch = listsym[num];
        prt(format("[%c] %s", ch, tvals[num].desc), row, col);
    }

    int max_num = num;
    if (!get_com("Get what type of object? ", &ch, FALSE))
        return 0;

    for (num = 0; num < max_num; num++) {
        if (listsym[num] == ch)
            break;
    }

    if ((num < 0) || (num >= max_num))
        return 0;

    tval_type tval = tvals[num].tval;
    concptr tval_desc = tvals[num].desc;
    term_clear();
    for (num = 0, i = 1; (num < 80) && (i < max_k_idx); i++) {
        object_kind *k_ptr = &k_info[i];
        if (k_ptr->tval != tval)
            continue;

        row = 2 + (num % 20);
        col = 20 * (num / 20);
        ch = listsym[num];
        strcpy(buf, "                    ");
        strip_name(buf, i);
        prt(format("[%c] %s", ch, buf), row, col);
        choice[num++] = i;
    }

    max_num = num;
    if (!get_com(format("What Kind of %s? ", tval_desc), &ch, FALSE))
        return 0;

    for (num = 0; num < max_num; num++)
        if (listsym[num] == ch)
            break;

    if ((num < 0) || (num >= max_num))
        return 0;

    return choice[num];
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

        switch (ch) {
        /* Apply bad magic, but first clear object */
        case 'w':
        case 'W': {
            object_prep(owner_ptr, q_ptr, o_ptr->k_idx);
            apply_magic(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART | AM_GOOD | AM_GREAT | AM_CURSED);
            break;
        }
        /* Apply bad magic, but first clear object */
        case 'c':
        case 'C': {
            object_prep(owner_ptr, q_ptr, o_ptr->k_idx);
            apply_magic(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART | AM_GOOD | AM_CURSED);
            break;
        }
        /* Apply normal magic, but first clear object */
        case 'n':
        case 'N': {
            object_prep(owner_ptr, q_ptr, o_ptr->k_idx);
            apply_magic(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART);
            break;
        }
        /* Apply good magic, but first clear object */
        case 'g':
        case 'G': {
            object_prep(owner_ptr, q_ptr, o_ptr->k_idx);
            apply_magic(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART | AM_GOOD);
            break;
        }
        /* Apply great magic, but first clear object */
        case 'e':
        case 'E': {
            object_prep(owner_ptr, q_ptr, o_ptr->k_idx);
            apply_magic(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART | AM_GOOD | AM_GREAT);
            break;
        }
        /* Apply special magic, but first clear object */
        case 's':
        case 'S': {
            object_prep(owner_ptr, q_ptr, o_ptr->k_idx);
            apply_magic(owner_ptr, q_ptr, owner_ptr->current_floor_ptr->dun_level, AM_GOOD | AM_GREAT | AM_SPECIAL);

            if (!object_is_artifact(q_ptr))
                become_random_artifact(owner_ptr, q_ptr, FALSE);

            break;
        }
        }

        q_ptr->iy = o_ptr->iy;
        q_ptr->ix = o_ptr->ix;
        q_ptr->next_o_idx = o_ptr->next_o_idx;
        q_ptr->marked = o_ptr->marked;
    }

    if (changed) {
        object_copy(o_ptr, q_ptr);
        owner_ptr->update |= (PU_BONUS);
        owner_ptr->update |= (PU_COMBINE | PU_REORDER);
        owner_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER);
    }
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
    object_type forge;
    object_type *q_ptr;

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

            q_ptr = &forge;
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
 * @brief アイテム検査のメインルーチン /
 * Play with an item. Options include:
 * @return なし
 * @details
 *   - Output statistics (via wiz_roll_item)<br>
 *   - Reroll item (via wiz_reroll_item)<br>
 *   - Change properties (via wiz_tweak_item)<br>
 *   - Change the number of items (via wiz_quantity_item)<br>
 */
static void do_cmd_wiz_play(player_type *creature_ptr)
{
    concptr q = "Play with which object? ";
    concptr s = "You have nothing to play with.";

    OBJECT_IDX item;
    object_type *o_ptr;
    o_ptr = choose_object(creature_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
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
        if (item >= 0) {
            creature_ptr->total_weight += (q_ptr->weight * q_ptr->number) - (o_ptr->weight * o_ptr->number);
        }

        object_copy(o_ptr, q_ptr);
        creature_ptr->update |= (PU_BONUS);
        creature_ptr->update |= (PU_COMBINE | PU_REORDER);
        creature_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER);
    } else {
        msg_print("Changes ignored.");
    }
}

/*!
 * @brief 任意のベースアイテム生成のメインルーチン /
 * Wizard routine for creating objects		-RAK-
 * @return なし
 * @details
 * Heavily modified to allow magification and artifactification  -Bernd-
 *
 * Note that wizards cannot create objects on top of other objects.
 *
 * Hack -- this routine always makes a "dungeon object", and applies
 * magic to it, and attempts to decline cursed items.
 */
static void wiz_create_item(player_type *caster_ptr)
{
    screen_save(caster_ptr);
    OBJECT_IDX k_idx = wiz_create_itemtype();
    screen_load(caster_ptr);
    if (!k_idx)
        return;

    if (k_info[k_idx].gen_flags & TRG_INSTA_ART) {
        for (ARTIFACT_IDX i = 1; i < max_a_idx; i++) {
            if ((a_info[i].tval != k_info[k_idx].tval) || (a_info[i].sval != k_info[k_idx].sval))
                continue;

            (void)create_named_art(caster_ptr, i, caster_ptr->y, caster_ptr->x);
            msg_print("Allocated(INSTA_ART).");
            return;
        }
    }

    object_type forge;
    object_type *q_ptr;
    q_ptr = &forge;
    object_prep(caster_ptr, q_ptr, k_idx);
    apply_magic(caster_ptr, q_ptr, caster_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART);
    (void)drop_near(caster_ptr, q_ptr, -1, caster_ptr->y, caster_ptr->x);
    msg_print("Allocated.");
}

/*!
 * @brief プレイヤーを完全回復する /
 * Cure everything instantly
 * @return なし
 */
static void do_cmd_wiz_cure_all(player_type *creature_ptr)
{
    (void)life_stream(creature_ptr, FALSE, FALSE);
    (void)restore_mana(creature_ptr, TRUE);
    (void)set_food(creature_ptr, PY_FOOD_MAX - 1);
}

/*!
 * @brief 任意のダンジョン及び階層に飛ぶ /
 * Go to any level
 * @return なし
 */
static void do_cmd_wiz_jump(player_type *creature_ptr)
{
    if (command_arg <= 0) {
        char ppp[80];
        char tmp_val[160];
        DUNGEON_IDX tmp_dungeon_type;
        sprintf(ppp, "Jump which dungeon : ");
        sprintf(tmp_val, "%d", creature_ptr->dungeon_idx);
        if (!get_string(ppp, tmp_val, 2))
            return;

        tmp_dungeon_type = (DUNGEON_IDX)atoi(tmp_val);
        if (!d_info[tmp_dungeon_type].maxdepth || (tmp_dungeon_type > current_world_ptr->max_d_idx))
            tmp_dungeon_type = DUNGEON_ANGBAND;

        sprintf(ppp, "Jump to level (0, %d-%d): ", (int)d_info[tmp_dungeon_type].mindepth, (int)d_info[tmp_dungeon_type].maxdepth);
        sprintf(tmp_val, "%d", (int)creature_ptr->current_floor_ptr->dun_level);
        if (!get_string(ppp, tmp_val, 10))
            return;

        command_arg = (COMMAND_ARG)atoi(tmp_val);
        creature_ptr->dungeon_idx = tmp_dungeon_type;
    }

    if (command_arg < d_info[creature_ptr->dungeon_idx].mindepth)
        command_arg = 0;

    if (command_arg > d_info[creature_ptr->dungeon_idx].maxdepth)
        command_arg = (COMMAND_ARG)d_info[creature_ptr->dungeon_idx].maxdepth;

    msg_format("You jump to dungeon level %d.", command_arg);
    if (autosave_l)
        do_cmd_save_game(creature_ptr, TRUE);

    creature_ptr->current_floor_ptr->dun_level = command_arg;
    prepare_change_floor_mode(creature_ptr, CFM_RAND_PLACE);
    if (!creature_ptr->current_floor_ptr->dun_level)
        creature_ptr->dungeon_idx = 0;

    creature_ptr->current_floor_ptr->inside_arena = FALSE;
    creature_ptr->wild_mode = FALSE;
    leave_quest_check(creature_ptr);
    if (record_stair)
        exe_write_diary(creature_ptr, DIARY_WIZ_TELE, 0, NULL);

    creature_ptr->current_floor_ptr->inside_quest = 0;
    free_turn(creature_ptr);
    creature_ptr->energy_need = 0;
    prepare_change_floor_mode(creature_ptr, CFM_FIRST_FLOOR);
    creature_ptr->leaving = TRUE;
}

/*!
 * @brief 全ベースアイテムを鑑定済みにする /
 * Become aware of a lot of objects
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void do_cmd_wiz_learn(player_type *caster_ptr)
{
    object_type forge;
    object_type *q_ptr;
    for (KIND_OBJECT_IDX i = 1; i < max_k_idx; i++) {
        object_kind *k_ptr = &k_info[i];
        if (k_ptr->level <= command_arg) {
            q_ptr = &forge;
            object_prep(caster_ptr, q_ptr, i);
            object_aware(caster_ptr, q_ptr);
        }
    }
}

/*!
 * @brief プレイヤー近辺の全モンスターを消去する /
 * Hack -- Delete all nearby monsters
 * @return なし
 */
static void do_cmd_wiz_zap(player_type *caster_ptr)
{
    for (MONSTER_IDX i = 1; i < caster_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr) || (i == caster_ptr->riding) || (m_ptr->cdis > MAX_SIGHT))
            continue;

        if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname) {
            GAME_TEXT m_name[MAX_NLEN];

            monster_desc(caster_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
            exe_write_diary(caster_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_WIZ_ZAP, m_name);
        }

        delete_monster_idx(caster_ptr, i);
    }
}

/*!
 * @brief フロアに存在する全モンスターを消去する /
 * Hack -- Delete all monsters
 * @param caster_ptr 術者の参照ポインタ
 * @return なし
 */
static void do_cmd_wiz_zap_all(player_type *caster_ptr)
{
    for (MONSTER_IDX i = 1; i < caster_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr) || (i == caster_ptr->riding))
            continue;

        if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname) {
            GAME_TEXT m_name[MAX_NLEN];
            monster_desc(caster_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
            exe_write_diary(caster_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_WIZ_ZAP, m_name);
        }

        delete_monster_idx(caster_ptr, i);
    }
}

/*!
 * @brief 指定された地点の地形IDを変更する /
 * Create desired feature
 * @param creaturer_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void do_cmd_wiz_create_feature(player_type *creature_ptr)
{
    POSITION y, x;
    if (!tgt_pt(creature_ptr, &x, &y))
        return;

    grid_type *g_ptr;
    g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
    static int prev_feat = 0;
    char tmp_val[160];
    sprintf(tmp_val, "%d", prev_feat);

    if (!get_string(_("地形: ", "Feature: "), tmp_val, 3))
        return;

    FEAT_IDX tmp_feat = (FEAT_IDX)atoi(tmp_val);
    if (tmp_feat < 0)
        tmp_feat = 0;
    else if (tmp_feat >= max_f_idx)
        tmp_feat = max_f_idx - 1;

    static int prev_mimic = 0;
    sprintf(tmp_val, "%d", prev_mimic);

    if (!get_string(_("地形 (mimic): ", "Feature (mimic): "), tmp_val, 3))
        return;

    FEAT_IDX tmp_mimic = (FEAT_IDX)atoi(tmp_val);
    if (tmp_mimic < 0)
        tmp_mimic = 0;
    else if (tmp_mimic >= max_f_idx)
        tmp_mimic = max_f_idx - 1;

    cave_set_feat(creature_ptr, y, x, tmp_feat);
    g_ptr->mimic = (s16b)tmp_mimic;
    feature_type *f_ptr;
    f_ptr = &f_info[get_feat_mimic(g_ptr)];

    if (have_flag(f_ptr->flags, FF_GLYPH) || have_flag(f_ptr->flags, FF_MINOR_GLYPH))
        g_ptr->info |= (CAVE_OBJECT);
    else if (have_flag(f_ptr->flags, FF_MIRROR))
        g_ptr->info |= (CAVE_GLOW | CAVE_OBJECT);

    note_spot(creature_ptr, y, x);
    lite_spot(creature_ptr, y, x);
    creature_ptr->update |= (PU_FLOW);
    prev_feat = tmp_feat;
    prev_mimic = tmp_mimic;
}

/*!
 * @brief 現在のオプション設定をダンプ出力する /
 * @param creature_ptr プレーヤーへの参照ポインタ
 * Hack -- Dump option bits usage
 * @return なし
 */
static void do_cmd_dump_options()
{
    char buf[1024];
    path_build(buf, sizeof buf, ANGBAND_DIR_USER, "opt_info.txt");
    FILE *fff;
    fff = angband_fopen(buf, "a");
    if (fff == NULL) {
        msg_format(_("ファイル %s を開けませんでした。", "Failed to open file %s."), buf);
        msg_print(NULL);
        return;
    }

    int **exist;
    C_MAKE(exist, NUM_O_SET, int *);
    C_MAKE(*exist, NUM_O_BIT * NUM_O_SET, int);
    for (int i = 1; i < NUM_O_SET; i++)
        exist[i] = *exist + i * NUM_O_BIT;

    for (int i = 0; option_info[i].o_desc; i++) {
        const option_type *ot_ptr = &option_info[i];
        if (ot_ptr->o_var)
            exist[ot_ptr->o_set][ot_ptr->o_bit] = i + 1;
    }

    fprintf(fff, "[Option bits usage on Hengband %d.%d.%d]\n\n", FAKE_VER_MAJOR - 10, FAKE_VER_MINOR, FAKE_VER_PATCH);
    fputs("Set - Bit (Page) Option Name\n", fff);
    fputs("------------------------------------------------\n", fff);
    for (int i = 0; i < NUM_O_SET; i++) {
        for (int j = 0; j < NUM_O_BIT; j++) {
            if (exist[i][j]) {
                const option_type *ot_ptr = &option_info[exist[i][j] - 1];
                fprintf(fff, "  %d -  %02d (%4d) %s\n", i, j, ot_ptr->o_page, ot_ptr->o_text);
            } else {
                fprintf(fff, "  %d -  %02d\n", i, j);
            }
        }

        fputc('\n', fff);
    }

    C_KILL(*exist, NUM_O_BIT * NUM_O_SET, int);
    C_KILL(exist, NUM_O_SET, int *);
    angband_fclose(fff);
    msg_format(_("オプションbit使用状況をファイル %s に書き出しました。", "Option bits usage dump saved to file %s."), buf);
}

/*!
 * @brief プレイ日数を変更する / Set gametime.
 * @return 実際に変更を行ったらTRUEを返す
 */
static void set_gametime(void)
{
    int tmp_int = 0;
    char ppp[80], tmp_val[40];

    sprintf(ppp, "Dungeon Turn (0-%ld): ", (long)current_world_ptr->dungeon_turn_limit);
    sprintf(tmp_val, "%ld", (long)current_world_ptr->dungeon_turn);
    if (!get_string(ppp, tmp_val, 10))
        return;

    tmp_int = atoi(tmp_val);

    /* Verify */
    if (tmp_int >= current_world_ptr->dungeon_turn_limit)
        tmp_int = current_world_ptr->dungeon_turn_limit - 1;
    else if (tmp_int < 0)
        tmp_int = 0;
    current_world_ptr->dungeon_turn = current_world_ptr->game_turn = tmp_int;
}

/*!
 * @brief デバッグコマンドを選択する処理のメインルーチン /
 * Ask for and parse a "debug command"
 * The "command_arg" may have been set.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * 番号を指定するには、それをN及びデバッグコマンドをXとしてとして「0N^aX」とする
 * a：全状態回復 / Cure all maladies
 * A：善悪の属性表示 / Know alignment
 * b：相手をテレポバック / Teleport to target
 * B：モンスター闘技場のモンスターを更新する / Update gambling monster
 * c：アイテム生成 / Create any object
 * C：指定番号の固定アーティファクトを生成する / Create a named artifact
 * d：全感知 / Detect everything
 * D：次元の扉 / Dimension_door
 * e：能力変更 / Edit character
 * E：全てのスペルをラーニング状態にする / Blue Mage Only
 * f：*鑑定* / Fully identification
 * F：地形ID変更 / Create desired feature
 * g：上質なアイテムを生成 / Good Objects
 * G：なし / Nothing
 * h：新生 / Hitpoint rerating
 * H：モンスターの群れ生成 / Generate monster group
 * i：鑑定 / Identification
 * I：なし / Nothing
 * j：ダンジョンの指定フロアへテレポート (ウィザードあり) / Jump to dungeon
 * J：なし / Nothing
 * k：自己分析 / Self info
 * K：なし / Nothing
 * l：番号指定したアイテムまで鑑定済にする / Learn about objects
 * L：なし / Nothing
 * m：魔法の地図 / Magic Mapping
 * M：突然変異 / Mutation / TODO: 指定した突然変異の除外機能を追加したい
 * n：番号指定したモンスターを生成 / Generate a monster
 * N：番号指定したペットを生成 / Generate a pet
 * o：アイテムのtval等を編集する / Edit object
 * O：現在のオプション設定をダンプ出力 / Output option settings
 * p：ショートテレポ / Blink
 * P：なし / Nothing
 * q：クエストを完了させる / Finish quest
 * Q：クエストに突入する (ウィザードあり) / Jump to quest
 * r：カオスパトロンから報酬を貰う / Gain reward from chaos patron
 * R：クラス変更 / Change class
 * s：フロア相応のモンスター召喚 / Summon a monster
 * S：高級品獲得ドロップ / Get a great item
 * t：テレポート / Teleport
 * T：プレイ日時変更 / Change time
 * u：啓蒙 (強制的に忍者以外) / Lite floor without ninja classified
 * U：なし / Nothing
 * v：特別品獲得ドロップ / Get a special item
 * V：クラス変更 / Change class / TODO: Rと同じなので何か変えたい
 * w：啓蒙 (忍者かどうか考慮) / Lite floor with ninja classified
 * W：なし / Nothing
 * x：経験値を得る / Gain experience
 * X：アイテムを初期状態に戻す / Return items to the initial ones
 * y：なし / Nothing
 * Y：なし / Nothing
 * z：近隣のモンスター消去 / Zap monsters around
 * Z：フロア中のモンスター消去 / Zap all monsters in the floor
 * @：特殊スペルの発動 / Special spell
 * "：スポイラーのダンプ / Dump spoiler
 * ?：ヘルプ表示 (通常の？と同じ) / Show help (same as normal help)
 */
void do_cmd_debug(player_type *creature_ptr)
{
    char cmd;
    get_com("Debug Command: ", &cmd, FALSE);
    switch (cmd) {
    case ESCAPE:
    case ' ':
    case '\n':
    case '\r':
        break;
    case 'a':
        do_cmd_wiz_cure_all(creature_ptr);
        break;
    case 'A':
        msg_format("Your alignment is %d.", creature_ptr->align);
        break;
    case 'b':
        do_cmd_wiz_bamf(creature_ptr);
        break;
    case 'B':
        update_gambling_monsters(creature_ptr);
        break;
    case 'c':
        wiz_create_item(creature_ptr);
        break;
    case 'C':
        wiz_create_named_art(creature_ptr);
        break;
    case 'd':
        detect_all(creature_ptr, DETECT_RAD_ALL * 3);
        break;
    case 'D':
        wiz_dimension_door(creature_ptr);
        break;
    case 'e':
        do_cmd_wiz_change(creature_ptr);
        break;
    case 'E':
        if (creature_ptr->pclass == CLASS_BLUE_MAGE) {
            do_cmd_wiz_blue_mage(creature_ptr);
        }

        break;
    case 'f':
        identify_fully(creature_ptr, FALSE, 0);
        break;
    case 'F':
        do_cmd_wiz_create_feature(creature_ptr);
        break;
    case 'g':
        if (command_arg <= 0)
            command_arg = 1;

        acquirement(creature_ptr, creature_ptr->y, creature_ptr->x, command_arg, FALSE, FALSE, TRUE);
        break;
    case 'h':
        roll_hitdice(creature_ptr, SPOP_DISPLAY_MES | SPOP_DEBUG);
        break;
    case 'H':
        do_cmd_summon_horde(creature_ptr);
        break;
    case 'i':
        (void)ident_spell(creature_ptr, FALSE, 0);
        break;
    case 'j':
        do_cmd_wiz_jump(creature_ptr);
        break;
    case 'k':
        self_knowledge(creature_ptr);
        break;
    case 'l':
        do_cmd_wiz_learn(creature_ptr);
        break;
    case 'm':
        map_area(creature_ptr, DETECT_RAD_ALL * 3);
        break;
    case 'M':
        (void)gain_mutation(creature_ptr, command_arg);
        break;
    case 'R':
        (void)do_cmd_wiz_reset_class(creature_ptr);
        break;
    case 'r':
        (void)gain_level_reward(creature_ptr, command_arg);
        break;
    case 'N':
        do_cmd_wiz_named_friendly(creature_ptr, command_arg);
        break;
    case 'n':
        do_cmd_wiz_named(creature_ptr, command_arg);
        break;
    case 'O':
        do_cmd_dump_options();
        break;
    case 'o':
        do_cmd_wiz_play(creature_ptr);
        break;
    case 'p':
        teleport_player(creature_ptr, 10, TELEPORT_SPONTANEOUS);
        break;
    case 'Q': {
        char ppp[30];
        char tmp_val[5];
        int tmp_int;
        sprintf(ppp, "QuestID (0-%d):", max_q_idx - 1);
        sprintf(tmp_val, "%d", 0);

        if (!get_string(ppp, tmp_val, 3))
            return;
        tmp_int = atoi(tmp_val);

        if (tmp_int < 0)
            break;
        if (tmp_int >= max_q_idx)
            break;

        creature_ptr->current_floor_ptr->inside_quest = (QUEST_IDX)tmp_int;
        parse_fixed_map(creature_ptr, "q_info.txt", 0, 0, 0, 0);
        quest[tmp_int].status = QUEST_STATUS_TAKEN;
        creature_ptr->current_floor_ptr->inside_quest = 0;
        break;
    }
    case 'q':
        if (creature_ptr->current_floor_ptr->inside_quest) {
            if (quest[creature_ptr->current_floor_ptr->inside_quest].status == QUEST_STATUS_TAKEN) {
                complete_quest(creature_ptr, creature_ptr->current_floor_ptr->inside_quest);
                break;
            }
        } else {
            msg_print("No current quest");
            msg_print(NULL);
        }

        break;
    case 's':
        if (command_arg <= 0)
            command_arg = 1;
        do_cmd_wiz_summon(creature_ptr, command_arg);
        break;
    case 'S':
        if (command_arg <= 0)
            command_arg = 1;

        acquirement(creature_ptr, creature_ptr->y, creature_ptr->x, command_arg, TRUE, TRUE, TRUE);
        break;
    case 't':
        teleport_player(creature_ptr, 100, TELEPORT_SPONTANEOUS);
        break;
    case 'T':
        set_gametime();
        break;
    case 'u':
        for (int y = 0; y < creature_ptr->current_floor_ptr->height; y++) {
            for (int x = 0; x < creature_ptr->current_floor_ptr->width; x++) {
                creature_ptr->current_floor_ptr->grid_array[y][x].info |= (CAVE_GLOW | CAVE_MARK);
            }
        }

        wiz_lite(creature_ptr, FALSE);
        break;
    case 'v':
        if (command_arg <= 0)
            command_arg = 1;
        acquirement(creature_ptr, creature_ptr->y, creature_ptr->x, command_arg, TRUE, FALSE, TRUE);
        break;
    case 'V':
        do_cmd_wiz_reset_class(creature_ptr);
        break;
    case 'w':
        wiz_lite(creature_ptr, (bool)(creature_ptr->pclass == CLASS_NINJA));
        break;
    case 'x':
        gain_exp(creature_ptr, command_arg ? command_arg : (creature_ptr->exp + 1));
        break;
    case 'X': {
        INVENTORY_IDX i;
        for (i = INVEN_TOTAL - 1; i >= 0; i--) {
            if (creature_ptr->inventory_list[i].k_idx)
                drop_from_inventory(creature_ptr, i, 999);
        }

        player_outfit(creature_ptr);
        break;
    }
    case 'z':
        do_cmd_wiz_zap(creature_ptr);
        break;
    case 'Z':
        do_cmd_wiz_zap_all(creature_ptr);
        break;
    case '_':
        probing(creature_ptr);
        break;
    case '@':
        do_cmd_debug_spell(creature_ptr);
        break;
    case '"':
        do_cmd_spoilers(creature_ptr);
        break;
    case '?':
        do_cmd_help(creature_ptr);
        break;
    default:
        msg_print("That is not a valid debug command.");
        break;
    }
}
