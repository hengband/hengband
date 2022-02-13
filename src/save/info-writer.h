#pragma once

#include "save/save.h"

struct store_type;
void wr_store(store_type *store_ptr);
void wr_randomizer(void);
void wr_options(save_type type);
void wr_ghost(void);
void save_quick_start(void);
