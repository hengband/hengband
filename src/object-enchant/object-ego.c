/*!
 * @brief エゴアイテムに関する処理
 * @date 2019/05/02
 * @author deskull
 * @details Ego-Item indexes (see "lib/edit/e_info.txt")
 */
#include "object-enchant/object-ego.h"

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
 * @return 選択されたエゴ情報のID、万一選択できなかった場合はmax_e_idxが返る。
 */
byte get_random_ego(byte slot, bool good)
{
    long total = 0L;
    for (int i = 1; i < max_e_idx; i++) {
        ego_item_type *e_ptr;
        e_ptr = &e_info[i];
        if (e_ptr->slot == slot && ((good && e_ptr->rating) || (!good && !e_ptr->rating))) {
            if (e_ptr->rarity)
                total += (255 / e_ptr->rarity);
        }
    }

    int value = randint1(total);
    int j;
    for (j = 1; j < max_e_idx; j++) {
        ego_item_type *e_ptr;
        e_ptr = &e_info[j];
        if (e_ptr->slot == slot && ((good && e_ptr->rating) || (!good && !e_ptr->rating))) {
            if (e_ptr->rarity)
                value -= (255 / e_ptr->rarity);
            if (value <= 0L)
                break;
        }
    }

    return (byte)j;
}
