/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "cachedDBM.h"
#include "ecolab_epilogue.h"

using namespace ecolab;

void write()
{
  cachedDBM<int,int> ii("ii",'w');
  cachedDBM<char*,int> ci("ci",'w');
  cachedDBM<int,char*> ic("ic",'w');
  cachedDBM<char*,char*> cc("cc",'w');

  ii[1]=3; ii[3]=5; ii[5]=1;
  ci["a"]=3; ci["c"]=5; ci["e"]=1;
  ic[1]="c"; ic[3]="e"; ic[5]="a";
  cc["a"]="c"; cc["c"]="e"; cc["e"]="a";
}

void read()
{
  cachedDBM<int,int> ii("ii",'r');
  cachedDBM<char*,int> ci("ci",'r');
  cachedDBM<int,char*> ic("ic",'r');
  cachedDBM<char*,char*> cc("cc",'r');
  
  assert(ii[1]==3 && ii[3]==5 && ii[5]==1);
  assert(ci["a"]==3 && ci["c"]==5 && ci["e"]==1);
  assert(strcmp(ic[1],"c")==0 && strcmp(ic[3],"e")==0 && strcmp(ic[5],"a")==0);
  assert(strcmp(cc["a"],"c")==0 && 
	 strcmp(cc["c"],"e")==0 && strcmp(cc["e"],"a")==0);
}  

int main()
{
  write();
  read();
  return 0;
}
