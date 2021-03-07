#pragma once

#include "system/angband.h"
#include "wizard/spoiler-util.h"

enum race_flags8 : uint32_t;
spoiler_output_status spoil_mon_desc_all(concptr fname);
spoiler_output_status spoil_mon_desc(concptr fname, bool show_all, race_flags8 RF8_flags);
spoiler_output_status spoil_mon_info(concptr fname);
