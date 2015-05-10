#ifndef __RDX_RUNTIMESTACKVALUE_HPP__
#define __RDX_RUNTIMESTACKVALUE_HPP__

#include "rdx_coretypes.hpp"
#include "rdx_reftypedefs.hpp"

union rdxURuntimeStackValue
{
	rdxBaseHdl::PODType asHdl;
	rdxOffsetHdlPOD asOffsetHdl;
	rdxDouble asDouble;
	rdxLargeInt asLargeInt;
	rdxEnumValue asEnumValue;
	rdxShort asShort;
	rdxChar asChar;
	rdxByte asByte;
	rdxInt asInt;
	rdxLong asLong;
};

static const rdxLargeUInt rdxALIGN_RuntimeStackValue	= rdxAlignOf(rdxURuntimeStackValue);

#endif
