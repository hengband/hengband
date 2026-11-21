#pragma once

#include "spell/magic-type.h"
#include <tl/optional.hpp>

/*!
 * @brief 得意さの段階 1 つあたりの、職業別の値の動き (ClassMagicDefinitions の aptitude_steps)
 */
struct AptitudeSteps {
    int learn_level_percent{}; //!< 習得レベルの動き (基準 × (50 − 基準) / 50 に対する百分率)
    int mana_cost_percent{}; //!< 消費MP の倍率の動き (百分率)
    int difficulty{}; //!< 難易度の動き
};

tl::optional<magic_type> derive_magic_by_aptitude(const magic_type &base, const AptitudeSteps &steps, int aptitude);
