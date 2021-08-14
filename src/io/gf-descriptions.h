#pragma once

#include "system/angband.h"

#define MAX_NAMED_NUM 103

typedef struct named_num
{
	concptr name;		/* The name of this thing */
	int num;			/* A number associated with it */
} named_num;

extern const named_num gf_desc[MAX_NAMED_NUM];
