
#include "bsp/board.h"
#include "device/usbd.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "true_lxio_structs.h"
#include "true_lxio_config.h"
#include "usb_descriptors.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "class/hid/hid.h"
#include "class/hid/hid_device.h"
#include "pico/flash.h"

#ifdef ENABLE_WS2812_SUPPORT
#include "true_lxio_ws2812.h"
#else
unsigned char inactive = 1;
unsigned char dev_changed = 0;
#endif

#define FLASH_TARGET_OFFSET (512 * 1024) // choosing to start at 512K

// From https://forums.raspberrypi.com/viewtopic.php?t=310821
struct MyData {
    int which;
};

struct MyData myData;

static void saveMyDataUnsafe(void* param) {
    (void) param;
    // flash_range_program needs a whole page as source
    static uint8_t page[FLASH_PAGE_SIZE];
    memset(page, 0xFF, sizeof(page));
    memcpy(page, &myData, sizeof(myData));

    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, page, FLASH_PAGE_SIZE);
}

void saveMyData() {
    // Skip the erase/program if nothing changed (saves flash wear)
    if (memcmp((const void*)(XIP_BASE + FLASH_TARGET_OFFSET), &myData, sizeof(myData)) == 0) return;
    // Core 1 (WS2812) also runs from flash, so it must be locked out while the
    // flash is being erased/programmed. flash_safe_execute does that for us.
    flash_safe_execute(saveMyDataUnsafe, NULL, 100);
}

void readBackMyData() {
    const uint8_t* flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
    memcpy(&myData, flash_target_contents, sizeof(myData));
}


const uint8_t pos[] = { 3, 0, 2, 1, 4 }; // don't touch this

// PIUIO input and output data
static uint8_t inputData[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t procInputData[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
struct lampArray lamp = {};

uint8_t LXInputData[16] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t LXLampData[16] = {};

bool tud_vendor_control_xfer_cb_piuio(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request) {
    // Request 0xAE = IO Time
    if (request->bRequest == 0xAE) {
        switch (request->bmRequestType) {
            case 0x40:
                return tud_control_xfer(rhport, request, (void *)&lamp.data, 8);
            case 0xC0:
                return tud_control_xfer(rhport, request, (void *)&inputData, 8);
            default:
                return false;
        }
    }

    return false;
}

bool tud_vendor_control_xfer_cb_lxio(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request) {
    // Control should be handled by the HID at this point
    return false;
}

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request) {
    // nothing to with DATA & ACK stage
    if (stage != CONTROL_STAGE_SETUP) return true;

    if (piuio_which_device == MODE_PIUIO)
        return tud_vendor_control_xfer_cb_piuio(rhport, stage, request);
    
    else
        return tud_vendor_control_xfer_cb_lxio(rhport, stage, request);
}

void handle_lxio_data() {
    // Do the conversion from the Lights and Input from regular IO
    // to the LX Lights and Input format
    // NOTE: Note that InputData will carry the 4 sensor anyways
//#define PIUIO_BUTTON
#ifndef PIUIO_BUTTON
    LXInputData[0] = inputData[0];
    LXInputData[1] = inputData[0];
    LXInputData[2] = inputData[0];
    LXInputData[3] = inputData[0];
    LXInputData[4] = inputData[2];
    LXInputData[5] = inputData[2];
    LXInputData[6] = inputData[2];
    LXInputData[7] = inputData[2];
    LXInputData[8] = inputData[1];
    LXInputData[9] = inputData[3];
    LXInputData[10] = 0xFF;
    LXInputData[11] = 0xFF;
    LXInputData[14] = 0xFF;
    LXInputData[15] = 0xFF;
#else
    LXInputData[0] = 0xFF;
    LXInputData[1] = 0xFF;
    LXInputData[2] = 0xFF;
    LXInputData[3] = 0xFF;
    LXInputData[4] = 0xFF;
    LXInputData[5] = 0xFF;
    LXInputData[6] = 0xFF;
    LXInputData[7] = 0xFF;
    LXInputData[8] = inputData[1];
    LXInputData[9] = inputData[3];
    LXInputData[10] = inputData[0];
    LXInputData[11] = inputData[2];
    LXInputData[14] = 0xFF;
    LXInputData[15] = 0xFF;
#endif

    memcpy(lamp.data, LXLampData, 8); // Just copy in the case of lamps
}

void tud_vendor_rx_cb(uint8_t itf, uint8_t const* buffer, uint16_t bufsize) {
    (void) itf;
    (void) buffer;
    (void) bufsize;
    // Not implemented, but maybe is necessary
}

void tud_vendor_tx_cb(uint8_t itf, uint32_t sent_bytes) {
    (void) itf;
    (void) sent_bytes;
    // Not implemented, but maybe is necessary
}

// Keyboard / Joystick state
// Pad switch bitmap: bit i = pinSwitch[i] pressed (P1 DL,UL,CN,UR,DR, P2 DL,UL,CN,UR,DR)
static uint16_t padPressed = 0;
static uint16_t lastSentPad = 0xFFFF; // Force a first report after (re)connecting

// Left pad: Q E S Z C, right pad: numpad 7 9 5 1 3 (ordered as pinSwitch)
static const uint8_t kbKeymap[10] = {
    HID_KEY_Z,       // P1 DL
    HID_KEY_Q,       // P1 UL
    HID_KEY_S,       // P1 CN
    HID_KEY_E,       // P1 UR
    HID_KEY_C,       // P1 DR
    HID_KEY_KEYPAD_1, // P2 DL
    HID_KEY_KEYPAD_7, // P2 UL
    HID_KEY_KEYPAD_5, // P2 CN
    HID_KEY_KEYPAD_9, // P2 UR
    HID_KEY_KEYPAD_3, // P2 DR
};

static uint16_t build_hid_report(uint8_t* buf, uint16_t maxlen) {
    if (piuio_which_device == MODE_KEYBOARD) {
        uint8_t report[KB_REPORT_LEN] = {0};
        int n = 2;
        for (int i = 0; i < 10 && n < KB_REPORT_LEN; i++)
            if (padPressed & (1u << i)) report[n++] = kbKeymap[i];
        uint16_t len = maxlen < KB_REPORT_LEN ? maxlen : KB_REPORT_LEN;
        memcpy(buf, report, len);
        return len;
    }
    if (piuio_which_device == MODE_JOYSTICK) {
        uint8_t report[JOY_REPORT_LEN] = {
            (uint8_t)(padPressed & 0xFF),
            (uint8_t)((padPressed >> 8) & 0x03),
            0, 0 // X, Y centered
        };
        uint16_t len = maxlen < JOY_REPORT_LEN ? maxlen : JOY_REPORT_LEN;
        memcpy(buf, report, len);
        return len;
    }
    return 0;
}

void tud_mount_cb(void) {
    lastSentPad = 0xFFFF;
}

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len) {
    (void) instance;
    if (!MODE_IS_LXIO(piuio_which_device)) return;

    // Get the LX data
    len = len > 16 ? 16 : len;
    //memcpy(report, LXInputData, len);
    handle_lxio_data();
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    // This example doesn't use multiple report and report ID
    (void) itf;
    (void) report_id;
    (void) report_type;
    if (!MODE_IS_LXIO(piuio_which_device)) return; // Keyboard LEDs etc. are ignored

    // Get the LX data
    bufsize = bufsize > 16 ? 16 : bufsize;
    memcpy(LXLampData, buffer, bufsize);
    handle_lxio_data();
}

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    (void) itf;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    if (!MODE_IS_LXIO(piuio_which_device)) return build_hid_report(buffer, reqlen);

    // Put the LX data
    handle_lxio_data();
    reqlen = reqlen > 16 ? 16 : reqlen;
    memcpy(buffer, LXInputData, reqlen);
    return reqlen;
}

void hid_task(void) {
  if(!MODE_IS_HID(piuio_which_device)) return; // Dont do this on PIUIO
  if(!tud_hid_n_ready(0)) return;

  if (MODE_IS_LXIO(piuio_which_device)) {
    // LXIO streams its input continuously
    handle_lxio_data();
    tud_hid_n_report(0, 0, LXInputData, 16);
  }
  else if (padPressed != lastSentPad) {
    // Keyboard / Joystick only report on changes
    uint8_t report[16];
    uint16_t len = build_hid_report(report, sizeof(report));
    if (tud_hid_n_report(0, 0, report, len)) lastSentPad = padPressed;
  }
}


int next_device = 0;
static int prev_switch;
static int prev_switch_state;
static int switch_notif = 0;

// On-the-fly mode switching: detach from the bus, save the new mode, wait
// long enough for the host to notice the detach, then re-attach so the host
// enumerates the new descriptors. No physical re-plug needed.
#define USB_RECONNECT_DELAY_MS 500
static bool reconnect_pending = false;
static absolute_time_t reconnect_time;

// How long the WS2812 shows the selected mode before returning to normal lamps
#define MODE_INDICATOR_MS 3000
static absolute_time_t mode_indicator_end;

void go_next_device(void) {
    if (reconnect_pending) return;
    dev_changed = 1;
    mode_indicator_end = make_timeout_time_ms(MODE_INDICATOR_MS);
    next_device++;
    if(next_device >= MODE_COUNT) next_device = 0;

    tud_disconnect();

    // Flash write happens while detached, so stalling interrupts can't upset the host
    myData.which = next_device;
    saveMyData();

    reconnect_time = make_timeout_time_ms(USB_RECONNECT_DELAY_MS);
    reconnect_pending = true;
}

void reconnect_task(void) {
    if (dev_changed && !reconnect_pending && time_reached(mode_indicator_end)) dev_changed = 0;
    if (!reconnect_pending || !time_reached(reconnect_time)) return;
    // Descriptors/callbacks follow piuio_which_device, so only swap it while detached
    piuio_which_device = next_device;
    memset(lamp.data, 0, sizeof(lamp.data));
    lastSentPad = 0xFFFF;
    reconnect_pending = false;
    tud_connect();
}

static struct lampArray prevLamp = {};
static uint8_t prevInputData [] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static absolute_time_t last_frame_time;
static absolute_time_t last_inactive_demo;
static absolute_time_t now;
static int inactive_index = 0;

const uint8_t inactiveLamps [] = {
    0x01, 0x01,
    0x02, 0x02,
    0x04, 0x04,
    0x08, 0x08,
    0x10, 0x10,
    0x00, 0x00,
    0x02, 0x02,
    0x01, 0x01,
    0x04, 0x04,
    0x10, 0x10,
    0x08, 0x08,
    0x00, 0x00,
    0x1F, 0x1F,
    0x1F, 0x1F,
    0x00, 0x00,
};

void change_dev_task() {
    // This has a debouncer
    volatile uint32_t* a = (uint32_t*)inputData;
    int this_switch = (*a) == 0xFFFEFDFE ? 1 : 0;
    //int this_switch = 0;
    //if(!(inputData[PLAYER_1] & (1 << pos[1]))) this_switch = 0;
    //if(!(inputData[PLAYER_2] & (1 << pos[1]))) this_switch = 0;
    //if(!(inputData[1] & (1 << 1))) this_switch = 0;

    prev_switch = prev_switch << 1;
    prev_switch |= this_switch;
    prev_switch &= 0xF;
    int cur_switch_state = prev_switch_state;
    if(prev_switch == 0xF) cur_switch_state = 1;
    else if(prev_switch == 0x0) cur_switch_state = 0;
    if(prev_switch_state != cur_switch_state && !cur_switch_state) {
        go_next_device();
        switch_notif = 1;
    }
    if(prev_switch_state != cur_switch_state && cur_switch_state) {
        switch_notif = 0;
    }
    prev_switch_state = cur_switch_state;
}

void piuio_task(void) {
    #ifdef ENABLE_WS2812_SUPPORT
    ws2812_lock_mtx();
    #endif

    #ifdef ENABLE_DEMO
    now = get_absolute_time();
    #endif

    // Raw pad bitmap for Keyboard / Joystick
    uint16_t pressed = 0;
    for (int i = 0; i < 10; i++)
        if(pinSwitch[i] != 255 && !gpio_get(pinSwitch[i])) pressed |= (1u << i);
    padPressed = pressed;

    // P1 / P2 inputs
    memset(procInputData, 0xFF, 8);
    for (int i = 0; i < 5; i++) {
        uint8_t* p1 = &procInputData[PLAYER_1];
        uint8_t* p2 = &procInputData[PLAYER_2];
        if(pinSwitch[i] != 255) *p1 &= ~(((uint8_t) !gpio_get(pinSwitch[i])) << pos[i]); //*p1 =  ? tu_bit_set(*p1, pos[i]) : tu_bit_clear(*p1, pos[i]);
        if(pinSwitch[i+5] != 255) *p2 &= ~(((uint8_t) !gpio_get(pinSwitch[i+5])) << pos[i]); // *p2 = gpio_get(pinSwitch[i+5]) ? tu_bit_set(*p2, pos[i]) : tu_bit_clear(*p2, pos[i]);
    }

    // Test/Service buttons
    if(pinSwitch[10] != 255) procInputData[1] &= ~(((uint8_t) !gpio_get(pinSwitch[10])) << 1); // procInputData[1] = gpio_get(pinSwitch[10]) ? tu_bit_set(procInputData[1], 1) : tu_bit_clear(procInputData[1], 1);
    if(pinSwitch[11] != 255) procInputData[1] &= ~(((uint8_t) !gpio_get(pinSwitch[11])) << 6); // procInputData[1] = gpio_get(pinSwitch[11]) ? tu_bit_set(procInputData[1], 6) : tu_bit_clear(procInputData[1], 6);
    if(pinSwitch[12] != 255) procInputData[1] &= ~(((uint8_t) !gpio_get(pinSwitch[12])) << 2); //procInputData[1] = gpio_get(pinSwitch[12]) ? tu_bit_set(procInputData[1], 2) : tu_bit_clear(procInputData[1], 2);
    if(pinSwitch[13] != 255) procInputData[3] &= ~(((uint8_t) !gpio_get(pinSwitch[13])) << 2); //procInputData[3] = gpio_get(pinSwitch[13]) ? tu_bit_set(procInputData[3], 2) : tu_bit_clear(procInputData[3], 2);

    for(int i =0; i < 8; i++) {
        inputData[i] = procInputData[i];
    }

    // Keyboard / Joystick have no host-driven lamps: light each pad while pressed
    if(piuio_which_device == MODE_KEYBOARD || piuio_which_device == MODE_JOYSTICK) {
        lamp.data[PLAYER_1] = (uint8_t)((~procInputData[PLAYER_1] & 0x1F) << 2);
        lamp.data[PLAYER_2] = (uint8_t)((~procInputData[PLAYER_2] & 0x1F) << 2);
    }

    // Write pad lamps
#ifdef ENABLE_DEMO
    if(inactive) {
        if(absolute_time_diff_us(last_inactive_demo, now) >= 250000) {
            inactive_index+=2;
            if(inactive_index >= (sizeof(inactiveLamps)/sizeof(uint8_t))) inactive_index=0;
            last_inactive_demo = now;
        }
        for (int i = 0; i < 5; i++) {
            if(pinLED[i] != 255) gpio_put(pinLED[i], tu_bit_test(inactiveLamps[inactive_index], pos[i]));
            if(pinLED[i+5] != 255) gpio_put(pinLED[i+5], tu_bit_test(inactiveLamps[inactive_index+1], pos[i]));
        }
    }
    else 
#endif
    {
        for (int i = 0; i < 5; i++) {
            if(pinLED[i] != 255) gpio_put(pinLED[i], tu_bit_test(lamp.data[PLAYER_1], pos[i] + 2));
            if(pinLED[i+5] != 255) gpio_put(pinLED[i+5], tu_bit_test(lamp.data[PLAYER_2], pos[i] + 2));
        }
    }

    // Write the bass neon to the onboard LED for testing + kicks
    gpio_put(pinled, lamp.bass_light | switch_notif);

#ifdef ENABLE_DEMO
    if(memcmp(prevLamp.data, lamp.data, sizeof(lamp.data)) != 0 || memcmp(prevInputData, inputData, sizeof(inputData)) != 0) {
        // There are changes. reset time
        last_frame_time = now;
        inactive = 0;
        inactive_index = 0;
    }
    else if(absolute_time_diff_us(last_frame_time, now) >= 10000000 && !inactive) {
        inactive = 1;
        last_inactive_demo = now;
    }

    memcpy(prevLamp.data, lamp.data, sizeof(lamp.data));
    memcpy(prevInputData, inputData, sizeof(inputData));
#endif 

    #ifdef ENABLE_WS2812_SUPPORT
    ws2812_unlock_mtx();
    #endif
}

int main(void) {
    board_init();
    readBackMyData();

    piuio_which_device = (int)myData.which;
    if(piuio_which_device < 0 || piuio_which_device >= MODE_COUNT) piuio_which_device = MODE_LXIOV1;
    next_device = piuio_which_device;

    // Init WS2812B
    #ifdef ENABLE_WS2812_SUPPORT
    ws2812_init(&lamp);
    #endif

    // Set up GPIO pins: Inputs first, then outputs
    for (int i = 0; i < (sizeof(pinSwitch)/sizeof(pinSwitch[0])); i++) if(pinSwitch[i] != 255) {
        gpio_init(pinSwitch[i]);
        gpio_set_dir(pinSwitch[i], false);
        gpio_pull_up(pinSwitch[i]);
    }

    prev_switch = 0;
    for(int i = 1; i < 4; i++) {
        prev_switch |= (prev_switch & 1) << i;
    }
    prev_switch_state = prev_switch == 0xF?1:0;

    for (int i = 0; i < (sizeof(pinLED)/sizeof(pinLED[0])); i++) if(pinLED[i] != 255) {
        gpio_init(pinLED[i]);
        gpio_set_dir(pinLED[i], true);
    }

    gpio_init(pinled);
    gpio_set_dir(pinled, true);

    // init device stack on configured roothub port
    const tusb_rhport_init_t rh_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUD_OPT_HIGH_SPEED ? TUSB_SPEED_HIGH : TUSB_SPEED_FULL
    };
    tud_rhport_init(BOARD_TUD_RHPORT, &rh_init);

    // Main loop
    while (true) {
        tud_task(); // tinyusb device task
        piuio_task();
        change_dev_task();
        reconnect_task();
        hid_task();
    }

    return 0;
}
