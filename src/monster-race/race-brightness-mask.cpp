#include "monster-race/race-brightness-mask.h"

const EnumClassFlagGroup<MonsterBrightnessType> lite_mask = {
    MonsterBrightnessType::HAS_LITE_1,
    MonsterBrightnessType::SELF_LITE_1,
    MonsterBrightnessType::HAS_LITE_2,
    MonsterBrightnessType::SELF_LITE_2
};

const EnumClassFlagGroup<MonsterBrightnessType> dark_mask = {
    MonsterBrightnessType::HAS_DARK_1,
    MonsterBrightnessType::SELF_DARK_1,
    MonsterBrightnessType::HAS_DARK_2,
    MonsterBrightnessType::SELF_DARK_2
};

const EnumClassFlagGroup<MonsterBrightnessType> has_ld_mask = {
    MonsterBrightnessType::HAS_LITE_1,
    MonsterBrightnessType::HAS_LITE_2,
    MonsterBrightnessType::HAS_DARK_1,
    MonsterBrightnessType::HAS_DARK_2
};

const EnumClassFlagGroup<MonsterBrightnessType> self_ld_mask = {
    MonsterBrightnessType::SELF_LITE_1,
    MonsterBrightnessType::SELF_LITE_2,
    MonsterBrightnessType::SELF_DARK_1,
    MonsterBrightnessType::SELF_DARK_2
};

const EnumClassFlagGroup<MonsterBrightnessType> ld_mask = EnumClassFlagGroup<MonsterBrightnessType>(lite_mask).set(dark_mask);
