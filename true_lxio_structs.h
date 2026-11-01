#ifndef TRUE_LXIO_PICO_PIUIO_STRUCTS_H
#define TRUE_LXIO_PICO_PIUIO_STRUCTS_H

#include <stdint.h>

/*
 * Protocol notes, reverse engineered from the Pump It Up Phoenix 2.12 executable
 * (class "CIOHID" = LXIO, "CIOUSB" = PIUIO). Everything is active high for lamps
 * and active LOW for inputs.
 *
 * Panel bit order used everywhere (inputs and lamps, shifted by 2 on lamps):
 *   bit0 = UL, bit1 = UR, bit2 = CN, bit3 = DL, bit4 = DR
 *
 * LXIO (0d2f:1020, fallback 0d2f:1040), HID, no report ID, 16 byte reports.
 *  The game uses hidapi (libusb backend): interrupt OUT on the first OUT
 *  interrupt endpoint (SET_REPORT(Output) on EP0 if there is none), and a
 *  read thread that keeps an interrupt IN transfer always pending and queues
 *  up to 30 reports. Its IO thread runs at 200 Hz (100 Hz for other IOs); every
 *  tick it writes ONE output report and pops ONE input report (1 ms timeout).
 *
 *  OUT report (host -> device):
 *   [0..3] Same as PIUIO lamp bytes 0..3 (see lampArray), mux bits always 0
 *   [4]    P1 front button lamps (bit = button pressed, same panel order)
 *   [5]    P2 front button lamps
 *   [6..15] 0
 *
 *  IN report (device -> host), active low:
 *   [0..3] P1 sensors, one byte per sensor set (panel order bits 0..4)
 *   [4..7] P2 sensors
 *   [8]    bit1 = test, bit2 = coin 1, bit6 = service, bit7 = clear
 *   [9]    bit2 = coin 2 (the rest is unused by the game)
 *   [10]   P1 front buttons (panel order bits 0..4)
 *   [11]   P2 front buttons
 *   [12..15] unused
 *
 * PIUIO (0547:1002), vendor requests 0xAE, 8 bytes each way. The game does one
 * mux step per tick: OUT (0x40) with the sensor index in bits 0..1 of bytes 0
 * and 2, then IN (0xC0). Bytes 0..3 of the input are P1, sys, P2, sys2 (the
 * same as LX bytes 0, 8, 4, 9).
 */

enum state {
    PLAYER_1,
    CABINET,
    PLAYER_2,
    UNUSED
};

struct lampArray {
    union{
        uint8_t data[8];
        struct {
            // p1 cmd byte 0
            uint8_t p1_mux : 2;
            uint8_t p1_ul_light : 1;
            uint8_t p1_ur_light : 1;
            uint8_t p1_cn_light : 1;
            uint8_t p1_dl_light : 1;
            uint8_t p1_dr_light : 1;
            uint8_t empty1 : 1;

            // p1 cmd byte 1
            uint8_t empty2 : 2;
            uint8_t bass_light : 1;
            uint8_t empty3 : 3;
            uint8_t report : 1;
            uint8_t empty4 : 1;

            // p2 cmd byte 2
            uint8_t p2_mux : 2;
            uint8_t p2_ul_light : 1;
            uint8_t p2_ur_light : 1;
            uint8_t p2_cn_light : 1;
            uint8_t p2_dl_light : 1;
            uint8_t p2_dr_light : 1;
            uint8_t r2_halo : 1;    // The game drives r2+r1 together (mask 0x180 of the high word)

            // p2 cmd byte 3
            uint8_t r1_halo : 1;
            uint8_t l2_halo : 1;    // The game drives l2+l1 together (mask 0x600 of the high word)
            uint8_t l1_halo : 1;
            uint8_t coin_counter : 1; // Pulsed 10 IO ticks on / 10 off per coin inserted
            uint8_t always_on : 1;    // Set at init (0x70 in this byte) and never touched again
            uint8_t coin_ctrl : 2;    // Both bits always driven together by the game (coin related)
            uint8_t empty5 : 1;

            uint32_t empty6 : 32;
        };
    };
};

#endif //TRUE_LXIO_PICO_PIUIO_STRUCTS_H
