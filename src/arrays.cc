/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "arrays.h"
#include "ecolab_epilogue.h"
#include <vector>
using namespace std;

//void iarray::print() 
//{for (int i=0; i<size; i++) cout << *((double*)list+i) << ' ';}
//void array::print() 
//{for (int i=0; i<size; i++) cout << *((int*)list+i) << ' ';}

#include "random.h"
#include "pack_stream.h"

namespace ecolab
{
  urand array_urand;
  gaussrand array_grand;

  
namespace array_ns
{
  using ::log;
  void fillrand(array<double>& x)
  {
    for (size_t i=0; i<x.size(); i++)
      x[i]=array_urand.rand();
  }

  void fillgrand(array<double>& x)
  {
    for (size_t i=0; i<x.size(); i++)
      x[i]=array_grand.rand();
  }

  void fillprand(array<double>& x)
  {
    for (size_t i=0; i<x.size(); i++)
      x[i]=-log(array_urand.rand());
  }

  void fill_unique_rand(array<int>& x, unsigned max)
  {
    /* if x.size > max/2 draw the results from a shuffled deck,
       otherwise use a bitset to keep track of whether a number has
       already been drawn */
    
    if (x.size()>max/2)
      {
        array<double> v(max); array<int> deck;
        fillrand(v);
        deck=rank(v);
        x=deck[pcoord(x.size())];
      }
    else
      {
        vector<bool> drawn(max);
        size_t i,j;
        for (i=0; i<x.size();)
          {
            j=(max*(double)rand())/RAND_MAX;
            if (!drawn[j])
              {
                drawn[j]=true;
                x[i++]=j;
              }
          }
      }
  }

  void lgspread( array<double>& a, const array<double>& s )
  {
    array<double> gran(a.size());
    fillgrand(gran);
    //  a *= 1.0 + s*gran;
    a = sign(a)*exp(log(abs(a))+s*gran);
  }

  void gspread( array<double>& a, const array<double>& s )
  {
    array<double> gran(a.size());
    fillgrand(gran);
    a += s*gran;
  }
}

}

