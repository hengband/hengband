#pragma once

#include <optional>

void repeat_push(short command);
std::optional<short> repeat_pull();
void repeat_check();
