#pragma once

#include "system/redrawing-flags-updater.h"
#include "util/flag-group.h"
#include <array>
#include <stdint.h>

constexpr auto MAX_WINDOW_ENTITIES = 8;

extern std::array<uint32_t, MAX_WINDOW_ENTITIES> g_option_flags;
extern std::array<uint32_t, MAX_WINDOW_ENTITIES> g_option_masks;
extern std::array<EnumClassFlagGroup<SubWindowRedrawingFlag>, MAX_WINDOW_ENTITIES> g_window_flags;
extern std::array<uint32_t, MAX_WINDOW_ENTITIES> g_window_masks;
