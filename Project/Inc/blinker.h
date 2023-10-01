#ifndef __BLINKER__
#define __BLINKER__

class Blinker
{
public:
	GPIO_TypeDef * output_port;
	uint16_t output_pin;

  int delay_on;
  int delay_off;
  int count;

  // 0 = disabled, 1 = pin high, -1 = pin low.
  int enabled;
  
  Blinker(GPIO_TypeDef * port, uint16_t pin, int on_time, int off_time)
  : output_port(port),
	output_pin(pin),
    delay_on(on_time),
    delay_off(off_time),
    count(0),
    enabled(0)
    {}

  void start(int on_time=-1, int off_time=-1)
  {
	  if (on_time>=0)
		  delay_on = on_time;

	  if (off_time>=0)
		  delay_off = off_time;

    enabled = 1;
    count = delay_on;

    HAL_GPIO_WritePin(output_port, output_pin, GPIO_PIN_SET);
  }

  void stop()
  {
    enabled = 0;
    count = 0;

    HAL_GPIO_WritePin(output_port, output_pin, GPIO_PIN_RESET);
  }

  void update()
  {
    if (enabled != 0)
    {
      --count;
      if (count <= 0)
      {
        if (enabled == 1)
        {
          count = delay_off;
          enabled = -1;
        }
        else
        {
          count = delay_on;
          enabled = 1;
        }

        HAL_GPIO_WritePin(output_port, output_pin, enabled==1 ? GPIO_PIN_SET : GPIO_PIN_RESET);
      }
    }
  }
};

#endif
