#ifndef __RDX_SSIDTABLE_HPP__
#define __RDX_SSIDTABLE_HPP__

#include "rdx_coretypes.hpp"

struct rdxGCInfo;

template<class Tkey, class TlookupType> class rdxCStaticLookupTable;
template<class Tvalue> class rdxCStaticLookupPODKey;

typedef rdxCStaticLookupTable<rdxCStaticLookupPODKey<rdxGCInfo*>, rdxLargeUInt> rdxCSSIDTable;

#endif
