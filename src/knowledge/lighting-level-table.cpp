#include "knowledge/lighting-level-table.h"

/*!
 * @brief キャラクタ色の明暗表現
 */
concptr lighting_level_str[F_LIT_MAX] = {
    _("標準色", "standard"),
    _("明色", "brightly lit"),
    _("暗色", "darkened"),
};
