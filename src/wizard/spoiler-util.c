#include "wizard/spoiler-util.h"

const char item_separator = ',';
const char list_separator = _(',', ';');
const int max_evolution_depth = 64;
concptr spoiler_indent = "    ";

/* The spoiler file being created */
FILE *spoiler_file = NULL;
