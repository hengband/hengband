#include "spell/spells-describer.h"

/*!
 * @brief 領域魔法に応じて技能の名称を返す。
 * @param tval 魔法書のtval
 * @return 領域魔法の技能名称を保管した文字列ポインタ
 */
concptr spell_category_name(tval_type tval)
{
    switch (tval) {
    case TV_HISSATSU_BOOK:
        return _("必殺技", "art");
    case TV_LIFE_BOOK:
        return _("祈り", "prayer");
    case TV_MUSIC_BOOK:
        return _("歌", "song");
    default:
        return _("呪文", "spell");
    }
}
