#ifndef __RDX_PACKAGE_HPP__
#define __RDX_PACKAGE_HPP__

#include "rdx_objectguid.hpp"
#include "rdx_reftypedefs.hpp"
#include "rdx_typerelationships.hpp"
#include "rdx_packagesymbolloc.hpp"

struct rdxIObjectManager;
struct rdxIObjectReferenceVisitor;
class rdxCStructuredType;
class rdxCType;

struct rdxSPackageManifestImport
{
	typedef void super;

	rdxSObjectGUID				objectGUID;
	rdxTracedRTRef(rdxCObject)	resolution;

	void VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable);
};

struct rdxSPackageArrayDef
{
	typedef void super;
	
	rdxSObjectGUID					internalGUID;
	rdxSObjectGUID					tempContainedTypeGUID;

	rdxSPackageReference			pkgRef;

	rdxLargeUInt					numDimensions;
	bool							isConstant;
	rdxTracedRTRef(rdxCArrayOfType)	resolvedArrayType;

	void VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable);
};

struct rdxSPackageManifestLocal
{
	typedef void super;

	enum
	{
		PMLF_Exported			= 1,			// Visible to other packages
		PMLF_Critical			= 2,			// Must be loaded before anything else
		PMLF_Duplicate			= 4,			// Already exists in the GST, only verify identity
		PMLF_Anonymous			= 8,			// Don't insert in GST even if this has a name
		PMLF_Constant			= 16,			// References are ConstPointer
		PMLF_Cloaked			= 32,			// Cloaked
		PMLF_CriticalBorder		= 64,			// Created during critical phase but deserialized during non-critical
	};

	rdxSPackageReference		typePkgRef;
	rdxTracedRTRef(rdxCType)	resolvedType;
	rdxSObjectGUID				objectGUID;
	rdxSObjectGUID				tempTypeGUID;
	rdxTracedRTRef(rdxCObject)	resolvedObject;
	rdxUInt8					flags;
	bool						duplicateChecked;

	rdxLargeUInt				fileOffset;
	rdxLargeUInt				numElements;

	void VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable);
};

struct rdxSPackageManifestString
{
	typedef void super;

	rdxLargeUInt				fileOffset;
	rdxLargeUInt				fileSize;
	rdxTracedRTRef(rdxCString)	resolvedStr;

	void VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable);
};

struct rdxSBinaryBucketCounts
{
	rdxLargeUInt numImports;
	rdxLargeUInt numLocalSymbols;
	rdxLargeUInt numStrings;
	rdxLargeUInt numArrayDefs;
};

class rdxCPackage : public rdxCObject
{
public:
	struct NativeProperties
	{
		rdxSBinaryBucketCounts							bucketCounts;
		rdxSDomainGUID									domain;
		rdxTracedArrayRTRef(rdxSPackageManifestImport)	importedSymbols;
		rdxTracedArrayRTRef(rdxSPackageManifestLocal)	localSymbols;
		rdxTracedArrayRTRef(rdxTracedRTRef(rdxCString))	strings;
		rdxTracedArrayRTRef(rdxSPackageArrayDef)		arrayDefs;
		bool											trusted;
		rdxTracedRTRef(rdxCObject)						firstObject;		// First object, kept for packages in DOMAIN_Runtime
	} m_native;

	rdxCPackage(rdxIObjectManager *objm, rdxGCInfo *info);
	void VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable);
};

void rdxDeserializeReference(rdxSOperationContext *ctx, rdxIObjectManager *objManager, rdxWeakHdl(rdxCPackage) pkg,
	rdxIPackageHost *host, rdxIFileStream *fs, bool isText, bool permitString, rdxCRef(rdxCString) *outStr, rdxSPackageReference *outPkgRef);
void rdxWriteReference(rdxIObjectManager *objm, rdxIFileStream *fs, bool isText, const rdxCSSIDTable *ssidTable, rdxWeakHdl(rdxCObject) obj, bool resTag);

#include "rdx_typeprocessor.hpp"

RDX_DECLARE_COMPLEX_NATIVE_CLASS(rdxCPackage);
RDX_DECLARE_COMPLEX_NATIVE_STRUCT(rdxSPackageManifestImport);
RDX_DECLARE_COMPLEX_NATIVE_STRUCT(rdxSPackageManifestLocal);
RDX_DECLARE_COMPLEX_NATIVE_STRUCT(rdxSPackageManifestString);
RDX_DECLARE_COMPLEX_NATIVE_STRUCT(rdxSPackageArrayDef);

#endif
