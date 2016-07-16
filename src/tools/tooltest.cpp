#include <stdio.h>
#include "wrap.h"

void testStore(int type, int *ia, int k, int w)
{
      if (type >= 2)
        wrapStore32(ia, k, w);
      else
        *ia = k;
}

int main(int argc, char *argv[])
{
  int type;
  int *ip = (int*)pmalloc(1024);

  if ((argc != 2) || ((type=atoi(argv[1])) <= 0) || (type > 4))
  {
    printf("Usage: %s type\n", argv[0]);
    printf("type=1 no wraps or wrapped stores or loads\n");
    printf("type=2 no wraps but wrapped ops\n");
    printf("type=3 everything wrapped\n");
    return 1;
  }
  //int type = atoi(argv[1]);

  for (int i = 0; i < 10; i++)
  {
	WRAPTOKEN w;
    if (type > 2)
      w = wrapOpen();
    for (int j = 0; j < 10; j++)
    {
      int k = i*j;
      int *ia = ip + (i*10+j);

      testStore(type, ia, k, w);

      if (type >= 2)
        k = wrapLoad32(ia, w);
      else
        k = *ia;

      if (k != i*j)
        printf("error k!=i*j!\n");
    }
    if (type > 2)
      wrapClose(w);
  }
  return 0;
}

