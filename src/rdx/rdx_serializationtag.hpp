#ifndef __RDX_SERIALIZATIONTAG_HPP__
#define __RDX_SERIALIZATIONTAG_HPP__

#include "rdx_coretypes.hpp"
#include "rdx_objectguid.hpp"

class rdxCPackage;

struct rdxSSerializationTag
{
	bool						isAnonymous;
	rdxSObjectGUID				gstSymbol;
	rdxTracedRTRef(rdxCPackage)	package;
	rdxLargeUInt				packageManifestOffset;
};

#endif
