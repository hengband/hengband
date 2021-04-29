#include "player-info/alignment.h"
#include "game-option/text-display-options.h"
#include "system/player-type-definition.h"

PlayerAlignment::PlayerAlignment(player_type *creature_ptr)
{
    this->creature_ptr = creature_ptr;
}

/*!
 * @brief クリーチャーの抽象的善悪アライメントの表記名のみを返す。 / Return only alignment title
 * @param creature_ptr 算出するクリーチャーの参照ポインタ。
 * @return アライメントの表記名
 */
concptr PlayerAlignment::alignment_label()
{
    if (this->creature_ptr->alignment > 150)
        return _("大善", "Lawful");
    else if (this->creature_ptr->alignment > 50)
        return _("中善", "Good");
    else if (this->creature_ptr->alignment > 10)
        return _("小善", "Neutral Good");
    else if (this->creature_ptr->alignment > -11)
        return _("中立", "Neutral");
    else if (this->creature_ptr->alignment > -51)
        return _("小悪", "Neutral Evil");
    else if (this->creature_ptr->alignment > -151)
        return _("中悪", "Evil");
    else
        return _("大悪", "Chaotic");
}

/*!
 * @brief クリーチャーの抽象的善悪アライメントの表記を返す。 / Return alignment title
 * @param creature_ptr 算出するクリーチャーの参照ポインタ。
 * @return アライメントの表記を返す。
 */
concptr PlayerAlignment::get_alignment_description(bool with_value)
{
    auto s = alignment_label();
    if (with_value || show_actual_value)
        return format(_("%s(%ld)", "%s (%ld)"), s, static_cast<long>(this->creature_ptr->alignment));

    return s;
}
