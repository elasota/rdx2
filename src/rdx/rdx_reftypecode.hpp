#ifndef __RDX_REFTYPECODE_HPP__
#define __RDX_REFTYPECODE_HPP__

#include "rdx_coretypes.hpp"
#include "rdx_reftypedefs.hpp"
#include "rdx_gcinfo.hpp"
#include "rdx_objectmanagement.hpp"
//#include "rdx_utility.hpp"
//#include "rdx_threading.hpp"


#include "rdx_reftypecode_baseref.hpp"
#include "rdx_reftypecode_rtref.hpp"
#include "rdx_reftypecode_ref.hpp"





// void offsetref
template<bool TCounting, class TRefPOD>
RDX_FORCEINLINE rdxTypelessOffsetObjRef<TCounting, TRefPOD>::rdxTypelessOffsetObjRef()
{
}

template<bool TCounting, class TRefPOD>
RDX_FORCEINLINE rdxTypelessOffsetObjRef<TCounting, TRefPOD>::rdxTypelessOffsetObjRef(const rdxTypelessOffsetObjRef<TCounting, TRefPOD> &other)
	: m_objectRef(other.m_objectRef)
	, m_offset(other.m_offset)
{
}

template<bool TCounting, class TRefPOD>
RDX_FORCEINLINE rdxTypelessOffsetObjRef<TCounting, TRefPOD>::rdxTypelessOffsetObjRef(const rdxWeakRTRef(rdxCObject) &base, rdxLargeUInt offset)
	: m_offset(offset)
{
	m_objectRef = base;
}


template<bool TCounting, class TRefPOD>
template<class TOtherBaseRef>
RDX_FORCEINLINE rdxTypelessOffsetObjRef<TCounting, TRefPOD>::rdxTypelessOffsetObjRef(rdxObjRef_CSignal_BaseRef_Type signal, const TOtherBaseRef &otherBaseRef, rdxLargeUInt newOffset)
	: m_objectRef(signal, otherBaseRef)
	, m_offset(newOffset)
{
}

template<bool TCounting, class TRefPOD>
RDX_FORCEINLINE rdxTypelessOffsetObjRef<TCounting, TRefPOD>::rdxTypelessOffsetObjRef(rdxObjRef_CSignal_RawRef_Type signal, void *ptr, rdxLargeUInt newOffset)
	: m_objectRef(signal, ptr)
	, m_offset(newOffset)
{
}

template<bool TCounting, class TRefPOD>
RDX_FORCEINLINE rdxTypelessOffsetObjRef<TCounting, TRefPOD>::rdxTypelessOffsetObjRef(rdxObjRef_CSignal_DataPointer_Type signal, rdxCObject *basePtr, void *internalPtr)
	: m_objectRef(signal, basePtr)
	, m_offset(static_cast<rdxLargeUInt>(reinterpret_cast<const rdxUInt8 *>(internalPtr) - reinterpret_cast<const rdxUInt8 *>(basePtr)))
{
}

template<bool TCounting, class TRefPOD>
RDX_FORCEINLINE rdxTypelessOffsetObjRef<TCounting, TRefPOD>::rdxTypelessOffsetObjRef(rdxObjRef_CSignal_GCInfo_Type signal, const rdxGCInfo *gcinfo, rdxLargeUInt newOffset)
	: m_objectRef(signal, gcinfo)
	, m_offset(newOffset)
{
}

template<bool TCounting, class TRefPOD>
RDX_FORCEINLINE rdxTypelessOffsetObjRef<TCounting, TRefPOD>::~rdxTypelessOffsetObjRef()
{
}

template<bool TCounting, typename TBaseRef>
template<bool TOtherCounting, class TOtherBaseRef>
RDX_FORCEINLINE rdxTypelessOffsetObjRef<TCounting, TBaseRef> &rdxTypelessOffsetObjRef<TCounting, TBaseRef>::operator =(const rdxTypelessOffsetObjRef<TOtherCounting, TOtherBaseRef> &other)
{
	m_objectRef = other.GetObjRef();
	m_offset = other.GetObjByteOffset();
	return *this;
}
	
template<bool TCounting, class TBaseRef>
RDX_FORCEINLINE rdxTypelessOffsetObjRef<TCounting, TBaseRef> &rdxTypelessOffsetObjRef<TCounting, TBaseRef>::operator =(const rdxTypelessOffsetObjRef<TCounting, TBaseRef> &other)
{
	m_objectRef = other.m_objectRef;
	m_offset = other.m_offset;
	return *this;
}
	
template<bool TCounting, class TBaseRef>
RDX_FORCEINLINE rdxTypelessOffsetObjRef<TCounting, rdxBaseRTRef> rdxTypelessOffsetObjRef<TCounting, TBaseRef>::ToRTRef() const
{
	return rdxTypelessOffsetObjRef<TCounting, rdxBaseRTRef>(rdxObjRef_CSignal_BaseRef, m_objectRef, m_offset);
}

template<bool TCounting, class TBaseRef>
RDX_FORCEINLINE rdxTypelessOffsetObjRef<TCounting, rdxBaseHdl> rdxTypelessOffsetObjRef<TCounting, TBaseRef>::ToHdl() const
{
	return rdxTypelessOffsetObjRef<TCounting, rdxBaseRTRef>(rdxObjRef_CSignal_BaseRef, m_objectRef, m_offset);
}


template<bool TCounting, class TBaseRef>
RDX_FORCEINLINE const void *rdxTypelessOffsetObjRef<TCounting, TBaseRef>::Data() const
{
	return reinterpret_cast<const rdxUInt8 *>(m_objectRef.Data()) + this->m_offset;
}

template<bool TCounting, class TBaseRef>
RDX_FORCEINLINE void *rdxTypelessOffsetObjRef<TCounting, TBaseRef>::Modify() const
{
	return reinterpret_cast<rdxUInt8 *>(m_objectRef.Modify()) + this->m_offset;
}

template<bool TCounting, class TBaseRef>
RDX_FORCEINLINE const rdxRef<rdxCObject, TCounting, TBaseRef> *rdxTypelessOffsetObjRef<TCounting, TBaseRef>::GetObjRef() const
{
	return &this->m_objectRef;
}

template<bool TCounting, class TBaseRef>
RDX_FORCEINLINE rdxLargeUInt rdxTypelessOffsetObjRef<TCounting, TBaseRef>::GetObjByteOffset() const
{
	return m_offset;
}

template<bool TCounting, class TBaseRef>
RDX_FORCEINLINE void rdxTypelessOffsetObjRef<TCounting, TBaseRef>::TranslateOffset(rdxLargeInt offset)
{
	m_offset = static_cast<rdxLargeUInt>(static_cast<rdxLargeInt>(m_offset) + offset);
}

template<bool TCounting, class TBaseRef>
template<class TNew>
RDX_FORCEINLINE rdxOffsetObjRef<TNew, TCounting, TBaseRef> rdxTypelessOffsetObjRef<TCounting, TBaseRef>::StaticCast() const
{
	return rdxOffsetObjRef<TNew, TCounting, TBaseRef>(rdxObjRef_CSignal_BaseRef, m_objectRef, m_offset);
}

template<bool TCounting, class TBaseRef>
template<class TNew>
RDX_FORCEINLINE rdxOffsetObjRef<TNew, TCounting, TBaseRef> rdxTypelessOffsetObjRef<TCounting, TBaseRef>::ReinterpretCast() const
{
	return rdxOffsetObjRef<TNew, TCounting, TBaseRef>(rdxObjRef_CSignal_BaseRef, m_objectRef, m_offset);
}

/////////////////////////////////////////////////////////////////////////////////
// Array
template<class TContents>
RDX_FORCEINLINE rdxCArray<TContents>::rdxCArray(rdxIObjectManager *objm, rdxGCInfo *gci)
	: rdxExplicitSuperResolver<rdxCArray<TContents> >::Type(objm, gci)
{
}

template<class TContents>
RDX_FORCEINLINE const TContents *rdxCArray<TContents>::ArrayData() const
{
	return static_cast<const TContents *>(this->GetRawData());
}

template<class TContents>
RDX_FORCEINLINE TContents *rdxCArray<TContents>::ArrayModify()
{
	return static_cast<TContents *>(this->ModifyRawData());
}

template<class TContents>
RDX_FORCEINLINE TContents &rdxCArray<TContents>::Element(rdxLargeUInt index)
{
	return (this->ArrayModify())[index];
}

template<class TContents>
RDX_FORCEINLINE const TContents &rdxCArray<TContents>::Element(rdxLargeUInt index) const
{
	return (this->ArrayData())[index];
}

template<class TContents>
RDX_FORCEINLINE typename rdxWeakOffsetRTRef(TContents) rdxCArray<TContents>::OffsetElementRTRef(rdxLargeUInt index)
{
	rdxWeakRTRef(rdxCObject)::FromPtr(this);
	return typename rdxWeakOffsetRTRef(TContents)(rdxObjRef_CSignal_DataPointer, this, this->ArrayModify() + index);
}

/////////////////////////////////////////////////////////////////////////////////
// Object interface implementation
RDX_FORCEINLINE rdxCObject *rdxSObjectInterfaceImplementation::GetImplementingObject() const
{
	return const_cast<rdxCObject*>(reinterpret_cast<const rdxCObject*>(reinterpret_cast<const rdxUInt8*>(this) - this->headOffset));
}

/////////////////////////////////////////////////////////////////////////////////
// Array container
RDX_FORCEINLINE rdxCArrayContainer::rdxCArrayContainer(rdxIObjectManager *objm, rdxGCInfo *info)
		: rdxCObject(objm, info)
{
}

RDX_FORCEINLINE void *rdxCArrayContainer::ModifyRawData() const
{
	return const_cast<void*>(GetRawData());
}

RDX_FORCEINLINE const void *rdxCArrayContainer::GetRawData() const
{
	return reinterpret_cast<const rdxUInt8*>(this) + rdxPaddedSize(sizeof(*this), RDX_MAX_ALIGNMENT);
}

RDX_FORCEINLINE rdxLargeUInt rdxCArrayContainer::NumElements() const
{
	return this->m_numElements;
}

RDX_FORCEINLINE rdxLargeUInt rdxCArrayContainer::Overflow() const
{
	return this->m_overflow;
}

RDX_FORCEINLINE rdxLargeUInt rdxCArrayContainer::Stride() const
{
	return this->m_stride;
}

inline void rdxCArrayContainer::SetDimension(rdxLargeUInt dimIndex, rdxLargeUInt dimValue)
{
	if(!this->m_dimensionsOffset)
		const_cast<rdxLargeUInt*>(RawDimensions())[dimIndex + 1] = dimValue;
}

inline rdxLargeUInt rdxCArrayContainer::Dimension(rdxLargeUInt dimIndex) const
{
	if(!this->m_dimensionsOffset)
		return RawDimensions()[dimIndex + 1];
	return this->m_numElements;
}


inline rdxLargeUInt rdxCArrayContainer::NumDimensions() const
{
	if(!this->m_dimensionsOffset)
		return 1;
	return RawDimensions()[0];
}

RDX_FORCEINLINE rdxIfcTypeInfo rdxCArrayContainer::ContentsTypeInfo() const
{
	return m_contentsTypeInfo;
}

RDX_FORCEINLINE const rdxLargeUInt *rdxCArrayContainer::RawDimensions() const
{
	const rdxUInt8 *thisBytes = reinterpret_cast<const rdxUInt8*>(this);
	return reinterpret_cast<const rdxLargeUInt *>(thisBytes + m_dimensionsOffset);
}

////////////////////////////////////////////////////////////////////////////////
// CObject
RDX_FORCEINLINE rdxCObject::rdxCObject(rdxIObjectManager *objm, rdxGCInfo *objectInfo)
	: m_objectInfo(objectInfo)
{
}

RDX_FORCEINLINE rdxGCInfo *rdxCObject::ObjectInfo() const
{
	return m_objectInfo;
}

RDX_FORCEINLINE bool rdxCObject::GetPropertyOffset(rdxWeakRTRef(rdxCString) propertyName, rdxLargeUInt *outOffset)
{
	return false;
}

inline rdxCObject &rdxCObject::operator =(const rdxCObject &other)
{
	// info->object->info is an invariant
	//m_objectInfo = other.m_objectInfo;
	return *this;
}

inline rdxCObject::~rdxCObject()
{
}

////////////////////////////////////////////////////////////////////////////////
// Box
template<class TContents>
RDX_FORCEINLINE rdxCBox<TContents>::rdxCBox(rdxIObjectManager *objm, rdxGCInfo *info)
	: rdxCStructContainer(objm, info)
{
}

template<class TContents>
RDX_FORCEINLINE const TContents *rdxCBox<TContents>::BoxData() const
{
	return static_cast<const TContents*>(GetRawData());
}

template<class TContents>
RDX_FORCEINLINE TContents *rdxCBox<TContents>::BoxModify()
{
	return static_cast<TContents*>(GetRawData());
}


////////////////////////////////////////////////////////////////////////////////
// Struct container
RDX_FORCEINLINE rdxCStructContainer::rdxCStructContainer(rdxIObjectManager *objm, rdxGCInfo *info)
	: rdxCObject(objm, info)
{
}

RDX_FORCEINLINE const void *rdxCStructContainer::GetRawData() const
{
	return reinterpret_cast<const rdxUInt8 *>(this) + rdxPaddedSize(sizeof(*this), RDX_MAX_ALIGNMENT);
}

RDX_FORCEINLINE void *rdxCStructContainer::ModifyRawData() const
{
	return const_cast<void*>(GetRawData());
}

// Ref converter
RDX_FORCEINLINE rdxCObject *rdxRefConverter<rdxCObject>::Convert(rdxCObject *ref, rdxCObject *scratch)
{
	return ref;
}

RDX_FORCEINLINE rdxCObject *rdxRefConverter<rdxCObject>::Convert(rdxGCInfo *ref, rdxGCInfo *scratch)
{
	return (ref == RDX_CNULL) ? RDX_CNULL : ref->ObjectDataPtr();
}

RDX_FORCEINLINE rdxCObject *rdxRefConverter<rdxCObject>::Convert(rdxSObjectInterfaceImplementation *ref, void *scratch)
{
	return (ref == RDX_CNULL) ? RDX_CNULL : ref->GetImplementingObject();
}

RDX_FORCEINLINE rdxGCInfo *rdxRefConverter<rdxGCInfo>::Convert(rdxCObject *ref, rdxCObject *scratch)
{
	return (ref == RDX_CNULL) ? RDX_CNULL : ref->ObjectInfo();
}

RDX_FORCEINLINE rdxGCInfo *rdxRefConverter<rdxGCInfo>::Convert(rdxGCInfo *ref, rdxGCInfo *scratch)
{
	return ref;
}

RDX_FORCEINLINE rdxGCInfo *rdxRefConverter<rdxGCInfo>::Convert(rdxSObjectInterfaceImplementation *ref, void *scratch)
{
	return (ref == RDX_CNULL) ? RDX_CNULL : ref->GetImplementingObject()->ObjectInfo();
}

RDX_FORCEINLINE rdxSObjectInterfaceImplementation *rdxRefConverter<rdxSObjectInterfaceImplementation>::Convert(rdxSObjectInterfaceImplementation *ref)
{
	return ref;
}

/*
	static rdxCObject *Convert(rdxGCInfo *ref);
};

template<>
struct rdxRefConverter<rdxGCInfo>
{
	static rdxGCInfo *Convert(rdxCObject *ref);
	static rdxGCInfo *Convert(rdxGCInfo *ref);
	*/

#endif
