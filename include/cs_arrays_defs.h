/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#define C_defs(name,type)\
void *new_##name(int s)\
{\
  name *r;\
  r=malloc(sizeof(*r));\
  r->size=s;\
  if (s==0) return;\
  r->sh = (shape*) malloc(sizeof(*r->sh));\
  allocate_shape(r->sh,1,s);\
  r->list=palloc(*r->sh,sizeof(type));\
  return r;\
}\
void delete_##name(name *x) \
{\
  if (x==NULL) return;\
  if (x->size!=0)  \
    {\
       pfree(x->list); \
       deallocate_shape(x->sh);\
       free(x->sh);\
    }\
  free(x);\
}\
type get_##type(name *x,int i) {return [i](*x->list);}\
void put_##type(name *x,int i,type y) {[i](*x->list)=y;}\
void copy_##type(name *x,name *y) \
{ \
  if (x->size==0) return;\
  with (*x->sh) *x->list=*y->list;\
}\
/* vector indexing */\
void get_##type##_array(name* r,name* x,iarray* i)\
{\
  if (r->size==0) return;\
  with (*r->sh)\
    *r->list = [*i->list](*x->list);\
}\
void put_##type##_array(name* x,iarray* i,name* y)\
{\
  if (y->size==0) return;\
  with (*y->sh)\
    [*i->list](*x->list) = *y->list;\
}\
void broadcast_##type(name* x,type y)\
{\
  if (x->size==0) return;\
  with(*x->sh)\
    *x->list=y;\
}\
void name##_plus(name* r,name* x,name* y)\
{\
  if (r->size==0) return;\
  with (*r->sh)\
    *r->list = *x->list + *y->list;\
}\
void name##_minus(name* r,name* x,name* y)\
{\
  if (r->size==0) return;\
  with (*r->sh)\
    *r->list = *x->list - *y->list;\
}\
void name##_mul(name* r,name* x,name* y)\
{\
  if (r->size==0) return;\
  with (*r->sh)\
    *r->list = *x->list * *y->list;\
}\
void name##_div(name* r,name* x,name* y)\
{\
  if (r->size==0) return;\
  with (*r->sh)\
    *r->list = *x->list / *y->list;\
}\
void name##_cat(name* r,name* x,name* y)\
{\
  if (x->size!=0) \
    {\
       with (*x->sh)\
	 [pcoord(0)]*r->list = *x->list;\
    }\
  if (y->size==0) return;\
  with (*y->sh)\
    [pcoord(0)+x->size]*r->list = *y->list;\
}\
void name##_lt(iarray *r, name *x,name*y)\
{\
  if (r->size==0) return;\
  with (*r->sh)\
    *r->list = *x->list < *y->list;\
}\
void name##_le(iarray *r, name *x, name *y)\
{\
  if (r->size==0) return;\
  with (*r->sh)\
    *r->list = *x->list <= *y->list;\
}\
void name##_gt(iarray *r, name  *x,name *y)\
{\
  if (r->size==0) return;\
  with (*r->sh)\
    *r->list = *x->list > *y->list;\
}\
void name##_ge(iarray *r, name  *x,name *y)\
{\
  if (r->size==0) return;\
  with (*r->sh)\
    *r->list = *x->list >= *y->list;\
}\
void name##_eq(iarray  *r,name *x, name *y)\
{\
  if (r->size==0) return;\
  with (*r->sh)\
    *r->list = *x->list == *y->list;\
}\
void name##_ne(iarray  *r, name *x, name *y)\
{\
  if (r->size==0) return;\
  with (*r->sh)\
    *r->list = *x->list != *y->list;\
}\

C_defs(array,double)
C_defs(iarray,int)
