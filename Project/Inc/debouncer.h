#ifndef __DEBOUNCER__
#define __DEBOUNCER__

#define PRESS_COUNT 5   // Stable time before registering pressed
#define RELEASE_COUNT 5 // Stable time before registering released

class Debouncer
{
public:
  GPIO_TypeDef *switch_port;
  uint16_t switch_pin;
  bool state;
  bool state_changed;
  int count;

  Debouncer(GPIO_TypeDef *port, uint16_t pin, bool initial_state = false)
      : switch_port(port),
        switch_pin(pin)
  {
    resetCount();
  }

  void resetCount()
  {
    state = HAL_GPIO_ReadPin(switch_port, switch_pin) == GPIO_PIN_SET;
    count = 0;
  }

  // Returns 1 if on, 0 if off or -1 if not yet debounced.
  int updateState()
  {
    bool new_state = HAL_GPIO_ReadPin(switch_port, switch_pin) == GPIO_PIN_SET;

    if (new_state == state)
    {
      if (state && count >= PRESS_COUNT)
        return 1;
      else if (!state && count >= RELEASE_COUNT)
        return 0;
      else
      {
        ++count;
        return -1;
      }
    }
    else
    {
      state = new_state;
      count = 0;
      return -1;
    }
  }
};

#endif
