/* adapted from https://github.com/ubieda/zephyr_button_debouncing */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "button.h"


static struct gpio_callback button_cb_data;

static button_event_handler_t user_cb;

static void cooldown_expired(struct k_work *work)
{
    ARG_UNUSED(work);

    int val = gpio_pin_get_dt(&button);
    enum button_evt evt = val ? BUTTON_EVT_PRESSED : BUTTON_EVT_RELEASED;
    if (user_cb) {
        user_cb(evt);
    }
}

static K_WORK_DELAYABLE_DEFINE(cooldown_work, cooldown_expired);

/**
void work_handler(struct k_work *work)
{
    int *user_data = (int *)work->user_data;
    printk("User data: %d\n", *user_data);
}

struct k_work_user work;
int user_data = 42;

void main(void)
{
    k_work_user_init(&work, work_handler);
    work.user_data = &user_data;

    k_work_reschedule(&work->work, K_SECONDS(5));
}
*/

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
    k_work_reschedule(&cooldown_work, K_MSEC(15));
}

int button_init(const struct gpio_dt_spec * button, button_event_handler_t handler)
{
    int err = -1;

    if (!handler) {
        return -EINVAL;
    }

    user_cb = handler;

	if (!device_is_ready(button.port)) {
		return -EIO;
	}

	err = gpio_pin_configure_dt(button, GPIO_INPUT);
	if (err) {
        return err;
	}

	err = gpio_pin_interrupt_configure_dt(button, GPIO_INT_EDGE_BOTH);
	if (err) {
		return err;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button->pin));
	err = gpio_add_callback(button->port, &button_cb_data);
    if (err) {
        return err;
    }

    return 0;
}

/*********************************************

#include <zephyr/kernel.h>
#include "button.h"

#define SW0_NODE	DT_ALIAS(sw0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios); 

static char *helper_button_evt_str(enum button_evt evt)
{
	switch (evt) {
	case BUTTON_EVT_PRESSED:
		return "Pressed";
	case BUTTON_EVT_RELEASED:
		return "Released";
	default:
		return "Unknown";
	}
}

static void button_event_handler(enum button_evt evt)
{
	printk("Button event: %s\n", helper_button_evt_str(evt));
}

void main(void)
{
	int err = -1;

	printk("Button Debouncing Sample!\n");

	err = button_init(&button, button_event_handler);
	if (err) {
		printk("Button Init failed: %d\n", err);
		return;
	}

	printk("Init succeeded. Waiting for event...\n");
}

*********************************************/