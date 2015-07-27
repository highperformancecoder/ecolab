/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include <TCL_obj_stl.h>
using namespace std;

// this shows how to provide a user defined streaming operation to
// allow TCL commands to access a variable
namespace bar
{
  struct foo {};
  
  std::ostream& operator<<(std::ostream& o, const foo& x)
  {return o<<"foo";}
  
}

struct foo
{
  array_ns::array<int> a;
  void seta(TCL_args);
  template <class T>
  T test(TCL_args args) {
    T x;
    args>>x;
    return x;
  }

  bool testBool(TCL_args args) {return test<bool>(args);}
  int testInt(TCL_args args) {return test<int>(args);}
  unsigned testUnsigned(TCL_args args) {return test<unsigned>(args);}
  long testLong(TCL_args args) {return test<long>(args);}
  float testFloat(TCL_args args) {return test<float>(args);}
  float testDouble(TCL_args args) {return test<double>(args);}
  string testString(TCL_args args) {return test<string>(args);}

  enum EnumTest {ff, bar};
  EnumTest testEnum(TCL_args args) {return test<EnumTest>(args);}
 
  vector<int> testVector(TCL_args args) {return test<vector<int> >(args);}
//  map<string,int> imap;
//  void testMap(TCL_args args) {args>>imap;}
  map<string,int> testMap(TCL_args args) {return test<map<string,int> >(args);}

  bar::foo b;
};
