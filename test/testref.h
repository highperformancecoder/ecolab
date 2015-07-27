/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

struct Int
{
  int x;
};

struct Foo
{
  ecolab::ref<Int> a, b;
  classdesc::ref<Int> c;
  void asg_atob() {b=a;}
  void asg_ctob() {b=c;}
};

