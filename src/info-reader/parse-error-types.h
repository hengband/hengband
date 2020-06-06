#pragma once

/*
 * @details 2 - 4は使われなくなったので欠番
 */
typedef enum parse_error_type {
	PARSE_ERROR_GENERIC = 1,
    PARSE_ERROR_INVALID_FLAG = 5,
    PARSE_ERROR_UNDEFINED_DIRECTIVE = 6,
    PARSE_ERROR_OUT_OF_MEMORY = 7,
    PARSE_ERROR_OUT_OF_BOUNDS = 8,
    PARSE_ERROR_TOO_FEW_ARGUMENTS = 9,
    PARSE_ERROR_UNDEFINED_TERRAIN_TAG = 10,
    PARSE_ERROR_MAX = 11,
} parse_error_type;
