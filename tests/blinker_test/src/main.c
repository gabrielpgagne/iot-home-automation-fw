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
	 gpio_pin_set_dt(&led, evt==blinker_EVT_ON ? 1 : 0);
}

struct blinker_context blinker;
const int n_steps = 2;
unsigned long sequence[] = {1000, 1000};

int main(void)
{
	bool ok;		

	printk("Blinker Sample!\n");
  	gpio_pin_configure_dt(&led, GPIO_OUTPUT);

	ok = blinker_init(
		&blinker,
		sequence,
		n_steps,
		blinker_event_handler,
		0);

	if (!ok) {
		printk("Blink 1 Init failed\n");
		return -1;
	}

	printk("Init succeeded. Waiting for event...\n");

	blinker_start(&blinker, true);

    // Main loop
  	while (1) {
    	k_sleep(K_SECONDS(1)); // Sleep for a while to save power
  	}

	return 0;
}
