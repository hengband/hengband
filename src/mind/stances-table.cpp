#include "mind/stances-table.h"

/*!
 * @brief 修行僧の構え能力テーブル
 */
const std::vector<blow_stance> monk_stances = {
    { _("玄武", "Genbu"), 25, _("", "(Black Tortoise) ") },
    { _("白虎", "Byakko"), 30, _("", "(White Tiger) ") },
    { _("青竜", "Seiryuu"), 35, _("", "(Blue Dragon) ") },
    { _("朱雀", "Suzaku"), 40, _("", "(Red Phoenix) ") },
};

/*!
 * @brief 剣術家の構え能力テーブル
 */
const std::vector<blow_stance> samurai_stances = {
    { _("居合", "Iai"), 25, "" },
    { _("風塵", "Huujin"), 30, "" },
    { _("降鬼", "Kouki"), 35, "" },
    { _("無想", "Musou"), 40, "" },
};
