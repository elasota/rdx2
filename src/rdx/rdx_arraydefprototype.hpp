#ifndef __RDX_ARRAYDEFPROTOTYPE_HPP__
#define __RDX_ARRAYDEFPROTOTYPE_HPP__

#include "rdx_coretypes.hpp"
#include "rdx_objectguid.hpp"

struct rdxSArrayDefPrototype
{
	rdxSObjectGUID				containedTypeGUID;
	rdxLargeUInt				numDimensions;
	bool						isConstant;
	rdxLargeUInt				tableIndex;
};

#endif
