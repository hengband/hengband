#pragma once

#include "system/angband.h"
#include "wizard/spoiler-util.h"

void spoiler_outlist(std::string_view header, const std::vector<std::string> &descriptions, char seperator);
SpoilerOutputResultType spoil_fixed_artifact(concptr fname);
