# True LXIO-PICO

This is a Pump it Up IO board (PIUIO/LXIO) clone based on the Raspberry Pi Pico microcontroller.

The build is aimed to switch the PIUIO and the LXIOv1/2 by the pusing of a button.

The original [piuio-pico](https://github.com/48productions/piuio-pico) is originally designed for [dj505's PicoFX handpump controller](https://github.com/dj505/PicoFX).
This repository will continue the original design, only changing the protocol and descriptors.

## Modes
The board can act as one of the following devices. The selected mode is saved in the internal flash.

| Mode | VID:PID | Notes |
|------|---------|-------|
| PIUIO | `0547:1002` | Vendor control transfers |
| LXIOv1 | `0d2f:1020` | HID, 16-byte reports |
| LXIOv2 | `0d2f:1040` | HID, 16-byte reports |
| Keyboard | `cafe:4c01` | Left pad `Q E S Z C`, right pad numpad `7 9 5 1 3` (needs NumLock on) |
| Joystick | `cafe:4c02` | 10 buttons (P1 DL,UL,CN,UR,DR = 1-5, P2 = 6-10) |

To change modes, hold **P1 UL + P2 UL + TEST** and release. The board detaches from USB,
saves the new mode, and re-attaches about half a second later in the new mode (no re-plug needed).
With WS2812 enabled, the LED at the mode's index lights white after a change.

## Hardware Setup
You don't need many components to put one together:
 - Raspberry Pi Pico
 - 12x switches (for the 10 pad switches, plus test and service buttons)
 - 10x LEDs and appropriate resistors

By default the pins are configured as shown below, but the pins can be changed in `piuio_config.h`.
![Schematic](Schematic.png)

## Firmware Setup
[See the Pi Pico SDK repo](https://github.com/raspberrypi/pico-sdk) for instructions on preparing the build environment. All the required CMake files should already be present and ready for compilation.

General overview:
 - Add the SDK via your method of choice (see the pico-sdk repo)
 - Run `git submodule update --init` inside the SDK directory to set up the other required libraries (namely tinyusb)
 - Create the "build" folder inside the "true-lxio-pico" folder.
 - Inside the build folder, run `cmake ..`, then run `make`. You'll get a .UF2 file in the build folder that you can upload to the Pico!
 - With the Pico unplugged from the PC, hold the "BOOTSEL" button on the Pico, then plug in the Pico.
 - The Pico will now show up as a "flash drive" in your OS. Copy the .UF2 file onto the Pico, and the code will be uploaded!

The setup can be executed like the following:
```bash
# In ubuntu, install these
sudo apt install gcc-arm-none-eabi
# The setup
git clone https://github.com/raspberrypi/pico-sdk.git
git clone https://github.com/ckdur/true-lxio-pico.git
export PICO_SDK_PATH=$PWD/pico-sdk
(cd $PICO_SDK_PATH && git submodule update --init)
mkdir -p true-lxio-pico/build
(cd true-lxio-pico/build && cmake .. -DPICO_SDK_PATH=$PICO_SDK_PATH -DENABLE_WS2812_SUPPORT=ON)
make -C true-lxio-pico/build
# Plugin the pico device with BOOTSEL pressed,
# then copy the .UF2 file into the flash device
cp true-lxio-pico/build/true_lxio_pico.uf2 /media/$USER/RPI-RP2
# Test
sudo lsusb -d 0d2f:1020 -v
sudo lsusb -d 0547:1002 -v
```

## PIU Online Notice
Using a hand controller with games connected to the official Pump it Up online service is considered cheating by Andamiro and may lead to actions being taken against your account.
As such, this use case is not officially endorsed by piuio-pico.


## Credits
Credit to [piuio-pico](https://github.com/48productions/piuio-pico) and [dj505's PicoFX handpump controller](https://github.com/dj505/PicoFX).
MIT license is preserved, and modified with the other author.

This project is based off of the [tinyusb device USB examples](https://github.com/hathach/tinyusb/tree/master/examples/device) (specifically webusb_serial and hid_generic_input)

Protocol information from the [PIUIO_Arduino](https://github.com/ckdur/PIUIO_Arduino/) and [piuio_clone](https://github.com/racerxdl/piuio_clone/) repositories.
