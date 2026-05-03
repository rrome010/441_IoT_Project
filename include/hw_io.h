#ifndef SMART_HOME_HW_IO_H
#define SMART_HOME_HW_IO_H

#include <stdint.h>

int  hw_init(void);
void hw_shutdown(void);

int  hw_read_switch(int sw_num);
int  hw_read_key(int key_num);
void hw_write_leds(uint32_t value);

#endif