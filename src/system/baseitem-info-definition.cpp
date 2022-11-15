/*!
 * @brief ベースアイテム情報の構造体 / Information about object "kinds", including player knowledge.
 * @date 2019/05/01
 * @author deskull
 * @details
 * ゲーム進行用のセーブファイル上では aware と tried のみ保存対象とすること。と英文ではあるが実際はもっとある様子である。 /
 * Only "aware" and "tried" are saved in the savefile
 */

#include "system/baseitem-info-definition.h"
#include "object/tval-types.h"

BaseitemKey::BaseitemKey(const ItemKindType type_value, const std::optional<int> &subtype_value)
    : type_value(type_value)
    , subtype_value(subtype_value)
{
}

bool BaseitemKey::operator==(const BaseitemKey &other) const
{
    return (this->type_value == other.type_value) && (this->subtype_value == other.subtype_value);
}

// @details type_valueに大小があればそれを判定し、同一ならばsubtype_valueの大小を判定する.
bool BaseitemKey::operator<(const BaseitemKey &other) const
{
    if (this->type_value < other.type_value) {
        return true;
    }

    if (this->type_value > other.type_value) {
        return false;
    }

    return this->subtype_value < other.subtype_value;
}

ItemKindType BaseitemKey::tval() const
{
    return this->type_value;
}

std::optional<int> BaseitemKey::sval() const
{
    return this->subtype_value;
}

BaseitemInfo::BaseitemInfo()
    : bi_key(ItemKindType::NONE)
{
}

std::vector<BaseitemInfo> baseitems_info;
