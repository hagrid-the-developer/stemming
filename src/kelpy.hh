#ifndef ___KELPY_HH
# define ___KELPY_HH

extern "C" {
# include <ucw/lib.h>
# include <ucw/mempool.h>
}

#include <cstdio>

namespace __
{
  namespace utils {
    struct GuardHelper
    {
    };

    template<bool>
    struct Guard
    {
      Guard(const GuardHelper&)
      {
      }
    };

    template<>
    struct Guard<false>
    {
    };
  }

  enum KelpyFlags
  {
    KELPY_FREE_POOL = 0x1,
  };

  template <unsigned FLAGS>
  class Kelpy
  {
  private:
    struct mempool *mp_;

  public:
    Kelpy(const utils::Guard<!!(FLAGS & KELPY_FREE_POOL)>& = utils::GuardHelper())
    : mp_(mp_new(4096))
    {
    }

    Kelpy(const unsigned size, const utils::Guard<!!(FLAGS & KELPY_FREE_POOL)>& = utils::GuardHelper())
    : mp_(mp_new(size))
    {
    }

    Kelpy(struct mempool *mp)
    : mp_(mp)
    {
      ASSERT(mp_);
    }

    struct mempool *operator()()
    {
      return mp_;
    }

    void flush()
    {
      mp_flush(mp_);
    }

    ~Kelpy()
    {
      if (FLAGS & KELPY_FREE_POOL)
        mp_delete(mp_);
    }
  };
}

#endif
