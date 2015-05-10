#include "../rdx/rdx_objectmanagement.hpp"
#include "../rdx/rdx_marshal.hpp"
#include "../rdx/rdx_utility.hpp"
#include "../rdx/rdx_blockcopy.hpp"
#include "../exported/RDXInterface/Core/Array.hpp"


using namespace RDX;
using namespace RDX::ObjectManagement;
using namespace RDX::Programmability;
using namespace RDXInterface;

rdxLargeInt RDXInterface::Core::Array::Length(Marshaling::ExportedCallEnvironment &callEnv)
{
	const GCInfo *info = GCInfo::From(this);
	if(info->numDimensions == 0)
		return 0;

	return info->dimensions[0];
}

void RDXInterface::Core::Array::BlockCopy(Marshaling::ExportedCallEnvironment &callEnv, rdxLargeInt srcStart, Core::Array *dest, rdxLargeInt destStart, rdxLargeInt length)
{
	const ArrayOfType *srcAOT = static_cast<const ArrayOfType *>(GCInfo::From(this)->containerType);
	const ArrayOfType *destAOT = static_cast<const ArrayOfType *>(GCInfo::From(dest)->containerType);
	const rdxLargeInt nSrcElements = GCInfo::From(this)->numElements;
	const rdxLargeInt nDestElements = GCInfo::From(dest)->numElements;
	const rdxLargeInt stride = GCInfo::From(this)->elementSize;

	if(!callEnv.objm->TypesCompatible(srcAOT, destAOT))
	{
		callEnv.Throw("Core.RDX.IncompatibleConversionException.instance");
		return;
	}

	if(length == 0)
		return;

	if(srcStart < 0 || destStart < 0 || length > nSrcElements || length > nDestElements
		|| (nSrcElements - length) < srcStart || (nDestElements - length) < destStart)
	{
		callEnv.Throw("Core.RDX.IndexOutOfBoundsException.instance");
		return;
	}

	Utility::BlockMove(reinterpret_cast<UInt8 *>(dest) + destStart * stride, reinterpret_cast<const UInt8 *>(this) + srcStart * stride, static_cast<LargeUInt>(length * stride));
}

rdxLargeInt RDXInterface::Core::Array::Dimension(Marshaling::ExportedCallEnvironment &callEnv, rdxLargeInt d)
{
	const GCInfo *info = GCInfo::From(this);
	if(d < 0 || d >= info->numDimensions)
	{
		callEnv.Throw("Core.RDX.IndexOutOfBoundsException.instance");
		return 0;
	}
	return info->dimensions[d];
}

static inline RDXInterface::Core::Array *arrayCopy(Marshaling::ExportedCallEnvironment &callEnv, const void *src, bool becomeConst, bool becomeNonConst)
{
	RDX_TRY(callEnv.ctx)
	{
		const GCInfo *selfInfo = GCInfo::From(src);
		CRef<const ArrayOfType> newType = static_cast<const ArrayOfType *>(selfInfo->containerType);
		if(becomeConst)
		{
			RDX_PROTECT_ASSIGN(callEnv.ctx, newType, callEnv.objm->CreateArrayType(callEnv.ctx, newType->type, newType->numDimensions, false));
		}
		else if(becomeNonConst)
		{
			RDX_PROTECT_ASSIGN(callEnv.ctx, newType, callEnv.objm->CreateArrayType(callEnv.ctx, newType->type, newType->numDimensions, false));
		}

		CRef<void> newArray;
		RDX_PROTECT_ASSIGN(callEnv.ctx, newArray, callEnv.objm->CreateContainer(callEnv.ctx, selfInfo->elementSize, selfInfo->numElements, selfInfo->numDimensions));
		Utility::BlockCopy(newArray, oldArray, static_cast<LargeUInt>(selfInfo->elementSize) * static_cast<LargeUInt>(selfInfo->numElements));

		GCInfo *newInfo = GCInfo::From(newArray);
		for(rdxLargeInt i=0;i<selfInfo->numDimensions;i++)
			newInfo->dimensions[i] = selfInfo->dimensions[i];
		return reinterpret_cast<RDXInterface::Core::Array *>(newArray.Object());
	}
	RDX_CATCH(callEnv.ctx)
	{
		callEnv.Throw("Core.RDX.AllocationFailureException.instance");
		return NULL;
	}
	RDX_ENDTRY

	return NULL;
}

RDXInterface::Core::Array *RDXInterface::Core::Array::Clone(Marshaling::ExportedCallEnvironment &callEnv)
{
	return arrayCopy(callEnv, this, false, false);
}

RDXInterface::Core::Array *RDXInterface::Core::Array::ToConst(Marshaling::ExportedCallEnvironment &callEnv)
{
	return arrayCopy(callEnv, this, true, false);
}

RDXInterface::Core::Array *RDXInterface::Core::Array::ToNonConst(Marshaling::ExportedCallEnvironment &callEnv)
{
	return arrayCopy(callEnv, this, false, true);
}
