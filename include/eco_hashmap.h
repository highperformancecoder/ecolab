/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/* default is to define hash_maps in terms of standard C++ maps:
ifdef HASH_hash_map, traditional STL hash_maps are used
   */


#ifndef ECO_HASH_MAP_H
#define ECO_HASH_MAP_H

#if defined(HASH_hash_map)
#  if (defined(__GNUC__) && __GNUC__>=3)
#   include <backward/hash_map.h>
#  else
#   include <hash_map>
    using std::hash_map;
#  endif  /* __GNUC__ */
#else
#  include <map>
namespace ecolab
{
   template <class K, class T> class hash_map: public std::map<K,T> {};
}
#endif  /* HASH_* */

#endif /* ECO_HASH_MAP_H */
