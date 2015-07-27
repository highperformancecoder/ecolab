/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "sparse_mat.h"
#include "ecolab_epilogue.h"

namespace ecolab
{
  using array_ns::array;
  using array_ns::pcoord;

  /** submatrix extraction and insertion -- closest thing to full vector
      indexing of sparse matrices -- the latter is too computationally
      expensive, and not needed */

  sparse_mat sparse_mat::submat(unsigned min, unsigned max)
  {
    sparse_mat r;
    array<bool> mask;

    /*  
        mask=pcoord(max-min)+min;
        r.diag = diag[mask];
        mask = row >= min && row < max && col >=min && col < max;
        int ntrue = sum(mask);
        r.rowsz=r.colsz=max-min;
        r.val = pack(val, mask, ntrue);
        r.row = pack(row, mask, ntrue)-min;
        r.col = pack(col, mask, ntrue)-min;
    */

    /* tuned version below */
    size_t i, j, ntrue;
    r.diag.resize(max-min);
    for (i=min; i<max; i++) r.diag[i-min]=diag[i];
    for (ntrue=i=0; i<row.size(); i++)
      if (row[i] >= min && row[i] < max && col[i] >=min && col[i] < max)
        ntrue++;
    r.row.resize(ntrue); r.col.resize(ntrue); r.val.resize(ntrue);
    r.rowsz=r.colsz=max-min;
    for (j=i=0; i<row.size(); i++)
      if (row[i] >= min && row[i] < max && col[i] >=min && col[i] < max)
        {
          r.row[j]=row[i]-min;
          r.col[j]=col[i]-min;
          r.val[j]=val[i];
          j++;
        }
    assert(sum(r.row>=r.diag.size())==0 && sum(r.col>=r.diag.size())==0);
    return r;
  }

  /** block diagonal submatrix insertion */

  void sparse_mat::insert(const sparse_mat& x, unsigned where, unsigned old_size)
  {
    assert(sum(x.row>=x.diag.size())==0 && sum(x.col>=x.diag.size())==0);
    array<int> slice1, slice2, mask;

    slice1 = pcoord(where); 
    slice2 = pcoord(diag.size()-where-old_size) + where + old_size;

    diag = diag[slice1] << x.diag << diag[slice2];

    /* cut out old submatrix */
    slice1 = row < where &&  col < where;
    slice2 = row >= where + old_size && col >= where + old_size;
    int adjust = x.diag.size() - old_size; 
    int ntrue1 = sum(slice1);
    int ntrue2 = sum(slice2);

    val = pack(val,slice1,ntrue1) << pack(val,slice2,ntrue2) << x.val;
    row = pack(row,slice1,ntrue1) << pack(row,slice2,ntrue2)+adjust <<
      x.row+where;
    col = pack(col,slice1,ntrue1) << pack(col,slice2,ntrue2)+adjust <<
      x.col+where;
  }

  /* create off diagonal elements with the connectivity per row have
     average conn and standard deviation sigma - assume diagonal already
     assigned */

  void sparse_mat::init_rand(unsigned conn, double sigma)
  {
    assert(sigma>=0);
    assert(conn<diag.size());

    array<int> rsize(diag.size()); array<double> tmp(diag.size());
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
