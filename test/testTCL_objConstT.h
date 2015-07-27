/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

struct Foo
{
  const std::string bar;
  static const std::string staticBar;
  static const std::string staticFun();
  enum Enum {a, b};
  const Enum abb;
  Foo(): bar("hello"), abb(a) {}

  void f() {}
};

