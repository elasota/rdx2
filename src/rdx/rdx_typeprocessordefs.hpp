#ifndef __RDX_TYPEPROCESSORDEFS_HPP__
#define __RDX_TYPEPROCESSORDEFS_HPP__

#include "rdx_reftypealiases.hpp"

struct rdxIObjectReferenceVisitor;
struct rdxIObjectManager;
struct rdxGCInfo;
struct rdxSOperationContext;
template<class T, rdxLargeUInt TFlags> struct rdxSAutoTypeFuncs;

enum rdxETypeInfoFlags
{
	rdxETIF_NoFlags				= 0,
	rdxETIF_LifeCycle			= 1,
	rdxETIF_VisitReferences		= 8,
	rdxETIF_OnLoad				= 32,
	rdxETIF_AllowBulk			= 64,
	rdxETIF_CompareDuplicate	= 128,
	rdxETIF_Steal				= 256,
	rdxETIF_RefAdjust			= 512,
	rdxETIF_ArrayFlag			= 1024,		// Flag GCInfo as Array
	rdxETIF_BoxFlag				= 2048,		// Flag GCInfo as Box

	rdxETIF_StandardClass		= (rdxETIF_LifeCycle | rdxETIF_RefAdjust)
};


struct rdxIfcTypeFuncs
{
	enum EFuncIndex
	{
		EFunc_Destroy,
		EFunc_Create,
		EFunc_Copy,
		EFunc_Assign,
		EFunc_Steal,
		EFunc_VisitReferences,
		EFunc_OnLoad,
		EFunc_DuplicateEqual,
		EFunc_CreateInspector,
		EFunc_RefToObjectHead,
		EFunc_ObjectHeadToRef,
	};

	typedef void *(*GetProcFunc)(EFuncIndex funcIndex);

	typedef void (*DestroyFunc)(void *obj);
	typedef void (*CreateFunc)(void *dest, rdxIObjectManager *objm, rdxGCInfo *info);
	typedef void (*CopyFunc)(void *dest, const void *source);
	typedef void (*AssignFunc)(void *dest, const void *source);
	typedef void (*StealFunc)(void *dest, void *source);
	typedef bool (*DuplicateEqualFunc)(const void *a, const void *b);
	typedef void (*CreateInspectorFunc)(void *pos, void *objPtrLoc);
	typedef void *(*RefToObjectHeadFunc)(rdxCObject *object);
	typedef rdxCObject *(*ObjectHeadToRefFunc)(void *ptr);
	typedef void (*VisitReferencesFunc)(rdxIObjectManager *objm, void *objData, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable);
	typedef bool (*OnLoadFunc)(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCObject) obj);

	GetProcFunc getProcFunc;

	inline DestroyFunc GetDestroyFunc() const
	{
		return reinterpret_cast<DestroyFunc>(getProcFunc(EFunc_Destroy));
	}

	inline CreateFunc GetCreateFunc() const
	{
		return reinterpret_cast<CreateFunc>(getProcFunc(EFunc_Create));
	}

	inline CopyFunc GetCopyFunc() const
	{
		return reinterpret_cast<CopyFunc>(getProcFunc(EFunc_Copy));
	}

	inline AssignFunc GetAssignFunc() const
	{
		return reinterpret_cast<AssignFunc>(getProcFunc(EFunc_Assign));
	}

	inline StealFunc GetStealFunc() const
	{
		return reinterpret_cast<StealFunc>(getProcFunc(EFunc_Steal));
	}

	inline DuplicateEqualFunc GetDuplicateEqualFunc() const
	{
		return reinterpret_cast<DuplicateEqualFunc>(getProcFunc(EFunc_DuplicateEqual));
	}

	inline CreateInspectorFunc GetCreateInspectorFunc() const
	{
		return reinterpret_cast<CreateInspectorFunc>(getProcFunc(EFunc_CreateInspector));
	}

	inline RefToObjectHeadFunc GetRefToObjectHeadFunc() const
	{
		return reinterpret_cast<RefToObjectHeadFunc>(getProcFunc(EFunc_RefToObjectHead));
	}

	inline ObjectHeadToRefFunc GetObjectHeadToRefFunc() const
	{
		return reinterpret_cast<ObjectHeadToRefFunc>(getProcFunc(EFunc_ObjectHeadToRef));
	}

	inline VisitReferencesFunc GetVisitReferencesFunc() const
	{
		return reinterpret_cast<VisitReferencesFunc>(getProcFunc(EFunc_VisitReferences));
	}

	inline OnLoadFunc GetOnLoadFunc() const
	{
		return reinterpret_cast<OnLoadFunc>(getProcFunc(EFunc_OnLoad));
	}

	inline bool IsNull() const
	{
		return getProcFunc == RDX_CNULL;
	}

	inline bool IsNotNull() const
	{
		return getProcFunc != RDX_CNULL;
	}
};

struct rdxIfcTypeInfo
{
	enum EDataIndex
	{
		EData_Size,
		EData_Alignment,
		EData_SizeInSubclass,
		EData_ObjectBaseOffset,
		EData_Flags,
		EData_TypeFuncs,
		EData_BoxTypeInfo,
	};

	typedef void (*FetchFunc)(EDataIndex index, rdxLargeUInt *uiOut, rdxUInt32 *u32out, void **ptrOut);

	FetchFunc fetchFunc;

	inline rdxLargeUInt Size() const
	{
		rdxLargeUInt rv;
		this->fetchFunc(EData_Size, &rv, RDX_CNULL, RDX_CNULL);
		return rv;
	}

	inline rdxLargeUInt Alignment() const
	{
		rdxLargeUInt rv;
		this->fetchFunc(EData_Alignment, &rv, RDX_CNULL, RDX_CNULL);
		return rv;
	}

	inline rdxLargeUInt SizeInSubclass() const
	{
		rdxLargeUInt rv;
		this->fetchFunc(EData_SizeInSubclass, &rv, RDX_CNULL, RDX_CNULL);
		return rv;
	}

	inline rdxLargeUInt ObjectBaseOffset() const
	{
		rdxLargeUInt rv;
		this->fetchFunc(EData_ObjectBaseOffset, &rv, RDX_CNULL, RDX_CNULL);
		return rv;
	}

	inline rdxUInt32 Flags() const
	{
		rdxUInt32 rv;
		this->fetchFunc(EData_Flags, RDX_CNULL, &rv, RDX_CNULL);
		return rv;
	}

	inline rdxIfcTypeFuncs TypeFuncs() const
	{
		rdxIfcTypeFuncs rv;
		void *ptr;
		this->fetchFunc(EData_TypeFuncs, RDX_CNULL, RDX_CNULL, &ptr);
		rv.getProcFunc = reinterpret_cast<rdxIfcTypeFuncs::GetProcFunc>(ptr);
		return rv;
	}

	inline rdxIfcTypeInfo BoxTypeInfo() const
	{
		rdxIfcTypeInfo rv;
		void *ptr;
		this->fetchFunc(EData_BoxTypeInfo, RDX_CNULL, RDX_CNULL, &ptr);
		rv.fetchFunc = reinterpret_cast<rdxIfcTypeInfo::FetchFunc>(ptr);
		return rv;
	}

	inline bool IsNull() const
	{
		return fetchFunc == RDX_CNULL;
	}

	inline bool IsNotNull() const
	{
		return fetchFunc != RDX_CNULL;
	}
};


template<class T>
struct rdxSSimpleTypeInfo
{
};

template<class T>
struct rdxSInvokingTypeInfo
{
};

// This handles counting tail padding space for Itanium C++ ABI
template<class T, int TNumSubclasses> struct rdxSSubclassByteAbsorptionChain;

template<class T, int TNumSubclasses>
struct rdxSSubclassByteAbsorptionChain : public rdxSSubclassByteAbsorptionChain<T, TNumSubclasses - 1>
{
	rdxUInt8 padByte;
};

template<class T>
struct rdxSSubclassByteAbsorptionChain<T, 0> : public T
{
};

template<bool TNeedMore, class T, int TAbsorbed> struct rdxSSubclassByteAbsorptionCounterRecursive;

template<class T, int TAbsorbed>
struct rdxSSubclassByteAbsorptionCounterRecursive<true, T, TAbsorbed>
{
	enum
	{
		NUM_ABSORBED = rdxSSubclassByteAbsorptionCounterRecursive<
			sizeof(rdxSSubclassByteAbsorptionChain<T, TAbsorbed>) == sizeof(rdxSSubclassByteAbsorptionChain<T, TAbsorbed+1>),
			T, TAbsorbed+1>::NUM_ABSORBED
	};
};

template<class T, int TAbsorbed>
struct rdxSSubclassByteAbsorptionCounterRecursive<false, T, TAbsorbed>
{
	enum
	{
		NUM_ABSORBED = TAbsorbed - 1
	};
};

template<class TBase>
struct rdxSSubclassByteAbsorptionCounter
{
	enum
	{
		NUM_ABSORBED = rdxSSubclassByteAbsorptionCounterRecursive<true, TBase, 0>::NUM_ABSORBED
	};
};

#define RDX_DECLARE_PROPERTY_LOOKUP public: static bool GetPropertyOffset(rdxWeakRTRef(rdxCString) propertyName, rdxLargeUInt *outOffset)

#define RDX_BEGIN_PROPERTY_LOOKUP_STRUCT(structureType)	\
	bool structureType::GetPropertyOffset(rdxWeakRTRef(rdxCString) propertyName, rdxLargeUInt *outOffset)	\
	{\
		typedef structureType PropertyLookupBase;

#define RDX_BEGIN_PROPERTY_LOOKUP_CLASS(structureType)	\
	bool structureType::GetPropertyOffset(rdxWeakRTRef(rdxCString) propertyName, rdxLargeUInt *outOffset)	\
	{\
		typedef structureType PropertyLookupBase;\
		if(rdxSuper(PropertyLookupBase)::GetPropertyOffset(propertyName, outOffset))\
			return true;

#define RDX_DEFINE_LOOKUP_PROPERTY(prop)	\
	if(propertyName->Equal(#prop))\
	{\
		*outOffset = rdxRuntimePropertyOffsetOf(&PropertyLookupBase::prop);\
		return true;\
	}


#define RDX_END_PROPERTY_LOOKUP	\
		return false;\
	}

#define RDX_DECLARE_BOXED_TYPE(T)	\
	template<>\
	struct rdxSInvokingTypeInfo<rdxCBox<T> >\
	{\
		static rdxIfcTypeInfo TypeInfoInterface();\
		static rdxIfcTypeFuncs TypeFuncsInterface();\
		static void Fetch(rdxIfcTypeInfo::EDataIndex index, rdxLargeUInt *uiOut, rdxUInt32 *u32out, void **ptrOut);\
	};\
	template<>\
	struct rdxSAutoTypeInfo<rdxCBox<T> >\
	{\
		typedef rdxCBox<T> ThisType;\
		static rdxIfcTypeInfo TypeInfoInterface();\
	}

template<class TContents>
struct rdxSBoxedTypeInfo
{
	static rdxIfcTypeFuncs TypeFuncsInterface()
	{
		return rdxSAutoTypeFuncs<rdxCBox<TContents>, (rdxCStructContainer::TYPE_FLAGS)>::TypeFuncsInterface();
	}
	static void Fetch(rdxIfcTypeInfo::EDataIndex index, rdxLargeUInt *uiOut, rdxUInt32 *u32out, void **ptrOut)
	{
		switch(index)\
		{
		case rdxIfcTypeInfo::EData_Size: *uiOut = sizeof(rdxCBox<TContents>); return;
		case rdxIfcTypeInfo::EData_Alignment: *uiOut = rdxAlignOf(rdxCBox<TContents>); return;
		case rdxIfcTypeInfo::EData_SizeInSubclass: *uiOut = sizeof(rdxCBox<TContents>) - rdxSSubclassByteAbsorptionCounter<rdxCBox<TContents> >::NUM_ABSORBED; return;
		case rdxIfcTypeInfo::EData_ObjectBaseOffset: *uiOut = 0; return;
		case rdxIfcTypeInfo::EData_Flags: *u32out = (rdxCStructContainer::TYPE_FLAGS); return;
		case rdxIfcTypeInfo::EData_TypeFuncs: *ptrOut = reinterpret_cast<void*>(rdxSBoxedTypeInfo<TContents>::TypeFuncsInterface().getProcFunc); return;
		case rdxIfcTypeInfo::EData_BoxTypeInfo: *ptrOut = RDX_CNULL; return;
		};
	}
	static rdxIfcTypeInfo TypeInfoInterface()
	{
		rdxIfcTypeInfo typeInfoInterface;
		typeInfoInterface.fetchFunc = Fetch;
		return typeInfoInterface;
	}
};

#define RDX_IMPLEMENT_BOXED_TYPE(T)	\
	rdxIfcTypeInfo rdxSAutoTypeInfo<rdxCBox<T> >::TypeInfoInterface()\
	{\
		return rdxSBoxedTypeInfo<T>::TypeInfoInterface();\
	}

template<class T>
struct rdxSSimpleStructTypeInfo
{
	static void Fetch(rdxIfcTypeInfo::EDataIndex index, rdxLargeUInt *uiOut, rdxUInt32 *u32out, void **ptrOut)
	{
		switch(index)
		{
		case rdxIfcTypeInfo::EData_Size: *uiOut = sizeof(T); return;
		case rdxIfcTypeInfo::EData_Alignment: *uiOut = rdxAlignOf(T); return;
		case rdxIfcTypeInfo::EData_SizeInSubclass: *uiOut = sizeof(T); return;
		case rdxIfcTypeInfo::EData_ObjectBaseOffset: *uiOut = 0; return;
		case rdxIfcTypeInfo::EData_Flags: *u32out = (0); return;
		case rdxIfcTypeInfo::EData_TypeFuncs: *ptrOut = RDX_CNULL; return;
		case rdxIfcTypeInfo::EData_BoxTypeInfo: *ptrOut = reinterpret_cast<void*>(rdxSAutoTypeInfo<rdxCBox<T> >::TypeInfoInterface().fetchFunc); return;
		};
	}
	static rdxIfcTypeInfo TypeInfoInterface()
	{
		rdxIfcTypeInfo typeInfoInterface;
		typeInfoInterface.fetchFunc = Fetch;
		return typeInfoInterface;
	}
};

#define RDX_IMPLEMENT_SIMPLE_NATIVE_STRUCT(T)	\
	RDX_IMPLEMENT_BOXED_TYPE(T);\
	rdxIfcTypeInfo rdxSAutoTypeInfo<T>::TypeInfoInterface()\
	{\
		return rdxSSimpleStructTypeInfo<T>::TypeInfoInterface();\
	}\

#define RDX_DECLARE_SIMPLE_NATIVE_STRUCT(T)	\
	RDX_DECLARE_BOXED_TYPE(T);\
	template<>\
	struct rdxSAutoTypeInfo<T>\
	{\
		static rdxIfcTypeInfo TypeInfoInterface();\
	}

template<class T, rdxUInt32 TTypeFlags>
struct rdxSComplexStructTypeInfo
{
	static rdxIfcTypeFuncs TypeFuncsInterface()
	{
		return rdxSAutoTypeFuncs<T, (TTypeFlags)>::TypeFuncsInterface();
	}
	static void Fetch(rdxIfcTypeInfo::EDataIndex index, rdxLargeUInt *uiOut, rdxUInt32 *u32out, void **ptrOut)
	{
		switch(index)
		{
		case rdxIfcTypeInfo::EData_Size: *uiOut = sizeof(T); return;
		case rdxIfcTypeInfo::EData_Alignment: *uiOut = rdxAlignOf(T); return;
		case rdxIfcTypeInfo::EData_SizeInSubclass: *uiOut = sizeof(T) - rdxSSubclassByteAbsorptionCounter<T>::NUM_ABSORBED; return;
		case rdxIfcTypeInfo::EData_ObjectBaseOffset: *uiOut = 0; return;
		case rdxIfcTypeInfo::EData_Flags: *u32out = (TTypeFlags); return;
		case rdxIfcTypeInfo::EData_TypeFuncs: *ptrOut = rdxSComplexStructTypeInfo<T, TTypeFlags>::TypeFuncsInterface().getProcFunc; return;
		case rdxIfcTypeInfo::EData_BoxTypeInfo: *ptrOut = rdxSAutoTypeInfo<rdxCBox<T> >::TypeInfoInterface().fetchFunc; return;
		};
	}
	static rdxIfcTypeInfo TypeInfoInterface()
	{
		rdxIfcTypeInfo typeInfoInterface;
		typeInfoInterface.fetchFunc = Fetch;
		return typeInfoInterface;
	}
};

#define RDX_IMPLEMENT_COMPLEX_NATIVE_STRUCT(T, typeFlags)	\
	RDX_IMPLEMENT_BOXED_TYPE(T);\
	rdxIfcTypeInfo rdxSAutoTypeInfo<T>::TypeInfoInterface()\
	{\
		return rdxSComplexStructTypeInfo<T, (typeFlags)>::TypeInfoInterface();\
	}
	

#define RDX_DECLARE_COMPLEX_NATIVE_STRUCT(T)	\
	RDX_DECLARE_BOXED_TYPE(T);\
	template<>\
	struct rdxSAutoTypeInfo<T>\
	{\
		static rdxIfcTypeInfo TypeInfoInterface();\
	}

template<class T, rdxUInt32 TTypeFlags>
struct rdxSComplexClassTypeInfo
{
	static rdxIfcTypeFuncs TypeFuncsInterface()
	{
		return rdxSAutoTypeFuncs<T, (TTypeFlags | rdxETIF_StandardClass)>::TypeFuncsInterface();
	}
	static void Fetch(rdxIfcTypeInfo::EDataIndex index, rdxLargeUInt *uiOut, rdxUInt32 *u32out, void **ptrOut)
	{
		switch(index)
		{
		case rdxIfcTypeInfo::EData_Size: *uiOut = sizeof(T); return;
		case rdxIfcTypeInfo::EData_Alignment: *uiOut = rdxAlignOf(T); return;
		case rdxIfcTypeInfo::EData_SizeInSubclass: *uiOut = sizeof(T) - rdxSSubclassByteAbsorptionCounter<T>::NUM_ABSORBED; return;
		case rdxIfcTypeInfo::EData_ObjectBaseOffset: *uiOut = rdxOffsetOfBaseClass<T, rdxCObject>(); return;
		case rdxIfcTypeInfo::EData_Flags: *u32out = (TTypeFlags | rdxETIF_LifeCycle | rdxETIF_RefAdjust); return;
		case rdxIfcTypeInfo::EData_TypeFuncs: *ptrOut = rdxSComplexClassTypeInfo<T, TTypeFlags>::TypeFuncsInterface().getProcFunc; return;
		case rdxIfcTypeInfo::EData_BoxTypeInfo: *ptrOut = RDX_CNULL; return;
		};
	}
	static rdxIfcTypeInfo TypeInfoInterface()
	{
		rdxIfcTypeInfo typeInfoInterface;
		typeInfoInterface.fetchFunc = Fetch;
		return typeInfoInterface;
	}
};


#define RDX_IMPLEMENT_COMPLEX_NATIVE_CLASS(T, typeFlags)	\
	rdxIfcTypeInfo rdxSAutoTypeInfo<T>::TypeInfoInterface()\
	{\
		return rdxSComplexClassTypeInfo<T, (typeFlags)>::TypeInfoInterface();\
	}

#define RDX_DECLARE_COMPLEX_NATIVE_CLASS(T)	\
	template<>\
	struct rdxSAutoTypeInfo<T>\
	{\
		static rdxIfcTypeInfo TypeInfoInterface();\
	}

#endif
