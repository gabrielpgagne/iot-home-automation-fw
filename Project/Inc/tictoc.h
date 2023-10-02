#ifndef __TICTOC__
#define __TICTOC__

class TicToc
{
protected:
  uint32_t start;

public:
  TicToc() : start(0) {}

  void tic()
  {
    start = HAL_GetTick();
  }

  unsigned long toc()
  {
	  uint32_t now = HAL_GetTick();
	  uint32_t elapsed;
    if (now < start)
    {
      // There was a wrap-around.
      // We saturate the value.
      const uint32_t SATURATION_VAL = 0xFFFFFFFF;
      uint32_t offset = (SATURATION_VAL - start);
  
//      if (now >= SATURATION_VAL || offset >= offset)
//        elapsed = SATURATION_VAL;
//      else
        elapsed = now + offset + 1;
    }
    else
    {
      elapsed = now - start;
    }

    return elapsed;
  }

  void wait(unsigned long wait_delay, bool hardware_delay=true)
  {
    unsigned long elapsed = toc();
    if (elapsed < wait_delay)
    {
      unsigned long delay = wait_delay-elapsed;
      if (hardware_delay && delay>0)
      {
    	  HAL_Delay(delay);
      }
      else
      {
        while (toc() < wait_delay)
        {;}
      }
    }
  }
};

#endif
