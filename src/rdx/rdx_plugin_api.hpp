#ifndef __RDX_PLUGIN_API_HPP__
#define __RDX_PLUGIN_API_HPP__

#include "rdx_coretypes.hpp"
#include "rdx_callbacks.hpp"
#include "rdx_typeprocessordefs.hpp"

struct rdxSExportedCallEnvironment;
struct rdxSOperationContext;
struct rdxIObjectManager;
class rdxCMethod;
class rdxCRuntimeThread;
class rdxCString;
union rdxURuntimeStackValue;

namespace _RDX_CPPX
{
	struct PluginGlue
	{
		template<rdxUInt64 domainGUID, rdxUInt64 methodGUID>
		static int RDX_DECL_API CallMethod(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv);
	};
}

struct rdxSPluginObjectFunctionLookup
{
	rdxUInt64 objectGUID;
	rdxNativeCallback callMethod;
};

struct rdxSPluginDomainFunctionLookup
{
	rdxUInt64 domainGUID;
	rdxLargeUInt numObjects;
	const rdxSPluginObjectFunctionLookup *objects;
};

struct rdxSPluginObjectNTILookup
{
	rdxUInt64 objectGUID;
	rdxIfcTypeInfo typeInfo;
	bool (*getPropertyOffsetFunc)(rdxWeakRTRef(rdxCString) name, rdxLargeUInt *outOffset);
};

struct rdxSPluginDomainNTILookup
{
	rdxUInt64 domainGUID;
	rdxLargeUInt numObjects;
	const rdxSPluginObjectNTILookup *objects;
};

struct rdxSPluginExport
{
	const rdxSPluginDomainFunctionLookup *functionLookups;
	const rdxSPluginDomainNTILookup *ntiLookups;
};

struct rdxPluginUtils
{
	template<class T>
	static void SetInitialRV(T *&rv, void *prv);

	template<class T, class TNext>
	static void LinkMarshalRV(T *&rv, TNext *nextRV);

	template<class T>
	static void LinkMarshalParam(T *&rv, void *prevParam);
};

#include "rdx_runtimestackvalue.hpp"

template<class T>
inline void rdxPluginUtils::SetInitialRV(T *&rv, void *prv)
{
	rv = reinterpret_cast<T*>(prv);
}

template<class T, class TNext>
inline void rdxPluginUtils::LinkMarshalRV(T *&rv, TNext *nextRV)
{
	rdxLargeUInt alignedSize = sizeof(TNext) + rdxALIGN_RuntimeStackValue - 1;
	alignedSize -= alignedSize % rdxALIGN_RuntimeStackValue;
	rv = reinterpret_cast<T*>(reinterpret_cast<rdxUInt8*>(nextRV) + alignedSize);
}


template<class T>
inline void rdxPluginUtils::LinkMarshalParam(T *&rv, void *prevParam)
{
	rdxLargeUInt alignedSize = sizeof(T) + rdxALIGN_RuntimeStackValue - 1;
	alignedSize -= alignedSize % rdxALIGN_RuntimeStackValue;
	rv = reinterpret_cast<T*>(reinterpret_cast<rdxUInt8*>(prevParam) - alignedSize);
}

#endif
