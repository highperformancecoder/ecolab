#include <stdio.h>
int main()
{
  printf("#define SIZEOF_INT %d\n",(int)sizeof(int));
  printf("#define SIZEOF_LONG %d\n",(int)sizeof(long));
#ifdef HAVE_LONGLONG
  printf("#define SIZEOF_LONG_LONG %d\n",(int)sizeof(long long));
#else
  printf("#define SIZEOF_LONG_LONG %d\n",0);
#endif
  return 0;
}
