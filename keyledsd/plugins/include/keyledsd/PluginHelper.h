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

#include "keyledsd/RenderTarget.h"
#include "keyledsd/plugin/interfaces.h"
#include "keyledsd/plugin/module.h"

namespace keyleds::plugin {

/****************************************************************************/

/** Empty implementation for the Effect and Renderer interface, for simple effects.
 */
class SimpleEffect : public Effect, public Renderer
{
public:
    void    handleContextChange(const string_map &) override {}
    void    handleGenericEvent(const string_map &) override {}
    void    handleKeyEvent(const KeyDatabase::Key &, bool) override {}

    Renderer * renderer() override { return this; }
};


/** Automatic plugin class for simple effects.
 * @tparam T Effect class, derived from Effect.
 */
template <typename T>
class SimplePlugin : public Plugin
{
public:
    explicit SimplePlugin(const char * name) : m_name(name) {}

    Effect * createEffect(const std::string & name, EffectService & service) override
    {
        if (name == m_name) { return new T(service); }
        return nullptr;
    }

    void destroyEffect(Effect * ptr, EffectService &) override
    {
        delete static_cast<T *>(ptr);
    }

protected:
    ~SimplePlugin() {}
    const char * name() const { return m_name; }

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
    class Klass##Plugin final : public plugin::SimplePlugin<Klass> { using SimplePlugin::SimplePlugin; }; \
    KEYLEDSD_EXPORT_PLUGIN(name, Klass##Plugin)

/****************************************************************************/

} // namespace plugin

#endif
