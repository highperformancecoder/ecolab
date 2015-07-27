/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

struct model: 
  public ecolab::cachedDBM<int,int>, 
  public classdesc::TCL_obj_t 
{
  // compile test
  void test1()
  {
    (*this)[0]=1;
    commit();
    del(0);
    for (firstkey(); !eof(); nextkey()) ;

    for (KeyValueIterator i=begin(); i!=end(); ++i);
    for (KeyIterator i=keys.begin(); i!=keys.end(); ++i);
 
    // finally
    classdesc::pack_t b;
    pack(b);
  }
};
