/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#undef UNURAN
#include "arrays.h"
#include "sparse_mat.h"
#include "ecolab_epilogue.h"
using namespace ecolab;
#include <stdio.h>
//#include <assert.h>
#undef assert
# define assert(expr) \
 if (!(expr)) {\
   fprintf(stderr,"Assertion failed: %s, file %s, line %d\n",#expr,__FILE__,__LINE__);\
   exit(1); \
}

#include <stdarg.h>
#include <math.h>
#include <iostream>
#include <vector>
using std::cout; 
using std::endl;
using namespace ecolab;

extern "C" void error(char *fmt,...)
{
  va_list args;
  va_start(args, fmt);
  vprintf(fmt,args);
  va_end(args);
  exit(0);
}

void asg(array<double>& x, double y[])
{
  memcpy(x.data(),y,x.size()*sizeof(x[0]));
}

void asg(array<int>& x, int y[])
{
  memcpy(x.data(),y,x.size()*sizeof(x[0]));
}

void print(const array<double>& x)
{
  for (size_t i=0; i<x.size(); i++) printf("%g ",double(x[i]));
  printf("\n");
}

void print(const array<int>& x)
{
  for (size_t i=0; i<x.size(); printf("%d ",(int)x[i++]));
  printf("\n");
}

int near(double x,double y)
{return (x==y)? 1: fabs(x-y)/fabs(x+y)<1E-4;}

int main()
{
  /* test array creation and indexing */
  int i;
  array<double> a(10);
  array<int> ia(10);
  int itest[]={1,2,3,4,5,6,7,8,9,10};
  double test[]={1,2,3,4,5,6,7,8,9,10};

  asg(a,test); asg(ia,itest);

  for (i=0; i<10; i++) 
    {
      assert(a[i]==i+1);
      assert(ia[i]==i+1);
      assert(a.data()[i]==i+1);
      assert(ia.data()[i]==i+1);
    }

  /* test sum */
  assert(sum(ia)==55);
  assert(sum(a)==55.0);
  assert(prod(ia)==3628800);
  assert(prod(a)==3628800.0);
  int ctest[]={0,0,1,0,0,3,0,2,0,0};
  asg(ia,ctest);
  assert(sum(ia)==6);
  

  /* test array equality */
  array<double> b(10);
  array<int> ib(10);
  asg(a,test); asg(b,test); b[0]=2; b[5]=7;
  asg(ia,itest); asg(ib,itest); ib[0]=2; ib[5]=7;
  assert(sum(a==a)==10);
  assert(sum(ia==ia)==10);
  assert(sum(a==b)==8);
  assert(sum(ia==ib)==8);
  assert(b[0]==2&&b[5]==7);
  assert(ib[0]==2&&ib[5]==7);

  /* broadcast and copy test */
  {
    array<double> a(10), b;
    array<int> ia(10), ib;
    a=2; ia=2;
    b=a; ib=ia;
    a[2]=2; ia[2]=2;
    for (i=0; i<10; i++) assert((double)(a[i])==2);
    for (i=0; i<10; i++) assert((int)(ia[i])==2);
  }
    
  /* test array assignment */

  int ibtest[]={10,9,8,7,4,6,4,3,2,1};
  double btest[]={10,9,8,7,4,6,4,3,2,1};

  b=a;   ib=ia;
  assert( sum(a==b) == 10 && sum(ia==ib) == 10);
  

  /* binary operators */
  array<double> c; array<int> ic;
  asg(a,test); asg(ia,itest);
  asg(b,btest); asg(ib,ibtest);

  c=a+b; ic=ia+ib;
  for (i=0; i<10; i++) assert( c[i]==a[i]+b[i] && ic[i]==ia[i]+ib[i] );

  c=a-b; ic=ia-ib;
  for (i=0; i<10; i++) assert( c[i]==a[i]-b[i] && ic[i]==ia[i]-ib[i] );

  c=a*b; ic=ia*ib;
  for (i=0; i<10; i++) assert( c[i]==a[i]*b[i] && ic[i]==ia[i]*ib[i] );

  c=a/b; ic=ia/ib;
  for (i=0; i<10; i++) 
    assert( fabs(c[i]-a[i]/b[i])<1e-15 && ic[i]==ia[i]/ib[i] );
  ic=ia%ib;
  for (i=0; i<10; i++) 
    assert(  ic[i]==ia[i]%ib[i] );
  

  c=a<<b; ic=ia<<ib;
  for (i=0; i<10; i++) assert( c[i]==a[i] && ic[i]==ia[i] );
  for (i=10; i<20; i++) assert( c[i]==b[i-10] && ic[i]==ib[i-10] );

  /* test logical operators */
  
  assert( sum(a<b)==4 && sum(ia<ib)==4);
  assert( sum(a<=b)==5 && sum(ia<=ib)==5);
  assert( sum(a>b)==5 && sum(ia>ib)==5);
  assert( sum(a>=b)==6 && sum(ia>=ib)==6);
  assert( sum(a!=b)==9 && sum(ia!=ib)==9);

  /* test broadcast ops */
  a=5; ia=1;
  assert(sum(a>b)==5 && sum(ia)==10);
  assert(sum(a==5)==10 && sum(ia==1)==10);
  assert(sum(5.0==a)==10 && sum(1==ia)==10);
  assert(sum(b+5.0==b+a)==10 && sum(ib+1==ib+ia)==10);
  assert(sum(b-5.0==b-a)==10 && sum(ib-1==ib-ia)==10);
  assert(sum(b*5.0==b*a)==10 && sum(ib*1==ib*ia)==10);
  // division by a constant is optimised, so equality is not quite preserved
  //  assert(sum(b/5.0==b/a)==10 && sum(ib/1==ib/ia)==10);
  assert(sum(abs(b/5.0-b/a))<1E-8);
  assert(sum(ib/1==ib/ia)==10);
  assert(sum((b<5.0)==(b<a))==10 && sum((ib<1)==(ib<ia))==10);
  assert(sum((b<=5.0)==(b<=a))==10 && sum((ib<=1)==(ib<=ia))==10);
  assert(sum((b>5.0)==(b>a))==10 && sum((ib>1)==(ib>ia))==10);
  assert(sum((b>=5.0)==(b>=a))==10 && sum((ib>=1)==(ib>=ia))==10);
  assert(sum((b!=5.0)==(b!=a))==10 && sum((ib!=1)==(ib!=ia))==10);
  assert(sum(5.0+b==a+b)==10 && sum(1+ib==ia+ib)==10);
  assert(sum(5.0-b==a-b)==10 && sum(1-ib==ia-ib)==10);
  assert(sum(5.0*b==a*b)==10 && sum(1*ib==ia*ib)==10);
  assert(sum(abs(5.0/b-a/b)<1e-5)==10 && sum(1/ib==ia/ib)==10);
  assert(sum((5.0<b)==(a<b))==10 && sum((1<ib)==(ia<ib))==10);
  assert(sum((5.0<=b)==(a<=b))==10 && sum((1<=ib)==(ia<=ib))==10);
  assert(sum((5.0>b)==(a>b))==10 && sum((1>ib)==(ia>ib))==10);
  assert(sum((5.0>=b)==(a>=b))==10 && sum((1>=ib)==(ia>=ib))==10);
  assert(sum((5.0!=b)==(a!=b))==10 && sum((1!=ib)==(ia!=ib))==10);
  a=b<<1; ia=ib<<1;
  assert(a[10]==1.0 && ia[10]==1);

  /* compound assignment */
  a.resize(10); ia.resize(10);
  asg(a,test); asg(b,test);
  asg(ia,itest); asg(ib,itest);
  c=a; ic=ib;
  c+=b; ic+=ib;
  assert(sum(c==a+b)==10 && sum(ic==ia+ib)==10 );
  c=a; ic=ib;
  c*=b; ic*=ib;
  assert(sum(c==a*b)==10 && sum(ic==ia*ib)==10 );
  c=a; ic=ib;
  c-=b; ic-=ib;
  assert(sum(c==a-b)==10 && sum(ic==ia-ib)==10 );
  c=a; ic=ib;
  c/=b; ic/=ib;
  assert(sum(c==a/b)==10 && sum(ic==ia/ib)==10 );
  c=a; ic=ib;
  c<<=b; ic<<=ib;
  assert(sum(c==a<<b)==20 && sum(ic==ia<<ib)==20);

  c=a; ic=ia;
  c+=2; ic+=2;
  assert(sum(c==a+2.0)==10 && sum(ic==ia+2)==10 );
  c=a; ic=ia;
  c*=2.0; ic*=2;
  assert(sum(c==a*2.0)==10 && sum(ic==ia*2)==10 );
  c=a; ic=ia;
  c-=2.0; ic-=2;
  assert(sum(c==a-2.0)==10 && sum(ic==ia-2)==10 );
  c=a; ic=ia;
  c/=2.0; ic/=2;
  assert(sum(c==a/2.0)==10 && sum(ic==ia/2)==10 );
  c=a; ic=ia;
  c<<=2.0; ic<<=2;
  assert(sum(c==a<<2.0)==11 && sum(ic==ia<<2)==11);

  c=a;
  c=-c;
  assert(all(c==-a));

  {
    int iresult[]={10,9,8,7,6,6,7,8,9,10}; array<int> ir(10); 
    int ibtest[]={10,9,8,7,6,5,4,3,2,1};
    double btest[]={10,9,8,7,6,5,4,3,2,1};
    asg(ia,itest); asg(ib,ibtest); asg(a,test); asg(b,btest); 
    ic=merge(ia>ib,ia,ib);

    asg(ir,iresult); assert(sum(ir!=ic)==0);
    c=array_ns::merge(a>b,a,b);
    assert(sum(ir!=c)==0);
    //assert(max(a,a<b)==5);
    assert(max(a)==10);
    //assert(max(ia,ia<ib)==5);
    assert(max(ia)==10);
    //assert(min(a,a>b)==6);
    assert(min(a)==1);
    //assert(min(ia,ia>ib)==6);
    assert(min(ia)==1);
    assert(sum(a,a<b)==15);
    assert(sum(ia,ia<ib)==15);
    assert(prod(a,a<b)==120);
    assert(prod(ia,ia<ib)==120);
    assert(sum(sign(a-b)!=merge(a>b,1,-1))==0);
    assert(sum(sign(ia-ib)!=merge(ia>ib,1,-1))==0);
    assert(sum(abs(a-b)*sign(a-b)==a-b));
    assert(sum(abs(ia-ib)*sign(ia-ib)==ia-ib));
  }


  /* test array indexing */
  int iresult[]={10,9,3,7,2,5,6,8,4,1}, index_data[]={9,8,2,6,1,4,5,7,3,0};
  double result[]={10,9,3,7,2,5,6,8,4,1};
  array<int> index(10);
  c.resize(10); ic.resize(10);

  asg(index,index_data);
  asg(a,test); asg(b,result); asg(ia,itest); asg(ib,iresult);
  c[index]=b; ic[index]=ib;
  assert(sum(a[index]==b)==10 && sum(ia[index]==b)==10 );
  assert(sum(c==a)==10 && sum(ic==ia)==10 );

  a[pcoord(10) % 5]+=1;
  assert(a[0]==test[0]+2 && a[5]==test[5]);

  /* test logicals */
  int ltest[]={0,1,0,1,0,1,0,1,0,1};
  int lresult[]={1,0,1,0,1,0,1,0,1,0};
  asg(ia,ltest); asg(ib,lresult);
  assert(sum((!ia)==ib)==10);

  assert(sum((ia&&ia)==ia)==10 && sum((ia&&ib)==0)==10);
  assert(sum((ia||ia)==ia)==10 && sum((ia||ib)==1)==10);

  /* test type conversion */

  asg(a,test); asg(ib,itest); 
  ia=a;
  assert(sum(ia==ib)==10);
  b=ia;
  assert(sum(a==b)==10);
  ia+=a;
  assert(sum(ia==2*ib)==10);
  assert(sum(a*ib==a*b)==10);
  
  /* test comms routines */

  int ipresult[]={2,4,6,8,10};
  ic.resize(5); c.resize(5);
  ib.resize(10); asg(ib,ltest);
  asg(ia,itest); asg(a,test);
  ic=pack(ia,ib); c=pack(a,ib);
  ib.resize(5);
  asg(ib,ipresult);
  assert(sum(c==ib)==5 && sum(ic==ib)==5);

  int ieresult[]={0,0,1,1,2,2,3,3,4,4};
  ic.resize(10); asg(ic,ieresult);
  ia.resize(10); asg(ia,ltest);
  ib.resize(10);
  ib=enumerate(ia);
  assert(sum(ib==ic)==10);

  {
    int testx[]={0,0,1,2,0,1}, testr[]={2,3,3,5};
    array<int> a(6), b(4);
    asg(a,testx); asg(b,testr);
    assert(size_t(sum(gen_index(a)==b))==b.size());
  }

  {
    sparse_mat a(2,2);
    array<int> b(2); array<double> c(2);
    a.diag[0]=1; a.diag[1]=2; a.val[0]=1; a.val[1]=2; 
    a.row[0]=0;a.row[1]=1;a.col[0]=1;a.col[1]=0;
    b[0]=1; b[1]=2;
    c[a.row] = a.val * b[a.col];
    c += a.diag*b;
    assert(sum(a*b == c )==2);
  }

  /* test sorting */
  {
    int testa[]={4,3,5,2};
    double testb[]={4,3,5,2};
    int resultf[]={3,1,0,2};
    int resultr[]={2,0,1,3};
    array<int> a(4),r(4); array<double> b(4);
    asg(a,testa); asg(b,testb); asg(r,resultf);
    assert(sum(rank(a)!=r)==0);
    assert(sum(rank(b)!=r)==0);
    asg(r,resultr);
    assert(sum(rank(a,array_ns::downwards)!=r)==0);
    assert(sum(rank(b,array_ns::downwards)!=r)==0);
  }

  /* log, exp etc */
  {
    array<double> a(5), b(5), c(5);
    double atest[]={1,2,3,4,5};
    asg(a,atest); b=log(a); c=exp(b);
    for (int i=0; i<5; i++)
      //      printf("%g %g %g %g\n",b[i],log(a[i]),c[i],exp(b[i]));
      { assert(near(b[i],log(a[i]))); assert(near(c[i],exp(b[i])));}
  }
  return 0;
}

