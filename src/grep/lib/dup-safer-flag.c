/* Duplicate a file descriptor result, avoiding clobbering
   STD{IN,OUT,ERR}_FILENO, with specific flags.

   Copyright (C) 2001, 2004-2006, 2009-2021 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Paul Eggert and Eric Blake.  */

#include <config.h>

/* Specification.  */
#include "unistd-safer.h"

#include <fcntl.h>
#include <unistd.h>

/* Like dup, but do not return STDIN_FILENO, STDOUT_FILENO, or
   STDERR_FILENO.  If FLAG contains O_CLOEXEC, behave like
   fcntl(F_DUPFD_CLOEXEC) rather than fcntl(F_DUPFD).  */

int
dup_safer_flag (int fd, int flag)
{
#if defined(KMK_GREP) && defined(_MSC_VER)
  int afd[STDERR_FILENO + 1];
  int i;
  for (i = 0; ; i++)
    {
      int fdNew = _dup(fd);
      if (fdNew >= STDERR_FILENO + 1 || fdNew < 0)
        {
          while (i-- > 0)
            close(afd[i]);
          return fdNew;
        }
      afd[i] = fdNew;
    }
#else
  return fcntl (fd, (flag & O_CLOEXEC) ? F_DUPFD_CLOEXEC : F_DUPFD,
                STDERR_FILENO + 1);
#endif
}
