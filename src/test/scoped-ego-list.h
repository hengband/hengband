#pragma once

#include "object-enchant/object-ego.h"

namespace test {

// エゴ定義を空の一覧へ交換し、スコープを抜けるときに元の一覧を復元する。
class ScopedEgoList {
public:
    ScopedEgoList()
    {
        this->saved_egos.swap(egos_info);
    }

    ~ScopedEgoList()
    {
        egos_info.swap(this->saved_egos);
    }

    ScopedEgoList(const ScopedEgoList &) = delete;
    ScopedEgoList &operator=(const ScopedEgoList &) = delete;

private:
    decltype(egos_info) saved_egos;
};

}
