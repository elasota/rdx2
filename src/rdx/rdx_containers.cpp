#include "rdx_reftypedefs.hpp"
#include "rdx_builtins.hpp"

void rdxCStructContainer::VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
{
	rdxDebugBreak(rdxBREAKCAUSE_Unimplemented);
}

void rdxCStructContainer::InitializeContents(rdxIObjectManager *objm, rdxWeakRTRef(rdxCStructuredType) st)
{
	// TODO MUSTFIX
}

class rdxXAPI_ArrayContainer_Shim
{
public:
	RDX_FORCEINLINE static void VisitReferences(rdxCArrayContainer *v, rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
	{
		v->VisitReferences_Local(objm, visitor, visitNonSerializable);
	}

	RDX_FORCEINLINE static void ComputeContainerSize(rdxSOperationContext *ctx, rdxLargeUInt elementSize, rdxLargeUInt numElements, rdxLargeUInt numDimensions, rdxLargeUInt *outDimensionsOffset, rdxLargeUInt *outSize)
	{
		rdxCArrayContainer::ComputeContainerSize_Local(ctx, elementSize, numElements, numDimensions, outDimensionsOffset, outSize);
	}
};

RDX_DYNLIB_API void rdxXAPI_ArrayContainer_VisitReferences(rdxCArrayContainer *v, rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
{
	rdxXAPI_ArrayContainer_Shim::VisitReferences(v, objm, visitor, visitNonSerializable);
}

RDX_DYNLIB_API void rdxXAPI_ArrayContainer_ComputeContainerSize(rdxSOperationContext *ctx, rdxLargeUInt elementSize, rdxLargeUInt numElements, rdxLargeUInt numDimensions, rdxLargeUInt *outDimensionsOffset, rdxLargeUInt *outSize)
{
	rdxXAPI_ArrayContainer_Shim::ComputeContainerSize(ctx, elementSize, numElements, numDimensions, outDimensionsOffset, outSize);
}

void rdxCArrayContainer::VisitReferences_Local(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
{
	rdxUInt8 *byteBase = reinterpret_cast<rdxUInt8*>(ModifyRawData());
	rdxLargeUInt stride = m_stride;
	rdxLargeUInt numElements = m_numElements;
	rdxIfcTypeInfo typeInfo = m_contentsTypeInfo;

	rdxWeakRTRef(rdxCStructuredType) structureST;
	rdxWeakRTRef(rdxCArrayOfType) arrayContainerType = this->ObjectInfo()->containerType.ToWeakRTRef().StaticCast<rdxCArrayOfType>();
	bool isCRefArray = false;
	bool isIRefArray = false;
	bool isStructArray = false;
	bool isPODArray = false;
	bool isUnknownTypeArray = true;
	if(arrayContainerType.IsNotNull())
	{
		rdxWeakRTRef(rdxCType) subscriptType = arrayContainerType->type.ToWeakRTRef();
		if(subscriptType.IsNotNull())
		{
			isUnknownTypeArray = false;

			rdxWeakRTRef(rdxCType) subscriptTypeType = subscriptType->ObjectInfo()->containerType.ToWeakRTRef();
			if(subscriptTypeType == objm->GetBuiltIns()->st_StructuredType)
			{
				rdxWeakRTRef(rdxCStructuredType) subscriptST = subscriptType.StaticCast<rdxCStructuredType>();
				if(subscriptST->storageSpecifier == rdxSS_RefStruct || subscriptST->storageSpecifier == rdxSS_ValStruct)
				{
					structureST = subscriptST;
					isStructArray = true;
				}
				else if(subscriptST->storageSpecifier == rdxSS_Class)
					isCRefArray = true;
				else if(subscriptST->storageSpecifier == rdxSS_Interface)
					isIRefArray = true;
				else if(subscriptST->storageSpecifier == rdxSS_Enum)
					isPODArray = true;
			}
			else if(subscriptTypeType == objm->GetBuiltIns()->st_ArrayOfType)
				isCRefArray = true;
			else if(subscriptTypeType == objm->GetBuiltIns()->st_DelegateType)
				isCRefArray = true;
		}
	}

	if(isCRefArray)
	{
		while(numElements--)
		{
			rdxTracedRTRef(rdxCObject) *refP = reinterpret_cast<rdxTracedRTRef(rdxCObject)*>(byteBase);
			visitor->VisitReference(objm, *refP);
			byteBase += sizeof(rdxTracedRTRef(rdxCObject));
		}
	}
	else if(isIRefArray)
	{
		while(numElements--)
		{
			rdxTracedTypelessIRef *refP = reinterpret_cast<rdxTracedTypelessIRef*>(byteBase);
			visitor->VisitReference(objm, *refP);
			byteBase += sizeof(rdxTracedTypelessIRef);
		}
	}
	else if(isStructArray)
	{
		objm->VisitStructureReferences(byteBase, visitor, visitNonSerializable, typeInfo, structureST, stride, numElements);
	}
	else if(isPODArray)
	{
	}
	else if(isUnknownTypeArray)
	{
		rdxIfcTypeFuncs typeFuncs = m_contentsTypeInfo.TypeFuncs();
		if(typeFuncs.IsNotNull())
		{
			rdxIfcTypeFuncs::VisitReferencesFunc vrf = m_contentsTypeInfo.TypeFuncs().GetVisitReferencesFunc();
			if(vrf != RDX_CNULL)
			{
				while(numElements--)
				{
					vrf(objm, byteBase, visitor, visitNonSerializable);
					byteBase += stride;
				}
			}
		}
	}
}

void rdxCArrayContainer::InitializeContents(rdxIObjectManager *objm, bool zeroFill, rdxWeakRTRef(rdxCType) contentsType)
{
	// TODO MUSTFIX - Need defaults
	int todo = 0;
}

void rdxCArrayContainer::InitializeArray(rdxLargeUInt numElements, rdxLargeUInt overflow, rdxLargeUInt stride, rdxLargeUInt dimensionsOffset, rdxIfcTypeInfo contentsTypeInfo)
{
	m_numElements = numElements;
	m_overflow = overflow;
	m_stride = stride;
	m_dimensionsOffset = dimensionsOffset;
	m_contentsTypeInfo = contentsTypeInfo;
}

void rdxCArrayContainer::ComputeContainerSize_Local(rdxSOperationContext *ctx, rdxLargeUInt elementSize, rdxLargeUInt numElements, rdxLargeUInt numDimensions, rdxLargeUInt *outDimensionsOffset, rdxLargeUInt *outSize)
{
	rdxLargeUInt sz = rdxPaddedSize(sizeof(rdxCArrayContainer), RDX_MAX_ALIGNMENT);
	if(!rdxCheckMulOverflowU(elementSize, numElements))
		RDX_LTHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);
	rdxLargeUInt arrayContentsSize = elementSize * numElements;
	rdxLargeUInt dimSize = 0;
	if(numDimensions > 0)
	{
		if(!rdxCheckAddOverflowU(numDimensions, 1))
			RDX_LTHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);
		if(!rdxCheckMulOverflowU(numDimensions + 1, sizeof(rdxLargeUInt)))
			RDX_LTHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);
		dimSize = (numDimensions + 1) * sizeof(rdxLargeUInt);
	}
	if(!rdxCheckAddOverflowU(dimSize, RDX_MAX_ALIGNMENT - 1))
		RDX_LTHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);
	dimSize += RDX_MAX_ALIGNMENT - 1;
	dimSize -= dimSize % RDX_MAX_ALIGNMENT;
	if(!rdxCheckAddOverflowU(sz, arrayContentsSize))
		RDX_LTHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);
	sz += arrayContentsSize;
	if(!rdxCheckAddOverflowU(sz, dimSize))
		RDX_LTHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);

	if(outDimensionsOffset != RDX_CNULL)
		*outDimensionsOffset = sz;
	sz += dimSize;

	if(outSize != RDX_CNULL)
		*outSize = sz;
}
