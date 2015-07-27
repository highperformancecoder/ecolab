/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef TYPENAME_EPILOGUE_H
#define TYPENAME_EPILOGUE_H

namespace classdesc
{
  // generically handle new integral types
  template <class T> typename 
  enable_if< Not<is_const<T> >,std::string>::T  
  integralTypeName() 
  {
    std::ostringstream os;
    os<<CHAR_BIT*sizeof(T);
    return (is_unsigned<T>::value?"unsigned int":"int")+os.str()+"_t";
  }

//  template <class T> typename 
//  enable_if< is_const<T>, std::string>::T  integralTypeName() 
//  {return "const "+integralTypeName<typename std::remove_const<T>::type>();}

  template <class T> typename
  enable_if<And<Not<is_const<T> >,is_integral<T> >,std::string>::T
  typeNamep() {return integralTypeName<T>();}

  template <class T> typename
  enable_if<And<Not<is_const<T> >,Not<is_integral<T> > >,std::string>::T
  typeNamep() {return tn<T>::name();}

  template <class T> typename
  enable_if<is_const<T>,std::string>::T
  typeNamep() {return "const "+typeName<typename remove_const<T>::type>();}

  template <class T> std::string typeName() {return typeNamep<T>();}

//  template <class T>
//  struct tn<const T>
//  {
//    static std::string name() {return "const "+typeName<T>();}
//  };
}
#endif
