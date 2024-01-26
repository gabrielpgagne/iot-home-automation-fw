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
    BLINKER_EVT_OFF = 0,
    BLINKER_EVT_ON = 1,
    BLINKER_EVT_STOP,
    BLINKER_EVT_START,
};

typedef void (*blinker_event_handler_t)(enum blinker_evt evt, long info);

// Context definition for the blinker. Must not be deleted.
// If sequence not defined, max_step==-1.
struct blinker_context {
    struct k_timer blinker_timer;
    bool repeat_sequence;
    int current_step;
    enum blinker_evt current_state;
    int max_step;
    unsigned long steps[6];
    blinker_event_handler_t user_callback;
    long info;
};

bool blinker_init(struct blinker_context * blinker_config,
                  blinker_event_handler_t user_callback,
                  long info);


bool blinker_start(struct blinker_context * blinker_config,
                   bool repeat_sequence);

void blinker_stop(struct blinker_context * blinker_config,
                  bool wait_end_sequence);

void blinker_sequence0(struct blinker_context * blinker_config,
                       long on1);

void blinker_sequence2(struct blinker_context * blinker_config,
                       long on1, long off1);

void blinker_sequence4(struct blinker_context * blinker_config,
                       long on1, long off1,
                       long on2, long off2);

void blinker_sequence6(struct blinker_context * blinker_config,
                       long on1, long off1,
                       long on2, long off2,
                       long on3, long off3);


#endif /* _blinker_H_ */
