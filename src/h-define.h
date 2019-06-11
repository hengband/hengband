/*!
 * @file h-define.h
 * @brief 変愚蛮怒で新しく追加された主要なマクロ定義ヘッダ / Define some simple constants
 * @date 2014/08/16
 * @author
 * 不明(変愚蛮怒開発チーム？)
 */

#ifndef INCLUDED_H_DEFINE_H
#define INCLUDED_H_DEFINE_H

/*
 * Hack -- Define NULL
 */
#ifndef NULL
# ifdef __STDC__
#  define NULL ((void*)0) /*!< コンパイル環境に定義がない場合のNULL定義 */
# else
#  define NULL ((char*)0) /*!< コンパイル環境に定義がない場合のNULL定義 */
# endif /* __STDC__ */
#endif /* NULL */


/*
 * Hack -- assist "main-acn.c" 
 */
#ifdef ACORN
# define O_RDONLY	0
# define O_WRONLY	1
# define O_RDWR		2
#endif


/*
 * The constants "TRUE" and "FALSE"
 */

#undef TRUE
#define TRUE	1 /*!< コンパイル環境に定義がない場合のTRUE定義 */

#undef FALSE
#define FALSE	0 /*!< コンパイル環境に定義がない場合のFALSE定義 */




/**** Simple "Macros" ****/

#ifdef ZANGBAND_JP
#define lbtokg(x) ((int)(((x)*4536)/1000)) /*!< Zangband基準のポンド→キログラム変換定義(全体) */
#define lbtokg1(x) ((lbtokg(x)+5)/100) /*!< Zangband基準のポンド→キログラム変換定義(整数部) */
#define lbtokg2(x) ( ( (lbtokg(x)+5)%100)/10)  /*!< Zangband基準のポンド→キログラム変換定義(少数部) */
#elif defined(JP)
#define lbtokg(x) ((int)((x)*5)) /*!< 変愚蛮怒基準のポンド→キログラム変換定義(全体) */
#define lbtokg1(x) (lbtokg(x)/100) /*!< 変愚蛮怒基準のポンド→キログラム変換定義(整数部) */
#define lbtokg2(x) ((lbtokg(x)%100)/10)  /*!< 変愚蛮怒基準のポンド→キログラム変換定義(少数部) */
#endif

/*
 * Force a character to lowercase/uppercase
 */
#define FORCELOWER(A)  ((isupper((A))) ? tolower((A)) : (A))
#define FORCEUPPER(A)  ((islower((A))) ? toupper((A)) : (A))


/*
 * Non-typed minimum value macro
 */
#undef MIN
#define MIN(a,b)	(((a) > (b)) ? (b)  : (a))

/*
 * Non-typed maximum value macro
 */
#undef MAX
#define MAX(a,b)	(((a) < (b)) ? (b)  : (a))

/*
 * Non-typed absolute value macro
 */
#undef ABS
#define ABS(a)		(((a) < 0)   ? (-(a)) : (a))

/*
 * Non-typed sign extractor macro
 */
#undef SGN
#define SGN(a)		(((a) < 0)   ? (-1) : ((a) != 0))

/*
 * Refer to the member at offset of structure
 */
#define atoffset(TYPE, STRUCT_PTR, OFFSET) (*(TYPE*)(((char*)STRUCT_PTR) + (OFFSET)))

#endif

