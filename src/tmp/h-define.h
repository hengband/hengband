/* File: h-define.h */

#ifndef INCLUDED_H_DEFINE_H
#define INCLUDED_H_DEFINE_H

/*
 * Define some simple constants
 */


/*
 * Hack -- Define NULL
 */
#ifndef NULL
# ifdef __STDC__
#  define NULL ((void*)0)
# else
#  define NULL ((char*)0)
# endif /* __STDC__ */
#endif /* NULL */


/*
 * Hack -- assist "main-acn.c" XXX XXX XXX
 */
#ifdef ACORN
# define O_RDONLY	0
# define O_WRONLY	1
# define O_RDWR		2
#endif


/*
 * Hack -- force definitions -- see fd_seek()
 */
#ifndef SEEK_SET
# define SEEK_SET	0
#endif
#ifndef SEEK_CUR
# define SEEK_CUR	1
#endif
#ifndef SEEK_END
# define SEEK_END	2
#endif

/*
 * Hack -- force definitions -- see fd_lock()  XXX XXX XXX
 */
#ifndef F_UNLCK
# define F_UNLCK	0
#endif
#ifndef F_RDLCK
# define F_RDLCK	1
#endif
#ifndef F_WRLCK
# define F_WRLCK	2
#endif


/*
 * The constants "TRUE" and "FALSE"
 */

#undef TRUE
#define TRUE	1

#undef FALSE
#define FALSE	0




/**** Simple "Macros" ****/

#ifdef ZANGBAND_JP
#define lbtokg(x) ((int)(((x)*4536)/1000))
#define lbtokg1(x) ((lbtokg(x)+5)/100)
#define lbtokg2(x) ( ( (lbtokg(x)+5)%100)/10) 
#elif defined(JP)
#define lbtokg(x) ((int)((x)*5))
#define lbtokg1(x) (lbtokg(x)/100)
#define lbtokg2(x) ((lbtokg(x)%100)/10) 
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
 * Hack -- allow use of "ASCII" and "EBCDIC" for "indexes", "digits",
 * and "Control-Characters".
 *
 * Note that all "index" values must be "lowercase letters", while
 * all "digits" must be "digits".  Control characters can be made
 * from any legal characters.  XXX XXX XXX
 */
#ifdef VM
#  define A2I(X)	alphatoindex(X)
#  define I2A(X)	indextoalpha(X)
#  define D2I(X)	((X) - '0')
#  define I2D(X)	((X) + '0')
#  define KTRL(X)	((X) & 0x1F)
#  define ESCAPE	'\033'
#else
#  define A2I(X)	((X) - 'a')
#  define I2A(X)	((X) + 'a')
#  define D2I(X)	((X) - '0')
#  define I2D(X)	((X) + '0')
#  define KTRL(X)	((X) & 0x1F)
#  define ESCAPE	'\033'
#endif

/*
 * Refer to the member at offset of structure
 */
#define atoffset(TYPE, STRUCT_PTR, OFFSET) (*(TYPE*)(((char*)STRUCT_PTR) + (OFFSET)))

#endif

