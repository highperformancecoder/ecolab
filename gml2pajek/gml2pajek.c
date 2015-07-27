/*
  GML to Pajek converter (use as a filter), using Mark Newman's readgml code
  Russell Standish 25th February 2009.
*/
#include "readgml.h"

int main()
{
  NETWORK net;
  VERTEX *v;
  int i,j;

  read_network(&net,stdin);
  printf("*Vertices %d\r\n",net.nvertices);
  for (i=0; i<net.nvertices; ++i)
    {
      VERTEX* v=net.vertex+i;
      if (v->label)
        printf("%d %s\r\n",i+1,v->label);
      else
        printf("%d %d\r\n",i+1,v->id);
    }

  if (net.directed)
    printf("*Arcs\r\n");
  else
    printf("*Edges\r\n");

  for (i=0; i<net.nvertices; ++i)
    {
      VERTEX* v=net.vertex+i;
      for (j=0; j<v->degree; ++j)
        {
          EDGE* e=v->edge+j;
          printf("%d %d %g\r\n",i+1,e->target+1,e->weight);
        }
    }
}
