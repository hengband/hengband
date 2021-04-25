#pragma once

#include "system/angband.h"
#include "parse-error-types.h"
#include "info-reader/info-reader-util.h"

errr parse_e_info(char *buf, angband_header *head);
