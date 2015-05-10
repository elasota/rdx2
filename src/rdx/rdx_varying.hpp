#ifndef __RDX_VARYING_HPP__
#define __RDX_VARYING_HPP__

#include "rdx_reftypedefs.hpp"

// WARNING: rdxILOP_tovarying_static depends on the order here
struct rdxSVarying
{
	typedef void super;

	rdxWeakTypelessOffsetRTRef	rtp;
	rdxWeakRTRef(rdxCType)		type;

	void VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable);
};
RDX_DECLARE_COMPLEX_NATIVE_STRUCT(rdxSVarying);


#endif
