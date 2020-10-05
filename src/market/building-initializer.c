#include "market/building-initializer.h"
#include "floor/floor-town.h"
#include "market/articles-on-sale.h"
#include "object/object-kind.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/building-type-definition.h"
#include "system/object-type-definition.h"

/*!
 * @brief 町情報読み込みのメインルーチン /
 * Initialize town array
 * @return エラーコード
 */
errr init_towns(void)
{
    C_MAKE(town_info, max_towns, town_type);
    for (int i = 1; i < max_towns; i++) {
        C_MAKE(town_info[i].store, MAX_STORES, store_type);
        for (int j = 0; j < MAX_STORES; j++) {
            store_type *store_ptr = &town_info[i].store[j];
            if ((i > 1) && (j == STORE_MUSEUM || j == STORE_HOME))
                continue;

            /*
             * 我が家が 20 ページまで使える隠し機能のための準備。
             * オプションが有効でもそうでなくても一応スペースを作っておく。
             */
            if (j == STORE_HOME) {
                store_ptr->stock_size = STORE_INVEN_MAX * 10;
            } else if (j == STORE_MUSEUM) {
                store_ptr->stock_size = STORE_INVEN_MAX * 50;
            } else {
                store_ptr->stock_size = STORE_INVEN_MAX;
            }

            C_MAKE(store_ptr->stock, store_ptr->stock_size, object_type);
            if ((j == STORE_BLACK) || (j == STORE_HOME) || (j == STORE_MUSEUM))
                continue;

            store_ptr->table_size = STORE_CHOICES;
            C_MAKE(store_ptr->table, store_ptr->table_size, s16b);
            for (int k = 0; k < STORE_CHOICES; k++) {
                KIND_OBJECT_IDX k_idx;
                int tv = store_table[j][k][0];
                int sv = store_table[j][k][1];
                for (k_idx = 1; k_idx < max_k_idx; k_idx++) {
                    object_kind *k_ptr = &k_info[k_idx];
                    if ((k_ptr->tval == tv) && (k_ptr->sval == sv))
                        break;
                }

                if (k_idx == max_k_idx)
                    continue;

                store_ptr->table[store_ptr->table_num++] = k_idx;
            }
        }
    }

    return 0;
}

/*!
 * @brief 店情報初期化のメインルーチン /
 * Initialize buildings
 * @return エラーコード
 */
errr init_buildings(void)
{
    for (int i = 0; i < MAX_BLDG; i++) {
        building[i].name[0] = '\0';
        building[i].owner_name[0] = '\0';
        building[i].owner_race[0] = '\0';

        for (int j = 0; j < 8; j++) {
            building[i].act_names[j][0] = '\0';
            building[i].member_costs[j] = 0;
            building[i].other_costs[j] = 0;
            building[i].letters[j] = 0;
            building[i].actions[j] = 0;
            building[i].action_restr[j] = 0;
        }

        for (int j = 0; j < MAX_CLASS; j++)
            building[i].member_class[j] = 0;

        for (int j = 0; j < MAX_RACES; j++)
            building[i].member_race[j] = 0;

        for (int j = 0; j < MAX_MAGIC + 1; j++)
            building[i].member_realm[j] = 0;
    }

    return 0;
}
