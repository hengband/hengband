#include "market/building-initializer.h"
#include "floor/floor-town.h"
#include "io/files-util.h"
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
#include "util/angband-files.h"
#include <filesystem>
#include <set>
#include <vector>

/*!
 * @brief ユニークな町の数を数える
 * @return ユニークな町の数
 * @details 町定義ファイル名の先頭2文字は番号であることを利用してカウントする.
 * 辺境の地を表すファイルは(01_*.txt) は3つあるのでユニークではない. また町番号は1から始まるので最後に加算する.
 */
static int count_town_numbers()
{
    const auto &path = path_build(ANGBAND_DIR_EDIT, "towns");
    std::set<std::string> unique_towns;
    for (const auto &entry : std::filesystem::directory_iterator(path)) {
        const auto &filename = entry.path().filename().string();
        if (!filename.ends_with(".txt")) {
            continue;
        }

        unique_towns.insert(filename.substr(0, 2));
    }

    return unique_towns.size() + 1;
}

/*!
 * @brief 町情報読み込みのメインルーチン /
 * Initialize town array
 * @details 「我が家を拡張する」オプションのON/OFFとは無関係に、ON時の容量を確保しておく.
 */
void init_towns(void)
{
    const auto town_numbers = count_town_numbers();
    towns_info = std::vector<town_type>(town_numbers);
    for (auto i = 1; i < town_numbers; i++) {
        auto &town = towns_info[i];
        for (auto sst : STORE_SALE_TYPE_LIST) {
            auto *store_ptr = &town.stores[sst];
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
    for (auto i = 0; i < MAX_BUILDINGS; i++) {
        buildings[i].name[0] = '\0';
        buildings[i].owner_name[0] = '\0';
        buildings[i].owner_race[0] = '\0';
        for (auto j = 0; j < 8; j++) {
            buildings[i].act_names[j][0] = '\0';
            buildings[i].member_costs[j] = 0;
            buildings[i].other_costs[j] = 0;
            buildings[i].letters[j] = 0;
            buildings[i].actions[j] = 0;
            buildings[i].action_restr[j] = 0;
        }

        buildings[i].member_class.assign(PLAYER_CLASS_TYPE_MAX, static_cast<short>(PlayerClassType::WARRIOR));
        buildings[i].member_race.assign(MAX_RACES, static_cast<short>(PlayerRaceType::HUMAN));
        buildings[i].member_realm.assign(MAX_MAGIC + 1, 0);
    }
}
