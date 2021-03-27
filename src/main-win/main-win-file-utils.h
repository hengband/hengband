#pragma once

#include "system/h-type.h"

#include <initializer_list>
#include <string>

bool check_file(concptr s);
bool check_dir(concptr s);
std::string find_any_file(concptr dir, std::initializer_list<concptr> files);
