/*!
 * @file main-win-file-utils.cpp
 * @brief Windows版固有実装(ファイル関連処理)
 */

#include "main-win/main-win-file-utils.h"
#include "main-win/main-win-define.h"
#include "main-win/main-win-windows.h"
#include "util/angband-files.h"

/*
 * Check for existance of a file
 */
bool check_file(concptr s)
{
    char path[MAIN_WIN_MAX_PATH];
    strcpy(path, s);
    DWORD attrib = GetFileAttributesA(path);
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

    DWORD attrib = GetFileAttributesA(path);
    if (attrib == INVALID_FILE_NAME)
        return FALSE;
    if (!(attrib & FILE_ATTRIBUTE_DIRECTORY))
        return FALSE;

    return TRUE;
}

/*!
 * @brief 候補リストを順に確認し、存在するファイルのパスを返す。
 * @param dir ディレクトリ
 * @param files ファイル名のリスト
 * @return ファイルのパスを返す。候補リストのファイルすべて存在しない場合は空文字列を返す。
 */
std::string find_any_file(concptr dir, std::initializer_list<concptr> files)
{
    char path[MAIN_WIN_MAX_PATH];

    for (concptr filename : files) {
        path_build(path, MAIN_WIN_MAX_PATH, dir, filename);
        if (check_file(path)) {
            return std::string(path);
        }
    }

    return std::string();
}
