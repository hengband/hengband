#pragma once

/*
 * Allow use of "ASCII" and "EBCDIC" for "indexes", "digits",
 * and "Control-Characters".
 *
 * Note that all "index" values must be "lowercase letters", while
 * all "digits" must be "digits".  Control characters can be made
 * from any legal characters.
 */
#define A2I(X) ((X) - 'a')
#define I2A(X) ((char)(X) + 'a')
#define D2I(X) ((X) - '0')
#define I2D(X) ((X) + '0')
#define KTRL(X) ((X)&0x1F)
#define ESCAPE '\033'
