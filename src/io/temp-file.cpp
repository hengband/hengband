#include "io/temp-file.h"
#include "locale/language-switcher.h"
#include "system/angband-exceptions.h"
#include <cstdio>
#include <cstdlib>
#include <fmt/format.h>
#include <fstream>
#include <system_error>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

TempFile::TempFile()
{
    std::error_code ec;
    const auto temp_dir = std::filesystem::temp_directory_path(ec);
    if (ec) {
        constexpr auto fmt = _("一時ファイル格納先フォルダの取得に失敗しました: {}", "Failed to get temporary directory path: {}");
        this->error_message = fmt::format(fmt, ec.message());
        return;
    }

#ifdef _WIN32
    wchar_t temp_file_path[MAX_PATH]{};
    if (GetTempFileNameW(temp_dir.wstring().data(), L"tmp", 0, temp_file_path) == 0) {
        this->error_message = _("一時ファイルの作成に失敗しました", "Failed to create temporary file name");
        return;
    }

    this->path = temp_file_path;
#else
    auto temp_file_path = (temp_dir / "tempfile_XXXXXX").string();
    const auto fd = mkstemp(&temp_file_path[0]);
    if (fd == -1) {
        constexpr auto fmt = _("一時ファイルの作成に失敗しました: {}", "Failed to create temporary file name: {}");
        this->error_message = fmt::format(fmt, temp_file_path);
        return;
    }

    fchmod(fd, S_IRUSR | S_IWUSR);
    close(fd);
    this->path = temp_file_path;
#endif
}

TempFile::TempFile(const std::filesystem::path &path)
    : path(path)
{
}

TempFile::~TempFile()
{
    if (!this->path.empty()) {
        std::error_code ec;
        std::filesystem::remove(this->path, ec);
    }
}

const std::filesystem::path &TempFile::get_path() const
{
    return this->path;
}

const tl::optional<std::string> &TempFile::get_error_message() const
{
    return this->error_message;
}

/*!
 * @details 日本語版ならばシフトJIS or EUC-JPでエンコードされた文字列しか書き込まないはずなので、再び読み込む時は何もしない.
 */
std::vector<std::string> TempFile::read_all()
{
    constexpr auto fmt = _("一時ファイルの読み込みに失敗しました: {}", "Failed to read contents of the temporary file: {}");
    std::ifstream ifs(this->path);
    if (!ifs) {
        this->error_message = fmt::format(fmt, this->path.string());
        return {};
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ifs, line)) {
        lines.push_back(line);
    }

    if (ifs.bad() || (ifs.fail() && !ifs.eof())) {
        this->error_message = fmt::format(fmt, this->path.string());
        return {};
    }

    return lines;
}

void TempFile::write_lines(const std::vector<std::string> &lines)
{
    constexpr auto fmt = _("一時ファイルの書き込みに失敗しました: {}", "Failed to write contents of the temporary file: {}");
    std::ofstream ofs(this->path, std::ios::app);
    if (!ofs) {
        this->error_message = fmt::format(fmt, this->path.string());
        return;
    }

    for (const auto &line : lines) {
        ofs << line << '\n';
    }

    if (ofs.fail()) {
        this->error_message = fmt::format(fmt, this->path.string());
        return;
    }
}
