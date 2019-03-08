/* Keyleds -- Gaming keyboard tool
 * Copyright (C) 2017 Julien Hartmann, juli1.hartmann@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "keyledsctl.h"

#include "config.h"
#include "keyleds.h"
#include "logging.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

/****************************************************************************/

static int main_help(int argc, char * argv[]);

struct main_modes {
    const char *    name;
    int             (*entry)(int argc, char * argv[]);
    const char *    usage;
};

static const struct main_modes main_modes[] = {
    { "help", main_help,
      "Usage: %s %s [subcommand]\n" },
    { "list", main_list,
      "Usage: %s [-dqv] %s\n" },
    { "info", main_info,
      "Usage: %s [-dqv] %s [-d device]\n" },
    { "gkeys", main_gkeys,
      "Usage: %s [-dqv] %s [-d device] on|off\n" },
    { "get-leds", main_get_leds,
      "Usage: %s [-dqv] %s [-d device] [key1 [key2 [...]]]\n" },
    { "set-leds", main_set_leds,
      "Usage: %s [-dqv] %s [-d device] [key1=color1 [key2=color2 [...]]]\n" },
    { "gamemode", main_gamemode,
      "Usage: %s [-dqv] %s [-d device] [key1 [key2 [...]]]\n" },
};

/****************************************************************************/


struct main_options {
    /*@observer@*/ const char * mode;
    int                         keyleds_verbosity;
    int                         verbosity;
};

void main_usage(FILE * stream, const char * name)
{
    unsigned idx;
    (void)fprintf(stream, "Usage: %s [-dqv] ", name);
    for (idx = 0; idx < sizeof(main_modes) / sizeof(main_modes[0]); idx += 1) {
        (void)fprintf(stream, idx == 0 ? "%s" : "|%s", main_modes[idx].name);
    }
    (void)fputc('\n', stream);
}

bool parse_main_options(int argc, char * argv[], /*@out@*/ struct main_options * const options)
{
    int opt;
    options->mode       = NULL;
    options->keyleds_verbosity = KEYLEDS_LOG_WARNING;
    options->verbosity  = LOG_WARNING;

    while ((opt = getopt(argc, argv, "+dqv")) != -1) {
        switch (opt) {
        case 'd': options->keyleds_verbosity += 1; break;
        case 'q':
            options->keyleds_verbosity = KEYLEDS_LOG_ERROR;
            options->verbosity = LOG_ERROR;
            break;
        case 'v': options->verbosity += 1; break;
        default:
            return false;
        }
    }
    if (optind >= argc) { return false; }
    options->mode = argv[optind++];
    return true;
}

int main(int argc, char * argv[])
{
    struct main_options options;
    unsigned idx;

    if (!parse_main_options(argc, argv, &options)) {
        main_usage(stderr, argv[0]);
        return 1;
    }

    g_debug_level = options.verbosity;
    g_keyleds_debug_level = options.keyleds_verbosity;

    for (idx = 0; idx < sizeof(main_modes) / sizeof(main_modes[0]); idx += 1) {
        if (strcmp(main_modes[idx].name, options.mode) == 0) {
            return (*main_modes[idx].entry)(argc, argv);
        }
    }

    (void)fprintf(stderr, "%s: unknown mode -- '%s'\n", argv[0], options.mode);
    main_usage(stderr, argv[0]);
    return 1;
}

/****************************************************************************/

void reset_getopt(int argc, char * const argv[], const char * optstring)
{
    int saved_optind = optind;
    optind = 0; opterr = 0;
    (void)getopt(argc, argv, optstring);
    optind = saved_optind; opterr = 1;
}

/****************************************************************************/

static int main_help(int argc, char * argv[])
{
    const char * mode;
    unsigned idx;

    if (optind >= argc) {
        main_usage(stdout, argv[0]);
        return EXIT_SUCCESS;
    }

    mode = argv[optind];

    for (idx = 1; idx < sizeof(main_modes) / sizeof(main_modes[0]); idx += 1) {
        if (strcmp(main_modes[idx].name, mode) == 0) {
            (void)printf(main_modes[idx].usage, argv[0], mode);
            return EXIT_SUCCESS;
        }
    }
    (void)fprintf(stderr, "Unknown mode '%s'\n", mode);
    main_usage(stderr, argv[0]);
    return 1;
}
