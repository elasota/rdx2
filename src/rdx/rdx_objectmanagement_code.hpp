#ifndef __RDX_OBJECTMANAGEMENT_CODE_HPP__
#define __RDX_OBJECTMANAGEMENT_CODE_HPP__

#include "rdx_objectmanagement.hpp"
#include "rdx_objectguid.hpp"

template<class T>
inline typename rdxCRef(T) rdxCInternalObjectFactory::CreateObject(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCStructuredType) t, rdxSDomainGUID domain)
{
	rdxCRef(rdxCObject) obj = objm->CreateObjectContainer(ctx, sizeof(T), domain, t, rdxSAutoTypeInfo<T>::TypeInfoInterface());
	return obj.StaticCast<T>();
}

template<class T>
inline typename rdxCRef(T) rdxCInternalObjectFactory::CreateObject(rdxSOperationContext *ctx, rdxIObjectManager *objm)
{
	return CreateObject<T>(ctx, objm, rdxWeakHdl(rdxCStructuredType)::Null(), rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime));
}

template<class T>
inline typename rdxArrayCRef(T) rdxCInternalObjectFactory::Create1DArray(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxLargeUInt count, rdxWeakHdl(rdxCArrayOfType) t, rdxSDomainGUID domain, rdxLargeUInt overflow, rdxLargeUInt stride, rdxIfcTypeInfo contentsTypeInfo)
{
	RDX_TRY(ctx)
	{
		rdxCRef(rdxCArrayContainer) container;
		RDX_PROTECT_ASSIGN(ctx, container, objm->CreateArrayContainer(ctx, stride, count, 1, domain, overflow, t, rdxSAutoTypeInfo<rdxCArray<T> >::TypeInfoInterface()));
		rdxLargeUInt dimOffset;
		RDX_PROTECT(ctx, rdxCArrayContainer::ComputeContainerSize(ctx, stride, count, 1, &dimOffset, RDX_CNULL));
		container->InitializeArray(count, overflow, sizeof(T), dimOffset, contentsTypeInfo);
		return container.StaticCast<rdxCArray<T> >();
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROWV(ctx, rdxArrayCRef(T)::Null());
	}
	RDX_ENDTRY
}

template<class T>
inline typename rdxArrayCRef(T) rdxCInternalObjectFactory::Create1DArray(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxLargeUInt count, rdxWeakHdl(rdxCArrayOfType) t, rdxSDomainGUID domain)
{
	return Create1DArray<T>(ctx, objm, count, t, domain, 0, sizeof(T), rdxSAutoTypeInfo<T>::TypeInfoInterface());
}


template<class T>
inline typename rdxArrayCRef(T) rdxCInternalObjectFactory::Create1DArray(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxLargeUInt count)
{
	return Create1DArray<T>(ctx, objm, count, rdxWeakHdl(rdxCArrayOfType)::Null(), rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime), 0, sizeof(T), rdxSAutoTypeInfo<T>::TypeInfoInterface());
}

template<class T>
rdxCRef(rdxCArrayOfType) rdxCInternalObjectFactory::AutoCreateArrayType(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCType) t, rdxLargeUInt numDimensions, bool constant)
{
	return objm->CreateArrayType(ctx, objm, t, numDimensions, constant, rdxSAutoTypeInfo<rdxCArray<T> >::TypeInfoInterface());
}

//=========================================================================================

/*
template<class T>
inline typename rdxCRef(T) rdxCInternalObjectFactory::CreateObject(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCStructuredType) t, rdxSDomainGUID domain)
{
	rdxCRef(rdxCObject) obj = objm->CreateObjectContainer(ctx, sizeof(T), domain, t, rdxSAutoTypeInfo<T>::TypeInfoInterface());
	return obj.StaticCast<T>();
}

template<class T>
inline typename rdxCRef(T) rdxCInternalObjectFactory::CreateObject(rdxSOperationContext *ctx, rdxIObjectManager *objm)
{
	return CreateObject<T>(ctx, objm, rdxWeakHdl(rdxCStructuredType)::Null(), rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime));
}
*/

template<class T>
inline typename rdxArrayCRef(T) rdxCExternalObjectFactory::Create1DArray(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxLargeUInt count, rdxWeakHdl(rdxCArrayOfType) t, rdxSDomainGUID domain, rdxLargeUInt overflow, rdxLargeUInt stride, rdxIfcTypeInfo contentsTypeInfo)
{
	return rdxXAPI_Create1DArrayCommon(ctx, objm, count, t, domain, 0, stride, contentsTypeInfo, ResolveArrayForContents(objm, static_cast<const T *>(RDX_CNULL))).StaticCast<rdxCArray<T> >();
}

template<class T>
inline typename rdxArrayCRef(T) rdxCExternalObjectFactory::Create1DArray(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxLargeUInt count, rdxWeakHdl(rdxCArrayOfType) t, rdxSDomainGUID domain)
{
	rdxCRef(rdxCArrayContainer) result;
	rdxXAPI_Create1DArrayIntuitive(ctx, objm, count, t, domain, sizeof(T), &result);
	return result.StaticCast<rdxCArray<T> >();
}

RDX_FORCEINLINE rdxIfcTypeInfo rdxCExternalObjectFactory::ResolveArrayForContents(rdxIObjectManager *objm, const void *p)
{
	return objm->GetBasicValueArrayTypeInfo();
}

RDX_FORCEINLINE rdxIfcTypeInfo rdxCExternalObjectFactory::ResolveArrayForContents(rdxIObjectManager *objm, const rdxTracedRTRef(rdxCObject) *p)
{
	return objm->GetBasicReferenceArrayTypeInfo();
}

template<class TContents>
rdxCRef(rdxCArrayOfType) rdxCExternalObjectFactory::AutoCreateArrayType(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCType) t, rdxLargeUInt numDimensions, bool constant)
{
	const TContents *resolvePtr = RDX_CNULL;
	return objm->CreateArrayType(ctx, t, numDimensions, constant, ResolveArrayForContents(objm, static_cast<const TContents *>(RDX_CNULL)));
}

#endif
