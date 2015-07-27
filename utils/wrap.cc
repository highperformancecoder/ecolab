/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/* word wrap input to column 80 */
#include <stdio.h>
#include <stdlib.h>
int main()
{
  int c, col=0;

  while ((c=getchar())!=EOF)
    {
      col++;
      if (col>60 && c==' ') 
	{
	  putchar('\n');
	  col=0;
	}
      putchar(c);
    }
return 0;
}
