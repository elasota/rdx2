#ifndef __RDX_GCINFO_HPP__
#define __RDX_GCINFO_HPP__

#include "rdx_reftypedefs.hpp"
#include "rdx_gcslots.hpp"
#include "rdx_typerelationships.hpp"
#include "rdx_debugtypeinspector.hpp"

struct rdxIObjectManager;
struct rdxIfcTypeInfo;
class rdxCType;
struct rdxSAllocator;
struct rdxSSerializationTag;
struct rdxSDomainGUID;

struct rdxGCInfo
{
	union VFTUnion
	{
		rdxBaseRTRef::PODType	vftRef;
		rdxBaseHdl::PODType		vftHdl;		// Set during compaction
	};

	rdxGCInfo							*prev[rdxGCL_Count];
	rdxGCInfo							*next[rdxGCL_Count];

	rdxIObjectManager					*ownerObjectManager;

	rdxWeakHdl(rdxCType)				containerType;			// Runtime type of the container, could be an ArrayOfType or a StructuredType
	rdxIfcTypeInfo						typeInfo;				// Custom processor grapher for this object (usually pulled from containedStructure)

	const rdxIDebugTypeInspector		*pInspector;
	rdxSDebugTypeInspector<void>		selfInspector;

#ifdef RDX_ENABLE_DEBUG_GST_SYMBOL
	rdxHandle<rdxChar>					debugGSTSymbol;
#endif

	rdxAtomicUInt						numTracedReferences;
	rdxAtomicUInt						numExternalReferences;

	mutable rdxUInt32					objectFlags;

	rdxLargeUInt						serializationTagOffset;
	rdxSSerializationTag				*serializationTag;

	VFTUnion							vftUnion;

	// GCLists:
	// White: Not GCNotGarbage or GCScanned
	// 

	enum Flags_GC
	{
		GCOF_GCMarkOnAssign				= 1,		// Relevant object, may or may not be scanned
		GCOF_GraphScanned				= 2,		// Has been scanned once, if marked again then goes to ScanOnCleanup
		GCOF_GraphMarked				= 4,		// Marked by a graphing operation
		GCOF_VaryingLayout				= 8,		// Layout may be changed by other threads, can't be scanned except in ST.

		GCOF_Duplicable					= 16,		// Can be duplicated, even if not in DOMAIN_Duplicable.  Does not cause inclusion in packages.
		GCOF_HasSerializationTag		= 32,		// Has a serialization tag
		GCOF_LinkTerminator				= 64,		// GCL terminator
		GCOF_CriticalBorder				= 128,		// Initial is created during critical phase, but not deserialized until non-critical
		GCOF_Core						= 256,		// Built-in core type
		GCOF_Critical					= 512,		// Built-in type, loaded in a separate phase and may only contain references to other critical types and Object
		GCOF_Unloaded					= 1024,		// GCL_Shell
		GCOF_Unprocessed				= 2048,		// GCL_PendingProcess
		GCOF_UnloadedPackage			= 4096,		// GCL_UnloadedPackage
		GCOF_ReferencesMayBeShells		= 8192,		//
		GCOF_TransientDuplicate			= 16384,	// Transient duplicate used for validation, shouldn't be subjected to instance-specific verification

		GCOF_Test						= 32768,	// For debugging only
		GCOF_External					= 65536,	// Object is in a package not in the current serialize scan set
		GCOF_ConstantArray				= 262144,	// Is a constant array
		GCOF_ConstantStructure			= 524288,	// Is a constant structure resource
		GCOF_Array						= 1048576,	// Is an array
		GCOF_ReferenceArray				= 2097152,	// Is a reference array
		GCOF_Cloaked					= 4194304,	// Cloaked, can only be read by resolvers that explicitly handle cloaking
		GCOF_InterfaceArray				= 8388608,	// Is an interface array
		GCOF_Box						= 16777216,	// Is a struct container
		GCOF_UserClass					= 33554432,	// Is a user class, extends a native one
	};

	rdxGCInfo();
	void Unlink(int gcl);
	void LinkBefore(rdxGCInfo *nnext, int gcl);
	void LinkAfter(rdxGCInfo *nprev, int gcl);
	void IncRef() const;
	void DecRef() const;

	void Release(const rdxSAllocator &alloc);

#ifdef RDX_ENABLE_SENTINELS
	rdxUInt32 sequentialID;
	rdxUInt32 *sentinel2;
	rdxUInt32 sentinel1;
#endif
	rdxLargeUInt debugCounter;	// TODO: Remove

	rdxCObject *m_objectDataPtr;

public:
	rdxGCInfo(rdxIObjectManager *ownerObjectManager, rdxWeakHdl(rdxCType) containerType, rdxIfcTypeInfo typeInfo, bool hasSerializationTag, rdxLargeUInt serTagOffset, rdxCObject *objectDataRoot);

	rdxWeakRTRef(rdxCObject) ObjectWeakRTRef() const;
	rdxCRef(rdxCObject) ObjectCRef() const;
	rdxCObject *ObjectDataPtr() const;
	rdxWeakArrayRTRef(rdxTracedRTRef(rdxCMethod)) VFT() const;

	rdxWeakHdl(rdxCStructuredType) ContainedStructure() const;

	rdxSSerializationTag *SerializationTag() const;

	rdxSDomainGUID Domain() const;
};

#include "rdx_utility.hpp"
#include "rdx_serializationtag.hpp"

inline rdxWeakRTRef(rdxCObject) rdxGCInfo::ObjectWeakRTRef() const
{
	return rdxWeakRTRef(rdxCObject)(rdxObjRef_CSignal_GCInfo, this);
}

inline rdxCRef(rdxCObject) rdxGCInfo::ObjectCRef() const
{
	return rdxCRef(rdxCObject)(rdxObjRef_CSignal_GCInfo, this);
}

inline rdxCObject *rdxGCInfo::ObjectDataPtr() const
{
	return this->m_objectDataPtr;
}

inline rdxWeakArrayRTRef(rdxTracedRTRef(rdxCMethod)) rdxGCInfo::VFT() const
{
	return rdxWeakArrayRTRef(rdxTracedRTRef(rdxCMethod))(rdxObjRef_CSignal_RawRef, this->vftUnion.vftRef);
}

inline rdxSSerializationTag *rdxGCInfo::SerializationTag() const
{
	if(this->serializationTagOffset == 0)
		return NULL;
	return reinterpret_cast<rdxSSerializationTag*>(reinterpret_cast<rdxUInt8*>(m_objectDataPtr) + this->serializationTagOffset);
}

#endif
