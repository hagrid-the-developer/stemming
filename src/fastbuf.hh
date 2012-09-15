#ifndef ___FASTBUF_HH
# define ___FASTBUF_HH

extern "C" {
# include <ucw/lib.h>
# include <ucw/fastbuf.h>
}

namespace __
{
  class Fastbuf
  {
    private:
    struct fastbuf *fb_;

    public:
    Fastbuf(struct fastbuf *fb)
    : fb_(fb)
    {
      ASSERT(fb_);
    }

    struct fastbuf* operator()()
    {
      return fb_;
    }

    const struct fastbuf* operator()() const
    {
      return fb_;
    }

    void close()
    {
      if (!fb_)
        return;
      bclose(fb_);
      fb_ = 0;
    }

    ~Fastbuf()
    {
      close();
    }
  };
}

#endif
