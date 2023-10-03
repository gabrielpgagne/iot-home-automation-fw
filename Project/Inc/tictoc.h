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

	uint32_t toc()
	{
		return HAL_GetTick() - start;
	}

  
	void wait(uint32_t wait_delay, bool hardware_delay=true)
	{
		uint32_t elapsed = toc();
		if (elapsed < wait_delay)
		{
			uint32_t delay = wait_delay-elapsed;
			if (delay>0)
			{
				hardware_delay ? sleep(delay) : HAL_Delay(delay);
			}
		}
	}
};

#endif
