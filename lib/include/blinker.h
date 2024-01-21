/* adapted from https://github.com/ubieda/zephyr_button_debouncing */

#ifndef _blinker_H_
#define _blinker_H_

#include "zephyr/dt-bindings/gpio/gpio.h"
#include "zephyr/sys/time_units.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>


enum blinker_evt {
    blinker_EVT_OFF = 0,
    blinker_EVT_ON = 1,
};

typedef void (*blinker_event_handler_t)(enum blinker_evt evt, long info);

// Context definition for the button. Must not be deleted.
struct blinker_context {
    struct k_timer blinker_timer;
    bool repeat_sequence;
    int current_step;
    enum blinker_evt current_state;
    int max_step;
    const unsigned long * steps;
    blinker_event_handler_t user_callback;
    long info;
};

bool blinker_init(struct blinker_context * blinker_config, 
                 const unsigned long * steps,
				 int step_count,
                 blinker_event_handler_t user_callback,
                 long info);

void blinker_start(struct blinker_context * blinker_config, bool repeat_sequence);
void blinker_stop(struct blinker_context * blinker_config, bool wait_end_sequence);

#endif /* _blinker_H_ */
