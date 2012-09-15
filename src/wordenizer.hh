#ifndef WORD_TOKENIZER_HH
# define WORD_TOKENIZER_HH

extern "C" {
# include <ucw/lib.h>
# include <ucw/unicode.h>
# include <charset/unicat.h>
}

namespace extractor
{
  class Wordenizer
  {
  private:
    char *s_;
    char *pos_;
    char *act_u_pos_;
    uns act_u_;
    uns act_u_type_;
    int saved_c_;

  public:
    enum
    {
      END,
      ALPHA,
      DIGIT,
      SPACE,
      OCH,      /* One Char Token */
      OTHER,

      SKIP_S = 1<<SPACE | 1<<OTHER,
      WORD_S = 1<<DIGIT | 1<<ALPHA,
    };

  private:
    int getCharType(const uns u)
    {
      if (!u)
        return END;

      if (Ualpha(u))
        return ALPHA;

      if (Udigit(u))
        return DIGIT;

      if (Uspace(u))
        return SPACE;

      if (u == '.' || u == ',' || u == ';' || u == '-' || u == ':' || u == '#')
        return OCH;

      return OTHER;
    }

    int getNextChar()
    {
      if (!pos_ || !*pos_)
        return END;

      uns u;
      pos_ = reinterpret_cast<char*>( utf8_get(reinterpret_cast<byte*>(pos_), &u) );
      return u;
    }

    void nextChar()
    {
      act_u_pos_ = pos_;
      act_u_ = getNextChar();
      act_u_type_ = getCharType(act_u_);
    }

  public:
    struct Token
    {
      const char *s_;
      uns l_;
      uns t_;
    };

  public:
    Wordenizer(char *s)
    : s_(s), pos_(s_), saved_c_(0)
    {
      nextChar();
    }

    const Token& operator()(Token &token)
    {
      return next(token);
    }

    const Token& next(Token &token)
    {
      for (; (1<<act_u_type_) & SKIP_S; nextChar());

      char *f = act_u_pos_;
      uns type_s = 1<<act_u_type_;

      if ((1<<act_u_type_) & WORD_S) {
        do { type_s |= 1<<act_u_type_; nextChar(); } while((1<<act_u_type_) & WORD_S);
      }
      else
      if (act_u_type_ == OCH) {
        nextChar();
      }

      char *l = act_u_pos_;

      if (type_s & (1<<OCH | WORD_S)) {
        if (!*f) {
          *f = saved_c_;
        }
        saved_c_ = *l;
        *l = 0;
        token.s_ = f;
        token.l_ = l - f;
      }
      else {
        token.s_ = 0;
      }

      token.t_ = type_s;

      return token;
    }
  };
}

#endif
