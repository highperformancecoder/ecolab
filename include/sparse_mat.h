/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief Sparse matrices
*/
#ifndef SPARSE_MAT_H
#define SPARSE_MAT_H

#include <arrays.h>

namespace ecolab
{
  /// sparse matrix class
  template <class F, template<class> class A=std::allocator>
  class sparse_mat
  {
  public:
    /* row and column dimension of matrix (used if diag.size==0) */
    unsigned rowsz, colsz;
    /*diagonal and values of offdiagonal*/
    array_ns::array<F, A<F>> diag, val;
    /*row and columns of offdiagonal elements*/
    array_ns::array<unsigned, A<unsigned>> row, col;
    sparse_mat(int s=0, int o=0, const A<F>& falloc={},
               const A<unsigned>& ialloc={}): rowsz(s), colsz(s),
                                              diag(s,falloc), val(o,falloc),
                                              row(o,ialloc), col(o,ialloc) {} 
    sparse_mat(const sparse_mat& x) 
    {
      rowsz=x.rowsz; colsz=x.colsz; 
      diag=x.diag; val=x.val; 
      row=x.row; col=x.col;
    }
    void setAllocators(const A<unsigned>& ialloc, const A<F>& falloc) {
      diag.allocator(falloc);
      val.allocator(falloc);
      row.allocator(ialloc);
      col.allocator(ialloc);
    }

    /*matrix multiplication*/
    template <class E> typename
    array_ns::enable_if
    <
      array_ns::is_expression<E>,
      array_ns::array
      <
        F,
        typename array_ns::MakeAllocator<F,typename E::Allocator>::type
        >
      >::T
    operator*(const E& x) const
    {
      auto alloc=array_ns::makeAllocator<F>(x.allocator());
      array_ns::array<F,decltype(alloc)> r(alloc);
      assert(row.size()==col.size() && row.size()==val.size());
      if (diag.size()>0)
	{
	  assert(diag.size()==x.size());
	  r = diag*x;
          assert(r.size()==x.size());
	}
      else
	r.resize(rowsz,0);
      assert(r.size()>max(row) && x.size()>max(col));
      r[row]+=val*x[col];
      
      return r;
    }
    /* initialise sparse mat in a random configuration */
    void init_rand(unsigned conn, double sigma);
  };

  /// transpose matrix
  template <class F, template<class> class A>
  inline sparse_mat<F,A> tr(const sparse_mat<F,A>& x)
  {
    sparse_mat<F,A> r=x;
    std::swap(r.row,r.col);
    std::swap(r.rowsz,r.colsz);
    return r;
  }

  /* create off diagonal elements with the connectivity per row have
     average conn and standard deviation sigma - assume diagonal already
     assigned */

  template <class F, template<class> class A>
  void sparse_mat<F,A>::init_rand(unsigned conn, double sigma)
  {
    assert(sigma>=0);
    assert(conn<diag.size());

    array<int,A<int>> rsize(diag.size(),row.allocator());
    array<F,A<F>> tmp(diag.size(),diag.allocator());
    fillgrand(tmp);
    tmp=tmp*sigma+(double)conn;
    rsize=merge(tmp>0,tmp,0.0);  /* assume that sigma is relatively 
                                    smaller than conn */
    row.resize(0); col.resize(0);  /* initialise to empty arrays */
    array<int> r, c, mask; 
    for (unsigned i=0; i<diag.size(); i++)
      {
        r.resize(rsize[i]);
        c.resize(rsize[i]);
        r=i; array_ns::fill_unique_rand(c,diag.size());
        mask=c!=r; 
        row<<=pack(r,mask); col<<=pack(c,mask);
      }
    val.resize(row.size(),0);   /* ensure sanity */
  }



}

#include "sparse_mat.cd"
#endif
