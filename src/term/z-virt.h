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

#include <algorithm>
#include <type_traits>

/*
 * Memory management routines.
 *
 * These routines work best as a *replacement* for malloc/free.
 *
 * The string_make() and string_free() routines handle dynamic strings.
 * A dynamic string is a string allocated at run-time, which should not
 * be modified once it has been created.
 *
 * Note the macros below which simplify the details of allocation,
 * deallocation, setting, clearing, casting, size extraction, etc.
 *
 * The macros MAKE/C_MAKE and KILL/C_KILL have a "procedural" metaphor,
 * and they actually modify their arguments.
 *
 * Note that, for some reason, some allocation macros may disallow
 * "stars" in type names, but you can use typedefs to circumvent
 * this.  For example, instead of "type **p; MAKE(p,type*);" you
 * can use "typedef type *type_ptr; type_ptr *p; MAKE(p,type_ptr)".
 *
 * Note that it is assumed that "memset()" will function correctly,
 * in particular, that it returns its first argument.
 */

//
// データクリアマクロ WIPE/C_WIPE の実装
//

// トリビアル型は memset でゼロクリアする
template <typename T, std::enable_if_t<std::is_trivial_v<T>, std::nullptr_t> = nullptr>
inline T *c_wipe_impl(T *p, size_t n)
{
    return static_cast<T *>(memset(p, 0, sizeof(T) * n));
}
template <typename T, std::enable_if_t<std::is_trivial_v<T>, std::nullptr_t> = nullptr>
inline T *wipe_impl(T *p)
{
    return static_cast<T *>(memset(p, 0, sizeof(T)));
}

// 非トリビアル型はデフォルトコンストラクタで生成したオブジェクトをコピーする
template <typename T, std::enable_if_t<!std::is_trivial_v<T>, std::nullptr_t> = nullptr>
inline T *c_wipe_impl(T *p, size_t n)
{
    std::fill_n(p, n, T{});
    return p;
}
template <typename T, std::enable_if_t<!std::is_trivial_v<T>, std::nullptr_t> = nullptr>
inline T *wipe_impl(T *p)
{
    *p = T{};
    return p;
}

//
// データコピーマクロ COPY の実装
//

// 単要素の場合はトリビアルコピーの可/不可に関わらずコピー代入する
template <typename T>
inline T *copy_impl(T *p1, T *p2)
{
    *p1 = *p2;
    return p1;
}

/**** Available macros ****/

/* Wipe an array of type T[N], at location P, and return P */
#define C_WIPE(P, N, T) (c_wipe_impl<T>(P, N))

/* Wipe a thing of type T, at location P, and return P */
#define WIPE(P, T) (wipe_impl<T>(P))

/* Load a thing of type T, at location P1, from another, at location P2 */
#define COPY(P1, P2, T) (copy_impl<T>(P1, P2))

/**** Available functions ****/

/* Create a "dynamic string" */
extern concptr string_make(concptr str);

/* Free a string allocated with "string_make()" */
extern errr string_free(concptr str);

#endif
