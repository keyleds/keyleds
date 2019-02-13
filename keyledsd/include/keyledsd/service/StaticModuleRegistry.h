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
#ifndef KEYLEDSD_EFFECT_STATIC_MODULE_REGISTRY_H_B1E3D20E
#define KEYLEDSD_EFFECT_STATIC_MODULE_REGISTRY_H_B1E3D20E

#include <utility>
#include <vector>

namespace keyleds::plugin { struct module_definition; }

namespace keyleds::service {

/****************************************************************************/

class StaticModuleRegistry final
{
    using registration_list = std::vector<std::pair<const char *, const plugin::module_definition *>>;
public:
    struct Registration;
private:
                                StaticModuleRegistry();
                                ~StaticModuleRegistry();
public:
                                StaticModuleRegistry(const StaticModuleRegistry &) = delete;
    static StaticModuleRegistry & instance();

    void                        add(const char *, const plugin::module_definition *);
    const registration_list &   modules() const { return m_modules; }

private:
    registration_list   m_modules;
};


struct StaticModuleRegistry::Registration final
{
    Registration(const char * name, const plugin::module_definition * module)
    {
        StaticModuleRegistry::instance().add(name, module);
    }
};

/****************************************************************************/

} // namespace keyleds::service

#endif
