/* $Id: mscfakes.h 2592 2012-06-17 22:50:38Z bird $ */
/** @file
 * Unix fakes for MSC.
 */

/*
 * Copyright (c) 2005-2010 knut st. osmundsen <bird-kBuild-spamx@anduin.net>
 *
 * This file is part of kBuild.
 *
 * kBuild is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * kBuild is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with kBuild.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#ifndef ___mscfakes_h
#define ___mscfakes_h
#ifdef _MSC_VER

#include <io.h>
#include <direct.h>
#include <time.h>
#include <stdarg.h>
#include <malloc.h>
#include "getopt.h"

/* Note: Duplicated it config.h.win */
#include <sys/stat.h>
#include <io.h>
#include <direct.h>
#if defined(MSC_DO_64_BIT_IO) && _MSC_VER >= 1400 /* We want 64-bit file lengths here when possible. */
# define off_t __int64
# undef stat
# define stat  _stat64
# define fstat _fstat64
# define lseek _lseeki64
#else
# ifndef STAT_REDEFINED_ALREADY
#  define STAT_REDEFINED_ALREADY
#  undef stat
#  define stat(_path, _st) bird_w32_stat(_path, _st)
extern int bird_w32_stat(const char *, struct stat *);
# endif
#endif

#ifndef S_ISDIR
# define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#endif
#ifndef S_ISREG
# define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif
#define S_ISLNK(m)  0
#define	S_IRWXU (_S_IREAD | _S_IWRITE | _S_IEXEC)
#define	S_IXUSR _S_IEXEC
#define	S_IWUSR _S_IWRITE
#define	S_IRUSR _S_IREAD
#define S_IRWXG 0000070
#define S_IRGRP	0000040
#define S_IWGRP	0000020
#define S_IXGRP 0000010
#define S_IRWXO 0000007
#define S_IROTH	0000004
#define S_IWOTH	0000002
#define S_IXOTH 0000001
#define	S_ISUID 0004000
#define	S_ISGID 0002000
#define ALLPERMS 0000777

#undef  PATH_MAX
#define PATH_MAX   _MAX_PATH
#undef  MAXPATHLEN
#define MAXPATHLEN _MAX_PATH

#define EX_OK 0
#define EX_OSERR 1
#define EX_NOUSER 1
#define EX_USAGE 1

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4

#define EFTYPE EINVAL

#define _PATH_DEVNULL "/dev/null"

#ifndef MAX
# define MAX(a,b) ((a) >= (b) ? (a) : (b))
#endif

typedef int mode_t;
typedef unsigned short nlink_t;
#if 0 /* found in config.h */
typedef unsigned short uid_t;
typedef unsigned short gid_t;
#endif
typedef long ssize_t;
typedef unsigned long u_long;
typedef unsigned int u_int;
typedef unsigned short u_short;

#ifndef timerisset
struct timeval
{
    long tv_sec;
    long tv_usec;
};
#endif

struct iovec
{
    char *iov_base;
    size_t iov_len;
};

typedef __int64 intmax_t;
#if 0 /* found in config.h */
typedef unsigned __int64 uintmax_t;
#endif

#define chown(path, uid, gid) 0         /** @todo implement fchmod! */
char *dirname(char *path);
#define fsync(fd)  0
#define fchown(fd,uid,gid) 0
#define fchmod(fd, mode) 0              /** @todo implement fchmod! */
#define geteuid()  0
#define getegid()  0
#define lstat(path, s) stat(path, s)
int lchmod(const char *path, mode_t mode);
int msc_chmod(const char *path, mode_t mode);
#define chmod msc_chmod
#define lchown(path, uid, gid) chown(path, uid, gid)
#define lutimes(path, tvs) utimes(path, tvs)
int link(const char *pszDst, const char *pszLink);
int mkdir_msc(const char *path, mode_t mode);
#define mkdir(path, mode) mkdir_msc(path, mode)
#define mkfifo(path, mode) -1
#define mknod(path, mode, devno) -1
int mkstemp(char *temp);
#define readlink(link, buf, size) -1
#define reallocf(old, size) realloc(old, size)
int rmdir_msc(const char *path);
#define rmdir(path) rmdir_msc(path)
intmax_t strtoimax(const char *nptr, char **endptr, int base);
uintmax_t strtoumax(const char *nptr, char **endptr, int base);
#define strtoll(a,b,c) strtoimax(a,b,c)
#define strtoull(a,b,c) strtoumax(a,b,c)
int asprintf(char **strp, const char *fmt, ...);
int vasprintf(char **strp, const char *fmt, va_list ap);
#if _MSC_VER < 1400
int snprintf(char *buf, size_t size, const char *fmt, ...);
#else
#define snprintf _snprintf
#endif
int symlink(const char *pszDst, const char *pszLink);
int utimes(const char *pszPath, const struct timeval *paTimes);
int writev(int fd, const struct iovec *vector, int count);

#endif /* _MSC_VER */
#endif

