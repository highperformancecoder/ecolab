/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef CLASSDESC_ACCESS
/**\file
\brief descriptor access to a class's privates
*/

namespace classdesc
{
  template <class T> struct base_cast;
}

namespace classdesc_access
{
  template <class T> struct access_pack;
  template <class T> struct access_unpack;
  template <class T> struct access_xml_pack;
  template <class T> struct access_xml_unpack;
  template <class T> struct access_json_pack;
  template <class T> struct access_json_unpack;
  template <class T> struct access_random_init;
}

/// add friend statements for each accessor function 
/** 
  This declaration is necessary for classes with private/protected
  parts and descriptors where -respect_private has not been specified
  @param type the name of the enclosing class
 */
#define CLASSDESC_ACCESS(type)                                          \
  template <class _CD_ARG_TYPE> friend struct classdesc::base_cast;     \
  friend struct classdesc_access::access_pack<type>;                    \
  friend struct classdesc_access::access_unpack<type>;                  \
  friend struct classdesc_access::access_xml_pack<type>;                \
  friend struct classdesc_access::access_xml_unpack<type>;              \
  friend struct classdesc_access::access_json_pack<type>;               \
  friend struct classdesc_access::access_json_unpack<type>;             \
  friend struct classdesc_access::access_random_init<type>

// for backward compatibility. Deprecated.
#define CLASSDESC_ACCESS_TEMPLATE(type) CLASSDESC_ACCESS(type)

#endif
