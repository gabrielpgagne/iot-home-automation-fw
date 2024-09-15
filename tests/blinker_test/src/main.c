#include "zephyr/dt-bindings/gpio/gpio.h"
#include "zephyr/sys/time_units.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

//#include <zephyr/logging/log.h>
//LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

#include <blinker.h>

const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(statusled), gpios);

static void blinker_event_handler(enum blinker_evt evt, long info)
{
	gpio_pin_set_dt(&led, evt==BLINKER_EVT_OFF ? 1 : 0);
}

struct blinker_context blinker;

int main(void)
{
	bool ok;		

	printk("Blinker Sample!\n");
  	gpio_pin_configure_dt(&led, GPIO_OUTPUT);
	gpio_pin_set_dt(&led, 1);

	ok = blinker_init(
		&blinker,
		blinker_event_handler,
		0);

	if (!ok) {
		printk("Blink 1 Init failed\n");
		return -1;
	}

	printk("Init succeeded. Waiting for event...\n");

	// Sequence defined in ms.
	blinker_sequence2(&blinker, 1000, 500);
	blinker_start(&blinker, true);

    // Main loop
  	while (1) {
		// We can sleep now, the blinker will handle itself.
    	k_sleep(K_SECONDS(10));
  	}

	return 0;
}
