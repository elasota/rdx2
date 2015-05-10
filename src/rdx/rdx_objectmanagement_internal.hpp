#ifndef __RDX_OBJECTMANAGEMENT_INTERNAL_HPP__
#define __RDX_OBJECTMANAGEMENT_INTERNAL_HPP__

#include "rdx_objectmanagement.hpp"
#include "rdx_hashmap.hpp"
#include "rdx_arraydefprototype.hpp"
#include "rdx_builtins.hpp"
#include "rdx_reftypedefs.hpp"

struct rdxSPackageManifestLocal;
struct rdxSLoadShell;

struct rdxSCandidateString
{
	enum CSFormat
	{
		CSF_ASCII,
		CSF_UTF8,
		CSF_Char,
		CSF_Concatenated,
	};

	enum CSInputSource
	{
		CSIS_Handle,
		CSIS_Pointer,
	};

	CSFormat format;
	CSInputSource source;
	rdxWeakTypelessOffsetHdl input1hdl;
	const void *input1ptr;
	rdxLargeUInt numRawBytes1;

	rdxWeakTypelessOffsetHdl input2hdl;
	const void *input2ptr;
	rdxLargeUInt numRawBytes2;

	rdxLargeUInt numCharacters;

	rdxArrayCRef(rdxChar) feedback;

	// Decodes a single character from a byte stream
	rdxChar DecodeCharacter(const void **pData, rdxLargeUInt *pAvailableBytes) const;
	// Counts characters and populates numCharacters, returns true if the string is valid, false otherwise
	bool CountCharacters();

	const void *GetInitialData() const;
	const void *GetSecondaryData() const;
};

struct rdxSAOTKey
{
	rdxWeakHdl(rdxCType) type;
	rdxLargeUInt numDimensions;
	bool isConstant;

	inline void Set(rdxWeakHdl(rdxCType) type, rdxLargeUInt numDimensions, rdxBool isConstant)
	{
		memset(this, 0, sizeof(*this));
		this->type = type;
		this->numDimensions = numDimensions;
		this->isConstant = (isConstant != rdxFalseValue);
	}
	inline bool operator ==(const rdxSAOTKey &rs) const
	{
		return (type == rs.type) && (numDimensions == rs.numDimensions) && (isConstant == rs.isConstant);
	}
	inline bool operator !=(const rdxSAOTKey &rs) const
	{
		return !(*this == rs);
	}

	inline rdxHashValue Hash() const
	{
		return rdxHashBytes(&isConstant, sizeof(isConstant)) +
				rdxHashBytes(&type, sizeof(type)) +
				rdxHashBytes(&numDimensions, sizeof(numDimensions));
	}
};

// Hash table that maps character arrays to strings
class rdxCHashMapStringTable : public rdxCHashMapSimple<rdxWeakHdl(rdxCArray<rdxChar>), rdxWeakHdl(rdxCString)>
{
public:
	typedef rdxSCandidateString CandidateKey;
	typedef rdxWeakHdl(rdxCString) Value;

	typedef rdxSHashEntry<rdxWeakArrayHdl(rdxChar), rdxWeakHdl(rdxCString)> MyHashEntry;

	// Reference to the char[C] type
	rdxCRef(rdxCArrayOfType) m_aot_ConstChar;

	void InitializeElement(MyHashEntry *v);
	rdxLargeUInt UnitSize() const;
	bool CompareKey(const rdxSCandidateString *cs, const MyHashEntry *e) const;
	rdxHashValue HashCKey(const void *ckey) const;

	rdxWeakArrayHdl(rdxChar) ConstructKey(rdxSOperationContext *ctx, const void *ckey, bool &recheck);
};


class rdxCObjectManager : public rdxIObjectManager
{
public:
	struct GCObjectList
	{
		rdxArrayCRef(rdxTracedRTRef(rdxCObject)) m_objects;
		rdxLargeUInt m_numObjects;

		inline void Add(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCObject) obj, rdxWeakHdl(rdxCArrayOfType) aotRefs)
		{
			RDX_TRY(ctx)
			{
				rdxLargeUInt capacity = 0;
				if(m_objects.IsNotNull())
					capacity = m_objects->NumElements();

				if(m_objects.IsNull())
					m_numObjects = 0;		// Shut up analyze

				if(m_numObjects == capacity)
				{
					// Enlarge
					rdxArrayCRef(rdxTracedRTRef(rdxCObject)) newObjects;
					if(!rdxCheckMulOverflowU(m_numObjects, 2))
						RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);

					rdxLargeUInt newCapacity = m_numObjects * 2;
					if(newCapacity == 0)
						newCapacity = 8;
					RDX_PROTECT_ASSIGN(ctx, newObjects, rdxCInternalObjectFactory::Create1DArray<rdxTracedRTRef(rdxCObject)>(ctx, objm, newCapacity, aotRefs, rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime)));
					for(rdxLargeUInt i=0;i<m_numObjects;i++)
						newObjects->Element(i) = m_objects->Element(i);
					m_objects = newObjects;
				}
				m_objects->Element(m_numObjects++) = obj.ToWeakRTRef();
			}
			RDX_CATCH(ctx)
			{
				RDX_RETHROW(ctx);
			}
			RDX_ENDTRY
		}

		inline void EvictAll()
		{
			m_objects = rdxArrayCRef(rdxTracedRTRef(rdxCObject))::Null();
			m_numObjects = 0;
		}
	};

	enum ESaveMode
	{
		SAVEMODE_SingleObject,
		SAVEMODE_Package,
	};

	enum GCChain
	{
		GCCHAIN_Unscanned,
		GCCHAIN_Thread,
		GCCHAIN_FirstScan,
		GCCHAIN_ScanDuringCleanup,
		GCCHAIN_NeedsSecondScan,

		GCCHAIN_Count,
	};

	typedef rdxCHashMap<rdxCHashMapSimple<rdxSObjectGUID, rdxSLoadShell> > ResourceHashMap;
	typedef rdxCHashMap<rdxCHashMapSimple<rdxSObjectGUID, rdxSArrayDefPrototype> > ArrayDefHashMap;

private:
	rdxSBuiltIns m_builtins;
	rdxSAllocator m_allocator;
	const rdxINativeTypeHost *m_nth;
	const rdxICodeProvider *m_codeProvider;

	rdxCHashMap<rdxCHashMapStringTable> m_stringTable;
	rdxCHashMap<rdxCHashMapSimple<rdxSAOTKey, rdxWeakHdl(rdxCArrayOfType)> > m_aotTable;

	static const rdxLargeUInt STRING_ENCODE_BACKLOG_SIZE	= 50;

	ResourceHashMap m_parseLocalResources;
	ResourceHashMap m_parseImportedResources;
	ArrayDefHashMap m_arrayDefHashMap;
	rdxCHashMap<rdxCHashMapSimple<rdxSObjectGUID, rdxWeakHdl(rdxCObject)> > m_gst;
	rdxCHashMap<rdxCHashMapSimple<rdxSDomainGUID, rdxWeakHdl(rdxCPackage)> > m_packages;

	rdxGCInfo m_scannedRoot;
	rdxGCInfo m_markedRoot;
	rdxGCInfo m_unmarkedRoot;

	rdxGCInfo m_liveRoot;

	GCObjectList m_unloadedPackages;
	GCObjectList m_unprocessedObjects;
	bool m_initialized;

	rdxLargeUInt m_gcFrequency;
	rdxAtomicUInt m_gcCounter;
	bool m_shouldDisposeIL;
	
	bool m_singleThreadMode;

#ifdef RDX_ENABLE_SMP
	volatile bool m_operationsActive[rdxCOREMUTEX_Count];
	rdxIMutex *m_coreMutexes[rdxCOREMUTEX_Count];
	bool m_ignoreMutexesDuringSyncs;
#endif
	bool m_forcePackagesRelevant;

	void InitializeSTAOT(rdxSOperationContext *ctx, rdxCRef(rdxCArrayOfType) aotRef, const char *name, rdxCRef(rdxCStructuredType) subtypeRef);
	void IncludeGCList(rdxISerializer &gc, GCObjectList *gcol);

public:
	rdxCObjectManager(rdxSAllocator alloc, const rdxINativeTypeHost *nth, const rdxICodeProvider *cp);
	virtual ~rdxCObjectManager();

	// Serializer implementation that does nothing but propagate marks, used for GC
	class GarbageCollectionSerializer : public rdxISerializer
	{
	private:
		bool m_isActive;
		bool m_permitFlexibleLayouts;

	public:
		GarbageCollectionSerializer();
		
		virtual bool TryIncludeObject(rdxWeakRTRef(rdxCObject) obj) RDX_OVERRIDE;
		virtual void SerializeReference(rdxWeakRTRef(rdxCObject) obj) RDX_OVERRIDE;
		virtual void SerializeBulk(rdxWeakTypelessOffsetRTRef data, rdxLargeUInt sz) RDX_OVERRIDE;
		virtual void SerializeData(rdxWeakRTRef(rdxCType) type, rdxWeakTypelessOffsetRTRef data) RDX_OVERRIDE;

		void SetActive();
		bool IsActive();

		void SetPermitFlexibleLayouts(bool permit);
	};
	GarbageCollectionSerializer m_garbageCollector;

	class FileWriteSerializer : public rdxISerializer
	{
	private:
		rdxSDomainGUID m_targetDomain;
		rdxIObjectManager *m_objm;

	public:
		FileWriteSerializer(rdxSDomainGUID targetDomain, rdxIObjectManager *objm);
		
		virtual bool TryIncludeObject(rdxWeakRTRef(rdxCObject) obj) RDX_OVERRIDE;
		virtual void SerializeReference(rdxWeakRTRef(rdxCObject) obj) RDX_OVERRIDE;
		virtual void SerializeBulk(rdxWeakTypelessOffsetRTRef data, rdxLargeUInt sz) RDX_OVERRIDE;
		virtual void SerializeData(rdxWeakRTRef(rdxCType) type, rdxWeakTypelessOffsetRTRef data) RDX_OVERRIDE;

		void SetActive();		// Use with ObjectList lock
		bool IsActive() const;	// Use with ObjectList lock
	};

	void InitializeBuiltInTypes(rdxSOperationContext *ctx);
	void InitializeCoreMutexes(rdxSOperationContext *ctx);

	static void ComputeGUID(const char *name, rdxUInt8 *output);
	static void ComputeGUID(const rdxSCharSpan &charSpan, rdxUInt8 *output);
	static void ComputeGUID(rdxWeakRTRef(rdxCString) domainName, rdxUInt8 *output);

private:
	void MarkGraphObject(rdxWeakRTRef(rdxCObject) obj, rdxISerializer *ser);	//  Use with ObjectList lock

	void VisitObjectReferences(rdxWeakRTRef(rdxCObject) obj, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable);
	void FinishSerialize(rdxISerializer *ser, rdxEGCLink gcl, bool markNative);

	bool MethodOverrideCompatible(rdxWeakRTRef(rdxCStructuredType) implementingType, rdxWeakRTRef(rdxCMethod) base, rdxWeakRTRef(rdxCMethod) ov) const;
	void ValidateStructure(rdxSOperationContext *ctx, rdxWeakHdl(rdxCStructuredType) st) const;
	bool ResolveStructure(rdxSOperationContext *ctx, rdxWeakHdl(rdxCStructuredType) st);

public:

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// rdxIObjectManager
	void *GetLocalResourcesMap() RDX_OVERRIDE;
	void *GetImportedResourcesMap() RDX_OVERRIDE;
	void *GetArrayDefMap() RDX_OVERRIDE;

	void Shutdown();

	rdxCRef(rdxCString) DeserializeBinaryString(rdxSOperationContext *ctx, rdxIFileStream *reader, rdxWeakHdl(rdxCPackage) pkg);
	rdxCRef(rdxCString) DeserializeTextString(rdxSOperationContext *ctx, rdxITextDeserializer *td);
	void SerializeBinaryString(rdxWeakRTRef(rdxCString) str, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable) const;
	void SerializeTextString(rdxWeakRTRef(rdxCString) str, rdxIFileStream *fs) const;

	rdxCRef(rdxCObject) FirstLiveObject() const;
	rdxCRef(rdxCObject) NextLiveObject(rdxWeakRTRef(rdxCObject) obj) const;

	void VisitStructureReferences(void *base, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable, rdxIfcTypeInfo typeInfo, rdxWeakRTRef(rdxCStructuredType) st, rdxLargeUInt stride, rdxLargeUInt count);
	
	rdxIfcTypeInfo GetBasicValueArrayTypeInfo() const;
	rdxIfcTypeInfo GetBasicReferenceArrayTypeInfo() const;
	rdxIfcTypeInfo GetBasicInterfaceArrayTypeInfo() const;
	rdxIfcTypeInfo GetBasicBoxTypeInfo() const;
	rdxIfcTypeInfo GetBasicReferenceTypeInfo() const;
	rdxIfcTypeInfo GetBasicInterfaceTypeInfo() const;

	void IncrementThreadCounter();
	void DecrementThreadCounter();
	rdxIMutex *GetCoreMutex(int mNum);

	const rdxINativeTypeHost *GetTypeHost() const;
	const rdxICodeProvider *GetCodeProvider() const;
	rdxSAllocator *GetAllocator();

	void CollectGarbage(rdxSOperationContext *ctx);
	void SetILDisposal(bool shouldDispose);
	void SetCollectionFrequency(rdxLargeUInt frequency);

	void StartGraphing(rdxISerializer *ser, bool isGC);
	void GraphIncludeObject(rdxISerializer *ser, rdxWeakRTRef(rdxCObject) obj) RDX_OVERRIDE;						// Use with ObjectList lock
	void ExternalGCMarkObject(rdxGCInfo *gcObject) RDX_OVERRIDE;											// Avoid ObjectList lock
	bool ContinueGraph(rdxISerializer *ser, bool markNonSerializable, rdxLargeUInt maxNumObjects = 0);		// Use with ObjectList lock
	void DestroyUnmarkedObjects(rdxSOperationContext *ctx);													// Use with ObjectList lock
	void DestroyObject(rdxWeakRTRef(rdxCObject) obj);

	rdxSDomainGUID ComputeDomainGUID(const char *domainName) RDX_OVERRIDE;
	rdxSDomainGUID ComputeDomainGUID(rdxWeakRTRef(rdxCString) domainName) RDX_OVERRIDE;
	rdxSDomainGUID ComputeDomainGUID(const rdxSCharSpan &charSpan) RDX_OVERRIDE;
	rdxSObjectGUID ComputeObjectGUID(rdxSDomainGUID domain, const char *objectName) RDX_OVERRIDE;
	rdxSObjectGUID ComputeObjectGUID(rdxSDomainGUID domain, rdxWeakRTRef(rdxCString) objectName) RDX_OVERRIDE;
	rdxSObjectGUID ComputeObjectGUID(rdxSDomainGUID domain, const rdxSCharSpan &charSpan) RDX_OVERRIDE;

	/*
	void SerializeIncludeObject(rdxISerializer *ser, rdxRef<void> obj, rdxEGCLink gcl);
	void SerializeIncludeStructure(rdxISerializer *ser, rdxRef<rdxCStructuredType> st, const void *obj, rdxEGCLink gcl);
	*/

	rdxCRef(rdxCObject) CreateObjectContainer(rdxSOperationContext *ctx, rdxLargeUInt containerSize, rdxSDomainGUID domain, rdxWeakHdl(rdxCType) t, rdxIfcTypeInfo tp);
	rdxCRef(rdxCString) CreateString(rdxSOperationContext *ctx, const rdxChar *str, bool numCharsSpecified = false, rdxLargeUInt numChars = 0, bool lookupOnly = false) RDX_OVERRIDE;
	rdxCRef(rdxCString) CreateString(rdxSOperationContext *ctx, rdxWeakOffsetHdl(rdxChar) str, bool numCharsSpecified = false, rdxLargeUInt numChars = 0, bool lookupOnly = false) RDX_OVERRIDE;
	rdxCRef(rdxCString) CreateStringASCII(rdxSOperationContext *ctx, const char *str, bool sizeSpecified = false, rdxLargeUInt size = 0, bool lookupOnly = false) RDX_OVERRIDE;
	rdxCRef(rdxCString) CreateStringUTF8(rdxSOperationContext *ctx, const rdxByte *str, bool sizeSpecified = false, rdxLargeUInt size = 0, bool lookupOnly = false) RDX_OVERRIDE;
	rdxCRef(rdxCString) CreateStringUTF8(rdxSOperationContext *ctx, rdxWeakOffsetHdl(rdxByte) str, bool sizeSpecified, rdxLargeUInt size, bool lookupOnly) RDX_OVERRIDE;
	rdxCRef(rdxCString) CreateStringConcatenated(rdxSOperationContext *ctx, rdxWeakHdl(rdxCString) str1, rdxWeakHdl(rdxCString) str2) RDX_OVERRIDE;
	rdxCRef(rdxCString) CreateStringSub(rdxSOperationContext *ctx, rdxWeakHdl(rdxCString) str, rdxLargeUInt startIndex, bool lengthSpecified = false, rdxLargeUInt length = 0) RDX_OVERRIDE;
	rdxCRef(rdxCObject) CreateInitialObject(rdxSOperationContext *ctx, rdxWeakHdl(rdxCType) objectType, rdxLargeUInt numElements, const rdxSObjectGUID& objectGUID) RDX_OVERRIDE;

	rdxCRef(rdxCRuntimeThread) CreateThread(rdxSOperationContext *ctx, rdxLargeUInt stackSize) RDX_OVERRIDE;

	rdxCRef(rdxCObject) LookupSymbolSimple(rdxSOperationContext *ctx, rdxSObjectGUID symbolName) RDX_OVERRIDE;
	rdxCRef(rdxCObject) LookupSymbol(rdxSOperationContext *ctx, rdxSObjectGUID symbolName, rdxIPackageHost *packageHost) RDX_OVERRIDE;

	rdxCRef(rdxCArrayOfType) CreateArrayType(rdxSOperationContext *ctx, rdxWeakHdl(rdxCType) t, rdxLargeUInt numDimensions, bool constant, rdxIfcTypeInfo typeInfo) RDX_OVERRIDE;

	void AddGlobalSymbol(rdxSOperationContext *ctx, rdxSObjectGUID symbolGUID, rdxWeakHdl(rdxCObject) object) RDX_OVERRIDE;
	void AddUnloadedObject(rdxSOperationContext *ctx, rdxWeakHdl(rdxCObject) object);

	rdxSBuiltIns *GetBuiltIns();
	const rdxSBuiltIns *GetBuiltIns() const;

	rdxCRef(rdxCPackage) LoadPackage(rdxSOperationContext *ctx, rdxSDomainGUID domain, rdxIPackageHost *packageHost);
	rdxCRef(rdxCObject) LoadObject(rdxSOperationContext *ctx, rdxIPackageHost *packageHost);
	void SavePackage(rdxSOperationContext *ctx, rdxSDomainGUID domain, rdxIPackageHost *packageHost);
	void SaveObject(rdxSOperationContext *ctx, rdxWeakHdl(rdxCObject) object, rdxIFileStream *fs, bool isText);
	void RegisterPackage(rdxSOperationContext *ctx, rdxSDomainGUID domain, rdxWeakHdl(rdxCPackage) pkg);
	bool PackageLoaded(rdxSDomainGUID domain);

	bool ObjectCompatible(rdxWeakRTRef(rdxCObject) from, rdxWeakRTRef(rdxCType) to) const;
	bool EnumCompatible(rdxEnumValue ev, rdxWeakArrayRTRef(rdxSEnumerant) enums) const;
	rdxWeakIRef(rdxSObjectInterfaceImplementation) FindInterface(rdxWeakRTRef(rdxCObject) obj, rdxWeakRTRef(rdxCStructuredType) interfaceType) const;
	bool TypesCompatible(rdxWeakRTRef(rdxCType) from, rdxWeakRTRef(rdxCType) to) const;
	bool TypesCompatiblePolymorphic(rdxWeakRTRef(rdxCType) from, rdxWeakRTRef(rdxCType) to) const;
	bool TypeImplementsInterface(rdxWeakRTRef(rdxCStructuredType) tClass, rdxWeakRTRef(rdxCStructuredType) tIfc) const;
	bool TypeIsObjectReference(rdxWeakRTRef(rdxCType) t) const;
	bool TypeIsArray(rdxWeakRTRef(rdxCType) t) const;
	bool TypeIsInterface(rdxWeakRTRef(rdxCType) t) const;
	bool TypeIsClass(rdxWeakRTRef(rdxCType) t) const;
	bool TypeCanBeTraced(rdxWeakRTRef(rdxCType) t) const;
	bool TypeIsValid(rdxWeakRTRef(rdxCType) t) const;
	bool ObjectIsConstant(rdxWeakRTRef(rdxCObject) v) const;
	void TypeValueSize(rdxSOperationContext *ctx, rdxWeakHdl(rdxCType) t, rdxLargeUInt &size, rdxLargeUInt &align) const;
	rdxLargeUInt TypeElementSize(rdxWeakRTRef(rdxCType) t) const;

	void AuditSentinels();

	void DebugDumpGST();

	static void ParseNumber(rdxWeakRTRef(rdxCString) str, rdxBool *pNegative, rdxHugeUInt *pAboveDec, rdxHugeUInt *pBelowDec, rdxHugeUInt *pFractionDivisor);

	struct BoolSerializer : public rdxITypeSerializer
	{
		void DeserializeTextInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakTypelessOffsetHdl instance, rdxITextDeserializer *td, rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool &outShouldProcessProperties) const RDX_OVERRIDE;
		void DeserializeBinaryInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakTypelessOffsetHdl instance, rdxIFileStream *reader, rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool &outShouldProcessProperties) const RDX_OVERRIDE;

		void SerializeBinaryInstance(rdxIObjectManager *objm, rdxWeakTypelessOffsetRTRef obj, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties) const RDX_OVERRIDE;
		void SerializeTextInstance(rdxIObjectManager *objm, rdxWeakTypelessOffsetRTRef obj, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties) const RDX_OVERRIDE;

		static BoolSerializer instance;
	};

	struct StringSerializer : public rdxITypeSerializer
	{
		void DeserializeTextInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakTypelessOffsetHdl instance, rdxITextDeserializer *td, rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool &outShouldProcessProperties) const RDX_OVERRIDE;
		void DeserializeBinaryInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakTypelessOffsetHdl instance, rdxIFileStream *reader, rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool &outShouldProcessProperties) const RDX_OVERRIDE;

		void SerializeBinaryInstance(rdxIObjectManager *objm, rdxWeakTypelessOffsetRTRef obj, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties) const RDX_OVERRIDE;
		void SerializeTextInstance(rdxIObjectManager *objm, rdxWeakTypelessOffsetRTRef obj, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties) const RDX_OVERRIDE;

		static StringSerializer instance;
	};

	rdxCRef(rdxCString) CreateStringFromCS(rdxSOperationContext *ctx, rdxSCandidateString *cs, bool lookupOnly);

	bool ResolveImports(rdxSOperationContext *ctx, rdxIPackageHost *host);
	bool ImportPackageObject(const rdxSObjectGUID &guid, rdxCRef(rdxCObject) *outObjRef);
	bool RefTypesCompatibleRecursive(rdxWeakRTRef(rdxCType) from, rdxWeakRTRef(rdxCType) to, rdxWeakRTRef(rdxCType) initialFrom, int depth) const;
	bool NonRefTypesCompatible(rdxWeakRTRef(rdxCType) from, rdxWeakRTRef(rdxCType) to) const;
	bool ResolveStructureDefault(rdxSOperationContext *ctx, rdxWeakHdl(rdxCStructuredType) st, rdxWeakHdl(rdxCPackage) pkg);
	void VerifyDuplicates(rdxSOperationContext *ctx, rdxIPackageHost *packageHost);
	bool CreateInitialObjects(rdxSOperationContext *ctx, rdxIPackageHost *packageHost, bool critical);
	void GeneratePackageArrayDefs(rdxSOperationContext *ctx, rdxIPackageHost *packageHost);
	rdxCRef(rdxCObject) CreateInitialObject(rdxSOperationContext *ctx, rdxWeakOffsetHdl(rdxSPackageManifestLocal) ml);
	void LoadSymbolsOfCriticality(rdxSOperationContext *ctx, rdxIPackageHost *packageHost, bool critical);
	void InitializeObject(rdxWeakRTRef(rdxCObject) obj, rdxLargeUInt overflow, bool forceZeroFill) RDX_OVERRIDE;
	rdxCRef(rdxCPackage) LoadSinglePackage(rdxSOperationContext *ctx, rdxSDomainGUID domain, rdxIPackageHost *packageHost);
	void HardenStructures(rdxSOperationContext *ctx);
	void ValidateStructures(rdxSOperationContext *ctx);
	void ResolveVFTs(rdxSOperationContext *ctx);
	void FinishPackageLoad(rdxSOperationContext *ctx);
	void FinishDeserialize(rdxSOperationContext *ctx, rdxIPackageHost *host);	// Runs after LoadSinglePackage
	void SavePackageFile(rdxSOperationContext *ctx, rdxSDomainGUID domain, rdxIFileStream *fs,
		rdxWeakHdl(rdxCObject) object, ESaveMode saveMode, bool isText);
	void InitializeObject(void *objectHead, rdxWeakRTRef(rdxCType) type, bool forceZeroFill);

#if 0
	rdxSBuiltIns *GetBuiltIns();
			const BuiltIns *GetBuiltIns() const;
			bool MethodOverrideCompatible(const StructuredType *implementingType, const Method *base, const Method *ov);
			bool ResolveStructure(rdxSOperationContext *ctx, StructuredType *st);
			void InitializeBuiltInTypes(rdxSOperationContext *ctx);

			rdxLargeInt TypeElementSize(const Type *t);
			CRef<void> CreateContainer(rdxSOperationContext *ctx, rdxLargeInt elementSize, rdxLargeInt numElements, Int numDimensions,
				Domain domain, const Type *t, const ITypeProcessor *tp, rdxLargeInt overflow);
			CRef<const String> CreateStringFromCS(rdxSOperationContext *ctx, CandidateString *cs, bool lookupOnly);
			CRef<const String> CreateStringASCII(rdxSOperationContext *ctx, const char *str, rdxLargeInt size = -1, bool lookupOnly = false);
			CRef<const String> CreateString(rdxSOperationContext *ctx, const Char *str, rdxLargeInt numChars, bool lookupOnly = false);
			CRef<const String> CreateStringUTF8(rdxSOperationContext *ctx, const Byte *str, rdxLargeInt size, bool lookupOnly = false);
			CRef<const String> CreateStringConcatenated(rdxSOperationContext *ctx, const String *str1, const String *str2);
			CRef<const String> CreateStringSub(rdxSOperationContext *ctx, const String *str, rdxLargeInt startIndex, rdxLargeInt length = -1);
			CRef<RuntimeThread> CreateThread(rdxSOperationContext *ctx, rdxLargeInt stackSize);
			void AddGlobalSymbol(rdxSOperationContext *ctx, const String *symbolName, void *object);
			Domain HostGetSymbolDomain(String *symbol);
			bool TypeIsValid(const Type *t) const;
			bool ObjectIsConstant(const void *obj) const;
			bool TypeIsObjectReference(const Type *t) const;
			bool TypeCanBeTraced(const Type *t) const;
			bool RefTypesCompatibleRecursive(const Type *from, const Type *to, const Type *initialFrom, int depth) const;
			bool NonRefTypesCompatible(const Type *from, const Type *to) const;
			bool TypesCompatible(const Type *from, const Type *to) const;
			bool TypesCompatiblePolymorphic(const Type *from, const Type *to) const;
			bool ObjectCompatible(const void *from, const Type *to) const;
			bool EnumCompatible(EnumValue ev, const Enumerant *enums) const;
			void TypeValueSize(rdxSOperationContext *ctx, const Type *t, rdxLargeInt &size, rdxLargeInt &align) const;
			void ResolveObjectInteriorReferences(rdxSOperationContext *ctx, void *obj, bool finalStep);
			void ResolveInteriorReferences(rdxSOperationContext *ctx, bool finalStep);
			bool ResolveStructureDefault(rdxSOperationContext *ctx, StructuredType *st);
			void LoadSymbolsOfCriticality(rdxSOperationContext *ctx, IPackageHost *packageHost, bool criticality);
			CRef<Package> LoadSinglePackage(rdxSOperationContext *ctx, Domain domain, IPackageHost *packageHost);


			CRef<Package> LoadPackage(rdxSOperationContext *ctx, Domain domain, IPackageHost *packageHost);
			CRef<void> LoadObject(rdxSOperationContext *ctx, IPackageHost *packageHost);	// Returns the first symbol in the DOMAIN_Runtime package from packageHost

			void SavePackage(rdxSOperationContext *ctx, Domain domain, IPackageHost *packageHost);
			void SaveObject(rdxSOperationContext *ctx, const void *object, IO::IFileStream *fs, bool isText);
			void SavePackageFile(rdxSOperationContext *ctx, Domain domain, IO::IFileStream *fs,
				const void *object, SaveMode saveMode, bool isText);

			void RegisterPackage(rdxSOperationContext *ctx, Domain domain, Package *pkg);
			bool PackageLoaded(Domain domain);
			CRef<ArrayOfType> CreateArrayType(rdxSOperationContext *ctx, Type *t, rdxLargeInt numDimensions, bool constant);
			CRef<void> LookupSymbolSimple(rdxSOperationContext *ctx, const String *symbolName);
			CRef<void> LookupSymbol(rdxSOperationContext *ctx, const String *symbolName, IPackageHost *packageHost);

			void DebugDumpGST();
#endif
};

#include "rdx_loadshell.hpp"

#endif
