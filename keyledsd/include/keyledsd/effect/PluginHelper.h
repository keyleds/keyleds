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
#ifndef KEYLEDSD_EFFECT_PLUGIN_HELPER_H_41D603AB
#define KEYLEDSD_EFFECT_PLUGIN_HELPER_H_41D603AB

#include "keyledsd/effect/interfaces.h"
#include "keyledsd/effect/module.h"

namespace plugin {

/****************************************************************************/

class Effect : public keyleds::effect::interface::Effect
{
protected:
    using EffectService = keyleds::effect::interface::EffectService;
public:
    void    handleContextChange(const string_map &) override {}
    void    handleGenericEvent(const string_map &) override {}
    void    handleKeyEvent(const KeyDatabase::Key &, bool) override {}
};

template <typename T>
class Plugin final : public keyleds::effect::interface::Plugin
{
    using EffectService = keyleds::effect::interface::EffectService;
public:
    Plugin(const char * name) : m_name(name) {}
    ~Plugin() {}

    keyleds::effect::interface::Effect *
    createEffect(const std::string & name, EffectService & service) override
    {
        if (name == m_name) { return new T(service); }
        return nullptr;
    }

    void destroyEffect(keyleds::effect::interface::Effect * ptr) override
    {
        delete static_cast<T *>(ptr);
    }

private:
    const char * m_name;
};

/****************************************************************************/

#define KEYLEDSD_EXPORT_PLUGIN(name, PluginKlass) \
    static void * keyledsd_simple_create(const struct host_definition * host) \
        { try { return new PluginKlass(name); } \
          catch (std::exception & err) { (*host->error)(err.what()); } \
          catch (...) { (*host->error)("unknown exception"); } \
          return nullptr; }\
    static bool keyledsd_simple_destroy(const struct host_definition * host, void * ptr) \
        { try { delete static_cast<PluginKlass *>(ptr); } \
          catch (std::exception & err) { (*host->error)(err.what()); return false; } \
          catch (...) {} \
          return true; } \
    KEYLEDSD_EXPORT_MODULE(name, keyledsd_simple_create, keyledsd_simple_destroy)

#define KEYLEDSD_SIMPLE_EFFECT(name, Klass) \
    using Klass##Plugin = plugin::Plugin<Klass>; \
    KEYLEDSD_EXPORT_PLUGIN(name, Klass##Plugin)

/****************************************************************************/

} // namespace plugin

#endif
