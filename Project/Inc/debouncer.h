#ifndef __DEBOUNCER__
#define __DEBOUNCER__

#define PRESS_COUNT 5 // Stable time before registering pressed
#define RELEASE_COUNT 5 // Stable time before registering released

class Debouncer
{
public:
	GPIO_TypeDef * switch_port;
	uint16_t switch_pin;
  bool state;
  bool state_changed;
  int count;

  Debouncer(GPIO_TypeDef * port, uint16_t pin, bool initial_state = false)
  : switch_port(port),
    switch_pin(pin),
    state(initial_state)
  {
    resetCount();
  }

  void resetCount()
  {
    count = (state ? RELEASE_COUNT : PRESS_COUNT);
  }

  // Returns true if state changed.
  bool updateState()
  {
    bool state_change = false;
    bool raw_state = HAL_GPIO_ReadPin(switch_port, switch_pin) == GPIO_PIN_SET;

    if (raw_state == state)
    {
      // Set the timer which allows a change from current state.
      resetCount();
    }
    else
    {
      // Key has changed - wait for new state to become stable.
      if (--count <= 0)
      {
        // Timer expired - accept the change.
        state = raw_state;
        state_change = true;
  
        // And reset the timer.
        resetCount();
      }
    }
    return state_change;
  }
};

#endif
