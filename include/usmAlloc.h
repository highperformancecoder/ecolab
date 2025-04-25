/*
  @copyright Russell Standish 2025
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef ECOLAB_USMALLOC_H
#define ECOLAB_USMALLOC_H
namespace ecolab
{
  // this type of macro stuff doesn't play nice with classdesc, hence why
  // it is a separate header
#ifdef SYCL_LANGUAGE_VERSION
  using USMAlloc=sycl::usm::alloc;
#else
  enum class USMAlloc {host, shared, device, unknown};
#endif
}
#endif
