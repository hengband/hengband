/*
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
#include "object-hook/hook-weapon.h"
#include "object/object-flags.h"
#include "object/tval-types.h"
#include "perception/object-perception.h"
#include "store/store-util.h"
#include "sv-definition/sv-amulet-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-ring-types.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include <sstream>

static concptr inven_res_label = _(
    "                               酸電火冷毒閃暗破轟獄因沌劣 盲恐乱麻視経感遅活浮",
    "                               AcElFiCoPoLiDkShSoNtNxCaDi BlFeCfFaSiHlEpSdRgLv");

#define IM_FLAG_STR _("＊", "* ")
#define HAS_FLAG_STR _("＋", "+ ")
#define NO_FLAG_STR _("・", ". ")

/*!
 * @brief 4元素耐性を表示する
 * @param immunity 4元素耐性の種類 (二重？)
 * @param resistance 4元素耐性
 * @param flags 耐性配列へのポインタ
 * @param fff 一時ファイルへのポインタ
 */
static void print_im_or_res_flag(tr_type immunity, tr_type resistance, const TrFlags &flags, FILE *fff)
{
    fputs(flags.has(immunity) ? IM_FLAG_STR : (flags.has(resistance) ? HAS_FLAG_STR : NO_FLAG_STR), fff);
}

/*!
 * @brief 4元素以外の耐性を表示する
 * @param tr 耐性
 * @param flags 耐性配列へのポインタ
 * @param fff 一時ファイルへのポインタ
 */
static void print_flag(tr_type tr, const TrFlags &flags, FILE *fff)
{
    fputs(flags.has(tr) ? HAS_FLAG_STR : NO_FLAG_STR, fff);
}

/*!
 * @brief 特殊なアイテムかどうかを調べる
 * @param o_ptr アイテムへの参照ポインタ
 * @param tval アイテム主分類番号
 * @return 特殊なアイテムならTRUE
 */
static bool determine_spcial_item_type(ItemEntity *o_ptr, ItemKindType tval)
{
    const auto bi_key = BaseitemKey(tval, o_ptr->bi_key.sval());
    auto is_special_item_type = bi_key == BaseitemKey(ItemKindType::AMULET, SV_AMULET_RESISTANCE);
    is_special_item_type |= bi_key == BaseitemKey(ItemKindType::RING, SV_RING_LORDLY);
    is_special_item_type |= bi_key == BaseitemKey(ItemKindType::SHIELD, SV_DRAGON_SHIELD);
    is_special_item_type |= bi_key == BaseitemKey(ItemKindType::HELM, SV_DRAGON_HELM);
    is_special_item_type |= bi_key == BaseitemKey(ItemKindType::GLOVES, SV_SET_OF_DRAGON_GLOVES);
    is_special_item_type |= bi_key == BaseitemKey(ItemKindType::BOOTS, SV_PAIR_OF_DRAGON_GREAVE);
    is_special_item_type |= o_ptr->is_fixed_or_random_artifact();
    return (o_ptr->is_wearable() && o_ptr->is_ego()) || is_special_item_type;
}

/*!
 * @brief アイテムに耐性の表示をする必要があるかを判定する
 * @param o_ptr アイテムへの参照ポインタ
 * @param tval アイテム主分類番号
 * @return 必要があるならTRUE
 */
static bool check_item_knowledge(ItemEntity *o_ptr, ItemKindType tval)
{
    if (!o_ptr->is_valid()) {
        return false;
    }
    if (o_ptr->bi_key.tval() != tval) {
        return false;
    }
    if (!o_ptr->is_known()) {
        return false;
    }
    if (!determine_spcial_item_type(o_ptr, tval)) {
        return false;
    }

    return true;
}

/*!
 * @brief 鑑定済アイテムの耐性を表示する
 * @param o_ptr アイテムへの参照ポインタ
 * @param fff 一時ファイルへの参照ポインタ
 * @todo ここの関数から表示用の関数に移したい
 */
static void display_identified_resistances_flag(ItemEntity *o_ptr, FILE *fff)
{
    auto flags = object_flags_known(o_ptr);

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
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff 一時ファイルへの参照ポインタ
 * @param o_ptr アイテムへの参照ポインタ
 * @param where アイテムの場所 (手持ち、家等) を示す文字列への参照ポインタ
 * @details 28文字ちょうどになるまで右側をスペースでパディングする
 */
static void do_cmd_knowledge_inventory_aux(PlayerType *player_ptr, FILE *fff, ItemEntity *o_ptr, char *where)
{
    constexpr auto max_item_length = 26;
    std::stringstream ss;
    ss << describe_flavor(player_ptr, o_ptr, OD_NAME_ONLY, max_item_length);
    const int item_length = ss.tellp();
    constexpr auto max_display_length = 28;
    for (auto i = item_length; i < max_display_length; i++) {
        ss << ' ';
    }

    fprintf(fff, "%s %s", where, ss.str().data());
    if (!o_ptr->is_fully_known()) {
        fputs(_("-------不明--------------- -------不明---------\n", "-------unknown------------ -------unknown------\n"), fff);
        return;
    }

    display_identified_resistances_flag(o_ptr, fff);
}

/*!
 * @brief 9行おきにラベルを追加する
 * @param label_number 現在の行数
 * @param fff 一時ファイルへの参照ポインタ
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
 */
static void reset_label_number(int *label_number, FILE *fff)
{
    if (*label_number == 0) {
        return;
    }

    for (; *label_number < 9; (*label_number)++) {
        fputc('\n', fff);
    }

    *label_number = 0;
    fprintf(fff, "%s\n", inven_res_label);
}

/*!
 * 装備中のアイテムについて、耐性を表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param tval アイテム主分類番号
 * @param label_number 現在の行数
 * @param fff ファイルへの参照ポインタ
 */
static void show_wearing_equipment_resistances(PlayerType *player_ptr, ItemKindType tval, int *label_number, FILE *fff)
{
    char where[32];
    strcpy(where, _("装", "E "));
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        auto *o_ptr = &player_ptr->inventory_list[i];
        if (!check_item_knowledge(o_ptr, tval)) {
            continue;
        }

        do_cmd_knowledge_inventory_aux(player_ptr, fff, o_ptr, where);
        add_res_label(label_number, fff);
    }
}

/*!
 * 手持ち中のアイテムについて、耐性を表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param tval アイテム主分類番号
 * @param label_number 現在の行数
 * @param fff ファイルへの参照ポインタ
 */
static void show_holding_equipment_resistances(PlayerType *player_ptr, ItemKindType tval, int *label_number, FILE *fff)
{
    char where[32];
    strcpy(where, _("持", "I "));
    for (int i = 0; i < INVEN_PACK; i++) {
        auto *o_ptr = &player_ptr->inventory_list[i];
        if (!check_item_knowledge(o_ptr, tval)) {
            continue;
        }

        do_cmd_knowledge_inventory_aux(player_ptr, fff, o_ptr, where);
        add_res_label(label_number, fff);
    }
}

/*!
 * 我が家のアイテムについて、耐性を表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param tval アイテム主分類番号
 * @param label_number 現在の行数
 * @param fff ファイルへの参照ポインタ
 */
static void show_home_equipment_resistances(PlayerType *player_ptr, ItemKindType tval, int *label_number, FILE *fff)
{
    store_type *store_ptr;
    store_ptr = &towns_info[1].stores[StoreSaleType::HOME];
    char where[32];
    strcpy(where, _("家", "H "));
    for (int i = 0; i < store_ptr->stock_num; i++) {
        auto *o_ptr = &store_ptr->stock[i];
        if (!check_item_knowledge(o_ptr, tval)) {
            continue;
        }

        do_cmd_knowledge_inventory_aux(player_ptr, fff, o_ptr, where);
        add_res_label(label_number, fff);
    }
}

/*
 * @brief Display *ID* ed weapons/armors's resistances
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_knowledge_inventory(PlayerType *player_ptr)
{
    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name)) {
        return;
    }

    fprintf(fff, "%s\n", inven_res_label);
    int label_number = 0;
    for (auto tval : TV_WEARABLE_RANGE) {
        reset_label_number(&label_number, fff);
        show_wearing_equipment_resistances(player_ptr, tval, &label_number, fff);
        show_holding_equipment_resistances(player_ptr, tval, &label_number, fff);
        show_home_equipment_resistances(player_ptr, tval, &label_number, fff);
    }

    angband_fclose(fff);
    (void)show_file(player_ptr, true, file_name, _("*鑑定*済み武器/防具の耐性リスト", "Resistances of *identified* equipment"), 0, 0);
    fd_kill(file_name);
}
