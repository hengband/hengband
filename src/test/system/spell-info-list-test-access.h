#pragma once

#include "system/spell-info-list.h"
#include <utility>
#include <vector>

namespace test {

/*! @brief 呪文定義表を空の初期状態に差し替え、未初期化状態を含む元の表を復元する。 */
class SpellInfoListTestAccess {
public:
    SpellInfoListTestAccess()
        : spells(SpellInfoList::get_instance())
        , saved(std::move(spells.spell_list))
    {
        spells.initialize();
    }

    SpellInfoListTestAccess(const SpellInfoListTestAccess &) = delete;
    SpellInfoListTestAccess &operator=(const SpellInfoListTestAccess &) = delete;

    ~SpellInfoListTestAccess()
    {
        spells.spell_list.swap(saved);
    }

private:
    SpellInfoList &spells;
    std::vector<std::vector<SpellInfo>> saved;
};

}
