/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/


template <class hash>
class hashmap
{
  typedef std::vector<objref> v;
  typedef std::map<short,v> vv;
  vv data;
  hash h;
  
  struct objref_eq
  {
    GraphID_t ID;
    objref_eq(GraphID_t i): ID(i) {}
    bool operator()(const objref& x) {return x.ID==ID;}
  };

protected:
  objref& at(GraphID_t i) 
  {
    v& bin=data[h(i)];
    v::iterator elem=find_if(bin.begin(),bin.end(),objref_eq(i));
    if (elem==bin.end())
      {
	bin.push_back(objref(i));
	return bin.back();
      }
    return *elem;
  }

  objref at(GraphID_t i) const
  {
    const v& bin=data[h(i)];
    v::const_iterator elem=find_if(bin.begin(),bin.end(),objref_eq(i));
    if (elem==bin.end())
      return objref(i);
    else
      return *elem;
  }
  

  public:
  hashmap() {}
  hashmap(const hashmap& x): data(x.data) {}

  template <class ret, class v_it, class vv_it>
  class iter
  {
    vv_it i1;
    v_it i2;
    void incr() {i2++; if (i2==i1->second.end()) {i1++; i2=i1->second.begin();}}
    void decr() {if (i2==i1->second.begin()) {i1--; i2=i1->second.end();} i2--;}
  public:
    iter() {}
    iter(const iter& x): i1(x.i1), i2(x.i2) {}
    iter(const vv_it& x,const v_it& y): 
      i1(x), i2(y) {}
    iter operator++(int) {iter r=*this; incr(); return r;}
    iter operator++() {incr(); return *this;}
    iter operator--(int) {iter r=*this; decr(); return r;}
    iter operator--() {decr(); return *this;}
    bool operator==(const iter& x) const {return i1==x.i1 && i2==x.i2 ;}
    bool operator!=(const iter& x) const {return !(x==*this);}
    ret& operator*() {return *i2;}
    ret* operator->() {return &*i2;}
  };
  
  typedef iter<objref,v::iterator,vv::iterator> iterator;
  typedef iter<const objref,v::const_iterator,vv::const_iterator> const_iterator;

  iterator begin() {
    return iterator(data.begin(), data.begin()->second.begin());}
  iterator end() {return iterator(data.end(),data.end()->second.begin());}
  const_iterator begin() const {
    return const_iterator(data.begin(), data.begin()->second.begin());}
  const_iterator end() const {
    return const_iterator(data.end(),data.end()->second.begin());}
  typedef GraphID_t size_type;
  size_type size() const
  {
    unsigned s=0;
    for (vv::const_iterator i=data.begin(); i!=data.end(); i++) 
      s+=i->second.size();
    return s;
  }
  void clear() {data.clear();}
};
