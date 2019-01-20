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
#ifndef KEYLEDSD_EFFECT_INTERFACES_H_07881F1A
#define KEYLEDSD_EFFECT_INTERFACES_H_07881F1A

#include <string>
#include <vector>
#include "keyledsd/KeyDatabase.h"
#include "keyledsd/RenderTarget.h"

namespace keyleds { struct RGBAColor; }

namespace keyleds { namespace effect { namespace interface {

class EffectService;

/****************************************************************************/
// IMPLEMENTED BY PLUGIN

/// Core object used by DeviceManager and RenderLoop
class Effect
{
protected:
    using KeyDatabase = keyleds::KeyDatabase;
    using Renderer = keyleds::Renderer;
    using string_map = std::vector<std::pair<std::string, std::string>>;
public:
    /// Invoked whenever the context of the service has changed while the plugin is active.
    /// Since plugins are loaded on context changes, this means this is always called
    /// once before periodic calls to render start.
    virtual void    handleContextChange(const string_map &) = 0;

    /// Invoked whenever the Service is sent a generic event while the plugin is active.
    /// string_map holds whatever values the event includes, keyledsd does not use it.
    virtual void    handleGenericEvent(const string_map &) = 0;

    /// Invoked whenever the user presses or releases a key while the plugin is active.
    virtual void    handleKeyEvent(const KeyDatabase::Key &, bool press) = 0;

    /// Return a Renderer interface that can draw the effect into a RenderTarget
    virtual Renderer * renderer() = 0;
};

/// Manages communication with engine
class Plugin
{
public:
    virtual Effect *    createEffect(const std::string & name, EffectService &) = 0;
    virtual void        destroyEffect(Effect *, EffectService &) = 0;

protected:
    ~Plugin() {}
};

/****************************************************************************/
// IMPLEMENTED BY ENGINE

/// Facade to DeviceManager & configuration passed to plugins
class EffectService
{
protected:
    using KeyGroup = KeyDatabase::KeyGroup;
    using string_map = std::vector<std::pair<std::string, std::string>>;
public:
    virtual ~EffectService() {}

    virtual const std::string & deviceName() const = 0;     ///< Name, as defined by user
    virtual const std::string & deviceModel() const = 0;    ///< Model string (eg usb product id)
    virtual const std::string & deviceSerial() const = 0;   ///< Unique device identifier

    virtual const KeyDatabase & keyDB() const = 0;          ///< Compiled key information
    virtual const std::vector<KeyGroup> & keyGroups() const = 0;    ///< Seen from effect scope

    virtual const string_map &  configuration() const = 0;
    virtual const std::string & getConfig(const char *) const = 0;

    virtual RenderTarget *      createRenderTarget() = 0;
    virtual void                destroyRenderTarget(RenderTarget *) = 0;

    virtual const std::string & getFile(const std::string &) = 0;

    virtual void                log(unsigned, const char *) = 0;
};

/****************************************************************************/

} } } // namespace keyleds::effect::interface

#endif
