/* adapted from https://github.com/ubieda/zephyr_button_debouncing */

#ifndef _BUTTON_H_
#define _BUTTON_H_

#include "zephyr/dt-bindings/gpio/gpio.h"
#include "zephyr/sys/time_units.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>


enum button_evt {
    BUTTON_EVT_PRESSED,
    BUTTON_EVT_RELEASED
};

typedef void (*button_event_handler_t)(enum button_evt, const struct button_context *);

// Context definition for the button. Must not be deleted.
struct button_context {
    const struct device *dev;
    gpio_pin_t pin;
    gpio_flags_t flags;
    struct k_timer debounce_timer;
    struct gpio_callback gpio_cb;
    bool pressed;
    button_event_handler_t user_callback;
};

// Returns true if OK.
bool button_init(struct button_context * button_config, 
				const struct gpio_dt_spec * device,
				button_event_handler_t handler,
                unsigned int options,
                button_event_handler_t user_callback);

#endif /* _BUTTON_H_ */
