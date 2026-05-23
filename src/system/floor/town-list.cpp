#include "system/floor/town-list.h"
#include "io/files-util.h"
#include "store/articles-on-sale.h"
#include "store/store.h"
#include "system/angband-exceptions.h"
#include "system/baseitem/baseitem-list.h"
#include "system/floor/town-info.h"
#include "util/angband-files.h"
#include <fmt/format.h>

namespace {
/*!
 * @brief ユニークな町の数を数える
 * @return ユニークな町の数
 * @details 町定義ファイル名の先頭2文字は番号であることを利用してカウントする.
 * 辺境の地を表すファイルは(01_*.txt) は3つあるのでユニークではない. また町番号は1から始まるので最後に加算する.
 */
int count_town_numbers()
{
    const auto path = path_build(ANGBAND_DIR_EDIT, "towns");
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
}

TownList TownList::instance{};

TownList &TownList::get_instance()
{
    return instance;
}

const TownInfo &TownList::get_town(size_t index) const
{
    if (index >= this->size()) {
        THROW_EXCEPTION(std::out_of_range, fmt::format("Invalid town index: {}", index));
    }

    return this->towns[index];
}

TownInfo &TownList::get_town(size_t index)
{
    if (index >= this->size()) {
        THROW_EXCEPTION(std::out_of_range, fmt::format("Invalid town index: {}", index));
    }

    return this->towns[index];
}

/*!
 * @brief 町情報読み込みのメインルーチン
 * @details 「我が家を拡張する」オプションのON/OFFとは無関係に、ON時の容量を確保しておく.
 */
void TownList::initialize()
{
    const auto &baseitems = BaseitemList::get_instance();
    const auto town_numbers = count_town_numbers();
    this->towns = std::vector<TownInfo>(town_numbers);
    for (auto i = 1; i < town_numbers; i++) {
        auto &town = towns[i];
        for (auto sst : STORE_SALE_TYPE_LIST) {
            auto &store = town.emplace(sst);
            if ((i > 1) && (sst == StoreSaleType::MUSEUM || sst == StoreSaleType::HOME)) {
                continue;
            }

            store.stock_size = store_get_stock_max(sst);
            std::vector<std::unique_ptr<ItemEntity>> stock;
            for (auto j = 0; j < store.stock_size; j++) {
                stock.push_back(std::make_unique<ItemEntity>());
            }

            store.stock = std::move(stock);
            if ((sst == StoreSaleType::BLACK) || (sst == StoreSaleType::HOME) || (sst == StoreSaleType::MUSEUM)) {
                continue;
            }

            for (const auto &bi_key : store_regular_sale_table.at(sst)) {
                const auto bi_id = baseitems.lookup_baseitem_id(bi_key);
                store.regular.push_back(bi_id);
            }

            for (const auto &bi_key : store_sale_table.at(sst)) {
                const auto bi_id = baseitems.lookup_baseitem_id(bi_key);
                store.table.push_back(bi_id);
            }
        }
    }
}
