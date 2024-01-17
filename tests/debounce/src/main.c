
#include "zephyr/dt-bindings/gpio/gpio.h"
#include "zephyr/sys/time_units.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/kernel.h>

#include "button.h"

#define DOOR DT_ALIAS(sw0)
#define CTRL DT_ALIAS(sw1)

const struct gpio_dt_spec door_sw = GPIO_DT_SPEC_GET(DOOR, gpios);
const struct gpio_dt_spec ctrl_sw = GPIO_DT_SPEC_GET(CTRL, gpios);

struct button_context door_sw_context;	// keep it alive !
struct button_context ctrl_sw_context;	// keep it alive !

static void button_event_handler(enum button_evt evt, int)
{
	printk("Button event: %d\n", (int)evt);
}

int main(void)
{
	int err = -1;

	printk("Button Debouncing Sample!\n");

	err = button_init(
		&door_sw_context,
		&door_sw,
		button_event_handler,
		0);

	if (err) {
		printk("Button 1 Init failed: %d\n", err);
		return -1;
	}

	err = button_init(
		&ctrl_sw_context,
		&ctrl_sw,
		button_event_handler,
		GPIO_PULL_DOWN);

	if (err) {
		printk("Button 2 Init failed: %d\n", err);
		return -1;
	}

	printk("Init succeeded. Waiting for event...\n");

  // Main loop
  while (1) {
      k_sleep(K_SECONDS(1)); // Sleep for a while to save power
  }

	return 0;
}
