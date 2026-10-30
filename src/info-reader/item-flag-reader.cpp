#include "info-reader/item-flag-reader.h"
#include "info-reader/baseitem-tokens-table.h"

/*!
 * @brief フラグ名を特性フラグ・生成フラグの順に探し、見つかった方のフラグを1つ立てる
 * @details 見つからなかったときはどちらのフラグも変えない。未知のフラグ名の表示は、定義の種類ごとに文言が違うため呼び出し側で行う。
 * @param flags 特性フラグの格納先
 * @param gen_flags 生成フラグの格納先
 * @param what フラグ名
 * @return どちらかの表で見つかったら true
 */
bool grab_one_item_flag(TrFlags &flags, EnumClassFlagGroup<ItemGenerationTraitType> &gen_flags, std::string_view what)
{
    if (TrFlags::grab_one_flag(flags, baseitem_flags, what)) {
        return true;
    }

    return EnumClassFlagGroup<ItemGenerationTraitType>::grab_one_flag(gen_flags, baseitem_geneneration_flags, what);
}
