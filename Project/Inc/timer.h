#ifndef __TIMER__
#define __TIMER__

class Timer
{
public:
  unsigned long divider_;
  unsigned long divider_count_;
  unsigned long count_;
  bool enabled_;
  
  Timer(unsigned long divider)
  : divider_(divider),
    divider_count_(0),
	count_(0),
    enabled_(false)
    {}

  void start(unsigned long delay_count)
  {
    divider_count_ = divider_;
    count_ = delay_count;
    enabled_ = true;
  }

  void stop()
  {
    enabled_ = false;
    enabled_ = 0;
  }

  // returns true if time elapsed.
  bool update()
  {
    if (enabled_)
    {      
      if (count_ == 0)
      {
        return true;
      }
      else
      {
        if (divider_count_ > 0 && divider_ > 1)
        {
          --divider_count_;
        }
        else
        {
          divider_count_ = divider_;

          if (count_ > 0)
            --count_;
         }
       }

       return false;
    }
    else
    {
      return false;
    }
  }
};
#endif
