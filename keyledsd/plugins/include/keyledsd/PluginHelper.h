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
#include "keyledsd/tools/utils.h"
#include <chrono>
#include <limits>
#include <type_traits>

namespace keyleds::plugin {

/****************************************************************************/

namespace detail {
    template <typename C>
    struct has_factory {
    private:
        template <typename U> static auto check(int) ->
            std::is_same<decltype(U::create(std::declval<EffectService &>())), U*>;
        template<typename> static std::false_type check(...);
    public:
        static constexpr bool value = decltype(check<C>(0))::value;
    };
    template <typename C> inline constexpr bool has_factory_v = has_factory<C>::value;
}

namespace detail {
    using variant_wrapper = std::reference_wrapper<const EffectService::config_map::value_type::second_type>;

    template <typename T, typename = std::void_t<> > struct get_config {};

    template <> struct get_config<std::string> {
        using alternative = std::string;
        using value_type = std::string;
        static std::optional<value_type> parse(const EffectService &, const alternative & str)
            { return str; }
    };

    template <typename T>
    struct get_config<T, typename std::enable_if_t<std::is_integral_v<T>>> {
        using alternative = std::string;
        using value_type = std::remove_cv_t<T>;
        static std::optional<value_type> parse(const EffectService &, const alternative & str) {
            auto val = tools::parseNumber(str);
            if (!val) { return std::nullopt; }
            if (*val < std::numeric_limits<T>::min() || *val > std::numeric_limits<T>::max()) {
                return std::nullopt;
            }
            return *val;
        }
    };

    template <> struct get_config<RGBAColor> {
        using alternative = std::string;
        using value_type = RGBAColor;
        static std::optional<value_type> parse(const EffectService &, const alternative & str)
            { return RGBAColor::parse(str); }
    };

    template <typename Rep, typename Period>
    struct get_config<std::chrono::duration<Rep, Period>> {
        using alternative = std::string;
        using value_type = std::chrono::duration<Rep, Period>;
        static std::optional<value_type>
        parse(const EffectService &, const alternative & str)
            { return tools::parseDuration<std::chrono::duration<Rep, Period>>(str); }
    };

    template <>
    struct get_config<std::vector<RGBAColor>> {
        using alternative = std::vector<std::string>;
        using value_type = std::vector<RGBAColor>;
        static std::optional<value_type> parse(const EffectService &, const alternative & seq);
    };

    template <>
    struct get_config<KeyDatabase::KeyGroup> {
        using alternative = std::string;
        using value_type = KeyDatabase::KeyGroup;
        static std::optional<value_type> parse(const EffectService &, const alternative & str);
    };

}


// Actual getConfig, retrieves a ref wrapper to variant value
std::optional<detail::variant_wrapper> getConfig(const EffectService &, const char * key);

template <typename T>
std::optional<T> getConfig(EffectService & service, const char * key)
{
    using Alternative = typename detail::get_config<T>::alternative;
    auto & Parser = detail::get_config<T>::parse;

    const auto & result = getConfig(service, key);
    if (!result) { return std::nullopt; }

    if constexpr (std::is_same_v<Alternative, void>) {
        return Parser(service, *result);
    } else {
        if (std::holds_alternative<Alternative>(result->get())) {
            return Parser(service, std::get<Alternative>(result->get()));
        }
    }
    return std::nullopt;
}

#ifndef KEYLEDS_IGNORE_COMPATIBILITY
std::optional<std::vector<RGBAColor>>
getColorsCompatibility(EffectService & service, const char * key);

template <> std::optional<std::vector<RGBAColor>> inline
getConfig<std::vector<RGBAColor>>(EffectService & service, const char * key)
{
    using Alternative = typename detail::get_config<std::vector<RGBAColor>>::alternative;
    auto & Parser = detail::get_config<std::vector<RGBAColor>>::parse;

    const auto & sequence = getConfig(service, key);
    if (sequence && std::holds_alternative<Alternative>(sequence->get())) {
        return Parser(service, std::get<Alternative>(sequence->get()));
    }
    return getColorsCompatibility(service, key);
}
#endif

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
        if (name == m_name) {
            if constexpr (detail::has_factory_v<T>) {
                return T::create(service);
            } else {
                return new T(service);
            }
        }
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
