#pragma once

#include "system/angband.h"

/*
 * Sort-array element
 */
struct tag_type {
    int tag;
    int index;
};

void tag_sort(tag_type elements[], int number);
