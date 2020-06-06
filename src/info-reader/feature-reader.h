#pragma once

#include "system/angband.h"
#include "info-reader/info-reader-util.h"

extern angband_header f_head;

errr parse_f_info(char *buf, angband_header *head);
errr init_feat_variables(void);
s16b f_tag_to_index(concptr str);
s16b f_tag_to_index_in_init(concptr str);
