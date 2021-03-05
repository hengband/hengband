﻿/*
 * @brief 装備の耐性を表示する
 * @date 2020/04/20
 * @author Hourier
 */

#include "knowledge/knowledge-inventory.h"
#include "core/show-file.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-town.h"
#include "inventory/inventory-slot-types.h"
#include "io-dump/dump-util.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-enchant.h"
#include "object-hook/hook-weapon.h"
#include "object/object-flags.h"
#include "perception/object-perception.h"
#include "store/store-util.h"
#include "sv-definition/sv-amulet-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-ring-types.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"

static concptr inven_res_label = _("                               酸電火冷毒光闇破轟獄因沌劣 盲怖乱痺透命感消復浮",
    "                               AcElFiCoPoLiDkShSoNtNxCaDi BlFeCfFaSeHlEpSdRgLv");

#define IM_FLAG_STR _("＊", "* ")
#define HAS_FLAG_STR _("＋", "+ ")
#define NO_FLAG_STR _("・", ". ")

/*!
 * @brief 4元素耐性を表示する
 * @param immunity 4元素耐性の種類 (二重？)
 * @param resistance 4元素耐性
 * @param flags 耐性配列へのポインタ
 * @param fff 一時ファイルへのポインタ
 * @return なし
 */
static void print_im_or_res_flag(int immunity, int resistance, BIT_FLAGS *flags, FILE *fff)
{
    fputs(has_flag(flags, immunity) ? IM_FLAG_STR : (has_flag(flags, resistance) ? HAS_FLAG_STR : NO_FLAG_STR), fff);
}

/*!
 * @brief 4元素以外の耐性を表示する
 * @param tr 耐性
 * @param flags 耐性配列へのポインタ
 * @param fff 一時ファイルへのポインタ
 * @return なし
 */
static void print_flag(int tr, BIT_FLAGS *flags, FILE *fff) { fputs(has_flag(flags, tr) ? HAS_FLAG_STR : NO_FLAG_STR, fff); }

/*!
 * @brief 特殊なアイテムかどうかを調べる
 * @param o_ptr アイテムへの参照ポインタ
 * @param tval アイテム主分類番号
 * @return 特殊なアイテムならTRUE
 */
static bool determine_spcial_item_type(object_type *o_ptr, tval_type tval)
{
    bool is_special_item_type = (object_is_wearable(o_ptr) && object_is_ego(o_ptr)) || ((tval == TV_AMULET) && (o_ptr->sval == SV_AMULET_RESISTANCE))
        || ((tval == TV_RING) && (o_ptr->sval == SV_RING_LORDLY)) || ((tval == TV_SHIELD) && (o_ptr->sval == SV_DRAGON_SHIELD))
        || ((tval == TV_HELM) && (o_ptr->sval == SV_DRAGON_HELM)) || ((tval == TV_GLOVES) && (o_ptr->sval == SV_SET_OF_DRAGON_GLOVES))
        || ((tval == TV_BOOTS) && (o_ptr->sval == SV_PAIR_OF_DRAGON_GREAVE)) || object_is_artifact(o_ptr);

    return is_special_item_type;
}

/*!
 * @brief アイテムに耐性の表示をする必要があるかを判定する
 * @param o_ptr アイテムへの参照ポインタ
 * @param tval アイテム主分類番号
 * @return 必要があるならTRUE
 */
static bool check_item_knowledge(object_type *o_ptr, tval_type tval)
{
    if (o_ptr->k_idx == 0)
        return FALSE;
    if (o_ptr->tval != tval)
        return FALSE;
    if (!object_is_known(o_ptr))
        return FALSE;
    if (!determine_spcial_item_type(o_ptr, tval))
        return FALSE;

    return TRUE;
}

/*!
 * todo ここの関数から表示用の関数に移したい
 * @brief 鑑定済アイテムの耐性を表示する
 * @param o_ptr アイテムへの参照ポインタ
 * @param fff 一時ファイルへの参照ポインタ
 * @return なし
 */
static void display_identified_resistances_flag(player_type *creature_ptr, object_type *o_ptr, FILE *fff)
{
    BIT_FLAGS flags[TR_FLAG_SIZE];
    object_flags_known(creature_ptr, o_ptr, flags);

    print_im_or_res_flag(TR_IM_ACID, TR_RES_ACID, flags, fff);
    print_im_or_res_flag(TR_IM_ELEC, TR_RES_ELEC, flags, fff);
    print_im_or_res_flag(TR_IM_FIRE, TR_RES_FIRE, flags, fff);
    print_im_or_res_flag(TR_IM_COLD, TR_RES_COLD, flags, fff);
    print_flag(TR_RES_POIS, flags, fff);
    print_flag(TR_RES_LITE, flags, fff);
    print_flag(TR_RES_DARK, flags, fff);
    print_flag(TR_RES_SHARDS, flags, fff);
    print_flag(TR_RES_SOUND, flags, fff);
    print_flag(TR_RES_NETHER, flags, fff);
    print_flag(TR_RES_NEXUS, flags, fff);
    print_flag(TR_RES_CHAOS, flags, fff);
    print_flag(TR_RES_DISEN, flags, fff);

    fputs(" ", fff);

    print_flag(TR_RES_BLIND, flags, fff);
    print_flag(TR_RES_FEAR, flags, fff);
    print_flag(TR_RES_CONF, flags, fff);
    print_flag(TR_FREE_ACT, flags, fff);
    print_flag(TR_SEE_INVIS, flags, fff);
    print_flag(TR_HOLD_EXP, flags, fff);
    print_flag(TR_TELEPATHY, flags, fff);
    print_flag(TR_SLOW_DIGEST, flags, fff);
    print_flag(TR_REGEN, flags, fff);
    print_flag(TR_LEVITATION, flags, fff);

    fputc('\n', fff);
}

/*!
 * @brief アイテム1つ当たりの耐性を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff 一時ファイルへの参照ポインタ
 * @param o_ptr アイテムへの参照ポインタ
 * @param where アイテムの場所 (手持ち、家等) を示す文字列への参照ポインタ
 * @return なし
 */
static void do_cmd_knowledge_inventory_aux(player_type *creature_ptr, FILE *fff, object_type *o_ptr, char *where)
{
    int i = 0;
    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(creature_ptr, o_name, o_ptr, OD_NAME_ONLY);
    while (o_name[i] && (i < 26)) {
#ifdef JP
        if (iskanji(o_name[i]))
            i++;
#endif
        i++;
    }

    if (i < 28) {
        while (i < 28) {
            o_name[i] = ' ';
            i++;
        }
    }

    o_name[i] = '\0';

    fprintf(fff, "%s %s", where, o_name);

    if (!object_is_fully_known(o_ptr)) {
        fputs(_("-------不明--------------- -------不明---------\n", "-------unknown------------ -------unknown------\n"), fff);
        return;
    }

    display_identified_resistances_flag(creature_ptr, o_ptr, fff);
}

/*!
 * @brief 9行おきにラベルを追加する
 * @param label_number 現在の行数
 * @param fff 一時ファイルへの参照ポインタ
 * @return なし
 */
static void add_res_label(int *label_number, FILE *fff)
{
    (*label_number)++;
    if (*label_number == 9) {
        *label_number = 0;
        fprintf(fff, "%s\n", inven_res_label);
    }
}

/*!
 * @brief 9行ごとに行数をリセットする
 * @param label_number 現在の行数
 * @param fff 一時ファイルへの参照ポインタ
 * @return なし
 */
static void reset_label_number(int *label_number, FILE *fff)
{
    if (*label_number == 0)
        return;

    for (; *label_number < 9; (*label_number)++) {
        fputc('\n', fff);
    }

    *label_number = 0;
    fprintf(fff, "%s\n", inven_res_label);
}

/*!
 * 装備中のアイテムについて、耐性を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param tval アイテム主分類番号
 * @param label_number 現在の行数
 * @param fff ファイルへの参照ポインタ
 * @return なし
 */
static void show_wearing_equipment_resistances(player_type *creature_ptr, tval_type tval, int *label_number, FILE *fff)
{
    char where[32];
    strcpy(where, _("装", "E "));
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        object_type *o_ptr = &creature_ptr->inventory_list[i];
        if (!check_item_knowledge(o_ptr, tval))
            continue;

        do_cmd_knowledge_inventory_aux(creature_ptr, fff, o_ptr, where);
        add_res_label(label_number, fff);
    }
}

/*!
 * 手持ち中のアイテムについて、耐性を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param tval アイテム主分類番号
 * @param label_number 現在の行数
 * @param fff ファイルへの参照ポインタ
 * @return なし
 */
static void show_holding_equipment_resistances(player_type *creature_ptr, tval_type tval, int *label_number, FILE *fff)
{
    char where[32];
    strcpy(where, _("持", "I "));
    for (int i = 0; i < INVEN_PACK; i++) {
        object_type *o_ptr = &creature_ptr->inventory_list[i];
        if (!check_item_knowledge(o_ptr, tval))
            continue;

        do_cmd_knowledge_inventory_aux(creature_ptr, fff, o_ptr, where);
        add_res_label(label_number, fff);
    }
}

/*!
 * 我が家のアイテムについて、耐性を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param tval アイテム主分類番号
 * @param label_number 現在の行数
 * @param fff ファイルへの参照ポインタ
 * @return なし
 */
static void show_home_equipment_resistances(player_type *creature_ptr, tval_type tval, int *label_number, FILE *fff)
{
    store_type *store_ptr;
    store_ptr = &town_info[1].store[STORE_HOME];
    char where[32];
    strcpy(where, _("家", "H "));
    for (int i = 0; i < store_ptr->stock_num; i++) {
        object_type *o_ptr = &store_ptr->stock[i];
        if (!check_item_knowledge(o_ptr, tval))
            continue;

        do_cmd_knowledge_inventory_aux(creature_ptr, fff, o_ptr, where);
        add_res_label(label_number, fff);
    }
}

/*
 * @brief Display *ID* ed weapons/armors's resistances
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_knowledge_inventory(player_type *creature_ptr)
{
    FILE *fff = NULL;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name))
        return;

    fprintf(fff, "%s\n", inven_res_label);
    int label_number = 0;
    for (int tval = TV_WEARABLE_BEGIN; tval <= TV_WEARABLE_END; tval++) {
        reset_label_number(&label_number, fff);
        show_wearing_equipment_resistances(creature_ptr, static_cast<tval_type>(tval), &label_number, fff);
        show_holding_equipment_resistances(creature_ptr, static_cast<tval_type>(tval), &label_number, fff);
        show_home_equipment_resistances(creature_ptr, static_cast<tval_type>(tval), &label_number, fff);
    }

    angband_fclose(fff);
    (void)show_file(creature_ptr, TRUE, file_name, _("*鑑定*済み武器/防具の耐性リスト", "Resistances of *identified* equipment"), 0, 0);
    fd_kill(file_name);
}
