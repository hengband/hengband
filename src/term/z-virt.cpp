/* File: z-virt.c */

/*
 * Copyright (c) 1997 Ben Harrison
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

/* Purpose: Memory management routines -BEN- */

#include <cstring>

#include "term/z-util.h"
#include "term/z-virt.h"

/*
 * Optional auxiliary "rnfree" function
 */
vptr (*rnfree_aux)(vptr, huge) = NULL;

/*
 * Free some memory (allocated by ralloc), return NULL
 */
vptr rnfree(vptr p, huge len)
{
    /* Easy to free zero bytes */
    if (len == 0)
        return (NULL);

    /* Use the "aux" function */
    if (rnfree_aux)
        return ((*rnfree_aux)(p, len));

    /* Use "free" */
    free((char *)(p));

    return (NULL);
}

/*
 * Optional auxiliary "rpanic" function
 */
vptr (*rpanic_aux)(huge) = NULL;

/*
 * The system is out of memory, so panic.  If "rpanic_aux" is set,
 * it can be used to free up some memory and do a new "ralloc()",
 * or if not, it can be used to save things, clean up, and exit.
 * By default, this function simply crashes the computer.
 */
vptr rpanic(huge len)
{
    /* Hopefully, we have a real "panic" function */
    if (rpanic_aux)
        return ((*rpanic_aux)(len));

    /* Attempt to crash before icky things happen */
    core("Out of Memory!");
    return ((vptr)(NULL));
}

/*
 * Optional auxiliary "ralloc" function
 */
vptr (*ralloc_aux)(huge) = NULL;

/*
 * Allocate some memory
 */
vptr ralloc(huge len)
{
    vptr mem;

    /* Allow allocation of "zero bytes" */
    if (len == 0)
        return ((vptr)(NULL));

#ifdef VERBOSE_RALLOC

    /* Count allocated memory */
    virt_make += len;

    /* Log important allocations */
    if (len > virt_size) {
        char buf[80];
        sprintf(buf, "Make (%ld): %ld - %ld = %ld.", len, virt_make, virt_kill, virt_make - virt_kill);
        plog(buf);
    }

#endif

    /* Use the aux function if set */
    if (ralloc_aux)
        mem = (*ralloc_aux)(len);

    /* Use malloc() to allocate some memory */
    else
        mem = ((vptr)(malloc((size_t)(len))));

    /* We were able to acquire memory */
    if (!mem)
        mem = rpanic(len);

    /* Return the memory, if any */
    return (mem);
}

/*!
 * @brief str の複製を返す。戻り値は使用後に string_free() で解放すること。
 *
 * nullptr が渡された場合、nullptr を返す。
 */
concptr string_make(const concptr str)
{
    if (!str)
        return nullptr;

    const auto bufsize = std::strlen(str) + 1;
    auto *const buf = new char[bufsize];
    std::strcpy(buf, str);

    return buf;
}

/*!
 * @brief string_make() で割り当てたバッファを解放する。
 * @return 常に 0
 *
 * nullptr が渡された場合、何もせず 0 を返す。
 */
errr string_free(const concptr str)
{
    if (!str)
        return 0;

    delete[] str;

    return 0;
}
