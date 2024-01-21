// #include <zephyr.h>
// #include <device.h>
// #include <drivers/gpio.h>
// #include <sys/printk.h>

#include "button.h"

#define DEBOUNCE_DELAY K_MSEC(50)  // or 15 ?

// GPIO callback function with user data
static void button_isr_callback(const struct device *dev,
								struct gpio_callback *cb,
								gpio_port_pins_t pins)
{
    struct button_context *btn = CONTAINER_OF(cb, struct button_context, gpio_cb);
	
    // Start or restart the debounce timer associated with the button
    k_timer_start(&btn->debounce_timer, DEBOUNCE_DELAY, K_NO_WAIT);
}

// Timer handler function for debouncing
static void button_handler(struct k_timer *timer_id)
{
    struct button_context *context = CONTAINER_OF(timer_id, struct button_context, debounce_timer);

    // Read the button state to confirm if it's still pressed/released
    int state = gpio_pin_get(context->dev, context->pin);

    // Check if the state has changed since the last interrupt
    if (state != context->pressed) {
        context->pressed = state; // Update the state
    
		if (context->user_callback != NULL)
		{
			context->user_callback(context->pressed, context->info);
		}
	}
}

// TODO remove options
bool button_init(struct button_context * button_config, 
				const struct gpio_dt_spec * device,
				button_event_handler_t user_callback,
				long info,
				unsigned int options)
{
	button_config->pin = device->pin;
	button_config->flags = device->dt_flags;
    button_config->dev = device->port;
	
	if (!button_config->dev) {
	 	printk("Error: Button device %s not found.\n", device);
		return false;
	}

	// Initialize the debounce timer for the button
	k_timer_init(&button_config->debounce_timer, button_handler, NULL);

	// Configure button pin with interrupts
	gpio_pin_configure(button_config->dev, button_config->pin, button_config->flags | GPIO_INPUT | GPIO_INT_EDGE_BOTH | options);

	// Initialize and add the GPIO callback
	gpio_init_callback(&button_config->gpio_cb, button_isr_callback, BIT(button_config->pin));
	gpio_add_callback(button_config->dev, &button_config->gpio_cb);

	// Enable interrupts for the button pin
	gpio_pin_interrupt_configure(button_config->dev, button_config->pin, GPIO_INT_EDGE_BOTH);

	button_config->user_callback = user_callback;
	button_config->info = info;
	return true;
}
