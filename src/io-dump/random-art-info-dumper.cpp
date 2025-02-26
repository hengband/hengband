#include "io-dump/random-art-info-dumper.h"
#include "inventory/inventory-slot-types.h"
#include "io/files-util.h"
#include "perception/object-perception.h"
#include "store/store-util.h"
#include "system/floor/town-info.h"
#include "system/floor/town-list.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "util/finalizer.h"
#include "view/display-messages.h"
#include "wizard/artifact-analyzer.h"
#include "wizard/fixed-artifacts-spoiler.h"
#include "wizard/spoiler-util.h"
#include <sstream>

/*!
 * @brief ランダムアーティファクト1件をスポイラー出力する
 * @param item ランダムアーティファクトへの参照
 * @param art_ptr 記述内容を収めた構造体参照ポインタ
 */
static void spoiler_print_randart(const ItemEntity &item, const ArtifactsDumpInfo *art_ptr, std::ofstream &ofs)
{
    const auto finalizer = util::make_finalizer([art_ptr, &ofs]() {
        ofs << spoiler_indent << art_ptr->misc_desc << "\n\n";
    });
    const auto *pval_ptr = &art_ptr->pval_info;
    ofs << art_ptr->description << '\n';
    if (!item.is_fully_known()) {
        ofs << format(_("%s不明\n", "%sUnknown\n"), spoiler_indent.data());
        return;
    }

    if (!pval_ptr->pval_desc.empty()) {
        std::stringstream ss;
        ss << pval_ptr->pval_desc << _("の修正:", " to");
        spoiler_outlist(ss.str(), pval_ptr->pval_affects, item_separator, ofs);
    }

    spoiler_outlist(_("対:", "Slay"), art_ptr->slays, item_separator, ofs);
    spoiler_outlist(_("武器属性:", ""), art_ptr->brands, list_separator, ofs);
    spoiler_outlist(_("免疫:", "Immunity to"), art_ptr->immunities, item_separator, ofs);
    spoiler_outlist(_("耐性:", "Resist"), art_ptr->resistances, item_separator, ofs);
    spoiler_outlist(_("維持:", "Sustain"), art_ptr->sustenances, item_separator, ofs);
    spoiler_outlist("", art_ptr->misc_magic, list_separator, ofs);
    if (!art_ptr->activation.empty()) {
        ofs << format(_("%s発動: %s\n", "%sActivates for %s\n"), spoiler_indent.data(), art_ptr->activation.data());
    }
}

/*!
 * @brief ランダムアーティファクト内容をスポイラー出力するサブルーチン /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param item ランダムアーティファクトのオブジェクト構造体参照ポインタ
 * @param tval 出力したいランダムアーティファクトの種類
 */
static void spoil_random_artifact_aux(PlayerType *player_ptr, const ItemEntity &item, ItemKindType tval, std::ofstream &ofs)
{
    if (!item.is_known() || !item.is_random_artifact() || (item.bi_key.tval() != tval)) {
        return;
    }

    const auto artifacts_list = random_artifact_analyze(player_ptr, item);
    spoiler_print_randart(item, &artifacts_list, ofs);
}

/*!
 * @brief ランダムアーティファクト内容をスポイラー出力するメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void spoil_random_artifact(PlayerType *player_ptr)
{
    const auto path = path_build(ANGBAND_DIR_USER, "randifact.txt");
    std::ofstream ofs(path);
    if (!ofs) {
        msg_print("Cannot create list file.");
        return;
    }

    spoiler_underline("Random artifacts list.\r", ofs);
    for (const auto &[tval_list, name] : group_artifact_list) {
        for (auto tval : tval_list) {
            for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
                auto &item = *player_ptr->inventory[i];
                spoil_random_artifact_aux(player_ptr, item, tval, ofs);
            }

            for (int i = 0; i < INVEN_PACK; i++) {
                auto &item = *player_ptr->inventory[i];
                spoil_random_artifact_aux(player_ptr, item, tval, ofs);
            }

            const auto &home = towns_info[1].get_store(StoreSaleType::HOME);
            for (int i = 0; i < home.stock_num; i++) {
                auto &item = *home.stock[i];
                spoil_random_artifact_aux(player_ptr, item, tval, ofs);
            }

            const auto &museum = towns_info[1].get_store(StoreSaleType::MUSEUM);
            for (int i = 0; i < museum.stock_num; i++) {
                auto &item = *museum.stock[i];
                spoil_random_artifact_aux(player_ptr, item, tval, ofs);
            }
        }
    }

    const auto mes = ofs.good() ? "Successfully created a list file." : "Failed to create a list file.";
    msg_print(mes);
}
