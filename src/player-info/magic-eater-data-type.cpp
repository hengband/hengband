#include "player-info/magic-eater-data-type.h"
#include "sv-definition/sv-rod-types.h"
#include "sv-definition/sv-staff-types.h"
#include "sv-definition/sv-wand-types.h"

magic_eater_data_type::magic_eater_data_type()
    : staves(SV_STAFF_MAX)
    , wands(SV_WAND_MAX)
    , rods(SV_ROD_MAX)
{
}

/*!
 * @brief 指定した種類の取り込んだ魔道具のデータの配列を取得する
 *
 * @param tval 魔道具の種類(TV_STAFF/TV_WAND/TV_ROD)を指定する
 * @return tvalで指定した種類の取り込んだ魔道具のデータ配列の参照
 */
std::vector<magic_eater_data_type::magic_type> &magic_eater_data_type::get_item_group(ItemKindType tval)
{
    switch (tval) {
    case ItemKindType::STAFF:
        return this->staves;
    case ItemKindType::WAND:
        return this->wands;
    case ItemKindType::ROD:
        return this->rods;
    default:
        // ダミーデータ。通常使用されることはない。
        return magic_eater_data_type::none;
    }
}
