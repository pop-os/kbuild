/* $Id: redirect.c 2839 2016-08-25 21:46:44Z bird $ */
/** @file
 * kmk_redirect - Do simple program <-> file redirection (++).
 */

/*
 * Copyright (c) 2007-2016 knut st. osmundsen <bird-kBuild-spamx@anduin.net>
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

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#ifdef __APPLE__
# define _POSIX_C_SOURCE 1 /* 10.4 sdk and unsetenv */
#endif
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#if defined(_MSC_VER)
# include <ctype.h>
# include <io.h>
# include <direct.h>
# include <process.h>
# include "quote_argv.h"
#else
# include <unistd.h>
#endif

#ifdef __OS2__
# define INCL_BASE
# include <os2.h>
# ifndef LIBPATHSTRICT
#  define LIBPATHSTRICT 3
# endif
#endif


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Number of times the '-v' switch was seen. */
static unsigned g_cVerbosity = 0;


#if defined(_MSC_VER)


/** Used by safeCloseFd. */
static void __cdecl ignore_invalid_parameter(const wchar_t *a, const wchar_t *b, const wchar_t *c, unsigned d, uintptr_t e)
{
}

#endif /* _MSC_VER */


/**
 * Safely works around MS CRT's pedantic close() function.
 *
 * @param   fd      The file handle.
 */
static void safeCloseFd(int fd)
{
#ifdef _MSC_VER
    _invalid_parameter_handler pfnOld = _get_invalid_parameter_handler();
    _set_invalid_parameter_handler(ignore_invalid_parameter);
    close(fd);
    _set_invalid_parameter_handler(pfnOld);
#else
    close(fd);
#endif
}


static const char *name(const char *pszName)
{
    const char *psz = strrchr(pszName, '/');
#if defined(_MSC_VER) || defined(__OS2__)
    const char *psz2 = strrchr(pszName, '\\');
    if (!psz2)
        psz2 = strrchr(pszName, ':');
    if (psz2 && (!psz || psz2 > psz))
        psz = psz2;
#endif
    return psz ? psz + 1 : pszName;
}


static int usage(FILE *pOut,  const char *argv0)
{
    fprintf(pOut,
            "usage: %s [-[rwa+tb]<fd> <file>] [-c<fd>] [-Z] [-E <var=val>] [-C <dir>] [--wcc-brain-damage] [-v] -- <program> [args]\n"
            "   or: %s --help\n"
            "   or: %s --version\n"
            "\n"
            "The rwa+tb is like for fopen, if not specified it defaults to w+.\n"
            "The <fd> is either a number or an alias for the standard handles:\n"
            "   i = stdin\n"
            "   o = stdout\n"
            "   e = stderr\n"
            "\n"
            "The -c switch will close the specified file descriptor.\n"
            "\n"
            "The -Z switch zaps the environment.\n"
            "\n"
            "The -E switch is for making changes to the environment in a putenv\n"
            "fashion.\n"
            "\n"
            "The -C switch is for changing the current directory. This takes immediate\n"
            "effect, so be careful where you put it.\n"
            "\n"
            "The --wcc-brain-damage switch is to work around wcc and wcc386 (Open Watcom)\n"
            "not following normal quoting conventions on Windows, OS/2, and DOS.\n"
            "\n"
            "The -v switch is for making the thing more verbose.\n"
            "\n"
            "This command was originally just a quick hack to avoid invoking the shell\n"
            "on Windows (cygwin) where forking is very expensive and has exhibited\n"
            "stability issues on SMP machines.  It has since grown into something like\n"
            "/usr/bin/env on steroids.\n"
            ,
            argv0, argv0, argv0);
    return 1;
}


int main(int argc, char **argv, char **envp)
{
    int i;
    int j;
#if defined(_MSC_VER)
    intptr_t rc;
#endif
    FILE *pStdErr = stderr;
    FILE *pStdOut = stdout;
    int fWatcomBrainDamage = 0;

    /*
     * Parse arguments.
     */
    if (argc <= 1)
        return usage(pStdErr, name(argv[0]));
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            int fd;
            int fdOpened;
            int fOpen;
            char *psz = &argv[i][1];
            if (*psz == '-')
            {
                /* '--' ? */
                if (!psz[1])
                {
                    i++;
                    break;
                }

                /* convert to short. */
                if (!strcmp(psz, "-help"))
                    psz = "h";
                else if (!strcmp(psz, "-version"))
                    psz = "V";
                else if (!strcmp(psz, "-env"))
                    psz = "E";
                else if (!strcmp(psz, "-chdir"))
                    psz = "C";
                else if (!strcmp(psz, "-zap-env"))
                    psz = "Z";
                else if (!strcmp(psz, "-close"))
                    psz = "c";
                else if (!strcmp(psz, "-wcc-brain-damage"))
                {
                    fWatcomBrainDamage = 1;
                    continue;
                }
            }

            /*
             * Deal with the obligatory help and version switches first.
             */
            if (*psz == 'h')
            {
                usage(pStdOut, name(argv[0]));
                return 0;
            }
            if (*psz == 'V')
            {
                printf("kmk_redirect - kBuild version %d.%d.%d (r%u)\n"
                       "Copyright (C) 2007-2012 knut st. osmundsen\n",
                       KBUILD_VERSION_MAJOR, KBUILD_VERSION_MINOR, KBUILD_VERSION_PATCH,
                       KBUILD_SVN_REV);
                return 0;
            }

            /*
             * Environment switch?
             */
            if (*psz == 'E')
            {
                psz++;
                if (*psz == ':' || *psz == '=')
                    psz++;
                else
                {
                    if (i + 1 >= argc)
                    {
                        fprintf(pStdErr, "%s: syntax error: no argument for %s\n", name(argv[0]), argv[i]);
                        return 1;
                    }
                    psz = argv[++i];
                }
#ifdef __OS2__
                if (    !strncmp(psz, "BEGINLIBPATH=",  sizeof("BEGINLIBPATH=") - 1)
                    ||  !strncmp(psz, "ENDLIBPATH=",    sizeof("ENDLIBPATH=") - 1)
                    ||  !strncmp(psz, "LIBPATHSTRICT=", sizeof("LIBPATHSTRICT=") - 1))
                {
                    ULONG ulVar = *psz == 'B' ? BEGIN_LIBPATH
                                : *psz == 'E' ? END_LIBPATH
                                :               LIBPATHSTRICT;
                    const char *pszVal = strchr(psz, '=') + 1;
                    APIRET rc = DosSetExtLIBPATH(pszVal, ulVar);
                    if (rc)
                    {
                        fprintf(pStdErr, "%s: error: DosSetExtLibPath(\"%s\", %.*s (%lu)): %lu\n",
                                name(argv[0]), pszVal, pszVal - psz - 1, psz, ulVar, rc);
                        return 1;
                    }
                }
                else
#endif /* __OS2__ */
                {
                    const char *pchEqual = strchr(psz, '=');
                    if (pchEqual && pchEqual[1] != '\0')
                    {
                        if (putenv(psz))
                        {
                            fprintf(pStdErr, "%s: error: putenv(\"%s\"): %s\n", name(argv[0]), psz, strerror(errno));
                            return 1;
                        }
                    }
                    else
                    {
                        size_t cchVar = pchEqual ? (size_t)(pchEqual - psz) : strlen(psz);
                        char *pszCopy = (char *)malloc(cchVar + 2);
                        memcpy(pszCopy, psz, cchVar);

#if defined(_MSC_VER) || defined(__OS2__)
                        pszCopy[cchVar] = '=';
                        pszCopy[cchVar + 1] = '\0';
                        if (putenv(pszCopy))
                        {
                            fprintf(pStdErr, "%s: error: putenv(\"%s\"): %s\n", name(argv[0]), pszCopy, strerror(errno));
                            return 1;
                        }
#else
                        pszCopy[cchVar] = '\0';
                        if (unsetenv(pszCopy))
                        {
                            fprintf(pStdErr, "%s: error: unsetenv(\"%s\"): %s\n", name(argv[0]), pszCopy, strerror(errno));
                            return 1;
                        }
#endif
                        free(pszCopy);
                    }
                }
                continue;
            }

            /*
             * Change directory switch?
             */
            if (*psz == 'C')
            {
                psz++;
                if (*psz == ':' || *psz == '=')
                    psz++;
                else
                {
                    if (i + 1 >= argc)
                    {
                        fprintf(pStdErr, "%s: syntax error: no argument for %s\n", name(argv[0]), argv[i]);
                        return 1;
                    }
                    psz = argv[++i];
                }
                if (!chdir(psz))
                    continue;
#ifdef _MSC_VER
                {
                    /* drop trailing slash if any. */
                    size_t cch = strlen(psz);
                    if (    cch > 2
                        &&  (psz[cch - 1] == '/' || psz[cch - 1] == '\\')
                        &&  psz[cch - 1] != ':')
                    {
                        int rc2;
                        char *pszCopy = strdup(psz);
                        do  pszCopy[--cch] = '\0';
                        while (    cch > 2
                               &&  (pszCopy[cch - 1] == '/' || pszCopy[cch - 1] == '\\')
                               &&  pszCopy[cch - 1] != ':');
                        rc2 = chdir(pszCopy);
                        free(pszCopy);
                        if (!rc2)
                            continue;
                    }
                }
#endif
                fprintf(pStdErr, "%s: error: chdir(\"%s\"): %s\n", name(argv[0]), psz, strerror(errno));
                return 1;
            }

            /*
             * Zap environment switch?
             * This is a bit of a hack.
             */
            if (*psz == 'Z')
            {
                unsigned j = 0;
                while (envp[j] != NULL)
                    j++;
                while (j-- > 0)
                {
                    char *pszEqual = strchr(envp[j], '=');
                    char *pszCopy;

                    if (pszEqual)
                        *pszEqual = '\0';
                    pszCopy = strdup(envp[j]);
                    if (pszEqual)
                        *pszEqual = '=';

#if defined(_MSC_VER) || defined(__OS2__)
                    putenv(pszCopy);
#else
                    unsetenv(pszCopy);
#endif
                    free(pszCopy);
                }
                continue;
            }

            /*
             * Verbose operation switch?
             */
            if (*psz == 'v')
            {
                g_cVerbosity++;
                continue;
            }

            /*
             * Close the specified file descriptor (no stderr/out/in aliases).
             */
            if (*psz == 'c')
            {
                psz++;
                if (!*psz)
                {
                    i++;
                    if (i >= argc)
                    {
                        fprintf(pStdErr, "%s: syntax error: missing filename argument.\n", name(argv[0]));
                        return 1;
                    }
                    psz = argv[i];
                }

                fd = (int)strtol(psz, &psz, 0);
                if (!fd || *psz)
                {
                    fprintf(pStdErr, "%s: error: failed to convert '%s' to a number\n", name(argv[0]), argv[i]);
                    return 1;

                }
                if (fd < 0)
                {
                    fprintf(pStdErr, "%s: error: negative fd %d (%s)\n", name(argv[0]), fd, argv[i]);
                    return 1;
                }
                /** @todo deal with stderr */
                safeCloseFd(fd);
                continue;
            }

            /*
             * Parse a file descriptor argument.
             */

            /* mode */
            switch (*psz)
            {
                case 'r':
                    psz++;
                    if (*psz == '+')
                    {
                        fOpen = O_RDWR;
                        psz++;
                    }
                    else
                        fOpen = O_RDONLY;
                    break;

                case 'w':
                    psz++;
                    if (*psz == '+')
                    {
                        psz++;
                        fOpen = O_RDWR | O_CREAT | O_TRUNC;
                    }
                    else
                        fOpen = O_WRONLY | O_CREAT | O_TRUNC;
                    break;

                case 'a':
                    psz++;
                    if (*psz == '+')
                    {
                        psz++;
                        fOpen = O_RDWR | O_CREAT | O_APPEND;
                    }
                    else
                        fOpen = O_WRONLY | O_CREAT | O_APPEND;
                    break;

                case 'i': /* make sure stdin is read-only. */
                    fOpen = O_RDONLY;
                    break;

                case '+':
                    fprintf(pStdErr, "%s: syntax error: Unexpected '+' in '%s'\n", name(argv[0]), argv[i]);
                    return 1;

                default:
                    fOpen = O_RDWR | O_CREAT | O_TRUNC;
                    break;
            }

            /* binary / text modifiers */
            switch (*psz)
            {
                case 'b':
#ifdef O_BINARY
                    fOpen |= O_BINARY;
#endif
                    psz++;
                    break;

                case 't':
#ifdef O_TEXT
                    fOpen |= O_TEXT;
#endif
                    psz++;
                    break;

                default:
#ifdef O_BINARY
                    fOpen |= O_BINARY;
#endif
                    break;

            }

            /* convert to file descriptor number */
            switch (*psz)
            {
                case 'i':
                    fd = 0;
                    psz++;
                    break;

                case 'o':
                    fd = 1;
                    psz++;
                    break;

                case 'e':
                    fd = 2;
                    psz++;
                    break;

                case '0':
                    if (!psz[1])
                    {
                        fd = 0;
                        psz++;
                        break;
                    }
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    fd = (int)strtol(psz, &psz, 0);
                    if (!fd)
                    {
                        fprintf(pStdErr, "%s: error: failed to convert '%s' to a number\n", name(argv[0]), argv[i]);
                        return 1;

                    }
                    if (fd < 0)
                    {
                        fprintf(pStdErr, "%s: error: negative fd %d (%s)\n", name(argv[0]), fd, argv[i]);
                        return 1;
                    }
                    break;

                /*
                 * Invalid argument.
                 */
                default:
                    fprintf(pStdErr, "%s: error: failed to convert '%s' ('%s') to a file descriptor\n", name(argv[0]), psz, argv[i]);
                    return 1;
            }

            /*
             * Check for the filename.
             */
            if (*psz)
            {
                if (*psz != ':' && *psz != '=')
                {
                    fprintf(pStdErr, "%s: syntax error: characters following the file descriptor: '%s' ('%s')\n", name(argv[0]), psz, argv[i]);
                    return 1;
                }
                psz++;
            }
            else
            {
                i++;
                if (i >= argc)
                {
                    fprintf(pStdErr, "%s: syntax error: missing filename argument.\n", name(argv[0]));
                    return 1;
                }
                psz = argv[i];
            }

            /*
             * Setup the redirection.
             */
            if (fd == fileno(pStdErr))
            {
                /*
                 * Move stderr to a new location, making it close on exec.
                 * If pStdOut has already teamed up with pStdErr, update it too.
                 */
                FILE *pNew;
                fdOpened = dup(fileno(pStdErr));
                if (fdOpened == -1)
                {
                    fprintf(pStdErr, "%s: error: failed to dup stderr (%d): %s\n", name(argv[0]), fileno(pStdErr), strerror(errno));
                    return 1;
                }
#ifdef _MSC_VER
                /** @todo figure out how to make the handle close-on-exec. We'll simply close it for now.
                 * SetHandleInformation + set FNOINHERIT in CRT.
                 */
#else
                if (fcntl(fdOpened, F_SETFD, FD_CLOEXEC) == -1)
                {
                    fprintf(pStdErr, "%s: error: failed to make stderr (%d) close-on-exec: %s\n", name(argv[0]), fdOpened, strerror(errno));
                    return 1;
                }
#endif

                pNew = fdopen(fdOpened, "w");
                if (!pNew)
                {
                    fprintf(pStdErr, "%s: error: failed to fdopen the new stderr (%d): %s\n", name(argv[0]), fdOpened, strerror(errno));
                    return 1;
                }
                if (pStdOut == pStdErr)
                    pStdOut = pNew;
                pStdErr = pNew;
            }
            else if (fd == 1 && pStdOut != pStdErr)
                pStdOut = pStdErr;

            /*
             * Close and open the new file descriptor.
             */
            safeCloseFd(fd);
#if defined(_MSC_VER)
            if (!strcmp(psz, "/dev/null"))
                psz = (char *)"nul";
#endif
            fdOpened = open(psz, fOpen, 0666);
            if (fdOpened == -1)
            {
                fprintf(pStdErr, "%s: error: failed to open '%s' as %d: %s\n", name(argv[0]), psz, fd, strerror(errno));
                return 1;
            }
            if (fdOpened != fd)
            {
                /* move it (dup2 returns 0 on MSC). */
                if (dup2(fdOpened, fd) == -1)
                {
                    fprintf(pStdErr, "%s: error: failed to dup '%s' as %d: %s\n", name(argv[0]), psz, fd, strerror(errno));
                    return 1;
                }
                close(fdOpened);
            }
        }
        else
        {
            fprintf(pStdErr, "%s: syntax error: Invalid argument '%s'.\n", name(argv[0]), argv[i]);
            return usage(pStdErr, name(argv[0]));
        }
    }

    /*
     * Make sure there's something to execute.
     */
    if (i >= argc)
    {
        fprintf(pStdErr, "%s: syntax error: nothing to execute!\n", name(argv[0]));
        return usage(pStdErr, name(argv[0]));
    }

#if defined(_MSC_VER)
    if (fileno(pStdErr) != 2) /* no close-on-exec flag on windows */
    {
        fclose(pStdErr);
        pStdErr = NULL;
    }

    /* MSC is a PITA since it refuses to quote the arguments... */
    quote_argv(argc - i, &argv[i], fWatcomBrainDamage, 0 /*fFreeOrLeak*/);
    if (g_cVerbosity > 0)
        for (j = i; j < argc; j++)
            fprintf(pStdErr, "kmk_redirect: debug: argv[%i]=%s<eos>\n", j - i, argv[j]);
    rc = _spawnvp(_P_WAIT, argv[i], &argv[i]);
    if (rc == -1 && pStdErr)
    {
        fprintf(pStdErr, "%s: error: _spawnvp(_P_WAIT, \"%s\", ...) failed: %s\n", name(argv[0]), argv[i], strerror(errno));
        rc = 1;
    }
    return rc;
#else
    if (g_cVerbosity > 0)
        for (j = i; j < argc; j++)
            fprintf(pStdErr, "kmk_redirect: debug: argv[%i]=%s<eos>\n", j - i, argv[j]);
    execvp(argv[i], &argv[i]);
    fprintf(pStdErr, "%s: error: _execvp(_P_WAIT, \"%s\", ...) failed: %s\n", name(argv[0]), argv[i], strerror(errno));
    return 1;
#endif
}

