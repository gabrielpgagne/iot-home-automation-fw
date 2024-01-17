/* adapted from https://github.com/ubieda/zephyr_button_debouncing */

#ifndef _BUTTON_H_
#define _BUTTON_H_

enum button_evt {
    BUTTON_EVT_PRESSED,
    BUTTON_EVT_RELEASED
};

// Context definition for the button. Must not be deleted.
struct button_context {
    const struct device *dev;
    gpio_pin_t pin;
    gpio_flags_t flags;
    struct k_timer debounce_timer;
    struct gpio_callback gpio_cb;
    bool pressed;
};

typedef void (*button_event_handler_t)(enum button_evt evt, int device_alias);

int button_init(struct button_context * button_config, 
				const struct gpio_dt_spec * device,
				button_event_handler_t handler);

#endif /* _BUTTON_H_ */
