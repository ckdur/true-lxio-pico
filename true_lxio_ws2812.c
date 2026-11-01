
#include "true_lxio_config.h"

#ifdef ENABLE_WS2812_SUPPORT
#include "true_lxio_ws2812.h"
#include "true_lxio_ws2812_helpers.h"

#include "ws2812.pio.h"
#include "pico/multicore.h"
#include "pico/flash.h"

mutex_t mutex;
semaphore_t sem;

static struct lampArray* lamp;

uint32_t wheel(uint8_t pos) {
    pos = 255 - pos;
    if (pos < 85) {
        return urgb_u32(255 - pos * 3, 0, pos * 3);
    }
    if (pos < 170) {
        pos -= 85;
        return urgb_u32(0, pos * 3, 255 - pos * 3);
    }
    pos -= 170;
    return urgb_u32(pos * 3, 255 - pos * 3, 0);
}

uint8_t offset_color = 0;
unsigned char inactive = 1;
unsigned char dev_changed = 0;
extern unsigned int next_device;

void ws2812_update() {
    if(dev_changed) {
        put_pixel(next_device == 0 ? urgb_u32(255, 255, 255) : urgb_u32(0, 0, 0));
        put_pixel(next_device == 1 ? urgb_u32(255, 255, 255) : urgb_u32(0, 0, 0));
        put_pixel(next_device == 2 ? urgb_u32(255, 255, 255) : urgb_u32(0, 0, 0));
        put_pixel(next_device == 3 ? urgb_u32(255, 255, 255) : urgb_u32(0, 0, 0));
        put_pixel(next_device == 4 ? urgb_u32(255, 255, 255) : urgb_u32(0, 0, 0));
        put_pixel(next_device == 5 ? urgb_u32(255, 255, 255) : urgb_u32(0, 0, 0));
    }
    else
#ifdef ENABLE_DEMO
    if(inactive) {
        for (int i = 0; i < 6; i++) {
            put_pixel(wheel(offset_color + i * 20));
        }
        offset_color++;
    }
    else 
#endif
    {
        // Write lamp.data to WS2812Bs
        put_pixel(lamp->l1_halo ? ws2812_color[0] : urgb_u32(0, 0, 0));
        put_pixel(lamp->l2_halo ? ws2812_color[1] : urgb_u32(0, 0, 0));
        for (int i = 0; i < 2; i++)
            put_pixel(lamp->bass_light ? ws2812_color[2] : urgb_u32(0, 0, 0));
        put_pixel(lamp->r2_halo ? ws2812_color[3] : urgb_u32(0, 0, 0));
        put_pixel(lamp->r1_halo ? ws2812_color[4] : urgb_u32(0, 0, 0));
    }

    
}

void ws2812_core1() {
    // Allow core 0 to pause this core while it writes the flash (mode save)
    flash_safe_execute_core_init();
    while (true) {
        ws2812_lock_mtx();
        ws2812_update();
        ws2812_unlock_mtx();
        sleep_ms(5);
    }
}

void ws2812_lock_mtx() {
    uint32_t owner = 0;

    sem_release(&sem);
    if (!mutex_try_enter(&mutex, &owner)) {
        mutex_enter_blocking(&mutex);
    }
}

void ws2812_unlock_mtx() {
    mutex_exit(&mutex);
}

void ws2812_init(struct lampArray* l) {
    mutex_init(&mutex);
    sem_init(&sem, 0, 2);
    lamp = l;

    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, WS2812_IS_RGBW);

    multicore_launch_core1(ws2812_core1);
}
#endif