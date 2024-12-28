/*!
 * @brief モンスター生成フィルタのデバッグ用統計情報実装
 * @author Hourier
 * @date 2024/12/28
 */

#include "wizard/monrace-filter-debug-info.h"
#include "floor/floor-base-definitions.h"
#include "locale/language-switcher.h"
#include "term/z-form.h"

MonraceFilterDebugInfo::MonraceFilterDebugInfo()
    : min_level(MAX_DEPTH)
{
}

void MonraceFilterDebugInfo::update(int probabilty, int level)
{
    if (probabilty <= 0) {
        return;
    }

    this->num_monrace++;
    if (this->min_level > level) {
        this->min_level = level;
    }

    if (this->max_level < level) {
        this->max_level = level;
    }

    this->total_probability += probabilty;
}

std::string MonraceFilterDebugInfo::to_string() const
{
    constexpr auto fmt = _("モンスター第2次候補数:%d(%d-%dF)%d ", "monster second selection:%d(%d-%dF)%d ");
    return format(fmt, this->num_monrace, this->min_level, this->max_level, this->total_probability);
}
