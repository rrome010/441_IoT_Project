#include "hw_io.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

// HPS-to-FPGA lightweight bridge: 0xFF200000 .. 0xFF300000 (2 MB).
// We only touch the first 64 KB, which is plenty for the PIO blocks below.
#define HW_REGS_BASE  0xFF200000
#define HW_REGS_SPAN  0x10000

// ===== PIO OFFSETS =====
#define LED_PIO_BASE 0x3000
#define SW_PIO_BASE  0x4000
#define KEY_PIO_BASE 0x5000

// ===== GLOBAL STATE =====
static int fd = -1;
static void *lw_virtual_base = NULL;

static volatile uint32_t *led_addr = NULL;
static volatile uint32_t *sw_addr  = NULL;
static volatile uint32_t *key_addr = NULL;

// ===== INIT =====
int hw_init(void)
{
    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem");
        return -1;
    }

    void *virtual_base = mmap(NULL, HW_REGS_SPAN,
                              PROT_READ | PROT_WRITE,
                              MAP_SHARED, fd, HW_REGS_BASE);

    if (virtual_base == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }

    lw_virtual_base = virtual_base;

    led_addr = (volatile uint32_t *)((uint8_t *)lw_virtual_base + LED_PIO_BASE);
    sw_addr  = (volatile uint32_t *)((uint8_t *)lw_virtual_base + SW_PIO_BASE);
    key_addr = (volatile uint32_t *)((uint8_t *)lw_virtual_base + KEY_PIO_BASE);

    return 0;
}

// ===== SHUTDOWN =====
void hw_shutdown(void)
{
    if (lw_virtual_base) {
        munmap(lw_virtual_base, HW_REGS_SPAN);
        lw_virtual_base = NULL;
    }

    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

// ===== WRITE LEDS =====
void hw_write_leds(uint32_t value)
{
    if (led_addr) {
        *led_addr = value;
    }
}

// ===== READ SWITCH =====
int hw_read_switch(int idx)
{
    if (!sw_addr || idx < 0 || idx > 9) return 0;

    uint32_t val = *sw_addr;
    return (val >> idx) & 0x1;
}

// ===== READ BUTTON =====
int hw_read_key(int idx)
{
    if (!key_addr || idx < 0 || idx > 3) return 0;

    uint32_t val = *key_addr;

    // DE10 keys are usually active-low
    return ((val >> idx) & 0x1) ? 0 : 1;
}