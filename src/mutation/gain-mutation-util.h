#pragma once

#include "system/angband.h"

// Gain Mutation.
typedef struct gm_type {
    BIT_FLAGS *muta_class;
    int muta_which; // mutation_flag_type_1 とmutation_flag_type_2 の両対応とするため、敢えてint型で定義する
    concptr muta_desc;
    bool muta_chosen;
    MUTATION_IDX choose_mut;
} gm_type;

gm_type *initialize_gm_type(gm_type *gm_ptr, MUTATION_IDX choose_mut);
