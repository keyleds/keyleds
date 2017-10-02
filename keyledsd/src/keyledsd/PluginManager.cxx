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
#include "keyledsd/PluginManager.h"

#include <algorithm>

using keyleds::EffectPlugin;
using keyleds::EffectPluginFactory;
using keyleds::EffectPluginManager;

EffectPlugin::~EffectPlugin() {}
void EffectPlugin::handleContextChange(const string_map &) {}
void EffectPlugin::handleGenericEvent(const string_map &) {}
void EffectPlugin::handleKeyEvent(const KeyDatabase::Key &, bool) {}

EffectPluginFactory::~EffectPluginFactory() {}

void EffectPluginManager::registerPlugin(EffectPluginFactory * plugin)
{
    m_plugins.push_back(plugin);
}

EffectPluginFactory * EffectPluginManager::get(const std::string & name)
{
    auto it = std::find_if(m_plugins.begin(), m_plugins.end(),
                           [&name](const auto & plugin) { return plugin->name() == name; });
    if (it == m_plugins.end()) { return nullptr; }
    return *it;
}

EffectPluginManager & EffectPluginManager::instance()
{
    static EffectPluginManager singleton;
    return singleton;
}
