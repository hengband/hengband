/*!
 * @file main-win-file-utils.cpp
 * @brief Windows版固有実装(ファイル関連処理)
 */

#include "main-win/main-win-file-utils.h"
#include "main-win/main-win-define.h"
#include "main-win/main-win-windows.h"

/*
 * Check for existance of a file
 */
bool check_file(concptr s)
{
    char path[MAIN_WIN_MAX_PATH];
    strcpy(path, s);
    DWORD attrib = GetFileAttributes(path);
    if (attrib == INVALID_FILE_NAME)
        return FALSE;
    if (attrib & FILE_ATTRIBUTE_DIRECTORY)
        return FALSE;

    return TRUE;
}

/*
 * Check for existance of a directory
 */
bool check_dir(concptr s)
{
    char path[MAIN_WIN_MAX_PATH];
    strcpy(path, s);
    int i = strlen(path);
    if (i && (path[i - 1] == '\\'))
        path[--i] = '\0';

    DWORD attrib = GetFileAttributes(path);
    if (attrib == INVALID_FILE_NAME)
        return FALSE;
    if (!(attrib & FILE_ATTRIBUTE_DIRECTORY))
        return FALSE;

    return TRUE;
}
