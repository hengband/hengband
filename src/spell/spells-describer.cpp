#include "spell/spells-describer.h"
#include "locale/language-switcher.h"
#include "object/tval-types.h"

/*!
 * @brief 領域魔法に応じて技能の名称を返す。
 * @param tval 魔法書のtval
 * @return 領域魔法の技能名称を保管した文字列ポインタ
 */
std::string spell_category_name(ItemKindType tval)
{
    switch (tval) {
    case ItemKindType::HISSATSU_BOOK:
        return _("必殺技", "art");
    case ItemKindType::LIFE_BOOK:
        return _("祈り", "prayer");
    case ItemKindType::MUSIC_BOOK:
        return _("歌", "song");
    default:
        return _("呪文", "spell");
    }
}
