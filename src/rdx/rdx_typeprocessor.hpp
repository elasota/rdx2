#ifndef __RDX_TYPEPROCESSOR_HPP__
#define __RDX_TYPEPROCESSOR_HPP__

#include "rdx_callbacks.hpp"
#include "rdx_typeprocessordefs.hpp"

struct rdxIObjectManager;
struct rdxIObjectReferenceVisitor;
struct rdxSOperationContext;
class rdxCStructuredType;
struct rdxSObjectGUID;
struct rdxSObjectGUID;
class rdxCString;
class rdxCObject;
struct rdxGCInfo;
template<class T> struct rdxSDebugTypeInspector;
template<class T> struct rdxRefType;

template<class T, bool TRequireMember>
struct rdxSAutoTypeFuncInvoker
{
};

template<class T>
struct rdxSAutoTypeFuncInvoker<T, true>
{
	static void Copy(void *dest, const void *source);
	static void Destroy(void *obj);
	static void Create(void *dest, rdxIObjectManager *objm, rdxGCInfo *info);
	static void Assign(void *dest, const void *source);
	static void Steal(void *dest, void *source);
	static void VisitReferences(rdxIObjectManager *objm, void *objData, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable);
	static bool OnLoad(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCObject) obj);
	static bool GetPropertyOffset(rdxWeakRTRef(rdxCString) propertyName, rdxLargeUInt *outOffset);
	static bool DuplicateEqual(const void *a, const void *b);
	static void *RefToObjectHead(rdxCObject *in);
	static rdxCObject *ObjectHeadToRef(void *in);
	
	inline static rdxIfcTypeFuncs::CopyFunc CopyFunc() { return Copy; }
	inline static rdxIfcTypeFuncs::DestroyFunc DestroyFunc() { return Destroy; }
	inline static rdxIfcTypeFuncs::CreateFunc CreateFunc() { return Create; }
	inline static rdxIfcTypeFuncs::AssignFunc AssignFunc() { return Assign; }
	inline static rdxIfcTypeFuncs::StealFunc StealFunc() { return Steal; }
	inline static rdxIfcTypeFuncs::VisitReferencesFunc VisitReferencesFunc() { return VisitReferences; }
	inline static rdxIfcTypeFuncs::OnLoadFunc OnLoadFunc() { return OnLoad; }
	inline static rdxIfcTypeFuncs::DuplicateEqualFunc DuplicateEqualFunc() { return DuplicateEqual; }
	inline static rdxIfcTypeFuncs::RefToObjectHeadFunc RefToObjectHeadFunc() { return RefToObjectHead; }
	inline static rdxIfcTypeFuncs::ObjectHeadToRefFunc ObjectHeadToRefFunc() { return ObjectHeadToRef; }
};

template<class T>
struct rdxSAutoTypeFuncInvoker<T, false>
{
	inline static rdxIfcTypeFuncs::CopyFunc CopyFunc() { return RDX_CNULL; }
	inline static rdxIfcTypeFuncs::DestroyFunc DestroyFunc() { return RDX_CNULL; }
	inline static rdxIfcTypeFuncs::CreateFunc CreateFunc() { return RDX_CNULL; }
	inline static rdxIfcTypeFuncs::AssignFunc AssignFunc() { return RDX_CNULL; }
	inline static rdxIfcTypeFuncs::StealFunc StealFunc() { return RDX_CNULL; }
	inline static rdxIfcTypeFuncs::VisitReferencesFunc VisitReferencesFunc() { return RDX_CNULL; }
	inline static rdxIfcTypeFuncs::OnLoadFunc OnLoadFunc() { return RDX_CNULL; }
	inline static rdxIfcTypeFuncs::DuplicateEqualFunc DuplicateEqualFunc() { return RDX_CNULL; }
	inline static rdxIfcTypeFuncs::RefToObjectHeadFunc RefToObjectHeadFunc() { return RDX_CNULL; }
	inline static rdxIfcTypeFuncs::ObjectHeadToRefFunc ObjectHeadToRefFunc() { return RDX_CNULL; }
};

template<class T, rdxLargeUInt TFlags>
struct rdxSAutoTypeFuncs
{
	static void *GetProcFunc(rdxIfcTypeFuncs::EFuncIndex funcIndex);
	static rdxIfcTypeFuncs TypeFuncsInterface();
};


////////////////////////////////////////////////////////////////////////////////


template<class T>
class rdxCInspectorCreator
{
public:
	inline static void CreateInspector(void *inspectorLoc, void *objPtrLoc)
	{
		rdxSDebugTypeInspector<T> *inspector = new (inspectorLoc) rdxSDebugTypeInspector<T>();
		inspector->m_objectVPtr = static_cast<T**>(objPtrLoc);
	}
};



//////////////////////////////////////////////////////////

#include <new>
#include "rdx_reftypedefs.hpp"


template<class T, rdxLargeUInt TFlags>
inline void *rdxSAutoTypeFuncs<T, TFlags>::GetProcFunc(rdxIfcTypeFuncs::EFuncIndex funcIndex)
{
	switch(funcIndex)
	{
	case rdxIfcTypeFuncs::EFunc_Copy: return reinterpret_cast<void*>(rdxSAutoTypeFuncInvoker<T, (TFlags & rdxETIF_LifeCycle) != 0>::CopyFunc());
	case rdxIfcTypeFuncs::EFunc_Destroy: return reinterpret_cast<void*>(rdxSAutoTypeFuncInvoker<T, (TFlags & rdxETIF_LifeCycle) != 0>::DestroyFunc());
	case rdxIfcTypeFuncs::EFunc_Create: return reinterpret_cast<void*>(rdxSAutoTypeFuncInvoker<T, (TFlags & rdxETIF_LifeCycle) != 0>::CreateFunc());
	case rdxIfcTypeFuncs::EFunc_Assign: return reinterpret_cast<void*>(rdxSAutoTypeFuncInvoker<T, (TFlags & rdxETIF_LifeCycle) != 0>::AssignFunc());
	case rdxIfcTypeFuncs::EFunc_Steal: return reinterpret_cast<void*>(rdxSAutoTypeFuncInvoker<T, (TFlags & rdxETIF_Steal) != 0>::StealFunc());
	case rdxIfcTypeFuncs::EFunc_VisitReferences: return reinterpret_cast<void*>(rdxSAutoTypeFuncInvoker<T, (TFlags & rdxETIF_VisitReferences) != 0>::VisitReferencesFunc());
	case rdxIfcTypeFuncs::EFunc_OnLoad: return reinterpret_cast<void*>(rdxSAutoTypeFuncInvoker<T, (TFlags & rdxETIF_OnLoad) != 0>::OnLoadFunc());
	case rdxIfcTypeFuncs::EFunc_DuplicateEqual: return reinterpret_cast<void*>(rdxSAutoTypeFuncInvoker<T, (TFlags & rdxETIF_CompareDuplicate) != 0>::DuplicateEqualFunc());
	case rdxIfcTypeFuncs::EFunc_RefToObjectHead: return reinterpret_cast<void*>(rdxSAutoTypeFuncInvoker<T, (TFlags & rdxETIF_RefAdjust) != 0>::RefToObjectHeadFunc());
	case rdxIfcTypeFuncs::EFunc_ObjectHeadToRef: return reinterpret_cast<void*>(rdxSAutoTypeFuncInvoker<T, (TFlags & rdxETIF_RefAdjust) != 0>::ObjectHeadToRefFunc());
	case rdxIfcTypeFuncs::EFunc_CreateInspector: return reinterpret_cast<void*>(rdxCInspectorCreator<T>::CreateInspector);
	};
	return RDX_CNULL;
}

	
template<class T, rdxLargeUInt TFlags>
inline rdxIfcTypeFuncs rdxSAutoTypeFuncs<T, TFlags>::TypeFuncsInterface()
{
	rdxIfcTypeFuncs typeFuncsInterface;
	typeFuncsInterface.getProcFunc = GetProcFunc;
	return typeFuncsInterface;
}

struct rdxINativeTypeHost
{
	virtual rdxIfcTypeInfo TypeInfoForType(rdxWeakRTRef(rdxCStructuredType) st) const = 0;
	virtual rdxNativeCallback HookMethod(rdxSObjectGUID methodGUID) const = 0;
	virtual rdxLargeUInt NumPlugins() const = 0;
	virtual void GetPlugin(rdxLargeUInt pluginIndex, const rdxSPCCMDomainIndex **pPCCM, const rdxSPluginExport **pPlugin) const = 0;
};

struct rdxIPluginProvider
{
	virtual rdxLargeUInt NumPlugins() const = 0;
	virtual void GetPlugin(rdxLargeUInt pluginIndex, const rdxSPCCMDomainIndex **pPCCM, const rdxSPluginExport **pPlugin) = 0;
};

template<class T>
inline void rdxSAutoTypeFuncInvoker<T, true>::Copy(void *dest, const void *source)
{
	new (dest) T(*static_cast<const T *>(source));
}

template<class T>
inline void rdxSAutoTypeFuncInvoker<T, true>::Destroy(void *obj)
{
	static_cast<T*>(obj)->~T();
}

template<class T>
inline void rdxSAutoTypeFuncInvoker<T, true>::Create(void *dest, rdxIObjectManager *objm, rdxGCInfo *info)
{
	new (dest) T(objm, info);
}

template<class T>
inline void rdxSAutoTypeFuncInvoker<T, true>::Assign(void *dest, const void *source)
{
	*static_cast<T*>(dest) = *static_cast<const T *>(source);
}

template<class T>
inline void rdxSAutoTypeFuncInvoker<T, true>::Steal(void *dest, void *source)
{
	static_cast<T*>(dest)->Steal(static_cast<T*>(source));
}

template<class T>
inline void rdxSAutoTypeFuncInvoker<T, true>::VisitReferences(rdxIObjectManager *objm, void *objData, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
{
	T *ref = static_cast<T*>(objData);
	ref->VisitReferences(objm, visitor, visitNonSerializable);
}

template<class T>
inline bool rdxSAutoTypeFuncInvoker<T, true>::OnLoad(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCObject) obj)
{
	return obj.StaticCast<T>().Modify()->OnLoad(ctx, objm);
}

template<class T>
inline bool rdxSAutoTypeFuncInvoker<T, true>::DuplicateEqual(const void *a, const void *b)
{
	return static_cast<const T*>(a)->DuplicateEqual(*static_cast<const T*>(b));
}

template<class T>
inline void *rdxSAutoTypeFuncInvoker<T, true>::RefToObjectHead(rdxCObject *in)
{
	return static_cast<T*>(in);
}

template<class T>
inline rdxCObject *rdxSAutoTypeFuncInvoker<T, true>::ObjectHeadToRef(void *in)
{
	return static_cast<T*>(in);
}

#endif
