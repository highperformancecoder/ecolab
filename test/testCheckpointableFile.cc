/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "CheckpointableFile.h"
#include "ecolab_epilogue.h"
#include <vector>

// create two files one original, and one that has been interrupted by a checkpoint
using ecolab::Checkpointable_file;
using classdesc::pack_t;
using namespace std;

int main()
{
  vector<int> data(3); data[0]=1; data[1]=2; data[2]=3;
  vector<int> data2(3); data2[0]=4; data2[1]=5; data2[2]=6;
  vector<int> data3(3); data3[0]=7; data3[1]=8; data3[2]=9;

  {
    Checkpointable_file whole("whole.dat");
    whole.write(data);
    whole.write(data2);
    whole.write(data3);
  }

  pack_t buf;
  {    
    Checkpointable_file ckpt("ckpt.dat");
    ckpt.write(data);
    buf << ckpt;
    ckpt.write(data);
  }

  {    
    Checkpointable_file ckpt;
    buf >> ckpt;
    ckpt.write(data2);
    ckpt.write(data3);
  }
 
  Checkpointable_file stream("stream.dat");
  stream << "hello "<<123<<"\n";
}
