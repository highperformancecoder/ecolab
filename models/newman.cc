/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "ecolab.h"
#include "arrays.h"
#include "newman_model.h"
#include "newman_model.cd"
#include "ecolab_epilogue.h"

newman_model newman;
make_model(newman);

void newman_model::step()
{
  array<int> new_sp, odensity;
  array<double> new_thresh, new_intr;
  double stress;

  if (tstep==0) make_consistent();

  odensity=density;

  /* perform extinctions */
  stress = density.size()*pstress->rand();
  density = (stress < threshold) * odensity;
  tstep++;

  /* get species that mutate */
  new_sp = mutation * odensity;

  /* generate index list of old species that mutate to the new */
  new_sp = gen_index(new_sp); 
  if (new_sp.size()==0) return;

  new_thresh.resize(new_sp.size());
  fillrand(new_thresh);

  new_sp = 1;
  density <<= new_sp;
  species <<= sp_cntr + pcoord(new_sp.size());
  new_sp=tstep;
  create <<= new_sp;
  threshold <<= new_thresh;
}
  
void newman_model::condense()
{
  array<int> mask;
  mask = density != 0;
  unsigned mask_true=sum(mask);
  if (density.size()==mask_true) return; /* no change ! */
  density = pack(density, mask,mask_true);
  species = pack(species, mask,mask_true);
  create = pack(create, mask,mask_true);
  threshold = pack(threshold, mask,mask_true);
}


array<int> newman_model::lifetimes() 
{ 
  array<int> lifetimes; 

  for (size_t i=0; i<density.size(); i++) 
    {
      if (create[i]==0 && density[i]>0) create[i]=tstep;
      else if (create[i]>0 && density[i]==0) 
	/* extinction */
	{
	  lifetimes <<= tstep - create[i];
	  create[i]=0;
	}
    }
  return lifetimes;
}


