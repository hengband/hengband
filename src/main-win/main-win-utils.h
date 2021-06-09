#pragma once
/*!
 * @file main-win-utils.h
 * @brief Windows版固有実装(ユーティリティー)ヘッダ
 */

#include "term/z-virt.h"

#include <windows.h>

/*!
 * @brief マルチバイト文字列(CP932)をワイド文字列へ変換するクラス
 */
class to_wchar {
public:
    to_wchar(const char *src)
        : buf(NULL)
        , buf_size(0)
    {
        if (!src)
            return;

        int size = ::MultiByteToWideChar(932, 0, src, -1, buf, 0);
        if (size > 0) {
            buf_size = size + 1;
            C_MAKE(buf, buf_size, WCHAR);
            if (::MultiByteToWideChar(932, 0, src, -1, buf, buf_size) == 0) {
                // fail
                kill();
            }
        }
    }

    virtual ~to_wchar()
    {
        kill();
    }

    to_wchar(const to_wchar &) = delete;
    to_wchar &operator=(const to_wchar &) = delete;

    WCHAR *wc_str()
    {
        return buf;
    }

protected:
    WCHAR *buf;
    uint buf_size;

    void kill()
    {
        if (buf) {
            C_KILL(buf, buf_size, WCHAR);
            buf = NULL;
        }
    }
};


/*!
 * @brief ワイド文字列をマルチバイト文字列(CP932)へ変換するクラス
 */
class to_multibyte {
public:
    to_multibyte(const WCHAR *src)
        : buf(NULL)
        , buf_size(0)
    {
        if (!src)
            return;

        int size = ::WideCharToMultiByte(932, 0, src, -1, buf, 0, NULL, NULL);
        if (size > 0) {
            buf_size = size + 1;
            C_MAKE(buf, buf_size, char);
            if (::WideCharToMultiByte(932, 0, src, -1, buf, buf_size, NULL, NULL) == 0) {
                // fail
                kill();
            }
        }
    }

    virtual ~to_multibyte()
    {
        kill();
    }

    to_multibyte(const to_multibyte &) = delete;
    char* &operator=(const char* &) = delete;

    char *c_str()
    {
        return buf;
    }

protected:
    char *buf;
    uint buf_size;

    void kill()
    {
        if (buf) {
            C_KILL(buf, buf_size, char);
            buf = NULL;
        }
    }
};

bool is_already_running(void);
void save_screen_as_html(HWND hWnd);
void open_dir_in_explorer(char* filename);
bool get_open_filename(OPENFILENAMEW *ofn, concptr dirname, char *filename, DWORD max_name_size);
