#ifndef __RDX_LOADSHELL_HPP__
#define __RDX_LOADSHELL_HPP__

#include "rdx_coretypes.hpp"
#include "rdx_objectguid.hpp"
#include "rdx_typeprocessor.hpp"

struct rdxSLoadShell
{
	rdxLargeUInt					fileOffset;
	bool							isAnonymous;
	bool							isConstant;
	bool							isCloaked;
	rdxSObjectGUID					typeName;
	rdxLargeUInt					tableIndex;
	rdxLargeUInt					numElements;
};
RDX_DECLARE_COMPLEX_NATIVE_STRUCT(rdxSLoadShell);

#endif
