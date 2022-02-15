#pragma once

#include "system/angband.h"
#include "effect/attribute-types.h"

#define MAX_NAMED_NUM 105

struct named_num
{
	concptr name;		/* The name of this thing */
    AttributeType num; /* A number associated with it */
};

extern const named_num gf_desc[MAX_NAMED_NUM];
