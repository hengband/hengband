#pragma once

#include <cstdint>

bool h_older_than(uint8_t major, uint8_t minor, uint8_t patch, uint8_t extra);
bool h_older_than(uint8_t major, uint8_t minor, uint8_t patch);
