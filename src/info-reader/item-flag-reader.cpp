#include "info-reader/item-flag-reader.h"
#include "info-reader/baseitem-tokens-table.h"
#include "view/display-messages.h"

/*!
 * @brief フラグ名を特性フラグ・生成フラグの順に探し、見つかった方のフラグを1つ立てる
 * @details 見つからなかったときはどちらのフラグも変えない。未知のフラグ名の表示は、定義の種類ごとの文言を受け取る grab_one_item_flag_or_report() で行う。
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

/*!
 * @brief フラグ名を grab_one_item_flag() で探し、見つからなければ未知のフラグ名として表示する
 * @param flags 特性フラグの格納先
 * @param gen_flags 生成フラグの格納先
 * @param what フラグ名
 * @param unknown_flag_format 見つからなかったときに表示する文言の書式。{} にフラグ名が入る
 * @return どちらかの表で見つかったら true
 */
bool grab_one_item_flag_or_report(TrFlags &flags, EnumClassFlagGroup<ItemGenerationTraitType> &gen_flags, std::string_view what,
    fmt::format_string<std::string_view> unknown_flag_format)
{
    if (grab_one_item_flag(flags, gen_flags, what)) {
        return true;
    }

    msg_print(unknown_flag_format, std::string_view(what));
    return false;
}
