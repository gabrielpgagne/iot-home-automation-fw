#ifndef __TICTOC__
#define __TICTOC__

class TicToc
{
protected:
  unsigned long start;

public:
  TicToc() : start(0) {}

  void tic()
  {
    start = HAL_GetTick();
  }

  unsigned long toc()
  {
    unsigned long now = HAL_GetTick();
    unsigned long elapsed;
    if (now < start)
    {
      // There was a wrap-around.
      // We saturate the value.
      const unsigned long SATURATION_VAL = 0x0FFFFFFFUL;
      unsigned long offset = (0xFFFFFFFFUL - start);
  
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

  void wait(unsigned long wait_delay)
  {
    unsigned long elapsed = toc();
    if (elapsed < wait_delay)
    {
//      if (hardware_delay)
//      {
//        delay(max(1, wait_delay-elapsed));
//      }
//      else
//      {
        while (toc() < wait_delay)
        {;}
//      }
    }
  }
};

#endif
