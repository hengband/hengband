#include "util/angband-files.h"
#include "locale/japanese.h"
#include "system/angband-exceptions.h"
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

std::filesystem::path path_parse(const std::filesystem::path &path)
#ifdef SET_UID
{
    /*
     * Extract a "parsed" path from an initial filename
     * Normally, we simply copy the filename into the buffer
     * But leading tilde symbols must be handled in a special way
     * Replace "~user/" by the home directory of the user named "user"
     * Replace "~/" by the home directory of the current user
     */
    const auto &file = path.string();
    if (file.empty() || (file[0] != '~')) {
        return file;
    }

    auto u = file.data() + 1;
    auto s = angband_strstr(u, PATH_SEP);
    constexpr auto user_size = 128;
    char user[user_size]{};
    if ((s != nullptr) && (s >= u + user_size)) {
        THROW_EXCEPTION(std::runtime_error, "User name is too long!");
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
        THROW_EXCEPTION(std::runtime_error, "Failed to get User ID!");
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
    return path;
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
    auto s = tmpnam(nullptr);
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
 * @brief OSの差異を吸収しつつ、絶対パスを生成する.
 * @param path file 引数があるディレクトリ
 * @param file ファイル名またはディレクトリ名
 */
std::filesystem::path path_build(const std::filesystem::path &path, std::string_view file)
{
    if ((file[0] == '~') || (prefix(file, PATH_SEP)) || path.empty()) {
        return file;
    }

    auto parsed_path = path_parse(path);
    const auto &path_ret = parsed_path.append(file);
    constexpr auto max_path_length = 1024;
    const auto path_str = path_ret.string();
    if (path_str.length() > max_path_length) {
        THROW_EXCEPTION(std::runtime_error, format("Path is too long! %s", path_str.data()));
    }

    return path_ret;
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
        THROW_EXCEPTION(std::logic_error, "Invalid file mode is specified!");
    }

    if (is_binary) {
        ss << 'b';
    }

    return ss.str();
}

/*!
 * @brief OSごとの差異を吸収してファイルを開く
 * @param path ファイルの相対パスまたは絶対パス
 * @param mode ファイルを開くモード
 * @param is_binary バイナリモードか否か (無指定の場合false：テキストモード)
 * @return ファイルポインタ
 */
FILE *angband_fopen(const std::filesystem::path &path, const FileOpenMode mode, const bool is_binary)
{
    const auto &parsed_path = path_parse(path);
    const auto &open_mode = make_file_mode(mode, is_binary);
    return fopen(parsed_path.string().data(), open_mode.data());
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

    std::vector<char> file_read_tmp(FILE_READ_BUFF_SIZE);
    if (fgets(file_read_tmp.data(), file_read_tmp.size(), fff)) {
#ifdef JP
        guess_convert_to_system_encoding(file_read_tmp.data(), FILE_READ_BUFF_SIZE);
#endif
        for (s = file_read_tmp.data(); *s; s++) {
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
void fd_kill(const std::filesystem::path &path)
{
    const auto &parsed_path = path_parse(path);

    std::error_code ec;
    std::filesystem::remove(parsed_path, ec);
}

/*!
 * @brief OSごとの差異を吸収してファイルを移動する
 * @param path_from 移動元のファイルの相対パスまたは絶対パス
 * @param path_to 移動先のファイルの相対パスまたは絶対パス
 */
void fd_move(const std::filesystem::path &path_from, const std::filesystem::path &path_to)
{
    const auto &abs_path_from = path_parse(path_from);
    const auto &abs_path_to = path_parse(path_to);

    std::error_code ec;
    std::filesystem::rename(abs_path_from, abs_path_to, ec);
}

/*!
 * @brief OSごとの差異を吸収してファイルを作成する
 * @param path 作成先ファイルの相対パスまたは絶対パス
 * @param can_write_group グループに書き込みを許可するか否か
 */
int fd_make(const std::filesystem::path &path, bool can_write_group)
{
    const auto permission = can_write_group ? 0644 : 0664;
    const auto &parsed_path = path_parse(path);
    return open(parsed_path.string().data(), O_CREAT | O_EXCL | O_WRONLY | O_BINARY, permission);
}

/*
 * @brief OSごとの差異を吸収してファイルを開く
 * @param path ファイルの相対パスまたは絶対パス
 * @param mode ファイルのオープンモード (読み書き、Append/Trunc等)
 */
int fd_open(const std::filesystem::path &path, int mode)
{
    const auto &path_abs = path_parse(path);
    return open(path_abs.string().data(), mode | O_BINARY, 0);
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
