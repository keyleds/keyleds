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
#include <string.h>

#include "config.h"
#include "keyleds.h"
#include "keyleds/device.h"
#include "keyleds/logging.h"

/****************************************************************************/
/* This is a partial HID report parser.
 * It misses push/pop support and does not properly associate local items
 * with their described input/outputs.
 * As Logitech hidpp does not use local items, it does not matter.
 */

typedef enum hid_type {
    HID_TYPE_MAIN = 0 << 2,
    HID_TYPE_GLOBAL = 1 << 2,
    HID_TYPE_LOCAL = 2 << 2
} hid_type_t;

typedef enum hid_tag {
    HID_TAG_INPUT = (8 << 4) | HID_TYPE_MAIN,
    HID_TAG_OUTPUT = (9 << 4) | HID_TYPE_MAIN,
    HID_TAG_FEATURE = (11 << 4) | HID_TYPE_MAIN,
    HID_TAG_COLLECTION = (10 << 4) | HID_TYPE_MAIN,
    HID_TAG_END_COLLECTION = (12 << 4) | HID_TYPE_MAIN,

    HID_TAG_USAGE_PAGE = (0 << 4) | HID_TYPE_GLOBAL,
    HID_TAG_LOGICAL_MINIMUM = (1 << 4) | HID_TYPE_GLOBAL,
    HID_TAG_LOGICAL_MAXIMUM = (2 << 4) | HID_TYPE_GLOBAL,
    HID_TAG_PHYSICAL_MINIMUM = (3 << 4) | HID_TYPE_GLOBAL,
    HID_TAG_PHYSICAL_MAXIMUM = (4 << 4) | HID_TYPE_GLOBAL,
    HID_TAG_UNIT_EXPONENT = (5 << 4) | HID_TYPE_GLOBAL,
    HID_TAG_UNIT = (6 << 4) | HID_TYPE_GLOBAL,
    HID_TAG_REPORT_SIZE = (7 << 4) | HID_TYPE_GLOBAL,
    HID_TAG_REPORT_ID = (8 << 4) | HID_TYPE_GLOBAL,
    HID_TAG_REPORT_COUNT = (9 << 4) | HID_TYPE_GLOBAL,
    HID_TAG_PUSH = (10 << 4) | HID_TYPE_GLOBAL,
    HID_TAG_POP = (11 << 4) | HID_TYPE_GLOBAL,

    HID_TAG_USAGE = (0 << 4) | HID_TYPE_LOCAL,

    HID_TAG_INVALID = -1
} hid_tag_t;

#define HID_USAGE_INVALID       ((uint32_t)-1)
#define HID_USAGE_IS_VENDOR(x)  (((x) & 0xff000000) == 0xff000000)

/* Short item description */
struct hid_item {
    hid_type_t  type;
    hid_tag_t   tag;
    unsigned    size;       /* How many bytes in data */
    uint8_t     data[4];    /* Copied as is from report descriptor */
};

/* Reconstructed main item - only a subset is handled */
struct hid_main_item {
    hid_tag_t   tag;
    uint32_t    flags;
    uint32_t    usage;    /* only one usage per item */
    int32_t     logical_minimum;
    int32_t     logical_maximum;
    int32_t     physical_minimum;
    int32_t     physical_maximum;
    int32_t     exponent;
    uint32_t    unit;
    uint8_t     report_id;
    uint32_t    report_size;
    uint32_t    report_count;

    uint32_t    defined;
};

#define HID_REPORT_ID_INVALID        (0x0000)
#define HID_DEFINED_USAGE            (1<<0)
#define HID_DEFINED_LOGICAL_MINIMUM  (1<<1)
#define HID_DEFINED_LOGICAL_MAXIMUM  (1<<2)
#define HID_DEFINED_PHYSICAL_MINIMUM (1<<3)
#define HID_DEFINED_PHYSICAL_MAXIMUM (1<<4)
#define HID_DEFINED_EXPONENT         (1<<5)
#define HID_DEFINED_UNIT             (1<<6)

/****************************************************************************/

/* Dump a hid_main_item for debugging purpose */
#if !defined(NDEBUG) && !defined(S_SPLINT_S)
static void dump_main_item(FILE * stream, const struct hid_main_item * const item)
{
    fprintf(stream, "REPORT item type %x [%04x]\n", item->tag >> 4, item->flags);
    if (item->defined & HID_DEFINED_USAGE) {
        fprintf(stream, "    Usage: %08x\n", item->usage);
    }
    if (item->tag == HID_TAG_INPUT ||
        item->tag == HID_TAG_OUTPUT ||
        item->tag == HID_TAG_FEATURE) {
        fprintf(stream, "    Report ID:          %02x\n", item->report_id);
        fprintf(stream, "    Report size:      %4d\n", item->report_size);
        fprintf(stream, "    Report count:     %4d\n", item->report_count);
        if (item->defined & HID_DEFINED_LOGICAL_MINIMUM) {
            fprintf(stream, "    Logical minimum:  %4d\n", item->logical_minimum);
        }
        if (item->defined & HID_DEFINED_LOGICAL_MAXIMUM) {
            fprintf(stream, "    Logical maximum:  %4d\n", item->logical_maximum);
        }
        if (item->defined & HID_DEFINED_PHYSICAL_MINIMUM) {
            fprintf(stream, "    Physical minimum: %4d\n", item->physical_maximum);
        }
        if (item->defined & HID_DEFINED_PHYSICAL_MAXIMUM) {
            fprintf(stream, "    Physical maximum: %4d\n", item->physical_maximum);
        }
    }
}
#endif

/* Extract a short item data as an unsigned integer */
static uint32_t get_unsigned_integer(const struct hid_item * state)
{
    uint32_t val = 0;
    if (state->size >= 1) { val |= (unsigned)state->data[0]; }
    if (state->size >= 2) { val |= (unsigned)state->data[1] << 8; }
    if (state->size >= 3) { val |= (unsigned)state->data[2] << 16; }
    if (state->size >= 4) { val |= (unsigned)state->data[3] << 24; }
    return val;
}

/* Extract a short item data as a signed integer */
static int32_t get_signed_integer(const struct hid_item * state)
{
    /* Be careful with sign extensions */
    switch (state->size) {
    case 0: return 0;
    case 1: return (int32_t)(int8_t)state->data[0];
    case 2: return (int32_t)(int16_t)(((uint16_t)state->data[1] << 8) |
                                     ((uint16_t)state->data[0] << 0));
    case 4: return (int32_t)(((uint32_t)state->data[3] << 24) |
                             ((uint32_t)state->data[2] << 16) |
                             ((uint32_t)state->data[1] << 8)  |
                             ((uint32_t)state->data[0] << 0));
    }
    abort();
}

static bool aggregate_main_item(const struct hid_item * state, unsigned state_items,
                                /*@out@*/ struct hid_main_item * item)
{
    unsigned idx;
    item->tag       = HID_TAG_INVALID;
    item->usage     = HID_USAGE_INVALID;
    item->report_id = HID_REPORT_ID_INVALID;
    item->report_size = 0;
    item->report_count = 0;
    item->defined = 0;

    for (idx = 0; idx < state_items; idx += 1) {
        if (state[idx].type == HID_TYPE_MAIN) {
            item->tag = state[idx].tag;
            item->flags = get_unsigned_integer(&state[idx]);
        }
        switch(state[idx].tag) {
        case HID_TAG_USAGE_PAGE:
            item->usage = (get_unsigned_integer(&state[idx]) << 16) | (item->usage & 0xffff);
            break;
        case HID_TAG_LOGICAL_MINIMUM:
            item->logical_minimum = get_signed_integer(&state[idx]);
            item->defined |= HID_DEFINED_LOGICAL_MINIMUM;
            break;
        case HID_TAG_LOGICAL_MAXIMUM:
            item->logical_maximum = get_signed_integer(&state[idx]);
            item->defined |= HID_DEFINED_LOGICAL_MAXIMUM;
            break;
        case HID_TAG_PHYSICAL_MINIMUM:
            item->physical_minimum = get_signed_integer(&state[idx]);
            item->defined |= HID_DEFINED_PHYSICAL_MINIMUM;
            break;
        case HID_TAG_PHYSICAL_MAXIMUM:
            item->physical_maximum = get_signed_integer(&state[idx]);
            item->defined |= HID_DEFINED_PHYSICAL_MAXIMUM;
            break;
        case HID_TAG_UNIT_EXPONENT:
            item->exponent = get_signed_integer(&state[idx]);
            item->defined |= HID_DEFINED_EXPONENT;
            break;
        case HID_TAG_UNIT:
            item->unit = get_unsigned_integer(&state[idx]);
            item->defined |= HID_DEFINED_UNIT;
            break;
        case HID_TAG_REPORT_SIZE:
            item->report_size = get_unsigned_integer(&state[idx]);
            break;
        case HID_TAG_REPORT_ID:
            item->report_id = get_unsigned_integer(&state[idx]);
            break;
        case HID_TAG_REPORT_COUNT:
            item->report_count = get_unsigned_integer(&state[idx]);
            break;
        case HID_TAG_USAGE:
            item->usage = (item->usage & (state[idx].size == 4 ? 0 : 0xffff0000))
                        | get_unsigned_integer(&state[idx]);
            item->defined |= HID_DEFINED_USAGE;
            break;
        case HID_TAG_PUSH:
        case HID_TAG_POP:
            KEYLEDS_LOG(ERROR, "REPORT descriptor with push/pop not supported yet");
            return false;
        default:
            break;
        }
    }
#if !defined(NDEBUG) && !defined(S_SPLINT_S)
    if (g_keyleds_debug_hid >= KEYLEDS_LOG_DEBUG) {
        dump_main_item(g_keyleds_debug_stream == NULL ? stderr : g_keyleds_debug_stream, item);
    }
#endif
    return true;
}

static unsigned filter_global_items(struct hid_item * state, unsigned nb_old)
{
    unsigned idx_old, idx_new = 0;

    for (idx_old = 0; idx_old < nb_old; idx_old += 1) {
        if (state[idx_old].type == HID_TYPE_GLOBAL) {
            memcpy(&state[idx_new], &state[idx_old], sizeof(state[0]));
            idx_new += 1;
        }
    }
    return idx_new;
}

/****************************************************************************/

static bool build_main_item_table(const uint8_t * data, const unsigned data_size,
                                 /*@out@*/ struct hid_main_item ** out,
                                  /*@out@*/ unsigned * out_nb)
{
    struct hid_item * state;
    unsigned state_nb = 0, state_capacity = 16;

    struct hid_main_item * main_items;
    unsigned main_nb = 0, main_capacity = 16;

    const uint8_t * current;

    if ((state = malloc(16 * sizeof(struct hid_item))) == NULL) { return false; }
    if ((main_items = malloc(16 * sizeof(struct hid_main_item))) == NULL) {
        free(state);
        return false;
    }

    current = data;
    while (current < data + data_size) {
        unsigned idx;

        /* Decode item prefix */
        hid_type_t type = (hid_type_t)(current[0] & 0x0c);
        hid_tag_t tag = (hid_tag_t)(current[0] & 0xfc);
        unsigned size = (unsigned)(current[0] & 0x03);
        if (size == 3) { size = 4; }

        /* Check for overflow bad usb descriptor */
        if (current + size >= data + data_size) {
            KEYLEDS_LOG(WARNING, "REPORT descriptor item at offset 0x%lx overflows",
                        (current - data) / sizeof(*current));
            break;
        }

        /* Locate item in state table */
        if (type == HID_TYPE_GLOBAL) {
            for (idx = 0; idx < state_nb; idx += 1) {
                if (state[idx].tag == tag) break;
            }
        } else {
            idx = state_nb;
        }

        /* If item does not exist yet, we will add it at the end of table */
        if (idx >= state_nb) {
            if (state_nb >= state_capacity) {
                state = realloc(
                    state,
                    2 * state_capacity * sizeof(state[0])
                );
                state_capacity *= 2;
            }
            state_nb += 1;
        }

        /* Copy item at the chosen place */
        state[idx].type = type;
        state[idx].tag = tag;
        state[idx].size = size;
        memcpy(state[idx].data, &current[1], size * sizeof(current[0]));

        /* Handle main items */
        if (type == HID_TYPE_MAIN) {
            if (main_nb >= main_capacity) {
                main_items = realloc(
                    main_items,
                    2 * main_capacity * sizeof(main_items[0])
                );
                main_capacity *= 2;
            }

            if (aggregate_main_item(state, state_nb, &main_items[main_nb])) {
                main_nb += 1;
            }
            state_nb = filter_global_items(state, state_nb);
        }

        /* Jump to next item, accounting for possible long items */
        current += current[0] == 0xfe ? (unsigned)current[1] + 3 : size + 1;
    }

    *out = main_items;
    *out_nb = main_nb;
    free(state);
    return true;
}

/****************************************************************************/

static int compare_reports(const struct keyleds_device_reports * a,
                           const struct keyleds_device_reports * b)
{
    return a->size < b->size ? -1 : a->size > b->size ? +1 : 0;
}

bool keyleds_parse_hid(const uint8_t * data, const unsigned data_size,
                       struct keyleds_device_reports ** out, unsigned * max_size)
{
    struct hid_main_item * main_items;
    unsigned main_items_nb;
    struct keyleds_device_reports * reports;
    size_t reports_nb = 0, reports_capacity = 4;
    unsigned idx;
    bool result = false;

    assert(data != NULL);
    assert(out != NULL);

    if (!build_main_item_table(data, data_size, &main_items, &main_items_nb)) { return false; }

    if ((reports = malloc(4 * sizeof(struct keyleds_device_reports))) == NULL) {
        goto err_free_main_items;
    }
    reports_capacity = 4;

    for (idx = 0; idx < main_items_nb; idx += 1) {
        if (main_items[idx].tag == HID_TAG_OUTPUT &&
            main_items[idx].logical_minimum == 0 &&
            main_items[idx].logical_maximum == 255 &&
            main_items[idx].flags == 0 &&
            HID_USAGE_IS_VENDOR(main_items[idx].usage)) {
            if (reports_nb + 1 >= reports_capacity) {
                struct keyleds_device_reports * oldptr = reports;
                reports = realloc(reports, 2 * reports_capacity * sizeof(reports[0]));
                if (reports == NULL) {
                    free(oldptr);
                    goto err_free_main_items;
                }
                reports_capacity *= 2;
            }
            reports[reports_nb].id = main_items[idx].report_id;
            reports[reports_nb].size = main_items[idx].report_count
                                     * main_items[idx].report_size / 8;
            KEYLEDS_LOG(DEBUG, "Found report ID %#2x (%d bytes)",
                        reports[reports_nb].id, reports[reports_nb].size);
            reports_nb += 1;
        }
    }
    qsort(reports, reports_nb, sizeof(reports[0]),
          (int (*)(const void*, const void*))compare_reports);
    reports[reports_nb].id = DEVICE_REPORT_INVALID;
    reports[reports_nb].size = 0;

    *max_size = reports_nb > 0 ? reports[reports_nb - 1].size : 0;
    *out = realloc(reports, (reports_nb + 1) * sizeof(reports[0]));
    result = true;

err_free_main_items:
    free(main_items);
    return result;
}

