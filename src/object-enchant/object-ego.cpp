/*!
 * @brief エゴアイテムに関する処理
 * @date 2019/05/02
 * @author deskull
 * @details Ego-Item indexes (see "lib/edit/e_info.txt")
 */
#include "object-enchant/object-ego.h"
#include "object-enchant/trg-types.h"
#include "util/bit-flags-calculator.h"
#include "util/probability-table.h"
#include <vector>

/*
 * The ego-item arrays
 */
ego_item_type *e_info;
char *e_name;
char *e_text;

/*
 * Maximum number of ego-items in e_info.txt
 */
EGO_IDX max_e_idx;

/*!
 * @brief アイテムのエゴをレア度の重みに合わせてランダムに選択する
 * Choose random ego type
 * @param slot 取得したいエゴの装備部位
 * @param good TRUEならば通常のエゴ、FALSEならば呪いのエゴが選択対象となる。
 * @return 選択されたエゴ情報のID、万一選択できなかった場合は0が返る。
 */
byte get_random_ego(byte slot, bool good)
{
    ProbabilityTable<EGO_IDX> prob_table;
    for (EGO_IDX i = 1; i < max_e_idx; i++) {
        ego_item_type *e_ptr = &e_info[i];
        if (e_ptr->slot != slot || e_ptr->rarity <= 0)
            continue;

        bool worthless = e_ptr->rating == 0 || e_ptr->gen_flags.has_any_of({ TRG::CURSED, TRG::HEAVY_CURSE, TRG::PERMA_CURSE });

        if (good != worthless) {
            prob_table.entry_item(i, (255 / e_ptr->rarity));
        }
    }

    if (!prob_table.empty()) {
        return prob_table.pick_one_at_random();
    }

    return (EGO_IDX)0;
}
