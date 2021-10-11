#include "spell/spells-describer.h"

/*!
 * @brief 領域魔法に応じて技能の名称を返す。
 * @param tval 魔法書のtval
 * @return 領域魔法の技能名称を保管した文字列ポインタ
 */
concptr spell_category_name(ItemPrimaryType tval)
{
    switch (tval) {
    case ItemPrimaryType::TV_HISSATSU_BOOK:
        return _("必殺技", "art");
    case ItemPrimaryType::TV_LIFE_BOOK:
        return _("祈り", "prayer");
    case ItemPrimaryType::TV_MUSIC_BOOK:
        return _("歌", "song");
    default:
        return _("呪文", "spell");
    }
}
