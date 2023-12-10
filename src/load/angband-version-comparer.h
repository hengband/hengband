#pragma once

#include <stdint.h>

bool h_older_than(uint8_t major, uint8_t minor, uint8_t state, uint8_t build);
bool h_older_than(uint8_t major, uint8_t minor, uint8_t state);
