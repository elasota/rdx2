#include "rdx_varying.hpp"

void rdxSVarying::VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
{
	if(visitNonSerializable)
	{
		visitor->VisitReference(objm, this->rtp);
		visitor->VisitReference(objm, this->type);
	}
}
