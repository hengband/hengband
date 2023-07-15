#include "locale/english.h"
#include "system/angband.h"
#include "util/string-processor.h"

#ifndef JP

/*!
 * @brief 英単語、句、説を複数形を変換する / Pluralize a monster name
 * @param Name 変換したい文字列の参照ポインタ
 */
void plural_aux(char *Name)
{
    int NameLen = strlen(Name);

    if (angband_strstr(Name, "Disembodied hand")) {
        strcpy(Name, "Disembodied hands that strangled people");
    } else if (angband_strstr(Name, "Colour out of space")) {
        strcpy(Name, "Colours out of space");
    } else if (angband_strstr(Name, "stairway to hell")) {
        strcpy(Name, "stairways to hell");
    } else if (angband_strstr(Name, "Dweller on the threshold")) {
        strcpy(Name, "Dwellers on the threshold");
    } else if (angband_strstr(Name, " of ")) {
        concptr aider = angband_strstr(Name, " of ");
        char dummy[80];
        int i = 0;
        concptr ctr = Name;

        while (ctr < aider) {
            dummy[i] = *ctr;
            ctr++;
            i++;
        }

        if (dummy[i - 1] == 's') {
            strcpy(&(dummy[i]), "es");
            i++;
        } else {
            strcpy(&(dummy[i]), "s");
        }

        strcpy(&(dummy[i + 1]), aider);
        strcpy(Name, dummy);
    } else if (angband_strstr(Name, "coins")) {
        char dummy[80];
        strcpy(dummy, "piles of ");
        strcat(dummy, Name);
        strcpy(Name, dummy);
    } else if (angband_strstr(Name, "Manes")) {
        return;
    } else if (suffix(Name, "y") && !(NameLen >= 2 && is_a_vowel(Name[NameLen - 2]))) {
        strcpy(&(Name[NameLen - 1]), "ies");
    } else if (suffix(Name, "ouse")) {
        strcpy(&(Name[NameLen - 4]), "ice");
    } else if (suffix(Name, "us")) {
        strcpy(&(Name[NameLen - 2]), "i");
    } else if (suffix(Name, "kelman")) {
        strcpy(&(Name[NameLen - 6]), "kelmen");
    } else if (suffix(Name, "wordsman")) {
        strcpy(&(Name[NameLen - 8]), "wordsmen");
    } else if (suffix(Name, "oodsman")) {
        strcpy(&(Name[NameLen - 7]), "oodsmen");
    } else if (suffix(Name, "eastman")) {
        strcpy(&(Name[NameLen - 7]), "eastmen");
    } else if (suffix(Name, "izardman")) {
        strcpy(&(Name[NameLen - 8]), "izardmen");
    } else if (suffix(Name, "geist")) {
        strcpy(&(Name[NameLen - 5]), "geister");
    } else if (suffix(Name, "ex")) {
        strcpy(&(Name[NameLen - 2]), "ices");
    } else if (suffix(Name, "lf")) {
        strcpy(&(Name[NameLen - 2]), "lves");
    } else if (suffix(Name, "ch") ||
               suffix(Name, "sh") ||
               suffix(Name, "nx") ||
               suffix(Name, "s") ||
               suffix(Name, "o")) {
        strcpy(&(Name[NameLen]), "es");
    } else {
        strcpy(&(Name[NameLen]), "s");
    }
}

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
        return true;
    }

    return false;
}

#endif // !JP
