#ifndef ___PROCESS_WORDS_HH
# define ___PROCESS_WORDS_HH

extern "C" {
# include <ucw/lib.h>
}

#include <boost/mpl/identity.hpp>
#include <boost/intrusive/set.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>

#include "str.hh"
#include "utf.hh"
#include "wordenizer.hh"
#include "fastbuf.hh"

namespace extractor
{
  struct Word : public boost::intrusive::set_base_hook<boost::intrusive::optimize_size<true>>
  {
    unsigned c_;
    __::EStr *w_;

    template<typename Kelpy, typename S>
    Word(Kelpy &kelpy, const S &s, const unsigned len)
    : c_(), w_(__::EStr::create(kelpy, s, len))
    {}

    unsigned add()
    {
      return ++c_;
    }
  };

  struct WordGt : public __::StrGt
  {
    template<typename S>
    bool operator()(const S &s, const Word &w) const
    {
      return __::StrGt::operator()(s, *w.w_);
    }

    template<typename S>
    bool operator()(const Word &w, const S &s) const
    {
      return __::StrGt::operator()(*w.w_, s);
    }

    bool operator()(const Word &w1, const Word &w2)
    {
      return __::StrGt::operator()(*w1.w_, *w2.w_);
    }
  };

  struct WordLt : public __::StrLt
  {
    template<typename S>
    bool operator()(const S &s, const Word &w) const
    {
      return __::StrLt::operator()(s, *w.w_);
    }

    template<typename S>
    bool operator()(const Word &w, const S &s) const
    {
      return __::StrLt::operator()(*w.w_, s);
    }

    bool operator()(const Word &w1, const Word &w2)
    {
      return __::StrLt::operator()(*w1.w_, *w2.w_);
    }
  };

  struct WordEq : public __::StrEq
  {
    template<typename S>
    bool operator()(const S &s, const Word &w) const
    {
      return __::StrEq::operator()(s, *w.w_);
    }

    template<typename S>
    bool operator()(const Word &w, const S &s) const
    {
      return __::StrEq::operator()(*w.w_, s);
    }

    bool operator()(const Word &w1, const Word &w2)
    {
      return __::StrEq::operator()(*w1.w_, *w2.w_);
    }
  };


  struct Utils
  {
    template<typename KelpyT>
    static const char *work_file_name(KelpyT &kelpy, const char *name)
    {
      const pid_t pid = getpid();
      const char *dir_name = mp_printf(kelpy(), "tmp/%ju",  static_cast<uintmax_t>(pid));
      if (mkdir(dir_name, 0700) == -1 && errno != EEXIST)
        die("Cannot create directory: %s : %m", dir_name);
      return mp_printf(kelpy(), "tmp/%ju/%s", static_cast<uintmax_t>(pid), name);
    }
  };

  class CountWords
  {
  private:
    __::Kelpy<__::KelpyFlags::KELPY_FREE_POOL> &kelpy_;
    boost::intrusive::set<Word, boost::intrusive::compare<WordLt>> words_set_;

  private:
    template<typename S>
    void insert_word(const S& w)
    {
      boost::mpl::identity<decltype(words_set_)>::type::insert_commit_data icd;
      auto ret = words_set_.insert_check(w, WordLt(), icd);
      if (ret.second) {
        ret.first = words_set_.insert_commit(*new(mp_alloc_fast(kelpy_(), sizeof(Word))) Word(kelpy_, w.getStr(), w.getLen()), icd);
      }
      ret.first->add();
    }

    void consume_input(__::Fastbuf &input)
    {
      using namespace __;

      Fastbuf bstdout(bfdopen_shared(1, 4096));
      __::Kelpy<__::KelpyFlags::KELPY_FREE_POOL> line_pool;
      for (char *line; (line = bgets_mp(input(), line_pool())); ) {
        Wordenizer wr(line);

        for (Wordenizer::Token tk; wr(tk).t_ != 1<<Wordenizer::END; ) {
          if (tk.t_ == 1<<Wordenizer::ALPHA) {
            CStr lower = __::utf::conv(line_pool, CStr(tk.s_, tk.l_), __::unicode::Lower());
            insert_word(lower);
          } /* if (tk.t == 1<<Wordenizer::ALPHA) */
        } /* for (Wordenizer::Token tk; wr(tk).t != 1<<Wordenizer::END; ) */
      } /* for (char *line; (line = bgets_mp(input_, line_pool())); ) */
    }

    void dump_word_counts()
    {
      __::Fastbuf fb(bopen(Utils::work_file_name(kelpy_, "word-counts"), O_WRONLY | O_CREAT | O_EXCL, 4096));
      for (auto it(words_set_.cbegin()), e(words_set_.cend()); it != e; ++it) {
        bprintf(fb(), "%u\t%s\n", it->c_, it->w_->getStr());
      }
      fb.close();
    }

  public:
    CountWords(__::Kelpy<__::KelpyFlags::KELPY_FREE_POOL> &kelpy)
    : kelpy_(kelpy)
    {}

    CountWords(const CountWords&) = delete;

    void operator()(__::Fastbuf &input)
    {
      consume_input(input);
      dump_word_counts();
    }
  };

  class CountSuffixes
  {
  private:
    __::Kelpy<__::KelpyFlags::KELPY_FREE_POOL> &kelpy_;
    const char *err_;

    char **words_;
    unsigned words_size_;

  private:
    /*constexpr*/ unsigned c_min_word_count() const
    {
      return 50;
    }

    /*constexpr*/ unsigned c_max_suffix_len() const
    {
      return 5;
    }

    /*constexpr*/ unsigned c_min_word_len() const
    {
      return 3;
    }

  private:
    const char *parse_int(char *line, unsigned &v, unsigned &ind)
    {
      v = 0;
      unsigned i;
      for (i = 0; line[i]; ++i) {
        const int c = line[i];
        if (c == '\t')
          break;
        if (c < '0' || c > '9')
          return (err_ = "Unexpexted character while parsing number");
        const unsigned cx = c - '0';
        if (((~0U) - cx)/10 < v)
          return (err_ = "Number out of range");
        v = v*10 + cx;
      }
      ind = i;

      return 0;
    }

    const char *parse_line(char *line, unsigned &v)
    {
      unsigned ind;
      return parse_int(line, v, ind);
    }

    const char *parse_line(char *line, unsigned &v, unsigned &wb, unsigned &we)
    {
      unsigned i;
      if (parse_int(line, v, i))
        return err_;

      if (line[i] != '\t')
        return (err_ = "Expected \\t");

      we = wb = ++i;
      while(line[we] && line[++we] != '\t');

      return 0;
    }

    const char *count_valid_words(__::Fastbuf &fb, unsigned &req_size)
    {
      req_size = 0;

      __::Kelpy<__::KelpyFlags::KELPY_FREE_POOL> line_pool(128);
      char *line;
      while ( (line = bgets_mp(fb(), line_pool())) ) {
        unsigned v = 0;
        if (parse_line(line, v))
          return err_;
        if (v > c_min_word_count())
          req_size++;
        line_pool.flush();
      }

      return 0;
    }

    const char *load_valid_words(__::Fastbuf &fb, const unsigned req_size)
    {
      words_ = static_cast<char**>( mp_alloc_fast(kelpy_(), sizeof(*words_)*req_size) );
      words_size_ = 0;

      __::Kelpy<__::KelpyFlags::KELPY_FREE_POOL> line_pool(128);
      for ( char *linf; (line = bgets_mp(fb(), line_pool())); ) {
        unsigned v = 0;
        unsigned wb, we;
        if (parse_line(line, v, wb, we))
          return err_;
        if (v > c_min_word_count()) {
          const unsigned i = words_size_++;
          if (i >= req_size)
            return (err_ = "File grew unexpectedly");
          char *p = words_[i] = static_cast<char*>( mp_alloc_fast(kelpy_(), we - wb + 1) );
          *static_cast<char*>(mempcpy(p, &line[wb], we - wb)) = 0;
        }
        line_pool.flush();
      }

      return 0;
    }

    unsigned find_chars(char *s, char **p_chs)
    {
      char *ps = s;
      if (!*ps)
        return 0;
      unsigned len = 0;
      unsigned u = 0;
      do { } while ( *(ps = reinterpret_cast<char*>( utf8_get(reinterpret_cast<byte*>(p_chs[len++] = ps), &u) )) );
      p_chs[len] = ps;
      return len;
    }

    const char *count_suffixes_stats()
    {
      for (unsigned i = 0; i < words_size_; ++i) {
        const unsigned usize = strlen(w);
        char *w = &words_[i];
        char *chs[usize + 1];
        const unsigned len = find_chars(w, chs);
        if (len) {
          unsigned j = len;
          do {
            if (j < get_min_word_len())
              break;
            if (i && !memcmp(w, words_[i - 1], chs[j] - w)) // if strlen(words_[i - 1] > chs[j] - w, then we must reach zero-character in words_[i - 1] first
              break;
            unsigned k = 0;
            for (; i + k + 1 < words_size_ && !memcmp(words_[i + k + 1], w_, chs[j] - w); ++k); // if strlen(words_[i + k + 1] > chs[j] - w, then we must reach zero-character in words_[i + k + 1] first
            if (k) {
              const unsigned suff_usize = &w[len] - &w[j];
              if(suff_usize && suff_usize < get_max_suffix_len())
                count_suffix(w, chs[j]);
              for (unsigned l = 0; l < k; ++l) {
                char *w2 = &words_[i + k + 1];
                if (strlen(&w2[chs[j] - w]) < get_max_suffix_len())
                  count_suffix(w2, &w2[chs[j] - w]);
              }
            }
          } while(j--);
        }
      }
    }

  public:
    CountSuffixes(__::Kelpy<__::KelpyFlags::KELPY_FREE_POOL> &kelpy)
    : kelpy_(kelpy), err_(0)
    {}

    CountSuffixes(const CountSuffixes&) = delete;

    const char *operator()()
    {
      __::Fastbuf fb(bopen(Utils::work_file_name(kelpy_, "word-counts"), O_RDONLY, 4096));
      unsigned req_size;
      msg(L_INFO, "Counting number of words with number of outcomes >= %u", c_min_word_count());
      if (count_valid_words(fb, req_size))
        return err_;
      msg(L_INFO, "Resulting number of requested words: %u", req_size);
      brewind(fb());
      if (load_valid_words(fb, req_size))
        return err_;
      for (unsigned i = 0; i < words_size_; ++i) {
        fprintf(stderr, "word:[%s]\n", words_[i]);
      }

      return 0;
    }
  };

  class ProcessWords
  {
  private:
    __::Kelpy<__::KelpyFlags::KELPY_FREE_POOL> &kelpy_;


  public:
    ProcessWords(__::Kelpy<__::KelpyFlags::KELPY_FREE_POOL> &kelpy)
    : kelpy_(kelpy)
    {}

    void operator()(__::Fastbuf &input)
    {
      (CountWords(kelpy_))(input);
      kelpy_.flush();
      const char *err = (CountSuffixes(kelpy_))();
      if (err)
        die("Cannot calculate suffixes: %s", err);
      kelpy_.flush();
    }
  };
}

#endif
