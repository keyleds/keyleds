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
#ifndef KEYLEDS_LOGGING_H
#define KEYLEDS_LOGGING_H

#include <stdio.h>
#include <string.h>

extern /*@null@*/ FILE * g_keyleds_debug_stream;
extern int g_keyleds_debug_level;
extern int g_keyleds_debug_hid;

#if !defined(NDEBUG) && !defined(S_SPLINT_S)
#define KEYLEDS_LOG(level, ...) \
    do { if (g_keyleds_debug_level >= KEYLEDS_LOG_##level) { \
        FILE * stream = g_keyleds_debug_stream == NULL ? stderr : g_keyleds_debug_stream; \
        (void)fprintf(stream, "%s:%d: ", strstr(__FILE__, "src/keyleds") + 4, __LINE__); \
        (void)fprintf(stream, __VA_ARGS__); \
        (void)fprintf(stream, "\n"); \
        (void)fflush(stream); \
    } } while (0)
#else
#define KEYLEDS_LOG(level, ...) \
    do { if (g_keyleds_debug_level >= KEYLEDS_LOG_##level) { \
        FILE * stream = g_keyleds_debug_stream == NULL ? stderr : g_keyleds_debug_stream; \
        (void)fprintf(stream, "keyleds: "); \
        (void)fprintf(stream, __VA_ARGS__); \
        (void)fprintf(stream, "\n"); \
        (void)fflush(stream); \
    } } while (0)
#endif

#endif
