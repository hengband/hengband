#pragma once

#include "system/angband.h"
#include "wizard/spoiler-util.h"

void spoiler_outlist(concptr header, concptr *list, char separator);
SpoilerOutputResultType spoil_fixed_artifact(concptr fname);
