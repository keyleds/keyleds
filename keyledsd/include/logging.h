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
#ifndef LOGGING_H
#define LOGGING_H

#include <cstdio>

extern /*@null@*/ std::FILE * g_debug_stream;
extern int g_debug_level;

#define LOG_ERROR       (1)
#define LOG_WARNING     (2)
#define LOG_INFO        (3)
#define LOG_DEBUG       (4)

#if !defined(NDEBUG) && !defined(S_SPLINT_S)
#define LOG(level, ...) \
    do { if (g_debug_level >= LOG_##level) { \
        FILE * stream = g_debug_stream == nullptr ? stderr : g_debug_stream; \
        (void)std::fprintf(stream, "%s:%d: ", strstr(__FILE__, "src/") + 4, __LINE__); \
        (void)std::fprintf(stream, __VA_ARGS__); \
        (void)std::fprintf(stream, "\n"); \
        (void)std::fflush(stream); \
    } } while (0)
#else
#define LOG(level, ...) \
    do { if (g_debug_level >= LOG_##level) { \
        FILE * stream = g_debug_stream == nullptr ? stderr : g_debug_stream; \
        (void)std::fprintf(stream, __VA_ARGS__); \
        (void)std::fprintf(stream, "\n"); \
        (void)std::fflush(stream); \
    } } while (0)
#endif

#endif
