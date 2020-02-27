#pragma once

#include "angband.h"

#define MAX_NAMED_NUM 99

typedef struct named_num
{
	concptr name;		/* The name of this thing */
	int num;			/* A number associated with it */
} named_num;

named_num gf_desc[MAX_NAMED_NUM];
