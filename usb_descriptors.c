/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "tusb.h"
#include "usb_descriptors.h"
#include "class/hid/hid.h"
#include "class/hid/hid_device.h"
#include "class/vendor/vendor_device.h"
#include "device/usbd_pvt.h"


/* A combination of interfaces must have a unique product id, since PC will save device driver after the first plug.
 * Same VID/PID with different interface e.g MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]       MIDI | HID | MSC | CDC          [LSB]
 */
#define _PID_MAP(itf, n)  ( (CFG_TUD_##itf) << (n) )
#define USB_PID           (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
                           _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4) )

int piuio_which_device = MODE_LXIOV1; // See MODE_* in usb_descriptors.h
//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device_piuio =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0100, // at least 2.1 or 3.x for BOS & webUSB

    // Use Interface Association Descriptor (IAD) for CDC
    // As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
    .bDeviceClass       = 0xFF,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE, // TODO: This SHOULD be 8 in PIUIO

    .idVendor           = 0x0547,
    .idProduct          = 0x1002,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x00, // No serial string is defined

    .bNumConfigurations = 0x01
};

tusb_desc_device_t const desc_device_lxiov1 =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0110,

    // MUST be 0 (class defined per interface). The game opens the LXIO through a
    // statically linked hidapi (libusb backend), whose hid_enumerate() skips every
    // device whose bDeviceClass != LIBUSB_CLASS_PER_INTERFACE.
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE, // Original is 16, the game does not care

    .idVendor           = 0x0d2f,
    .idProduct          = 0x1020,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x00, // Original has no serial string

    .bNumConfigurations = 0x01
};

tusb_desc_device_t const desc_device_lxiov2 =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0110,

    // MUST be 0 (class defined per interface). The game opens the LXIO through a
    // statically linked hidapi (libusb backend), whose hid_enumerate() skips every
    // device whose bDeviceClass != LIBUSB_CLASS_PER_INTERFACE.
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE, // Original is 16, the game does not care

    .idVendor           = 0x0d2f,
    .idProduct          = 0x1040,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x00, // Original has no serial string

    .bNumConfigurations = 0x01
};

// NOTE: Keyboard and Joystick use the TinyUSB example VID (0xCAFE) with their own PIDs,
// so the host caches a separate driver binding for each mode.
tusb_desc_device_t const desc_device_keyboard =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00, // Defined at interface level
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0xCAFE,
    .idProduct          = 0x4C01,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x00,

    .bNumConfigurations = 0x01
};

tusb_desc_device_t const desc_device_joystick =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00, // Defined at interface level
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0xCAFE,
    .idProduct          = 0x4C02,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x00,

    .bNumConfigurations = 0x01
};

const tusb_desc_device_t* desc_devices[MODE_COUNT] = {
    [MODE_PIUIO]    = &desc_device_piuio,
    [MODE_LXIOV1]   = &desc_device_lxiov1,
    [MODE_LXIOV2]   = &desc_device_lxiov2,
    [MODE_KEYBOARD] = &desc_device_keyboard,
    [MODE_JOYSTICK] = &desc_device_joystick,
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const * tud_descriptor_device_cb(void)
{
    return (uint8_t const *) desc_devices[piuio_which_device];
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+
enum
{
    ITF_NUM_VENDOR,
    ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN_PIUIO  (TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN)
#define CONFIG_TOTAL_LEN_LXIO   (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN)
#define CONFIG_TOTAL_LEN_HID    (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

#define EPNUM_VENDOR_IN_PIUIO 0x01
#define EPNUM_VENDOR_OUT_PIUIO 0x01

uint8_t const desc_configuration_piuio[] =
{
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN_PIUIO, 0x00, 500),

    // Interface number, string index, EP Out & IN address, EP size
    TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR, 0, EPNUM_VENDOR_OUT_PIUIO, 0x80 | EPNUM_VENDOR_IN_PIUIO,
                            TUD_OPT_HIGH_SPEED ? 512 : 64)
};


uint8_t const desc_hid_report_lxio[] =
{
    TUD_HID_REPORT_DESC_GENERIC_INOUT(16)
};

// Keyboard: modifier byte, reserved byte, KB_REPORT_KEYS keycodes (more than the
// boot-protocol 6 so all 10 panels can be held at once)
uint8_t const desc_hid_report_keyboard[] =
{
    HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     ),
    HID_USAGE      ( HID_USAGE_DESKTOP_KEYBOARD ),
    HID_COLLECTION ( HID_COLLECTION_APPLICATION ),
        // 8 bits Modifier Keys (Shift, Control, Alt)
        HID_USAGE_PAGE ( HID_USAGE_PAGE_KEYBOARD ),
        HID_USAGE_MIN    ( 224                                    ),
        HID_USAGE_MAX    ( 231                                    ),
        HID_LOGICAL_MIN  ( 0                                      ),
        HID_LOGICAL_MAX  ( 1                                      ),
        HID_REPORT_COUNT ( 8                                      ),
        HID_REPORT_SIZE  ( 1                                      ),
        HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
        // 8 bit reserved
        HID_REPORT_COUNT ( 1                                      ),
        HID_REPORT_SIZE  ( 8                                      ),
        HID_INPUT        ( HID_CONSTANT                           ),
        // Keycodes
        HID_USAGE_PAGE   ( HID_USAGE_PAGE_KEYBOARD                ),
        HID_USAGE_MIN    ( 0                                      ),
        HID_USAGE_MAX_N  ( 255, 2                                 ),
        HID_LOGICAL_MIN  ( 0                                      ),
        HID_LOGICAL_MAX_N( 255, 2                                 ),
        HID_REPORT_COUNT ( KB_REPORT_KEYS                         ),
        HID_REPORT_SIZE  ( 8                                      ),
        HID_INPUT        ( HID_DATA | HID_ARRAY | HID_ABSOLUTE    ),
    HID_COLLECTION_END
};

// Joystick: 10 buttons + 6 bits padding, plus X/Y axes always centered
// (some hosts ignore joysticks without any axes)
uint8_t const desc_hid_report_joystick[] =
{
    HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     ),
    HID_USAGE      ( HID_USAGE_DESKTOP_JOYSTICK ),
    HID_COLLECTION ( HID_COLLECTION_APPLICATION ),
        HID_USAGE_PAGE   ( HID_USAGE_PAGE_BUTTON                  ),
        HID_USAGE_MIN    ( 1                                      ),
        HID_USAGE_MAX    ( 10                                     ),
        HID_LOGICAL_MIN  ( 0                                      ),
        HID_LOGICAL_MAX  ( 1                                      ),
        HID_REPORT_COUNT ( 10                                     ),
        HID_REPORT_SIZE  ( 1                                      ),
        HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
        HID_REPORT_COUNT ( 1                                      ),
        HID_REPORT_SIZE  ( 6                                      ),
        HID_INPUT        ( HID_CONSTANT                           ),
        HID_USAGE_PAGE   ( HID_USAGE_PAGE_DESKTOP                 ),
        HID_USAGE        ( HID_USAGE_DESKTOP_X                    ),
        HID_USAGE        ( HID_USAGE_DESKTOP_Y                    ),
        HID_LOGICAL_MIN  ( 0x81                                   ),
        HID_LOGICAL_MAX  ( 0x7f                                   ),
        HID_REPORT_COUNT ( 2                                      ),
        HID_REPORT_SIZE  ( 8                                      ),
        HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
    HID_COLLECTION_END
};

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_hid_descriptor_report_cb(uint8_t itf)
{
    (void) itf;
    switch (piuio_which_device) {
        case MODE_KEYBOARD: return desc_hid_report_keyboard;
        case MODE_JOYSTICK: return desc_hid_report_joystick;
        default:            return desc_hid_report_lxio;
    }
}

uint8_t const desc_configuration_lxio[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN_LXIO, 0x00, 100),

    // HID Input & Output descriptor
    // Interface number, string index, protocol, report descriptor len, EP OUT & IN address, size & polling interval
    TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_VENDOR, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_lxio), EPNUM_VENDOR_OUT_LXIO, 0x80 | EPNUM_VENDOR_IN_LXIO, 16, 1)
};

// Both drivers are always registered: each one only claims interfaces of its own
// class (vendor / HID), so the configuration descriptor alone selects which one
// is active. This lets the mode change on the fly with just a USB disconnect +
// reconnect, without re-initializing the TinyUSB stack.
const usbd_class_driver_t _app_drivers[] = {
    {
        .name             = "VENDOR",
        .init             = vendord_init,
        .deinit           = vendord_deinit,
        .reset            = vendord_reset,
        .open             = vendord_open,
        .control_xfer_cb  = tud_vendor_control_xfer_cb,
        .xfer_cb          = vendord_xfer_cb,
        .sof              = NULL
    },
    {
        .name             = "HID",
        .init             = hidd_init,
        .deinit           = hidd_deinit,
        .reset            = hidd_reset,
        .open             = hidd_open,
        .control_xfer_cb  = hidd_control_xfer_cb,
        .xfer_cb          = hidd_xfer_cb,
        .sof              = NULL
    }
};

// Implement callback to add our custom driver
usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
    *driver_count = sizeof(_app_drivers)/sizeof(_app_drivers[0]);
    return _app_drivers;
}

uint8_t const desc_configuration_keyboard[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN_HID, 0x00, 100),
    // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(ITF_NUM_VENDOR, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_keyboard), 0x80 | EPNUM_VENDOR_IN_LXIO, 16, 1)
};

uint8_t const desc_configuration_joystick[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN_HID, 0x00, 100),
    TUD_HID_DESCRIPTOR(ITF_NUM_VENDOR, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_joystick), 0x80 | EPNUM_VENDOR_IN_LXIO, 16, 1)
};

const uint8_t * const desc_configurations[MODE_COUNT] = {
    [MODE_PIUIO]    = desc_configuration_piuio,
    [MODE_LXIOV1]   = desc_configuration_lxio,
    [MODE_LXIOV2]   = desc_configuration_lxio,
    [MODE_KEYBOARD] = desc_configuration_keyboard,
    [MODE_JOYSTICK] = desc_configuration_joystick,
};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
    (void) index; // for multiple configurations
    return desc_configurations[piuio_which_device];
}


//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
char const* string_desc_arr_piuio [] =
{
    (const char[]) { 0x09, 0x04 },  // 0: is supported language is English (0x0409)
    "Andamirn'to",                  // 1: Manufacturer
    "Andamirn'to PIUIO",            // 2: Product
};

char const* string_desc_arr_lxio [] =
{
    (const char[]) { 0x09, 0x04 },  // 0: is supported language is English (0x0409)
    "ANDAMIRO",                     // 1: Manufacturer
    "PIU HID V1.00",                // 2: Product
};

char const* string_desc_arr_keyboard [] =
{
    (const char[]) { 0x09, 0x04 },  // 0: is supported language is English (0x0409)
    "True LXIO Pico",               // 1: Manufacturer
    "True LXIO Pico Keyboard",      // 2: Product
};

char const* string_desc_arr_joystick [] =
{
    (const char[]) { 0x09, 0x04 },  // 0: is supported language is English (0x0409)
    "True LXIO Pico",               // 1: Manufacturer
    "True LXIO Pico Joystick",      // 2: Product
};

#define ARR_LEN(a) (sizeof(a)/sizeof((a)[0]))

char const** string_desc_arrs [MODE_COUNT] = {
    [MODE_PIUIO]    = string_desc_arr_piuio,
    [MODE_LXIOV1]   = string_desc_arr_lxio,
    [MODE_LXIOV2]   = string_desc_arr_lxio,
    [MODE_KEYBOARD] = string_desc_arr_keyboard,
    [MODE_JOYSTICK] = string_desc_arr_joystick,
};

const size_t string_desc_arr_sizes [MODE_COUNT] = {
    [MODE_PIUIO]    = ARR_LEN(string_desc_arr_piuio),
    [MODE_LXIOV1]   = ARR_LEN(string_desc_arr_lxio),
    [MODE_LXIOV2]   = ARR_LEN(string_desc_arr_lxio),
    [MODE_KEYBOARD] = ARR_LEN(string_desc_arr_keyboard),
    [MODE_JOYSTICK] = ARR_LEN(string_desc_arr_joystick),
};

static uint16_t _desc_str[32];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void) langid;

    uint8_t chr_count;

    if ( index == 0)
    {
        memcpy(&_desc_str[1], string_desc_arrs[piuio_which_device][0], 2);
        chr_count = 1;
    }else
    {
        // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

        if ( !(index < string_desc_arr_sizes[piuio_which_device]) ) return NULL;

        const char* str = string_desc_arrs[piuio_which_device][index];

        // Cap at max char
        chr_count = (uint8_t) strlen(str);
        if ( chr_count > 31 ) chr_count = 31;

        // Convert ASCII string into UTF-16
        for(uint8_t i=0; i<chr_count; i++)
        {
            _desc_str[1+i] = str[i];
        }
    }

    // first byte is length (including header), second byte is string type
    _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8 ) | (2*chr_count + 2));

    return _desc_str;
}
