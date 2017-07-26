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
#include <dlfcn.h>
#include <exception>
#include <iostream>
#include "keyledsd/PluginManager.h"

using keyleds::Renderer;
using keyleds::IRendererPlugin;
using keyleds::RendererPluginManager;

IRendererPlugin::~IRendererPlugin() {}

void RendererPluginManager::registerPlugin(std::string name, IRendererPlugin * plugin)
{
    m_plugins[name] = plugin;
}

IRendererPlugin * RendererPluginManager::get(const std::string & name)
{
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) { return nullptr; }
    return it->second;
}

RendererPluginManager & RendererPluginManager::instance()
{
    static RendererPluginManager singleton;
    return singleton;
}
