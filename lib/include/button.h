/* adapted from https://github.com/ubieda/zephyr_button_debouncing */

#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <zephyr/dt-bindings/gpio/gpio.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>


enum button_evt
{
    BUTTON_EVT_RELEASED = 0,
    BUTTON_EVT_PRESSED = 1,
};

typedef void (*button_event_handler_t)(enum button_evt, long info);

// Context definition for the button. Must not be deleted.
struct button_context {
    const struct device *dev;
    gpio_pin_t pin;
    gpio_flags_t flags;
    struct k_timer debounce_timer;
    struct gpio_callback gpio_cb;
    int pressed;    // -1 = unknown.
    button_event_handler_t user_callback;
    long info;
};

bool button_init(struct button_context * button_config, 
                const struct gpio_dt_spec * device,
                button_event_handler_t user_callback,
                long info,
                unsigned int options);

// Simulate a button action. Usefull to init the button.
void button_reset_state(struct button_context * button_config);

#endif /* _BUTTON_H_ */
