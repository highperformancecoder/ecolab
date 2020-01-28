#include "../include/accessor.h"

struct Foo
{
  string m_x;
  struct SetterGetter
  {
    Foo& f;
    SetterGetter(Foo& f): f(f) {}
    string operator()() const {return f.m_x;}
    string operator()(const string& x) const {return f.m_x=x;}
  };

  Accessor<string, SetterGetter, SetterGetter> x;
  Foo(): x(SetterGetter(*this), SetterGetter(*this)) {}
  Foo(const Foo& x):  m_x(x.m_x), x(SetterGetter(*this), SetterGetter(*this)) {}
};

