#pragma once

#include "system/angband.h"
#include "wizard/spoiler-util.h"

void spoiler_outlist(concptr header, concptr *list, char separator);
spoiler_output_status spoil_fixed_artifact(concptr fname);
