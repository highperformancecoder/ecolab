#include "vecBitSet.h"
#include <iostream>
using namespace std;

// TODO - convert this to a unit test

void print(VecBitSet<unsigned,4> x)
{
  for (unsigned i=0; i<x.size(); ++i)
    cout<<hex<<x[i]<<" ";
  cout<<endl;
}

int main()
{
  VecBitSet<unsigned,4> v(sycl::vec<unsigned,4>(1,3,7,15));
  auto x=v<<5;
  print(x);
  print(v<<40);
  print(v>>5);
  print(v>>40);
}
