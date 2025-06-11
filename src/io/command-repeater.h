#pragma once

#include <tl/optional.hpp>

void repeat_push(short command);
tl::optional<short> repeat_pull();
void repeat_check();
