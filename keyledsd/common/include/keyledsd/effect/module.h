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
#ifndef KEYLEDSD_EFFECT_MODULE_H_1C5DA494
#define KEYLEDSD_EFFECT_MODULE_H_1C5DA494

#include "keyledsd_config.h"

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif

/****************************************************************************/

#define KEYLEDSD_MODULE_SIGNATURE \
    0xa7, 0x96, 0x85, 0xd4, 0xa9, 0x0c, 0x11, 0xe7, \
    0x98, 0x22, 0x28, 0xb2, 0xbd, 0x4c, 0xbb, 0xe3

/// Presents the module some details about the keyleds engine
struct host_definition
{
    uint16_t    major;                  ///< Version of the keyleds engine, major
    uint16_t    minor;                  ///< Version of the keyleds engine, minor
    void        (*error)(const char *); ///< Signals an error in low-level exchange
};

/// Main module entry point
struct module_definition
{
    uint8_t     signature[16];          ///< A copy of KEYLEDSD_MODULE_SIGNATURE
    uint32_t    abi_version;            ///< A copy of KEYLEDSD_ABI_VERSION
    uint16_t    major;                  ///< Keyleds engine version module was compiled against, major
    uint16_t    minor;                  ///< Keyleds engine version module was compiled against, minor

    /// Main module entry point - may call host->error and return nullptr to signal failure
    void *      (*initialize)(const struct host_definition * host);

    /// Plugin shutdown point - may call host->error and return false to signal failure
    bool        (*shutdown)(const struct host_definition * host, void *);
};

/****************************************************************************/

#ifdef __cplusplus
} // extern "C"
#endif

#define KEYLEDSD_DEFINE_MODULE(initialize_fn, shutdown_fn) \
    const struct module_definition keyledsd_module = { \
        { KEYLEDSD_MODULE_SIGNATURE }, \
        KEYLEDSD_ABI_VERSION, KEYLEDSD_VERSION_MAJOR, KEYLEDSD_VERSION_MINOR, \
        initialize_fn, shutdown_fn \
    }

#ifdef KEYLEDSD_MODULES_STATIC
#  ifdef __cplusplus
#  include "keyledsd/effect/StaticModuleRegistry.h"
#  define KEYLEDSD_EXPORT_MODULE(name, initialize_fn, shutdown_fn) \
        KEYLEDSD_DEFINE_MODULE(initialize_fn, shutdown_fn); \
        static USED const keyleds::effect::StaticModuleRegistry::Registration \
        keyledsModuleRegistration(name, &keyledsd_module)
#  else
#    error C modules cannot be static
#  endif
#else
#  ifdef __cplusplus
#  define KEYLEDSD_EXPORT_MODULE(name, initialize_fn, shutdown_fn) \
        extern "C" KEYLEDSD_EXPORT const struct module_definition keyledsd_module; \
        KEYLEDSD_DEFINE_MODULE(initialize_fn, shutdown_fn)
#  else
#  define KEYLEDSD_EXPORT_MODULE(name, initialize_fn, shutdown_fn) \
        extern KEYLEDSD_EXPORT const struct module_definition keyledsd_module; \
        KEYLEDSD_DEFINE_MODULE(initialize_fn, shutdown_fn)
#  endif
#endif

#endif
