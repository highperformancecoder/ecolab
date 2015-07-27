/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef JAVACLASS_EPILOGUE_H
#define JAVACLASS_EPILOGUE_H

namespace classdesc
{
  template <class T>
  typename enable_if< is_leftOver<T>, void>::T
  javaClass(javaClass_t& cl, const string& desc, T& arg, dummy<0> d)
  {classdesc_access::access_javaClass<T>()(cl, desc, arg);}

}
#endif
