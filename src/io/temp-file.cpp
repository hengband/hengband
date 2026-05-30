#include "io/temp-file.h"
#include "system/angband-exceptions.h"
#include <cstdio>
#include <cstdlib>
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
        THROW_EXCEPTION(std::runtime_error, "Failed to get temporary directory path: " + ec.message());
    }

#ifdef _WIN32
    wchar_t temp_file_path[MAX_PATH]{};
    if (GetTempFileNameW(temp_dir.wstring().data(), L"tmp", 0, temp_file_path) == 0) {
        THROW_EXCEPTION(std::runtime_error, "Failed to generate temporary file name with GetTempFileNameA");
    }

    this->path = temp_file_path;
#else
    auto temp_file_path = (temp_dir / "tempfile_XXXXXX").string();
    const auto fd = mkstemp(&temp_file_path[0]);
    if (fd == -1) {
        THROW_EXCEPTION(std::runtime_error, "Failed to generate temporary file name with mkstemp");
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

/*!
 * @details 日本語版ならばシフトJIS or EUC-JPでエンコードされた文字列しか書き込まないはずなので、再び読み込む時は何もしない.
 */
std::vector<std::string> TempFile::read_all() const
{
    std::ifstream ifs(this->path);
    if (!ifs) {
        THROW_EXCEPTION(std::runtime_error, "Cannot open file for reading");
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ifs, line)) {
        lines.push_back(line);
    }

    return lines;
}

void TempFile::write_lines(const std::vector<std::string> &lines) const
{
    std::ofstream ofs(this->path, std::ios::app);
    if (!ofs) {
        THROW_EXCEPTION(std::runtime_error, "Cannot open file for writing");
    }

    for (const auto &line : lines) {
        ofs << line << '\n';
    }
}
