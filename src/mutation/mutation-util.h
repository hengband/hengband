#pragma once

#include "system/angband.h"

enum class PlayerMutationType;

struct glm_type {
    PlayerMutationType muta_which;
    concptr muta_desc;
    bool muta_chosen;
    MUTATION_IDX choose_mut;
};

glm_type *initialize_glm_type(glm_type *gm_ptr, MUTATION_IDX choose_mut);
