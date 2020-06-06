#pragma once

#include "system/angband.h"
#include "main/init.h"

bool add_text(u32b *offset, angband_header *head, concptr buf, bool normal_text);
bool add_name(u32b *offset, angband_header *head, concptr buf);
bool add_tag(STR_OFFSET *offset, angband_header *head, concptr buf);
