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
