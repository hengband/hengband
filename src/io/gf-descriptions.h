#pragma once

#include "system/angband.h"
#include "effect/attribute-types.h"
#include <vector>

struct named_num
{
	concptr name;		/* The name of this thing */
    AttributeType num; /* A number associated with it */
};

extern const std::vector<named_num> gf_descriptions;
