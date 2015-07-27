template <class F, template<class> class P>
struct AllArgs<F,P,0>
{
   static const bool value=true ;
};


template <class R> 
struct Arity<R (*)()> 
{
    static const int V=0;
    static const int value=0;
};

template <class R> 
struct Return<R (*)()> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R> 
struct Arity<R (C::*)()> 
{
    static const int V=0;
    static const int value=0;
};

template <class C, class R> 
struct Return<R (C::*)()> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R> 
struct Arity<R (C::*)() const> 
{
    static const int V=0;
    static const int value=0;
};

template <class C, class R> 
struct Return<R (C::*)() const> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R> 
struct is_member_function_ptr<R (C::*)()>
{
   static const bool value=true;
};

template <class C, class R> 
struct is_member_function_ptr<R (C::*)() const>
{
   static const bool value=true;
};

template <class C, class R> 
struct is_const_method<R (C::*)() const>
{
   static const bool value=true;
};

template <class R> 
struct is_nonmember_function_ptr<R (*)()>
{
   static const bool value=true;
};

template <class C, class D, class R>
class bound_method<C, R (D::*)()>
{
    typedef R (D::*M)();
    C* obj;
    M method;
    public:
    static const int arity=0;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    R operator()() const {return (obj->*method)();}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D>
class bound_method<C, void (D::*)()>
{
    typedef void (D::*M)();
    C* obj;
    M method;
    public:
    static const int arity=0;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    void operator()() const {(obj->*method)();}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class R>
class bound_method<C, R (D::*)() const>
{
    typedef R (D::*M)() const;
    C& obj;
    M method;
    public:
    static const int arity=0;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    R operator()() const {return (obj.*method)();}
};

template <class C, class D>
class bound_method<C, void (D::*)() const>
{
    typedef void (D::*M)() const;
    C& obj;
    M method;
    public:
    static const int arity=0;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    void operator()() const {(obj.*method)();}
};

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 0> >, 
   typename Return<F>::T>::T
apply_nonvoid_fn(F f, Args& a, Fdummy<F> dum=0)
{
  return f();
}

/*
 TODO: if any of the arguments to f are lvalues, we need to construct temporaries,
 which require C++-11 ability to test for the existence of a copy constructor. 
 If the return type is not copy constructable, the user must arrange for the return value to be discarded
*/

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 0> >,
    void>::T
apply_void_fn(F f, Args& a, Fdummy<F> dum=0)
{
  f();
}

template <class R, class A1> 
struct Arg<R (*)(A1), 1> 
{typedef A1 T;};

template <class C, class R, class A1> 
struct Arg<R (C::*)(A1), 1> 
{typedef A1 T;};

template <class C, class R, class A1> 
struct Arg<R (C::*)(A1) const, 1> 
{typedef A1 T;};
template <class F, template<class> class P>
struct AllArgs<F,P,1>
{
   static const bool value=true   && P<typename Arg<F,1>::T>::value;
};


template <class R, class A1> 
struct Arity<R (*)(A1)> 
{
    static const int V=1;
    static const int value=1;
};

template <class R, class A1> 
struct Return<R (*)(A1)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1> 
struct Arity<R (C::*)(A1)> 
{
    static const int V=1;
    static const int value=1;
};

template <class C, class R, class A1> 
struct Return<R (C::*)(A1)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1> 
struct Arity<R (C::*)(A1) const> 
{
    static const int V=1;
    static const int value=1;
};

template <class C, class R, class A1> 
struct Return<R (C::*)(A1) const> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1> 
struct is_member_function_ptr<R (C::*)(A1)>
{
   static const bool value=true;
};

template <class C, class R, class A1> 
struct is_member_function_ptr<R (C::*)(A1) const>
{
   static const bool value=true;
};

template <class C, class R, class A1> 
struct is_const_method<R (C::*)(A1) const>
{
   static const bool value=true;
};

template <class R, class A1> 
struct is_nonmember_function_ptr<R (*)(A1)>
{
   static const bool value=true;
};

template <class C, class D, class R, class A1>
class bound_method<C, R (D::*)(A1)>
{
    typedef R (D::*M)(A1);
    C* obj;
    M method;
    public:
    static const int arity=1;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    R operator()(A1 a1) const {return (obj->*method)(a1);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class A1>
class bound_method<C, void (D::*)(A1)>
{
    typedef void (D::*M)(A1);
    C* obj;
    M method;
    public:
    static const int arity=1;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    void operator()(A1 a1) const {(obj->*method)(a1);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class R, class A1>
class bound_method<C, R (D::*)(A1) const>
{
    typedef R (D::*M)(A1) const;
    C& obj;
    M method;
    public:
    static const int arity=1;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    R operator()(A1 a1) const {return (obj.*method)(a1);}
};

template <class C, class D, class A1>
class bound_method<C, void (D::*)(A1) const>
{
    typedef void (D::*M)(A1) const;
    C& obj;
    M method;
    public:
    static const int arity=1;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    void operator()(A1 a1) const {(obj.*method)(a1);}
};

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 1> >, 
   typename Return<F>::T>::T
apply_nonvoid_fn(F f, Args& a, Fdummy<F> dum=0)
{
  return f(a[0]);
}

/*
 TODO: if any of the arguments to f are lvalues, we need to construct temporaries,
 which require C++-11 ability to test for the existence of a copy constructor. 
 If the return type is not copy constructable, the user must arrange for the return value to be discarded
*/

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 1> >,
    void>::T
apply_void_fn(F f, Args& a, Fdummy<F> dum=0)
{
  f(a[0]);
}

template <class R, class A1, class A2> 
struct Arg<R (*)(A1,A2), 1> 
{typedef A1 T;};

template <class C, class R, class A1, class A2> 
struct Arg<R (C::*)(A1,A2), 1> 
{typedef A1 T;};

template <class C, class R, class A1, class A2> 
struct Arg<R (C::*)(A1,A2) const, 1> 
{typedef A1 T;};
template <class R, class A1, class A2> 
struct Arg<R (*)(A1,A2), 2> 
{typedef A2 T;};

template <class C, class R, class A1, class A2> 
struct Arg<R (C::*)(A1,A2), 2> 
{typedef A2 T;};

template <class C, class R, class A1, class A2> 
struct Arg<R (C::*)(A1,A2) const, 2> 
{typedef A2 T;};
template <class F, template<class> class P>
struct AllArgs<F,P,2>
{
   static const bool value=true  && P<typename Arg<F,1>::T>::value   && P<typename Arg<F,2>::T>::value;
};


template <class R, class A1, class A2> 
struct Arity<R (*)(A1,A2)> 
{
    static const int V=2;
    static const int value=2;
};

template <class R, class A1, class A2> 
struct Return<R (*)(A1,A2)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2> 
struct Arity<R (C::*)(A1,A2)> 
{
    static const int V=2;
    static const int value=2;
};

template <class C, class R, class A1, class A2> 
struct Return<R (C::*)(A1,A2)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2> 
struct Arity<R (C::*)(A1,A2) const> 
{
    static const int V=2;
    static const int value=2;
};

template <class C, class R, class A1, class A2> 
struct Return<R (C::*)(A1,A2) const> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2> 
struct is_member_function_ptr<R (C::*)(A1,A2)>
{
   static const bool value=true;
};

template <class C, class R, class A1, class A2> 
struct is_member_function_ptr<R (C::*)(A1,A2) const>
{
   static const bool value=true;
};

template <class C, class R, class A1, class A2> 
struct is_const_method<R (C::*)(A1,A2) const>
{
   static const bool value=true;
};

template <class R, class A1, class A2> 
struct is_nonmember_function_ptr<R (*)(A1,A2)>
{
   static const bool value=true;
};

template <class C, class D, class R, class A1, class A2>
class bound_method<C, R (D::*)(A1,A2)>
{
    typedef R (D::*M)(A1,A2);
    C* obj;
    M method;
    public:
    static const int arity=2;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    R operator()(A1 a1,A2 a2) const {return (obj->*method)(a1,a2);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class A1, class A2>
class bound_method<C, void (D::*)(A1,A2)>
{
    typedef void (D::*M)(A1,A2);
    C* obj;
    M method;
    public:
    static const int arity=2;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    void operator()(A1 a1,A2 a2) const {(obj->*method)(a1,a2);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class R, class A1, class A2>
class bound_method<C, R (D::*)(A1,A2) const>
{
    typedef R (D::*M)(A1,A2) const;
    C& obj;
    M method;
    public:
    static const int arity=2;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    R operator()(A1 a1,A2 a2) const {return (obj.*method)(a1,a2);}
};

template <class C, class D, class A1, class A2>
class bound_method<C, void (D::*)(A1,A2) const>
{
    typedef void (D::*M)(A1,A2) const;
    C& obj;
    M method;
    public:
    static const int arity=2;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    void operator()(A1 a1,A2 a2) const {(obj.*method)(a1,a2);}
};

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 2> >, 
   typename Return<F>::T>::T
apply_nonvoid_fn(F f, Args& a, Fdummy<F> dum=0)
{
  return f(a[0],a[1]);
}

/*
 TODO: if any of the arguments to f are lvalues, we need to construct temporaries,
 which require C++-11 ability to test for the existence of a copy constructor. 
 If the return type is not copy constructable, the user must arrange for the return value to be discarded
*/

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 2> >,
    void>::T
apply_void_fn(F f, Args& a, Fdummy<F> dum=0)
{
  f(a[0],a[1]);
}

template <class R, class A1, class A2, class A3> 
struct Arg<R (*)(A1,A2,A3), 1> 
{typedef A1 T;};

template <class C, class R, class A1, class A2, class A3> 
struct Arg<R (C::*)(A1,A2,A3), 1> 
{typedef A1 T;};

template <class C, class R, class A1, class A2, class A3> 
struct Arg<R (C::*)(A1,A2,A3) const, 1> 
{typedef A1 T;};
template <class R, class A1, class A2, class A3> 
struct Arg<R (*)(A1,A2,A3), 2> 
{typedef A2 T;};

template <class C, class R, class A1, class A2, class A3> 
struct Arg<R (C::*)(A1,A2,A3), 2> 
{typedef A2 T;};

template <class C, class R, class A1, class A2, class A3> 
struct Arg<R (C::*)(A1,A2,A3) const, 2> 
{typedef A2 T;};
template <class R, class A1, class A2, class A3> 
struct Arg<R (*)(A1,A2,A3), 3> 
{typedef A3 T;};

template <class C, class R, class A1, class A2, class A3> 
struct Arg<R (C::*)(A1,A2,A3), 3> 
{typedef A3 T;};

template <class C, class R, class A1, class A2, class A3> 
struct Arg<R (C::*)(A1,A2,A3) const, 3> 
{typedef A3 T;};
template <class F, template<class> class P>
struct AllArgs<F,P,3>
{
   static const bool value=true  && P<typename Arg<F,1>::T>::value  && P<typename Arg<F,2>::T>::value   && P<typename Arg<F,3>::T>::value;
};


template <class R, class A1, class A2, class A3> 
struct Arity<R (*)(A1,A2,A3)> 
{
    static const int V=3;
    static const int value=3;
};

template <class R, class A1, class A2, class A3> 
struct Return<R (*)(A1,A2,A3)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3> 
struct Arity<R (C::*)(A1,A2,A3)> 
{
    static const int V=3;
    static const int value=3;
};

template <class C, class R, class A1, class A2, class A3> 
struct Return<R (C::*)(A1,A2,A3)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3> 
struct Arity<R (C::*)(A1,A2,A3) const> 
{
    static const int V=3;
    static const int value=3;
};

template <class C, class R, class A1, class A2, class A3> 
struct Return<R (C::*)(A1,A2,A3) const> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3> 
struct is_member_function_ptr<R (C::*)(A1,A2,A3)>
{
   static const bool value=true;
};

template <class C, class R, class A1, class A2, class A3> 
struct is_member_function_ptr<R (C::*)(A1,A2,A3) const>
{
   static const bool value=true;
};

template <class C, class R, class A1, class A2, class A3> 
struct is_const_method<R (C::*)(A1,A2,A3) const>
{
   static const bool value=true;
};

template <class R, class A1, class A2, class A3> 
struct is_nonmember_function_ptr<R (*)(A1,A2,A3)>
{
   static const bool value=true;
};

template <class C, class D, class R, class A1, class A2, class A3>
class bound_method<C, R (D::*)(A1,A2,A3)>
{
    typedef R (D::*M)(A1,A2,A3);
    C* obj;
    M method;
    public:
    static const int arity=3;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    R operator()(A1 a1,A2 a2,A3 a3) const {return (obj->*method)(a1,a2,a3);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class A1, class A2, class A3>
class bound_method<C, void (D::*)(A1,A2,A3)>
{
    typedef void (D::*M)(A1,A2,A3);
    C* obj;
    M method;
    public:
    static const int arity=3;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    void operator()(A1 a1,A2 a2,A3 a3) const {(obj->*method)(a1,a2,a3);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class R, class A1, class A2, class A3>
class bound_method<C, R (D::*)(A1,A2,A3) const>
{
    typedef R (D::*M)(A1,A2,A3) const;
    C& obj;
    M method;
    public:
    static const int arity=3;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    R operator()(A1 a1,A2 a2,A3 a3) const {return (obj.*method)(a1,a2,a3);}
};

template <class C, class D, class A1, class A2, class A3>
class bound_method<C, void (D::*)(A1,A2,A3) const>
{
    typedef void (D::*M)(A1,A2,A3) const;
    C& obj;
    M method;
    public:
    static const int arity=3;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    void operator()(A1 a1,A2 a2,A3 a3) const {(obj.*method)(a1,a2,a3);}
};

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 3> >, 
   typename Return<F>::T>::T
apply_nonvoid_fn(F f, Args& a, Fdummy<F> dum=0)
{
  return f(a[0],a[1],a[2]);
}

/*
 TODO: if any of the arguments to f are lvalues, we need to construct temporaries,
 which require C++-11 ability to test for the existence of a copy constructor. 
 If the return type is not copy constructable, the user must arrange for the return value to be discarded
*/

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 3> >,
    void>::T
apply_void_fn(F f, Args& a, Fdummy<F> dum=0)
{
  f(a[0],a[1],a[2]);
}

template <class R, class A1, class A2, class A3, class A4> 
struct Arg<R (*)(A1,A2,A3,A4), 1> 
{typedef A1 T;};

template <class C, class R, class A1, class A2, class A3, class A4> 
struct Arg<R (C::*)(A1,A2,A3,A4), 1> 
{typedef A1 T;};

template <class C, class R, class A1, class A2, class A3, class A4> 
struct Arg<R (C::*)(A1,A2,A3,A4) const, 1> 
{typedef A1 T;};
template <class R, class A1, class A2, class A3, class A4> 
struct Arg<R (*)(A1,A2,A3,A4), 2> 
{typedef A2 T;};

template <class C, class R, class A1, class A2, class A3, class A4> 
struct Arg<R (C::*)(A1,A2,A3,A4), 2> 
{typedef A2 T;};

template <class C, class R, class A1, class A2, class A3, class A4> 
struct Arg<R (C::*)(A1,A2,A3,A4) const, 2> 
{typedef A2 T;};
template <class R, class A1, class A2, class A3, class A4> 
struct Arg<R (*)(A1,A2,A3,A4), 3> 
{typedef A3 T;};

template <class C, class R, class A1, class A2, class A3, class A4> 
struct Arg<R (C::*)(A1,A2,A3,A4), 3> 
{typedef A3 T;};

template <class C, class R, class A1, class A2, class A3, class A4> 
struct Arg<R (C::*)(A1,A2,A3,A4) const, 3> 
{typedef A3 T;};
template <class R, class A1, class A2, class A3, class A4> 
struct Arg<R (*)(A1,A2,A3,A4), 4> 
{typedef A4 T;};

template <class C, class R, class A1, class A2, class A3, class A4> 
struct Arg<R (C::*)(A1,A2,A3,A4), 4> 
{typedef A4 T;};

template <class C, class R, class A1, class A2, class A3, class A4> 
struct Arg<R (C::*)(A1,A2,A3,A4) const, 4> 
{typedef A4 T;};
template <class F, template<class> class P>
struct AllArgs<F,P,4>
{
   static const bool value=true  && P<typename Arg<F,1>::T>::value  && P<typename Arg<F,2>::T>::value  && P<typename Arg<F,3>::T>::value   && P<typename Arg<F,4>::T>::value;
};


template <class R, class A1, class A2, class A3, class A4> 
struct Arity<R (*)(A1,A2,A3,A4)> 
{
    static const int V=4;
    static const int value=4;
};

template <class R, class A1, class A2, class A3, class A4> 
struct Return<R (*)(A1,A2,A3,A4)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4> 
struct Arity<R (C::*)(A1,A2,A3,A4)> 
{
    static const int V=4;
    static const int value=4;
};

template <class C, class R, class A1, class A2, class A3, class A4> 
struct Return<R (C::*)(A1,A2,A3,A4)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4> 
struct Arity<R (C::*)(A1,A2,A3,A4) const> 
{
    static const int V=4;
    static const int value=4;
};

template <class C, class R, class A1, class A2, class A3, class A4> 
struct Return<R (C::*)(A1,A2,A3,A4) const> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4> 
struct is_member_function_ptr<R (C::*)(A1,A2,A3,A4)>
{
   static const bool value=true;
};

template <class C, class R, class A1, class A2, class A3, class A4> 
struct is_member_function_ptr<R (C::*)(A1,A2,A3,A4) const>
{
   static const bool value=true;
};

template <class C, class R, class A1, class A2, class A3, class A4> 
struct is_const_method<R (C::*)(A1,A2,A3,A4) const>
{
   static const bool value=true;
};

template <class R, class A1, class A2, class A3, class A4> 
struct is_nonmember_function_ptr<R (*)(A1,A2,A3,A4)>
{
   static const bool value=true;
};

template <class C, class D, class R, class A1, class A2, class A3, class A4>
class bound_method<C, R (D::*)(A1,A2,A3,A4)>
{
    typedef R (D::*M)(A1,A2,A3,A4);
    C* obj;
    M method;
    public:
    static const int arity=4;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    R operator()(A1 a1,A2 a2,A3 a3,A4 a4) const {return (obj->*method)(a1,a2,a3,a4);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class A1, class A2, class A3, class A4>
class bound_method<C, void (D::*)(A1,A2,A3,A4)>
{
    typedef void (D::*M)(A1,A2,A3,A4);
    C* obj;
    M method;
    public:
    static const int arity=4;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    void operator()(A1 a1,A2 a2,A3 a3,A4 a4) const {(obj->*method)(a1,a2,a3,a4);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class R, class A1, class A2, class A3, class A4>
class bound_method<C, R (D::*)(A1,A2,A3,A4) const>
{
    typedef R (D::*M)(A1,A2,A3,A4) const;
    C& obj;
    M method;
    public:
    static const int arity=4;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    R operator()(A1 a1,A2 a2,A3 a3,A4 a4) const {return (obj.*method)(a1,a2,a3,a4);}
};

template <class C, class D, class A1, class A2, class A3, class A4>
class bound_method<C, void (D::*)(A1,A2,A3,A4) const>
{
    typedef void (D::*M)(A1,A2,A3,A4) const;
    C& obj;
    M method;
    public:
    static const int arity=4;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    void operator()(A1 a1,A2 a2,A3 a3,A4 a4) const {(obj.*method)(a1,a2,a3,a4);}
};

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 4> >, 
   typename Return<F>::T>::T
apply_nonvoid_fn(F f, Args& a, Fdummy<F> dum=0)
{
  return f(a[0],a[1],a[2],a[3]);
}

/*
 TODO: if any of the arguments to f are lvalues, we need to construct temporaries,
 which require C++-11 ability to test for the existence of a copy constructor. 
 If the return type is not copy constructable, the user must arrange for the return value to be discarded
*/

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 4> >,
    void>::T
apply_void_fn(F f, Args& a, Fdummy<F> dum=0)
{
  f(a[0],a[1],a[2],a[3]);
}

template <class R, class A1, class A2, class A3, class A4, class A5> 
struct Arg<R (*)(A1,A2,A3,A4,A5), 1> 
{typedef A1 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5), 1> 
{typedef A1 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5) const, 1> 
{typedef A1 T;};
template <class R, class A1, class A2, class A3, class A4, class A5> 
struct Arg<R (*)(A1,A2,A3,A4,A5), 2> 
{typedef A2 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5), 2> 
{typedef A2 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5) const, 2> 
{typedef A2 T;};
template <class R, class A1, class A2, class A3, class A4, class A5> 
struct Arg<R (*)(A1,A2,A3,A4,A5), 3> 
{typedef A3 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5), 3> 
{typedef A3 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5) const, 3> 
{typedef A3 T;};
template <class R, class A1, class A2, class A3, class A4, class A5> 
struct Arg<R (*)(A1,A2,A3,A4,A5), 4> 
{typedef A4 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5), 4> 
{typedef A4 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5) const, 4> 
{typedef A4 T;};
template <class R, class A1, class A2, class A3, class A4, class A5> 
struct Arg<R (*)(A1,A2,A3,A4,A5), 5> 
{typedef A5 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5), 5> 
{typedef A5 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5) const, 5> 
{typedef A5 T;};
template <class F, template<class> class P>
struct AllArgs<F,P,5>
{
   static const bool value=true  && P<typename Arg<F,1>::T>::value  && P<typename Arg<F,2>::T>::value  && P<typename Arg<F,3>::T>::value  && P<typename Arg<F,4>::T>::value   && P<typename Arg<F,5>::T>::value;
};


template <class R, class A1, class A2, class A3, class A4, class A5> 
struct Arity<R (*)(A1,A2,A3,A4,A5)> 
{
    static const int V=5;
    static const int value=5;
};

template <class R, class A1, class A2, class A3, class A4, class A5> 
struct Return<R (*)(A1,A2,A3,A4,A5)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5> 
struct Arity<R (C::*)(A1,A2,A3,A4,A5)> 
{
    static const int V=5;
    static const int value=5;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5> 
struct Return<R (C::*)(A1,A2,A3,A4,A5)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5> 
struct Arity<R (C::*)(A1,A2,A3,A4,A5) const> 
{
    static const int V=5;
    static const int value=5;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5> 
struct Return<R (C::*)(A1,A2,A3,A4,A5) const> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5> 
struct is_member_function_ptr<R (C::*)(A1,A2,A3,A4,A5)>
{
   static const bool value=true;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5> 
struct is_member_function_ptr<R (C::*)(A1,A2,A3,A4,A5) const>
{
   static const bool value=true;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5> 
struct is_const_method<R (C::*)(A1,A2,A3,A4,A5) const>
{
   static const bool value=true;
};

template <class R, class A1, class A2, class A3, class A4, class A5> 
struct is_nonmember_function_ptr<R (*)(A1,A2,A3,A4,A5)>
{
   static const bool value=true;
};

template <class C, class D, class R, class A1, class A2, class A3, class A4, class A5>
class bound_method<C, R (D::*)(A1,A2,A3,A4,A5)>
{
    typedef R (D::*M)(A1,A2,A3,A4,A5);
    C* obj;
    M method;
    public:
    static const int arity=5;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    R operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5) const {return (obj->*method)(a1,a2,a3,a4,a5);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class A1, class A2, class A3, class A4, class A5>
class bound_method<C, void (D::*)(A1,A2,A3,A4,A5)>
{
    typedef void (D::*M)(A1,A2,A3,A4,A5);
    C* obj;
    M method;
    public:
    static const int arity=5;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    void operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5) const {(obj->*method)(a1,a2,a3,a4,a5);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class R, class A1, class A2, class A3, class A4, class A5>
class bound_method<C, R (D::*)(A1,A2,A3,A4,A5) const>
{
    typedef R (D::*M)(A1,A2,A3,A4,A5) const;
    C& obj;
    M method;
    public:
    static const int arity=5;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    R operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5) const {return (obj.*method)(a1,a2,a3,a4,a5);}
};

template <class C, class D, class A1, class A2, class A3, class A4, class A5>
class bound_method<C, void (D::*)(A1,A2,A3,A4,A5) const>
{
    typedef void (D::*M)(A1,A2,A3,A4,A5) const;
    C& obj;
    M method;
    public:
    static const int arity=5;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    void operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5) const {(obj.*method)(a1,a2,a3,a4,a5);}
};

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 5> >, 
   typename Return<F>::T>::T
apply_nonvoid_fn(F f, Args& a, Fdummy<F> dum=0)
{
  return f(a[0],a[1],a[2],a[3],a[4]);
}

/*
 TODO: if any of the arguments to f are lvalues, we need to construct temporaries,
 which require C++-11 ability to test for the existence of a copy constructor. 
 If the return type is not copy constructable, the user must arrange for the return value to be discarded
*/

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 5> >,
    void>::T
apply_void_fn(F f, Args& a, Fdummy<F> dum=0)
{
  f(a[0],a[1],a[2],a[3],a[4]);
}

template <class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6), 1> 
{typedef A1 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6), 1> 
{typedef A1 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6) const, 1> 
{typedef A1 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6), 2> 
{typedef A2 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6), 2> 
{typedef A2 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6) const, 2> 
{typedef A2 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6), 3> 
{typedef A3 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6), 3> 
{typedef A3 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6) const, 3> 
{typedef A3 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6), 4> 
{typedef A4 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6), 4> 
{typedef A4 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6) const, 4> 
{typedef A4 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6), 5> 
{typedef A5 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6), 5> 
{typedef A5 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6) const, 5> 
{typedef A5 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6), 6> 
{typedef A6 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6), 6> 
{typedef A6 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6) const, 6> 
{typedef A6 T;};
template <class F, template<class> class P>
struct AllArgs<F,P,6>
{
   static const bool value=true  && P<typename Arg<F,1>::T>::value  && P<typename Arg<F,2>::T>::value  && P<typename Arg<F,3>::T>::value  && P<typename Arg<F,4>::T>::value  && P<typename Arg<F,5>::T>::value   && P<typename Arg<F,6>::T>::value;
};


template <class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arity<R (*)(A1,A2,A3,A4,A5,A6)> 
{
    static const int V=6;
    static const int value=6;
};

template <class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Return<R (*)(A1,A2,A3,A4,A5,A6)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arity<R (C::*)(A1,A2,A3,A4,A5,A6)> 
{
    static const int V=6;
    static const int value=6;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Return<R (C::*)(A1,A2,A3,A4,A5,A6)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Arity<R (C::*)(A1,A2,A3,A4,A5,A6) const> 
{
    static const int V=6;
    static const int value=6;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct Return<R (C::*)(A1,A2,A3,A4,A5,A6) const> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct is_member_function_ptr<R (C::*)(A1,A2,A3,A4,A5,A6)>
{
   static const bool value=true;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct is_member_function_ptr<R (C::*)(A1,A2,A3,A4,A5,A6) const>
{
   static const bool value=true;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct is_const_method<R (C::*)(A1,A2,A3,A4,A5,A6) const>
{
   static const bool value=true;
};

template <class R, class A1, class A2, class A3, class A4, class A5, class A6> 
struct is_nonmember_function_ptr<R (*)(A1,A2,A3,A4,A5,A6)>
{
   static const bool value=true;
};

template <class C, class D, class R, class A1, class A2, class A3, class A4, class A5, class A6>
class bound_method<C, R (D::*)(A1,A2,A3,A4,A5,A6)>
{
    typedef R (D::*M)(A1,A2,A3,A4,A5,A6);
    C* obj;
    M method;
    public:
    static const int arity=6;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    R operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6) const {return (obj->*method)(a1,a2,a3,a4,a5,a6);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class A1, class A2, class A3, class A4, class A5, class A6>
class bound_method<C, void (D::*)(A1,A2,A3,A4,A5,A6)>
{
    typedef void (D::*M)(A1,A2,A3,A4,A5,A6);
    C* obj;
    M method;
    public:
    static const int arity=6;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    void operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6) const {(obj->*method)(a1,a2,a3,a4,a5,a6);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class R, class A1, class A2, class A3, class A4, class A5, class A6>
class bound_method<C, R (D::*)(A1,A2,A3,A4,A5,A6) const>
{
    typedef R (D::*M)(A1,A2,A3,A4,A5,A6) const;
    C& obj;
    M method;
    public:
    static const int arity=6;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    R operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6) const {return (obj.*method)(a1,a2,a3,a4,a5,a6);}
};

template <class C, class D, class A1, class A2, class A3, class A4, class A5, class A6>
class bound_method<C, void (D::*)(A1,A2,A3,A4,A5,A6) const>
{
    typedef void (D::*M)(A1,A2,A3,A4,A5,A6) const;
    C& obj;
    M method;
    public:
    static const int arity=6;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    void operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6) const {(obj.*method)(a1,a2,a3,a4,a5,a6);}
};

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 6> >, 
   typename Return<F>::T>::T
apply_nonvoid_fn(F f, Args& a, Fdummy<F> dum=0)
{
  return f(a[0],a[1],a[2],a[3],a[4],a[5]);
}

/*
 TODO: if any of the arguments to f are lvalues, we need to construct temporaries,
 which require C++-11 ability to test for the existence of a copy constructor. 
 If the return type is not copy constructable, the user must arrange for the return value to be discarded
*/

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 6> >,
    void>::T
apply_void_fn(F f, Args& a, Fdummy<F> dum=0)
{
  f(a[0],a[1],a[2],a[3],a[4],a[5]);
}

template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7), 1> 
{typedef A1 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7), 1> 
{typedef A1 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7) const, 1> 
{typedef A1 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7), 2> 
{typedef A2 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7), 2> 
{typedef A2 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7) const, 2> 
{typedef A2 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7), 3> 
{typedef A3 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7), 3> 
{typedef A3 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7) const, 3> 
{typedef A3 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7), 4> 
{typedef A4 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7), 4> 
{typedef A4 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7) const, 4> 
{typedef A4 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7), 5> 
{typedef A5 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7), 5> 
{typedef A5 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7) const, 5> 
{typedef A5 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7), 6> 
{typedef A6 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7), 6> 
{typedef A6 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7) const, 6> 
{typedef A6 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7), 7> 
{typedef A7 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7), 7> 
{typedef A7 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7) const, 7> 
{typedef A7 T;};
template <class F, template<class> class P>
struct AllArgs<F,P,7>
{
   static const bool value=true  && P<typename Arg<F,1>::T>::value  && P<typename Arg<F,2>::T>::value  && P<typename Arg<F,3>::T>::value  && P<typename Arg<F,4>::T>::value  && P<typename Arg<F,5>::T>::value  && P<typename Arg<F,6>::T>::value   && P<typename Arg<F,7>::T>::value;
};


template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arity<R (*)(A1,A2,A3,A4,A5,A6,A7)> 
{
    static const int V=7;
    static const int value=7;
};

template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Return<R (*)(A1,A2,A3,A4,A5,A6,A7)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arity<R (C::*)(A1,A2,A3,A4,A5,A6,A7)> 
{
    static const int V=7;
    static const int value=7;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Return<R (C::*)(A1,A2,A3,A4,A5,A6,A7)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Arity<R (C::*)(A1,A2,A3,A4,A5,A6,A7) const> 
{
    static const int V=7;
    static const int value=7;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct Return<R (C::*)(A1,A2,A3,A4,A5,A6,A7) const> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct is_member_function_ptr<R (C::*)(A1,A2,A3,A4,A5,A6,A7)>
{
   static const bool value=true;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct is_member_function_ptr<R (C::*)(A1,A2,A3,A4,A5,A6,A7) const>
{
   static const bool value=true;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct is_const_method<R (C::*)(A1,A2,A3,A4,A5,A6,A7) const>
{
   static const bool value=true;
};

template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7> 
struct is_nonmember_function_ptr<R (*)(A1,A2,A3,A4,A5,A6,A7)>
{
   static const bool value=true;
};

template <class C, class D, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
class bound_method<C, R (D::*)(A1,A2,A3,A4,A5,A6,A7)>
{
    typedef R (D::*M)(A1,A2,A3,A4,A5,A6,A7);
    C* obj;
    M method;
    public:
    static const int arity=7;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    R operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7) const {return (obj->*method)(a1,a2,a3,a4,a5,a6,a7);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
class bound_method<C, void (D::*)(A1,A2,A3,A4,A5,A6,A7)>
{
    typedef void (D::*M)(A1,A2,A3,A4,A5,A6,A7);
    C* obj;
    M method;
    public:
    static const int arity=7;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    void operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7) const {(obj->*method)(a1,a2,a3,a4,a5,a6,a7);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
class bound_method<C, R (D::*)(A1,A2,A3,A4,A5,A6,A7) const>
{
    typedef R (D::*M)(A1,A2,A3,A4,A5,A6,A7) const;
    C& obj;
    M method;
    public:
    static const int arity=7;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    R operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7) const {return (obj.*method)(a1,a2,a3,a4,a5,a6,a7);}
};

template <class C, class D, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
class bound_method<C, void (D::*)(A1,A2,A3,A4,A5,A6,A7) const>
{
    typedef void (D::*M)(A1,A2,A3,A4,A5,A6,A7) const;
    C& obj;
    M method;
    public:
    static const int arity=7;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    void operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7) const {(obj.*method)(a1,a2,a3,a4,a5,a6,a7);}
};

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 7> >, 
   typename Return<F>::T>::T
apply_nonvoid_fn(F f, Args& a, Fdummy<F> dum=0)
{
  return f(a[0],a[1],a[2],a[3],a[4],a[5],a[6]);
}

/*
 TODO: if any of the arguments to f are lvalues, we need to construct temporaries,
 which require C++-11 ability to test for the existence of a copy constructor. 
 If the return type is not copy constructable, the user must arrange for the return value to be discarded
*/

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 7> >,
    void>::T
apply_void_fn(F f, Args& a, Fdummy<F> dum=0)
{
  f(a[0],a[1],a[2],a[3],a[4],a[5],a[6]);
}

template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8), 1> 
{typedef A1 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8), 1> 
{typedef A1 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8) const, 1> 
{typedef A1 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8), 2> 
{typedef A2 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8), 2> 
{typedef A2 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8) const, 2> 
{typedef A2 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8), 3> 
{typedef A3 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8), 3> 
{typedef A3 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8) const, 3> 
{typedef A3 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8), 4> 
{typedef A4 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8), 4> 
{typedef A4 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8) const, 4> 
{typedef A4 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8), 5> 
{typedef A5 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8), 5> 
{typedef A5 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8) const, 5> 
{typedef A5 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8), 6> 
{typedef A6 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8), 6> 
{typedef A6 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8) const, 6> 
{typedef A6 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8), 7> 
{typedef A7 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8), 7> 
{typedef A7 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8) const, 7> 
{typedef A7 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8), 8> 
{typedef A8 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8), 8> 
{typedef A8 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8) const, 8> 
{typedef A8 T;};
template <class F, template<class> class P>
struct AllArgs<F,P,8>
{
   static const bool value=true  && P<typename Arg<F,1>::T>::value  && P<typename Arg<F,2>::T>::value  && P<typename Arg<F,3>::T>::value  && P<typename Arg<F,4>::T>::value  && P<typename Arg<F,5>::T>::value  && P<typename Arg<F,6>::T>::value  && P<typename Arg<F,7>::T>::value   && P<typename Arg<F,8>::T>::value;
};


template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arity<R (*)(A1,A2,A3,A4,A5,A6,A7,A8)> 
{
    static const int V=8;
    static const int value=8;
};

template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Return<R (*)(A1,A2,A3,A4,A5,A6,A7,A8)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arity<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8)> 
{
    static const int V=8;
    static const int value=8;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Return<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Arity<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8) const> 
{
    static const int V=8;
    static const int value=8;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct Return<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8) const> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct is_member_function_ptr<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8)>
{
   static const bool value=true;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct is_member_function_ptr<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8) const>
{
   static const bool value=true;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct is_const_method<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8) const>
{
   static const bool value=true;
};

template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> 
struct is_nonmember_function_ptr<R (*)(A1,A2,A3,A4,A5,A6,A7,A8)>
{
   static const bool value=true;
};

template <class C, class D, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
class bound_method<C, R (D::*)(A1,A2,A3,A4,A5,A6,A7,A8)>
{
    typedef R (D::*M)(A1,A2,A3,A4,A5,A6,A7,A8);
    C* obj;
    M method;
    public:
    static const int arity=8;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    R operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8) const {return (obj->*method)(a1,a2,a3,a4,a5,a6,a7,a8);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
class bound_method<C, void (D::*)(A1,A2,A3,A4,A5,A6,A7,A8)>
{
    typedef void (D::*M)(A1,A2,A3,A4,A5,A6,A7,A8);
    C* obj;
    M method;
    public:
    static const int arity=8;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    void operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8) const {(obj->*method)(a1,a2,a3,a4,a5,a6,a7,a8);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
class bound_method<C, R (D::*)(A1,A2,A3,A4,A5,A6,A7,A8) const>
{
    typedef R (D::*M)(A1,A2,A3,A4,A5,A6,A7,A8) const;
    C& obj;
    M method;
    public:
    static const int arity=8;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    R operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8) const {return (obj.*method)(a1,a2,a3,a4,a5,a6,a7,a8);}
};

template <class C, class D, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
class bound_method<C, void (D::*)(A1,A2,A3,A4,A5,A6,A7,A8) const>
{
    typedef void (D::*M)(A1,A2,A3,A4,A5,A6,A7,A8) const;
    C& obj;
    M method;
    public:
    static const int arity=8;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    void operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8) const {(obj.*method)(a1,a2,a3,a4,a5,a6,a7,a8);}
};

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 8> >, 
   typename Return<F>::T>::T
apply_nonvoid_fn(F f, Args& a, Fdummy<F> dum=0)
{
  return f(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7]);
}

/*
 TODO: if any of the arguments to f are lvalues, we need to construct temporaries,
 which require C++-11 ability to test for the existence of a copy constructor. 
 If the return type is not copy constructable, the user must arrange for the return value to be discarded
*/

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 8> >,
    void>::T
apply_void_fn(F f, Args& a, Fdummy<F> dum=0)
{
  f(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7]);
}

template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9), 1> 
{typedef A1 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9), 1> 
{typedef A1 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9) const, 1> 
{typedef A1 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9), 2> 
{typedef A2 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9), 2> 
{typedef A2 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9) const, 2> 
{typedef A2 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9), 3> 
{typedef A3 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9), 3> 
{typedef A3 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9) const, 3> 
{typedef A3 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9), 4> 
{typedef A4 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9), 4> 
{typedef A4 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9) const, 4> 
{typedef A4 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9), 5> 
{typedef A5 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9), 5> 
{typedef A5 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9) const, 5> 
{typedef A5 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9), 6> 
{typedef A6 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9), 6> 
{typedef A6 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9) const, 6> 
{typedef A6 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9), 7> 
{typedef A7 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9), 7> 
{typedef A7 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9) const, 7> 
{typedef A7 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9), 8> 
{typedef A8 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9), 8> 
{typedef A8 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9) const, 8> 
{typedef A8 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9), 9> 
{typedef A9 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9), 9> 
{typedef A9 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9) const, 9> 
{typedef A9 T;};
template <class F, template<class> class P>
struct AllArgs<F,P,9>
{
   static const bool value=true  && P<typename Arg<F,1>::T>::value  && P<typename Arg<F,2>::T>::value  && P<typename Arg<F,3>::T>::value  && P<typename Arg<F,4>::T>::value  && P<typename Arg<F,5>::T>::value  && P<typename Arg<F,6>::T>::value  && P<typename Arg<F,7>::T>::value  && P<typename Arg<F,8>::T>::value   && P<typename Arg<F,9>::T>::value;
};


template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arity<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9)> 
{
    static const int V=9;
    static const int value=9;
};

template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Return<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arity<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9)> 
{
    static const int V=9;
    static const int value=9;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Return<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Arity<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9) const> 
{
    static const int V=9;
    static const int value=9;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct Return<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9) const> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct is_member_function_ptr<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9)>
{
   static const bool value=true;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct is_member_function_ptr<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9) const>
{
   static const bool value=true;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct is_const_method<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9) const>
{
   static const bool value=true;
};

template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> 
struct is_nonmember_function_ptr<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9)>
{
   static const bool value=true;
};

template <class C, class D, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
class bound_method<C, R (D::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9)>
{
    typedef R (D::*M)(A1,A2,A3,A4,A5,A6,A7,A8,A9);
    C* obj;
    M method;
    public:
    static const int arity=9;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    R operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8,A9 a9) const {return (obj->*method)(a1,a2,a3,a4,a5,a6,a7,a8,a9);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
class bound_method<C, void (D::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9)>
{
    typedef void (D::*M)(A1,A2,A3,A4,A5,A6,A7,A8,A9);
    C* obj;
    M method;
    public:
    static const int arity=9;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    void operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8,A9 a9) const {(obj->*method)(a1,a2,a3,a4,a5,a6,a7,a8,a9);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
class bound_method<C, R (D::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9) const>
{
    typedef R (D::*M)(A1,A2,A3,A4,A5,A6,A7,A8,A9) const;
    C& obj;
    M method;
    public:
    static const int arity=9;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    R operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8,A9 a9) const {return (obj.*method)(a1,a2,a3,a4,a5,a6,a7,a8,a9);}
};

template <class C, class D, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
class bound_method<C, void (D::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9) const>
{
    typedef void (D::*M)(A1,A2,A3,A4,A5,A6,A7,A8,A9) const;
    C& obj;
    M method;
    public:
    static const int arity=9;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    void operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8,A9 a9) const {(obj.*method)(a1,a2,a3,a4,a5,a6,a7,a8,a9);}
};

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 9> >, 
   typename Return<F>::T>::T
apply_nonvoid_fn(F f, Args& a, Fdummy<F> dum=0)
{
  return f(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8]);
}

/*
 TODO: if any of the arguments to f are lvalues, we need to construct temporaries,
 which require C++-11 ability to test for the existence of a copy constructor. 
 If the return type is not copy constructable, the user must arrange for the return value to be discarded
*/

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 9> >,
    void>::T
apply_void_fn(F f, Args& a, Fdummy<F> dum=0)
{
  f(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8]);
}

template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10), 1> 
{typedef A1 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10), 1> 
{typedef A1 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10) const, 1> 
{typedef A1 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10), 2> 
{typedef A2 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10), 2> 
{typedef A2 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10) const, 2> 
{typedef A2 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10), 3> 
{typedef A3 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10), 3> 
{typedef A3 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10) const, 3> 
{typedef A3 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10), 4> 
{typedef A4 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10), 4> 
{typedef A4 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10) const, 4> 
{typedef A4 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10), 5> 
{typedef A5 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10), 5> 
{typedef A5 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10) const, 5> 
{typedef A5 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10), 6> 
{typedef A6 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10), 6> 
{typedef A6 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10) const, 6> 
{typedef A6 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10), 7> 
{typedef A7 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10), 7> 
{typedef A7 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10) const, 7> 
{typedef A7 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10), 8> 
{typedef A8 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10), 8> 
{typedef A8 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10) const, 8> 
{typedef A8 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10), 9> 
{typedef A9 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10), 9> 
{typedef A9 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10) const, 9> 
{typedef A9 T;};
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10), 10> 
{typedef A10 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10), 10> 
{typedef A10 T;};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arg<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10) const, 10> 
{typedef A10 T;};
template <class F, template<class> class P>
struct AllArgs<F,P,10>
{
   static const bool value=true  && P<typename Arg<F,1>::T>::value  && P<typename Arg<F,2>::T>::value  && P<typename Arg<F,3>::T>::value  && P<typename Arg<F,4>::T>::value  && P<typename Arg<F,5>::T>::value  && P<typename Arg<F,6>::T>::value  && P<typename Arg<F,7>::T>::value  && P<typename Arg<F,8>::T>::value  && P<typename Arg<F,9>::T>::value   && P<typename Arg<F,10>::T>::value;
};


template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arity<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10)> 
{
    static const int V=10;
    static const int value=10;
};

template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Return<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arity<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10)> 
{
    static const int V=10;
    static const int value=10;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Return<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Arity<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10) const> 
{
    static const int V=10;
    static const int value=10;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct Return<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10) const> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct is_member_function_ptr<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10)>
{
   static const bool value=true;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct is_member_function_ptr<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10) const>
{
   static const bool value=true;
};

template <class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct is_const_method<R (C::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10) const>
{
   static const bool value=true;
};

template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> 
struct is_nonmember_function_ptr<R (*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10)>
{
   static const bool value=true;
};

template <class C, class D, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10>
class bound_method<C, R (D::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10)>
{
    typedef R (D::*M)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10);
    C* obj;
    M method;
    public:
    static const int arity=10;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    R operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8,A9 a9,A10 a10) const {return (obj->*method)(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10>
class bound_method<C, void (D::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10)>
{
    typedef void (D::*M)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10);
    C* obj;
    M method;
    public:
    static const int arity=10;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    void operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8,A9 a9,A10 a10) const {(obj->*method)(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10>
class bound_method<C, R (D::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10) const>
{
    typedef R (D::*M)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10) const;
    C& obj;
    M method;
    public:
    static const int arity=10;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    R operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8,A9 a9,A10 a10) const {return (obj.*method)(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10);}
};

template <class C, class D, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10>
class bound_method<C, void (D::*)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10) const>
{
    typedef void (D::*M)(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10) const;
    C& obj;
    M method;
    public:
    static const int arity=10;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    void operator()(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8,A9 a9,A10 a10) const {(obj.*method)(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10);}
};

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 10> >, 
   typename Return<F>::T>::T
apply_nonvoid_fn(F f, Args& a, Fdummy<F> dum=0)
{
  return f(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9]);
}

/*
 TODO: if any of the arguments to f are lvalues, we need to construct temporaries,
 which require C++-11 ability to test for the existence of a copy constructor. 
 If the return type is not copy constructable, the user must arrange for the return value to be discarded
*/

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, 10> >,
    void>::T
apply_void_fn(F f, Args& a, Fdummy<F> dum=0)
{
  f(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9]);
}

