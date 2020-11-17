#pragma once

#include "system/angband.h"

/*
 * Sort-array element
 */
typedef struct tag_type {
    int tag;
    int index;
} tag_type;

void tag_sort(tag_type elements[], int number);
