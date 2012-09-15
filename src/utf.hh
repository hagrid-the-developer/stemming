#ifndef __UTF_HH
# define __UTF_HH

extern "C" {
# include "ucw/lib.h"
# include "ucw/mempool.h"
# include "ucw/unicode.h"
# include "charset/unicat.h"
}

namespace __
{
  namespace unicode
  {
    struct Lower
    {
      unsigned operator()(const unsigned u) const
      {
        return Utolower(u);
      }
    };

    struct Unaccent
    {
      unsigned operator()(const unsigned u) const
      {
        return Uunaccent(u);
      }
    };
  }

  namespace utf
  {
    template<typename Kelpy, typename S, typename Conv>
    Str conv(Kelpy &kelpy, const S &s, const Conv &conv)
    {
      char *dst = static_cast<char*>( mp_alloc_fast(kelpy(), 3*s.getLen() + 1) );
      char *pd = dst;
      for (const char *ps = s.getStr(); *ps; ) {
        uns u;
        ps = reinterpret_cast<const char*>( utf8_get(reinterpret_cast<const byte*>(ps), &u) );

        const uns v = conv(u);
        pd = reinterpret_cast<char*>( utf8_put(reinterpret_cast<byte*>(pd), v) );
      }
      const unsigned dlen = pd - dst;
      *pd = 0;

      return Str(dst, dlen);
    }
  }
}

#endif
