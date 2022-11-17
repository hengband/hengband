﻿#include "market/building-initializer.h"
#include "floor/floor-town.h"
#include "object/object-kind-hook.h"
#include "player-info/class-types.h"
#include "store/articles-on-sale.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/angband.h"
#include "system/baseitem-info.h"
#include "system/building-type-definition.h"
#include "system/item-entity.h"
#include <vector>

/*!
 * @brief 町情報読み込みのメインルーチン /
 * Initialize town array
 * @details 「我が家を拡張する」オプションのON/OFFとは無関係に、ON時の容量を確保しておく.
 */
void init_towns(void)
{
    town_info = std::vector<town_type>(max_towns);
    for (auto i = 1; i < max_towns; i++) {
        town_info[i].store = std::vector<store_type>(MAX_STORES);
        for (auto sst : STORE_SALE_TYPE_LIST) {
            auto *store_ptr = &town_info[i].store[enum2i(sst)];
            if ((i > 1) && (sst == StoreSaleType::MUSEUM || sst == StoreSaleType::HOME)) {
                continue;
            }

            store_ptr->stock_size = store_get_stock_max(sst);
            store_ptr->stock = std::make_unique<ItemEntity[]>(store_ptr->stock_size);
            if ((sst == StoreSaleType::BLACK) || (sst == StoreSaleType::HOME) || (sst == StoreSaleType::MUSEUM)) {
                continue;
            }

            for (const auto &baseitem : store_regular_sale_table.at(sst)) {
                auto bi_id = lookup_baseitem_id(baseitem);
                store_ptr->regular.push_back(bi_id);
            }

            for (const auto &baseitem : store_sale_table.at(sst)) {
                auto bi_id = lookup_baseitem_id(baseitem);
                store_ptr->table.push_back(bi_id);
            }
        }
    }
}

/*!
 * @brief 店情報初期化のメインルーチン /
 * Initialize buildings
 */
void init_buildings(void)
{
    for (auto i = 0; i < MAX_BLDG; i++) {
        building[i].name[0] = '\0';
        building[i].owner_name[0] = '\0';
        building[i].owner_race[0] = '\0';
        for (auto j = 0; j < 8; j++) {
            building[i].act_names[j][0] = '\0';
            building[i].member_costs[j] = 0;
            building[i].other_costs[j] = 0;
            building[i].letters[j] = 0;
            building[i].actions[j] = 0;
            building[i].action_restr[j] = 0;
        }

        building[i].member_class.assign(PLAYER_CLASS_TYPE_MAX, static_cast<short>(PlayerClassType::WARRIOR));
        building[i].member_race.assign(MAX_RACES, static_cast<short>(PlayerRaceType::HUMAN));
        building[i].member_realm.assign(MAX_MAGIC + 1, 0);
    }
}
