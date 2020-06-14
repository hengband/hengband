#include "locale/vowel-checker.h"

/*
 * Check a char for "vowel-hood"
 */
bool is_a_vowel(int ch)
{
    switch (ch) {
    case 'a':
    case 'e':
    case 'i':
    case 'o':
    case 'u':
    case 'A':
    case 'E':
    case 'I':
    case 'O':
    case 'U':
        return TRUE;
    }

    return FALSE;
}
