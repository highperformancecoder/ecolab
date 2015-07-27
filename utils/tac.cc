/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main()
{
  const int bufsize=1024;
  char buf[bufsize];
  vector<string> lines;
  while (cin) 
    {
      cin.getline(buf,bufsize);
      lines.push_back(buf);
    }
  while (!lines.empty()) 
    {
      if (!lines.back().empty()) cout << lines.back() << endl;
      lines.pop_back();
    }
  return 0;
}
