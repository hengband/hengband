/*!
 * @file main-win-file-utils.cpp
 * @brief Windows版固有実装(ファイル関連処理)
 */

#include "main-win/main-win-file-utils.h"
#include "main-win/main-win-define.h"
#include "main-win/main-win-utils.h"
#include "util/angband-files.h"

#include <windows.h>

/*
 * Check for existance of a file
 * @param s path of file
 * @retval true 指定ファイルが存在する（かつディレクトリではない）
 * @retval false 指定ファイルが存在しない、またはディレクトリである
 */
bool check_file(const std::filesystem::path &s)
{
    const auto &file = s.string();
    DWORD attrib = GetFileAttributesW(to_wchar(file.data()).wc_str());
    if (attrib == INVALID_FILE_NAME) {
        return false;
    }
    if (attrib & FILE_ATTRIBUTE_DIRECTORY) {
        return false;
    }

    return true;
}

/*
 * Check for existance of a directory
 * @param s path of directory
 * @retval true 指定ディレクトリが存在する
 * @retval false 指定ディレクトリが存在しない、またはディレクトリではない
 */
bool check_dir(const std::filesystem::path &s)
{
    const auto &dir = s.string();
    char path[MAIN_WIN_MAX_PATH];
    strcpy(path, dir.data());
    int i = strlen(path);
    if (i && (path[i - 1] == '\\')) {
        path[--i] = '\0';
    }

    DWORD attrib = GetFileAttributesW(to_wchar(path).wc_str());
    if (attrib == INVALID_FILE_NAME) {
        return false;
    }
    if (!(attrib & FILE_ATTRIBUTE_DIRECTORY)) {
        return false;
    }

    return true;
}

/*!
 * @brief 候補リストを順に確認し、存在するファイルのパスを返す。
 * @param dir ディレクトリ
 * @param files ファイル名のリスト
 * @return ファイルのパスを返す。候補リストのファイルすべて存在しない場合は空文字列を返す。
 */
std::string find_any_file(const std::filesystem::path &dir, std::initializer_list<concptr> files)
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
