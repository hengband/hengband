#include "util/angband-files.h"
#include "locale/japanese.h"
#include "util/string-processor.h"
#include <sstream>
#include <string>

#ifdef SET_UID

#ifndef HAVE_USLEEP

/*
 * For those systems that don't have "usleep()" but need it.
 *
 * Fake "usleep()" function grabbed from the inl netrek server -cba
 */
int usleep(ulong usecs)
{
    struct timeval timer;

    int nfds = 0;

    fd_set *no_fds = nullptr;
    if (usecs > 4000000L) {
        core(_("不当な usleep() 呼び出し", "Illegal usleep() call"));
    }

    timer.tv_sec = (usecs / 1000000L);
    timer.tv_usec = (usecs % 1000000L);
    if (select(nfds, no_fds, no_fds, no_fds, &timer) < 0) {
        if (errno != EINTR) {
            return -1;
        }
    }

    return 0;
}
#endif

/*
 * Hack -- External functions
 */
#ifdef SET_UID
struct passwd *getpwuid(uid_t uid);
struct passwd *getpwnam(concptr name);
#endif

/*
 * Find a default user name from the system.
 */
void user_name(char *buf, int id)
{
    struct passwd *pw;
    if ((pw = getpwuid(id))) {
        (void)strcpy(buf, pw->pw_name);
        buf[16] = '\0';

#ifdef JP
        if (!iskanji(buf[0]))
#endif
            if (islower(buf[0])) {
                buf[0] = toupper(buf[0]);
            }

        return;
    }

    strcpy(buf, "PLAYER");
}

#endif /* SET_UID */

std::filesystem::path path_parse(std::string_view file)
#ifdef SET_UID
{
    /*
     * Extract a "parsed" path from an initial filename
     * Normally, we simply copy the filename into the buffer
     * But leading tilde symbols must be handled in a special way
     * Replace "~user/" by the home directory of the user named "user"
     * Replace "~/" by the home directory of the current user
     */
    if (file.empty() || (file[0] != '~')) {
        return file;
    }

    auto u = file.data() + 1;
    auto s = angband_strstr(u, PATH_SEP);
    constexpr auto user_size = 128;
    char user[user_size]{};
    if ((s != nullptr) && (s >= u + user_size)) {
        throw std::runtime_error("User name is too long!");
    }

    if (s != nullptr) {
        int i;
        for (i = 0; u < s; ++i) {
            user[i] = *u++;
        }

        u = user;
    }

    if (u[0] == '\0') {
        u = getlogin();
    }

    struct passwd *pw;
    if (u != nullptr) {
        pw = getpwnam(u);
    } else {
        pw = getpwuid(getuid());
    }

    if (pw == nullptr) {
        throw std::runtime_error("Failed to get User ID!");
    }

    if (s == nullptr) {
        return pw->pw_dir;
    }

    std::stringstream ss;
    ss << pw->pw_dir << s;
    return ss.str();
}
#else
{
    return file;
}
#endif /* SET_UID */

#ifndef HAVE_MKSTEMP

/*
 * Hack -- acquire a "temporary" file name if possible
 *
 * This filename is always in "system-specific" form.
 */
static errr path_temp(char *buf, int max)
{
    concptr s = tmpnam(nullptr);
    if (!s) {
        return -1;
    }

#if !defined(WIN32) || (defined(_MSC_VER) && (_MSC_VER >= 1900))
    (void)strnfmt(buf, max, "%s", s);
#else
    (void)strnfmt(buf, max, ".%s", s);
#endif

    return 0;
}
#endif

/*!
 * @brief ファイル入出力のためのパス生成する。/ Create a new path by appending a file (or directory) to a path.
 * @param buf ファイルのフルを返すバッファ
 * @param max bufのサイズ
 * @param path ファイルパス
 * @param file ファイル名
 * @return エラーコード(ただし常に0を返す)
 *
 * This requires no special processing on simple machines, except
 * for verifying the size of the filename, but note the ability to
 * bypass the given "path" with certain special file-names.
 *
 * Note that the "file" may actually be a "sub-path", including
 * a path and a file.
 *
 * Note that this function yields a path which must be "parsed"
 * using the "parse" function above.
 */
errr path_build(char *buf, int max, concptr path, concptr file)
{
    if (file[0] == '~') {
        (void)strnfmt(buf, max, "%s", file);
    } else if (prefix(file, PATH_SEP) && !streq(PATH_SEP, "")) {
        (void)strnfmt(buf, max, "%s", file);
    } else if (!path[0]) {
        (void)strnfmt(buf, max, "%s", file);
    } else {
        (void)strnfmt(buf, max, "%s%s%s", path, PATH_SEP, file);
    }

    return 0;
}

static std::string make_file_mode(const FileOpenMode mode, const bool is_binary)
{
    std::stringstream ss;
    switch (mode) {
    case FileOpenMode::READ:
        ss << 'r';
        break;
    case FileOpenMode::WRITE:
        ss << 'w';
        break;
    case FileOpenMode::APPEND:
        ss << 'a';
        break;
    default:
        throw std::logic_error("Invalid file mode is specified!");
    }

    if (is_binary) {
        ss << 'b';
    }

    return ss.str();
}

/*!
 * @brief OSごとの差異を吸収してファイルを開く
 * @param file ファイルの相対パスまたは絶対パス
 * @param mode ファイルを開くモード
 * @param is_binary バイナリモードか否か (無指定の場合false：テキストモード)
 * @return ファイルポインタ
 */
FILE *angband_fopen(const std::filesystem::path &file, const FileOpenMode mode, const bool is_binary)
{
    const auto &path = path_parse(file.string());
    const auto &open_mode = make_file_mode(mode, is_binary);
    return fopen(path.string().data(), open_mode.data());
}

/*
 * Hack -- replacement for "fclose()"
 */
errr angband_fclose(FILE *fff)
{
    if (!fff) {
        return -1;
    }
    if (fclose(fff) == EOF) {
        return 1;
    }
    return 0;
}

#ifdef HAVE_MKSTEMP
FILE *angband_fopen_temp(char *buf, int max)
{
    strncpy(buf, "/tmp/anXXXXXX", max);
    int fd = mkstemp(buf);
    if (fd < 0) {
        return nullptr;
    }

    return fdopen(fd, "w");
}
#else /* HAVE_MKSTEMP */
FILE *angband_fopen_temp(char *buf, int max)
{
    if (path_temp(buf, max)) {
        return nullptr;
    }
    return angband_fopen(buf, FileOpenMode::WRITE);
}
#endif /* HAVE_MKSTEMP */

/*
 * Hack -- replacement for "fgets()"
 *
 * Read a string, without a newline, to a file
 *
 * Process tabs, replace internal non-printables with '?'
 */
errr angband_fgets(FILE *fff, char *buf, ulong n)
{
    ulong i = 0;
    char *s;

    if (n <= 1) {
        return 1;
    }
    // Reserve for null termination
    --n;

    std::vector<char> file_read__tmp(FILE_READ_BUFF_SIZE);
    if (fgets(file_read__tmp.data(), file_read__tmp.size(), fff)) {
#ifdef JP
        guess_convert_to_system_encoding(file_read__tmp.data(), FILE_READ_BUFF_SIZE);
#endif
        for (s = file_read__tmp.data(); *s; s++) {
            if (*s == '\n') {
                buf[i] = '\0';
                return 0;
            } else if (*s == '\t') {
                if (i + 8 >= n) {
                    break;
                }

                buf[i++] = ' ';
                while (0 != (i % 8)) {
                    buf[i++] = ' ';
                }
            }
#ifdef JP
            else if (iskanji(*s)) {
                if (i + 1 >= n) {
                    break;
                }
                if (!s[1]) {
                    break;
                }
                buf[i++] = *s++;
                buf[i++] = *s;
            } else if (iskana(*s)) {
                /* 半角かなに対応 */
                buf[i++] = *s;
                if (i >= n) {
                    break;
                }
            }
#endif
            else if (isprint((unsigned char)*s)) {
                buf[i++] = *s;
                if (i >= n) {
                    break;
                }
            } else {
                buf[i++] = '?';
                if (i >= n) {
                    break;
                }
            }
        }

        buf[i] = '\0';
        return 0;
    }

    buf[0] = '\0';
    return 1;
}

/*
 * Hack -- replacement for "fputs()"
 * Dump a string, plus a newline, to a file
 * Process internal weirdness?
 */
errr angband_fputs(FILE *fff, concptr buf, ulong n)
{
    n = n ? n : 0;
    (void)fprintf(fff, "%s\n", buf);
    return 0;
}

/*
 * Several systems have no "O_BINARY" flag
 */
#ifndef O_BINARY
#define O_BINARY 0
#endif /* O_BINARY */

/*!
 * @brief OSごとの差異を吸収してファイルを削除する
 * @param file ファイルの相対パスまたは絶対パス
 */
void fd_kill(std::string_view file)
{
    const auto &path = path_parse(file);
    if (!std::filesystem::exists(path)) {
        return;
    }

    std::filesystem::remove(path);
}

/*!
 * @brief OSごとの差異を吸収してファイルを移動する
 * @param from 移動元のファイルの相対パスまたは絶対パス
 * @param to 移動先のファイルの相対パスまたは絶対パス
 */
void fd_move(std::string_view from, std::string_view to)
{
    const auto &path_from = path_parse(from);
    if (!std::filesystem::exists(path_from)) {
        return;
    }

    const auto &path_to = path_parse(to);
    const auto directory = std::filesystem::path(path_to).remove_filename();
    if (!std::filesystem::exists(directory)) {
        std::filesystem::create_directory(directory);
    }

    std::filesystem::rename(path_from, path_to);
}

/*!
 * @brief OSごとの差異を吸収してファイルを作成する
 * @param file 作成先ファイルの相対パスまたは絶対パス
 * @param can_write_group グループに書き込みを許可するか否か
 */
int fd_make(std::string_view file, bool can_write_group)
{
    const auto permission = can_write_group ? 0644 : 0664;
    const auto &path = path_parse(file);
    return open(path.string().data(), O_CREAT | O_EXCL | O_WRONLY | O_BINARY, permission);
}

/*
 * @brief OSごとの差異を吸収してファイルを開く
 * @param file ファイルの相対パスまたは絶対パス
 * @param mode ファイルのオープンモード (読み書き、Append/Trunc等)
 */
int fd_open(std::string_view file, int mode)
{
    const auto &path = path_parse(file);
    return open(path.string().data(), mode | O_BINARY, 0);
}

/*
 * Hack -- attempt to lock a file descriptor
 *
 * Legal lock types -- F_UNLCK, F_RDLCK, F_WRLCK
 */
errr fd_lock(int fd, int what)
{
    what = what ? what : 0;
    if (fd < 0) {
        return -1;
    }

#if defined(SET_UID) && defined(LOCK_UN) && defined(LOCK_EX)
    if (what == F_UNLCK) {
        (void)flock(fd, LOCK_UN);
    } else {
        if (flock(fd, LOCK_EX) != 0) {
            return 1;
        }
    }
#endif

    return 0;
}

/*
 * Hack -- attempt to seek on a file descriptor
 */
errr fd_seek(int fd, ulong n)
{
    if (fd < 0) {
        return -1;
    }

    ulong p = lseek(fd, n, SEEK_SET);
    if (p != n) {
        return 1;
    }

    return 0;
}

/*
 * Hack -- attempt to read data from a file descriptor
 */
errr fd_read(int fd, char *buf, ulong n)
{
    if (fd < 0) {
        return -1;
    }
#ifndef SET_UID
    while (n >= 16384) {
        if (read(fd, buf, 16384) != 16384) {
            return 1;
        }

        buf += 16384;
        n -= 16384;
    }
#endif

    if (read(fd, buf, n) != (int)n) {
        return 1;
    }

    return 0;
}

/*
 * Hack -- Attempt to write data to a file descriptor
 */
errr fd_write(int fd, concptr buf, ulong n)
{
    if (fd < 0) {
        return -1;
    }

#ifndef SET_UID
    while (n >= 16384) {
        if (write(fd, buf, 16384) != 16384) {
            return 1;
        }

        buf += 16384;
        n -= 16384;
    }
#endif

    if (write(fd, buf, n) != (int)n) {
        return 1;
    }

    return 0;
}

/*
 * Hack -- attempt to close a file descriptor
 */
errr fd_close(int fd)
{
    if (fd < 0) {
        return -1;
    }

    (void)close(fd);
    return 0;
}
