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
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "config.h"
#include "keyleds.h"
#include "keyleds/device.h"
#include "keyleds/error.h"
#include "keyleds/features.h"
#include "keyleds/logging.h"

enum reportrate_feature_function {
    F_GET_SUPPORTED_RATES = 0,
    F_GET_REPORT_RATE = 1,
    F_SET_REPORT_RATE = 2
};

bool keyleds_get_reportrates(Keyleds * device, uint8_t target_id, unsigned ** out)
{
    uint8_t data[1];
    unsigned length, idx, rate_idx;
    unsigned * rates;

    assert(device != NULL);
    assert(out != NULL);

    if (keyleds_call(device, data, (unsigned)sizeof(data),
                     target_id, KEYLEDS_FEATURE_REPORTRATE, F_GET_SUPPORTED_RATES, 0, NULL) < 0) {
        return false;
    }

    length = 0;
    for (idx = 0; idx < 8; idx += 1) {
        if ((data[0] & (1 << idx)) != 0) { length += 1; }
    }

    rates = malloc((length + 1) * sizeof(rates[0]));
    if (rates == NULL) { keyleds_set_error_errno(); return false; }

    rate_idx = 0;
    for (idx = 0; idx < 8; idx += 1) {
        unsigned rate = 1 << idx;
        if ((data[0] & rate) != 0) {
            rates[rate_idx] = idx + 1;
            rate_idx += 1;
        }
    }
    rates[length] = 0;
    *out = rates;
    return true;
}

bool keyleds_get_reportrate(Keyleds * device, uint8_t target_id, unsigned * rate)
{
    uint8_t data[1];

    assert(device != NULL);
    assert(rate != NULL);

    if (keyleds_call(device, data, (unsigned)sizeof(data),
                     target_id, KEYLEDS_FEATURE_REPORTRATE, F_GET_REPORT_RATE, 0, NULL) < 0) {
        return false;
    }
    *rate = (unsigned)data[0];
    return true;
}

bool keyleds_set_reportrate(Keyleds * device, uint8_t target_id, unsigned rate)
{
    if (keyleds_call(device, NULL, 0,
                     target_id, KEYLEDS_FEATURE_REPORTRATE, F_SET_REPORT_RATE,
                     1, (uint8_t[]){rate}) < 0) {
        return false;
    }
    return true;
}
