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
#ifndef KEYLEDS_FEATURES_H
#define KEYLEDS_FEATURES_H

#define KEYLEDS_FEATURE_ROOT                (0x0000)
#define KEYLEDS_FEATURE_FEATURE             (0x0001)
#define KEYLEDS_FEATURE_VERSION             (0x0003)
#define KEYLEDS_FEATURE_NAME                (0x0005)
#define KEYLEDS_FEATURE_DFU_CONTROL         (0x00c1)
#define KEYLEDS_FEATURE_BATTERY             (0x1000)
#define KEYLEDS_FEATURE_GAMEMODE            (0x4522)
#define KEYLEDS_FEATURE_KEYBOARD_LAYOUT_2   (0x4540)
#define KEYLEDS_FEATURE_REPORTRATE          (0x8060)
#define KEYLEDS_FEATURE_LED_EFFECTS         (0x8070)
#define KEYLEDS_FEATURE_LEDS                (0x8080)

/* Those are hardcoded, always at those indices */
#define KEYLEDS_FEATURE_IDX_ROOT    (0x00)
#define KEYLEDS_FEATURE_IDX_FEATURE (0x01)

#endif
