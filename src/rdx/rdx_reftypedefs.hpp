#ifndef __RDX_REFTYPEDEFS_HPP__
#define __RDX_REFTYPEDEFS_HPP__

#include "rdx_coretypes.hpp"
#include "rdx_atomic.hpp"
#include "rdx_reftypealiases.hpp"
#include "rdx_superresolver.hpp"
#include "rdx_typeprocessordefs.hpp"
#include "rdx_api.hpp"

struct rdxGCInfo;
struct rdxIfcTypeInfo;
struct rdxIObjectManager;
struct rdxIObjectReferenceVisitor;
struct rdxSOperationContext;
class rdxCType;
class rdxCString;
class rdxCStructuredType;
class rdxCArrayContainer;
template<class T> struct rdxSInvokingTypeInfo;
template<class T> struct rdxSAutoTypeInfo;
template<class T, class TBaseRef> struct rdxRTRef;
template<class T, bool TCounting, class TBaseRef> struct rdxRef;
template<class T, bool TCounting, class TBaseRef> struct rdxOffsetObjRef;
template<class T> class rdxCBox;


class rdxCObject
{
	friend class rdxCObjectOperationProxy;
public:
	rdxCObject(rdxIObjectManager *objm, rdxGCInfo *objectInfo);
	virtual ~rdxCObject();
	rdxGCInfo *ObjectInfo() const;

	static bool GetPropertyOffset(rdxWeakRTRef(rdxCString) propertyName, rdxLargeUInt *outOffset);

	rdxCObject &operator =(const rdxCObject &other);

private:
	rdxGCInfo *m_objectInfo;
};
RDX_DECLARE_COMPLEX_NATIVE_CLASS(rdxCObject);

struct rdxSObjectInterfaceImplementation
{
	rdxLargeUInt headOffset;
	rdxLargeUInt vftOffset;

	rdxCObject *GetImplementingObject() const;
};

class rdxXAPI_ArrayContainer_Shim;

RDX_DYNLIB_API void rdxXAPI_ArrayContainer_VisitReferences(rdxCArrayContainer *v, rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable);
RDX_DYNLIB_API void rdxXAPI_ArrayContainer_ComputeContainerSize(rdxSOperationContext *ctx, rdxLargeUInt elementSize, rdxLargeUInt numElements, rdxLargeUInt numDimensions, rdxLargeUInt *outDimensionsOffset, rdxLargeUInt *outSize);

class rdxCArrayContainer : public rdxCObject
{
	friend class rdxXAPI_ArrayContainer_Shim;

public:
	explicit rdxCArrayContainer(rdxIObjectManager *objm, rdxGCInfo *info);

	void *ModifyRawData() const;
	const void *GetRawData() const;

	rdxLargeUInt NumElements() const;
	rdxLargeUInt Overflow() const;
	rdxLargeUInt Stride() const;
	rdxLargeUInt NumDimensions() const;
	rdxIfcTypeInfo ContentsTypeInfo() const;

	RDX_FORCEINLINE void VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
	{
		rdxXAPI_ArrayContainer_VisitReferences(this, objm, visitor, visitNonSerializable);
	}

	RDX_FORCEINLINE static void ComputeContainerSize(rdxSOperationContext *ctx, rdxLargeUInt elementSize, rdxLargeUInt numElements, rdxLargeUInt numDimensions, rdxLargeUInt *outDimensionsOffset, rdxLargeUInt *outSize)
	{
		rdxXAPI_ArrayContainer_ComputeContainerSize(ctx, elementSize, numElements, numDimensions, outDimensionsOffset, outSize);
	}

	void InitializeArray(rdxLargeUInt numElements, rdxLargeUInt overflow, rdxLargeUInt stride, rdxLargeUInt dimensionsOffset, rdxIfcTypeInfo contentsTypeInfo);
	void InitializeContents(rdxIObjectManager *objm, bool zeroFill, rdxWeakRTRef(rdxCType) contentsType);

	void SetDimension(rdxLargeUInt dimIndex, rdxLargeUInt dimValue);
	rdxLargeUInt Dimension(rdxLargeUInt dimIndex) const;

	static const rdxUInt32 TYPE_FLAGS = (rdxETIF_StandardClass | rdxETIF_VisitReferences | rdxETIF_ArrayFlag);
private:
	const rdxLargeUInt *RawDimensions() const;
	void VisitReferences_Local(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable);
	static void ComputeContainerSize_Local(rdxSOperationContext *ctx, rdxLargeUInt elementSize, rdxLargeUInt numElements, rdxLargeUInt numDimensions, rdxLargeUInt *outDimensionsOffset, rdxLargeUInt *outSize);

	rdxLargeUInt m_numElements;
	rdxLargeUInt m_overflow;
	rdxLargeUInt m_stride;
	rdxLargeUInt m_dimensionsOffset;
	rdxIfcTypeInfo m_contentsTypeInfo;
};
RDX_DECLARE_COMPLEX_NATIVE_CLASS(rdxCArrayContainer);


template<class TContents>
struct rdxExplicitSuperResolver<rdxCArray<TContents> >
{
	typedef rdxCArrayContainer Type;
};

template<>
struct rdxExplicitSuperResolver<rdxCArray<rdxRTRef<rdxCObject, rdxBaseRTRef> > >
{
	typedef rdxCArrayContainer Type;
};

template<class TRefType>
struct rdxExplicitSuperResolver<rdxCArray<rdxRTRef<TRefType, rdxBaseRTRef> > >
{
	typedef rdxCArray<rdxRTRef<typename rdxExplicitSuperResolver<TRefType>::Type, rdxBaseRTRef> > Type;
};

template<class TContents>
struct rdxSInvokingTypeInfo<rdxCArray<TContents> >
{
	inline static rdxIfcTypeInfo TypeInfoInterface()
	{
		rdxIfcTypeInfo typeInfoInterface;
		typeInfoInterface.fetchFunc = Fetch;
		return typeInfoInterface;
	}
	inline static rdxIfcTypeFuncs TypeFuncsInterface()
	{
		return rdxSAutoTypeFuncs<rdxCArray<TContents>, (rdxCArrayContainer::TYPE_FLAGS)>::TypeFuncsInterface();
	}
	inline static void Fetch(rdxIfcTypeInfo::EDataIndex index, rdxLargeUInt *uiOut, rdxUInt32 *u32out, void **ptrOut)
	{
		switch(index)
		{
		case rdxIfcTypeInfo::EData_Size: *uiOut = sizeof(rdxCArray<TContents>); return;
		case rdxIfcTypeInfo::EData_Alignment: *uiOut = rdxAlignOf(rdxCArray<TContents>); return;
		case rdxIfcTypeInfo::EData_SizeInSubclass: *uiOut = 0; return;
		case rdxIfcTypeInfo::EData_ObjectBaseOffset: *uiOut = rdxOffsetOfBaseClass<rdxCArray<TContents>, rdxCObject>(); return;
		case rdxIfcTypeInfo::EData_Flags: *u32out = (rdxCArrayContainer::TYPE_FLAGS); return;
		case rdxIfcTypeInfo::EData_TypeFuncs: *ptrOut = reinterpret_cast<void*>(rdxSInvokingTypeInfo<rdxCArray<TContents> >::TypeFuncsInterface().getProcFunc); return;
		case rdxIfcTypeInfo::EData_BoxTypeInfo: *ptrOut = RDX_CNULL; return;
		};
	}
};

#ifdef RDX_BUILD_COREMODULE

template<class TContents>
struct rdxSAutoTypeInfo<rdxCArray<TContents> >
{
	typedef rdxCArray<TContents> ThisType;
	inline static rdxIfcTypeInfo TypeInfoInterface()
	{
		return rdxSInvokingTypeInfo<rdxCArray<TContents> >::TypeInfoInterface();
	}
};

#endif


template<class TContents>
class rdxCArray : public rdxExplicitSuperResolver<rdxCArray<TContents> >::Type
{
public:
	rdxCArray(rdxIObjectManager *objm, rdxGCInfo *gci);

	const TContents *ArrayData() const;
	TContents *ArrayModify();

	TContents &Element(rdxLargeUInt index);
	const TContents &Element(rdxLargeUInt index) const;
	typename rdxWeakOffsetRTRef(TContents) OffsetElementRTRef(rdxLargeUInt index);
};

class rdxCStructContainer : public rdxCObject
{
private:
	rdxLargeUInt dataOffset;	// From rdxCObject

public:
	rdxCStructContainer(rdxIObjectManager *objm, rdxGCInfo *info);
	static const rdxUInt32 TYPE_FLAGS	= (rdxETIF_StandardClass | rdxETIF_VisitReferences | rdxETIF_BoxFlag);

	void VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable);
	void InitializeContents(rdxIObjectManager *objm, rdxWeakRTRef(rdxCStructuredType) st);
	const void *GetRawData() const;
	void *ModifyRawData() const;
};
RDX_DECLARE_COMPLEX_NATIVE_CLASS(rdxCStructContainer);


template<class TContents>
struct rdxSInvokingTypeInfo<rdxCBox<TContents> >
{
	inline static rdxIfcTypeInfo TypeInfoInterface()
	{
		rdxIfcTypeInfo typeInfoInterface;
		typeInfoInterface.fetchFunc = Fetch;
		return typeInfoInterface;
	}
	inline static rdxIfcTypeFuncs TypeFuncsInterface()
	{
		return rdxSAutoTypeFuncs<rdxCBox<TContents>, (rdxCStructContainer::TYPE_FLAGS)>::TypeFuncsInterface();
	}
	inline static void Fetch(rdxIfcTypeInfo::EDataIndex index, rdxLargeUInt *uiOut, rdxUInt32 *u32out, void **ptrOut)
	{
		switch(index)
		{
		case rdxIfcTypeInfo::EData_Size: *uiOut = sizeof(rdxCBox<TContents>); return;
		case rdxIfcTypeInfo::EData_Alignment: *uiOut = rdxAlignOf(rdxCBox<TContents>); return;
		case rdxIfcTypeInfo::EData_SizeInSubclass: *uiOut = sizeof(rdxCBox<TContents>) - rdxSSubclassByteAbsorptionCounter<rdxCBox<TContents> >::NUM_ABSORBED; return;
		case rdxIfcTypeInfo::EData_ObjectBaseOffset: *uiOut = rdxOffsetOfBaseClass<rdxCBox<TContents>, rdxCObject>(); return;
		case rdxIfcTypeInfo::EData_Flags: *u32out = (rdxCStructContainer::TYPE_FLAGS); return;
		case rdxIfcTypeInfo::EData_TypeFuncs: *ptrOut = rdxSInvokingTypeInfo<rdxCBox<TContents> >::TypeFuncsInterface().getProcFunc; return;
		case rdxIfcTypeInfo::EData_BoxTypeInfo: *ptrOut = RDX_CNULL; return;
		};
	}
};


#ifdef RDX_BUILD_COREMODULE

template<class TContents>
struct rdxSAutoTypeInfo<rdxCBox<TContents> >
{
	typedef rdxCBox<TContents> ThisType;
	inline static rdxIfcTypeInfo TypeInfoInterface()
	{
		return rdxSInvokingTypeInfo<rdxCBox<TContents> >::TypeInfoInterface();
	}
};

#endif

template<class TContents>
class rdxCBox : public rdxCStructContainer
{
public:
	explicit rdxCBox(rdxIObjectManager *objm, rdxGCInfo *info);

	const TContents *BoxData() const;
	TContents *BoxModify();
};

template<class TTarget>
struct rdxRefConverter
{
};

template<>
struct rdxRefConverter<rdxCObject>
{

private:
	// Preferential disambiguation of rdxCObject over rdxSObjectInterfaceImplementation
	static rdxCObject *Convert(rdxCObject *ref, rdxCObject *scratch);
	static rdxCObject *Convert(rdxSObjectInterfaceImplementation *ref, void *scratch);
	static rdxCObject *Convert(rdxGCInfo *ref, rdxGCInfo *scratch);

public:
	template<class T>
	static RDX_FORCEINLINE rdxCObject *Convert(T *ref)
	{
		return Convert(ref, static_cast<T*>(RDX_CNULL));
	}
};

template<>
struct rdxRefConverter<rdxGCInfo>
{
	// Preferential disambiguation of rdxCObject over rdxSObjectInterfaceImplementation
	static rdxGCInfo *Convert(rdxCObject *ref, rdxCObject *scratch);
	static rdxGCInfo *Convert(rdxSObjectInterfaceImplementation *ref, void *scratch);
	static rdxGCInfo *Convert(rdxGCInfo *ref, rdxGCInfo *scratch);

public:
	template<class T>
	static RDX_FORCEINLINE rdxGCInfo *Convert(T *ref)
	{
		return Convert(ref, static_cast<T*>(RDX_CNULL));
	}
};

template<>
struct rdxRefConverter<rdxSObjectInterfaceImplementation>
{
	static rdxSObjectInterfaceImplementation *Convert(rdxSObjectInterfaceImplementation *ref);
};

template<class TObjectRefType>
struct rdxBaseRef
{
	typedef TObjectRefType *PODType;
	typedef TObjectRefType DerefPODType;

	static TObjectRefType *UnextractPOD(void *ptr);
	TObjectRefType *GetPOD() const;

	explicit rdxBaseRef(rdxBaseRef_ConSignal_Null_Type nullSig);
	rdxBaseRef(rdxBaseRef_ConSignal_POD_Type signalValue, TObjectRefType *ref);
	rdxBaseRef(rdxBaseRef_ConSignal_Marking_Type signalValue, TObjectRefType *ref);
	rdxBaseRef(rdxBaseRef_ConSignal_Counting_Type signalValue, TObjectRefType *ref);
	void AssignRef(rdxBaseRef_ConSignal_POD_Type signalValue, TObjectRefType *ref);
	void AssignRef(rdxBaseRef_ConSignal_Marking_Type signalValue, TObjectRefType *ref);
	void AssignRef(rdxBaseRef_ConSignal_Counting_Type signalValue, TObjectRefType *ref);
	void DestroyRef(rdxBaseRef_ConSignal_POD_Type signalValue);
	void DestroyRef(rdxBaseRef_ConSignal_Marking_Type signalValue);
	void DestroyRef(rdxBaseRef_ConSignal_Counting_Type signalValue);

private:
	rdxAtomic<void *> m_ref;

	RDX_FORCEINLINE rdxBaseRef(const rdxBaseRef &rs) : m_ref(rs.m_ref) {}
	rdxBaseRef &operator =(const rdxBaseRef &rs);
};

enum rdxObjRef_CSignal_BaseRef_Type
{
	rdxObjRef_CSignal_BaseRef
};

enum rdxObjRef_CSignal_DataPointer_Type
{
	rdxObjRef_CSignal_DataPointer
};

enum rdxObjRef_CSignal_GCInfo_Type
{
	rdxObjRef_CSignal_GCInfo
};

enum rdxObjRef_CSignal_RawRef_Type
{
	rdxObjRef_CSignal_RawRef
};

enum rdxObjRef_CSignal_Null_Type
{
	rdxObjRef_CSignal_Null
};

template<class TBaseRef>
struct rdxTypelessRTRef
{
protected:
	TBaseRef m_baseRef;

	void InitRef(typename TBaseRef::PODType data);
	void AssignRawRef(typename TBaseRef::PODType data);

public:
	typedef void super;

	rdxTypelessRTRef();
	rdxTypelessRTRef(const rdxTypelessRTRef<TBaseRef> &other);
	template<class TOtherBaseRef>
	rdxTypelessRTRef(rdxObjRef_CSignal_BaseRef_Type signal, const TOtherBaseRef &otherBaseRef);
	rdxTypelessRTRef(rdxObjRef_CSignal_RawRef_Type signal, void *ptr);
	rdxTypelessRTRef(rdxObjRef_CSignal_DataPointer_Type signal, rdxCObject *ptr);
	rdxTypelessRTRef(rdxObjRef_CSignal_GCInfo_Type signal, const rdxGCInfo *gcinfo);
	~rdxTypelessRTRef();

	rdxTypelessOffsetObjRef<false, TBaseRef> NoOffset() const;
	bool IsNull() const;
	bool IsNotNull() const;
	typename TBaseRef::PODType GetPOD() const;

	typename rdxWeakRTRef(rdxCObject) ToWeakRTRef() const;

	template<class TNew>
	rdxRTRef<TNew, TBaseRef> ReinterpretCast() const;
};

template<class T, class TBaseRef>
struct rdxRefTypeSuperResolver;

template<class TBaseRef>
struct rdxRefTypeSuperResolver<rdxCObject, TBaseRef>
{
	typedef rdxTypelessRTRef<TBaseRef> Type;
};

template<>
struct rdxRefTypeSuperResolver<rdxSObjectInterfaceImplementation, rdxBaseIfcRef>
{
	typedef rdxTypelessRTRef<rdxBaseIfcRef> Type;
};

// TracedRTRef inheritance
template<class T, class TBaseRef>
struct rdxRefTypeSuperResolver
{
	typedef rdxRTRef<typename rdxSuper(T), TBaseRef> Type;
};

template<class TArrayContents, class TBaseRef>
struct rdxRefTypeSuperResolver<rdxCArray<TArrayContents>, TBaseRef>
{
	typedef rdxRTRef<rdxCArrayContainer, TBaseRef> Type;
};


template<class T, class TBaseRef>
struct rdxRTRef : public rdxRefTypeSuperResolver<T, TBaseRef>::Type
{
	typedef typename rdxRefTypeSuperResolver<T, TBaseRef>::Type Super;

	rdxRTRef();
	rdxRTRef(const rdxRTRef<T, TBaseRef> &other);

	template<class TOtherBaseRef>
	rdxRTRef(rdxObjRef_CSignal_BaseRef_Type signal, const TOtherBaseRef &otherBaseRef);
	rdxRTRef(rdxObjRef_CSignal_RawRef_Type signal, void *ptr);
	rdxRTRef(rdxObjRef_CSignal_DataPointer_Type signal, T *ptr);
	rdxRTRef(rdxObjRef_CSignal_GCInfo_Type signal, const rdxGCInfo *gcinfo);
	explicit rdxRTRef(rdxObjRef_CSignal_Null_Type signal);

	template<class TOtherBaseRef>
	rdxRTRef<T, TBaseRef> &operator =(const rdxRTRef<T, TOtherBaseRef> &other);
	template<class TOther, bool TOtherCounting, class TOtherBaseRef>
	rdxRTRef<T, TBaseRef> &operator =(const rdxRef<TOther, TOtherCounting, TOtherBaseRef> &other);
	rdxRTRef<T, TBaseRef> &operator =(const rdxRTRef<T, TBaseRef> &other);
	T *operator ->() const;

	template<class TOther, class TOtherRefPOD>
	bool operator ==(const rdxRTRef<TOther, TOtherRefPOD> &other) const;
	template<class TOther, bool TOtherCounting, class TOtherRefPOD>
	bool operator ==(const rdxRef<TOther, TOtherCounting, TOtherRefPOD> &other) const;
	template<class TOther, class TOtherRefPOD>
	bool operator !=(const rdxRTRef<TOther, TOtherRefPOD> &other) const;
	template<class TOther, bool TOtherCounting, class TOtherRefPOD>
	bool operator !=(const rdxRef<TOther, TOtherCounting, TOtherRefPOD> &other) const;

	typename rdxWeakRTRef(T) ToWeakRTRef() const;
	typename rdxWeakHdl(T) ToWeakHdl() const;
	typename rdxCRef(T) ToCRef() const;

	const T *Data() const;
	T *Modify() const;
	static rdxRTRef<T, TBaseRef> Null();				// NOTE: Only mark replacements if TCounting
	static rdxRTRef<T, TBaseRef> FromPtr(T *ptr);

	template<class TNew>
	rdxRef<TNew, false, TBaseRef> StaticCast() const;

	template<class SelfT, class TMemberType>
	rdxOffsetObjRef<TMemberType, false, TBaseRef> OffsetMember(TMemberType SelfT::* member) const;

	bool IsNull() const;
	bool IsNotNull() const;
};

template<class T, bool TCounting, class TBaseRef>
struct rdxRef
{
	rdxRef();
	rdxRef(const rdxRef<T, TCounting, TBaseRef> &other);
	template<class TOther, bool TOtherCounting, class TOtherBaseRef>
	rdxRef(const rdxRef<TOther, TOtherCounting, TOtherBaseRef> &other);
	~rdxRef();

	template<class TOtherBaseRef>
	rdxRef(rdxObjRef_CSignal_BaseRef_Type signal, const TOtherBaseRef &otherBaseRef);
	rdxRef(rdxObjRef_CSignal_RawRef_Type signal, void *ptr);
	rdxRef(rdxObjRef_CSignal_DataPointer_Type signal, T *ptr);
	rdxRef(rdxObjRef_CSignal_GCInfo_Type signal, const rdxGCInfo *gcinfo);
	explicit rdxRef(rdxObjRef_CSignal_Null_Type signal);

	template<bool TOtherCounting, class TOtherBaseRef>
	rdxRef<T, TCounting, TBaseRef> &operator =(const rdxRef<T, TOtherCounting, TOtherBaseRef> &other);
	template<class TOther, class TOtherBaseRef>
	rdxRef<T, TCounting, TBaseRef> &operator =(const rdxRTRef<TOther, TOtherBaseRef> &other);
	rdxRef<T, TCounting, TBaseRef> &operator =(const rdxRef<T, TCounting, TBaseRef> &other);
	T *operator ->() const;
	
	template<class TOther, bool TOtherCounting, class TOtherRefPOD>
	bool operator ==(const rdxRef<TOther, TOtherCounting, TOtherRefPOD> &other) const;
	template<class TOther, class TOtherBaseRef>
	bool operator ==(const rdxRTRef<TOther, TOtherBaseRef> &other) const;
	template<class TOther, bool TOtherCounting, class TOtherRefPOD>
	bool operator !=(const rdxRef<TOther, TOtherCounting, TOtherRefPOD> &other) const;
	template<class TOther, class TOtherBaseRef>
	bool operator !=(const rdxRTRef<TOther, TOtherBaseRef> &other) const;

	typename rdxWeakRTRef(T) ToWeakRTRef() const;
	typename rdxWeakHdl(T) ToWeakHdl() const;
	typename rdxCRef(T) ToCRef() const;

	const T *Data() const;
	T *Modify() const;
	static rdxRef<T, TCounting, TBaseRef> Null();				// NOTE: Only mark replacements if TCounting
	static rdxRef<T, TCounting, TBaseRef> FromPtr(T *ptr);

	template<class TNew>
	rdxRef<TNew, TCounting, TBaseRef> StaticCast() const;
	template<class TNew>
	rdxRef<TNew, TCounting, TBaseRef> ReinterpretCast() const;
	template<class TNew>
	rdxRef<TNew, TCounting, rdxBaseIfcRef> StaticCastToInterface() const;

	template<class SelfT, class TMemberType>
	rdxOffsetObjRef<TMemberType, TCounting, TBaseRef> OffsetMember(TMemberType SelfT::* member) const;

	bool IsNull() const;
	bool IsNotNull() const;

	rdxTypelessOffsetObjRef<TCounting, TBaseRef> NoOffset() const;
	typename TBaseRef::PODType GetPOD() const;
	TBaseRef &GetBaseRef();
	const TBaseRef &GetBaseRef() const;

private:
	void InitRef(typename TBaseRef::PODType data);
	void AssignRawRef(typename TBaseRef::PODType data);

	TBaseRef m_baseRef;
};

template<bool TCounting, class TBaseRef>
struct rdxTypelessOffsetObjRef
{
protected:
	rdxRef<rdxCObject, TCounting, TBaseRef>	m_objectRef;
	rdxLargeUInt							m_offset;

public:
	typedef void super;
	
	rdxTypelessOffsetObjRef();
	rdxTypelessOffsetObjRef(const rdxTypelessOffsetObjRef<TCounting, TBaseRef> &other);

	rdxTypelessOffsetObjRef(const rdxWeakRTRef(rdxCObject) &base, rdxLargeUInt offset);
	template<class TOtherBaseRef>
	rdxTypelessOffsetObjRef(rdxObjRef_CSignal_BaseRef_Type signal, const TOtherBaseRef &otherBaseRef, rdxLargeUInt newOffset);
	rdxTypelessOffsetObjRef(rdxObjRef_CSignal_RawRef_Type signal, void *ptr, rdxLargeUInt newOffset);
	rdxTypelessOffsetObjRef(rdxObjRef_CSignal_DataPointer_Type signal, rdxCObject *basePtr, void *internalPtr);
	rdxTypelessOffsetObjRef(rdxObjRef_CSignal_GCInfo_Type signal, const rdxGCInfo *gcinfo, rdxLargeUInt newOffset);
	~rdxTypelessOffsetObjRef();
	
	template<bool TOtherCounting, class TOtherBaseRef>
	rdxTypelessOffsetObjRef<TCounting, TBaseRef> &operator =(const rdxTypelessOffsetObjRef<TOtherCounting, TOtherBaseRef> &other);
	rdxTypelessOffsetObjRef<TCounting, TBaseRef> &operator =(const rdxTypelessOffsetObjRef<TCounting, TBaseRef> &other);

	rdxTypelessOffsetObjRef<TCounting, rdxBaseRTRef> ToRTRef() const;
	rdxTypelessOffsetObjRef<TCounting, rdxBaseHdl> ToHdl() const;
	const void *Data() const;
	void *Modify() const;
	void TranslateOffset(rdxLargeInt offset);

	const rdxRef<rdxCObject, TCounting, TBaseRef> *GetObjRef() const;
	rdxLargeUInt GetObjByteOffset() const;

	template<class TNew>
	rdxOffsetObjRef<TNew, TCounting, TBaseRef> StaticCast() const;

	template<class TNew>
	rdxOffsetObjRef<TNew, TCounting, TBaseRef> ReinterpretCast() const;
};


template<class T, bool TCounting, class TBaseRef>
struct rdxOffsetObjRef : public rdxTypelessOffsetObjRef<TCounting, TBaseRef>
{
	RDX_FORCEINLINE rdxOffsetObjRef()
		: rdxTypelessOffsetObjRef<TCounting, TBaseRef>()
	{
	}

	RDX_FORCEINLINE rdxOffsetObjRef(const rdxOffsetObjRef<T, TCounting, TBaseRef> &other)
		: rdxTypelessOffsetObjRef<TCounting, TBaseRef>(other)
	{
	}
	
	RDX_FORCEINLINE rdxOffsetObjRef(const rdxWeakRTRef(rdxCObject) &base, rdxLargeUInt offset)
		: rdxTypelessOffsetObjRef<TCounting, TBaseRef>(base, offset)
	{
	}

	template<class TOtherBaseRef>
	RDX_FORCEINLINE rdxOffsetObjRef(rdxObjRef_CSignal_BaseRef_Type signal, const TOtherBaseRef &otherBaseRef, rdxLargeUInt newOffset)
		: rdxTypelessOffsetObjRef<TCounting, TBaseRef>(signal, otherBaseRef, newOffset)
	{
	}

	RDX_FORCEINLINE rdxOffsetObjRef(rdxObjRef_CSignal_RawRef_Type signal, void *ptr, rdxLargeUInt newOffset)
		: rdxTypelessOffsetObjRef<TCounting, TBaseRef>(signal, ptr, newOffset)
	{
	}

	RDX_FORCEINLINE rdxOffsetObjRef(rdxObjRef_CSignal_DataPointer_Type signal, rdxCObject *basePtr, T *internalPtr)
		: rdxTypelessOffsetObjRef<TCounting, TBaseRef>(signal, basePtr, internalPtr)
	{
	}

	RDX_FORCEINLINE rdxOffsetObjRef(rdxObjRef_CSignal_GCInfo_Type signal, const rdxGCInfo *gcinfo, rdxLargeUInt newOffset)
		: rdxTypelessOffsetObjRef<TCounting, TBaseRef>(signal, gcinfo, newOffset)
	{
	}

	RDX_FORCEINLINE ~rdxOffsetObjRef()
	{
	}
	
	template<bool TOtherCounting, class TOtherRefPOD>
	RDX_FORCEINLINE rdxOffsetObjRef<T, TCounting, TBaseRef> &operator =(const rdxOffsetObjRef<T, TOtherCounting, TOtherRefPOD> &other)
	{
		this->m_objectRef = *other.GetObjRef();
		this->m_offset = other.GetObjByteOffset();
		return *this;
	}
	
	RDX_FORCEINLINE rdxOffsetObjRef<T, TCounting, TBaseRef> &operator =(const rdxOffsetObjRef<T, TCounting, TBaseRef> &other)
	{
		this->m_objectRef = *other.GetObjRef();
		this->m_offset = other.GetObjByteOffset();
		return *this;
	}

	RDX_FORCEINLINE rdxOffsetObjRef<T, TCounting, TBaseRef> &operator ++()
	{
		this->m_offset += sizeof(T);
		return *this;
	}

	RDX_FORCEINLINE rdxOffsetObjRef<T, TCounting, TBaseRef> operator ++(int)
	{
		rdxLargeUInt oldOffset = this->m_offset;
		this->m_offset += sizeof(T);
		return rdxOffsetObjRef<T, TCounting, TBaseRef>(this->m_objectRef.ToWeakRTRef(), oldOffset);
	}

	RDX_FORCEINLINE rdxOffsetObjRef<T, TCounting, TBaseRef> &operator --()
	{
		this->m_offset -= sizeof(T);
		return *this;
	}

	RDX_FORCEINLINE rdxOffsetObjRef<T, TCounting, TBaseRef> operator --(int)
	{
		rdxLargeUInt oldOffset = this->m_offset;
		this->m_offset -= sizeof(T);
		return rdxOffsetObjRef<T, TCounting, TBaseRef>(this->m_objectRef.ToWeakRTRef(), oldOffset);
	}

	RDX_FORCEINLINE rdxOffsetObjRef<T, TCounting, TBaseRef> operator +(rdxLargeInt rs) const
	{
		rdxLargeUInt newOffset = static_cast<rdxLargeUInt>(static_cast<rdxLargeInt>(this->m_offset) + rs * static_cast<rdxLargeInt>(sizeof(T)));
		return rdxOffsetObjRef<T, TCounting, TBaseRef>(rdxObjRef_CSignal_BaseRef, this->m_objectRef, newOffset);
	}

	RDX_FORCEINLINE rdxOffsetObjRef<T, TCounting, TBaseRef> operator +(rdxLargeUInt rs) const
	{
		return rdxOffsetObjRef<T, TCounting, TBaseRef>(rdxObjRef_CSignal_BaseRef, this->m_objectRef, this->m_offset + rs * sizeof(T));
	}

	RDX_FORCEINLINE rdxOffsetObjRef<T, TCounting, TBaseRef> operator -(rdxLargeInt rs) const
	{
		rdxLargeUInt newOffset = static_cast<rdxLargeUInt>(static_cast<rdxLargeInt>(this->m_offset) - rs * static_cast<rdxLargeInt>(sizeof(T)));
		return rdxOffsetObjRef<void, TCounting, TBaseRef>(this, newOffset);
	}

	RDX_FORCEINLINE rdxOffsetObjRef<T, TCounting, TBaseRef> operator -(rdxLargeUInt rs) const
	{
		return rdxOffsetObjRef<void, TCounting, TBaseRef>(this, this->m_offset + rs * sizeof(T));
	}

	template<bool TOtherCounting>
	RDX_FORCEINLINE rdxLargeInt operator -(const rdxOffsetObjRef<T, TOtherCounting, TBaseRef> &otherRef) const
	{
		return (static_cast<rdxLargeInt>(this->m_offset) - static_cast<rdxLargeInt>(otherRef.GetObjByteOffset())) / static_cast<rdxLargeInt>(sizeof(T));
	}

	RDX_FORCEINLINE rdxOffsetObjRef<T, TCounting, TBaseRef> &operator +=(rdxLargeInt rs)
	{
		this->m_offset = static_cast<rdxLargeUInt>(static_cast<rdxLargeInt>(this->m_offset) + rs * static_cast<rdxLargeInt>(sizeof(T)));
		return *this;
	}

	RDX_FORCEINLINE rdxOffsetObjRef<T, TCounting, TBaseRef> &operator +=(rdxLargeUInt rs)
	{
		this->m_offset += rs * sizeof(T);
		return *this;
	}

	RDX_FORCEINLINE rdxOffsetObjRef<T, TCounting, TBaseRef> &operator -=(rdxLargeInt rs)
	{
		this->m_offset = static_cast<rdxLargeUInt>(static_cast<rdxLargeInt>(this->m_offset) - rs * static_cast<rdxLargeInt>(sizeof(T)));
		return *this;
	}

	RDX_FORCEINLINE rdxOffsetObjRef<T, TCounting, TBaseRef> &operator -=(rdxLargeUInt rs)
	{
		this->m_offset -= rs * sizeof(T);
		return *this;
	}

	RDX_FORCEINLINE T *operator ->() const
	{
		return this->Modify();
	}
	
	template<bool TNewCounting, class TNewRefPOD>
	RDX_FORCEINLINE rdxOffsetObjRef<T, TNewCounting, TNewRefPOD> Reclassify() const
	{
		return rdxOffsetObjRef<T, TNewCounting, TNewRefPOD>(rdxObjRef_CSignal_BaseRef, this->m_baseRef, this->m_offset);
	}

	RDX_FORCEINLINE typename rdxWeakOffsetRTRef(T) ToRTRef() const
	{
		return rdxWeakOffsetRTRef(T)(rdxObjRef_CSignal_BaseRef, this->m_objectRef, this->m_offset);
	}

	RDX_FORCEINLINE typename rdxWeakOffsetHdl(T) ToHdl() const
	{
		return typename rdxWeakOffsetHdl(T)(rdxObjRef_CSignal_BaseRef, this->m_objectRef, this->m_offset);
	}

	RDX_FORCEINLINE const T *Data() const
	{
		return reinterpret_cast<const T *>(reinterpret_cast<const rdxUInt8 *>(this->m_objectRef.Data()) + this->m_offset);
	}

	RDX_FORCEINLINE T *Modify() const
	{
		return reinterpret_cast<T *>(reinterpret_cast<rdxUInt8 *>(this->m_objectRef.Modify()) + this->m_offset);
	}

	template<class TNew>
	RDX_FORCEINLINE rdxOffsetObjRef<TNew, TCounting, TBaseRef> StaticCast() const
	{
		TNew *validator = static_cast<TNew*>(static_cast<T*>(RDX_CNULL));
		return rdxOffsetObjRef<TNew, TCounting, TBaseRef>(rdxObjRef_CSignal_RefPOD, this->m_objectRef.GetPOD(), this->m_offset);
	}
};

template<class T>
struct rdxSRefTypeInfo
{
	typedef typename rdxTracedRTRef(T) TracedRTRefType;

	inline static void Fetch(rdxIfcTypeInfo::EDataIndex index, rdxLargeUInt *uiOut, rdxUInt32 *u32out, void **ptrOut)
	{
		switch(index)
		{
		case rdxIfcTypeInfo::EData_Size: *uiOut = sizeof(TracedRTRefType); return;
		case rdxIfcTypeInfo::EData_Alignment: *uiOut = rdxAlignOf(TracedRTRefType); return;
		case rdxIfcTypeInfo::EData_SizeInSubclass: *uiOut = sizeof(TracedRTRefType) - rdxSSubclassByteAbsorptionCounter<TracedRTRefType>::NUM_ABSORBED; return;
		case rdxIfcTypeInfo::EData_ObjectBaseOffset: *uiOut = 0; return;
		case rdxIfcTypeInfo::EData_Flags: *u32out = 0; return;
		case rdxIfcTypeInfo::EData_TypeFuncs: *ptrOut = RDX_CNULL; return;
		case rdxIfcTypeInfo::EData_BoxTypeInfo: *ptrOut = RDX_CNULL; return;
		};
	}
	inline static rdxIfcTypeInfo TypeInfoInterface()
	{
		rdxIfcTypeInfo typeInfoInterface;
		typeInfoInterface.fetchFunc = Fetch;
		return typeInfoInterface;
	}
};

template<class T>
struct rdxSInterfaceRefTypeInfo
{
	typedef typename rdxTracedIRef(T) TracedIRefType;

	inline static void Fetch(rdxIfcTypeInfo::EDataIndex index, rdxLargeUInt *uiOut, rdxUInt32 *u32out, void **ptrOut)
	{
		switch(index)
		{
		case rdxIfcTypeInfo::EData_Size: *uiOut = sizeof(TracedIRefType); return;
		case rdxIfcTypeInfo::EData_Alignment: *uiOut = rdxAlignOf(TracedIRefType); return;
		case rdxIfcTypeInfo::EData_SizeInSubclass: *uiOut = sizeof(TracedIRefType) - rdxSSubclassByteAbsorptionCounter<TracedIRefType>::NUM_ABSORBED; return;
		case rdxIfcTypeInfo::EData_ObjectBaseOffset: *uiOut = 0; return;
		case rdxIfcTypeInfo::EData_Flags: *u32out = 0; return;
		case rdxIfcTypeInfo::EData_TypeFuncs: *ptrOut = RDX_CNULL; return;
		case rdxIfcTypeInfo::EData_BoxTypeInfo: *ptrOut = RDX_CNULL; return;
		};
	}
	inline static rdxIfcTypeInfo TypeInfoInterface()
	{
		rdxIfcTypeInfo typeInfoInterface;
		typeInfoInterface.fetchFunc = Fetch;
		return typeInfoInterface;
	}
};


#ifdef RDX_BUILD_COREMODULE

template<class T>
struct rdxSAutoTypeInfo<rdxRTRef<T, rdxBaseRTRef> >
{
	inline static rdxIfcTypeInfo TypeInfoInterface()
	{
		return rdxSRefTypeInfo<T>::TypeInfoInterface();
	}
};

template<class T>
struct rdxSAutoTypeInfo<rdxRTRef<T, rdxBaseIfcRef> >
{
	inline static rdxIfcTypeInfo TypeInfoInterface()
	{
		return rdxSInterfaceRefTypeInfo<T>::TypeInfoInterface();
	}
};

#endif


struct rdxRuntimeOffsetRTRef
{
	rdxTracedTypelessRTRef rtref;
	rdxLargeUInt offset;
};

struct rdxOffsetHdlPOD
{
	rdxBaseHdl::PODType	hdl;
	rdxLargeUInt		offset;
};

#include "rdx_reftypecode.hpp"

#endif
