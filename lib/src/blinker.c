#include "blinker.h"

// Timer handler function for debouncing
static void blinker_handler(struct k_timer *timer_id)
{
    struct blinker_context *context = CONTAINER_OF(timer_id, struct blinker_context, blinker_timer);
	++context->current_step;

	if (context->current_step >= context->max_step)
	{
		context->current_step = 0;
	}

	bool new_state = context->current_step & 1;
	context->current_state = new_state ? blinker_EVT_OFF : blinker_EVT_ON;
	
	if (context->user_callback)
	{
		context->user_callback(context->current_state, context->info);
	}

	if (context->repeat_sequence || context->current_step<(context->max_step-1))
	{
		k_timer_start(&context->blinker_timer, K_MSEC(context->steps[context->current_step]), K_NO_WAIT);
	}
	else
	{
		k_timer_stop(&context->blinker_timer);
	}
}

bool blinker_init(struct blinker_context * blinker_config,
				const unsigned long * steps,
				int step_count,
                blinker_event_handler_t user_callback,
				long info)
{
	k_timer_init(&blinker_config->blinker_timer, blinker_handler, NULL);
	blinker_config->repeat_sequence = false;
	blinker_config->current_step = -1;
	blinker_config->current_step = blinker_EVT_OFF;
	blinker_config->max_step = step_count;
	blinker_config->steps = steps;
	blinker_config->user_callback = user_callback;
	blinker_config->info = info;

	return blinker_config->steps != NULL;
}

void blinker_start(struct blinker_context * blinker_config, bool repeat_sequence)
{
	// TODO: Check if already enabled.
	blinker_config->repeat_sequence = repeat_sequence;
	blinker_config->current_step = -1;

	blinker_handler(&blinker_config->blinker_timer);
}

void blinker_stop(struct blinker_context * blinker_config, bool wait_end_sequence)
{
	blinker_config->repeat_sequence = false;
	if (wait_end_sequence)
	{
		// do something
	}
	else
	{
		k_timer_stop(&blinker_config->blinker_timer);
	}
}