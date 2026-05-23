#pragma once

#include "util/abstract-vector-wrapper.h"
#include <vector>

constexpr short VALID_TOWNS = 6; // @details 旧海底都市クエストのマップを除外する. 有効な町に差し替え完了したら不要になるので注意.

class TownInfo;
class TownList : public util::AbstractVectorWrapper<TownInfo> {
public:
    TownList(TownList &&) = delete;
    TownList(const TownList &) = delete;
    TownList &operator=(const TownList &) = delete;
    TownList &operator=(TownList &&) = delete;

    static TownList &get_instance();

    void initialize();

    const TownInfo &get_town(size_t index) const;
    TownInfo &get_town(size_t index);

private:
    TownList() = default;

    static TownList instance;
    std::vector<TownInfo> towns;

    std::vector<TownInfo> &get_inner_container() override
    {
        return this->towns;
    }
};
