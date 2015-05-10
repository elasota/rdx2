#include "rdx_ilcomp.hpp"
#include "rdx_package.hpp"
#include "rdx_varying.hpp"


rdxCRuntimeThread::rdxCRuntimeThread(rdxIObjectManager *objm, rdxGCInfo *info)
	: rdxCObject(objm, info)
{
}

void rdxCRuntimeThread::DeserializationState::VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
{
}

void rdxCRuntimeThread::DeserializationState::StoredJournal::VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
{
}

void rdxCRuntimeThread::DeserializationState::StoredFrame::VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
{
}


rdxCPackage::rdxCPackage(rdxIObjectManager *objm, rdxGCInfo *info)
	: rdxCObject(objm, info)
{
}

void rdxSVarying::VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
{
	visitor->VisitReference(objm, this->rtp);
	visitor->VisitReference(objm, this->type);
}

//////////////////////////////////////////////////////////////////////
RDX_IMPLEMENT_COMPLEX_NATIVE_CLASS(rdxCException, (rdxETIF_NoFlags));

RDX_BEGIN_PROPERTY_LOOKUP_CLASS(rdxCException)
	RDX_DEFINE_LOOKUP_PROPERTY(innerException)
RDX_END_PROPERTY_LOOKUP

rdxCException::rdxCException(rdxIObjectManager *objm, rdxGCInfo *info)
	: rdxCObject(objm, info)
{
}


//////////////////////////////////////////////////////////////////////
RDX_IMPLEMENT_COMPLEX_NATIVE_CLASS(rdxCRuntimeThread, rdxETIF_VisitReferences);

RDX_BEGIN_PROPERTY_LOOKUP_CLASS(rdxCRuntimeThread)
RDX_END_PROPERTY_LOOKUP

//////////////////////////////////////////////////////////////////////
RDX_IMPLEMENT_COMPLEX_NATIVE_STRUCT(rdxCRuntimeThread::DeserializationState, rdxETIF_VisitReferences);
RDX_IMPLEMENT_COMPLEX_NATIVE_STRUCT(rdxCRuntimeThread::DeserializationState::StoredJournal, rdxETIF_VisitReferences);
RDX_IMPLEMENT_COMPLEX_NATIVE_STRUCT(rdxCRuntimeThread::DeserializationState::StoredFrame, rdxETIF_VisitReferences);

RDX_IMPLEMENT_COMPLEX_NATIVE_STRUCT(rdxSVarying, (rdxETIF_VisitReferences));
