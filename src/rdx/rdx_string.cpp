#include "rdx_objectmanagement.hpp"


RDX_IMPLEMENT_COMPLEX_NATIVE_CLASS(rdxCString, (rdxETIF_VisitReferences));

void rdxCString::VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
{
	if(visitNonSerializable)
		visitor->VisitReference(objm, this->m_native.characters);
}
