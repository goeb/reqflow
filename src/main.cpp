/*   Req
 *   Copyright (C) 2014 Frederic Hoerni
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "global.h"
#include "logging.h"

void usage()
{
    printf("Usage: req [--version] [--help]\n"
           "           [options] <command> [<args>]\n"
           "\n"
           "Commands:\n"
           "\n"
           "    stat\n"
           "\n"
           "    list\n"
           "\n"
           "    --version\n"
           "    --help\n"
           "\n"
           "\n");
    exit(1);
}

int showVersion()
{
    printf("Small Issue Tracker v%s\n"
           "Copyright (C) 2013 Frederic Hoerni\n"
           "\n"
           "This program is free software; you can redistribute it and/or modify\n"
           "it under the terms of the GNU General Public License as published by\n"
           "the Free Software Foundation; either version 2 of the License, or\n"
           "(at your option) any later version.\n"
           "\n"
           "This program is distributed in the hope that it will be useful,\n"
           "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
           "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
           "GNU General Public License for more details.\n"
           , VERSION);
    exit(1);
}

int main(int argc, const char **argv)
{
    if (argc < 2) usage();

    int i = 1;
    const char *command = 0;
    while (i<argc) {

        command = argv[1]; i++;

        if (0 == strcmp(command, "stat")) {

        } else if (0 == strcmp(command, "--version")) {
            return showVersion();

        } else if (0 == strcmp(command, "list")) {

        } else usage();

    }

    return 0;
}
