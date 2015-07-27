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
  class sparse_mat
  {
  public:
    /* row and column dimension of matrix (used if diag.size==0) */
    unsigned rowsz, colsz;
    /*diagonal and values of offdiagonal*/
    array_ns::array<double> diag, val;
    /*row and columns of offdiagonal elements*/
    array_ns::array<unsigned> row, col;
    sparse_mat(int s=0, int o=0): rowsz(s), colsz(s), diag(s), val(o), row(o), col(o) {} 
    sparse_mat(const sparse_mat& x) 
    {
      rowsz=x.rowsz; colsz=x.colsz; 
      diag=x.diag; val=x.val; 
      row=x.row; col=x.col;
    }
    /*matrix multiplication*/
    template <class E> typename
    array_ns::enable_if< array_ns::is_expression<E>, array_ns::array<double> >::T
    operator*(const E& x)  
    {
      array_ns::array<double> r;
      assert(row.size()==col.size() && row.size()==val.size());      
      if (diag.size()>0)
	{
	  assert(diag.size()==x.size());
	  r = diag*x; 
	}
      else
	r.resize(rowsz,0);
      r[row]+=val*x[col];
      return r;
    }
    /* not quite sure how to do this yet!
       sparse_mat operator*(sparse_mat x)
       {
    */
    /*assignment*/
    sparse_mat& operator=(const sparse_mat& x) 
    {
      rowsz=x.rowsz; colsz=x.colsz; 
      diag=x.diag; val=x.val; 
      row=x.row; col=x.col; 
      return *this;
    }
    /*return the submatrix bounded by #min# and #max#*/
    sparse_mat submat(unsigned min, unsigned max);
    /* submatrix extraction and insertion */
    void insert(const sparse_mat& x, unsigned where, unsigned old_size);

    /* initialise sparse mat in a random configuration */
    void init_rand(unsigned conn, double sigma);
  };

  /// transpose matrix
  inline sparse_mat tr(const sparse_mat& x)
  {
    sparse_mat r=x;
    std::swap(r.row,r.col);
    std::swap(r.rowsz,r.colsz);
    return r;
  }
}

#include "sparse_mat.cd"
#endif
