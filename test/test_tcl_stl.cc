/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "ecolab.h"
#include "TCL_obj_stl.h"
#include "ecolab_epilogue.h"

#include <vector>
#include <deque>
#include <list>
#include <set>
#include <map>

using namespace std;
using namespace ecolab;

vector<int> vec(10);
make_model(vec);

NEWCMD(check_vec,2)
{tclreturn() << (vec[atoi(argv[1])]==atoi(argv[2]));}

deque<int> deq(10);
make_model(deq);

NEWCMD(check_deq,2)
{tclreturn() << (deq[atoi(argv[1])]==atoi(argv[2]));}

list<int> l(10);
make_model(l);

NEWCMD(check_list,2)
{
  int i;
  list<int>::iterator j;
  for (i=0, j=l.begin(); i<atoi(argv[1]); i++, j++);
  tclreturn() << (*j==atoi(argv[2]));
}

set<int> s;
make_model(s);
static int _i=(s.insert(1),s.insert(3),s.insert(5),1);

NEWCMD(check_set,2)
{
  int i;
  set<int>::iterator j;
  for (i=0, j=s.begin(); i<atoi(argv[1]); i++, j++);
  tclreturn() << (*j==atoi(argv[2]));
}

map<int,int> m1;
make_model(m1);

NEWCMD(check_map1,2)
{ tclreturn() << (m1[atoi(argv[1])]==atoi(argv[2]));}

map<std::string,int> m2;
make_model(m2);

NEWCMD(check_map2,2)
{ tclreturn() << (m2[argv[1]]==atoi(argv[2]));}

classdesc::string string1;
make_model(string1);

std::string string2;
make_model(string2);

std::vector<std::string> vstring(2);
make_model(vstring);

typedef std::vector<std::vector<std::string> > VVS;
TCLTYPE(VVS);

VVS vvstring(2);
make_model(vvstring);


typedef map<int,int> MII;
TCLTYPE(MII);

typedef vector<int> VI;
TCLTYPE(VI);

typedef multimap<int,int> MMII;
TCLTYPE(MMII);

typedef multiset<int> MSII;
TCLTYPE(MSII);
