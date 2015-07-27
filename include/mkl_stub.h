/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef MKL_STUB_H
#define MKL_STUB_H
#ifdef MKL_STUB
#ifdef MASS
#include <massv.h>
inline void vdInv(int n, double *x, double *y) {for (int i=0; i<n; i++) y[i]=1/x[i];}
inline void vdSqrt(int n, double *x, double *y) {vsqrt(y,x,&n);}
inline void vdExp(int n, double *x, double *y) {vexp(y,x,&n);}
inline void vdLn(int n, double *x, double *y) {vlog(y,x,&n);}
inline void vdCos(int n, double *x, double *y) {vcos(y,x,&n);}
inline void vdSin(int n, double *x, double *y) {vsin(y,x,&n);}
#else
inline void vdDiv(int n, const double *x, const double *y, double *z) {
  for (int i=0; i<n; i++) z[i]=x[i]/y[i];}
inline void vdPow(int n, const double *x, const double *y, double *z) {
  for (int i=0; i<n; i++) z[i]=pow(x[i],y[i]);}
inline void vdPowx(int n, const double *x, double y, double *z) {
  for (int i=0; i<n; i++) z[i]=pow(x[i],y);}
inline void vdInv  (int n, const double *x, double *y) {for (int i=0; i<n; i++) y[i]=1/x[i];}
inline void vdSqrt (int n, const double *x, double *y) {for (int i=0; i<n; i++) y[i]=sqrt(x[i]);}
inline void vdExp  (int n, const double *x, double *y) {for (int i=0; i<n; i++) y[i]=exp(x[i]);}
inline void vdLn   (int n, const double *x, double *y) {for (int i=0; i<n; i++) y[i]=log(x[i]);}
inline void vdLog10(int n, const double *x, double *y) {for (int i=0; i<n; i++) y[i]=log10(x[i]);}
inline void vdCos  (int n, const double *x, double *y) {for (int i=0; i<n; i++) y[i]=cos(x[i]);}
inline void vdSin  (int n, const double *x, double *y) {for (int i=0; i<n; i++) y[i]=sin(x[i]);}
inline void vdTan  (int n, const double *x, double *y) {for (int i=0; i<n; i++) y[i]=tan(x[i]);}
inline void vdSinh (int n, const double *x, double *y) {for (int i=0; i<n; i++) y[i]=sinh(x[i]);}
inline void vdCosh (int n, const double *x, double *y) {for (int i=0; i<n; i++) y[i]=cosh(x[i]);}
inline void vdTanh (int n, const double *x, double *y) {for (int i=0; i<n; i++) y[i]=tanh(x[i]);}
inline void vdAsin (int n, const double *x, double *y) {for (int i=0; i<n; i++) y[i]=asin(x[i]);}
inline void vdAcos (int n, const double *x, double *y) {for (int i=0; i<n; i++) y[i]=acos(x[i]);}
inline void vdAtan (int n, const double *x, double *y) {for (int i=0; i<n; i++) y[i]=atan(x[i]);}
inline void vdAtan2(int n, const double *x, const double *y, double *z) {
  for (int i=0; i<n; i++) z[i]=atan2(x[i],y[i]);}

inline void vsDiv(int n, const float *x, const float *y, float *z) {
  for (int i=0; i<n; i++) z[i]=x[i]/y[i];}
inline void vsPow(int n, const float *x, const float *y, float *z) {
  for (int i=0; i<n; i++) z[i]=pow(x[i],y[i]);}
inline void vsPowx(int n, const float *x, float y, float *z) {
  for (int i=0; i<n; i++) z[i]=pow(x[i],y);}
inline void vsInv  (int n, const float *x, float *y) {for (int i=0; i<n; i++) y[i]=1/x[i];}
inline void vsSqrt (int n, const float *x, float *y) {for (int i=0; i<n; i++) y[i]=sqrt(x[i]);}
inline void vsExp  (int n, const float *x, float *y) {for (int i=0; i<n; i++) y[i]=exp(x[i]);}
inline void vsLn   (int n, const float *x, float *y) {for (int i=0; i<n; i++) y[i]=log(x[i]);}
inline void vsLog10(int n, const float *x, float *y) {for (int i=0; i<n; i++) y[i]=log10(x[i]);}
inline void vsCos  (int n, const float *x, float *y) {for (int i=0; i<n; i++) y[i]=cos(x[i]);}
inline void vsSin  (int n, const float *x, float *y) {for (int i=0; i<n; i++) y[i]=sin(x[i]);}
inline void vsTan  (int n, const float *x, float *y) {for (int i=0; i<n; i++) y[i]=tan(x[i]);}
inline void vsSinh (int n, const float *x, float *y) {for (int i=0; i<n; i++) y[i]=sinh(x[i]);}
inline void vsCosh (int n, const float *x, float *y) {for (int i=0; i<n; i++) y[i]=cosh(x[i]);}
inline void vsTanh (int n, const float *x, float *y) {for (int i=0; i<n; i++) y[i]=tanh(x[i]);}
inline void vsAsin (int n, const float *x, float *y) {for (int i=0; i<n; i++) y[i]=asin(x[i]);}
inline void vsAcos (int n, const float *x, float *y) {for (int i=0; i<n; i++) y[i]=acos(x[i]);}
inline void vsAtan (int n, const float *x, float *y) {for (int i=0; i<n; i++) y[i]=atan(x[i]);}
inline void vsAtan2(int n, const float *x, const float *y, float *z) {
  for (int i=0; i<n; i++) z[i]=atan2(x[i],y[i]);}
#endif
#endif
#endif
