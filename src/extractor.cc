#include "kelpy.hh"
#include "str.hh"
#include "fastbuf.hh"
#include "process-words.hh"

#include <cstdio>
#include <string>

using namespace extractor;
using namespace __;

int
main(void)
{
  Fastbuf bstdin(bfdopen_shared(0, 4096));

  Kelpy<KELPY_FREE_POOL> kelpy(1*1024*1024);

  (ProcessWords(kelpy))(bstdin);

#if 0
  Kelpy<KELPY_FREE_POOL> kelpy;

  char bs[] = "Kocka.leze dirou";
  Str s(bs);
  Wordenizer wrdnzr(s.get<1>());

  Wordenizer::Token tk;
  while (!(wrdnzr.next(tk).t & 1<<Wordenizer::END)) {
    fprintf(stderr, ">> [%s]\n", tk.s);
  }
#endif

  return 0;
}
