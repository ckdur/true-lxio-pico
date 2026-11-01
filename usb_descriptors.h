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
 */

#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

enum
{
    REPORT_ID_KEYBOARD = 1,
    REPORT_ID_MOUSE,
    REPORT_ID_CONSUMER_CONTROL,
    REPORT_ID_GAMEPAD,
    REPORT_ID_COUNT
};

// Device modes. Order defines the cycling order of the mode-switch combo.
enum
{
    MODE_PIUIO = 0,
    MODE_LXIOV1,
    MODE_LXIOV2,
    MODE_KEYBOARD,
    MODE_JOYSTICK,
    MODE_COUNT
};

#define MODE_IS_LXIO(m) ((m) == MODE_LXIOV1 || (m) == MODE_LXIOV2)
#define MODE_IS_HID(m)  ((m) != MODE_PIUIO)

// Keyboard report: modifier, reserved, then up to KB_REPORT_KEYS keycodes
#define KB_REPORT_KEYS  10
#define KB_REPORT_LEN   (2 + KB_REPORT_KEYS)
// Joystick report: 10 buttons (2 bytes, padded), X, Y (centered)
#define JOY_REPORT_LEN  4

extern int piuio_which_device;

#define EPNUM_VENDOR_IN_LXIO 0x01
#define EPNUM_VENDOR_OUT_LXIO 0x02

#endif /* USB_DESCRIPTORS_H_ */