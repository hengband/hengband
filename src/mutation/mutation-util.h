#pragma once

#include "system/angband.h"

// Gain and Lose Mutation.
typedef struct glm_type {
    BIT_FLAGS *muta_class;
    int muta_which; // mutation_flag_type_1 とmutation_flag_type_2 の両対応とするため、敢えてint型で定義する
    concptr muta_desc;
    bool muta_chosen;
    MUTATION_IDX choose_mut;
} glm_type;

glm_type *initialize_glm_type(glm_type *gm_ptr, MUTATION_IDX choose_mut);
