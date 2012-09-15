#ifndef ___STR_HH
# define ___STR_HH

#include "kelpy.hh"

#include <boost/tuple/tuple.hpp>
#include <cstring>
#include <new>

namespace __
{
  class Str : public boost::tuple<unsigned, char*>
  {
  public:
    Str(char *s)
    : boost::tuple<unsigned, char*>(s ? strlen(s) : 0, s)
    {}

    Str(char *s, const unsigned len)
    : boost::tuple<unsigned, char*>(len, s)
    {}


    unsigned getLen() const
    {
        return get<0>();
    }

    char *getStr()
    {
      get<1>();
    }

    const char *getStr() const
    {
      get<1>();
    }
  };

  class CStr : public boost::tuple<unsigned, const char*>
  {
  public:
    CStr(const char *s)
    : boost::tuple<unsigned, const char*>(s ? strlen(s) : 0, s)
    {}

    CStr(const char *s, const unsigned len)
    : boost::tuple<unsigned, const char*>(len, s)
    {}

    CStr(const Str &s)
    : boost::tuple<unsigned, const char*>(s)
    {}

    unsigned getLen() const
    {
      return get<0>();
    }

    const char *getStr() const
    {
      return get<1>();
    }
  };

  class EStr
  {
    private:
    unsigned len_;

    private:
    static size_t size()
    {
      return sizeof(EStr);
    }

    protected:
    EStr(const unsigned len)
    : len_(len)
    {}

    public:
    template<typename Kelpy>
    static EStr* create(Kelpy &kelpy, const char *str, const unsigned len)
    {
      char *mem = static_cast<char*>(mp_alloc_fast(kelpy(), size() + len + 2));
      EStr *es = new(mem) EStr(len);
      *static_cast<char*>(mempcpy(&mem[size()], str, len)) = 0;

      return es;
    }

    unsigned getLen() const
    {
      return len_;
    }

    char* getStr()
    {
      return &(reinterpret_cast<char*>(this))[size()];
    }

    const char* getStr() const
    {
      return &(reinterpret_cast<const char*>(this))[size()];
    }
  };

  struct StrGt
  {
    bool operator()(const char *s1, const unsigned s1_len, const char *s2, const unsigned s2_len) const
    {
      const unsigned len = s1_len < s2_len ? s1_len : s2_len;
      const int cmp = len ? memcmp(s1, s2, len) : 0;
      if (cmp > 0)
        return true;
      if (!cmp)
        return s1_len > s2_len;
      return false;
    }

    template<typename S1, typename S2>
    bool operator()(const S1 &s1, const S2 &s2) const
    {
      this->operator()(s1.getStr(), s1.getLen(), s2.getStr(), s2.getLen());
    }
  };

  struct StrLt
  {
    bool operator()(const char *s1, const unsigned s1_len, const char *s2, const unsigned s2_len) const
    {
      const unsigned len = s1_len < s2_len ? s1_len : s2_len;
      const int cmp = len ? memcmp(s1, s2, len) : 0;
      if (cmp < 0)
        return true;
      if (!cmp)
        return s1_len < s2_len;
      return false;
    }

    template<typename S1, typename S2>
    bool operator()(const S1 &s1, const S2 &s2) const
    {
      this->operator()(s1.getStr(), s1.getLen(), s2.getStr(), s2.getLen());
    }
  };

  struct StrEq
  {
    bool operator()(const char *s1, const unsigned s1_len, const char *s2, const unsigned s2_len) const
    {
      const unsigned len = s1_len;
      if (len != s2_len)
        return false;
      return len ? !memcmp(s1, s2, len) : true;
    }

    template<typename S1, typename S2>
    bool operator()(const S1 &s1, const S2 &s2) const
    {
      this->operator()(s1.getStr(), s1.getLen(), s2.getStr(), s2.getLen());
    }
  };
}

#endif
