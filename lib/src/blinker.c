#include <assert.h>



#include "blinker.h"

// Abort immediatly the current blink
static void blinker_abort(struct blinker_context *context)
{
    assert(context != NULL);

    if (k_timer_status_get(&context->blinker_timer) < 0)
    {
        // A timer is running.
        k_timer_stop(&context->blinker_timer);
        k_timer_status_sync(&context->blinker_timer);
    }

    if (context->user_callback)
    {
        context->user_callback(BLINKER_EVT_STOP, context->info);
    }
}

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
    context->current_state = new_state ? BLINKER_EVT_OFF : BLINKER_EVT_ON;

    if (context->user_callback)
    {
        context->user_callback(context->current_state, context->info);
    }

    if (context->repeat_sequence || context->current_step < (context->max_step - 1))
    {
        k_timer_start(&context->blinker_timer, K_MSEC(context->steps[context->current_step]), K_NO_WAIT);
    }
    else
    {
        // We stop: do not renew the timer.
        if (context->user_callback)
        {
            context->user_callback(BLINKER_EVT_STOP, context->info);
        }
    }
}

bool blinker_init(struct blinker_context *blinker_config,
                  blinker_event_handler_t user_callback,
                  long info)
{
    k_timer_init(&blinker_config->blinker_timer, blinker_handler, NULL);
    // TODO: check timer ok ?

    blinker_config->repeat_sequence = false;
    blinker_config->current_step = -1;
    blinker_config->current_step = BLINKER_EVT_OFF;
    blinker_config->max_step = 0;
    blinker_config->user_callback = user_callback;
    blinker_config->info = info;

    return true;
}

bool blinker_start(struct blinker_context *blinker_config, bool repeat_sequence)
{
    // Check if the sequence was defined.
    if (blinker_config->max_step <= 0)
    {
        // One of the blinker_sequence function must be called
        // before trying to start the blink.
        return false;
    }

    blinker_abort(blinker_config);

    blinker_config->repeat_sequence = repeat_sequence;
    blinker_config->current_step = blinker_config->max_step - 1;

    if (blinker_config->user_callback)
    {
        blinker_config->user_callback(BLINKER_EVT_START, blinker_config->info);
    }

    blinker_handler(&blinker_config->blinker_timer);
    return true;
}

void blinker_stop(struct blinker_context *blinker_config, bool wait_end_sequence)
{
    blinker_config->repeat_sequence = false;
    if (wait_end_sequence)
    {
        // do something
    }
    else
    {
        blinker_abort(blinker_config);
    }
}

void blinker_sequence1(struct blinker_context *blinker_config,
                       long on1, long off1)
{
    assert(on1 > 0 && off1 > 0);

    blinker_abort(blinker_config);

    blinker_config->steps[0] = on1;
    blinker_config->steps[1] = off1;
    blinker_config->max_step = 2;
}

void blinker_sequence2(struct blinker_context *blinker_config,
                       long on1, long off1,
                       long on2, long off2)
{
    assert(on1 > 0 && off1 > 0);
    assert(on2 > 0 && off2 > 0);

    blinker_abort(blinker_config);

    blinker_config->steps[0] = on1;
    blinker_config->steps[1] = off1;
    blinker_config->steps[2] = on2;
    blinker_config->steps[3] = off2;
    blinker_config->max_step = 4;
}

void blinker_sequence3(struct blinker_context *blinker_config,
                       long on1, long off1,
                       long on2, long off2,
                       long on3, long off3)
{
    assert(on1 > 0 && off1 > 0);
    assert(on2 > 0 && off2 > 0);
    assert(on3 > 0 && off3 > 0);

    blinker_abort(blinker_config);

    blinker_config->steps[0] = on1;
    blinker_config->steps[1] = off1;
    blinker_config->steps[2] = on2;
    blinker_config->steps[3] = off2;
    blinker_config->steps[4] = on3;
    blinker_config->steps[5] = off3;
    blinker_config->max_step = 6;
}