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

using keyleds::KeyDatabase;
using keyleds::EffectPlugin;
using keyleds::EffectPluginFactory;
using keyleds::EffectPluginManager;

EffectPlugin::~EffectPlugin() {}
void EffectPlugin::handleKeyEvent(const KeyDatabase::Key &, bool) {}

EffectPluginFactory::~EffectPluginFactory() {}

void EffectPluginManager::registerPlugin(std::string name, EffectPluginFactory * plugin)
{
    m_plugins[name] = plugin;
}

EffectPluginFactory * EffectPluginManager::get(const std::string & name)
{
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) { return nullptr; }
    return it->second;
}

EffectPluginManager & EffectPluginManager::instance()
{
    static EffectPluginManager singleton;
    return singleton;
}
