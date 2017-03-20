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
#include <stdint.h>
#include <stdlib.h>

#include "config.h"
#include "keyleds.h"
#include "keyleds/command.h"
#include "keyleds/error.h"


KeyledsCommand * keyleds_command_alloc(uint8_t target_id,
                                       uint8_t feature_idx, uint8_t function,
                                       uint8_t app_id, unsigned length)
{
    KeyledsCommand * cmd = malloc(sizeof(KeyledsCommand) + length);
    if (cmd == NULL) {
        keyleds_set_error_errno();
        return NULL;
    }
    cmd->target_id = target_id;
    cmd->feature_idx = feature_idx;
    cmd->function = function;
    cmd->app_id = app_id;
    cmd->length = length;
    return cmd;
}

KeyledsCommand * keyleds_command_alloc_empty(unsigned length)
{
    KeyledsCommand * cmd = malloc(sizeof(KeyledsCommand) + length);
    if (cmd == NULL) {
        keyleds_set_error_errno();
        return NULL;
    }
    cmd->length = length;
    return cmd;
}

void keyleds_command_free(KeyledsCommand * cmd)
{
    free(cmd);
}
