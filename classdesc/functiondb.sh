:

# system shell is assumed to be bash.

max_arity=10
arity=0

# required to handle reference arguments
remove_reference()
{
    i=0
    while [ $i -lt $arity ]; do
        echo "  typename remove_reference<A$[$i+1]>::type a$i(a[$[$i]]);"
        j=$[i++]
    done
}    

while [ $arity -le $max_arity ]; do
    i=1
    template_args=
    arg_types=
    arg_decl=
    args=
    vec_args=
    type_and_args=
    while [ $i -lt $arity ]; do
        template_args="${template_args}, class A$i"
        arg_types="${arg_types}A$i,"
        arg_decl="${arg_decl}A$i a$i,"
        args="${args}a$i,"
        vec_args="${vec_args}a[$[$i-1]],"
#        vec_args="${vec_args}a$[$i-1],"
        type_and_args="${type_and_args} && P<typename Arg<F,$i>::T>::value "
        j=$[i++]
    done
    if [ $i -eq $arity ]; then
        template_args="${template_args}, class A$i"
        arg_types="${arg_types}A$i"
        arg_decl="${arg_decl}A$i a$i"
        args="${args}a$i"
        vec_args="${vec_args}a[$[$i-1]]"
#        vec_args="${vec_args}a$[$i-1]"
        type_and_args="${type_and_args}  && P<typename Arg<F,$i>::T>::value"
    fi

# definition of Arg type accessors
    arg=1
    while [ $arg -le $arity ]; do
cat <<EOF
template <class R$template_args> 
struct Arg<R (*)($arg_types), $arg> 
{typedef A$arg T;};

template <class C, class R$template_args> 
struct Arg<R (C::*)($arg_types), $arg> 
{typedef A$arg T;};

template <class C, class R$template_args> 
struct Arg<R (C::*)($arg_types) const, $arg> 
{typedef A$arg T;};
EOF
    j=$[arg++]
    done

    cat <<EOF
template <class F, template<class> class P>
struct AllArgs<F,P,$arity>
{
   static const bool value=true $type_and_args;
};


template <class R$template_args> 
struct Arity<R (*)($arg_types)> 
{
    static const int V=$arity;
    static const int value=$arity;
};

template <class R$template_args> 
struct Return<R (*)($arg_types)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R$template_args> 
struct Arity<R (C::*)($arg_types)> 
{
    static const int V=$arity;
    static const int value=$arity;
};

template <class C, class R$template_args> 
struct Return<R (C::*)($arg_types)> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R$template_args> 
struct Arity<R (C::*)($arg_types) const> 
{
    static const int V=$arity;
    static const int value=$arity;
};

template <class C, class R$template_args> 
struct Return<R (C::*)($arg_types) const> 
{
    typedef R T;
    typedef R type;
};

template <class C, class R$template_args> 
struct is_member_function_ptr<R (C::*)($arg_types)>
{
   static const bool value=true;
};

template <class C, class R$template_args> 
struct is_member_function_ptr<R (C::*)($arg_types) const>
{
   static const bool value=true;
};

template <class C, class R$template_args> 
struct is_const_method<R (C::*)($arg_types) const>
{
   static const bool value=true;
};

template <class R$template_args> 
struct is_nonmember_function_ptr<R (*)($arg_types)>
{
   static const bool value=true;
};

template <class C, class D, class R$template_args>
class bound_method<C, R (D::*)($arg_types)>
{
    typedef R (D::*M)($arg_types);
    C* obj;
    M method;
    public:
    static const int arity=$arity;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    R operator()($arg_decl) const {return (obj->*method)($args);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D$template_args>
class bound_method<C, void (D::*)($arg_types)>
{
    typedef void (D::*M)($arg_types);
    C* obj;
    M method;
    public:
    static const int arity=$arity;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(&obj), method(method) {}
    void operator()($arg_decl) const {(obj->*method)($args);}
    void rebind(C& newObj) {obj=&newObj;}
};

template <class C, class D, class R$template_args>
class bound_method<C, R (D::*)($arg_types) const>
{
    typedef R (D::*M)($arg_types) const;
    C& obj;
    M method;
    public:
    static const int arity=$arity;
    typedef R Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    R operator()($arg_decl) const {return (obj.*method)($args);}
};

template <class C, class D$template_args>
class bound_method<C, void (D::*)($arg_types) const>
{
    typedef void (D::*M)($arg_types) const;
    C& obj;
    M method;
    public:
    static const int arity=$arity;
    typedef void Ret;
    template <int i> struct Arg: public functional::Arg<M,i> {};
    bound_method(C& obj, M method): obj(obj), method(method) {}
    void operator()($arg_decl) const {(obj.*method)($args);}
};

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, $arity> >, 
   typename Return<F>::T>::T
apply_nonvoid_fn(F f, Args& a, Fdummy<F> dum=0)
{
  return f($vec_args);
}

/*
 TODO: if any of the arguments to f are lvalues, we need to construct temporaries,
 which require C++-11 ability to test for the existence of a copy constructor. 
 If the return type is not copy constructable, the user must arrange for the return value to be discarded
*/

template <class F, class Args> 
typename enable_if<And<AllArgs<F, is_rvalue>, Eq<Arity<F>::value, $arity> >,
    void>::T
apply_void_fn(F f, Args& a, Fdummy<F> dum=0)
{
  f($vec_args);
}

EOF
    j=$[arity++]
done
