#pragma once

enum class SaveType;
struct store_type;

void wr_store(store_type *store_ptr);
void wr_randomizer(void);
void wr_options(SaveType type);
void wr_ghost(void);
void save_quick_start(void);
