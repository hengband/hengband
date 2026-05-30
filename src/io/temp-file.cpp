#include "io/temp-file.h"
#include "system/angband-exceptions.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

TempFile::TempFile()
{
#ifdef _WIN32
    char win_temp[MAX_PATH]{};
    GetTempPathA(MAX_PATH, win_temp);
    std::filesystem::path temp_dir = win_temp;

    auto temp_str = (temp_dir / "tempfile_XXXXXX").string();
    const auto *result = _mktemp(&temp_str[0]);
    if (result == nullptr) {
        THROW_EXCEPTION(std::runtime_error, "Failed to generate temporary file name with _mktemp");
    }

    this->path = std::filesystem::path(temp_str);
#else
    const auto *tmp_dir_env = std::getenv("TMPDIR");
    std::filesystem::path temp_dir = tmp_dir_env ? tmp_dir_env : "/tmp";

    auto temp_str = (temp_dir / "tempfile_XXXXXX").string();
    const auto fd = mkstemp(&temp_str[0]);
    if (fd != -1) {
        close(fd);
    }

    this->path = std::filesystem::path(temp_str);
#endif

    this->create_empty_file();
    this->set_permissions();
}

TempFile::~TempFile()
{
    if (!this->path.empty()) {
        std::filesystem::remove(this->path);
    }
}

const std::filesystem::path &TempFile::get_path() const
{
    return this->path;
}

void TempFile::write_line(std::string_view line) const
{
    std::ofstream ofs(this->path, std::ios::app);
    if (!ofs) {
        THROW_EXCEPTION(std::runtime_error, "Cannot open file for writing");
    }

    ofs << line << std::endl;
}

/*!
 * @details 日本語版ならばシフトJIS or EUC-JPでエンコードされた文字列しか書き込まないはずなので、再び読み込む時は何もしない.
 */
tl::optional<std::string> TempFile::read_line()
{
    std::ifstream ifs(this->path);
    if (!ifs) {
        THROW_EXCEPTION(std::runtime_error, "Cannot open file for reading");
    }

    std::string line;
    size_t line_num = 0;
    while (std::getline(ifs, line)) {
        if (line_num == this->current_line_index) {
            this->current_line_index++;
            return line;
        }

        line_num++;
    }

    this->current_line_index = line_num;
    return tl::nullopt;
}

void TempFile::write_lines(const std::vector<std::string> &lines) const
{
    std::ofstream ofs(this->path, std::ios::app);
    if (!ofs) {
        THROW_EXCEPTION(std::runtime_error, "Cannot open file for writing");
    }

    for (const auto &line : lines) {
        ofs << line << std::endl;
    }
}

/*!
 * @details 日本語版ならばシフトJIS or EUC-JPでエンコードされた文字列しか書き込まないはずなので、再び読み込む時は何もしない.
 */
std::vector<std::string> TempFile::read_all()
{
    std::vector<std::string> lines;
    std::ifstream ifs(this->path);
    if (!ifs) {
        THROW_EXCEPTION(std::runtime_error, "Cannot open file for reading");
    }

    std::string line;
    while (std::getline(ifs, line)) {
        lines.push_back(line);
    }

    this->current_line_index = lines.size(); // 全部読んだら終端に合わせる
    return lines;
}

void TempFile::create_empty_file() const
{
    std::ofstream ofs(this->path, std::ios::out);
    if (!ofs) {
        THROW_EXCEPTION(std::runtime_error, "Failed to create temporary file");
    }
}

void TempFile::set_permissions() const
{
#ifdef _WIN32
#else
    chmod(this->path.string().data(), 0666);
#endif
}
