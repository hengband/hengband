#pragma once

#include <map>
#include <vector>

enum class StoreSaleType;
class BaseitemKey;
extern const std::map<StoreSaleType, std::vector<BaseitemKey>> store_regular_sale_table;
extern const std::map<StoreSaleType, std::vector<BaseitemKey>> store_sale_table;
