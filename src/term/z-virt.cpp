/* File: z-virt.c */

/*
 * Copyright (c) 1997 Ben Harrison
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

/* Purpose: Memory management routines -BEN- */

#include "term/z-virt.h"
#include <cstring>

/*!
 * @brief str の複製を返す。戻り値は使用後に string_free() で解放すること。
 *
 * nullptr が渡された場合、nullptr を返す。
 */
const char *string_make(const char *str)
{
    if (!str) {
        return nullptr;
    }

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
int string_free(const char *str)
{
    if (!str) {
        return 0;
    }

    delete[] str;

    return 0;
}
