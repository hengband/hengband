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
// データコピーマクロ COPY/C_COPY の実装
//

// トリビアルコピーが可能な型は memcpy でコピーする
template <typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, std::nullptr_t> = nullptr>
inline T *c_copy_impl(T *p1, T *p2, size_t n)
{
    return static_cast<T *>(memcpy(p1, p2, sizeof(T) * n));
}

// トリビアルコピーが不可能な型は要素を1つずつコピー代入する
template <typename T, std::enable_if_t<!std::is_trivially_copyable_v<T>, std::nullptr_t> = nullptr>
inline T *c_copy_impl(T *p1, T *p2, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        *(p1 + i) = *(p2 + i);
    }
    return p1;
}

// 単要素の場合はトリビアルコピーの可/不可に関わらずコピー代入する
template <typename T>
inline T *copy_impl(T *p1, T *p2)
{
    *p1 = *p2;
    return p1;
}

//
// メモリ領域確保マクロ RNEW/C_RNEW の実装
// MAKE/C_MAKE から最終的に呼ばれる
//

template <typename T>
inline T *c_rnew_impl(size_t count)
{
    return new T[count];
}

template <typename T>
inline T *rnew_impl()
{
    return new T;
}

//
// メモリ領域解放マクロ FREE/C_FREE の実装
// KILL/C_KILL からも FREE/C_FREE を介して呼ばれる
//

template <typename T>
inline T *c_free_impl(T *p, size_t count)
{
    (void)count; // unused;

    delete[] p;
    return nullptr;
}

template <typename T>
inline T *free_impl(T *p)
{
    delete p;
    return nullptr;
}

/**** Available macros ****/

/* Size of 'N' things of type 'T' */
#define C_SIZE(N, T) ((ulong)((N) * (sizeof(T))))

/* Size of one thing of type 'T' */
#define SIZE(T) ((ulong)(sizeof(T)))

#if 0
/* Compare two arrays of type T[N], at locations P1 and P2 */
#define C_DIFF(P1, P2, N, T) (memcmp((char *)(P1), (char *)(P2), C_SIZE(N, T)))

/* Compare two things of type T, at locations P1 and P2 */
#define DIFF(P1, P2, T) (memcmp((char *)(P1), (char *)(P2), SIZE(T)))

/* Set every byte in an array of type T[N], at location P, to V, and return P */
#define C_BSET(P, V, N, T) (T *)(memset((char *)(P), (V), C_SIZE(N, T)))

/* Set every byte in a thing of type T, at location P, to V, and return P */
#define BSET(P, V, T) (T *)(memset((char *)(P), (V), SIZE(T)))
#endif

/* Wipe an array of type T[N], at location P, and return P */
#define C_WIPE(P, N, T) (c_wipe_impl<T>(P, N))

/* Wipe a thing of type T, at location P, and return P */
#define WIPE(P, T) (wipe_impl<T>(P))

/* Load an array of type T[N], at location P1, from another, at location P2 */
#define C_COPY(P1, P2, N, T) (c_copy_impl<T>(P1, P2, N))

/* Load a thing of type T, at location P1, from another, at location P2 */
#define COPY(P1, P2, T) (copy_impl<T>(P1, P2))

/* Free an array of N things of type T at P, return nullptr */
#define C_FREE(P, N, T) (c_free_impl<T>(P, N))

/* Free one thing of type T at P, return nullptr */
#define FREE(P, T) (free_impl<T>(P))

/* Allocate, and return, an array of type T[N] */
#define C_RNEW(N, T) (c_rnew_impl<T>(N))

/* Allocate, and return, a thing of type T */
#define RNEW(T) (rnew_impl<T>())

/* Allocate, wipe, and return an array of type T[N] */
#define C_ZNEW(N, T) ((T *)(C_WIPE(C_RNEW(N, T), N, T)))

/* Allocate, wipe, and return a thing of type T */
#define ZNEW(T) ((T *)(WIPE(RNEW(T), T)))

/* Allocate a wiped array of type T[N], assign to pointer P */
#define C_MAKE(P, N, T) ((P) = C_ZNEW(N, T))

/* Allocate a wiped thing of type T, assign to pointer P */
#define MAKE(P, T) ((P) = ZNEW(T))

/* Free an array of type T[N], at location P, and set P to nullptr */
#define C_KILL(P, N, T) ((P) = C_FREE(P, N, T))

/* Free a thing of type T, at location P, and set P to nullptr */
#define KILL(P, T) ((P) = FREE(P, T))

/**** Available functions ****/

/* Create a "dynamic string" */
extern concptr string_make(concptr str);

/* Free a string allocated with "string_make()" */
extern errr string_free(concptr str);

#endif
