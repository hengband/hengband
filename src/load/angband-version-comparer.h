#pragma once

#include <stdint.h>

bool h_older_than(uint8_t major, uint8_t minor, uint8_t patch, uint8_t extra);
bool h_older_than(uint8_t major, uint8_t minor, uint8_t patch);
