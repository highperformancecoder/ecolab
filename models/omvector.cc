#include <set>
#include <vector>
#include <complex>
#include <iostream>
using namespace std;

inline unsigned maskTest(unsigned i, unsigned test)
{
  if (i & test)
    return test-1;
  else
    return (test-1)>>1;
}

inline unsigned maskTestC(unsigned i, unsigned test)
{
  if (i & (0xC*test))
    return maskTest(i, 8*test);
  else
    return maskTest(i, 2*test);
}

inline unsigned maskTestF(unsigned i, unsigned test)
{
  if (i & (0xF*test))
    return maskTestC(i, test);
  else
    return maskTestC(i, test/16);
}

inline unsigned maskTestFF(unsigned i, unsigned test)
{
  if (i & (0xFF*test))
    return maskTestF(i, 16*test);
  else
    return maskTestF(i, test/16);
}

inline unsigned mask(unsigned i)
{
  if (i & 0xFFFF0000)
    return maskTestFF(i, 0x1000000);
  else
    return maskTestFF(i, 0x100);
}

// intersection of 2 atomic OMs. An atomic OM is all bitstrings satisfying the reverse bitstring, minus the leading 1 bit
// Thus we're considering string only up to length 31.
// Example: 0 = empty set
// 1 = all bitstrings
// 2 = all bitstrings starting with 0
// 3 = all bitstrings starting with 1
// 4 = all bitstrings starting with 00
// ...
// 31 = all bitstrings starting with 1111
// ...
unsigned intersection(unsigned i, unsigned j)
{
  if (j<i) swap(i,j);
  return (mask(i)&i)==(mask(j)&j)? i: 0;
}

// Subset represents a finite union of atomic OMs.
class Subset: public set<unsigned>
{
  // remove contained subsets
  void removeContained() {
    for (auto i=begin(); i!=end(); ) {
      auto j=i++;
      for (auto& k: *this)
        if (*j!=k && intersection(*j, k)==*j) {
          erase(j);
          break;
        }
    }
  }
    
public:
  //Subset(unsigned x=0) {if (x) insert(x);}

  // set union
  Subset& operator+=(const Subset& s) {
    for (auto& i: s)
      insert(i);
    removeContained();
    return *this;
  }
  Subset operator+(Subset s) const  {return s+=*this;}
  Subset& operator+=(unsigned i) {
    insert(i);
    removeContained();
    return *this;
  }
  Subset operator+(unsigned i) const  {
    auto tmp=*this;
    return tmp+=i;
  }

  // set intersection
  Subset operator*(const Subset& s) const  {
    Subset tmp;
    // apply distribution of intersection over unions
    for (auto& i: s)
      for (auto& j: *this)
        if (unsigned k=intersection(i,j))
          tmp.insert(k);
    tmp.removeContained();
    return tmp;
  }

  Subset operator*(unsigned i) const {
    Subset tmp;
    // apply distribution of intersection over unions
    for (auto& j: *this)
      if (unsigned k=intersection(i,j))
        tmp.insert(k);
    tmp.removeContained();
    return tmp;
  }

    
  Subset& operator*=(const Subset& s) {*this=*this*s;}
};

ostream& operator<<(ostream& o, const Subset& s)
{
  if (s.empty())
    return o<<"{}";
  for (auto i=s.begin(); i!=s.end(); ++i)
    o<<(i==s.begin()?"":"u")<<*i;
  return o;
}

struct QMVector: public vector<complex<double>>
{
public:
  QMVector& operator+=(const QMVector& x) {
    if (x.size()>size()) resize(x.size());
    for (size_t i=0; i<x.size(); ++i)
      (*this)[i]+=x[i];
    return *this;
  }
  QMVector& operator-=(const QMVector& x) {
    if (x.size()>size()) resize(x.size());
    for (size_t i=0; i<x.size(); ++i)
      (*this)[i]-=x[i];
    return *this;
  }
  
  QMVector operator+(QMVector x) const  {return x+=*this;}
  QMVector operator-(QMVector x) const  {auto tmp=*this; return tmp-=x;}

  template <class T>
  QMVector& operator*=(const T& x) {
    for (auto& i: *this) i*=x;
    return *this;
  }
  template <class T>
  QMVector operator*(const T& x) const {
    auto tmp=*this;
    return tmp*=x;
  }
};

ostream& operator<<(ostream& o, const QMVector& x)
{
  for (auto& i: x)
    o<<" "<<i.real()<<" "<<i.imag();
  return o;
}

template <class T>
QMVector operator*(const T& x, const QMVector& v) {
  return v*x;
}

/// vector function of atomic OMs
QMVector qmVector(unsigned i)
{
  if (i<=1) return QMVector();
  auto m=mask(i), m1=m>>1;
  auto r=0.5*qmVector((m1 & i)|(m1+1));
  QMVector::value_type E=1;
  if (i&(m1^(m>>2))) // 2nd top bit
    E*=QMVector::value_type{0,1}; // multiply by i
  if (i&(m^m1)) // top bit
    E*=-1;
  r.push_back(E*ldexp(0.5,-r.size()));
  return r;
}

/// vector function of subsets
QMVector qmVector(Subset s)
{
 if (s.empty()) return QMVector();
  // pop off first element
  unsigned i=*s.begin();
  s.erase(s.begin());
  return qmVector(i)+qmVector(s)-qmVector(s*i);
}

void printAllVectors(unsigned n, unsigned numAtomicOMs, const Subset& s=Subset())
{
  if (s.size()==numAtomicOMs)
    cout << s << " "<<qmVector(s)<<endl;
  else
    for (unsigned i=s.empty()? 2: *s.rbegin(); i<(2<<n); ++i)
      {
        auto t=s+i;
        if (t.size()>s.size())
          printAllVectors(n,numAtomicOMs,t);
      }
}

int main()
{
  unsigned n=4; // length of bitstring prefix
  for (unsigned numAtomicOMs=1; numAtomicOMs<(2<<n); ++numAtomicOMs)
    printAllVectors(n, numAtomicOMs);
}
