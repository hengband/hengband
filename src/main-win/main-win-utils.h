#pragma once
/*!
 * @file main-win-utils.h
 * @brief Windows版固有実装(ユーティリティー)ヘッダ
 */

#include <filesystem>
#include <string>
#include <string_view>
#include <tl/optional.hpp>
#include <vector>
#define NOMINMAX
#include <windows.h>

constexpr UINT SHIFT_JIS_CODE_PAGE = 932; //!< Shift_JIS (CP932) のコードページ

//! 内部の文字コードのコードページ (日本語版の内部の文字コードは UTF-8、英語版は CP932 として扱う)
#ifdef JP
constexpr UINT SYSTEM_CODE_PAGE = CP_UTF8;
#else
constexpr UINT SYSTEM_CODE_PAGE = SHIFT_JIS_CODE_PAGE;
#endif

/*!
 * @brief マルチバイト文字列(既定では内部の文字コード)をワイド文字列へ変換するクラス
 */
class to_wchar {
public:
    explicit to_wchar(std::string_view src, UINT code_page = SYSTEM_CODE_PAGE, DWORD flags = 0)
    {
        if (src.empty()) {
            return;
        }

        const auto size = ::MultiByteToWideChar(code_page, flags, src.data(), src.length(), NULL, 0);
        if (size > 0) {
            this->buf = std::vector<WCHAR>(size + 1);
            if (::MultiByteToWideChar(code_page, flags, src.data(), src.length(), this->buf->data(), this->buf->size()) == 0) {
                // fail
                this->buf = tl::nullopt;
            }
        }
    }

    virtual ~to_wchar() = default;

    to_wchar(const to_wchar &) = delete;
    to_wchar &operator=(const to_wchar &) = delete;

    WCHAR *wc_str()
    {
        return this->buf ? this->buf->data() : NULL;
    }

protected:
    tl::optional<std::vector<WCHAR>> buf;
};

/*!
 * @brief ワイド文字列をマルチバイト文字列(内部の文字コード)へ変換するクラス
 */
class to_multibyte {
public:
    to_multibyte(const WCHAR *src)
    {
        if (src == nullptr) {
            return;
        }

        const auto size = ::WideCharToMultiByte(SYSTEM_CODE_PAGE, 0, src, -1, NULL, 0, NULL, NULL);
        if (size > 0) {
            this->buf = std::vector<char>(size + 1);
            if (::WideCharToMultiByte(SYSTEM_CODE_PAGE, 0, src, -1, this->buf->data(), this->buf->size(), NULL, NULL) == 0) {
                // fail
                this->buf = tl::nullopt;
            }
        }
    }

    virtual ~to_multibyte() = default;

    to_multibyte(const to_multibyte &) = delete;
    char *&operator=(const char *&) = delete;

    char *c_str()
    {
        return this->buf ? this->buf->data() : NULL;
    }

protected:
    tl::optional<std::vector<char>> buf;
};

bool is_already_running();
void attach_console();
void save_screen_as_html(HWND hWnd);
void open_dir_in_explorer(const std::filesystem::path &path);
tl::optional<std::filesystem::path> get_open_filename(OPENFILENAMEW *ofn, const std::filesystem::path &path_dir, const std::filesystem::path &path_file, DWORD max_name_size);
