#include "util/angband-files.h"
#include "util/string-processor.h"
#ifdef JP
#include "locale/japanese.h"
#endif

#ifdef SET_UID

#ifndef HAVE_USLEEP

/*
 * For those systems that don't have "usleep()" but need it.
 *
 * Fake "usleep()" function grabbed from the inl netrek server -cba
 */
int usleep(huge usecs)
{
    struct timeval timer;

    int nfds = 0;

    fd_set *no_fds = NULL;
    if (usecs > 4000000L)
        core(_("不当な usleep() 呼び出し", "Illegal usleep() call"));

    timer.tv_sec = (usecs / 1000000L);
    timer.tv_usec = (usecs % 1000000L);
    if (select(nfds, no_fds, no_fds, no_fds, &timer) < 0) {
        if (errno != EINTR)
            return -1;
    }

    return 0;
}
#endif

/*
 * Hack -- External functions
 */
#ifdef SET_UID
extern struct passwd *getpwuid(uid_t uid);
extern struct passwd *getpwnam(concptr name);
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
            if (islower(buf[0]))
                buf[0] = toupper(buf[0]);

        return;
    }

    strcpy(buf, "PLAYER");
}

#endif /* SET_UID */

#ifdef SET_UID
/*
 * Extract a "parsed" path from an initial filename
 * Normally, we simply copy the filename into the buffer
 * But leading tilde symbols must be handled in a special way
 * Replace "~user/" by the home directory of the user named "user"
 * Replace "~/" by the home directory of the current user
 */
errr path_parse(char *buf, int max, concptr file)
{
    buf[0] = '\0';
    if (!file)
        return -1;

    if (file[0] != '~') {
        (void)strnfmt(buf, max, "%s", file);
        return 0;
    }

    concptr u = file + 1;
    concptr s = angband_strstr(u, PATH_SEP);
    char user[128];
    if (s && (s >= u + sizeof(user)))
        return 1;

    if (s) {
        int i;
        for (i = 0; u < s; ++i)
            user[i] = *u++;
        user[i] = '\0';
        u = user;
    }

    if (u[0] == '\0')
        u = getlogin();

    struct passwd *pw;
    if (u)
        pw = getpwnam(u);
    else
        pw = getpwuid(getuid());

    if (!pw)
        return 1;

    if (s)
        strnfmt(buf, max, "%s%s", pw->pw_dir, s);
    else
        strnfmt(buf, max, "%s", pw->pw_dir);

    return 0;
}
#else /* SET_UID */
/*
 * Extract a "parsed" path from an initial filename
 *
 * This requires no special processing on simple machines,
 * except for verifying the size of the filename.
 */
errr path_parse(char *buf, int max, concptr file)
{
    (void)strnfmt(buf, max, "%s", file);
    return 0;
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
    concptr s = tmpnam(NULL);
    if (!s)
        return -1;

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

/*
 * Hack -- replacement for "fopen()"
 */
FILE *angband_fopen(concptr file, concptr mode)
{
    char buf[1024];
    if (path_parse(buf, 1024, file))
        return (NULL);

    return (fopen(buf, mode));
}

/*
 * Hack -- replacement for "fclose()"
 */
errr angband_fclose(FILE *fff)
{
    if (!fff)
        return -1;
    if (fclose(fff) == EOF)
        return 1;
    return 0;
}

#ifdef HAVE_MKSTEMP
FILE *angband_fopen_temp(char *buf, int max)
{
    strncpy(buf, "/tmp/anXXXXXX", max);
    int fd = mkstemp(buf);
    if (fd < 0)
        return (NULL);

    return (fdopen(fd, "w"));
}
#else /* HAVE_MKSTEMP */
FILE *angband_fopen_temp(char *buf, int max)
{
    if (path_temp(buf, max))
        return (NULL);
    return (angband_fopen(buf, "w"));
}
#endif /* HAVE_MKSTEMP */

/*
 * Hack -- replacement for "fgets()"
 *
 * Read a string, without a newline, to a file
 *
 * Process tabs, strip internal non-printables
 */
errr angband_fgets(FILE *fff, char *buf, huge n)
{
    huge i = 0;
    char *s;
    char tmp[1024];

    if (fgets(tmp, 1024, fff)) {
#ifdef JP
        guess_convert_to_system_encoding(tmp, sizeof(tmp));
#endif
        for (s = tmp; *s; s++) {
            if (*s == '\n') {
                buf[i] = '\0';
                return 0;
            } else if (*s == '\t') {
                if (i + 8 >= n)
                    break;

                buf[i++] = ' ';
                while (0 != (i % 8))
                    buf[i++] = ' ';
            }
#ifdef JP
            else if (iskanji(*s)) {
                if (!s[1])
                    break;
                buf[i++] = *s++;
                buf[i++] = *s;
            } else if (iskana(*s)) {
                /* 半角かなに対応 */
                buf[i++] = *s;
                if (i >= n)
                    break;
            }
#endif
            else if (isprint((unsigned char)*s)) {
                buf[i++] = *s;
                if (i >= n)
                    break;
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
errr angband_fputs(FILE *fff, concptr buf, huge n)
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

/*
 * Hack -- attempt to delete a file
 */
errr fd_kill(concptr file)
{
    char buf[1024];
    if (path_parse(buf, 1024, file))
        return -1;

    (void)remove(buf);
    return 0;
}

/*
 * Hack -- attempt to move a file
 */
errr fd_move(concptr file, concptr what)
{
    char buf[1024];
    char aux[1024];
    if (path_parse(buf, 1024, file))
        return -1;
    if (path_parse(aux, 1024, what))
        return -1;

    (void)rename(buf, aux);
    return 0;
}

/*
 * Hack -- attempt to copy a file
 */
errr fd_copy(concptr file, concptr what)
{
    char buf[1024];
    char aux[1024];
    int read_num;
    int src_fd, dst_fd;

    if (path_parse(buf, 1024, file))
        return -1;
    if (path_parse(aux, 1024, what))
        return -1;

    src_fd = fd_open(buf, O_RDONLY);
    if (src_fd < 0)
        return -1;

    dst_fd = fd_open(aux, O_WRONLY | O_TRUNC | O_CREAT);
    if (dst_fd < 0)
        return -1;

    while ((read_num = read(src_fd, buf, 1024)) > 0) {
        int write_num = 0;
        while (write_num < read_num) {
            int ret = write(dst_fd, buf + write_num, read_num - write_num);
            if (ret < 0) {
                fd_close(src_fd);
                fd_close(dst_fd);

                return ret;
            }

            write_num += ret;
        }
    }

    fd_close(src_fd);
    fd_close(dst_fd);
    return 0;
}

/*
 * Hack -- attempt to open a file descriptor (create file)
 * This function should fail if the file already exists
 * Note that we assume that the file should be "binary"
 */
int fd_make(concptr file, BIT_FLAGS mode)
{
    char buf[1024];
    if (path_parse(buf, 1024, file))
        return -1;

    return (open(buf, O_CREAT | O_EXCL | O_WRONLY | O_BINARY, mode));
}

/*
 * Hack -- attempt to open a file descriptor (existing file)
 *
 * Note that we assume that the file should be "binary"
 */
int fd_open(concptr file, int flags)
{
    char buf[1024];
    if (path_parse(buf, 1024, file))
        return -1;

    return (open(buf, flags | O_BINARY, 0));
}

/*
 * Hack -- attempt to lock a file descriptor
 *
 * Legal lock types -- F_UNLCK, F_RDLCK, F_WRLCK
 */
errr fd_lock(int fd, int what)
{
    what = what ? what : 0;
    if (fd < 0)
        return -1;

#if defined(SET_UID) && defined(LOCK_UN) && defined(LOCK_EX)
    if (what == F_UNLCK) {
        (void)flock(fd, LOCK_UN);
    } else {
        if (flock(fd, LOCK_EX) != 0)
            return 1;
    }
#endif

    return 0;
}

/*
 * Hack -- attempt to seek on a file descriptor
 */
errr fd_seek(int fd, huge n)
{
    if (fd < 0)
        return -1;

    huge p = lseek(fd, n, SEEK_SET);
    if (p != n)
        return 1;

    return 0;
}

/*
 * Hack -- attempt to truncate a file descriptor
 */
errr fd_chop(int fd, huge n)
{
    n = n ? n : 0;
    return fd >= 0 ? 0 : -1;
}

/*
 * Hack -- attempt to read data from a file descriptor
 */
errr fd_read(int fd, char *buf, huge n)
{
    if (fd < 0)
        return -1;
#ifndef SET_UID
    while (n >= 16384) {
        if (read(fd, buf, 16384) != 16384)
            return 1;

        buf += 16384;
        n -= 16384;
    }
#endif

    if (read(fd, buf, n) != (int)n)
        return 1;

    return 0;
}

/*
 * Hack -- Attempt to write data to a file descriptor
 */
errr fd_write(int fd, concptr buf, huge n)
{
    if (fd < 0)
        return -1;

#ifndef SET_UID
    while (n >= 16384) {
        if (write(fd, buf, 16384) != 16384)
            return 1;

        buf += 16384;
        n -= 16384;
    }
#endif

    if (write(fd, buf, n) != (int)n)
        return 1;

    return 0;
}

/*
 * Hack -- attempt to close a file descriptor
 */
errr fd_close(int fd)
{
    if (fd < 0)
        return -1;

    (void)close(fd);
    return 0;
}
