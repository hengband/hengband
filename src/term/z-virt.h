/* File: z-virt.h */

/*
 * Copyright (c) 1997 Ben Harrison
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

#ifndef INCLUDED_Z_VIRT_H
#define INCLUDED_Z_VIRT_H

#include "system/h-basic.h"

/*
 * Memory management routines.
 *
 * The string_make() and string_free() routines handle dynamic strings.
 * A dynamic string is a string allocated at run-time, which should not
 * be modified once it has been created.
 */

/**** Available functions ****/

/* Create a "dynamic string" */
extern concptr string_make(concptr str);

/* Free a string allocated with "string_make()" */
extern errr string_free(concptr str);

#endif
