#pragma once

#include "system/angband.h"
#include "wizard/spoiler-util.h"

spoiler_output_status spoil_mon_desc_all(concptr fname);
spoiler_output_status spoil_mon_desc(concptr fname, bool show_all, BIT_FLAGS RF8_flags);
spoiler_output_status spoil_mon_info(concptr fname);
