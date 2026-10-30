#include "info-reader/monster-flag-reader.h"
#include "info-reader/race-info-tokens-table.h"
#include "locale/language-switcher.h"
#include "view/display-messages.h"

/*!
 * @brief フラグ名を能力フラグ(魔法・ブレス)の表で探し、見つかったフラグを1つ立てる
 * @details 見つからなかったときはフラグを変えず、未知のモンスター・フラグとして表示する。
 * @param ability_flags 能力フラグの格納先
 * @param what フラグ名
 * @return 見つけたら true
 */
bool grab_one_monster_spell_flag(EnumClassFlagGroup<MonsterAbilityType> &ability_flags, std::string_view what)
{
    if (EnumClassFlagGroup<MonsterAbilityType>::grab_one_flag(ability_flags, r_info_ability_flags, what)) {
        return true;
    }

    msg_print(_("未知のモンスター・フラグ '{}'。", "Unknown monster flag '{}'."), what);
    return false;
}
