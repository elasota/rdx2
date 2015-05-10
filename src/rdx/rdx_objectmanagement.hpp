/*
 * Copyright (C) 2011-2013 Eric Lasota
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __RDX_OBJECTMANAGEMENT_HPP__
#define __RDX_OBJECTMANAGEMENT_HPP__

#include <string.h>

#include "rdx_coretypes.hpp"
#include "rdx_reftypealiases.hpp"
#include "rdx_atomic.hpp"
#include "rdx_lutdecls.hpp"
#include "rdx_api.hpp"
//#include "rdx_processing.hpp"
//#include "rdx_basictypes.hpp"

// Load staging:
// A type can be laid out if:
//  - The parent class, if any, is laid out
//  - All properties of struct/enum type are laid out
// Step 1: Symbols of core types (ones with GCOF_Core) are loaded.
// Step 2: Default instances are loaded

// Concurrency stuff:
// - RDX only follows OS concurrency rules.  It never reorders reads or writes, but does not stop CPU reordering.
//   Host-controlled threads are possible with ExecuteTimeSlice
//
//   It is possible to isolate objects using domains.  Objects can only access global symbols in compatible domains
//   and the host controls domain visibility.  The runtime does not enforce that objects only reference symbols
//   in properly-visible domains, but violations may cause problems with serialization.
//
// Global operations execute when only one thread (the one that caused the global operation) is the only active one.
// If another thread attempts to initiate a global operation, it will block until the global operation completes.
// Global operations are always safe and must never attempt to block.
//
// Lock process:
// - Acquire global sync status check mutex (blocks thread activation)
//   Check the synchronization flags to see if the operation can be run (including global sync, even if global sync isn't needed to run)
//   If a flag is in use:
//      If blocked globally, decrement active thread counter
//      Release check mutex
//          If blocked globally:
//              Acquire global sync mutex
//              Increment active thread counter
//              Release global sync mutex
//      Repeat lock attempt
//   If all are available:
//      Mark desired flags as unavailable
//      Acquire applicable mutex
//      Release check mutex
//      If a global operation, spin until the active thread counter hits 1
//      Perform operations
//      Release applicable mutex
//
// Global operations:
// - Serialization writes
// - Emergency garbage collection
//
// Mutually exclusive operations (non-global):
// - Allocation operations
// - String lookups
// - GST operations and serialization reads
// - GC list manipulation
//
// - The mutex process is as follows:
//   Check volatile global sync flag
//   - If enabled:
//       Disable
//
// - IncRef/DecRef cause interlocks
//
// - Graph interactions with reference counter:
//   If an object is created, it is added to the marked set AND the live set.
//   If an object's reference count hits zero, then a free is attempted.
//      If the GC is active, then the object is removed from its scan list.
//      Regardless of action, the object is freed immediately.
//   If an assignment is made to a traced reference, and the object being assigned is not marked, a locking GC add call is made.
//
// - Threads during graphs:
//   Visitor will acquire a state lock on the thread.  If the thread is active, it
//   will set a scan-on-exit flag on the thread.  If it is inactive, it will
//   scan the thread before releasing the lock on it.

struct rdxSOperationContext;
	
template<class Tvalue> class rdxCStaticLookupPODKey;
template<class Tkey, class TlookupType> class rdxCStaticLookupTable;
template<class T> struct rdxSAutoTypeInfo;
template<class T> class rdxCArray;

class rdxCType;
class rdxCString;
struct rdxSEnumerant;
class rdxCArrayOfType;
class rdxCDelegateType;
struct rdxSProperty;
struct rdxICodeProvider;
class rdxCStructuredType;
class rdxCRuntimeThread;
class rdxCObjectManager;
class rdxCObject;
class rdxCArrayContainer;

struct rdxIObjectReferenceVisitor;
struct rdxIFileStream;
struct rdxITextDeserializer;
struct rdxIMutex;
struct rdxIfcTypeInfo;
struct rdxSCharSpan;

struct rdxIObjectManager;
struct rdxISerializer;
struct rdxITypeProcessor;
class rdxCPackage;
struct rdxSPackageManifestLocal;
struct rdxGCInfo;
struct rdxITypeSerializer;
struct rdxINativeTypeHost;
struct rdxSAllocator;
struct rdxSDomainGUID;
struct rdxSObjectGUID;

struct rdxSBuiltIns;
class rdxCPackage;

struct rdxSPCCMDomainIndex;
struct rdxSPluginExport;


enum rdxEDomainStatePreservation
{
	rdxDSP_NeverCollect,	// Package will never be collected
	rdxDSP_Save,			// Package contents will be written to disk if collected
	rdxDSP_Volatile,		// Don't care
};

// Probably the most confusing thing in the RDX code is the handling of unloaded objects.
//
// Shells can appear in only 2 places:
// - Within the data space of objects that have not yet been loaded.
// - In the GST, and only while the SymbolTableAccess mutex is locked.

// Unloaded objects may contain references to non-objects that, when .GCInfo() is called, returns an
// rdxObjInfoRef that doesn't point to a rdxGCInfo, but rather, to an rdxSLoadShell.  These references are
// still traceable by the GC and are used to allow permanent pinning in the non-moving implementation.

// If memory is immovable, then GC info is stored before the object and .ObjectInfo returns a pointer to a
// fixed offset before the reference.  Shelled references point to non-data addresses that, when .ObjectInfo
// is invoked on them, resolves to the shell.

// If memory is movable, then GC info is stored in object tables and .ObjectInfo returns a pointer retrieved
// from memory just before the actual object.  Shelled references point to the load shell itself, since it
// will contain a GC info ref that points to itself before it.


struct rdxSObjectSerializationInfo
{
	rdxLargeUInt serializeCatalogOffset;
	rdxLargeUInt serializeDataOffset;
};

struct rdxIPackageHost
{
	virtual rdxIFileStream *StreamForDomain(rdxIObjectManager *om, rdxSDomainGUID domain, bool write, bool &isText) = 0;
	virtual bool DomainsVisible(rdxSDomainGUID sourceDomain, rdxSDomainGUID destDomain) = 0;
	virtual bool DomainCanContainMethods(rdxSDomainGUID domain) = 0;
};

enum rdxEAllocUsage
{
	rdxALLOC_KeepExistingUsage,
	rdxALLOC_Object,
	rdxALLOC_ExecutableInstructions,
	rdxALLOC_InstructionArgs,
	rdxALLOC_ThreadStack,
	rdxALLOC_LongTerm,
	rdxALLOC_ShortTerm,
};

struct rdxSAllocator
{
	void *opaque;
	void *(*reallocFunc) (void *opaque, void *ptr, rdxLargeUInt sz, rdxLargeUInt align, rdxEAllocUsage allocUsage);
	void (*auditHeapFunc)();

	inline void *Alloc(rdxLargeUInt size, rdxLargeUInt align, rdxEAllocUsage allocUsage) const { return this->reallocFunc(this->opaque, NULL, size, align, allocUsage); }

	inline void *Realloc(void *ptr, rdxLargeUInt size, rdxLargeUInt align) const { return this->reallocFunc(this->opaque, ptr, size, align, rdxALLOC_KeepExistingUsage); }

	template<class T>
	inline T* Realloc(T* ptr, rdxLargeUInt count) const { return reinterpret_cast<T*>(this->reallocFunc(this->opaque, ptr, count * static_cast<rdxLargeUInt>(sizeof(T)), rdxAlignOf(T), rdxALLOC_KeepExistingUsage)); }

	template<class T>
	inline T* CAlloc(rdxLargeUInt count, rdxEAllocUsage usage) const { return reinterpret_cast<T*>(this->reallocFunc(this->opaque, NULL, count * static_cast<rdxLargeInt>(sizeof(T)), rdxAlignOf(T), usage)); }

	inline void Free(void *ptr) const { this->reallocFunc(this->opaque, ptr, 0, 1, rdxALLOC_KeepExistingUsage); }
};

struct rdxIObjectManager
{
	enum ManagerOperationContext
	{
		MOC_Deserialize,
		MOC_Initialize,
	};

	virtual void *GetLocalResourcesMap() = 0;
	virtual void *GetImportedResourcesMap() = 0;
	virtual void *GetArrayDefMap() = 0;

	virtual void Shutdown() = 0;

	virtual rdxCRef(rdxCString) DeserializeBinaryString(rdxSOperationContext *ctx, rdxIFileStream *reader, rdxWeakHdl(rdxCPackage) pkg) = 0;
	virtual rdxCRef(rdxCString) DeserializeTextString(rdxSOperationContext *ctx, rdxITextDeserializer *td) = 0;
	virtual void SerializeBinaryString(rdxWeakRTRef(rdxCString) str, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable) const = 0;
	virtual void SerializeTextString(rdxWeakRTRef(rdxCString) str, rdxIFileStream *fs) const = 0;

	virtual void IncrementThreadCounter() = 0;
	virtual void DecrementThreadCounter() = 0;
	virtual rdxIMutex *GetCoreMutex(int mNum) = 0;

	virtual const rdxINativeTypeHost *GetTypeHost() const = 0;
	virtual const rdxICodeProvider *GetCodeProvider() const = 0;
	virtual rdxSAllocator *GetAllocator() = 0;

	virtual void CollectGarbage(rdxSOperationContext *ctx) = 0;
	virtual void ExternalGCMarkObject(rdxGCInfo *gcObject) = 0;
	virtual void SetILDisposal(bool shouldDispose) = 0;
	virtual void SetCollectionFrequency(rdxAtomicUInt::ContainedType frequency) = 0;
	//virtual void StartSerialize(rdxISerializer *ser) = 0;
	//virtual void SerializeIncludeObject(rdxISerializer *ser, rdxWeakTypelessRTRef obj, rdxEGCLink gcl) = 0;
	virtual void GraphIncludeObject(rdxISerializer *ser, rdxWeakRTRef(rdxCObject) obj) = 0;
	virtual rdxCRef(rdxCObject) CreateObjectContainer(rdxSOperationContext *ctx, rdxLargeUInt containerSize, rdxSDomainGUID domain, rdxWeakHdl(rdxCType) t, rdxIfcTypeInfo tp) = 0;
	virtual rdxCRef(rdxCString) CreateString(rdxSOperationContext *ctx, const rdxChar *str, bool numCharsSpecified = false, rdxLargeUInt numChars = 0, bool lookupOnly = false) = 0;
	virtual rdxCRef(rdxCString) CreateString(rdxSOperationContext *ctx, rdxWeakOffsetHdl(rdxChar) str, bool numCharsSpecified = false, rdxLargeUInt numChars = 0, bool lookupOnly = false) = 0;
	virtual rdxCRef(rdxCString) CreateStringASCII(rdxSOperationContext *ctx, const char *str, bool sizeSpecified = false, rdxLargeUInt size = 0, bool lookupOnly = false) = 0;
	virtual rdxCRef(rdxCString) CreateStringUTF8(rdxSOperationContext *ctx, const rdxByte *str, bool sizeSpecified = false, rdxLargeUInt size = 0, bool lookupOnly = false) = 0;
	virtual rdxCRef(rdxCString) CreateStringUTF8(rdxSOperationContext *ctx, rdxWeakOffsetHdl(rdxByte) str, bool sizeSpecified = false, rdxLargeUInt size = 0, bool lookupOnly = false) = 0;
	virtual rdxCRef(rdxCString) CreateStringConcatenated(rdxSOperationContext *ctx, rdxWeakHdl(rdxCString) str1, rdxWeakHdl(rdxCString) str2) = 0;
	virtual rdxCRef(rdxCString) CreateStringSub(rdxSOperationContext *ctx, rdxWeakHdl(rdxCString) str, rdxLargeUInt startIndex, bool lengthSpecified = false, rdxLargeUInt length = 0) = 0;
	virtual rdxCRef(rdxCObject) CreateInitialObject(rdxSOperationContext *ctx, rdxWeakHdl(rdxCType) objectType, rdxLargeUInt numElements, const rdxSObjectGUID& objectGUID) = 0;
	virtual void InitializeObject(rdxWeakRTRef(rdxCObject) obj, rdxLargeUInt overflow, bool forceZeroFill) = 0;

	virtual rdxCRef(rdxCRuntimeThread) CreateThread(rdxSOperationContext *ctx, rdxLargeUInt stackSize) = 0;

	virtual rdxCRef(rdxCObject) LookupSymbolSimple(rdxSOperationContext *ctx, rdxSObjectGUID symbolName) = 0;		// Never fails, never excepts
	virtual rdxCRef(rdxCObject) LookupSymbol(rdxSOperationContext *ctx, rdxSObjectGUID symbolName, rdxIPackageHost *packageHost) = 0;

	virtual rdxCRef(rdxCArrayOfType) CreateArrayType(rdxSOperationContext *ctx, rdxWeakHdl(rdxCType) t, rdxLargeUInt numDimensions, bool constant, rdxIfcTypeInfo typeInfo) = 0;

	virtual void AddGlobalSymbol(rdxSOperationContext *ctx, rdxSObjectGUID guid, rdxWeakHdl(rdxCObject) object) = 0;
	virtual rdxSDomainGUID ComputeDomainGUID(const char *domainName) = 0;
	virtual rdxSDomainGUID ComputeDomainGUID(rdxWeakRTRef(rdxCString) domainName) = 0;
	virtual rdxSDomainGUID ComputeDomainGUID(const rdxSCharSpan &charSpan) = 0;
	virtual rdxSObjectGUID ComputeObjectGUID(rdxSDomainGUID domain, const char *objectName) = 0;
	virtual rdxSObjectGUID ComputeObjectGUID(rdxSDomainGUID domain, rdxWeakRTRef(rdxCString) objectName) = 0;
	virtual rdxSObjectGUID ComputeObjectGUID(rdxSDomainGUID domain, const rdxSCharSpan &charSpan) = 0;
	virtual void AddUnloadedObject(rdxSOperationContext *ctx, rdxWeakHdl(rdxCObject) object) = 0;

	virtual rdxSBuiltIns *GetBuiltIns() = 0;
	virtual const rdxSBuiltIns *GetBuiltIns() const = 0;

	virtual rdxCRef(rdxCPackage) LoadPackage(rdxSOperationContext *ctx, rdxSDomainGUID domain, rdxIPackageHost *packageHost) = 0;
	virtual rdxCRef(rdxCObject) LoadObject(rdxSOperationContext *ctx, rdxIPackageHost *packageHost) = 0;
	virtual void SavePackage(rdxSOperationContext *ctx, rdxSDomainGUID domain, rdxIPackageHost *packageHost) = 0;
	virtual void SaveObject(rdxSOperationContext *ctx, rdxWeakHdl(rdxCObject) object, rdxIFileStream *fs, bool isText) = 0;
	virtual void RegisterPackage(rdxSOperationContext *ctx, rdxSDomainGUID domain, rdxWeakHdl(rdxCPackage) pkg) = 0;
	virtual bool PackageLoaded(rdxSDomainGUID domain) = 0;

	virtual bool ObjectCompatible(rdxWeakRTRef(rdxCObject) from, rdxWeakRTRef(rdxCType) to) const = 0;
	virtual bool EnumCompatible(rdxEnumValue ev, rdxWeakArrayRTRef(rdxSEnumerant) enums) const = 0;
	virtual rdxWeakIRef(rdxSObjectInterfaceImplementation) FindInterface(rdxWeakRTRef(rdxCObject) obj, rdxWeakRTRef(rdxCStructuredType) interfaceType) const = 0;
	virtual bool TypesCompatible(rdxWeakRTRef(rdxCType) from, rdxWeakRTRef(rdxCType) to) const = 0;
	virtual bool TypesCompatiblePolymorphic(rdxWeakRTRef(rdxCType) from, rdxWeakRTRef(rdxCType) to) const = 0;
	virtual bool TypeIsObjectReference(rdxWeakRTRef(rdxCType) t) const = 0;
	virtual bool TypeIsArray(rdxWeakRTRef(rdxCType) t) const = 0;
	virtual bool TypeIsInterface(rdxWeakRTRef(rdxCType) t) const = 0;
	virtual bool TypeIsClass(rdxWeakRTRef(rdxCType) t) const = 0;
	virtual bool TypeCanBeTraced(rdxWeakRTRef(rdxCType) t) const = 0;
	virtual bool TypeIsValid(rdxWeakRTRef(rdxCType) t) const = 0;
	virtual bool ObjectIsConstant(rdxWeakRTRef(rdxCObject) v) const = 0;
	virtual void TypeValueSize(rdxSOperationContext *ctx, rdxWeakHdl(rdxCType) t, rdxLargeUInt &size, rdxLargeUInt &align) const = 0;
	virtual rdxLargeUInt TypeElementSize(rdxWeakRTRef(rdxCType) t) const = 0;

	virtual void AuditSentinels() = 0;

	virtual rdxCRef(rdxCObject) FirstLiveObject() const = 0;
	virtual rdxCRef(rdxCObject) NextLiveObject(rdxWeakRTRef(rdxCObject) gci) const = 0;

	virtual void VisitStructureReferences(void *base, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable, rdxIfcTypeInfo typeInfo, rdxWeakRTRef(rdxCStructuredType) st, rdxLargeUInt stride, rdxLargeUInt count) = 0;

	virtual rdxIfcTypeInfo GetBasicValueArrayTypeInfo() const = 0;
	virtual rdxIfcTypeInfo GetBasicReferenceArrayTypeInfo() const = 0;
	virtual rdxIfcTypeInfo GetBasicInterfaceArrayTypeInfo() const = 0;
	virtual rdxIfcTypeInfo GetBasicBoxTypeInfo() const = 0;
	virtual rdxIfcTypeInfo GetBasicReferenceTypeInfo() const = 0;
	virtual rdxIfcTypeInfo GetBasicInterfaceTypeInfo() const = 0;
	//virtual void DebugDumpGST() = 0;

	rdxCRef(rdxCArrayContainer) CreateArrayContainer(rdxSOperationContext *ctx, rdxLargeUInt elementSize, rdxLargeUInt numElements, rdxLargeUInt numDimensions, rdxSDomainGUID domain, rdxLargeUInt overflow, rdxWeakHdl(rdxCArrayOfType) t, rdxIfcTypeInfo typeInfo);
};

class rdxCInternalObjectFactory
{
public:
	template<class T>
	static typename rdxCRef(T) CreateObject(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCStructuredType) t, rdxSDomainGUID domain);
	template<class T>
	static typename rdxCRef(T) CreateObject(rdxSOperationContext *ctx, rdxIObjectManager *objm);
	template<class T>
	static typename rdxArrayCRef(T) Create1DArray(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxLargeUInt count, rdxWeakHdl(rdxCArrayOfType) t, rdxSDomainGUID domain, rdxLargeUInt overflow, rdxLargeUInt stride, rdxIfcTypeInfo contentsTypeInfo);
	template<class T>
	static typename rdxArrayCRef(T) Create1DArray(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxLargeUInt count, rdxWeakHdl(rdxCArrayOfType) t, rdxSDomainGUID domain);
	template<class T>
	static typename rdxArrayCRef(T) Create1DArray(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxLargeUInt count);
	template<class T>
	static rdxCRef(rdxCArrayOfType) AutoCreateArrayType(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCType) t, rdxLargeUInt numDimensions, bool constant);
};

RDX_DYNLIB_API void rdxXAPI_Create1DArrayCommon(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxLargeUInt count, rdxWeakHdl(rdxCArrayOfType) t, rdxSDomainGUID domain, rdxLargeUInt overflow, rdxLargeUInt stride, rdxIfcTypeInfo contentsTypeInfo, rdxIfcTypeInfo arrayTypeInfo, rdxCRef(rdxCArrayContainer) *result);
RDX_DYNLIB_API void rdxXAPI_Create1DArrayIntuitive(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxLargeUInt count, rdxWeakHdl(rdxCArrayOfType) t, rdxSDomainGUID domain, rdxLargeUInt stride, rdxCRef(rdxCArrayContainer) *result);

class rdxCExternalObjectFactory
{
private:
	static rdxIfcTypeInfo ResolveArrayForContents(rdxIObjectManager *objm, const void *p);
	static rdxIfcTypeInfo ResolveArrayForContents(rdxIObjectManager *objm, const rdxTracedRTRef(rdxCObject) *p);

	// This isn't valid for externals (a type is required to provide type info)
	template<class T>
	static typename rdxArrayCRef(T) Create1DArray(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxLargeUInt count);

public:
	template<class T>
	static typename rdxCRef(T) CreateObject(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCStructuredType) t, rdxSDomainGUID domain);
	template<class T>
	static typename rdxCRef(T) CreateObject(rdxSOperationContext *ctx, rdxIObjectManager *objm);
	template<class T>
	static typename rdxArrayCRef(T) Create1DArray(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxLargeUInt count, rdxWeakHdl(rdxCArrayOfType) t, rdxSDomainGUID domain, rdxLargeUInt overflow, rdxLargeUInt stride, rdxIfcTypeInfo contentsTypeInfo);
	template<class T>
	static typename rdxArrayCRef(T) Create1DArray(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxLargeUInt count, rdxWeakHdl(rdxCArrayOfType) t, rdxSDomainGUID domain);
	template<class TContents>
	static rdxCRef(rdxCArrayOfType) AutoCreateArrayType(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCType) t, rdxLargeUInt numDimensions, bool constant);
};

rdxIObjectManager *rdxCreateObjectManager(rdxSAllocator alloc, const rdxINativeTypeHost *nth, const rdxICodeProvider *cp);

rdxCRef(rdxCPackage) rdxDeserializePackage(rdxSOperationContext *ctx, rdxIObjectManager *objManager, rdxSDomainGUID domain,
	rdxIPackageHost *host, rdxIFileStream *stream, bool isText);
bool rdxDeserializeObject(rdxSOperationContext *ctx, rdxCObjectManager *objManager, rdxWeakHdl(rdxCPackage) pkg, rdxWeakHdl(rdxCObject) targetContainer, rdxWeakOffsetHdl(rdxSPackageManifestLocal) mfl,
	rdxSDomainGUID domain, rdxIPackageHost *host, rdxIFileStream *stream, bool isText);

void rdxWriteHeader(rdxIObjectManager *objm, rdxIFileStream *fs, bool isText, rdxLargeUInt numLocalSymbols, rdxLargeUInt numImports, rdxLargeUInt numStrings, rdxLargeUInt numArrayDefs);
void rdxWriteImport(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxIFileStream *fs, bool isText, const rdxCSSIDTable *ssidTable, rdxSObjectGUID symbolName);
void rdxReserveObjectExport(rdxIObjectManager *objm, rdxIFileStream *fs, bool isText, const rdxCSSIDTable *ssidTable, rdxWeakHdl(rdxCObject) obj);
void rdxWriteObjectExport(rdxIObjectManager *objm, rdxIFileStream *fs, bool isText, rdxLargeUInt *catalogOffset, rdxLargeUInt objectOffset, rdxWeakRTRef(rdxCObject) obj);
void rdxWriteObject(rdxIObjectManager *objm, rdxIFileStream *fs, bool isText, const rdxCSSIDTable *ssidTable, rdxWeakRTRef(rdxCObject) obj);
void rdxWriteValue(rdxIObjectManager *objm, rdxIFileStream *fs, bool isText, const rdxCSSIDTable *ssidTable, rdxWeakHdl(rdxCType) t, rdxWeakTypelessOffsetRTRef obj);

void rdxDeserializeValue(rdxSOperationContext *ctx, rdxIObjectManager *objManager, rdxWeakHdl(rdxCPackage) pkg,
	rdxSDomainGUID domain, rdxIPackageHost *host, rdxIFileStream *fs, bool isText, rdxWeakHdl(rdxCType) t, rdxWeakTypelessOffsetHdl propertyLocation, int depth);

#include "rdx_longflow.hpp"
#include "rdx_programmability.hpp"
#include "rdx_threading.hpp"
#include "rdx_objectguid.hpp"
#include "rdx_errorcodes.hpp"

inline rdxGCInfo::rdxGCInfo()
{
}

inline void rdxGCInfo::Unlink(int gcl)
{
	if(prev[gcl] != NULL)
		prev[gcl]->next[gcl] = next[gcl];
	if(next[gcl] != NULL)
		next[gcl]->prev[gcl] = prev[gcl];
	next[gcl] = prev[gcl] = NULL;
}

inline void rdxGCInfo::LinkBefore(rdxGCInfo *nnext, int gcl)
{
	if(nnext != NULL)
	{
		if(nnext->prev[gcl] != NULL)
			prev[gcl] = nnext->prev[gcl];
		nnext->prev[gcl]->next[gcl] = this;
		nnext->prev[gcl] = this;
		next[gcl] = nnext;
	}
	else
		next[gcl] = NULL;
}

inline void rdxGCInfo::LinkAfter(rdxGCInfo *nprev, int gcl)
{
	if(nprev != NULL)
	{
		if(nprev->next[gcl] != NULL)
			next[gcl] = nprev->next[gcl];
		nprev->next[gcl]->prev[gcl] = this;
		nprev->next[gcl] = this;
		prev[gcl] = nprev;
	}
	else
		prev[gcl] = NULL;
}

inline rdxGCInfo::rdxGCInfo(rdxIObjectManager *ownerObjectManager, rdxWeakHdl(rdxCType) containerType, rdxIfcTypeInfo typeInfo, bool hasSerializationTag, rdxLargeUInt serTagOffset, rdxCObject *objectDataRoot)
{
	static rdxLargeUInt globalCounter = 0;

	this->debugCounter = globalCounter++;
	this->containerType = containerType;
	this->typeInfo = typeInfo;
	this->ownerObjectManager = ownerObjectManager;
	this->objectFlags = 0;
	this->numExternalReferences.WriteUnfenced(0);
	this->numTracedReferences.WriteUnfenced(0);
#ifdef RDX_ENABLE_DEBUG_GST_SYMBOL
	this->debugGSTSymbol = rdxHandle<rdxChar>::Null();
#endif
	this->pInspector = &this->selfInspector;

	this->m_objectDataPtr = objectDataRoot;
	
	if(hasSerializationTag)
	{
		this->objectFlags |= rdxGCInfo::GCOF_HasSerializationTag;
		this->serializationTagOffset = serTagOffset;
		this->serializationTag = reinterpret_cast<rdxSSerializationTag*>(reinterpret_cast<rdxUInt8*>(objectDataRoot) + serTagOffset);
	}
	else
	{
		this->serializationTagOffset = 0;
		this->serializationTag = RDX_CNULL;
	}
}

inline rdxCRef(rdxCArrayContainer) rdxIObjectManager::CreateArrayContainer(rdxSOperationContext *ctx, rdxLargeUInt elementSize, rdxLargeUInt numElements, rdxLargeUInt numDimensions, rdxSDomainGUID domain, rdxLargeUInt overflow, rdxWeakHdl(rdxCArrayOfType) t, rdxIfcTypeInfo typeInfo)
{
	RDX_TRY(ctx)
	{
		if(!rdxCheckAddOverflowU(numElements, overflow))
			RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);
		rdxLargeUInt containerSize;
		RDX_PROTECT(ctx, rdxCArrayContainer::ComputeContainerSize(ctx, elementSize, numElements + overflow, numDimensions, RDX_CNULL, &containerSize));
		rdxCRef(rdxCArrayContainer) arrayCont;
		RDX_PROTECT_ASSIGN(ctx, arrayCont, CreateObjectContainer(ctx, containerSize, domain, t, typeInfo).StaticCast<rdxCArrayContainer>());
		return arrayCont;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROWV(ctx, rdxCRef(rdxCArrayContainer)::Null());
	}
	RDX_ENDTRY
}

#include "rdx_objectmanagement_code.hpp"

#endif
