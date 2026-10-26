#include "game-option/option-flags.h"
#include "system/deferred-update-requests.h"

std::array<uint32_t, MAX_WINDOW_ENTITIES> g_option_flags = {};
std::array<uint32_t, MAX_WINDOW_ENTITIES> g_option_masks = {};
std::array<EnumClassFlagGroup<SubWindowRedrawingFlag>, MAX_WINDOW_ENTITIES> g_window_flags = {};
std::array<EnumClassFlagGroup<SubWindowRedrawingFlag>, MAX_WINDOW_ENTITIES> g_window_masks = {};
