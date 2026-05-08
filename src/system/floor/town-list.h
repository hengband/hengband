#pragma once

#include "system/floor/town-info.h"
#include "util/abstract-vector-wrapper.h"
#include <vector>

constexpr size_t VALID_TOWNS = 6; // @details 旧海底都市クエストのマップを除外する. 有効な町に差し替え完了したら不要になるので注意.

enum class TownId;
class TownList : public util::AbstractVectorWrapper<TownInfo> {
public:
    TownList(TownList &&) = delete;
    TownList(const TownList &) = delete;
    TownList &operator=(const TownList &) = delete;
    TownList &operator=(TownList &&) = delete;
    ~TownList() = default;

    static TownList &get_instance();

    void initialize();
    void overwrite_town_name();
    const TownInfo &get_town(size_t index) const;
    TownInfo &get_town(size_t index);
    const TownInfo &get_town(TownId town_id) const;
    TownInfo &get_town(TownId town_id);
    bool is_all_initialized() const;

private:
    TownList() = default;

    static TownList instance;
    std::vector<TownInfo> towns;

    std::vector<TownInfo> &get_inner_container() override
    {
        return this->towns;
    }
};
