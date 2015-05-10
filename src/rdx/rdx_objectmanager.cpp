/*
 * Copyright (C) 2011-2014 Eric Lasota
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
#include <string.h>
#include <stdio.h>
#include <new>

#include "rdx_pragmas.hpp"
#include "rdx_assert.hpp"

#include "rdx_programmability.hpp"
#include "rdx_objectmanagement.hpp"
#include "rdx_objectmanagement_internal.hpp"
#include "rdx_hashmap.hpp"
#include "rdx_basictypes.hpp"
#include "rdx_opcodes.hpp"
#include "rdx_constants.hpp"
#include "rdx_processing.hpp"
#include "rdx_guid.hpp"

#include "rdx_longflow.hpp"
#include "rdx_io.hpp"
#include "rdx_ilcomp.hpp"
#include "rdx_intrinsics.hpp"
#include "rdx_runtime.hpp"
#include "rdx_lut.hpp"
#include "rdx_package.hpp"
#include "rdx_varying.hpp"
#include "rdx_coretypeattribs.hpp"
#include "rdx_builtins.hpp"

static const rdxLargeUInt STRING_ENCODE_BACKLOG_SIZE = 50;

rdxGCInfo *DEBUG_buggy_object;

class rdxCSerializationGraphVisitor : public rdxIObjectReferenceVisitor
{
private:
	rdxISerializer *m_ser;
	rdxEGCLink m_gcl;

public:
	rdxCSerializationGraphVisitor(rdxISerializer *ser, rdxEGCLink gcl);
	virtual void VisitReference(rdxIObjectManager *objm, rdxWeakTypelessOffsetRTRef &ref) RDX_OVERRIDE;
	virtual void VisitReference(rdxIObjectManager *objm, rdxBaseRTRef &ref) RDX_OVERRIDE;
	virtual void VisitReference(rdxIObjectManager *objm, rdxBaseHdl &ref) RDX_OVERRIDE;
	virtual void VisitReference(rdxIObjectManager *objm, rdxBaseIfcRef &ref) RDX_OVERRIDE;
	virtual void VisitReference(rdxIObjectManager *objm, rdxTracedTypelessRTRef &ref) RDX_OVERRIDE;
	virtual void VisitReference(rdxIObjectManager *objm, rdxTracedTypelessIfcRef &ref) RDX_OVERRIDE;
};


// ==============================================================================================
rdxCNumericTypeSerializer<rdxLargeInt, rdxBinPackageLargeInt> rdxLargeIntSerializer;
rdxCNumericTypeSerializer<rdxLargeUInt, rdxBinPackageLargeUInt> rdxLargeUIntSerializer;
rdxCNumericTypeSerializer<rdxInt, rdxInt> rdxIntSerializer;
rdxCNumericTypeSerializer<rdxUInt, rdxUInt> rdxUIntSerializer;
rdxCNumericTypeSerializer<rdxShort, rdxShort> rdxShortSerializer;
rdxCNumericTypeSerializer<rdxUShort, rdxUShort> rdxUShortSerializer;
rdxCNumericTypeSerializer<rdxLong, rdxLong> rdxLongSerializer;
rdxCNumericTypeSerializer<rdxULong, rdxULong> rdxULongSerializer;
rdxCNumericTypeSerializer<rdxFloat, rdxFloat> rdxFloatSerializer;
rdxCNumericTypeSerializer<rdxDouble, rdxDouble> rdxDoubleSerializer;
rdxCNumericTypeSerializer<rdxHashValue, rdxHashValue> rdxHashValueSerializer;
rdxCNumericTypeSerializer<rdxChar, rdxChar> rdxCharSerializer;
rdxCNumericTypeSerializer<rdxByte, rdxByte> rdxByteSerializer;

rdxChar rdxSCandidateString::DecodeCharacter(const void **pData, rdxLargeUInt *pAvailableBytes) const
{
	switch(format)
	{
	case CSF_UTF8:
		{
			return rdxDecodeUTF8Char(pData, pAvailableBytes);
		}
		break;
	case CSF_ASCII:
		{
			const char *pASCII = static_cast<const char *>(*pData);

			if(pASCII[0] & 0x80)
				return rdxCHAR_Invalid;

			(*pAvailableBytes)--;
			*pData = pASCII+1;
			return static_cast<rdxChar>(pASCII[0]);
		}
		break;
	case CSF_Char:
		{
			const rdxChar *pChar = static_cast<const rdxChar *>(*pData);
			(*pAvailableBytes) -= sizeof(rdxChar);
			*pData = pChar+1;
			return pChar[0];
		}
		break;
	case CSF_Concatenated:
		{
			const rdxChar *pChar = static_cast<const rdxChar *>(*pData);
			rdxLargeUInt availableBytes = *pAvailableBytes - sizeof(rdxChar);
			if(availableBytes == numRawBytes2)
				*pData = this->GetSecondaryData();
			else
				*pData = pChar + 1;
			*pAvailableBytes = availableBytes;
			return pChar[0];
		}
		break;
	}

	return rdxCHAR_Invalid;
}

// Returns true if the string is valid
bool rdxSCandidateString::CountCharacters()
{
	if (!rdxCheckAddOverflowU(numRawBytes1, numRawBytes2))
		return false;	// Too big

	rdxLargeUInt availableBytes = numRawBytes1 + numRawBytes2;

	const void *data = GetInitialData();
	switch(this->source)
	{
	case rdxSCandidateString::CSIS_Handle:
		data = this->input1hdl.Data();
		break;
	case rdxSCandidateString::CSIS_Pointer:
		data = this->input1ptr;
		break;
	}

	if(format == CSF_Concatenated || format == CSF_Char)
	{
		numCharacters = availableBytes / sizeof(rdxChar);
		return true;
	}
	if(format == CSF_ASCII)
	{
		numCharacters = availableBytes;
		return true;
	}

	numCharacters = 0;

	while(availableBytes > 0)
	{
		rdxChar c = DecodeCharacter(&data, &availableBytes);
		if(c == rdxCHAR_Invalid)
			break;
		numCharacters++;
	}

	return availableBytes == 0;
}

inline const void *rdxSCandidateString::GetInitialData() const
{
	switch(this->source)
	{
	case rdxSCandidateString::CSIS_Handle:
		return this->input1hdl.Data();
		break;
	case rdxSCandidateString::CSIS_Pointer:
		return this->input1ptr;
		break;
	}
	return NULL;
}

inline const void *rdxSCandidateString::GetSecondaryData() const
{
	switch(this->source)
	{
	case rdxSCandidateString::CSIS_Handle:
		return this->input2hdl.Data();
		break;
	case rdxSCandidateString::CSIS_Pointer:
		return this->input2ptr;
		break;
	}
	return NULL;
}


inline void rdxCHashMapStringTable::InitializeElement(MyHashEntry *v)
{
	v->e.elementType = rdxSHashElement::HMT_ElementTypeEmpty;
	v->e.hash = rdxHashValue(0);
	v->k = rdxWeakArrayHdl(rdxChar)::Null();
	v->v = rdxWeakHdl(rdxCString)::Null();
}

inline rdxLargeUInt rdxCHashMapStringTable::UnitSize() const
{
	return sizeof(MyHashEntry);
}

inline bool rdxCHashMapStringTable::CompareKey(const rdxSCandidateString *cs, const rdxCHashMapStringTable::MyHashEntry *e) const
{
	rdxWeakArrayHdl(rdxChar) ekvHdl = e->k;
	const rdxChar *ekvData = ekvHdl->ArrayData();

	const void *csData = cs->GetInitialData();
	rdxLargeUInt csBytesRemaining = cs->numRawBytes1 + cs->numRawBytes2;

	if(cs->format == rdxSCandidateString::CSF_Char)
		return (cs->numCharacters == e->k->NumElements()) && !memcmp(csData, ekvData, sizeof(rdxChar)*cs->numCharacters);
	else
	{
		if(cs->numCharacters != e->k->NumElements())
			return false;

		if(cs->format == rdxSCandidateString::CSF_ASCII)
		{
			rdxLargeUInt nChars = cs->numCharacters;
			const char *asciiChar = static_cast<const char *>(csData);
			for(rdxLargeUInt i=0;i<nChars;i++)
			{
				if(ekvData[i] != static_cast<rdxChar>(asciiChar[i]))
					return false;
			}
		}
		else if(cs->format == rdxSCandidateString::CSF_Char)
		{
			rdxLargeUInt nChars = cs->numCharacters;
			const rdxChar *csChars = static_cast<const rdxChar *>(csData);
			for(rdxLargeUInt i=0;i<nChars;i++)
			{
				if(ekvData[i] != csChars[i])
					return false;
			}
		}
		else
		{
			rdxLargeUInt i = 0;
			while(csBytesRemaining)
			{
				rdxChar csc = cs->DecodeCharacter(&csData, &csBytesRemaining);
				if(csc == rdxCHAR_Invalid)
					return false;
				if(ekvData[i++] != csc)
					return false;
			}
		}
	}

	return true;
}

inline rdxHashValue rdxCHashMapStringTable::HashCKey(const void *ckey) const
{
	const rdxSCandidateString *cs = static_cast<const rdxSCandidateString *>(ckey);

	if(cs->format == rdxSCandidateString::CSF_Char)
		return rdxHashBytes(cs->GetInitialData(), sizeof(rdxChar)*cs->numCharacters);
	else
	{
		rdxCIntermediateHash hv;

		const void *csData = cs->GetInitialData();
		rdxLargeUInt csBytesRemaining = cs->numRawBytes1 + cs->numRawBytes2;

		while(csBytesRemaining)
		{
			rdxChar c = cs->DecodeCharacter(&csData, &csBytesRemaining);
			if(c == rdxCHAR_Invalid)
				break;
			hv.FeedBytes(&c, sizeof(rdxChar));
		}
		return hv.Flush();
	}
}

rdxWeakArrayHdl(rdxChar) rdxCHashMapStringTable::ConstructKey(rdxSOperationContext *ctx, const void *ckey, bool &recheck)
{
	const rdxSCandidateString *cs = static_cast<const rdxSCandidateString *>(ckey);
	recheck = true;

	rdxArrayCRef(rdxChar) carray;

	RDX_TRY(ctx)
	{
		RDX_PROTECT_ASSIGN(ctx, carray, rdxCInternalObjectFactory::Create1DArray<rdxChar>(ctx, m_native.objectManager, cs->numCharacters, m_aot_ConstChar.ToWeakHdl(), rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime), 1, sizeof(rdxChar), rdxSAutoTypeInfo<rdxChar>::TypeInfoInterface()));
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROWV(ctx, rdxWeakArrayHdl(rdxChar)::Null());
	}
	RDX_ENDTRY

	rdxWeakArrayHdl(rdxChar) carrayH = carray.ToWeakHdl();

	carrayH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_ConstantArray;

	if(cs->format == rdxSCandidateString::CSF_Char)
		memcpy(carrayH->ArrayModify(), cs->GetInitialData(), sizeof(rdxChar) * (cs->numCharacters));
	else
	{
		const void *csData = cs->GetInitialData();
		rdxLargeUInt csBytesRemaining = cs->numRawBytes1 + cs->numRawBytes2;

		rdxChar *kv = carrayH->ArrayModify();

		while(csBytesRemaining)
		{
			*kv = cs->DecodeCharacter(&csData, &csBytesRemaining);
			kv++;
		}
	}
	carrayH->Element(cs->numCharacters) = static_cast<rdxChar>(0);

	const_cast<rdxSCandidateString *>(cs)->feedback = carrayH;
	return carrayH;
}

// =======================================================================================
// rdxCSerializationGraphVisitor
rdxCSerializationGraphVisitor::rdxCSerializationGraphVisitor(rdxISerializer *ser, rdxEGCLink gcl)
{
	m_ser = ser;
	m_gcl = gcl;
}

void rdxCSerializationGraphVisitor::VisitReference(rdxIObjectManager *objm, rdxWeakTypelessOffsetRTRef &obj)
{
	objm->GraphIncludeObject(m_ser, *obj.GetObjRef());
}

void rdxCSerializationGraphVisitor::VisitReference(rdxIObjectManager *objm, rdxBaseRTRef &obj)
{
	if(obj.GetPOD() != RDX_CNULL)
		objm->GraphIncludeObject(m_ser, rdxWeakRTRef(rdxCObject)(rdxObjRef_CSignal_BaseRef, obj));
}

void rdxCSerializationGraphVisitor::VisitReference(rdxIObjectManager *objm, rdxBaseHdl &obj)
{
	if(obj.GetPOD() != RDX_CNULL)
		objm->GraphIncludeObject(m_ser, rdxWeakRTRef(rdxCObject)(rdxObjRef_CSignal_BaseRef, obj));
}

void rdxCSerializationGraphVisitor::VisitReference(rdxIObjectManager *objm, rdxBaseIfcRef &obj)
{
	if(obj.GetPOD() != RDX_CNULL)
		objm->GraphIncludeObject(m_ser, rdxWeakRTRef(rdxCObject)(rdxObjRef_CSignal_BaseRef, obj));
}

void rdxCSerializationGraphVisitor::VisitReference(rdxIObjectManager *objm, rdxTracedTypelessRTRef &obj)
{
	if(obj.IsNotNull())
		objm->GraphIncludeObject(m_ser, obj.ToWeakRTRef());
}

void rdxCSerializationGraphVisitor::VisitReference(rdxIObjectManager *objm, rdxTracedTypelessIfcRef &obj)
{
	if(obj.IsNotNull())
		objm->GraphIncludeObject(m_ser, obj.ToWeakRTRef());
}

// =======================================================================================
// rdxCObjectManager_Impl
rdxCObjectManager::rdxCObjectManager(rdxSAllocator alloc, const rdxINativeTypeHost *nth, const rdxICodeProvider *cp)
{
	m_allocator = alloc;
	m_nth = nth;
	m_codeProvider = cp;

	m_singleThreadMode = true;

	m_stringTable.SetObjectManager(this);
	m_aotTable.SetObjectManager(this);
	m_gst.SetObjectManager(this);
	m_packages.SetObjectManager(this);
	m_parseLocalResources.SetObjectManager(this);
	m_parseImportedResources.SetObjectManager(this);
	m_arrayDefHashMap.SetObjectManager(this);

	m_initialized = false;

#ifdef RDX_ENABLE_SMP
	m_ignoreMutexesDuringSyncs = false;
	for(int i=0;i<rdxCOREMUTEX_Count;i++)
	{
		m_operationsActive[i] = false;
		m_coreMutexes[i] = NULL;
	}
#endif

	m_forcePackagesRelevant = false;

	for(int i=0;i<rdxGCL_Count;i++)
	{
		m_liveRoot.next[i] = m_liveRoot.prev[i] = &m_liveRoot;
		m_unmarkedRoot.next[i] = m_unmarkedRoot.prev[i] = &m_unmarkedRoot;
		m_markedRoot.next[i] = m_markedRoot.prev[i] = &m_markedRoot;
		m_scannedRoot.next[i] = m_scannedRoot.prev[i] = &m_scannedRoot;
	}

	m_gcFrequency = 5000;
	m_gcCounter.WriteUnfenced(0);

	m_liveRoot.objectFlags = rdxGCInfo::GCOF_LinkTerminator;
	m_unmarkedRoot.objectFlags = rdxGCInfo::GCOF_LinkTerminator;
	m_markedRoot.objectFlags = rdxGCInfo::GCOF_LinkTerminator;
	m_scannedRoot.objectFlags = rdxGCInfo::GCOF_LinkTerminator;

	m_unloadedPackages.EvictAll();
	m_unprocessedObjects.EvictAll();

	m_shouldDisposeIL = true;
}

rdxCObjectManager::~rdxCObjectManager()
{
}

rdxCObjectManager::FileWriteSerializer::FileWriteSerializer(rdxSDomainGUID targetDomain, rdxIObjectManager *objm)
{
	m_targetDomain = targetDomain;
	m_objm = objm;
}

bool rdxCObjectManager::FileWriteSerializer::TryIncludeObject(rdxWeakRTRef(rdxCObject) obj)
{
	if(obj.IsNull())
		return false;

	rdxGCInfo *info = obj->ObjectInfo();

	if(info->containerType == m_objm->GetBuiltIns()->st_String)
		return true;	// Always need strings

	if(info->Domain() == m_targetDomain
		|| info->Domain() == rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime)
		|| info->Domain() == rdxSDomainGUID::Builtin(rdxDOMAIN_Duplicable))
	{
		return true;
	}
	return false;
}

void rdxCObjectManager::FileWriteSerializer::SerializeReference(rdxWeakRTRef(rdxCObject) obj)
{
}

void rdxCObjectManager::FileWriteSerializer::SerializeBulk(rdxWeakTypelessOffsetRTRef data, rdxLargeUInt sz)
{
}

void rdxCObjectManager::FileWriteSerializer::SerializeData(rdxWeakRTRef(rdxCType) type, rdxWeakTypelessOffsetRTRef data)
{
}

rdxCObjectManager::GarbageCollectionSerializer::GarbageCollectionSerializer()
{
	m_isActive = false;
}

bool rdxCObjectManager::GarbageCollectionSerializer::TryIncludeObject(rdxWeakRTRef(rdxCObject) obj)
{
	// Remove from the condemned set
	rdxGCInfo *gcInfo = obj->ObjectInfo();

	return true;
}

void rdxCObjectManager::GarbageCollectionSerializer::SerializeReference(rdxWeakRTRef(rdxCObject) obj)
{
}

void rdxCObjectManager::GarbageCollectionSerializer::SerializeData(rdxWeakRTRef(rdxCType) type, rdxWeakTypelessOffsetRTRef data)
{
}

void rdxCObjectManager::GarbageCollectionSerializer::SerializeBulk(rdxWeakTypelessOffsetRTRef data, rdxLargeUInt sz)
{
}


void rdxCObjectManager::GarbageCollectionSerializer::SetActive()
{
	m_isActive = true;
}

bool rdxCObjectManager::GarbageCollectionSerializer::IsActive()
{
	return m_isActive;
}

void rdxCObjectManager::GarbageCollectionSerializer::SetPermitFlexibleLayouts(bool permit)
{
	m_permitFlexibleLayouts = true;
}

void rdxCObjectManager::ExternalGCMarkObject(rdxGCInfo *gcObject)
{
	if(gcObject->objectFlags & rdxGCInfo::GCOF_GCMarkOnAssign)
	{
#ifdef RDX_ENABLE_SMP
		m_coreMutexes[rdxCOREMUTEX_ObjectList]->Acquire();
#endif

		gcObject->objectFlags &= ~static_cast<rdxUInt32>(rdxGCInfo::GCOF_GCMarkOnAssign);
		MarkGraphObject(gcObject->ObjectWeakRTRef(), &m_garbageCollector);

#ifdef RDX_ENABLE_SMP
		m_coreMutexes[rdxCOREMUTEX_ObjectList]->Release();
#endif
	}
}

void rdxCObjectManager::MarkGraphObject(rdxWeakRTRef(rdxCObject) obj, rdxISerializer *ser)
{
	if(obj.IsNull())
		return;

	rdxGCInfo *objGCI = obj->ObjectInfo();

	if(objGCI->objectFlags & rdxGCInfo::GCOF_GraphMarked)
		return;		// Already marked this

	objGCI->objectFlags |= rdxGCInfo::GCOF_GraphMarked;

	// Mark this as an inclusion
	objGCI->Unlink(rdxGCL_ScanLink);
	objGCI->LinkBefore(&m_markedRoot, rdxGCL_ScanLink);

	// Flag as external
	if(ser->TryIncludeObject(obj))
		objGCI->objectFlags &= (~rdxGCInfo::GCOF_External);
	else
		objGCI->objectFlags |= rdxGCInfo::GCOF_External;
}

void rdxCObjectManager::VisitStructureReferences(void *base, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable, rdxIfcTypeInfo typeInfo, rdxWeakRTRef(rdxCStructuredType) st, rdxLargeUInt stride, rdxLargeUInt count)
{
	rdxIfcTypeFuncs::VisitReferencesFunc vrf = RDX_CNULL;

	if(typeInfo.IsNotNull())
	{
		rdxIfcTypeFuncs typeFuncs = typeInfo.TypeFuncs();
		if(typeFuncs.IsNotNull())
			vrf = typeFuncs.GetVisitReferencesFunc();
	}

	rdxLargeUInt nRefs = 0;
	const rdxCStructuredType::NativeProperties::ContainedReference *refsData = RDX_CNULL;

	// Scan out references from the reference layout table
	if(st.IsNotNull())
	{
		rdxWeakArrayRTRef(rdxCStructuredType::NativeProperties::ContainedReference) refs = st->m_native.containedReferences.ToWeakRTRef();

		if(refs.IsNotNull())
		{
			nRefs = refs->NumElements();
			refsData = refs->ArrayData();
		}
	}
	
	rdxUInt8 *bytesBase = reinterpret_cast<rdxUInt8*>(base);
	for(rdxLargeUInt i=0;i<count;i++)
	{
		for(rdxLargeUInt j=0;j<nRefs;j++)
			visitor->VisitReference(this, *reinterpret_cast<rdxTracedTypelessRTRef *>(bytesBase + refsData[j].offset));
		if(vrf)
			vrf(this, bytesBase, visitor, visitNonSerializable);
		bytesBase += stride;
	}
}

rdxIfcTypeInfo rdxCObjectManager::GetBasicValueArrayTypeInfo() const
{
	return rdxSAutoTypeInfo<rdxCArrayContainer>::TypeInfoInterface();
}

rdxIfcTypeInfo rdxCObjectManager::GetBasicReferenceArrayTypeInfo() const
{
	return rdxSAutoTypeInfo<rdxCArray<rdxTracedRTRef(rdxCObject)> >::TypeInfoInterface();
}

rdxIfcTypeInfo rdxCObjectManager::GetBasicInterfaceArrayTypeInfo() const
{
	return rdxSAutoTypeInfo<rdxCArray<rdxTracedIRef(rdxSObjectInterfaceImplementation)> >::TypeInfoInterface();
}

rdxIfcTypeInfo rdxCObjectManager::GetBasicBoxTypeInfo() const
{
	return rdxSAutoTypeInfo<rdxCStructContainer>::TypeInfoInterface();
}

rdxIfcTypeInfo rdxCObjectManager::GetBasicReferenceTypeInfo() const
{
	return rdxSAutoTypeInfo<rdxTracedRTRef(rdxCObject)>::TypeInfoInterface();
}

rdxIfcTypeInfo rdxCObjectManager::GetBasicInterfaceTypeInfo() const
{
	return rdxSAutoTypeInfo<rdxTracedIRef(rdxSObjectInterfaceImplementation)>::TypeInfoInterface();
}


void rdxCObjectManager::VisitObjectReferences(rdxWeakRTRef(rdxCObject) obj, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
{
	rdxSSerializationTag *objDebugTag = obj->ObjectInfo()->SerializationTag();
	rdxGCInfo *info = obj->ObjectInfo();

	if(info->objectFlags & rdxGCInfo::GCOF_Array)
		obj.StaticCast<rdxCArrayContainer>()->VisitReferences(this, visitor, visitNonSerializable);
	else if(info->objectFlags & rdxGCInfo::GCOF_Box)
		obj.StaticCast<rdxCStructContainer>()->VisitReferences(this, visitor, visitNonSerializable);
	else
	{
		// Object

		// Feed the structure through its type processor
		rdxIfcTypeInfo ti = info->typeInfo;

		rdxWeakRTRef(rdxCStructuredType) st = info->containerType.StaticCast<rdxCStructuredType>().ToWeakRTRef();

		if(ti.TypeFuncs().IsNotNull() && ti.TypeFuncs().GetVisitReferencesFunc())
		{
			rdxIfcTypeFuncs::VisitReferencesFunc vrf = ti.TypeFuncs().GetVisitReferencesFunc();
			void *objectHead = ti.TypeFuncs().GetRefToObjectHeadFunc()(obj.Modify());
			
			vrf(this, objectHead, visitor, visitNonSerializable);
		}

		// Scan out references from the reference layout table
		if(st.IsNotNull())
		{
			rdxWeakArrayRTRef(rdxCStructuredType::NativeProperties::ContainedReference) refs = st->m_native.containedReferences.ToWeakRTRef();
			rdxSSerializationTag *tag = st->ObjectInfo()->SerializationTag();

			void *cObjectHead = obj.Modify();
			if(refs.IsNotNull())
			{
				rdxLargeUInt nRefs = refs->NumElements();

				rdxUInt8 *bytesBase = reinterpret_cast<rdxUInt8*>(obj.Modify());
				for(rdxLargeUInt i=0;i<nRefs;i++)
					visitor->VisitReference(this, *reinterpret_cast<rdxTracedTypelessRTRef *>(bytesBase + refs->Element(i).offset));
			}
		}
	}

	// Also mark anything referenced by this object
	visitor->VisitReference(this, info->containerType);

	rdxSSerializationTag *serializationTag = info->SerializationTag();
	if (serializationTag)
	{
		if(visitNonSerializable)
			visitor->VisitReference(this, serializationTag->package);
	}
}

void rdxCObjectManager::StartGraphing(rdxISerializer *ser, bool isGC)
{
#ifdef RDX_ENABLE_SMP
	this->m_coreMutexes[rdxCOREMUTEX_ObjectList]->Acquire();
#endif

	rdxGCInfo *info = m_liveRoot.next[rdxGCL_LiveObjects];

	// Clear all scan graphs
	m_markedRoot.next[rdxGCL_ScanLink] = m_markedRoot.prev[rdxGCL_ScanLink] = &m_markedRoot;
	m_unmarkedRoot.next[rdxGCL_ScanLink] = m_unmarkedRoot.prev[rdxGCL_ScanLink] = &m_unmarkedRoot;
	m_scannedRoot.next[rdxGCL_ScanLink] = m_scannedRoot.prev[rdxGCL_ScanLink] = &m_scannedRoot;

	while (info != &m_liveRoot)
	{
		info->objectFlags &= ~(rdxGCInfo::GCOF_GraphMarked | rdxGCInfo::GCOF_GraphScanned);
		if(isGC)
			info->objectFlags |= rdxGCInfo::GCOF_GCMarkOnAssign;

		info->next[rdxGCL_ScanLink] = info->prev[rdxGCL_ScanLink] = NULL;

		// NOTE: We may be reading this value before a subsequent decrement or addition,
		// but those will occur after we read this.
		if (isGC && info->numExternalReferences.ReadUnfenced() != 0)
			MarkGraphObject(info->ObjectWeakRTRef(), ser);
		else
			info->LinkBefore(&m_unmarkedRoot, rdxGCL_ScanLink);
		info = info->next[rdxGCL_LiveObjects];
	}

	if(isGC)
		m_garbageCollector.SetActive();
	
#ifdef RDX_ENABLE_SMP
	this->m_coreMutexes[rdxCOREMUTEX_ObjectList]->Release();
#endif
}

bool rdxCObjectManager::ContinueGraph(rdxISerializer *ser, bool markNonSerializable, rdxLargeUInt maxNumObjects /*= 0*/)
{
	// TODO MUSTFIX: Can't do this while another thread is deserializing until structures are hardened
	bool maxNumObjectsDefined = (maxNumObjects != 0);
	bool deferredAny = false;
	
	rdxCSerializationGraphVisitor v(ser, rdxGCL_ScanLink);

	rdxGCInfo *scanStartPoint = &m_markedRoot;

	while(!maxNumObjectsDefined || maxNumObjects)
	{
		maxNumObjects--;

		rdxGCInfo *obj = scanStartPoint->next[rdxGCL_ScanLink];
		if(obj == &m_markedRoot)
			return !deferredAny;
		if((obj->objectFlags & rdxGCInfo::GCOF_VaryingLayout) && !m_singleThreadMode)
		{
			// Varying-layout objects can't be scanned if SMP is active
			scanStartPoint = obj;
			deferredAny = true;
			continue;
		}

		//rdxWeakRTRef(rdxCObject) temp1 = m_unloadedPackages.m_objects.ToWeakRTRef();
		rdxWeakRTRef(rdxCObject) temp2 = obj->ObjectWeakRTRef();
		
		if(const rdxSSerializationTag *serTag = obj->SerializationTag())
		{
			//printf("Visiting %s\n", serTag->gstSymbol.DebugStr());
		}

		// Note that this will visit unloaded objects.  This is necessary because strings deserialized from text files may wind up
		// placed in partly-deserialized arrays.  In the future, maybe this can be avoided by changing the text format so that
		// manifest parsing can locate all strings.
		if(!(obj->objectFlags & rdxGCInfo::GCOF_External))
			this->VisitObjectReferences(obj->ObjectWeakRTRef(), &v, markNonSerializable);

		obj->objectFlags |= rdxGCInfo::GCOF_GraphScanned;
		obj->Unlink(rdxGCL_ScanLink);
		obj->LinkBefore(&m_scannedRoot, rdxGCL_ScanLink);
	}
	return false;
}

void rdxCObjectManager::GraphIncludeObject(rdxISerializer *ser, rdxWeakRTRef(rdxCObject) obj)
{
	if(obj.IsNull())
		return;

	rdxGCInfo *objInfo = obj->ObjectInfo();
	if((objInfo->objectFlags & rdxGCInfo::GCOF_GraphMarked) == 0)
		this->MarkGraphObject(obj, ser);
}

void rdxCObjectManager::AuditSentinels()
{
#ifdef RDX_ENABLE_SENTINELS
	GCInfo *link = m_linkHead.next[_liveSetGCL];
	while(link != &_linkTail)
	{
		UInt32 s1 = link->sentinel1;
		UInt32 s2 = link->sentinel2[0];
		if(s1 != 0xdeadbeef || s2 != 0xdeadbeef)
			Utility::DebugBreak(rdxBREAKCAUSE_SentinelAuditFailed);
		if(link->next[_liveSetGCL] && link->next[m_liveSetGCL]->prev[m_liveSetGCL] != link)
			Utility::DebugBreak(rdxBREAKCAUSE_SentinelAuditFailed);
		link = link->next[m_liveSetGCL];
	}
#endif
}

rdxCRef(rdxCObject) rdxCObjectManager::FirstLiveObject() const
{
	rdxGCInfo *obj = m_liveRoot.next[rdxGCL_LiveObjects];
	if(obj == &m_liveRoot)
		return rdxCRef(rdxCObject)::Null();
	return obj->ObjectCRef();
}

rdxCRef(rdxCObject) rdxCObjectManager::NextLiveObject(rdxWeakRTRef(rdxCObject) obj) const
{
	rdxGCInfo *nextObj = obj->ObjectInfo()->next[rdxGCL_LiveObjects];
	if(nextObj == &m_liveRoot)
		return rdxCRef(rdxCObject)::Null();
	return nextObj->ObjectCRef();
}

void rdxCObjectManager::InitializeCoreMutexes(rdxSOperationContext *ctx)
{
#ifdef RDX_ENABLE_SMP
	RDX_TRY(ctx)
	{
		m_ignoreMutexesDuringSyncs = true;
		for(int i=0;i<rdxCOREMUTEX_Count;i++)
		{
			m_coreMutexes[i] = rdxCreateMutex(m_allocator);
			if(!m_coreMutexes[i])
				RDX_STHROW(ctx, RDX_ERROR_ALLOCATION_FAILED);
		}
		m_ignoreMutexesDuringSyncs = false;
	}
	RDX_CATCH(ctx)
	{
		m_ignoreMutexesDuringSyncs = false;
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
#endif
}

rdxIMutex *rdxCObjectManager::GetCoreMutex(int mNum)
{
#ifdef RDX_ENABLE_SMP
	return m_coreMutexes[mNum];
#else
	return NULL;
#endif
}

void rdxCObjectManager::IncrementThreadCounter()
{
	// TODO: Delete
	//m_coreMutexes[rdxCOREMUTEX_OperationContext]->Acquire();
	//m_activeContextCount.Increment<rdxEAS_FullFence>();
	//m_coreMutexes[rdxCOREMUTEX_OperationContext]->Release();
}

void rdxCObjectManager::DecrementThreadCounter()
{
	// TODO: Delete
	//m_coreMutexes[rdxCOREMUTEX_OperationContext]->Acquire();
	//m_activeContextCount.Decrement<rdxEAS_FullFence>();
	//m_coreMutexes[rdxCOREMUTEX_OperationContext]->Release();
}

const rdxINativeTypeHost *rdxCObjectManager::GetTypeHost() const
{
	return m_nth;
}

const rdxICodeProvider *rdxCObjectManager::GetCodeProvider() const
{
	return m_codeProvider;
}

rdxSAllocator *rdxCObjectManager::GetAllocator()
{
	return &m_allocator;
}

void *rdxCObjectManager::GetLocalResourcesMap()
{
	return &m_parseLocalResources;
}

void *rdxCObjectManager::GetImportedResourcesMap()
{
	return &m_parseImportedResources;
}

void *rdxCObjectManager::GetArrayDefMap()
{
	return &m_arrayDefHashMap;
}


void rdxCObjectManager::Shutdown()
{
	rdxSAllocator alloc = m_allocator;

	m_markedRoot.next[rdxGCL_ScanLink] = m_markedRoot.prev[rdxGCL_ScanLink] = &m_markedRoot;
	m_unmarkedRoot.next[rdxGCL_ScanLink] = m_unmarkedRoot.prev[rdxGCL_ScanLink] = &m_unmarkedRoot;
	m_scannedRoot.next[rdxGCL_ScanLink] = m_scannedRoot.prev[rdxGCL_ScanLink] = &m_scannedRoot;

	// Unlink everything in the live set
	rdxGCInfo *info = m_liveRoot.next[rdxGCL_LiveObjects];
	while(info != &m_liveRoot)
	{
		info->objectFlags &= ~(rdxGCInfo::GCOF_GCMarkOnAssign | rdxGCInfo::GCOF_GraphScanned | rdxGCInfo::GCOF_GraphMarked);
		info->LinkBefore(&m_unmarkedRoot, rdxGCL_ScanLink);
		info = info->next[rdxGCL_LiveObjects];
	}

	DestroyUnmarkedObjects(NULL);

	// Release mutexes
#ifdef RDX_ENABLE_SMP
	for (rdxLargeUInt i=0;i<rdxCOREMUTEX_Count;i++)
	{
		if(m_coreMutexes[i] != NULL)
			rdxDestroyMutex(m_coreMutexes[i]);
	}
#endif

	// Destroy all members
	this->~rdxCObjectManager();

	// Release everything
	alloc.Free(this);
}
		
void rdxCObjectManager::SetCollectionFrequency(rdxLargeUInt frequency)
{
	m_gcFrequency = frequency;
}
		
void rdxCObjectManager::SetILDisposal(bool shouldDispose)
{
	this->m_shouldDisposeIL = shouldDispose;
}


void rdxCObjectManager::CollectGarbage(rdxSOperationContext *ctx)
{
	// Can't collect garbage until builtin structures are resolved because it'll trash the internal refs
	if(!m_initialized)
		return;

	// Can't collect garbage in MT
	if(!m_singleThreadMode)
		RDX_LTHROW(ctx, RDX_ERROR_SMP_OPERATION_FORBIDDEN);

	StartGraphing(&m_garbageCollector, true);
	m_garbageCollector.SetPermitFlexibleLayouts(true);
	ContinueGraph(&m_garbageCollector, true);
	m_garbageCollector.SetPermitFlexibleLayouts(false);
	DestroyUnmarkedObjects(ctx);
}

void rdxCObjectManager::DestroyUnmarkedObjects(rdxSOperationContext *ctx)
{
#ifdef RDX_ENABLE_SMP
	m_coreMutexes[rdxCOREMUTEX_ObjectList]->Acquire();
#endif

	{
		rdxGCInfo *current = m_unmarkedRoot.next[rdxGCL_ScanLink];
		while(current != &m_unmarkedRoot)
		{
			rdxGCInfo *next = current->next[rdxGCL_ScanLink];
			current = next;
		}
	}

	// Destroy non-critical objects
	{
		rdxGCInfo *obj = this->m_unmarkedRoot.next[rdxGCL_ScanLink];
		while(obj != &m_unmarkedRoot)
		{
			if((obj->objectFlags & rdxGCInfo::GCOF_Critical) == 0)
			{
				DestroyObject(obj->ObjectWeakRTRef());
			}
			obj = obj->next[rdxGCL_ScanLink];
		}
	}

	// Remove dead strings
	if(m_stringTable.m_native.untracedEntries)
	{
		rdxSHashEntry<rdxWeakHdl(rdxCArray<rdxChar>), rdxWeakHdl(rdxCString)> *entries = m_stringTable.m_native.untracedEntries;
		rdxLargeUInt numEntries = m_stringTable.m_native.numEntries;
		for(rdxLargeUInt i=0;i<numEntries;i++)
		{
			if(entries[i].e.elementType > rdxSHashElement::HMT_ContainsData &&
				!(entries[i].v->ObjectInfo()->objectFlags & rdxGCInfo::GCOF_GraphMarked))
			{
				m_stringTable.RemoveEntry(i);
			}
		}
		m_stringTable.Balance();
	}
	
	// Remove dead AOTs
	if(m_aotTable.m_native.untracedEntries)
	{
		rdxSHashEntry<rdxSAOTKey, rdxWeakHdl(rdxCArrayOfType)> *entries = m_aotTable.m_native.untracedEntries;
		rdxLargeUInt numEntries = m_aotTable.m_native.numEntries;
		for(rdxLargeUInt i=0;i<numEntries;i++)
		{
			if(entries[i].e.elementType > rdxSHashElement::HMT_ContainsData &&
				!(entries[i].v->ObjectInfo()->objectFlags & rdxGCInfo::GCOF_GraphMarked))
			{
				m_aotTable.RemoveEntry(i);
			}
		}
		m_aotTable.Balance();
	}

	// Remove dead global symbols
	if(m_gst.m_native.untracedEntries)
	{
		rdxSHashEntry<rdxSObjectGUID, rdxWeakHdl(rdxCObject)> *entries = m_gst.m_native.untracedEntries;
		rdxLargeUInt numEntries = m_gst.m_native.numEntries;
		for(rdxLargeUInt i=0;i<numEntries;i++)
		{
			if(entries[i].e.elementType > rdxSHashElement::HMT_ContainsData)
			{
				rdxWeakHdl(rdxCObject) entry = entries[i].v;
				if(entry.IsNotNull())
				{
					if(!(entry->ObjectInfo()->objectFlags & rdxGCInfo::GCOF_GraphMarked))
						m_gst.RemoveEntry(i);
				}
			}
		}
		m_gst.Balance();
	}

	// Remove dead packages
	if(m_packages.m_native.untracedEntries)
	{
		rdxSHashEntry<rdxSDomainGUID, rdxWeakHdl(rdxCPackage)> *entries = m_packages.m_native.untracedEntries;
		rdxLargeUInt numEntries = m_packages.m_native.numEntries;
		for(rdxLargeUInt i=0;i<numEntries;i++)
		{
			if(entries[i].e.elementType > rdxSHashElement::HMT_ContainsData)
			{
				rdxWeakHdl(rdxCPackage) pkg = entries[i].v;

				if(pkg.IsNotNull())
				{
					if(!(pkg->ObjectInfo()->objectFlags & rdxGCInfo::GCOF_GraphMarked))
						m_packages.RemoveEntry(i);
				}
			}
		}
		m_packages.Balance();
	}
	
	// Destroy critical objects
	{
		rdxGCInfo *obj = this->m_unmarkedRoot.next[rdxGCL_ScanLink];
		while(obj != &m_unmarkedRoot)
		{
			if((obj->objectFlags & rdxGCInfo::GCOF_Critical) != 0)
			{
				DestroyObject(obj->ObjectWeakRTRef());
			}
			obj = obj->next[rdxGCL_ScanLink];
		}
	}

	// Delete everything in the condemned set
	{
		rdxGCInfo *current = m_unmarkedRoot.next[rdxGCL_ScanLink];
		while(current != &m_unmarkedRoot)
		{
			if(current->objectFlags & rdxGCInfo::GCOF_GraphMarked)
			{
				rdxDebugBreak(rdxBREAKCAUSE_CriticalInternalFailure);
			}
			rdxGCInfo *next = current->next[rdxGCL_ScanLink];
			current->Unlink(rdxGCL_LiveObjects);
			current->Release(m_allocator);
			current = next;
		}
	}

#ifdef RDX_ENABLE_SMP
	m_coreMutexes[rdxCOREMUTEX_ObjectList]->Release();
#endif
}

void rdxCObjectManager::DestroyObject(rdxWeakRTRef(rdxCObject) obj)
{
	rdxGCInfo *gcInfo = obj->ObjectInfo();
	rdxIfcTypeInfo typeInfo = gcInfo->typeInfo;

	if(typeInfo.Flags() & rdxETIF_LifeCycle)
	{
		rdxIfcTypeFuncs::DestroyFunc destroyFunc = typeInfo.TypeFuncs().GetDestroyFunc();
		void *objectHead = typeInfo.TypeFuncs().GetRefToObjectHeadFunc()(obj.Modify());
		destroyFunc(objectHead);
	}
}

void rdxCObjectManager::ParseNumber(rdxWeakRTRef(rdxCString) str, rdxBool *pNegative, rdxHugeUInt *pAboveDec, rdxHugeUInt *pBelowDec, rdxHugeUInt *pFractionDivisor)
{
	rdxLargeUInt nChars = str->Length();
	const rdxChar *chars = str->AsChars()->ArrayData();
	bool negative = false;
	bool pastDecimal = false;

	rdxHugeUInt aboveDec, belowDec, fractionDivisor;

	aboveDec = 0;
	belowDec = 0;
	fractionDivisor = 1;

	for(rdxLargeUInt i=0;i<nChars;i++)
	{
		rdxChar c = chars[i];
		if(c == '-')
			negative = true;
		else if(c == '.')
			pastDecimal = true;
		else
		{
			rdxHugeUInt digit = static_cast<rdxHugeUInt>(static_cast<char>(c) - '0');

			if(pastDecimal)
			{
				fractionDivisor = fractionDivisor * 10;
				belowDec = belowDec * static_cast<rdxHugeUInt>(10) + digit;
			}
			else
				aboveDec = aboveDec * static_cast<rdxHugeUInt>(10) + digit;
		}
	}

	*pNegative = (negative ? rdxTrueValue : rdxFalseValue);
	*pAboveDec = aboveDec;
	*pBelowDec = belowDec;
	*pFractionDivisor = fractionDivisor;
}

rdxCRef(rdxCString) rdxCObjectManager::DeserializeTextString(rdxSOperationContext *ctx, rdxITextDeserializer *td)
{
	RDX_TRY(ctx)
	{
		rdxITextDeserializer::SCompactToken token;
		bool isString;
		RDX_PROTECT(ctx, td->ParseToken(ctx, this, &isString, &token));

		if(!isString)
			return rdxCRef(rdxCString)::Null();
		rdxCRef(rdxCString) str;
		// TODO MUSTFIX: This is dumb if the token is already backed by a CString
		RDX_PROTECT_ASSIGN(ctx, str, this->CreateString(ctx, token.GetCharSpan().Chars(), true, token.GetCharSpan().Length()));
		return str;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROWV(ctx, rdxCRef(rdxCString)::Null());
	}
	RDX_ENDTRY
}

rdxCRef(rdxCString) rdxCObjectManager::DeserializeBinaryString(rdxSOperationContext *ctx, rdxIFileStream *reader, rdxWeakHdl(rdxCPackage) pkg)
{
	RDX_TRY(ctx)
	{
		rdxLargeUInt i;
		bool overflowed = false;
		bool readFailed = false;
		if(!reader->ReadConverted<rdxBinPackageLargeUInt, rdxLargeUInt>(&i, overflowed, readFailed))
		{
			if(overflowed)
				RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);
			if(readFailed)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
		}
		if(i == 1)
			RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
		if(i == 0)
			return rdxCRef(rdxCString)::Null();
		else
		{
			i--;
			rdxLargeUInt numExports = pkg->m_native.localSymbols->NumElements();
			if(i < numExports)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
			i -= numExports;
			if(i >= pkg->m_native.strings->NumElements())
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
			return pkg->m_native.strings->Element(i).ToCRef();
		}
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROWV(ctx, rdxCRef(rdxCString)::Null());
	}
	RDX_ENDTRY
}

void rdxCObjectManager::SerializeBinaryString(rdxWeakRTRef(rdxCString) str, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable) const
{
	rdxLargeUInt i = 0;
	if(str.IsNotNull())
	{
		rdxCStaticLookupPODKey<rdxBaseHdl::PODType> infoKey(str.ToWeakHdl().GetPOD());
		i = *ssidTable->Lookup(infoKey);
	}
	fs->WriteConverted<rdxLargeUInt, rdxBinPackageLargeUInt>(i);
}

static void WriteEncodedStringBytes(rdxIFileStream *fs, const rdxUInt8 *base, rdxLargeUInt nBase)
{
	rdxUInt8 refined[STRING_ENCODE_BACKLOG_SIZE*3];
	rdxLargeUInt sz = 0;
	for(rdxLargeUInt i=0;i<nBase;i++)
	{
		rdxUInt8 b = base[i];
		if(b < 32)
		{
			refined[sz++] = '\\';
			refined[sz++] = static_cast<rdxUInt8>('0' + b/10);
			refined[sz++] = static_cast<rdxUInt8>('0' + b%10);
		}
		else if(b == 92)
		{
			refined[sz++] = '\\';
			refined[sz++] = '\\';
		}
		else if(b == 39)
		{
			refined[sz++] = '\\';
			refined[sz++] = '\'';
		}
		else
			refined[sz++] = b;
	}
	fs->WriteBytes(refined, sz);
}

void rdxCObjectManager::SerializeTextString(rdxWeakRTRef(rdxCString) str, rdxIFileStream *fs) const
{
	if(str.IsNull())
	{
		fs->WriteBytes(" null", 5);
		return;
	}
			
	fs->WriteBytes(" \'", 2);

	rdxLargeUInt nChars = str->Length();

	const rdxLargeUInt backlogSize = STRING_ENCODE_BACKLOG_SIZE;
	const rdxLargeUInt backlogLimit = STRING_ENCODE_BACKLOG_SIZE - 5;
	rdxUInt8 backlog[backlogSize];
	rdxLargeUInt backlogOffset = 0;

	const rdxChar *asChars = str->AsChars()->ArrayData();
	for(rdxLargeUInt i=0;i<nChars;i++)
	{
		backlogOffset += rdxEncodeUTF8Char(asChars[i], backlog + backlogOffset);
		if(backlogOffset >= backlogLimit)
		{
			WriteEncodedStringBytes(fs, backlog, backlogOffset);
			backlogOffset = 0;
		}
	}

	if(backlogOffset)
		WriteEncodedStringBytes(fs, backlog, backlogOffset);
	fs->WriteBytes("\'", 1);
}


rdxSBuiltIns *rdxCObjectManager::GetBuiltIns()
{
	return &m_builtins;
}

const rdxSBuiltIns *rdxCObjectManager::GetBuiltIns() const
{
	return &m_builtins;
}

bool rdxCObjectManager::MethodOverrideCompatible(rdxWeakRTRef(rdxCStructuredType) implementingType, rdxWeakRTRef(rdxCMethod) base, rdxWeakRTRef(rdxCMethod) ov) const
{
	if(ov.IsNull() || base.IsNull())
		return false;	// NULLs are always invalid

	if(base == ov)
		return true;

	if(ov->isAbstract != rdxFalseValue && base->isAbstract == rdxFalseValue)
		return false;	// Abstracts can't override valid methods

	rdxLargeUInt numOverrideRV = 0;
	if(ov->returnTypes.IsNotNull())
		numOverrideRV = ov->returnTypes->NumElements();

	rdxLargeUInt numOverrideP = 0;
	if(ov->parameters.IsNotNull())
		numOverrideP = ov->parameters->NumElements();

	rdxLargeUInt numBaseRV = 0;
	if(base->returnTypes.IsNotNull())
		numBaseRV = base->returnTypes->NumElements();

	rdxLargeUInt numBaseP = 0;
	if(base->parameters.IsNotNull())
		numBaseP = base->parameters->NumElements();

	if(numOverrideRV != numBaseRV || numOverrideP != numBaseP)
		return false;

	for(rdxLargeUInt i=0;i<numBaseRV;i++)
	{
		// Bad method type def
		if(ov->returnTypes->Element(i).IsNull() || base->returnTypes->Element(i).IsNull())
			return false;

		if(ov->returnTypes->Element(i) != base->returnTypes->Element(i))
			return false;
	}

	if(base->thisParameterOffset != ov->thisParameterOffset)
		return false;

	for(rdxLargeUInt i=0;i<numBaseP;i++)
	{
		// Bad method type def
		if(ov->parameters->Element(i).type.IsNull() || base->parameters->Element(i).type.IsNull())
			return false;

		if(i == (base->thisParameterOffset - 1))
		{
			if(!TypesCompatible(implementingType.ToWeakRTRef(), ov->parameters->Element(i).type.ToWeakRTRef()))
				return false;
		}
		else
		{
			if(ov->parameters->Element(i).type != base->parameters->Element(i).type)
				return false;
		}
		if(ov->parameters->Element(i).isConstant != base->parameters->Element(i).isConstant)
			return false;
		if(ov->parameters->Element(i).isNotNull != base->parameters->Element(i).isNotNull)
			return false;
	}
	return true;
}

// Validates a structure.
// This happens after loading and determines if structures are complaint with various limitations that don't won't stop deserialization.
void rdxCObjectManager::ValidateStructure(rdxSOperationContext *ctx, rdxWeakHdl(rdxCStructuredType) st) const
{
	RDX_TRY(ctx)
	{
		if(st->storageSpecifier == rdxSS_Enum)
		{
			if(st->enumerants.IsNull())
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

			bool hasZero = false;
			rdxLargeUInt nEnumerants = st->enumerants->NumElements();
			for(rdxLargeUInt i=0;i<nEnumerants;i++)
			{
				if(st->enumerants->Element(i).value == 0)
				{
					hasZero = true;
					break;
				}
			}

			if(!hasZero)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
		}

		if(st->parentClass.IsNotNull())
		{
			// Only classes can have parents, the parent must be a class
			if(st->storageSpecifier != rdxSS_Class)
				RDX_STHROW(ctx, RDX_ERROR_FORBIDDEN_CLASS_EXTENSION);
			if(st->parentClass->storageSpecifier != rdxSS_Class)
				RDX_STHROW(ctx, RDX_ERROR_FORBIDDEN_CLASS_EXTENSION);
			if(st->parentClass->isFinal)
				RDX_STHROW(ctx, RDX_ERROR_FORBIDDEN_CLASS_EXTENSION);
					
			if(st->parentClass->isLocalized && !(st->ObjectInfo()->objectFlags & rdxGCInfo::GCOF_TransientDuplicate))
			{
				// Parent is localized, make sure this is in the same domain as its parent
				if(st->ObjectInfo()->Domain() != st->parentClass->ObjectInfo()->Domain())
					RDX_STHROW(ctx, RDX_ERROR_FORBIDDEN_CLASS_EXTENSION);
			}
		}

		rdxLargeUInt numVirtualMethods = 0;
		rdxLargeUInt numProperties = 0;
		if(st->virtualMethods.IsNotNull())
		{
			numVirtualMethods = st->virtualMethods->NumElements();
			rdxLargeUInt numParentVirtualMethods = 0;
			if(st->parentClass.IsNotNull() && st->parentClass->virtualMethods.IsNotNull())
				numParentVirtualMethods = st->parentClass->virtualMethods->NumElements();

			if(numParentVirtualMethods > numVirtualMethods)
				RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_VTABLE);

			// Make sure that all VFT functions are valid
			for(rdxLargeUInt i=0;i<numVirtualMethods;i++)
			{
				rdxWeakHdl(rdxCMethod) vMethod = st->virtualMethods->Element(i).ToWeakHdl();
				if(vMethod.IsNull() || vMethod->parameters.IsNull())
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

				rdxLargeUInt numParameters = vMethod->parameters->NumElements();
				if(vMethod->thisParameterOffset <= 0 || vMethod->thisParameterOffset > numParameters)
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

				if(vMethod->parameters->Element(vMethod->thisParameterOffset - 1).type != st)
				{
					// "this" parameter isn't this type.  That's OK if this was inherited, otherwise the VTable is incompatible.
					if(st->parentClass.IsNull() || st->parentClass->virtualMethods.IsNull() || i >= numParentVirtualMethods || st->parentClass->virtualMethods->Element(i) != vMethod)
						RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_VTABLE);
				}
				if(vMethod->parameters->Element(vMethod->thisParameterOffset - 1).isNotNull == rdxFalseValue ||
					vMethod->parameters->Element(vMethod->thisParameterOffset - 1).isConstant == rdxFalseValue)
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
			}

			// Make sure that all overrides are compatible
			for(rdxLargeUInt i=0;i<numParentVirtualMethods;i++)
			{
				rdxWeakRTRef(rdxCMethod) pMethod = st->parentClass->virtualMethods->Element(i).ToWeakRTRef();
				rdxWeakRTRef(rdxCMethod) vMethod = st->virtualMethods->Element(i).ToWeakRTRef();

				if(pMethod.IsNull() || vMethod.IsNull())
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

				if(!MethodOverrideCompatible(st.ToWeakRTRef(), pMethod, vMethod))
					RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_VTABLE);

				if(vMethod->isAbstract && !st->isAbstract)
					RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_VTABLE);
			}
		}

		if(st->properties.IsNotNull())
		{
			numProperties = st->properties->NumElements();

			if(st->parentClass.IsNotNull())
			{
				rdxLargeUInt numParentProperties = 0;
				if(st->parentClass->properties.IsNotNull())
					numParentProperties = st->parentClass->properties->NumElements();

				if(numParentProperties > numProperties)
					RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_PROPERTIES);

				rdxWeakArrayRTRef(rdxSProperty) props1 = st->properties.ToWeakRTRef();
				rdxWeakArrayRTRef(rdxSProperty) props2 = st->parentClass->properties.ToWeakRTRef();
				for(rdxLargeUInt i=0;i<numParentProperties;i++)
				{
					if(props1->Element(i).type != props2->Element(i).type ||
						props1->Element(i).mustBeConstant != props2->Element(i).mustBeConstant ||
						props1->Element(i).isConstant != props2->Element(i).isConstant)
						RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_PROPERTIES);
				}
			}
		}

		if(st->interfaces.IsNotNull())
		{
			rdxLargeUInt numInterfaces = st->interfaces->NumElements();
			rdxLargeUInt numParentInterfaces = 0;
			if(st->parentClass.IsNotNull())
			{
				if(st->parentClass->interfaces.IsNotNull())
					numParentInterfaces = st->parentClass->interfaces->NumElements();

				if(numParentInterfaces > numInterfaces)
					RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_VTABLE);

				for(rdxLargeUInt i=0;i<numParentInterfaces;i++)
					if(st->interfaces->Element(i).type != st->parentClass->interfaces->Element(i).type ||
						st->interfaces->Element(i).vftOffset != st->parentClass->interfaces->Element(i).vftOffset)
							RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_VTABLE);
			}

			// Can start later in the list since the first are identical and the vtable match can take care of those
			for(rdxLargeUInt i=numParentInterfaces;i<numInterfaces;i++)
			{
				rdxWeakOffsetRTRef(rdxSInterfaceImplementation) impl = st->interfaces.ToWeakRTRef()->OffsetElementRTRef(i);
				if(impl->type.IsNull())
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

				rdxWeakRTRef(rdxCStructuredType) ist = impl->type.ToWeakRTRef();
				rdxLargeUInt numIVT = 0;
				if(ist->virtualMethods.IsNotNull())
					numIVT = ist->virtualMethods->NumElements();

				if(ist->storageSpecifier != rdxSS_Interface)
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

				// Make sure the VTable set is in range
				if(impl->vftOffset < 0 || numIVT > numVirtualMethods || (numVirtualMethods - numIVT) < impl->vftOffset)
					RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_VTABLE);

				for(rdxLargeUInt ivti=0;ivti<numIVT;ivti++)
				{
					// CAUTION: Using refs
					rdxWeakRTRef(rdxCMethod) iMethod = ist->virtualMethods->Element(ivti).ToWeakRTRef();
					rdxWeakRTRef(rdxCMethod) vMethod = st->virtualMethods->Element(impl->vftOffset + ivti).ToWeakRTRef();

					if(iMethod.IsNull())
						RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

					if(!MethodOverrideCompatible(st.ToWeakRTRef(), iMethod, vMethod))
						RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_VTABLE);
				}
			}
		}
		else
		{
			// No interfaces, make sure the parent has none either
			if(st->parentClass.IsNotNull() && st->interfaces.IsNotNull())
				RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_VTABLE);
		}
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}


// Critical structures are guaranteed to be resolved during this except during initialize
bool rdxCObjectManager::ResolveStructure(rdxSOperationContext *ctx, rdxWeakHdl(rdxCStructuredType) st)
{
	bool structureIsMutable = false;
	rdxGCInfo *sti = st->ObjectInfo();

	RDX_TRY(ctx)
	{
		bool zeroFill = true;	// True if the structure is zero-fill
		bool bulk = true;		// True if the structure can be bulk serialized

		if(st->storageSpecifier == rdxSS_Enum)
		{
			if(st->properties.IsNotNull() || (st->m_native.defaultValueReference.symbolLoc != rdxPSL_Null))
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

			st->m_native.size = sizeof(rdxEnumValue);
			st->m_native.alignment = rdxALIGN_EnumValue;
			st->m_native.flags |= (rdxCStructuredType::NativeProperties::STF_FinalDefaultEvaluated | rdxCStructuredType::NativeProperties::STF_StructureEvaluated | rdxCStructuredType::NativeProperties::STF_ZeroFill);
#ifndef RDX_VALIDATE_DESERIALIZED_ENUMS
			st->m_native.flags |= (rdxCStructuredType::NativeProperties::STF_AllowBulkSerialize);
#endif
			return true;
		}

		rdxIfcTypeInfo nativeTypeInfo = st->m_native.nativeTypeInfo;	// May be predefined, i.e. builtins
		if(nativeTypeInfo.IsNull())
		{
			const rdxINativeTypeHost *nth = this->GetTypeHost();
			if(nth)
				nativeTypeInfo = nth->TypeInfoForType(st.ToWeakRTRef());
		}
		bool isNativeType = nativeTypeInfo.IsNotNull();
		
		rdxLargeUInt nProperties = 0;
		rdxLargeUInt nInterfaces = 0;
		if(st->properties.IsNotNull())
			nProperties = st->properties->NumElements();
		if(st->interfaces.IsNotNull())
			nInterfaces = st->interfaces->NumElements();
		
		rdxLargeUInt numParentProperties = 0;
		rdxLargeUInt numParentContainedReferences = 0;
		rdxLargeUInt numContainedReferences = 0;
		rdxLargeUInt currentAlignment = 0;
		rdxLargeUInt currentSize = 0;
		rdxLargeUInt objectBaseOffset = 0;
		rdxLargeUInt parentSize = 0;
		rdxLargeUInt parentAlignment = 0;
		rdxLargeUInt numParentInterfaces = 0;

		if(st->storageSpecifier == rdxSS_Interface)
		{
			if(st->properties.IsNotNull())
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
			if(st->parentClass.IsNotNull())
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
		}

		rdxWeakHdl(rdxCStructuredType) parentClass = rdxWeakHdl(rdxCStructuredType)::Null();
		if(st->storageSpecifier == rdxSS_Class)
		{
			if(st->parentClass.IsNull())
			{
				if(st != m_builtins.st_Object)
					parentClass = m_builtins.st_Object;
			}
			else
			{
				parentClass = st->parentClass;
				if(st == m_builtins.st_Object)
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);	// Core.Object must be parentless
			}
		}
		else
		{
			if(st->parentClass.IsNotNull())
				RDX_STHROW(ctx, RDX_ERROR_NON_CLASS_EXTENDS_CLASS);
		}

		if(parentClass.IsNotNull())
		{
			if(!(parentClass->m_native.flags & rdxCStructuredType::NativeProperties::STF_StructureEvaluated))
				return false;	// Parent isn't loaded, defer
			
			numParentProperties = 0;
			if(parentClass->m_native.propertyOffsets.IsNotNull())
				numParentProperties = parentClass->m_native.propertyOffsets->NumElements();

			if(numParentProperties > nProperties)
				RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_PROPERTIES);
			
			numParentInterfaces = 0;
			if(parentClass->interfaces.IsNotNull())
				numParentInterfaces = parentClass->interfaces->NumElements();

			if(numParentInterfaces > nInterfaces)
				RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_PROPERTIES);

			numParentContainedReferences = numContainedReferences = parentClass->m_native.numContainedReferences;

			if (nativeTypeInfo.IsNotNull())
			{
				if(parentClass->m_native.flags & rdxCStructuredType::NativeProperties::STF_InheritedTypeInfo)
					RDX_STHROW(ctx, RDX_ERROR_NATIVE_TYPE_EXTENDS_NON_NATIVE_TYPE);
			}
			else
			{
				// ST itself didn't have native type info, meaning it's an RDX class/struct
				currentSize = st->parentClass->m_native.sizeInSubclass;

				nativeTypeInfo = st->parentClass->m_native.nativeTypeInfo;

				st->m_native.flags |= rdxCStructuredType::NativeProperties::STF_InheritedTypeInfo;
				if(!(st->parentClass->m_native.flags & st->m_native.flags & rdxCStructuredType::NativeProperties::STF_InheritedTypeInfo))
					currentSize = nativeTypeInfo.SizeInSubclass();

				currentAlignment = st->parentClass->m_native.alignment;

				if(!(st->parentClass->m_native.flags & rdxCStructuredType::NativeProperties::STF_ZeroFill))
					zeroFill = false;
				if(!(st->parentClass->m_native.flags & rdxCStructuredType::NativeProperties::STF_AllowBulkSerialize))
					bulk = false;
				if(st->m_native.flags & rdxCStructuredType::NativeProperties::STF_StructureIsMutable)
					structureIsMutable = true;
			}
		}

		if(nativeTypeInfo.IsNull())
		{
			if(st->storageSpecifier == rdxSS_Class)
				RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);	// Class types must always have NTI
			objectBaseOffset = 0;
		}
		else
			objectBaseOffset = nativeTypeInfo.ObjectBaseOffset();

		if(st->interfaces.IsNotNull())
		{
			zeroFill = false;

			if(st->storageSpecifier != rdxSS_Class &&
				st->storageSpecifier != rdxSS_ValStruct &&
				st->storageSpecifier != rdxSS_RefStruct)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

			if(st->m_native.interfaceOffsets.IsNull())
				RDX_PROTECT_ASSIGN(ctx, st->m_native.interfaceOffsets, rdxCInternalObjectFactory::Create1DArray<rdxLargeUInt>(ctx, this, nInterfaces));
			
			for(rdxLargeUInt i=0;i<numParentInterfaces;i++)
			{
				bulk = false;
				st->m_native.interfaceOffsets->Element(i) = st->parentClass->m_native.interfaceOffsets->Element(i);
			}

			// Determine new contained references/LCPs
			for(rdxLargeUInt i=numParentInterfaces;i<nInterfaces;i++)
			{
				rdxLargeUInt oiiAlign = rdxAlignOf(rdxSObjectInterfaceImplementation);
				if(currentAlignment < oiiAlign)
					currentAlignment = oiiAlign;
				bool overflowed;
				currentSize = rdxPaddedSize(currentSize, oiiAlign, overflowed);
				st->m_native.interfaceOffsets->Element(i) = currentSize;
				if(overflowed)
					RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);
				if(!rdxCheckAddOverflowU(currentSize, sizeof(rdxSObjectInterfaceImplementation)))
					RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);
				currentSize += sizeof(rdxSObjectInterfaceImplementation);
			}
		}

		if(st->properties.IsNotNull())
		{
			if(st->storageSpecifier != rdxSS_Class &&
				st->storageSpecifier != rdxSS_ValStruct &&
				st->storageSpecifier != rdxSS_RefStruct)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

			if(st->m_native.propertyOffsets.IsNull())
				RDX_PROTECT_ASSIGN(ctx, st->m_native.propertyOffsets, rdxCInternalObjectFactory::Create1DArray<rdxLargeUInt>(ctx, this, nProperties) );

			// TODO MUSTFIX: Fix bulk...
			/*
			if(nativeTypeInfo)
			{
				if(nativeTypeInfo->typeFuncs == NULL && nProperties != 0)
					RDX_STHROW(ctx, RDX_ERROR_NATIVE_PROPERTY_BIND_FAILED);

				rdxLargeUInt unpaddedNext = 0;
				for(rdxLargeUInt i=0;i<nProperties;i++)
				{
					rdxLargeUInt offset;
					bool succeeded = nativeTypeInfo->typeFuncs->getPropertyOffsetFunc(st->properties->Element(i).name.ToWeakRTRef(), &offset);
					if(!succeeded)
						RDX_STHROW(ctx, RDX_ERROR_NATIVE_PROPERTY_BIND_FAILED);
					st->m_native.propertyOffsets->Element(i) = offset;

					if(i == 0 && offset != 0)
						bulk = false;
					
#ifndef RDX_ENABLE_PADDED_BULK_SERIALIZE
					if(bulk)
					{
						if(unpaddedNext != offset)
							bulk = false;
						else
						{
							rdxLargeUInt size, align;
							RDX_PROTECT(ctx, this->TypeValueSize(ctx, st->properties->Element(i).type.ToWeakHdl(), size, align));
							if(size == 0)
								return false;

							unpaddedNext += size;
						}
					}
#endif
				}
				
#ifndef RDX_ENABLE_PADDED_BULK_SERIALIZE
				if(unpaddedNext != nativeTypeInfo->size)
					bulk = false;
#endif
			}
			*/

			{
				// Copy the parent properties
				for(rdxLargeUInt i=0;i<numParentProperties;i++)
					st->m_native.propertyOffsets->Element(i) = st->parentClass->m_native.propertyOffsets->Element(i);

				// Determine new contained references/LCPs
				for(rdxLargeUInt i=numParentProperties;i<nProperties;i++)
				{
					bool isClassRef = false;
					bool isInterfaceRef = false;

					rdxWeakOffsetHdl(rdxSProperty) prop = st->properties->OffsetElementRTRef(i).ToHdl();
					rdxLargeUInt pSize = 0;
					rdxLargeUInt pAlignment = 1;
					rdxWeakHdl(rdxCType) pType = prop->type.ToWeakHdl();

					if(prop->name.IsNull())
						RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

					if(!prop->isConstant)
						structureIsMutable = true;

					if(pType.IsNull())
						RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

					if(pType == m_builtins.st_Varying)
						RDX_STHROW(ctx, RDX_ERROR_FORBIDDEN_VARYING_USAGE);

					rdxWeakHdl(rdxCType) pTypeType = pType->ObjectInfo()->containerType.ToWeakHdl();
					if(pTypeType == m_builtins.st_StructuredType)
					{
						rdxWeakHdl(rdxCStructuredType) pst = pType.StaticCast<rdxCStructuredType>();
						switch(pst->storageSpecifier)
						{
						case rdxSS_Interface:
							isInterfaceRef = true;
							break;
						case rdxSS_Class:
							isClassRef = true;
							break;
						case rdxSS_Enum:
							pSize = sizeof(rdxEnumValue);
							pAlignment = rdxALIGN_EnumValue;
							break;
						case rdxSS_ValStruct:
						case rdxSS_RefStruct:
							// Make sure this has been evaluated
							if(!(pst->m_native.flags & rdxCStructuredType::NativeProperties::STF_StructureEvaluated))
								return false;

							if(!rdxCheckAddOverflowU(numContainedReferences, pst->m_native.numContainedReferences))
								RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);

							numContainedReferences += pst->m_native.numContainedReferences;

							pSize = pst->m_native.size;
							pAlignment = pst->m_native.alignment;

							if(!(pst->m_native.flags & rdxCStructuredType::NativeProperties::STF_AllowBulkSerialize))
								bulk = false;
							break;
						}
					}
					else if(pTypeType == m_builtins.st_ArrayOfType)
						isClassRef = true;
					else if(pTypeType == m_builtins.st_DelegateType)
						isClassRef = true;
					else
						RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

					if(isClassRef)
					{
						pSize = sizeof(rdxTracedRTRef(rdxCObject));
						pAlignment = rdxAlignOf(rdxTracedRTRef(rdxCObject));
						
						if(!rdxCheckAddOverflowU(numContainedReferences, 1))
							RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);

						numContainedReferences++;
					}
					else if(isInterfaceRef)
					{
						pSize = sizeof(rdxTracedTypelessIRef);
						pAlignment = rdxAlignOf(rdxTracedTypelessIRef);
						
						if(!rdxCheckAddOverflowU(numContainedReferences, 1))
							RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);

						numContainedReferences++;
					}

					if(st->m_native.getPropertyOffsetFunc)
					{
						rdxLargeUInt propOffset;
						if(!st->m_native.getPropertyOffsetFunc(prop->name.ToWeakRTRef(), &propOffset))
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
						st->m_native.propertyOffsets->Element(i) = propOffset - objectBaseOffset;
					}
					else
					{
						if(pAlignment > currentAlignment)
							currentAlignment = pAlignment;

						// Align
						rdxLargeUInt slippage = currentSize % pAlignment;
						rdxLargeUInt padding = 0;
						if(slippage)
							padding = pAlignment - slippage;
#ifndef RDX_ENABLE_PADDED_BULK_SERIALIZE
						if(padding != 0)
							bulk = false;
#endif
						if(!rdxCheckAddOverflowU(currentSize, padding))
							RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);

						currentSize += padding;


						st->m_native.propertyOffsets->Element(i) = currentSize - objectBaseOffset;

						if(!rdxCheckAddOverflowU(currentSize, pSize))
							RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);

						currentSize += pSize;
					}
				}
			}
		}

		rdxLargeUInt sizeInSubclass;
		if(isNativeType && !(st->m_native.flags & rdxCStructuredType::NativeProperties::STF_InheritedTypeInfo))
		{
			currentSize = nativeTypeInfo.Size();
			currentAlignment = nativeTypeInfo.Alignment();
			sizeInSubclass = nativeTypeInfo.SizeInSubclass();
		}
		else
		{
			sizeInSubclass = currentSize;
			if(currentAlignment == 0)
				currentAlignment = 1;
			if(currentSize == 0)
				currentSize = 1;
		
			// Pad to struct size
			{
				rdxLargeUInt slippage = currentSize % currentAlignment;
				rdxLargeUInt padding = 0;
				if(slippage)
					padding = currentAlignment - slippage;
	#ifndef RDX_ENABLE_PADDED_BULK_SERIALIZE
				if(padding != 0)
					bulk = false;
#endif

				if(!rdxCheckAddOverflowU(currentSize, padding))
					RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);

				currentSize += padding;
			}
			
			// Determine if we're using Itanium ABI.
			// Itanium ABI allows tail padding usage, Microsoft ABI does not.
			{
				struct tailPadCheckA
				{
					virtual void ForceVTable() = 0;
					rdxUInt32 a;
					rdxUInt8 b;
				};
				struct tailPadCheckB : public tailPadCheckA
				{
					rdxUInt8 c;
				};
				if(sizeof(tailPadCheckB) != sizeof(tailPadCheckA))
					sizeInSubclass = currentSize;
			}
		}

		// Map out references
		if(numContainedReferences)
		{
			bulk = false;

			if(numContainedReferences && st->m_native.containedReferences.IsNull())
				RDX_PROTECT_ASSIGN(ctx, st->m_native.containedReferences,
					rdxCInternalObjectFactory::Create1DArray<rdxCStructuredType::NativeProperties::ContainedReference>(ctx, this, numContainedReferences) );

			rdxWeakArrayRTRef(rdxCStructuredType::NativeProperties::ContainedReference) refs = st->m_native.containedReferences.ToWeakRTRef();

			// Copy parent contained references
			for(rdxLargeUInt i=0;i<numParentContainedReferences;i++)
				refs->Element(i) = st->parentClass->m_native.containedReferences->Element(i);

			rdxLargeUInt numRefs = numParentContainedReferences;
			for(rdxLargeUInt i=numParentProperties;i<nProperties;i++)
			{
				rdxLargeUInt pOffset = st->m_native.propertyOffsets->Element(i);

				bool isClassRef = false;
				bool isInterfaceRef = false;
				rdxWeakHdl(rdxCType) pType = st->properties->Element(i).type.ToWeakHdl();
				if(pType.IsNull())
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

				rdxWeakHdl(rdxCType) pTypeType = pType->ObjectInfo()->containerType.ToWeakHdl();
				if(pTypeType == m_builtins.st_StructuredType)
				{
					rdxWeakHdl(rdxCStructuredType) pst = pType.StaticCast<rdxCStructuredType>();
					switch(pst->storageSpecifier)
					{
					case rdxSS_Interface:
						isInterfaceRef = true;
						break;
					case rdxSS_Class:
						isClassRef = true;
						break;
					case rdxSS_Enum:
						break;
					case rdxSS_ValStruct:
					case rdxSS_RefStruct:
						{
							rdxLargeUInt nxRefs = pst->m_native.numContainedReferences;
							if(nxRefs)
							{
								rdxWeakArrayRTRef(rdxCStructuredType::NativeProperties::ContainedReference) xrefs = pst->m_native.containedReferences.ToWeakRTRef();
								for(rdxLargeUInt xi=0;xi<nxRefs;xi++)
								{
									rdxCStructuredType::NativeProperties::ContainedReference cr;
									cr = xrefs->Element(xi);
									cr.offset = xrefs->Element(xi).offset + pOffset;
									refs->Element(numRefs++) = cr;
								}
							}
						}
						break;
					}
				}
				else if(pTypeType == m_builtins.st_DelegateType)
					isClassRef = true;
				else if(pTypeType == m_builtins.st_ArrayOfType)
					isClassRef = true;
				else
					RDX_STHROW(ctx, RDX_ERROR_INTERNAL_BAD_TYPE);

				// TODO MUSTFIX: Interfaces
				if(isClassRef || isInterfaceRef)
				{
					rdxCStructuredType::NativeProperties::ContainedReference cr;
					cr.requiredType = pType;
					cr.offset = pOffset;
					cr.mustBeConstant = (st->properties->Element(i).mustBeConstant != rdxFalseValue);
					cr.isInterface = isInterfaceRef;
					refs->Element(numRefs++) = cr;
				}
			}
		}

		// TODO: Don't allow defaults and lifecycle at the same time
		if(zeroFill && !(st->m_native.defaultValueReference.symbolLoc != rdxPSL_Null))
		{
			st->m_native.currentDefaultValue = rdxWeakHdl(rdxCObject)::Null();
			st->m_native.flags |= (rdxCStructuredType::NativeProperties::STF_ZeroFill |
				rdxCStructuredType::NativeProperties::STF_DependencyDefaultsEvaluated | 
				rdxCStructuredType::NativeProperties::STF_FinalDefaultEvaluated);
		}

		if(bulk)
			st->m_native.flags |= rdxCStructuredType::NativeProperties::STF_AllowBulkSerialize;

		// Done
		st->m_native.numContainedReferences = numContainedReferences;
		st->m_native.size = currentSize;
		st->m_native.sizeInSubclass = sizeInSubclass;
		st->m_native.alignment = currentAlignment;
		st->m_native.flags |= rdxCStructuredType::NativeProperties::STF_StructureEvaluated;

		if(structureIsMutable)
			st->m_native.flags |= rdxCStructuredType::NativeProperties::STF_StructureIsMutable;

		st->m_native.nativeTypeInfo = nativeTypeInfo;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROWV(ctx, false);
	}
	RDX_ENDTRY

	return true;
}

#define INITIALIZE_PROPERTY(propArray, index, propName, propType)	\
RDX_PROTECT_ASSIGN(ctx, m_builtins.propArray->Element(index).name, CreateStringASCII(ctx, propName) );\
m_builtins.propArray->Element(index).type = m_builtins.propType;\
m_builtins.propArray->Element(index).isConstant = rdxTrueValue;\
if(m_builtins.propType->ObjectInfo()->containerType == m_builtins.st_ArrayOfType)\
	m_builtins.propArray->Element(index).mustBeConstant = rdxTrueValue
	
#define ADD_BUILTIN_SYMBOL(domainName, objName, biObj)	\
	RDX_PROTECT(ctx, AddGlobalSymbol(ctx, ComputeObjectGUID(ComputeDomainGUID(domainName), objName), m_builtins.biObj.ToWeakHdl()) )

#define ADD_BUILTIN_SYMBOL_TYPE(domainName, objName, biObj)	\
	/*RDX_PROTECT_ASSIGN(ctx, m_builtins.biObj->typeFullName, CreateStringASCII(ctx, domainName objName));*/	\
	ADD_BUILTIN_SYMBOL(domainName, objName, biObj)

#define INITIALIZE_SIMPLE_NUMERIC_TYPE(builtinName, nativeType, serializer, symbolName)	\
	do {\
		rdxWeakHdl(rdxCStructuredType) objH = m_builtins.builtinName.ToWeakHdl();\
		objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable;\
		objH->storageSpecifier = rdxSS_ValStruct;\
		objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<nativeType>::TypeInfoInterface();\
		objH->m_native.user.flags |= rdxCStructuredType::NativeProperties::UserProperties::STUF_NativeSafeToSerialize;\
		objH->m_native.user.typeSerializer = &serializer;\
		ADD_BUILTIN_SYMBOL_TYPE("Core", symbolName, builtinName);\
	} while(0)

#define INITIALIZE_PROPERTY_LIST(targetBI, numProperties)	\
	RDX_PROTECT_ASSIGN(ctx, m_builtins.targetBI, rdxCInternalObjectFactory::Create1DArray<rdxSProperty>(ctx, this, numProperties, m_builtins.aot_Property.ToWeakHdl(), rdxSDomainGUID::Builtin(rdxDOMAIN_Core)) )

#define INITIALIZE_ENUMERANT(list, index, enumerantName, enumerantValue)	\
	RDX_PROTECT_ASSIGN(ctx, list->Element(index).name, CreateStringASCII(ctx, enumerantName) );	\
	list->Element(index).value = enumerantValue

void rdxCObjectManager::InitializeSTAOT(rdxSOperationContext *ctx, rdxCRef(rdxCArrayOfType) aotRef, const char *name, const rdxCRef(rdxCStructuredType) subtypeRef)
{
	RDX_TRY(ctx)
	{
		aotRef->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable;
		aotRef->numDimensions = 1;
		aotRef->type = subtypeRef;
		aotRef->isConstant = rdxTrueValue;

		RDX_PROTECT(ctx, AddGlobalSymbol(ctx, ComputeObjectGUID(rdxSDomainGUID::Builtin(rdxDOMAIN_Duplicable), name), aotRef.ToWeakHdl()) );
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

void rdxCObjectManager::InitializeBuiltInTypes(rdxSOperationContext *ctx)
{
	RDX_TRY(ctx)
	{
		this->m_initialized = false;

		rdxCRef(rdxCString) str = rdxCRef(rdxCString)::Null();

		rdxSDomainGUID coreGUID = rdxSDomainGUID::Builtin(rdxDOMAIN_Core);
		rdxSDomainGUID duplicableGUID = rdxSDomainGUID::Builtin(rdxDOMAIN_Duplicable);

		{
			RDX_PROTECT_ASSIGN(ctx, m_builtins.st_StructuredType, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, rdxWeakHdl(rdxCStructuredType)::Null(), coreGUID) );
			rdxGCInfo *objI = m_builtins.st_StructuredType->ObjectInfo();

			objI->containerType = m_builtins.st_StructuredType;
			objI->objectFlags = rdxGCInfo::GCOF_Duplicable | rdxGCInfo::GCOF_Critical;
			m_builtins.st_StructuredType->m_native.user.flags |= rdxCStructuredType::NativeProperties::UserProperties::STUF_AllowDuplicates;
			m_builtins.st_StructuredType->isFinal = true;
			m_builtins.st_StructuredType->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxCStructuredType>::TypeInfoInterface();
		}

		rdxWeakHdl(rdxCStructuredType) st_st = m_builtins.st_StructuredType.ToWeakHdl();
		
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_Type, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		st_st->parentClass = m_builtins.st_Type;

		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_ArrayOfType, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_DelegateType, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );

		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_String, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );

		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_Object, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_Array, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_Bool, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_Varying, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_LargeInt, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_LargeUInt, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_Int, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_UInt, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_Short, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_UShort, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_Long, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_ULong, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_Float, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_Double, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_Byte, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_Char, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_HashValue, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_Property, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_MethodParameter, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_Enumerant, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_StorageSpecifier, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_Method, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_Thread, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_InstructionFileInfo, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.st_InterfaceImplementation, rdxCInternalObjectFactory::CreateObject<rdxCStructuredType>(ctx, this, st_st, coreGUID) );
		
		rdxWeakHdl(rdxCStructuredType) st_aot = m_builtins.st_ArrayOfType.ToWeakHdl();

		RDX_PROTECT_ASSIGN(ctx, m_builtins.aot_InterfaceImplementation, CreateArrayType(ctx, m_builtins.st_InterfaceImplementation.ToWeakHdl(), 1, true, rdxSAutoTypeInfo<rdxCArray<rdxSInterfaceImplementation> >::TypeInfoInterface() ));
		RDX_PROTECT_ASSIGN(ctx, m_builtins.aot_StructuredType, CreateArrayType(ctx, m_builtins.st_StructuredType.ToWeakHdl(), 1, true, rdxSAutoTypeInfo<rdxCArray<rdxTracedRTRef(rdxCStructuredType)> >::TypeInfoInterface() ));
		RDX_PROTECT_ASSIGN(ctx, m_builtins.aot_Type, CreateArrayType(ctx, m_builtins.st_Type.ToWeakHdl(), 1, true, rdxSAutoTypeInfo<rdxCArray<rdxTracedRTRef(rdxCType) > >::TypeInfoInterface() ));
		RDX_PROTECT_ASSIGN(ctx, m_builtins.aot_Enumerant, CreateArrayType(ctx, m_builtins.st_Enumerant.ToWeakHdl(), 1, true, rdxSAutoTypeInfo<rdxCArray<rdxSEnumerant> >::TypeInfoInterface() ) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.aot_MethodParameter, CreateArrayType(ctx, m_builtins.st_MethodParameter.ToWeakHdl(), 1, true, rdxSAutoTypeInfo<rdxCArray<rdxSMethodParameter> >::TypeInfoInterface() ) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.aot_Property, CreateArrayType(ctx, m_builtins.st_Property.ToWeakHdl(), 1, true, rdxSAutoTypeInfo<rdxCArray<rdxSProperty> >::TypeInfoInterface() ) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.aot_Method, CreateArrayType(ctx, m_builtins.st_Method.ToWeakHdl(), 1, true, rdxSAutoTypeInfo<rdxCArray<rdxTracedRTRef(rdxCMethod)> >::TypeInfoInterface() ) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.aot_String, CreateArrayType(ctx, m_builtins.st_String.ToWeakHdl(), 1, true, rdxSAutoTypeInfo<rdxCArray<rdxTracedRTRef(rdxCString)> >::TypeInfoInterface() ) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.aot_Int, CreateArrayType(ctx, m_builtins.st_Int.ToWeakHdl(), 1, true, rdxSAutoTypeInfo<rdxCArray<rdxInt> >::TypeInfoInterface() ) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.aot_UInt, CreateArrayType(ctx, m_builtins.st_UInt.ToWeakHdl(), 1, true, rdxSAutoTypeInfo<rdxCArray<rdxUInt> >::TypeInfoInterface() ) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.aot_Char, CreateArrayType(ctx, m_builtins.st_Char.ToWeakHdl(), 1, true, rdxSAutoTypeInfo<rdxCArray<rdxChar> >::TypeInfoInterface() ) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.aot_Byte, CreateArrayType(ctx, m_builtins.st_Byte.ToWeakHdl(), 1, true, rdxSAutoTypeInfo<rdxCArray<rdxByte> >::TypeInfoInterface() ) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.aot_Object, CreateArrayType(ctx, m_builtins.st_Object.ToWeakHdl(), 1, true, rdxSAutoTypeInfo<rdxCArray<rdxTracedRTRef(rdxCObject)> >::TypeInfoInterface() ) );
		RDX_PROTECT_ASSIGN(ctx, m_builtins.aot_InstructionFileInfo, CreateArrayType(ctx, m_builtins.st_InstructionFileInfo.ToWeakHdl(), 1, true, rdxSAutoTypeInfo<rdxCArray<rdxSInstructionFileInfo> >::TypeInfoInterface() ) );

		// String needs to be set up immediately so StringProcessor is properly assigned for all future strings
		{
			rdxWeakHdl(rdxCStructuredType) objH = m_builtins.st_String.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable;
			objH->parentClass = m_builtins.st_Object;
			objH->storageSpecifier = rdxSS_Class;
			objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxCString>::TypeInfoInterface();
			objH->m_native.user.flags |= rdxCStructuredType::NativeProperties::UserProperties::STUF_SerializeAsReference;
			m_builtins.st_String->isFinal = rdxTrueValue;
		}

		// Its dependent char types need to be set up as well so that new strings have the correct type for the char array
		{
			rdxWeakHdl(rdxCStructuredType) objH = m_builtins.st_Char.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable;
			objH->storageSpecifier = rdxSS_ValStruct;
			objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxChar>::TypeInfoInterface();
			objH->m_native.user.flags |= rdxCStructuredType::NativeProperties::UserProperties::STUF_NativeSafeToSerialize;
		}

		{
			rdxWeakHdl(rdxCArrayOfType) objH = m_builtins.aot_Char.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable;
			objH->numDimensions = 1;
			objH->type = m_builtins.st_Char;
			objH->isConstant = rdxTrueValue;
		}

		// Set this as the char array type for the string table
		m_stringTable.m_aot_ConstChar = m_builtins.aot_Char.ToWeakHdl();

		// ****************************************************
		// Strings can be used from this point forward
		ADD_BUILTIN_SYMBOL_TYPE("Core", "char", st_Char);
		ADD_BUILTIN_SYMBOL_TYPE("Core", "int", st_Int);
		ADD_BUILTIN_SYMBOL_TYPE("Core", "uint", st_UInt);
		ADD_BUILTIN_SYMBOL_TYPE("Core", "string", st_String);

		// Set values
		{
			rdxWeakHdl(rdxCStructuredType) objH = m_builtins.st_Object.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable;
			objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxCObject>::TypeInfoInterface();
			objH->storageSpecifier = rdxSS_Class;
			ADD_BUILTIN_SYMBOL_TYPE("Core", "Object", st_Object);
		}

		{
			rdxWeakHdl(rdxCStructuredType) objH = m_builtins.st_Array.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable;
			objH->storageSpecifier = rdxSS_Class;
			objH->parentClass = m_builtins.st_Object;
			objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxCArrayContainer>::TypeInfoInterface();
			objH->isAbstract = rdxTrueValue;
			objH->isLocalized = rdxTrueValue;
			// Arrays are fake, they have no type info
			ADD_BUILTIN_SYMBOL_TYPE("Core", "Array", st_Array);
		}

		{
			rdxWeakHdl(rdxCStructuredType) objH = m_builtins.st_ArrayOfType.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable | rdxGCInfo::GCOF_Critical;
			objH->storageSpecifier = rdxSS_Class;
			objH->parentClass = m_builtins.st_Type;
			objH->isFinal = rdxTrueValue;
			objH->m_native.user.flags = rdxCStructuredType::NativeProperties::UserProperties::STUF_AllowDuplicates;
			objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxCArrayOfType>::TypeInfoInterface();
			objH->m_native.getPropertyOffsetFunc = rdxCArrayOfType::GetPropertyOffset;
			ADD_BUILTIN_SYMBOL_TYPE("Core", "RDX.ArrayOfType", st_ArrayOfType);
		}

		{
			rdxWeakHdl(rdxCStructuredType) objH = m_builtins.st_DelegateType.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable | rdxGCInfo::GCOF_Critical;
			objH->storageSpecifier = rdxSS_Class;
			objH->parentClass = m_builtins.st_Type;
			objH->isFinal = rdxTrueValue;
			objH->m_native.user.flags = rdxCStructuredType::NativeProperties::UserProperties::STUF_AllowDuplicates;
			objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxCDelegateType>::TypeInfoInterface();
			objH->m_native.getPropertyOffsetFunc = rdxCDelegateType::GetPropertyOffset;
			ADD_BUILTIN_SYMBOL_TYPE("Core", "RDX.DelegateType", st_DelegateType);
		}

		{
			rdxWeakHdl(rdxCStructuredType) objH = m_builtins.st_Enumerant.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable;
			objH->storageSpecifier = rdxSS_RefStruct;
			objH->m_native.user.flags = rdxCStructuredType::NativeProperties::UserProperties::STUF_AllowDuplicates;
			objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxSEnumerant>::TypeInfoInterface();
			objH->m_native.getPropertyOffsetFunc = rdxSEnumerant::GetPropertyOffset;
			ADD_BUILTIN_SYMBOL_TYPE("Core", "RDX.Enumerant", st_Enumerant);
		}

		{
			rdxWeakHdl(rdxCStructuredType) objH = m_builtins.st_Type.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable | rdxGCInfo::GCOF_Critical;
			objH->parentClass = m_builtins.st_Object;
			objH->storageSpecifier = rdxSS_Class;
			objH->isAbstract = rdxTrueValue;
			objH->isLocalized = rdxTrueValue;
			objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxCType>::TypeInfoInterface();
			objH->m_native.getPropertyOffsetFunc = rdxCType::GetPropertyOffset;
			ADD_BUILTIN_SYMBOL_TYPE("Core", "RDX.Type", st_Type);
		}
		
		{
			rdxWeakHdl(rdxCStructuredType) objH = m_builtins.st_Bool.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable;
			objH->storageSpecifier = rdxSS_ValStruct;
			objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxBool>::TypeInfoInterface();
			objH->m_native.user.typeSerializer = &rdxCObjectManager::BoolSerializer::instance;
#ifdef RDX_ENABLE_BOOL_BULK_SERIALIZE
			objH->m_native.user.flags |= rdxCStructuredType::NativeProperties::UserProperties::STUF_NativeSafeToSerialize;
#endif
			ADD_BUILTIN_SYMBOL_TYPE("Core", "bool", st_Bool);
		}
		
		{
			rdxWeakHdl(rdxCStructuredType) objH = m_builtins.st_Varying.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable;
			objH->storageSpecifier = rdxSS_ValStruct;
			objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxSVarying>::TypeInfoInterface();
			ADD_BUILTIN_SYMBOL_TYPE("Core", "varying", st_Varying);
		}

		{
			rdxWeakHdl(rdxCStructuredType) objH = m_builtins.st_LargeInt.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable;
			objH->storageSpecifier = rdxSS_ValStruct;
			objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxLargeInt>::TypeInfoInterface();
			objH->m_native.user.typeSerializer = &rdxLargeIntSerializer;
#ifdef RDX_ENABLE_LARGEINT_BULK_SERIALIZE
			objH->m_native.user.flags |= rdxCStructuredType::NativeProperties::UserProperties::STUF_NativeSafeToSerialize;
#endif
			ADD_BUILTIN_SYMBOL_TYPE("Core", "largeint", st_LargeInt);
		}

		{
			rdxWeakHdl(rdxCStructuredType) objH = m_builtins.st_LargeUInt.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable;
			objH->storageSpecifier = rdxSS_ValStruct;
			objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxLargeUInt>::TypeInfoInterface();
			objH->m_native.user.typeSerializer = &rdxLargeUIntSerializer;
#ifdef RDX_ENABLE_LARGEINT_BULK_SERIALIZE
			objH->m_native.user.flags |= rdxCStructuredType::NativeProperties::UserProperties::STUF_NativeSafeToSerialize;
#endif
			ADD_BUILTIN_SYMBOL_TYPE("Core", "largeuint", st_LargeUInt);
		}
		
		INITIALIZE_SIMPLE_NUMERIC_TYPE(st_Int, rdxInt, rdxIntSerializer, "int");
		INITIALIZE_SIMPLE_NUMERIC_TYPE(st_UInt, rdxUInt, rdxUIntSerializer, "uint");
		INITIALIZE_SIMPLE_NUMERIC_TYPE(st_Short, rdxShort, rdxShortSerializer, "short");
		INITIALIZE_SIMPLE_NUMERIC_TYPE(st_UShort, rdxUShort, rdxUShortSerializer, "ushort");
		INITIALIZE_SIMPLE_NUMERIC_TYPE(st_Long, rdxLong, rdxLongSerializer, "long");
		INITIALIZE_SIMPLE_NUMERIC_TYPE(st_ULong, rdxULong, rdxULongSerializer, "ulong");
		INITIALIZE_SIMPLE_NUMERIC_TYPE(st_Float, rdxFloat, rdxFloatSerializer, "float");
		INITIALIZE_SIMPLE_NUMERIC_TYPE(st_Double, rdxDouble, rdxDoubleSerializer, "double");
		INITIALIZE_SIMPLE_NUMERIC_TYPE(st_Byte, rdxByte, rdxByteSerializer, "byte");
		INITIALIZE_SIMPLE_NUMERIC_TYPE(st_HashValue, rdxHashValue, rdxHashValueSerializer, "hashcode");
		
		{
			rdxWeakHdl(rdxCStructuredType) objH = m_builtins.st_InstructionFileInfo.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable;
			objH->storageSpecifier = rdxSS_RefStruct;
			objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxSInstructionFileInfo>::TypeInfoInterface();
			objH->m_native.getPropertyOffsetFunc = rdxSInstructionFileInfo::GetPropertyOffset;
			objH->m_native.user.flags = rdxCStructuredType::NativeProperties::UserProperties::STUF_AllowDuplicates;
			ADD_BUILTIN_SYMBOL_TYPE("Core", "RDX.InstructionFileInfo", st_InstructionFileInfo);
		}

		{
			rdxWeakHdl(rdxCStructuredType) objH = m_builtins.st_Thread.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable;
			objH->storageSpecifier = rdxSS_Class;
			objH->parentClass = m_builtins.st_Object;
			objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxCRuntimeThread>::TypeInfoInterface();
			objH->isFinal = rdxTrueValue;
			ADD_BUILTIN_SYMBOL_TYPE("Core", "RDX.Thread", st_Thread);
		}
		
		{
			rdxWeakHdl(rdxCStructuredType) objH = m_builtins.st_Method.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable;
			objH->storageSpecifier = rdxSS_Class;
			objH->parentClass = m_builtins.st_Object;
			objH->isFinal = rdxTrueValue;
			objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxCMethod>::TypeInfoInterface();
			objH->m_native.getPropertyOffsetFunc = rdxCMethod::GetPropertyOffset;
			objH->m_native.user.flags = rdxCStructuredType::NativeProperties::UserProperties::STUF_AllowDuplicates;
			ADD_BUILTIN_SYMBOL_TYPE("Core", "RDX.Method", st_Method);
		}

		{
			rdxWeakHdl(rdxCStructuredType) objH = m_builtins.st_InterfaceImplementation.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable;
			objH->storageSpecifier = rdxSS_RefStruct;
			objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxSInterfaceImplementation>::TypeInfoInterface();
			objH->m_native.getPropertyOffsetFunc = rdxSInterfaceImplementation::GetPropertyOffset;
			objH->m_native.user.flags = rdxCStructuredType::NativeProperties::UserProperties::STUF_AllowDuplicates;
			ADD_BUILTIN_SYMBOL_TYPE("Core", "RDX.InterfaceImplementation", st_InterfaceImplementation);
		}
		
		{
			rdxWeakHdl(rdxCStructuredType) objH = m_builtins.st_Property.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable;
			objH->storageSpecifier = rdxSS_RefStruct;
			objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxSProperty>::TypeInfoInterface();
			objH->m_native.getPropertyOffsetFunc = rdxSProperty::GetPropertyOffset;
			objH->m_native.user.flags = rdxCStructuredType::NativeProperties::UserProperties::STUF_AllowDuplicates;
			ADD_BUILTIN_SYMBOL_TYPE("Core", "RDX.Property", st_Property);
		}
		
		{
			rdxWeakHdl(rdxCStructuredType) objH = m_builtins.st_MethodParameter.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable;
			objH->storageSpecifier = rdxSS_RefStruct;
			objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxSMethodParameter>::TypeInfoInterface();
			objH->m_native.getPropertyOffsetFunc = rdxSMethodParameter::GetPropertyOffset;
			objH->m_native.user.flags |= rdxCStructuredType::NativeProperties::UserProperties::STUF_AllowDuplicates;
			ADD_BUILTIN_SYMBOL_TYPE("Core", "RDX.MethodParameter", st_MethodParameter);
		}

		m_builtins.aot_Enumerant->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Critical;
		m_builtins.aot_Property->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Critical;
		m_builtins.aot_Method->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_CriticalBorder;
		m_builtins.aot_MethodParameter->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Critical;	// TODO: ???
		m_builtins.aot_InterfaceImplementation->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Critical;

		{
			rdxWeakHdl(rdxCStructuredType) objH = m_builtins.st_StructuredType.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable | rdxGCInfo::GCOF_Critical;
			objH->parentClass = m_builtins.st_Type;
			objH->storageSpecifier = rdxSS_Class;
			objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxCStructuredType>::TypeInfoInterface();
			objH->m_native.getPropertyOffsetFunc = rdxCStructuredType::GetPropertyOffset;
			objH->m_native.user.flags |= rdxCStructuredType::NativeProperties::UserProperties::STUF_AllowDuplicates;
			objH->m_native.user.typeSerializer = &rdxCStructuredTypeSerializer::instance;
			ADD_BUILTIN_SYMBOL_TYPE("Core", "RDX.StructuredType", st_StructuredType);
		}

		RDX_PROTECT_ASSIGN(ctx, m_builtins.e_StorageSpecifierEnumerants, rdxCInternalObjectFactory::Create1DArray<rdxSEnumerant>(ctx, this, 5, m_builtins.aot_Enumerant.ToWeakHdl(), rdxSDomainGUID::Builtin(rdxDOMAIN_Core)) );

		INITIALIZE_PROPERTY_LIST(p_EnumerantProperties, 2);
		//INITIALIZE_PROPERTY_LIST(p_MethodParameterProperties, 3);

		RDX_PROTECT_ASSIGN(ctx, m_builtins.p_MethodParameterProperties, rdxCInternalObjectFactory::Create1DArray<rdxSProperty>(ctx, this, 3, m_builtins.aot_Property.ToWeakHdl(), rdxSDomainGUID::Builtin(rdxDOMAIN_Core)) );

		INITIALIZE_PROPERTY_LIST(p_PropertyProperties, 4);
		INITIALIZE_PROPERTY_LIST(p_ArrayOfTypeProperties, 3);
		INITIALIZE_PROPERTY_LIST(p_DelegateTypeProperties, 2);
		INITIALIZE_PROPERTY_LIST(p_StructuredTypeProperties, 9);
		//INITIALIZE_PROPERTY_LIST(p_TypeProperties, 0);
		INITIALIZE_PROPERTY_LIST(p_MethodProperties, 9);
		INITIALIZE_PROPERTY_LIST(p_InstructionFileInfoProperties, 3);
		INITIALIZE_PROPERTY_LIST(p_InterfaceImplementationProperties, 2);

		{
			m_builtins.e_StorageSpecifierEnumerants->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Duplicable;
			rdxWeakArrayHdl(rdxSEnumerant) enumerants = m_builtins.e_StorageSpecifierEnumerants.ToWeakHdl();

			INITIALIZE_ENUMERANT(enumerants, 0, "SS_Class", rdxSS_Class);
			INITIALIZE_ENUMERANT(enumerants, 1, "SS_RefStruct", rdxSS_RefStruct);
			INITIALIZE_ENUMERANT(enumerants, 2, "SS_ValStruct", rdxSS_ValStruct);
			INITIALIZE_ENUMERANT(enumerants, 3, "SS_Enum", rdxSS_Enum);
			INITIALIZE_ENUMERANT(enumerants, 4, "SS_Interface", rdxSS_Interface);
			ADD_BUILTIN_SYMBOL("Core", "RDX.StorageSpecifier/enumerants", e_StorageSpecifierEnumerants);
		}

		{
			rdxWeakHdl(rdxCStructuredType) objH = m_builtins.st_StorageSpecifier.ToWeakHdl();
			objH->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Core | rdxGCInfo::GCOF_Duplicable;
			objH->storageSpecifier = rdxSS_Enum;
			objH->enumerants = m_builtins.e_StorageSpecifierEnumerants;
			objH->m_native.nativeTypeInfo = rdxSAutoTypeInfo<rdxEnumValue>::TypeInfoInterface();
			ADD_BUILTIN_SYMBOL_TYPE("Core", "RDX.StorageSpecifier", st_StorageSpecifier);
		}

		m_builtins.p_ArrayOfTypeProperties->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Duplicable;
		INITIALIZE_PROPERTY(p_ArrayOfTypeProperties, 0, "type", st_Type);
		INITIALIZE_PROPERTY(p_ArrayOfTypeProperties, 1, "numDimensions", st_LargeUInt);
		INITIALIZE_PROPERTY(p_ArrayOfTypeProperties, 2, "isConstant", st_Bool);
		ADD_BUILTIN_SYMBOL("Core", "RDX.ArrayOfType/properties", p_ArrayOfTypeProperties);
		
		m_builtins.p_DelegateTypeProperties->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Duplicable;
		INITIALIZE_PROPERTY(p_DelegateTypeProperties, 0, "parameters", aot_MethodParameter);
		INITIALIZE_PROPERTY(p_DelegateTypeProperties, 1, "returnTypes", aot_Type);
		ADD_BUILTIN_SYMBOL("Core", "RDX.DelegateType/properties", p_DelegateTypeProperties);
		
		m_builtins.p_MethodParameterProperties->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Duplicable;
		INITIALIZE_PROPERTY(p_MethodParameterProperties, 0, "type", st_Type);
		INITIALIZE_PROPERTY(p_MethodParameterProperties, 1, "isConstant", st_Bool);
		INITIALIZE_PROPERTY(p_MethodParameterProperties, 2, "isNotNull", st_Bool);
		ADD_BUILTIN_SYMBOL("Core", "RDX.MethodParameter/properties", p_MethodParameterProperties);
		
		m_builtins.p_PropertyProperties->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Duplicable;
		INITIALIZE_PROPERTY(p_PropertyProperties, 0, "name", st_String);
		INITIALIZE_PROPERTY(p_PropertyProperties, 1, "type", st_Type);
		INITIALIZE_PROPERTY(p_PropertyProperties, 2, "isConstant", st_Bool);
		INITIALIZE_PROPERTY(p_PropertyProperties, 3, "mustBeConstant", st_Bool);
		ADD_BUILTIN_SYMBOL("Core", "RDX.Property/properties", p_PropertyProperties);
		
		m_builtins.p_InterfaceImplementationProperties->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Duplicable;
		INITIALIZE_PROPERTY(p_InterfaceImplementationProperties, 0, "type", st_StructuredType);
		INITIALIZE_PROPERTY(p_InterfaceImplementationProperties, 1, "vftOffset", st_UInt);
		ADD_BUILTIN_SYMBOL("Core", "RDX.InterfaceImplementation/properties", p_InterfaceImplementationProperties);
		
		m_builtins.p_InstructionFileInfoProperties->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Duplicable;
		INITIALIZE_PROPERTY(p_InstructionFileInfoProperties, 0, "filename", st_String);
		INITIALIZE_PROPERTY(p_InstructionFileInfoProperties, 1, "line", st_UInt);
		INITIALIZE_PROPERTY(p_InstructionFileInfoProperties, 2, "firstInstruction", st_UInt);
		ADD_BUILTIN_SYMBOL("Core", "RDX.InstructionFileInfo/properties", p_InstructionFileInfoProperties);
		
		m_builtins.p_MethodProperties->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Duplicable;
		INITIALIZE_PROPERTY(p_MethodProperties, 0, "parameters", aot_MethodParameter);
		INITIALIZE_PROPERTY(p_MethodProperties, 1, "returnTypes", aot_Type);
		INITIALIZE_PROPERTY(p_MethodProperties, 2, "bytecode", aot_Byte);
		INITIALIZE_PROPERTY(p_MethodProperties, 3, "resArgs", aot_Object);
		INITIALIZE_PROPERTY(p_MethodProperties, 4, "instructionFileInfos", aot_InstructionFileInfo);
		INITIALIZE_PROPERTY(p_MethodProperties, 5, "numInstructions", st_UInt);
		INITIALIZE_PROPERTY(p_MethodProperties, 6, "vftIndex", st_UInt);
		INITIALIZE_PROPERTY(p_MethodProperties, 7, "thisParameterOffset", st_UInt);
		INITIALIZE_PROPERTY(p_MethodProperties, 8, "isAbstract", st_Bool);
		ADD_BUILTIN_SYMBOL("Core", "RDX.Method/properties", p_MethodProperties);
		
		m_builtins.p_EnumerantProperties->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Duplicable;
		INITIALIZE_PROPERTY(p_EnumerantProperties, 0, "name", st_String);
		INITIALIZE_PROPERTY(p_EnumerantProperties, 1, "value", RDX_ENUM_INTERNAL_TYPE);
		ADD_BUILTIN_SYMBOL("Core", "RDX.Enumerant/properties", p_EnumerantProperties);
		
		m_builtins.p_StructuredTypeProperties->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Duplicable;
		INITIALIZE_PROPERTY(p_StructuredTypeProperties, 0, "parentClass", st_StructuredType);
		INITIALIZE_PROPERTY(p_StructuredTypeProperties, 1, "interfaces", aot_InterfaceImplementation);
		INITIALIZE_PROPERTY(p_StructuredTypeProperties, 2, "storageSpecifier", st_StorageSpecifier);
		INITIALIZE_PROPERTY(p_StructuredTypeProperties, 3, "virtualMethods", aot_Method);
		INITIALIZE_PROPERTY(p_StructuredTypeProperties, 4, "properties", aot_Property);
		INITIALIZE_PROPERTY(p_StructuredTypeProperties, 5, "enumerants", aot_Enumerant);
		INITIALIZE_PROPERTY(p_StructuredTypeProperties, 6, "isFinal", st_Bool);
		INITIALIZE_PROPERTY(p_StructuredTypeProperties, 7, "isAbstract", st_Bool);
		INITIALIZE_PROPERTY(p_StructuredTypeProperties, 8, "isLocalized", st_Bool);
		ADD_BUILTIN_SYMBOL("Core", "RDX.StructuredType/properties", p_StructuredTypeProperties);

		// Assign enumerants and properties
		m_builtins.st_StorageSpecifier->enumerants = m_builtins.e_StorageSpecifierEnumerants;

		m_builtins.st_Enumerant->properties = m_builtins.p_EnumerantProperties;
		m_builtins.st_MethodParameter->properties = m_builtins.p_MethodParameterProperties;
		m_builtins.st_Property->properties = m_builtins.p_PropertyProperties;
		m_builtins.st_ArrayOfType->properties = m_builtins.p_ArrayOfTypeProperties;
		m_builtins.st_DelegateType->properties = m_builtins.p_DelegateTypeProperties;
		m_builtins.st_StructuredType->properties = m_builtins.p_StructuredTypeProperties;
		m_builtins.st_Method->properties = m_builtins.p_MethodProperties;
		m_builtins.st_InstructionFileInfo->properties = m_builtins.p_InstructionFileInfoProperties;
		m_builtins.st_InterfaceImplementation->properties = m_builtins.p_InterfaceImplementationProperties;

		// Resolve built-in structures
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_Object.ToWeakHdl()) );

		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_Type.ToWeakHdl()) );

		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_Bool.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_Varying.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_LargeInt.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_LargeUInt.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_Int.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_UInt.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_Short.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_UShort.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_Long.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_ULong.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_Float.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_Double.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_Char.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_Byte.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_HashValue.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_String.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_MethodParameter.ToWeakHdl()) );

		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_ArrayOfType.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_StructuredType.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_DelegateType.ToWeakHdl()) );

		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_Property.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_Enumerant.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_StorageSpecifier.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_Method.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_InstructionFileInfo.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_InterfaceImplementation.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_Thread.ToWeakHdl()) );
		RDX_PROTECT(ctx, this->ResolveStructure(ctx, m_builtins.st_Array.ToWeakHdl()) );

		this->m_initialized = true;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

// Returns what the size of a single element would be if created as an instance of the specified type.
// For arrays, this is the size of an object reference.  For objects, this is the size of the contained structure.
rdxLargeUInt rdxCObjectManager::TypeElementSize(rdxWeakRTRef(rdxCType) t) const
{
	if(t.IsNull())
		return 0;

	rdxWeakHdl(rdxCType) subType;
	if(t->ObjectInfo()->containerType == m_builtins.st_StructuredType)
		return t.StaticCast<rdxCStructuredType>()->m_native.size;
	else if(t->ObjectInfo()->containerType == m_builtins.st_ArrayOfType)
	{
		subType = t.StaticCast<rdxCArrayOfType>()->type;
		rdxWeakHdl(rdxCType) subTypeContainerType = subType->ObjectInfo()->containerType.ToWeakHdl();

		if(subTypeContainerType == m_builtins.st_StructuredType)
		{
			rdxWeakHdl(rdxCStructuredType) st = subType.StaticCast<rdxCStructuredType>();
			switch(st->storageSpecifier)
			{
			case rdxSS_Enum:
				return sizeof(rdxEnumValue);
			case rdxSS_Class:
				return sizeof(rdxTracedRTRef(rdxCObject));
			case rdxSS_Interface:
				return sizeof(rdxTracedTypelessIRef);
			case rdxSS_RefStruct:
			case rdxSS_ValStruct:
				return st->m_native.size;
				break;
			default:
				return 0;
			};
		}
		else if(subTypeContainerType == m_builtins.st_ArrayOfType)
		{
			return sizeof(rdxTracedRTRef(rdxCObject));
		}
		else if(subTypeContainerType == m_builtins.st_DelegateType)
		{
			return sizeof(rdxTracedRTRef(rdxCObject));
		}
		else
			return 0;
	}
	else	// Can't create delegates, or anything else
		return 0;
}

void rdxCObjectManager::InitializeObject(void *objectHead, rdxWeakRTRef(rdxCType) type, bool forceZeroFill)
{
	rdxWeakRTRef(rdxCType) typeOfType = type->ObjectInfo()->containerType.ToWeakRTRef();
	if(typeOfType == m_builtins.st_ArrayOfType)
	{
		rdxWeakRTRef(rdxCArrayOfType) arrayType = type.StaticCast<rdxCArrayOfType>();
		rdxIfcTypeFuncs typeFuncs = arrayType->m_native.arrayTypeInfo.TypeFuncs();
		//typeFuncs.GetCreateFunc()(objectHead, this);
		static_cast<rdxCArrayContainer*>(objectHead)->InitializeContents(this, forceZeroFill, arrayType->type.ToWeakRTRef());
	}
	else if(typeOfType == m_builtins.st_StructuredType)
	{
		rdxWeakRTRef(rdxCStructuredType) st = type.StaticCast<rdxCStructuredType>();
		rdxIfcTypeInfo typeInfo = st->m_native.nativeTypeInfo;

		switch(st->storageSpecifier)
		{
		case rdxSS_Class:
			{
				rdxIfcTypeFuncs typeFuncs = typeInfo.TypeFuncs();

				if(st->m_native.currentDefaultValue.IsNotNull() && (st->m_native.flags & rdxCStructuredType::NativeProperties::STF_FinalDefaultEvaluated))
				{
					const void *defaultHead = typeFuncs.GetRefToObjectHeadFunc()(st->m_native.currentDefaultValue.Modify());
					typeFuncs.GetAssignFunc()(objectHead, defaultHead);

					// Copy extension
					if(typeInfo.Size() < st->m_native.size)
						rdxBlockCopy(static_cast<rdxUInt8*>(objectHead) + typeInfo.Size(), static_cast<const rdxUInt8*>(defaultHead) + typeInfo.Size(), st->m_native.size - typeInfo.Size());
				}
				else
				{
					// TODO MUSTFIX: Was this already done?  Need to call constructor.
					memset(objectHead, 0, st->m_native.size);
					//typeFuncs.GetCreateFunc()(objectHead, this);
				}
			}
			break;
		case rdxSS_RefStruct:
		case rdxSS_ValStruct:
		case rdxSS_Enum:
			{
				rdxIfcTypeFuncs typeFuncs = typeInfo.TypeFuncs();
				//typeFuncs.GetCreateFunc()(objectHead, this);
				static_cast<rdxCStructContainer*>(objectHead)->InitializeContents(this, st);
			}
			break;
		};
	}
}

static int numStringAllocs = 0;

rdxCRef(rdxCObject) rdxCObjectManager::CreateObjectContainer(rdxSOperationContext *ctx, rdxLargeUInt containerSize,
	rdxSDomainGUID domain, rdxWeakHdl(rdxCType) t, rdxIfcTypeInfo typeInfo)
{
	bool hasSerializationTag = domain != rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime);
	rdxLargeUInt serTagOffset = 0;
	
	if(hasSerializationTag)
	{
		bool overflowed;
		containerSize = rdxPaddedSize(containerSize, rdxAlignOf(rdxSSerializationTag), overflowed);
		if(overflowed)
			RDX_LTHROWV(ctx, RDX_ERROR_INTEGER_OVERFLOW, rdxCRef(rdxCString)::Null());
		serTagOffset = containerSize;
		containerSize -= rdxAlignOf(rdxSSerializationTag);
		if(!rdxCheckAddOverflowU(containerSize, sizeof(rdxSSerializationTag)))
			RDX_LTHROWV(ctx, RDX_ERROR_INTEGER_OVERFLOW, rdxCRef(rdxCString)::Null());
		containerSize += sizeof(rdxSSerializationTag);
	}

	bool collected = false;
	rdxAtomicUInt::ContainedType cycle = m_gcCounter.IncrementFullFence();

	if(cycle % this->m_gcFrequency == 0 && m_initialized)
	{
		collected = true;
		this->CollectGarbage(ctx);
	}

	rdxGCInfo *gci = static_cast<rdxGCInfo *>(m_allocator.Realloc(NULL, sizeof(rdxGCInfo), rdxAlignOf(rdxGCInfo)));
	void *dataBytes = m_allocator.Realloc(NULL, containerSize, RDX_MAX_ALIGNMENT);
	if(gci == NULL || dataBytes == NULL)
	{
		if(!m_initialized)
		{
			if(dataBytes)
				m_allocator.Realloc(dataBytes, 0, RDX_MAX_ALIGNMENT);
			if(gci)
				m_allocator.Realloc(gci, 0, rdxAlignOf(rdxGCInfo));
			RDX_LTHROWV(ctx, RDX_ERROR_ALLOCATION_FAILED, rdxCRef(rdxCObject)::Null());
		}

		// Failed allocation, try collecting garbage and allocating again
		if(!collected && m_initialized)
		{
			collected = true;
			this->CollectGarbage(ctx);
			if(gci == NULL)
				gci = static_cast<rdxGCInfo *>(m_allocator.Realloc(NULL, sizeof(rdxGCInfo), rdxAlignOf(rdxGCInfo)));
			if(dataBytes == NULL)
				dataBytes = m_allocator.Realloc(NULL, containerSize, RDX_MAX_ALIGNMENT);
		}
		if(gci == NULL || dataBytes == NULL)
		{
			if(dataBytes)
				m_allocator.Realloc(dataBytes, 0, RDX_MAX_ALIGNMENT);
			if(gci)
				m_allocator.Realloc(gci, 0, rdxAlignOf(rdxGCInfo));

			RDX_LTHROWV(ctx, RDX_ERROR_ALLOCATION_FAILED, rdxCRef(rdxCObject)::Null());
		}
	}
	
	// Zero-fill the entire thing
	memset(dataBytes, 0, containerSize);

	// Create object
	rdxIfcTypeFuncs typeFuncs = typeInfo.TypeFuncs();
	typeFuncs.GetCreateFunc()(dataBytes, this, gci);

	rdxCObject *obj = typeFuncs.GetObjectHeadToRefFunc()(dataBytes);

	// Convert serTagOffset to CObject relative
	serTagOffset = static_cast<rdxLargeUInt>(static_cast<const rdxUInt8*>(dataBytes) + serTagOffset - reinterpret_cast<const rdxUInt8*>(obj));

	new (gci) rdxGCInfo(this, t, typeInfo, hasSerializationTag, serTagOffset, obj);

	if (rdxSSerializationTag *serializationTag = gci->SerializationTag())
	{
		serializationTag->isAnonymous = true;
		serializationTag->gstSymbol = rdxSObjectGUID::Invalid();
		serializationTag->gstSymbol.m_domain = domain;
		serializationTag->package = rdxTracedRTRef(rdxCPackage)::Null();
		serializationTag->packageManifestOffset = 0;
	}

	gci->objectFlags = 0;

#ifdef RDX_ENABLE_SENTINELS
	static UInt32 crudeSequentialID = 0;

	gci->sequentialID = crudeSequentialID++;
	gci->sentinel1 = 0xdeadbeef;
	gci->sentinel2 = reinterpret_cast<UInt32*>(reinterpret_cast<char*>(gci) + sz);
	*gci->sentinel2 = 0xdeadbeef;
#endif

	// TODO MUSTFIX:
	// ConstantArray
	// ReferenceArray
	// InitializeObject

#ifdef RDX_ENABLE_SMP
	m_coreMutexes[rdxCOREMUTEX_ObjectList]->Acquire();
#endif

	gci->LinkBefore(&m_liveRoot, rdxGCL_LiveObjects);

	if(m_garbageCollector.IsActive())
		gci->LinkBefore(&m_markedRoot, rdxGCL_ScanLink);

	rdxUInt32 typeFlags = typeInfo.Flags();
	if(typeFlags & rdxETIF_BoxFlag)
		gci->objectFlags |= rdxGCInfo::GCOF_Box;
	if(typeFlags & rdxETIF_ArrayFlag)
	{
		gci->objectFlags |= rdxGCInfo::GCOF_Array;
		if(t.IsNotNull() && t.StaticCast<rdxCArrayOfType>()->isConstant)
			gci->objectFlags |= rdxGCInfo::GCOF_ConstantArray;
	}

#ifdef RDX_ENABLE_SMP
	m_coreMutexes[rdxCOREMUTEX_ObjectList]->Release();
#endif

	return gci->ObjectCRef();
}

rdxCRef(rdxCString) rdxCObjectManager::CreateStringFromCS(rdxSOperationContext *ctx, rdxSCandidateString *cs, bool lookupOnly)
{
	rdxCRef(rdxCString) strRef;
	rdxHashValue hash;
	rdxWeakHdl(rdxCString) outStrPtr;

#ifdef RDX_ENABLE_SMP
	m_coreMutexes[rdxCOREMUTEX_StringTableInsert]->Acquire();
#endif
	bool found = m_stringTable.GetElement(cs, &outStrPtr, &hash);

	if(!found)
	{
		if(lookupOnly)
		{
#ifdef RDX_ENABLE_SMP
			m_coreMutexes[rdxCOREMUTEX_StringTableInsert]->Release();
#endif
			return rdxCRef(rdxCString)::Null();
		}

		RDX_TRY(ctx)
		{
			rdxCRef(rdxCString) newString;
			//_heapchk();
			RDX_PROTECT_ASSIGN(ctx, newString, rdxCInternalObjectFactory::CreateObject<rdxCString>(ctx, this, m_builtins.st_String.ToWeakHdl(), rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime)) );
			//_heapchk();

			rdxWeakHdl(rdxCString) strp = newString.ToWeakHdl();

			numStringAllocs++;
			{
				int bp = 0;
			}

			//_heapchk();
			RDX_PROTECT(ctx, m_stringTable.Insert(ctx, cs, &strp, hash));
			//_heapchk();

			strp->Initialize(cs->feedback.ToWeakRTRef(), cs->feedback->NumElements());
			//_heapchk();

			strRef = newString;
		}
		RDX_CATCH(ctx)
		{
#ifdef RDX_ENABLE_SMP
			m_coreMutexes[rdxCOREMUTEX_StringTableInsert]->Release();
#endif
			RDX_RETHROWV(ctx, rdxCRef(rdxCString)::Null());
		}
		RDX_ENDTRY
	}
	else
	{
		strRef = outStrPtr;
	}

#ifdef RDX_ENABLE_SMP
	m_coreMutexes[rdxCOREMUTEX_StringTableInsert]->Release();
#endif

	return strRef;
}

rdxCRef(rdxCString) rdxCObjectManager::CreateStringASCII(rdxSOperationContext *ctx, const char *str, bool sizeSpecified, rdxLargeUInt size, bool lookupOnly)
{
	if(str == NULL)
		return rdxCRef(rdxCString)::Null();
	if(!sizeSpecified)
		size = static_cast<rdxLargeUInt>(strlen(str));

	rdxSCandidateString cs;
	cs.format = rdxSCandidateString::CSF_ASCII;
	cs.numRawBytes1 = size;
	cs.input1ptr = str;
	cs.source = rdxSCandidateString::CSIS_Pointer;
	cs.numRawBytes2 = 0;
	if(!cs.CountCharacters())
		return rdxCRef(rdxCString)::Null();

	return CreateStringFromCS(ctx, &cs, lookupOnly);
}


rdxCRef(rdxCArrayOfType) rdxCObjectManager::CreateArrayType(rdxSOperationContext *ctx, rdxWeakHdl(rdxCType) t, rdxLargeUInt numDimensions, bool constant, rdxIfcTypeInfo typeInfo)
{
	if(t.IsNull())
		RDX_LTHROWV(ctx, RDX_ERROR_INTERNAL_BAD_TYPE, rdxCRef(rdxCArrayOfType)::Null());
	if(t == m_builtins.st_Varying)
		RDX_LTHROWV(ctx, RDX_ERROR_FORBIDDEN_VARYING_USAGE, rdxCRef(rdxCArrayOfType)::Null());

	rdxCRef(rdxCArrayOfType) aotRef;
	rdxWeakHdl(rdxCArrayOfType) outAOTPtr;

	rdxSAOTKey aotKey;
	aotKey.Set(t, numDimensions, (constant ? rdxTrueValue : rdxFalseValue));

#ifdef RDX_ENABLE_SMP
	m_coreMutexes[rdxCOREMUTEX_AOTTableInsert]->Acquire();
#endif
	bool found = m_aotTable.GetElement(&aotKey, &outAOTPtr);

	if(!found)
	{
		RDX_TRY(ctx)
		{
			rdxCRef(rdxCArrayOfType) newAOT;
			RDX_PROTECT_ASSIGN(ctx, newAOT, rdxCInternalObjectFactory::CreateObject<rdxCArrayOfType>(ctx, this, m_builtins.st_ArrayOfType.ToWeakHdl(), rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime)) );

			rdxWeakHdl(rdxCArrayOfType) aotp = newAOT.ToWeakHdl();
			RDX_PROTECT(ctx, m_aotTable.Insert(ctx, &aotKey, &aotp));

			aotp->isConstant = constant ? rdxTrueValue : rdxFalseValue;
			aotp->numDimensions = numDimensions;	// TODO: Truncation to the data format, or extend ArrayOfType!
			aotp->type = t;
			if(typeInfo.IsNull())
				aotp->m_native.arrayTypeInfo = rdxSAutoTypeInfo<rdxCArrayContainer>::TypeInfoInterface();
			else
				aotp->m_native.arrayTypeInfo = typeInfo;

			// TODO: Remove this when unloaded flags are refactored
			aotp->ObjectInfo()->objectFlags &= ~rdxGCInfo::GCOF_Unloaded;
			aotRef = newAOT;
		}
		RDX_CATCH(ctx)
		{
#ifdef RDX_ENABLE_SMP
			m_coreMutexes[rdxCOREMUTEX_AOTTableInsert]->Release();
#endif
			RDX_RETHROWV(ctx, rdxCRef(rdxCArrayOfType)::Null());
		}
		RDX_ENDTRY
	}
	else
	{
		aotRef = outAOTPtr;
	}

#ifdef RDX_ENABLE_SMP
	m_coreMutexes[rdxCOREMUTEX_AOTTableInsert]->Release();
#endif

	return aotRef;
}

rdxCRef(rdxCString) rdxCObjectManager::CreateString(rdxSOperationContext *ctx, const rdxChar *str, bool sizeSpecified, rdxLargeUInt numChars, bool lookupOnly)
{
	if(str == NULL)
		return rdxCRef(rdxCString)::Null();

	if(!sizeSpecified)
	{
		const rdxChar *scan = str;
		numChars = 0;

		while(*scan)
		{
			numChars++;
			scan++;
		}
	}

	rdxSCandidateString cs;
	cs.format = rdxSCandidateString::CSF_Char;
	cs.source = rdxSCandidateString::CSIS_Pointer;
	cs.numRawBytes1 = numChars * sizeof(rdxChar);
	cs.input1ptr = str;
	cs.numRawBytes2 = 0;
	if(!cs.CountCharacters())
		return rdxCRef(rdxCString)::Null();

	return CreateStringFromCS(ctx, &cs, lookupOnly);
}

rdxCRef(rdxCString) rdxCObjectManager::CreateString(rdxSOperationContext *ctx, rdxWeakOffsetHdl(rdxChar) str, bool sizeSpecified, rdxLargeUInt numChars, bool lookupOnly)
{
	if(!sizeSpecified)
	{
		rdxWeakOffsetHdl(rdxChar) scan = str;
		numChars = 0;

		while(*scan.Data())
		{
			numChars++;
			scan++;
		}
	}

	rdxSCandidateString cs;
	cs.format = rdxSCandidateString::CSF_Char;
	cs.source = rdxSCandidateString::CSIS_Pointer;
	cs.numRawBytes1 = numChars * sizeof(rdxChar);
	cs.input1hdl = str;
	cs.numRawBytes2 = 0;
	if(!cs.CountCharacters())
		return rdxCRef(rdxCString)::Null();

	return CreateStringFromCS(ctx, &cs, lookupOnly);
}

rdxCRef(rdxCString) rdxCObjectManager::CreateStringConcatenated(rdxSOperationContext *ctx, rdxWeakHdl(rdxCString) str1, rdxWeakHdl(rdxCString) str2)
{
	if(str2.IsNull())
		return str1.ToCRef();
	if(str1.IsNull())
		return str2.ToCRef();

	rdxSCandidateString cs;
	cs.format = rdxSCandidateString::CSF_Concatenated;
	cs.source = rdxSCandidateString::CSIS_Handle;
	cs.numRawBytes1 = str1->Length() * static_cast<rdxLargeUInt>(sizeof(rdxChar));
	cs.input1hdl = str1->AsChars()->OffsetElementRTRef(0).ToHdl();
	cs.numRawBytes2 = str2->Length() * static_cast<rdxLargeUInt>(sizeof(rdxChar));
	cs.input2hdl = str2->AsChars()->OffsetElementRTRef(0).ToHdl();

	// Change the first one to the actual byte count
	if(!rdxCheckAddOverflowU(cs.numRawBytes1, cs.numRawBytes2))
		RDX_LTHROWV(ctx, RDX_ERROR_INTEGER_OVERFLOW, rdxCRef(rdxCString)::Null());

	cs.numCharacters = (cs.numRawBytes1 + cs.numRawBytes2) / sizeof(rdxChar);

	return CreateStringFromCS(ctx, &cs, false);
}

rdxCRef(rdxCString) rdxCObjectManager::CreateStringSub(rdxSOperationContext *ctx, rdxWeakHdl(rdxCString) str, rdxLargeUInt startIndex, bool lengthSpecified, rdxLargeUInt length)
{
	if(str.IsNull())
		return rdxCRef(rdxCString)::Null();
	if(startIndex >= str->Length())
		return CreateString(ctx, RDX_STATIC_STRING(""), true, 0);
			
	rdxLargeUInt maxLength = str->Length() - startIndex;
	if(length > maxLength)
		length = maxLength;

	return CreateString(ctx, str->AsChars()->OffsetElementRTRef(startIndex).ToHdl(), true, length, false);
}

rdxCRef(rdxCString) rdxCObjectManager::CreateStringUTF8(rdxSOperationContext *ctx, const rdxByte *str, bool sizeSpecified, rdxLargeUInt size, bool lookupOnly)
{
	if(str == NULL)
		return rdxCRef(rdxCString)::Null();

	if(!sizeSpecified)
	{
		const rdxByte *scan = str;
		size = 0;

		while(*scan)
		{
			size++;
			scan++;
		}
	}

	rdxSCandidateString cs;
	cs.format = rdxSCandidateString::CSF_UTF8;
	cs.source = rdxSCandidateString::CSIS_Pointer;
	cs.input1ptr = str;
	cs.numRawBytes1 = size;
	cs.numRawBytes2 = 0;
	if(!cs.CountCharacters())
		return rdxCRef(rdxCString)::Null();

	return CreateStringFromCS(ctx, &cs, lookupOnly);
}

rdxCRef(rdxCString) rdxCObjectManager::CreateStringUTF8(rdxSOperationContext *ctx, rdxWeakOffsetHdl(rdxByte) str, bool sizeSpecified, rdxLargeUInt size, bool lookupOnly)
{
	if(!sizeSpecified)
	{
		const rdxByte *scan = str.Data();
		size = 0;

		while(*scan)
		{
			size++;
			scan++;
		}
	}

	rdxSCandidateString cs;
	cs.format = rdxSCandidateString::CSF_UTF8;
	cs.source = rdxSCandidateString::CSIS_Handle;
	cs.input1hdl = str;
	cs.numRawBytes1 = size;
	cs.numRawBytes2 = 0;
	if(!cs.CountCharacters())
		return rdxCRef(rdxCString)::Null();

	return CreateStringFromCS(ctx, &cs, lookupOnly);
}

rdxCRef(rdxCRuntimeThread) rdxCObjectManager::CreateThread(rdxSOperationContext *ctx, rdxLargeUInt stackSize)
{
	rdxCRef(rdxCRuntimeThread) rt;

	RDX_TRY(ctx)
	{
		RDX_PROTECT_ASSIGN(ctx, rt, CreateObjectContainer(ctx, sizeof(rdxCRuntimeThread),
			rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime), m_builtins.st_Thread.ToWeakHdl(),
			rdxSAutoTypeInfo<rdxCRuntimeThread>::TypeInfoInterface()).StaticCast<rdxCRuntimeThread>() );

		rt->stackBytes = static_cast<rdxUInt8*>(m_allocator.Alloc(stackSize, RDX_MAX_ALIGNMENT, rdxALLOC_ThreadStack));
		if(!rt->stackBytes)
			RDX_STHROW(ctx, RDX_ERROR_ALLOCATION_FAILED);

		rt->stackCapacity = stackSize;
		rt->ownerObjectManager = this;
		rt->insertionPoint = rt->stackBytes + rt->stackCapacity;
		return rt;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROWV(ctx, rdxCRef(rdxCRuntimeThread)::Null());
	}
	RDX_ENDTRY
}

void rdxCObjectManager::AddGlobalSymbol(rdxSOperationContext *ctx, rdxSObjectGUID symbolGUID, rdxWeakHdl(rdxCObject) object)
{
	RDX_TRY(ctx)
	{
		rdxSSerializationTag *serTag = object->ObjectInfo()->SerializationTag();
		serTag->isAnonymous = false;
		serTag->gstSymbol = symbolGUID;
#ifdef RDX_ENABLE_DEBUG_GST_SYMBOL
#endif
		RDX_PROTECT(ctx, m_gst.Insert(ctx, &symbolGUID, &object));
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

// Attempts to resolve any import symbols in unloaded packages.  Returns true if all imports were resolved, false otherwise.
bool rdxCObjectManager::ResolveImports(rdxSOperationContext *ctx, rdxIPackageHost *host)
{
	bool allResolved = true;

	RDX_TRY(ctx)
	{
		for(rdxLargeUInt oi=0;oi<m_unloadedPackages.m_numObjects;oi++)
		{
			rdxWeakHdl(rdxCPackage) pkg = m_unloadedPackages.m_objects->Element(oi).ToWeakHdl().StaticCast<rdxCPackage>();

			// Load symbols
			rdxLargeUInt nSymbols = pkg->m_native.importedSymbols->NumElements();
			for(rdxLargeUInt i=0;i<nSymbols;i++)
			{
				rdxWeakOffsetHdl(rdxSPackageManifestImport) mi = pkg->m_native.importedSymbols->OffsetElementRTRef(i).ToHdl();

				if(mi->resolution.IsNull())
				{
					rdxCRef(rdxCObject) objRef;

					bool found;
					found = ImportPackageObject(mi->objectGUID, &objRef);

					if(!found)
					{
						allResolved = false;
					}
					else
					{
						if(!host->DomainsVisible(pkg->m_native.domain, mi->objectGUID.m_domain))
							RDX_STHROW(ctx, RDX_ERROR_DOMAIN_POLICY_VIOLATION);
						mi->resolution = objRef;
					}
				}
			}
		}

		return allResolved;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROWV(ctx, false);
	}
	RDX_ENDTRY
}

bool rdxCObjectManager::ImportPackageObject(const rdxSObjectGUID &guid, rdxCRef(rdxCObject) *outObjRef)
{
	{
		rdxWeakHdl(rdxCObject) gstRef;
		if(m_gst.GetElement(&guid, &gstRef))
		{
			*outObjRef = gstRef;
			return true;
		}
	}

	// Look in unloaded packages
	for(rdxLargeUInt oi=0;oi<m_unloadedPackages.m_numObjects;oi++)
	{
		rdxWeakHdl(rdxCPackage) pkg = m_unloadedPackages.m_objects->Element(oi).ToWeakHdl().StaticCast<rdxCPackage>();

		if(pkg->m_native.domain == guid.m_domain)
		{
			// Load symbols
			rdxLargeUInt nSymbols = pkg->m_native.localSymbols->NumElements();
			for(rdxLargeUInt i=0;i<nSymbols;i++)
			{
				const rdxSPackageManifestLocal &resource = pkg->m_native.localSymbols->Element(i);
				if((resource.flags & rdxSPackageManifestLocal::PMLF_Anonymous) == 0 && resource.objectGUID == guid)
				{
					*outObjRef = resource.resolvedObject;
					return true;
				}
			}
		}
	}

	return false;
}

bool rdxCObjectManager::TypeIsValid(rdxWeakRTRef(rdxCType) t) const
{
	if(t.IsNull()) return true;
	if(t->ObjectInfo()->containerType == m_builtins.st_ArrayOfType ||
		t->ObjectInfo()->containerType == m_builtins.st_StructuredType ||
		t->ObjectInfo()->containerType == m_builtins.st_DelegateType)
		return true;
	return false;
}

bool rdxCObjectManager::ObjectIsConstant(rdxWeakRTRef(rdxCObject) obj) const
{
	if(obj.IsNull())
		return false;
	if((obj->ObjectInfo()->objectFlags & rdxGCInfo::GCOF_ConstantArray) != 0)
		return true;
	return false;
}

bool rdxCObjectManager::TypeCanBeTraced(rdxWeakRTRef(rdxCType) t) const
{
	if(t.IsNull())
		return false;	// NULLs are object references, but they're not traceable

	if(TypeIsObjectReference(t) || TypeIsInterface(t))
		return true;

	if(t->ObjectInfo()->containerType == m_builtins.st_StructuredType)
	{
		rdxWeakRTRef(rdxCStructuredType) st = t.StaticCast<rdxCStructuredType>();
		if((st->m_native.nativeTypeInfo.IsNotNull() && (st->m_native.nativeTypeInfo.Flags() & rdxETIF_VisitReferences)) ||
			st->m_native.containedReferences.IsNotNull())
			return true;
	}
	return false;
}

bool rdxCObjectManager::TypeIsArray(rdxWeakRTRef(rdxCType) t) const
{
	if(t.IsNull()) return false;
	return t->ObjectInfo()->containerType == m_builtins.st_ArrayOfType;
}

bool rdxCObjectManager::TypeIsInterface(rdxWeakRTRef(rdxCType) t) const
{
	if(t.IsNull()) return false;	// NULL is considered an object reference
	rdxGCInfo *objInfo = t->ObjectInfo();
	if(objInfo->containerType == m_builtins.st_StructuredType)
	{
		rdxWeakRTRef(rdxCStructuredType) st = t.StaticCast<rdxCStructuredType>();
		if(st->storageSpecifier == rdxSS_Interface)
			return true;
	}
	return false;
}

bool rdxCObjectManager::TypeIsClass(rdxWeakRTRef(rdxCType) t) const
{
	if(t.IsNull()) return false;	// NULL isn't strictly a class (TODO: This may be kind of messed up)
	rdxGCInfo *objInfo = t->ObjectInfo();
	if(objInfo->containerType == m_builtins.st_StructuredType)
	{
		rdxWeakRTRef(rdxCStructuredType) st = t.StaticCast<rdxCStructuredType>();
		if(st->storageSpecifier == rdxSS_Class)
			return true;
	}
	return false;
}

bool rdxCObjectManager::TypeIsObjectReference(rdxWeakRTRef(rdxCType) t) const
{
	if(t.IsNull()) return true;	// NULL itself is a reference
	rdxGCInfo *objInfo = t->ObjectInfo();
	if(objInfo->containerType == m_builtins.st_DelegateType) return true;
	if(objInfo->containerType == m_builtins.st_ArrayOfType) return true;
	if(objInfo->containerType == m_builtins.st_StructuredType)
	{
		rdxWeakRTRef(rdxCStructuredType) st = t.StaticCast<rdxCStructuredType>();
		if(st->storageSpecifier == rdxSS_Class)
			return true;
	}
	return false;
}

bool rdxCObjectManager::RefTypesCompatibleRecursive(rdxWeakRTRef(rdxCType) from, rdxWeakRTRef(rdxCType) to, rdxWeakRTRef(rdxCType) initialFrom, int depth) const
{
	if(depth == RDX_MAX_RECURSION_DEPTH)
		return false;

	const rdxGCInfo *toInfo = to->ObjectInfo();

	if(from.IsNull())
	{
		// NULL can be cast to reference types
		return TypeIsObjectReference(to) || TypeIsInterface(to);
	}

	if(from == to)
		return true;

	if(from.IsNull() || to.IsNull())
		return false;	// Corrupt type def?

	if(to == m_builtins.st_Object)
		return true;

	const rdxGCInfo *fromInfo = from->ObjectInfo();

	if(fromInfo->containerType == m_builtins.st_ArrayOfType)
	{
		if(to == m_builtins.st_Array || to == m_builtins.st_Object) return true;

		if(toInfo->containerType == m_builtins.st_ArrayOfType)
		{
			rdxWeakRTRef(rdxCArrayOfType) aotFrom = from.StaticCast<rdxCArrayOfType>();
			rdxWeakRTRef(rdxCArrayOfType) aotTo = to.StaticCast<rdxCArrayOfType>();
			rdxWeakRTRef(rdxCType) subFrom = aotFrom->type.ToWeakRTRef();
			rdxWeakRTRef(rdxCType) subTo = aotTo->type.ToWeakRTRef();

			if(aotFrom->isConstant != rdxFalseValue && aotTo->isConstant == rdxFalseValue)
				return false;	// Conversion from constant to non-constant

			if(aotFrom->numDimensions != aotTo->numDimensions)
				return false;	// Dimension count mismatch

			if(subFrom == initialFrom)
				return false;	// Circular type def

			if(subFrom == subTo)
				return true;	// Subtypes match exactly

			// Not an exact match
			if(aotTo->isConstant == rdxFalseValue)
				return false;	// Can't convert to a parent type unless the array is constant

			if(TypeIsObjectReference(subFrom))
				return RefTypesCompatibleRecursive(subFrom, subTo, initialFrom, depth + 1);
			else
				return NonRefTypesCompatible(subFrom, subTo);
		}

		return false;
	}

	if(fromInfo->containerType == m_builtins.st_DelegateType)
	{
		if(to == m_builtins.st_Method || to == m_builtins.st_Object) return true;
		return false;
	}

	if(fromInfo->containerType == m_builtins.st_StructuredType &&
		from.StaticCast<rdxCStructuredType>()->storageSpecifier == rdxSS_Class)
	{
		if(toInfo->containerType == m_builtins.st_StructuredType)
		{
			if(to.StaticCast<rdxCStructuredType>()->storageSpecifier == rdxSS_Class)
			{
				rdxWeakRTRef(rdxCStructuredType) st = from.StaticCast<rdxCStructuredType>()->parentClass.ToWeakRTRef();
				while(st.IsNotNull())
				{
					if(st == initialFrom)	// Recursive (corrupt)
						return false;
					if(st == to)
						return true;
					st = st->parentClass;
				}
				return false;
			}
		}
		return false;
	}
	return false;
}

		
bool rdxCObjectManager::NonRefTypesCompatible(rdxWeakRTRef(rdxCType) from, rdxWeakRTRef(rdxCType) to) const
{
	if(from->ObjectInfo()->containerType == m_builtins.st_StructuredType
		&& from.StaticCast<rdxCStructuredType>()->storageSpecifier == rdxSS_Enum
		&& to == m_builtins.RDX_ENUM_INTERNAL_TYPE)
		return true;

	return from == to;
}

bool rdxCObjectManager::TypesCompatible(rdxWeakRTRef(rdxCType) from, rdxWeakRTRef(rdxCType) to) const
{
	if(from == to)
		return true;

	if(TypeIsObjectReference(from))
		return RefTypesCompatibleRecursive(from, to, from, 0);

	if(from->ObjectInfo()->containerType == m_builtins.st_StructuredType
		&& from.StaticCast<rdxCStructuredType>()->storageSpecifier == rdxSS_Enum
		&& to == m_builtins.RDX_ENUM_INTERNAL_TYPE)
		return true;

	return false;
}

bool rdxCObjectManager::TypeImplementsInterface(rdxWeakRTRef(rdxCStructuredType) tClass, rdxWeakRTRef(rdxCStructuredType) tIfc) const
{
	if(tClass.IsNull() || tIfc.IsNull())
		return false;
	if(tClass->storageSpecifier != rdxSS_Class || tIfc->storageSpecifier != rdxSS_Interface)
		return false;
	if(tClass->interfaces.IsNull())
		return false;
	rdxLargeUInt numInterfaces = tClass->interfaces->NumElements();
	for(rdxLargeUInt i=0;i<numInterfaces;i++)
		if(tClass->interfaces->Element(i).type == tIfc)
			return true;
	return false;
}

bool rdxCObjectManager::TypesCompatiblePolymorphic(rdxWeakRTRef(rdxCType) from, rdxWeakRTRef(rdxCType) to) const
{
	if(TypesCompatible(to, from))
		return true;	// Target can be directly converted to source, so they're definitely convertable

	// Conversions to and from interfaces are valid if a subclass could contain it.
	// This means that interface-interface conversions are valid and interface-class conversions are valid as long as the class isn't final.
	if(from->ObjectInfo()->containerType == m_builtins.st_StructuredType && to->ObjectInfo()->containerType == m_builtins.st_StructuredType)
	{
		rdxWeakRTRef(rdxCStructuredType) fromST = from.StaticCast<rdxCStructuredType>();
		rdxWeakRTRef(rdxCStructuredType) toST = to.StaticCast<rdxCStructuredType>();

		if(fromST->storageSpecifier == rdxSS_Interface)
		{
			if(toST->storageSpecifier == rdxSS_Interface)
				return true;
			if(toST->storageSpecifier == rdxSS_Class)
			{
				if(!toST->isFinal)
					return true;	// Subclass could contain
				// Class is final, it must contain the source interface
				if(TypeImplementsInterface(toST, fromST))
					return true;
				return false;
			}
			return false;
		}
		if(toST->storageSpecifier == rdxSS_Interface)
		{
			if(fromST->storageSpecifier == rdxSS_Class)
			{
				if(fromST->isFinal == rdxFalseValue)
					return true;	// Subclass could contain
				if(TypeImplementsInterface(fromST, toST))
					return true;
			}
			return false;
		}
	}

	return false;
}

void rdxCObjectManager::TypeValueSize(rdxSOperationContext *ctx, rdxWeakHdl(rdxCType) t, rdxLargeUInt &size, rdxLargeUInt &align) const
{
	if(t.IsNull())
	{
		size = sizeof(rdxTracedTypelessRTRef);
		align = rdxAlignOf(rdxTracedTypelessRTRef);
		return;
	}

	if(t == m_builtins.st_Varying)
	{
		// This should never be called on varying
		RDX_LTHROW(ctx, RDX_ERROR_INTERNAL_BAD_TYPE);
	}
	else if(t->ObjectInfo()->containerType == m_builtins.st_StructuredType)
	{
		rdxWeakRTRef(rdxCStructuredType) st = t.ToWeakRTRef().StaticCast<rdxCStructuredType>();
		switch(st->storageSpecifier)
		{
		case rdxSS_Interface:
			size = sizeof(rdxTracedTypelessIRef);
			align = rdxAlignOf(rdxTracedTypelessIRef);
			break;
		case rdxSS_Class:
			size = sizeof(rdxTracedTypelessRTRef);
			align = rdxAlignOf(rdxTracedTypelessRTRef);
			break;
		case rdxSS_Enum:
			size = sizeof(rdxEnumValue);
			align = rdxALIGN_EnumValue;
			break;
		case rdxSS_ValStruct:
		case rdxSS_RefStruct:
			if((st->m_native.flags & rdxCStructuredType::NativeProperties::STF_StructureEvaluated) == 0)
				size = align = 0;
			else
			{
				size = st->m_native.size;
				align = st->m_native.alignment;
			}
			break;
		default:
			RDX_LTHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
		};
	}
	else if(t->ObjectInfo()->containerType == m_builtins.st_DelegateType ||
		t->ObjectInfo()->containerType == m_builtins.st_ArrayOfType)
	{
		size = sizeof(rdxTracedTypelessRTRef);
		align = rdxAlignOf(rdxTracedTypelessRTRef);
	}
	else
		RDX_LTHROW(ctx, RDX_ERROR_INTERNAL_BAD_TYPE);
}

bool rdxCObjectManager::EnumCompatible(rdxEnumValue ev, rdxWeakArrayRTRef(rdxSEnumerant) enums) const
{
	if(enums.IsNull())
		return false;

	if(ev == 0)
		return true;	// 0 is always valid

	rdxLargeUInt searchMin = 0;
	rdxLargeUInt searchSpan = enums->NumElements();

	// Binary search
	while(searchSpan > 0)
	{
		rdxLargeUInt searchMid = searchMin + (searchSpan / 2);
		rdxEnumValue evMid = enums->Element(searchMid).value;
		if(evMid < ev)
		{
			rdxLargeUInt newMin = searchMid + 1;
			searchSpan = searchMin + searchSpan - newMin;
			searchMin = newMin;
		}
		else if(evMid > ev)
			searchSpan = searchMid - searchMin;
		else
			return true;	// Matched, this is OK
	}
	return false;
}

rdxWeakIRef(rdxSObjectInterfaceImplementation) rdxCObjectManager::FindInterface(rdxWeakRTRef(rdxCObject) obj, rdxWeakRTRef(rdxCStructuredType) interfaceType) const
{
	if(obj.IsNotNull())
	{
		rdxGCInfo *objInfo = obj->ObjectInfo();
		rdxWeakRTRef(rdxCType) objType = objInfo->containerType;
		if(objType.IsNotNull() && objType->ObjectInfo()->containerType == m_builtins.st_StructuredType)
		{
			rdxWeakRTRef(rdxCStructuredType) st = objType.StaticCast<rdxCStructuredType>();
			if(st->storageSpecifier == rdxSS_Class)
			{
				if(st->interfaces.IsNotNull())
				{
					rdxLargeUInt nInterfaces = st->interfaces->NumElements();
					for(rdxLargeUInt i=0;i<nInterfaces;i++)
					{
						rdxSInterfaceImplementation &ii = st->interfaces->Element(i);
						if(ii.type == interfaceType)
						{
							rdxSObjectInterfaceImplementation *oii = reinterpret_cast<rdxSObjectInterfaceImplementation*>(reinterpret_cast<rdxUInt8*>(static_cast<rdxCObject*>(obj.Modify())) + st->m_native.interfaceOffsets->Element(i));
							return rdxWeakIRef(rdxSObjectInterfaceImplementation)(rdxObjRef_CSignal_DataPointer, oii);
						}
					}
				}
			}
		}
	}
	return rdxWeakIRef(rdxSObjectInterfaceImplementation)::Null();
}

bool rdxCObjectManager::ObjectCompatible(rdxWeakRTRef(rdxCObject) from, rdxWeakRTRef(rdxCType) to) const
{
	if(from.IsNull())
		return true;
	if(to.IsNull())
		return from.IsNull();

	if(to == m_builtins.st_Object)
		return true;	// Can cast anything to Object, including structural resources

	if(to->ObjectInfo()->containerType == m_builtins.st_DelegateType)
	{
		if(from->ObjectInfo()->containerType != m_builtins.st_Method)
			return false;
		rdxWeakRTRef(rdxCMethod) m = from.StaticCast<rdxCMethod>();
		rdxWeakRTRef(rdxCDelegateType) dt = to.StaticCast<rdxCDelegateType>();

		if(m->returnTypes != dt->returnTypes)
			return false;
		if(m->parameters != dt->parameters)
			return false;
		return true;
	}
	else
		return TypesCompatible(from->ObjectInfo()->containerType.ToWeakRTRef(), to);
}

void CopyClassDefault(rdxCObject *dest, const rdxCObject *src, const rdxCStructuredType *st)
{
	rdxIfcTypeInfo nti = st->m_native.nativeTypeInfo;
	rdxIfcTypeFuncs typeFuncs = nti.TypeFuncs();
	rdxIfcTypeFuncs::RefToObjectHeadFunc rtohf = typeFuncs.GetRefToObjectHeadFunc();

	void *destHead = rtohf(dest);
	const void *srcHead = rtohf(const_cast<rdxCObject*>(src));
	// Need the NTI size in subclass, not the ST's, because the ST may be a subclass.
	// We only want to assign the NTI portion and block copy the rest
	rdxLargeUInt sizeInSubclass = nti.SizeInSubclass();
	typeFuncs.GetAssignFunc()(destHead, srcHead);

	rdxBlockCopy(static_cast<rdxUInt8*>(destHead) + sizeInSubclass, static_cast<const rdxUInt8*>(srcHead) + sizeInSubclass, st->m_native.size - sizeInSubclass);
}


bool rdxCObjectManager::ResolveStructureDefault(rdxSOperationContext *ctx, rdxWeakHdl(rdxCStructuredType) st, rdxWeakHdl(rdxCPackage) pkg)
{
	// This is called on structures that can't zero-fill and need a default.
	// We need to do a few things here:
	// - If all dependency defaults are evaluated, set DependencyDefaultsEvaluated.
	// - If there are dependency defaults, generate an initial default.
	// - If there is no default reference, set FinalDefaultEvaluated.
	// Note that the default is loaded into an initial object THEN copied into the default
	RDX_TRY(ctx)
	{
		bool madeProgress = false;
		rdxWeakHdl(rdxCStructuredType) parentClass = st->parentClass.ToWeakHdl();
		
		rdxLargeUInt numProperties = 0;
		rdxWeakArrayHdl(rdxSProperty) properties = st->properties.ToWeakHdl();
		if(properties.IsNotNull())
			numProperties = properties->NumElements();

		rdxLargeUInt numInterfaces = 0;
		if(st->interfaces.IsNotNull())
			numInterfaces = st->interfaces->NumElements();

		if(!(st->m_native.flags & rdxCStructuredType::NativeProperties::STF_DependencyDefaultsEvaluated) &&
			(st->m_native.flags & rdxCStructuredType::NativeProperties::STF_StructureEvaluated))
		{
			rdxLargeUInt numParentProperties = 0;

			if(parentClass.IsNotNull())
			{
				if(!(st->parentClass->m_native.flags & rdxCStructuredType::NativeProperties::STF_FinalDefaultEvaluated))
					return madeProgress;	// Parent wasn't evaluated

				// Verify that the initial properties are all of the same type
				rdxWeakArrayHdl(rdxSProperty) parentProperties = st->parentClass->properties.ToWeakHdl();
				if(parentProperties.IsNotNull())
					numParentProperties = parentProperties->NumElements();

				if(numParentProperties > numProperties)
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

				for(rdxLargeUInt i=0;i<numParentProperties;i++)
				{
					if(properties->Element(i).type != parentProperties->Element(i).type)
						RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
				}
			}

			for(rdxLargeUInt i=0;i<numProperties;i++)
			{
				rdxWeakOffsetHdl(rdxSProperty) p = properties->OffsetElementRTRef(i).ToHdl();
				if(p->type->ObjectInfo()->containerType == m_builtins.st_StructuredType)
				{
					rdxWeakHdl(rdxCStructuredType) pst = p->type.ToWeakHdl().StaticCast<rdxCStructuredType>();
					if(pst.IsNull())
						RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

					if(pst->storageSpecifier == rdxSS_ValStruct ||
						pst->storageSpecifier == rdxSS_RefStruct)
					{
						rdxLargeUInt pOffset = st->m_native.propertyOffsets->Element(i);

						// Copy the struct
						if(!(pst->m_native.flags & rdxCStructuredType::NativeProperties::STF_FinalDefaultEvaluated))
							return madeProgress;
					}
				}
			}

			// Dependency defaults are all OK
			st->m_native.flags |= rdxCStructuredType::NativeProperties::STF_DependencyDefaultsEvaluated;
			madeProgress = true;
		}

		if(!(st->m_native.flags & rdxCStructuredType::NativeProperties::STF_FinalDefaultEvaluated) &&
			(st->m_native.flags & rdxCStructuredType::NativeProperties::STF_DependencyDefaultsEvaluated))
		{
			// Dependency defaults are evaluated, but don't have a final default yet
			if(st->m_native.currentDefaultValue.IsNull())
			{
				// Create an initial default
				RDX_PROTECT_ASSIGN(ctx, st->m_native.currentDefaultValue, CreateObjectContainer(ctx, st->m_native.size, st->ObjectInfo()->SerializationTag()->gstSymbol.m_domain, st, st->m_native.nativeTypeInfo));
				if(st->ObjectInfo()->SerializationTag() && st->ObjectInfo()->SerializationTag()->gstSymbol == rdxSObjectGUID::FromObjectName("RDX.Compiler", "MemberDeclNode"))
				{
					DEBUG_buggy_object = st->m_native.currentDefaultValue->ObjectInfo();
					int bp = 0;
				}

				rdxUInt8 *defaultInstanceBytes;
				if(st->storageSpecifier == rdxSS_RefStruct || st->storageSpecifier == rdxSS_ValStruct)
					defaultInstanceBytes = reinterpret_cast<rdxUInt8*>(st->m_native.currentDefaultValue.StaticCast<rdxCStructContainer>()->ModifyRawData());
				else
					defaultInstanceBytes = reinterpret_cast<rdxUInt8*>(st->m_native.currentDefaultValue.Modify());

				if(parentClass.IsNotNull())
				{
					if(parentClass->m_native.currentDefaultValue.IsNotNull())
					{
						CopyClassDefault(reinterpret_cast<rdxCObject*>(defaultInstanceBytes), parentClass->m_native.currentDefaultValue.Data(), parentClass.Data());
					}
				}

				for(rdxLargeUInt i=0;i<numProperties;i++)
				{
					rdxWeakOffsetHdl(rdxSProperty) p = properties->OffsetElementRTRef(i).ToHdl();
					if(p->type->ObjectInfo()->containerType == m_builtins.st_StructuredType)
					{
						rdxWeakHdl(rdxCStructuredType) pst = p->type.ToWeakHdl().StaticCast<rdxCStructuredType>();

						if(pst->storageSpecifier == rdxSS_ValStruct ||
							pst->storageSpecifier == rdxSS_RefStruct)
						{
							rdxLargeUInt pOffset = st->m_native.propertyOffsets->Element(i);

							if(pst->m_native.currentDefaultValue.IsNotNull())
							{
								void *propertyLocation = defaultInstanceBytes + pOffset;
								rdxBlockCopy(propertyLocation, pst->m_native.currentDefaultValue.Data(), static_cast<rdxLargeUInt>(pst->m_native.size));
							}
						}
					}
				}

				for(rdxLargeUInt i=0;i<numInterfaces;i++)
				{
					rdxLargeUInt iOffset = st->m_native.interfaceOffsets->Element(i);
					rdxSObjectInterfaceImplementation *oii = reinterpret_cast<rdxSObjectInterfaceImplementation*>(defaultInstanceBytes + iOffset);
					oii->vftOffset = st->interfaces->Element(i).vftOffset;
					oii->headOffset = iOffset;
				}
			}

			st->m_native.flags |= rdxCStructuredType::NativeProperties::STF_DependencyDefaultsEvaluated;
			if(st->m_native.defaultValueReference.symbolLoc == rdxPSL_Null)
			{
				// If nothing overrides, keep this
				st->m_native.flags |= rdxCStructuredType::NativeProperties::STF_FinalDefaultEvaluated;
				// If the parent class has the same size and nothing is overriding, then recycle its default
				if(st->parentClass.IsNotNull() && st->m_native.size == st->parentClass->m_native.size)
				{
					// TODO MUSTFIX: Check this against native types...
					st->m_native.currentDefaultValue = st->parentClass->m_native.currentDefaultValue;
					if(st->ObjectInfo()->SerializationTag() && st->ObjectInfo()->SerializationTag()->gstSymbol == rdxSObjectGUID::FromObjectName("RDX.Compiler", "MemberDeclNode"))
					{
						int bp = 0;
					}
				}
			}
			else if(st->m_native.defaultValueReference.symbolLoc == rdxPSL_Local)
			{
				rdxWeakHdl(rdxCObject) resolvedObject = rdxWeakHdl(rdxCObject)::Null();
				RDX_PROTECT(ctx, st->m_native.defaultValueReference.ConvertToReference(ctx, pkg, true, &resolvedObject));

				if(resolvedObject.IsNotNull() && !(resolvedObject->ObjectInfo()->objectFlags & rdxGCInfo::GCOF_Unloaded))
				{
					st->m_native.originalDefaultValue = resolvedObject;

					if(!(resolvedObject->ObjectInfo()->objectFlags & rdxGCInfo::GCOF_Cloaked))
						RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);	// Only cloaked objects are permitted

					if(resolvedObject->ObjectInfo()->containerType != st)
						RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

					st->m_native.currentDefaultValue = resolvedObject;
					if(st->ObjectInfo()->SerializationTag() && st->ObjectInfo()->SerializationTag()->gstSymbol == rdxSObjectGUID::FromObjectName("RDX.Compiler", "MemberDeclNode"))
					{
						int bp = 0;
					}
					st->m_native.flags |= rdxCStructuredType::NativeProperties::STF_FinalDefaultEvaluated;
				}
			}
			else
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);	// Non-local defaults are forbidden

			madeProgress = true;
		}

		return madeProgress;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROWV(ctx, false);
	}
	RDX_ENDTRY
}

void rdxCObjectManager::VerifyDuplicates(rdxSOperationContext *ctx, rdxIPackageHost *packageHost)
{
	// TODO MUSTFIX
#if 0
	rdxIFileStream *stream = NULL;
	bool isText = false;

	RDX_TRY(ctx)
	{
		for(rdxLargeUInt oi=0;oi<m_unloadedPackages.m_numObjects;oi++)
		{
			rdxWeakHdl(rdxCPackage) pkg = m_unloadedPackages.m_objects->Element(oi).ToWeakHdl().StaticCast<rdxCPackage>();

			// Load symbols
			rdxLargeUInt nSymbols = pkg->m_native.localSymbols->NumElements();
			for(rdxLargeUInt i=0;i<nSymbols;i++)
			{
				rdxWeakOffsetHdl(rdxSPackageManifestLocal) ml = pkg->m_native.localSymbols->OffsetElementRTRef(i).ToHdl();

				if(ml->flags & rdxSPackageManifestLocal::PMLF_Duplicate)
				{
					if(!stream)
						stream = packageHost->StreamForDomain(this, pkg->m_native.domain, false, isText);

					// This is of the appropriate criticality, load it
					bool loadedOK;
					rdxCRef(rdxCObject) duplicateObj;

					RDX_PROTECT_ASSIGN(ctx, duplicateObj, CreateInitialObject(ctx, ml));
					InitializeObject(duplicateObj.ToWeakRTRef(), 0, false);
					RDX_PROTECT_ASSIGN(ctx, loadedOK, rdxDeserializeObject(ctx, this, pkg, duplicateObj.ToWeakHdl(), ml, pkg->m_native.domain, packageHost, stream, isText) );
					
					if(!loadedOK)
						RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
					duplicateObj->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_TransientDuplicate;

					rdxWeakHdl(rdxCObject) gstObj;
					bool foundElement = m_gst.GetElement(&ml->objectGUID, &gstObj);

					if(!foundElement)
						RDX_STHROW(ctx, RDX_ERROR_INTERNAL_DUPLICATE_LOST_GST);

					if(gstObj->ObjectInfo()->containerType != duplicateObj->ObjectInfo()->containerType)
						RDX_STHROW(ctx, RDX_ERROR_DUPLICATED_SYMBOL_MISMATCH);

					rdxLargeUInt numElements = gstObj->NumElements();
					if(numElements != duplicateObj->NumElements())
						RDX_STHROW(ctx, RDX_ERROR_DUPLICATED_SYMBOL_MISMATCH);

					rdxLargeUInt numDimensions = gstObj->ObjectInfo()->NumDimensions();
					for(rdxLargeUInt i=0;i<numDimensions;i++)
						if(gstObj->ObjectInfo()->Dimension(i) != duplicateObj->ObjectInfo()->Dimension(i))
							RDX_STHROW(ctx, RDX_ERROR_DUPLICATED_SYMBOL_MISMATCH);


					rdxLargeUInt elementSize = sizeof(rdxTracedRTRef(rdxCObject));
					if(gstObj->ObjectInfo()->ContainedStructure().IsNotNull())
						elementSize = gstObj->ObjectInfo()->ContainedStructure()->m_native.size;

					int memcmpResult;

					if(gstObj->ObjectInfo()->ContainedStructure().IsNotNull() &&
						gstObj->ObjectInfo()->ContainedStructure()->m_native.nativeTypeInfo &&
						(gstObj->ObjectInfo()->ContainedStructure()->m_native.nativeTypeInfo->flags & rdxETIF_CompareDuplicate))
					{
						rdxSTypeFuncs::DuplicateEqualFunc equalFunc = gstObj->ObjectInfo()->ContainedStructure()->m_native.nativeTypeInfo->typeFuncs->duplicateEqualFunc;
						memcmpResult = equalFunc(gstObj.Data(), duplicateObj.Data()) ? 0 : 1;
					}
					else
						memcmpResult = memcmp(gstObj.Data(), duplicateObj.Data(), static_cast<size_t>(elementSize * numElements));

					if(memcmpResult)
						RDX_STHROW(ctx, RDX_ERROR_DUPLICATED_SYMBOL_MISMATCH);
				}
			}

			if(stream)
				stream->Close();
			stream = NULL;
		}
	}
	RDX_CATCH(ctx)
	{
		if(stream)
			stream->Close();
		stream = NULL;
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
#endif
}

rdxCRef(rdxCObject) rdxCObjectManager::CreateInitialObject(rdxSOperationContext *ctx, rdxWeakOffsetHdl(rdxSPackageManifestLocal) ml)
{
	// TODO MUSTFIX: Disallow array-of-type and string creation from this
	return this->CreateInitialObject(ctx, ml->resolvedType.ToWeakHdl(), ml->numElements, ml->objectGUID);
}

rdxCRef(rdxCObject) rdxCObjectManager::CreateInitialObject(rdxSOperationContext *ctx, rdxWeakHdl(rdxCType) objectType, rdxLargeUInt numElements, const rdxSObjectGUID& objectGUID)
{
	RDX_TRY(ctx)
	{
		if(objectType == m_builtins.st_ArrayOfType)
			RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
		if(objectType->ObjectInfo()->containerType == m_builtins.st_StructuredType)
		{
			if(numElements != 1)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
			rdxWeakHdl(rdxCStructuredType) objST = objectType.StaticCast<rdxCStructuredType>();

			rdxLargeUInt containerSize;
			switch(objST->storageSpecifier)
			{
			case rdxSS_Class:
				containerSize = objST->m_native.size;
				break;
			case rdxSS_RefStruct:
			case rdxSS_ValStruct:
				{
					rdxLargeUInt paddedBoxSize = rdxPaddedSize(sizeof(rdxCStructContainer), objST->m_native.alignment);
					if(!rdxCheckAddOverflowU(paddedBoxSize, objST->m_native.size))
						RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);
					containerSize = paddedBoxSize + objST->m_native.size;
				}
				break;
			default:
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
			}

			rdxCRef(rdxCObject) obj;
			RDX_PROTECT_ASSIGN(ctx, obj, CreateObjectContainer(ctx, containerSize, objectGUID.m_domain, objST, objST->m_native.nativeTypeInfo));
			if(rdxSSerializationTag *serTag = obj->ObjectInfo()->SerializationTag())
				serTag->gstSymbol = objectGUID;

			obj->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Unloaded;
			return obj;
		}
		else if(objectType->ObjectInfo()->containerType == m_builtins.st_ArrayOfType)
		{
			rdxWeakHdl(rdxCArrayOfType) objAOT = objectType.StaticCast<rdxCArrayOfType>();

			rdxLargeUInt elementSize = TypeElementSize(objAOT.ToWeakRTRef());
			if(!elementSize)
				RDX_STHROW(ctx, RDX_ERROR_INTERNAL_BAD_TYPE);

			rdxLargeUInt containerSize, dimOffset;
			RDX_PROTECT(ctx, rdxCArrayContainer::ComputeContainerSize(ctx, elementSize, numElements, objAOT->numDimensions, &dimOffset, &containerSize));

			rdxCRef(rdxCArrayContainer) obj;
			RDX_PROTECT_ASSIGN(ctx, obj, CreateObjectContainer(ctx, containerSize, objectGUID.m_domain, objAOT, objAOT->m_native.arrayTypeInfo).StaticCast<rdxCArrayContainer>());
			if(rdxSSerializationTag *serTag = obj->ObjectInfo()->SerializationTag())
				serTag->gstSymbol = objectGUID;

			rdxIfcTypeInfo nullTypeInfo;
			nullTypeInfo.fetchFunc = RDX_CNULL;

			obj->InitializeArray(numElements, 0, elementSize, dimOffset, nullTypeInfo);
			obj->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Unloaded;
			return obj;
		}
		else
			RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROWV(ctx, rdxCRef(rdxCObject)::Null());
	}
	RDX_ENDTRY
}

// Returns true if more rounds are needed
bool rdxCObjectManager::CreateInitialObjects(rdxSOperationContext *ctx, rdxIPackageHost *packageHost, bool criticalOnly)
{
	rdxIFileStream *stream = NULL;
	bool isText = false;
	bool needMoreRounds = false;

	printf("CIO round\n");

	RDX_TRY(ctx)
	{
		for(rdxLargeUInt oi=0;oi<m_unloadedPackages.m_numObjects;oi++)
		{
			rdxWeakHdl(rdxCPackage) pkg = m_unloadedPackages.m_objects->Element(oi).ToWeakHdl().StaticCast<rdxCPackage>();

			rdxGCInfo *isi = pkg->m_native.importedSymbols->ObjectInfo();

			// Load symbols
			rdxLargeUInt nSymbols = pkg->m_native.localSymbols->NumElements();
			for(rdxLargeUInt i=0;i<nSymbols;i++)
			{
				rdxWeakOffsetHdl(rdxSPackageManifestLocal) ml = pkg->m_native.localSymbols->OffsetElementRTRef(i).ToHdl();
				
				//printf("Attempting object %s:%s\n", ml->objectGUID.m_domain.DebugStr(), ml->objectGUID.DebugStr());
				if(ml->resolvedType.IsNull())
				{
					rdxWeakHdl(rdxCObject) typeObj;
					RDX_PROTECT(ctx, ml->typePkgRef.ConvertToReference(ctx, pkg, false, &typeObj));

					if(typeObj.IsNotNull())
					{
						if(!TypesCompatible(typeObj->ObjectInfo()->containerType.ToWeakRTRef(), m_builtins.st_Type.ToWeakRTRef()))
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

						rdxWeakHdl(rdxCType) t = typeObj.StaticCast<rdxCType>();
						if(!TypeIsValid(t.ToWeakRTRef()))
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
						ml->resolvedType = t;

						if(t->ObjectInfo()->objectFlags & rdxGCInfo::GCOF_Critical)
							ml->flags |= rdxSPackageManifestLocal::PMLF_Critical;
						if(t->ObjectInfo()->objectFlags & rdxGCInfo::GCOF_CriticalBorder)
							ml->flags |= rdxSPackageManifestLocal::PMLF_CriticalBorder;

						needMoreRounds = true;
					}
					else
					{
						if(!criticalOnly)
							RDX_STHROW(ctx, RDX_ERROR_UNRESOLVED_SYMBOL_REFERENCE);
					}
				}

				if(ml->resolvedObject.IsNull())
				{
					bool isDuplicate = false;

					if(!ml->duplicateChecked)
					{
						// See if this is a duplicate

						rdxWeakHdl(rdxCObject) objRef;
						if(!(ml->flags & rdxSPackageManifestLocal::PMLF_Anonymous) && m_gst.GetElement(&ml->objectGUID, &objRef))
						{
							if(ml->objectGUID.m_domain != rdxSDomainGUID::Builtin(rdxDOMAIN_Duplicable) && !(objRef->ObjectInfo()->objectFlags & rdxGCInfo::GCOF_Duplicable))
								RDX_STHROW(ctx, RDX_ERROR_NON_DUPLICABLE_SYMBOL_COLLISION);

							ml->flags |= rdxSPackageManifestLocal::PMLF_Duplicate;
							ml->resolvedObject = objRef;
							needMoreRounds = true;
							isDuplicate = true;
						}
						
						ml->duplicateChecked = true;
					}

					if(!isDuplicate && ml->resolvedType.IsNotNull() && !(ml->resolvedType->ObjectInfo()->objectFlags & rdxGCInfo::GCOF_Unloaded))
					{
						if(!criticalOnly || (ml->flags & (rdxSPackageManifestLocal::PMLF_Critical | rdxSPackageManifestLocal::PMLF_CriticalBorder)))
						{
							if(!strcmp(ml->objectGUID.m_debugStr, "Parser.operators"))
							{
								int bp = 0;
							}

							// Don't try to create non-critical objects in the critical round, it usually won't work
							printf("Creating initial object for %s:%s\n", ml->objectGUID.m_domain.DebugStr(), ml->objectGUID.DebugStr());
							RDX_PROTECT_ASSIGN(ctx, ml->resolvedObject, CreateInitialObject(ctx, ml));
							needMoreRounds = true;
						}
					}
					else
					{
						if(!criticalOnly)
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
					}
				}
			}
		}
	}
	RDX_CATCH(ctx)
	{
		if(stream)
			stream->Close();
		RDX_RETHROWV(ctx, false);
	}
	RDX_ENDTRY

	return needMoreRounds;
}


void rdxCObjectManager::GeneratePackageArrayDefs(rdxSOperationContext *ctx, rdxIPackageHost *packageHost)
{
	rdxIFileStream *stream = NULL;
	bool isText = false;
	bool deferredAny = true;

	RDX_TRY(ctx)
	{
		for(rdxLargeUInt oi=0;oi<m_unloadedPackages.m_numObjects;oi++)
		{
			rdxWeakHdl(rdxCPackage) pkg = m_unloadedPackages.m_objects->Element(oi).ToWeakHdl().StaticCast<rdxCPackage>();

			// Resolve non-array refs
			rdxLargeUInt nArrayDefs = pkg->m_native.arrayDefs->NumElements();
			for(rdxLargeUInt i=0;i<nArrayDefs;i++)
			{
				rdxWeakOffsetHdl(rdxSPackageArrayDef) adef = pkg->m_native.arrayDefs->OffsetElementRTRef(i).ToHdl();
				rdxWeakHdl(rdxCObject) resolvedRef;

				bool isArrayOfArrays = false;

				switch(adef->pkgRef.symbolLoc)
				{
				case rdxPSL_Array:
					isArrayOfArrays = true;
					break;
				case rdxPSL_Imported:
				case rdxPSL_Local:
				case rdxPSL_Null:
					RDX_PROTECT(ctx, adef->pkgRef.ConvertToReference(ctx, pkg, false, &resolvedRef) );
					break;
				default:
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
				};

				if(isArrayOfArrays)
					continue;

				if(resolvedRef.IsNull())
					RDX_STHROW(ctx, RDX_ERROR_UNRESOLVED_SYMBOL_REFERENCE);

				if(!this->ObjectCompatible(resolvedRef.ToWeakRTRef(), m_builtins.st_Type.ToWeakRTRef()))
					RDX_STHROW(ctx, RDX_ERROR_INVALID_ARRAY_TYPE);
				if(resolvedRef == m_builtins.st_Varying)
					RDX_STHROW(ctx, RDX_ERROR_FORBIDDEN_VARYING_USAGE);

				rdxCRef(rdxCArrayOfType) arrayType;
				RDX_PROTECT_ASSIGN(ctx, arrayType, this->CreateArrayType(ctx, resolvedRef.StaticCast<rdxCArrayOfType>(), adef->numDimensions, adef->isConstant, rdxSAutoTypeInfo<rdxCArrayContainer>::TypeInfoInterface()));

				adef->resolvedArrayType = arrayType.ToWeakRTRef();
			}

			bool deferredAny = true;
			while(deferredAny)
			{
				deferredAny = false;
				rdxLargeUInt nArrayDefs = pkg->m_native.arrayDefs->NumElements();
				for(rdxLargeUInt i=0;i<nArrayDefs;i++)
				{
					rdxWeakOffsetHdl(rdxSPackageArrayDef) adef = pkg->m_native.arrayDefs->OffsetElementRTRef(i).ToHdl();

					if(adef->pkgRef.symbolLoc == rdxPSL_Array)
					{
						if(adef->resolvedArrayType.IsNotNull())
							continue;

						if(adef->pkgRef.index == i)
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
						rdxWeakOffsetHdl(rdxSPackageArrayDef) subADef = pkg->m_native.arrayDefs->OffsetElementRTRef(adef->pkgRef.index).ToHdl();
						if(subADef->resolvedArrayType.IsNotNull())
						{
							rdxCRef(rdxCArrayOfType) newADef;
							RDX_PROTECT_ASSIGN(ctx, newADef, this->CreateArrayType(ctx, subADef->resolvedArrayType.ToWeakHdl(), adef->numDimensions, adef->isConstant, rdxSAutoTypeInfo<rdxCArrayContainer>::TypeInfoInterface()));
							adef->resolvedArrayType = newADef.ToWeakRTRef();
						}
						else
							deferredAny = true;
					}
				}
			}
		}
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}


void rdxCObjectManager::LoadSymbolsOfCriticality(rdxSOperationContext *ctx, rdxIPackageHost *packageHost, bool criticality)
{
	rdxIFileStream *stream = NULL;
	bool isText = false;

	RDX_TRY(ctx)
	{
		bool defer = true;
		int passNum = 0;
		bool gstInserts = false;	// TODO: Delete this
		while(defer)
		{
			passNum++;

			bool loadedAny = false;
			defer = false;

			gstInserts = false;

			for(rdxLargeUInt oi=0;oi<m_unloadedPackages.m_numObjects;oi++)
			{
				rdxWeakHdl(rdxCPackage) pkg = m_unloadedPackages.m_objects->Element(oi).ToWeakHdl().StaticCast<rdxCPackage>();

				rdxGCInfo *isi = pkg->m_native.importedSymbols->ObjectInfo();

				// Load symbols
				rdxLargeUInt nSymbols = pkg->m_native.localSymbols->NumElements();
				for(rdxLargeUInt i=0;i<nSymbols;i++)
				{
					rdxWeakOffsetHdl(rdxSPackageManifestLocal) ml = pkg->m_native.localSymbols->OffsetElementRTRef(i).ToHdl();

					// Resolving types is a first-run operation, ALL type references need to be resolved or failures can result later
					// So, always defer if a type resolve fails.
					if(ml->resolvedType.IsNull())
					{
						if(!criticality)
							RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);	// This should never happen, CreateInitialObjects should always fail if a type fails to resolve.
						continue;
					}

					if(ml->resolvedObject.IsNull())
					{
						if(!criticality)
							RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);	// This should never happen, CreateInitialObjects should always fail if a type fails to resolve.
						continue;
					}

					rdxWeakHdl(rdxCObject) objRef = ml->resolvedObject.ToWeakHdl();

					// Don't load duplicates
					if(ml->flags & rdxSPackageManifestLocal::PMLF_Duplicate)
						continue;
					// Don't load objects that have already been loaded
					if(!(objRef->ObjectInfo()->objectFlags & rdxGCInfo::GCOF_Unloaded))
					{
						// We do need to perform some processing on structures to resolve their defaults though.
						if(!criticality)
						{
							if(ml->resolvedObject->ObjectInfo()->containerType == m_builtins.st_StructuredType &&
								!(ml->resolvedObject.StaticCast<rdxCStructuredType>()->m_native.flags & rdxCStructuredType::NativeProperties::STF_FinalDefaultEvaluated) )
							{
								printf("Can't deserialize %s:%s, no default\n", ml->tempTypeGUID.m_domain.DebugStr(), ml->tempTypeGUID.DebugStr());
								bool loadedOK;
								defer = true;
								RDX_PROTECT_ASSIGN(ctx, loadedOK, ResolveStructureDefault(ctx, ml->resolvedObject.StaticCast<rdxCStructuredType>().ToWeakHdl(), pkg) );
								if(loadedOK)
									loadedAny = true;
							}
						}
						continue;
					}

					// Don't load objects of the wrong criticality
					if(((ml->flags & rdxSPackageManifestLocal::PMLF_Critical) != 0) != criticality)
						continue;

					// Validate method domain violations
					if(criticality == false
						&& ml->resolvedType == m_builtins.st_Method
						&& !packageHost->DomainCanContainMethods(pkg->m_native.domain))
					{
						RDX_STHROW(ctx, RDX_ERROR_DOMAIN_POLICY_VIOLATION);
					}

					if(!stream)
						stream = packageHost->StreamForDomain(this, pkg->m_native.domain, false, isText);

					// This is of the appropriate criticality.  If this is the default value, then we can load it into the default
					// of its owning type.  Otherwise, we need a final default.
					bool loadedOK;
					rdxWeakHdl(rdxCObject) obj = ml->resolvedObject.ToWeakHdl();
					RDX_PROTECT_ASSIGN( ctx, loadedOK, rdxDeserializeObject(ctx, this, pkg, obj, ml, pkg->m_native.domain, packageHost, stream, isText) );

					if(loadedOK)
					{
						loadedAny = true;

						// Assign it to the GST if it's the first instance
						if(!(ml->flags & rdxSPackageManifestLocal::PMLF_Anonymous))
						{
							gstInserts = true;
							RDX_PROTECT( ctx, AddGlobalSymbol(ctx, ml->objectGUID, ml->resolvedObject.ToWeakHdl()) );
						}

						objRef->ObjectInfo()->objectFlags &= ~rdxGCInfo::GCOF_Unloaded;
					}
					else
					{
						printf("Can't deserialize %s:%s, ???\n", ml->tempTypeGUID.m_domain.DebugStr(), ml->tempTypeGUID.DebugStr());
						defer = true;
					}
				}

				if(stream)
					stream->Close();
				stream = NULL;
			}

			if(defer && !loadedAny)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);	// Unresolvable dependency
		}
	}
	RDX_CATCH(ctx)
	{
		if(stream)
			stream->Close();
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

rdxCRef(rdxCPackage) rdxCObjectManager::LoadSinglePackage(rdxSOperationContext *ctx, rdxSDomainGUID domain, rdxIPackageHost *packageHost)
{
	rdxCRef(rdxCPackage) pkg = rdxCRef(rdxCPackage)::Null();
	rdxIFileStream *stream = NULL;

	RDX_TRY(ctx)
	{
		bool isText = false;

		rdxWeakHdl(rdxCPackage) pkgRef;
		if(m_packages.GetElement(&domain, &pkgRef))
			return pkgRef.ToCRef();

		isText = false;
		stream = packageHost->StreamForDomain(this, domain, false, isText);

		if(!stream)
			RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

		RDX_PROTECT_ASSIGN(ctx, pkg, rdxDeserializePackage(ctx, this, domain, packageHost, stream, isText) );

		stream->Close();
		stream = NULL;

		rdxWeakHdl(rdxCPackage) packageHdl = pkg.ToWeakHdl();
		RDX_PROTECT(ctx, m_packages.Insert(ctx, &domain, &packageHdl));

		pkg->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_UnloadedPackage;
		RDX_PROTECT(ctx, m_unloadedPackages.Add(ctx, this, packageHdl, m_builtins.aot_Object.ToWeakHdl()));
		return pkg;
	}
	RDX_CATCH(ctx)
	{
		if(stream)
			stream->Close();
		RDX_RETHROWV(ctx, rdxCRef(rdxCPackage)::Null());
	}
	RDX_ENDTRY
}

rdxCRef(rdxCObject) rdxCObjectManager::LoadObject(rdxSOperationContext *ctx, rdxIPackageHost *packageHost)
{
#ifdef RDX_ENABLE_SMP
	m_coreMutexes[rdxCOREMUTEX_SymbolTableAccess]->Acquire();
#endif

	RDX_TRY(ctx)
	{
		rdxCRef(rdxCPackage) pkg;
		RDX_PROTECT_ASSIGN(ctx, pkg, LoadPackage(ctx, rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime), packageHost) );

		// Throw out this package
		if(pkg.IsNotNull())
		{
			rdxSDomainGUID domain = rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime);
			bool found;
			m_packages.RemoveEntry(m_packages.FindElementIndex(&domain, found, NULL));
			m_packages.Balance();
		}

		if(pkg.IsNull() || pkg->m_native.localSymbols.IsNull() || pkg->m_native.localSymbols->NumElements() == 0)
			RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

#ifdef RDX_ENABLE_SMP
		m_coreMutexes[rdxCOREMUTEX_SymbolTableAccess]->Release();
#endif

		rdxCRef(rdxCObject) obj = pkg->m_native.firstObject.ToCRef();
		pkg->m_native.firstObject = rdxWeakRTRef(rdxCObject)::Null();
		return obj;
	}
	RDX_CATCH(ctx)
	{
		// The unloaded package will be garbage
#ifdef RDX_ENABLE_SMP
		m_coreMutexes[rdxCOREMUTEX_SymbolTableAccess]->Release();
#endif
		RDX_RETHROWV(ctx, rdxCRef(rdxCObject)::Null());
	}
	RDX_ENDTRY
}

void rdxCObjectManager::HardenStructures(rdxSOperationContext *ctx)
{
	RDX_TRY(ctx)
	{
		bool resolvedAny = false;
		bool firstPass = true;
		bool deferredAny = true;

		while(deferredAny)
		{
			resolvedAny = false;
			deferredAny = false;

			for(rdxLargeUInt pi=0;pi<m_unloadedPackages.m_numObjects;pi++)
			{
				rdxWeakHdl(rdxCPackage) pkg = m_unloadedPackages.m_objects->Element(pi).ToWeakHdl().StaticCast<rdxCPackage>();
				rdxLargeUInt nSymbols = pkg->m_native.localSymbols->NumElements();
				for(rdxLargeUInt oi=0;oi<nSymbols;oi++)
				{
					rdxWeakHdl(rdxCObject) obj = pkg->m_native.localSymbols->Element(oi).resolvedObject.ToWeakHdl();
					if(obj.IsNull() || (pkg->m_native.localSymbols->Element(oi).flags & rdxSPackageManifestLocal::PMLF_Duplicate) != 0)
						continue;

					rdxGCInfo *gci = obj->ObjectInfo();
					if(gci->containerType == m_builtins.st_StructuredType)
					{
						rdxWeakHdl(rdxCStructuredType) st = obj.StaticCast<rdxCStructuredType>();

						if(!(st->m_native.flags & rdxCStructuredType::NativeProperties::STF_StructureEvaluated) )
						{
							bool resolved;
							if(rdxSSerializationTag *serTag = st->ObjectInfo()->SerializationTag())
							{
								printf("Resolving %s::%s\n", serTag->gstSymbol.m_domain.DebugStr(), serTag->gstSymbol.DebugStr());
							}
							RDX_PROTECT_ASSIGN(ctx, resolved, ResolveStructure(ctx, st) );
							if(resolved)
							{
								resolvedAny = true;
							}
							else
							{
								deferredAny = true;
							}
						}
					}
					else if(gci->containerType == m_builtins.st_ArrayOfType)
					{
						if(firstPass)
						{
							rdxWeakHdl(rdxCArrayOfType) t = obj.StaticCast<rdxCArrayOfType>();
							if(t->type == m_builtins.st_Varying)
								RDX_STHROW(ctx, RDX_ERROR_FORBIDDEN_VARYING_USAGE);
							if(t->type.IsNull())
								RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
						}
					}
				}
			}

			if(!resolvedAny && deferredAny)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

			firstPass = false;
		}
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

void rdxCObjectManager::ValidateStructures(rdxSOperationContext* ctx)
{
	// Perform runtime safety validation on structures
	// NOTE: This is performed AFTER data loading, so checks that determine whether a type is safe
	// to load must occur at a different stage, such as ResolveStructure
	RDX_TRY(ctx)
	{
		for(rdxLargeUInt pi=0;pi<m_unloadedPackages.m_numObjects;pi++)
		{
			rdxWeakHdl(rdxCPackage) pkg = m_unloadedPackages.m_objects->Element(pi).ToWeakHdl().StaticCast<rdxCPackage>();
			rdxLargeUInt nSymbols = pkg->m_native.localSymbols->NumElements();
			for(rdxLargeUInt oi=0;oi<nSymbols;oi++)
			{
				rdxWeakHdl(rdxCObject) obj = pkg->m_native.localSymbols->Element(oi).resolvedObject.ToWeakHdl();
				if(obj.IsNull() || (pkg->m_native.localSymbols->Element(oi).flags & rdxSPackageManifestLocal::PMLF_Duplicate) != 0)
					continue;

				if(obj->ObjectInfo()->containerType == m_builtins.st_StructuredType)
					RDX_PROTECT(ctx, ValidateStructure(ctx, obj.StaticCast<rdxCStructuredType>()));
			}
		}
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

void rdxCObjectManager::ResolveVFTs(rdxSOperationContext *ctx)
{
}

void rdxCObjectManager::FinishPackageLoad(rdxSOperationContext* ctx)
{
	RDX_TRY(ctx)
	{
		if(m_codeProvider)
		{
			RDX_PROTECT(ctx, m_codeProvider->InitializeSymbolDictionary(ctx, this) );
		}

		// Move all non-duplicate objects to pending process
		
		for(rdxLargeUInt pi=0;pi<m_unloadedPackages.m_numObjects;pi++)
		{
			rdxWeakHdl(rdxCPackage) pkg = m_unloadedPackages.m_objects->Element(pi).ToWeakHdl().StaticCast<rdxCPackage>();
			rdxLargeUInt nSymbols = pkg->m_native.localSymbols->NumElements();
			for(rdxLargeUInt oi=0;oi<nSymbols;oi++)
			{
				rdxWeakHdl(rdxCObject) obj = pkg->m_native.localSymbols->Element(oi).resolvedObject.ToWeakHdl();
				if(obj.IsNull())
					RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);
				if((pkg->m_native.localSymbols->Element(oi).flags & rdxSPackageManifestLocal::PMLF_Duplicate) != 0)
					continue;
				obj->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_Unprocessed;
				RDX_PROTECT(ctx, m_unprocessedObjects.Add(ctx, this, obj, m_builtins.aot_Object.ToWeakHdl()));
			}
		}

		bool defer = true;
		while(defer)
		{
			bool loadedAny = false;
			defer = false;

			for(rdxLargeUInt oi=0;oi<m_unprocessedObjects.m_numObjects;oi++)
			{
				rdxWeakHdl(rdxCObject) obj = m_unprocessedObjects.m_objects->Element(oi).ToWeakHdl();
				if(obj.IsNull())
					continue;

				bool deferThis = false;

				rdxWeakHdl(rdxCType) containerType = obj->ObjectInfo()->containerType.ToWeakHdl();
				if(containerType.IsNotNull() && containerType->ObjectInfo()->containerType == m_builtins.st_StructuredType)
				{
					rdxWeakHdl(rdxCStructuredType) st = containerType.StaticCast<rdxCStructuredType>();
					rdxIfcTypeInfo typeInfo = st->m_native.nativeTypeInfo;

					if(typeInfo.Flags() & rdxETIF_OnLoad)
					{
						bool loadedOK;
						RDX_PROTECT_ASSIGN(ctx, loadedOK, typeInfo.TypeFuncs().GetOnLoadFunc()(ctx, this, obj.ToWeakHdl()));
						if(!loadedOK)
							deferThis = true;
						else
							loadedAny = true;
					}
				}

				if(!deferThis)
				{
					obj->ObjectInfo()->objectFlags &= ~rdxGCInfo::GCOF_Unprocessed;
					m_unprocessedObjects.m_objects->Element(oi) = rdxWeakRTRef(rdxCObject)::Null();
				}
				else
					defer = true;
			}

			if(defer && !loadedAny)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
		}

		m_unprocessedObjects.EvictAll();

		// Wipe all non-anchored references from packages
		for(rdxLargeUInt oi=0;oi<m_unloadedPackages.m_numObjects;oi++)
		{
			rdxWeakHdl(rdxCPackage) pkg = m_unloadedPackages.m_objects->Element(oi).ToWeakHdl().StaticCast<rdxCPackage>();
			if(pkg->m_native.localSymbols.IsNotNull())
			{
				rdxLargeUInt numLocalSymbols = pkg->m_native.localSymbols->NumElements();
				if(numLocalSymbols > 0 && pkg->m_native.domain == rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime))
					pkg->m_native.firstObject = pkg->m_native.localSymbols->Element(0).resolvedObject;
				for(rdxLargeUInt si=0;si<numLocalSymbols;si++)
				{
					rdxWeakOffsetRTRef(rdxSPackageManifestLocal) mfl = pkg->m_native.localSymbols->OffsetElementRTRef(si);
					if(mfl->flags & rdxSPackageManifestLocal::PMLF_Anonymous)
					{
						mfl->resolvedObject = rdxWeakRTRef(rdxCObject)::Null();
						mfl->resolvedType = rdxWeakRTRef(rdxCType)::Null();
					}
				}
			}
					
			if(pkg->m_native.importedSymbols.IsNotNull())
			{
				rdxLargeUInt numImportedSymbols = pkg->m_native.importedSymbols->NumElements();
				for(rdxLargeUInt si=0;si<numImportedSymbols;si++)
				{
					rdxWeakOffsetRTRef(rdxSPackageManifestImport) imp = pkg->m_native.importedSymbols->OffsetElementRTRef(si);
					imp->resolution = rdxWeakRTRef(rdxCObject)::Null();
				}
			}
			pkg->ObjectInfo()->objectFlags &= ~rdxGCInfo::GCOF_UnloadedPackage;
		}
		m_unloadedPackages.EvictAll();
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

void rdxCObjectManager::FinishDeserialize(rdxSOperationContext *ctx, rdxIPackageHost *packageHost)
{
	RDX_TRY(ctx)
	{
		// Cycle until all critical objects are loaded
		{
			bool shouldDefer = true;
			while(shouldDefer)
			{
				RDX_PROTECT_ASSIGN(ctx, shouldDefer, CreateInitialObjects(ctx, packageHost, true));
				RDX_PROTECT(ctx, ResolveImports(ctx, packageHost) );
			}
		}

		// All types should be resolved at this point
		RDX_PROTECT(ctx, GeneratePackageArrayDefs(ctx, packageHost) );

		// Resolve any other critical objects that became valid from the array creation
		RDX_PROTECT(ctx, CreateInitialObjects(ctx, packageHost, true));

		// Load initial critical objects
		RDX_PROTECT(ctx, LoadSymbolsOfCriticality(ctx, packageHost, true) );

		RDX_PROTECT(ctx, HardenStructures(ctx) );

		RDX_PROTECT(ctx, CreateInitialObjects(ctx, packageHost, false));
		bool resolved;
		RDX_PROTECT_ASSIGN(ctx, resolved, ResolveImports(ctx, packageHost) );
		if(!resolved)
			RDX_STHROW(ctx, RDX_ERROR_UNRESOLVED_SYMBOL_REFERENCE);
		RDX_PROTECT(ctx, LoadSymbolsOfCriticality(ctx, packageHost, false) );

		RDX_PROTECT(ctx, VerifyDuplicates(ctx, packageHost) );

		RDX_PROTECT(ctx, ValidateStructures(ctx));

		RDX_PROTECT(ctx, ResolveVFTs(ctx));

		RDX_PROTECT(ctx, FinishPackageLoad(ctx) );
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

rdxCRef(rdxCPackage) rdxCObjectManager::LoadPackage(rdxSOperationContext *ctx, rdxSDomainGUID domain, rdxIPackageHost *packageHost)
{
	// This function is not reentrant.  It will fully load one package and dependencies.  Individual packages are loaded via LoadSinglePackage.
	rdxCRef(rdxCPackage) pkg = rdxCRef(rdxCPackage)::Null();

	RDX_TRY(ctx)
	{
		m_forcePackagesRelevant = true;	// Make sure we don't collect unloaded packages

		RDX_PROTECT_ASSIGN(ctx, pkg, LoadSinglePackage(ctx, domain, packageHost) );
		
		for(rdxLargeUInt oi=0;oi<m_unloadedPackages.m_numObjects;oi++)
		{
			rdxWeakArrayHdl(rdxTracedRTRef(rdxCObject)) packageObjects = m_unloadedPackages.m_objects.ToWeakHdl();
			rdxLargeUInt nDimensions = packageObjects->NumElements();
			rdxWeakHdl(rdxCPackage) pkg = m_unloadedPackages.m_objects->Element(oi).ToWeakHdl().StaticCast<rdxCPackage>();
			if(pkg.IsNull())
				continue;

			if(pkg->m_native.importedSymbols.IsNotNull())
			{
				rdxLargeUInt nImports = pkg->m_native.importedSymbols->NumElements();
				for(rdxLargeUInt i=0;i<nImports;i++)
				{
					rdxWeakOffsetHdl(rdxSPackageManifestImport) import = pkg->m_native.importedSymbols->OffsetElementRTRef(i).ToHdl();
					RDX_PROTECT(ctx, LoadSinglePackage(ctx, import->objectGUID.m_domain, packageHost) );
				}
			}
		}

		RDX_PROTECT(ctx, FinishDeserialize(ctx, packageHost) );

		m_forcePackagesRelevant = false;
	}
	RDX_CATCH(ctx)
	{
		m_forcePackagesRelevant = false;

		// Evict processing lists for collection
		m_unloadedPackages.EvictAll();
		m_unprocessedObjects.EvictAll();

		RDX_RETHROWV(ctx, rdxCRef(rdxCPackage)::Null());
	}
	RDX_ENDTRY

	return pkg;
}

static void rdxWriteUTF8String(rdxWeakRTRef(rdxCString) str, rdxIFileStream *fs)
{
	const rdxChar *chars = str->AsChars()->ArrayData();
	rdxLargeUInt nChars = str->Length();

	{
		// Determine UTF8 length;
		rdxLargeUInt utf8len = 0;
		for(rdxLargeUInt i=0;i<nChars;i++)
		{
			rdxUInt8 encoded[10];
			utf8len += rdxEncodeUTF8Char(chars[i], encoded);
		}

		fs->WriteConverted<rdxLargeUInt, rdxBinPackageLargeUInt>(utf8len);
	}

	const rdxLargeUInt backlogSize = 50;
	const rdxLargeUInt backlogLimit = 45;
	rdxUInt8 backlog[backlogSize];
	rdxLargeUInt backlogOffset = 0;

	// Determine UTF8 length

	for(rdxLargeUInt i=0;i<nChars;i++)
	{
		backlogOffset += rdxEncodeUTF8Char(chars[i], backlog + backlogOffset);
		if(backlogOffset >= backlogLimit)
		{
			fs->WriteBytes(backlog, backlogOffset);
			backlogOffset = 0;
		}
	}

	if(backlogOffset)
		fs->WriteBytes(backlog, backlogOffset);
}


		
void rdxCObjectManager::SavePackageFile(rdxSOperationContext *ctx, rdxSDomainGUID domain, rdxIFileStream *fs,
	rdxWeakHdl(rdxCObject) object, rdxCObjectManager::ESaveMode saveMode, bool isText)
{
	FileWriteSerializer fws(domain, this);

	rdxISerializer *ser = &fws;

	// TODO: Cancel or complete any incremental GC
	this->StartGraphing(ser, false);

	switch(saveMode)
	{
	case SAVEMODE_Package:
		{
			rdxWeakHdl(rdxCPackage) pkg = object.StaticCast<rdxCPackage>();
				
			rdxWeakArrayHdl(rdxSPackageManifestLocal) symbols = pkg->m_native.localSymbols.ToWeakHdl();
			rdxLargeUInt nSymbols = symbols->NumElements();
			for(rdxLargeUInt i=0;i<nSymbols;i++)
			{
				rdxWeakOffsetHdl(rdxSPackageManifestLocal) ml = symbols->OffsetElementRTRef(i).ToHdl();

				GraphIncludeObject(&fws, ml->resolvedObject.ToWeakRTRef());
			}
		}
		break;
	case SAVEMODE_SingleObject:
		{
			GraphIncludeObject(&fws, object.ToWeakRTRef());
		}
		break;
	};

	ContinueGraph(&fws, false);

	rdxLargeUInt numIncludedObjects = 0;
	{
		rdxGCInfo *scanObj = m_scannedRoot.next[rdxGCL_ScanLink];
		while(scanObj != &m_scannedRoot)
		{
			numIncludedObjects++;
			scanObj = scanObj->next[rdxGCL_ScanLink];
		}
	}

	rdxCSSIDTable::Entry *objectSymbolIDs = m_allocator.CAlloc<rdxCSSIDTable::Entry>(numIncludedObjects, rdxALLOC_ShortTerm);
	if(!objectSymbolIDs)
	{
		fs->Abort();
		return;
	}

	// Count all object types and populate the SSID table
	rdxLargeUInt numImports = 0;
	rdxLargeUInt numLocalSymbols = 0;
	rdxLargeUInt numStrings = 0;
	rdxLargeUInt numArrayDefs = 0;

	rdxLargeUInt buckets[rdxPSL_Count];
	for(rdxLargeUInt i=0;i<rdxPSL_Count;i++)
		buckets[i] = 0;
	buckets[rdxPSL_Null] = 1;

	{
		rdxGCInfo *scanObj = m_scannedRoot.next[rdxGCL_ScanLink];
		while(scanObj != &m_scannedRoot)
		{
			if(scanObj->objectFlags & rdxGCInfo::GCOF_External)
				buckets[rdxPSL_Imported]++;
			else
			{
				if(scanObj->containerType == m_builtins.st_String)
					buckets[rdxPSL_String]++;
				else if(scanObj->containerType == m_builtins.st_ArrayOfType)
					buckets[rdxPSL_Array]++;
				else
					buckets[rdxPSL_Local]++;
			}
			scanObj = scanObj->next[rdxGCL_ScanLink];
		}
	}

	numImports = buckets[rdxPSL_Imported];
	numLocalSymbols = buckets[rdxPSL_Local];
	numStrings = buckets[rdxPSL_String];
	numArrayDefs = buckets[rdxPSL_Array];

	// Convert count buckets into offsets
	{
		rdxLargeUInt runningTotal = 0;
		for(rdxLargeUInt i=1;i<rdxPSL_Count;i++)
		{
			rdxLargeUInt offset = runningTotal;
			runningTotal += buckets[i];
			buckets[i] = offset;
		}
	}

	{
		rdxLargeUInt ssidIndex = 0;
		rdxGCInfo *scanObj = m_scannedRoot.next[rdxGCL_ScanLink];
		while(scanObj != &m_scannedRoot)
		{
			rdxLargeUInt ssid;
			if(scanObj->objectFlags & rdxGCInfo::GCOF_External)
				ssid = buckets[rdxPSL_Imported]++;
			else
			{
				if(scanObj->containerType == m_builtins.st_String)
					ssid = buckets[rdxPSL_String]++;
				else if(scanObj->containerType == m_builtins.st_ArrayOfType)
					ssid = buckets[rdxPSL_Array]++;
				else
					ssid = buckets[rdxPSL_Local]++;
			}
			
			objectSymbolIDs[ssidIndex].key = rdxCStaticLookupPODKey<rdxBaseHdl::PODType>(scanObj->ObjectWeakRTRef().ToWeakHdl().GetPOD());
			objectSymbolIDs[ssidIndex].value = ssid;
			ssidIndex++;
			scanObj = scanObj->next[rdxGCL_ScanLink];
		}
	}

	// Create the SSID table
	rdxCSSIDTable ssidTable(objectSymbolIDs, numIncludedObjects);

	rdxWriteHeader(this, fs, isText, numLocalSymbols, numImports, numStrings, numArrayDefs);

	// Write string table
	rdxLargeUInt stringTableLocation = fs->Tell();
	if(!isText)
	{
		rdxGCInfo *scanObj = m_scannedRoot.next[rdxGCL_ScanLink];
		while(scanObj != &m_scannedRoot)
		{
			if(!(scanObj->objectFlags & rdxGCInfo::GCOF_External))
			{
				if(scanObj->containerType == m_builtins.st_String)
					rdxWriteUTF8String(scanObj->ObjectWeakRTRef().StaticCast<rdxCString>(), fs);
			}
			scanObj = scanObj->next[rdxGCL_ScanLink];
		}
	}
	
	RDX_VERBOSE( printf("Array table written to %i\n", fs->Tell()) );
	
	// Write array table
	rdxLargeUInt arrayTableLocation = fs->Tell();
	if(!isText)
	{
		rdxGCInfo *scanObj = m_scannedRoot.next[rdxGCL_ScanLink];
		while(scanObj != &m_scannedRoot)
		{
			if(!(scanObj->objectFlags & rdxGCInfo::GCOF_External))
			{
				if(scanObj->containerType == m_builtins.st_ArrayOfType)
				{
					rdxWeakRTRef(rdxCArrayOfType) aot = scanObj->ObjectWeakRTRef().StaticCast<rdxCArrayOfType>();
					rdxWriteValue(this, fs, isText, &ssidTable, m_builtins.st_ArrayOfType.ToWeakHdl(), aot.OffsetMember(&rdxCArrayOfType::type));
					rdxWriteValue(this, fs, isText, &ssidTable, m_builtins.st_LargeUInt.ToWeakHdl(), aot.OffsetMember(&rdxCArrayOfType::numDimensions));
					rdxWriteValue(this, fs, isText, &ssidTable, m_builtins.st_Bool.ToWeakHdl(), aot.OffsetMember(&rdxCArrayOfType::isConstant));
				}
			}
			scanObj = scanObj->next[rdxGCL_ScanLink];
		}
	}

	RDX_VERBOSE( printf("String table written to %i\n", fs->Tell()) );


	// Write catalogs
	{
		rdxGCInfo *scanObj = m_scannedRoot.next[rdxGCL_ScanLink];
		while(scanObj != &m_scannedRoot)
		{
			if(scanObj->objectFlags & rdxGCInfo::GCOF_External)
			{
				rdxSSerializationTag *serTag = scanObj->SerializationTag();
				if(!serTag || serTag->isAnonymous)
					RDX_LTHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);	// Why is this external if it's anonymous?
				rdxWriteImport(ctx, this, fs, isText, &ssidTable, serTag->gstSymbol);
			}
			scanObj = scanObj->next[rdxGCL_ScanLink];
		}
	}

	RDX_VERBOSE( printf("Import catalog written to %i\n", fs->Tell()) );

	rdxLargeUInt exportCatalogStartLocation = fs->Tell();
	{
		rdxGCInfo *scanObj = m_scannedRoot.next[rdxGCL_ScanLink];
		while(scanObj != &m_scannedRoot)
		{
			if(!(scanObj->objectFlags & rdxGCInfo::GCOF_External))
			{
				if(scanObj->containerType != m_builtins.st_String)
					rdxReserveObjectExport(this, fs, isText, &ssidTable, scanObj->ObjectWeakRTRef().ToWeakHdl());
			}
			scanObj = scanObj->next[rdxGCL_ScanLink];
		}
	}
				
	// Write objects
	{
		rdxLargeUInt catalogLocation = exportCatalogStartLocation;
		rdxLargeUInt objectLocation = fs->Tell();
		rdxGCInfo *scanObj = m_scannedRoot.next[rdxGCL_ScanLink];
		while(scanObj != &m_scannedRoot)
		{
			if(!(scanObj->objectFlags & rdxGCInfo::GCOF_External))
			{
				if(scanObj->containerType != m_builtins.st_String)
				{
					rdxLargeUInt objectStartLocation = fs->Tell();
					rdxWriteObject(this, fs, isText, &ssidTable, scanObj->ObjectWeakRTRef());
					rdxWriteObjectExport(this, fs, isText, &catalogLocation, objectStartLocation, scanObj->ObjectWeakRTRef());
				}
			}
			scanObj = scanObj->next[rdxGCL_ScanLink];
		}
	}

	// Unlink everything
	// TODO MUSTFIX: Figure out if this is actually necessary.  Probably isn't.
	if(false)
	{
		rdxLargeUInt objectIndex = 0;
		rdxGCInfo *scanObj = m_markedRoot.next[rdxGCL_ScanLink];
		while(scanObj != &m_markedRoot)
		{
			rdxGCInfo *next = scanObj->next[rdxGCL_ScanLink];
			scanObj->Unlink(rdxGCL_ScanLink);
			scanObj = next;
		}
	}

	m_allocator.Free(objectSymbolIDs);
}

void rdxCObjectManager::SaveObject(rdxSOperationContext *ctx, rdxWeakHdl(rdxCObject) object, rdxIFileStream *fs, bool isText)
{
	if(object->ObjectInfo()->Domain() != rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime))
		RDX_LTHROW(ctx, RDX_ERROR_SAVED_PACKAGED_OBJECT);

	if(!this->m_singleThreadMode)
		RDX_LTHROW(ctx, RDX_ERROR_SMP_OPERATION_FORBIDDEN);

	RDX_PROTECT(ctx, CollectGarbage(ctx));

	RDX_TRY(ctx)
	{
		RDX_PROTECT(ctx, SavePackageFile(ctx, rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime), fs, object, SAVEMODE_SingleObject, isText) );
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

void rdxCObjectManager::SavePackage(rdxSOperationContext *ctx, rdxSDomainGUID domain, rdxIPackageHost *packageHost)
{
	if(!this->m_singleThreadMode)
		RDX_LTHROW(ctx, RDX_ERROR_SMP_OPERATION_FORBIDDEN);

	rdxWeakHdl(rdxCPackage) pkg;
	if(!m_packages.GetElement(&domain, &pkg))
	{
		return;
	}

	bool isText;
	rdxIFileStream *fs = packageHost->StreamForDomain(this, domain, true, isText);

	if(fs)
	{
		SavePackageFile(ctx, domain, fs, pkg, SAVEMODE_Package, isText);
		fs->Close();
	}
}

void rdxCObjectManager::RegisterPackage(rdxSOperationContext *ctx, rdxSDomainGUID domain, rdxWeakHdl(rdxCPackage) pkg)
{
	m_packages.Insert(ctx, &domain, &pkg);
}

bool rdxCObjectManager::PackageLoaded(rdxSDomainGUID domain)
{
	return m_packages.ContainsKey(&domain);
}


rdxCRef(rdxCObject) rdxCObjectManager::LookupSymbolSimple(rdxSOperationContext *ctx, rdxSObjectGUID symbolName)
{
#ifdef RDX_ENABLE_SMP
	m_coreMutexes[rdxCOREMUTEX_SymbolTableAccess]->Acquire();
#endif

	rdxWeakHdl(rdxCObject) rv = rdxWeakHdl(rdxCObject)::Null();
	if(m_gst.GetElement(&symbolName, &rv))
	{
		return rv.ToCRef();
	}
#ifdef RDX_ENABLE_SMP
	m_coreMutexes[rdxCOREMUTEX_SymbolTableAccess]->Release();
#endif
	return rv.ToCRef();
}

rdxCRef(rdxCObject) rdxCObjectManager::LookupSymbol(rdxSOperationContext *ctx, rdxSObjectGUID symbolName, rdxIPackageHost *packageHost)
{
#ifdef RDX_ENABLE_SMP
	m_coreMutexes[rdxCOREMUTEX_SymbolTableAccess]->Acquire();
#endif

	rdxCRef(rdxCObject) rv;
	rv = this->LookupSymbolSimple(ctx, symbolName);
	if(rv.IsNotNull())
	{
#ifdef RDX_ENABLE_SMP
		m_coreMutexes[rdxCOREMUTEX_SymbolTableAccess]->Release();
#endif
		return rv;
	}

	rdxSDomainGUID domain = symbolName.m_domain;

	RDX_TRY(ctx)
	{
		rdxCRef(rdxCPackage) pkg;
		RDX_PROTECT_ASSIGN(ctx, pkg, this->LoadPackage(ctx, domain, packageHost) );
		rv = this->LookupSymbolSimple(ctx, symbolName);

#ifdef RDX_ENABLE_SMP
		m_coreMutexes[rdxCOREMUTEX_SymbolTableAccess]->Release();
#endif
		return rv;
	}
	RDX_CATCH(ctx)
	{
#ifdef RDX_ENABLE_SMP
		m_coreMutexes[rdxCOREMUTEX_SymbolTableAccess]->Release();
#endif
		RDX_RETHROWV(ctx, rdxCRef(rdxCObject)::Null());
	}
	RDX_ENDTRY
}

/*
void rdxCObjectManager::ComputeGUID(const char *name, rdxUInt8 *output)
{
	rdxCDualCRC32Generator generator;
	generator.Digest(name, strlen(name));
	generator.Finish(output);
}
*/

void rdxCObjectManager::ComputeGUID(const char *name, rdxUInt8 *output)
{
	RDX_ComputeGUID(name, output);
}

void rdxCObjectManager::ComputeGUID(const rdxSCharSpan &charSpan, rdxUInt8 *output)
{
	// This must match RDX_ComputeGUID
	rdxCSHA256Generator generator;
	rdxLargeUInt sz = charSpan.Length();
	const rdxChar *charData = charSpan.Chars();
	for(rdxLargeUInt i=0;i<sz;i++)
	{
		rdxUInt8 utfBytes[10];
		rdxLargeUInt nBytes = rdxEncodeUTF8Char(charData[i], utfBytes);
		generator.FeedBytes(utfBytes, nBytes);
	}
	rdxUInt8 flushed[generator.OUTPUT_SIZE_BYTES];
	generator.Flush(flushed);
	memcpy(output, flushed, rdxSDomainGUID::GUID_SIZE);

	rdxStaticAssert(sizeof(flushed) >= rdxSDomainGUID::GUID_SIZE);
}

void rdxCObjectManager::ComputeGUID(rdxWeakRTRef(rdxCString) str, rdxUInt8 *output)
{
	ComputeGUID(str->AsCharSpan(), output);
}

rdxSDomainGUID rdxCObjectManager::ComputeDomainGUID(const char *domainName)
{
	rdxSDomainGUID guid;
	ComputeGUID(domainName, guid.m_bytes);
	guid.m_debugStr = new char[strlen(domainName) + 1];
	strcpy(guid.m_debugStr, domainName);
	return guid;
}

rdxSDomainGUID rdxCObjectManager::ComputeDomainGUID(rdxWeakRTRef(rdxCString) domainName)
{
	return ComputeDomainGUID(domainName->AsCharSpan());
}

rdxSDomainGUID rdxCObjectManager::ComputeDomainGUID(const rdxSCharSpan &domainName)
{
	rdxSDomainGUID guid;
	ComputeGUID(domainName, guid.m_bytes);
	guid.m_debugStr = new char[domainName.Length() + 1];
	for(rdxLargeUInt i=0;i<domainName.Length();i++)
		guid.m_debugStr[i] = static_cast<char>(domainName.Chars()[i]);
	guid.m_debugStr[domainName.Length()] = '\0';
	return guid;
}

rdxSObjectGUID rdxCObjectManager::ComputeObjectGUID(rdxSDomainGUID domain, const char *objectName)
{
	rdxSObjectGUID guid;
	ComputeGUID(objectName, guid.m_bytes);
	guid.m_domain = domain;
	guid.m_debugStr = new char[strlen(objectName) + 1];
	strcpy(guid.m_debugStr, objectName);
	return guid;
}

void rdxCObjectManager::AddUnloadedObject(rdxSOperationContext *ctx, rdxWeakHdl(rdxCObject) object)
{
	// TODO MUSTFIX
}

void rdxCObjectManager::InitializeObject(rdxWeakRTRef(rdxCObject) obj, rdxLargeUInt overflow, bool forceZeroFill)
{
	rdxWeakRTRef(rdxCType) objType = obj->ObjectInfo()->containerType.ToWeakRTRef();
	if(objType.IsNull())
	{
		// Can't do anything
		return;
	}

	rdxWeakRTRef(rdxCType) objTypeType = objType->ObjectInfo()->containerType.ToWeakRTRef();
	rdxGCInfo *gci = obj->ObjectInfo();

	if(objTypeType == m_builtins.st_ArrayOfType)
	{
		gci->vftUnion.vftRef = m_builtins.st_Array->virtualMethods.ToWeakRTRef().GetPOD();
	}
	else if(objTypeType == m_builtins.st_StructuredType)
	{
		gci->vftUnion.vftRef = objType.StaticCast<rdxCStructuredType>()->virtualMethods.ToWeakRTRef().GetPOD();
	}

	if(forceZeroFill)
	{
		// TODO: Remove this case, zero-fill should happen on alloc
		return;
	}

	if(objTypeType == m_builtins.st_ArrayOfType)
	{
		rdxWeakRTRef(rdxCArrayOfType) aot = objTypeType.StaticCast<rdxCArrayOfType>();
		rdxWeakRTRef(rdxCType) aotContentsType = aot->type.ToWeakRTRef();
		rdxWeakRTRef(rdxCType) aotContentsTypeType = aotContentsType->ObjectInfo()->containerType;
		if(aotContentsTypeType == m_builtins.st_StructuredType)
		{
			rdxWeakRTRef(rdxCStructuredType) st = aotContentsType.StaticCast<rdxCStructuredType>();
			rdxWeakRTRef(rdxCObject) defaultValue = st->m_native.currentDefaultValue.ToWeakRTRef();
			if(defaultValue.IsNotNull() && (st->storageSpecifier == rdxSS_RefStruct || st->storageSpecifier == rdxSS_ValStruct))
			{
				const void *copySrc = defaultValue.StaticCast<rdxCStructContainer>()->GetRawData();
				void *copyDest = obj.StaticCast<rdxCArrayContainer>()->ModifyRawData();
				rdxLargeUInt copySize = st->m_native.size;
				rdxLargeUInt copyCount = obj.StaticCast<rdxCArrayContainer>()->NumElements();

				for(rdxLargeUInt i=0;i<copyCount;i++)
				{
					rdxBlockCopy(copyDest, copySrc, copySize);
					copyDest = reinterpret_cast<rdxUInt8*>(copyDest) + copySize;
					copySrc = reinterpret_cast<const rdxUInt8*>(copySrc) + copySize;
				}
			}
		}
	}
	else if(objTypeType == m_builtins.st_StructuredType)
	{
		rdxWeakRTRef(rdxCStructuredType) st = objType.StaticCast<rdxCStructuredType>();
		rdxWeakRTRef(rdxCObject) defaultValue = st->m_native.currentDefaultValue.ToWeakRTRef();
		if(defaultValue.IsNotNull())
		{
			if(st->storageSpecifier == rdxSS_RefStruct || st->storageSpecifier == rdxSS_ValStruct)
			{
				rdxBlockCopy(obj.StaticCast<rdxCStructContainer>()->ModifyRawData(), defaultValue.StaticCast<rdxCStructContainer>()->GetRawData(), st->m_native.size);
			}
			else if(st->storageSpecifier == rdxSS_Class)
			{
				CopyClassDefault(obj.Modify(), defaultValue.Data(), st.Data());
			}
		}
	}
	else if(objTypeType == m_builtins.st_DelegateType)
	{
		// TODO: Error
		rdxDebugBreak(rdxBREAKCAUSE_Unimplemented);
	}
	else
	{
		// TODO: Error
		rdxDebugBreak(rdxBREAKCAUSE_Unimplemented);
	}
}

rdxSObjectGUID rdxCObjectManager::ComputeObjectGUID(rdxSDomainGUID domain, const rdxSCharSpan &objectName)
{
	rdxSObjectGUID guid;
	ComputeGUID(objectName, guid.m_bytes);
	guid.m_domain = domain;
	guid.m_debugStr = new char[objectName.Length() + 1];
	for(rdxLargeUInt i=0;i<objectName.Length();i++)
		guid.m_debugStr[i] = static_cast<char>(objectName.Chars()[i]);
	guid.m_debugStr[objectName.Length()] = '\0';
	return guid;
}

rdxSObjectGUID rdxCObjectManager::ComputeObjectGUID(rdxSDomainGUID domain, rdxWeakRTRef(rdxCString) objectName)
{
	return ComputeObjectGUID(domain, objectName->AsCharSpan());
}

RDX_DYNLIB_API void rdxXAPI_Create1DArrayCommon(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxLargeUInt count, rdxWeakHdl(rdxCArrayOfType) t, rdxSDomainGUID domain, rdxLargeUInt overflow, rdxLargeUInt stride, rdxIfcTypeInfo contentsTypeInfo, rdxIfcTypeInfo arrayTypeInfo, rdxCRef(rdxCArrayContainer) *result)
{
	RDX_TRY(ctx)
	{
		rdxCRef(rdxCArrayContainer) container;
		RDX_PROTECT_ASSIGN(ctx, container, objm->CreateArrayContainer(ctx, stride, count, 1, domain, overflow, t, arrayTypeInfo));
		rdxLargeUInt dimOffset;
		RDX_PROTECT(ctx, rdxCArrayContainer::ComputeContainerSize(ctx, stride, count, 1, &dimOffset, RDX_CNULL));
		container->InitializeArray(count, overflow, stride, dimOffset, contentsTypeInfo);
		*result = container;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}


RDX_DYNLIB_API void rdxXAPI_Create1DArrayIntuitive(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxLargeUInt count, rdxWeakHdl(rdxCArrayOfType) t, rdxSDomainGUID domain, rdxLargeUInt stride, rdxCRef(rdxCArrayContainer) *result)
{
	rdxWeakRTRef(rdxCType) contentsType = t->type.ToWeakRTRef();
	rdxWeakRTRef(rdxCType) ttype = contentsType->ObjectInfo()->containerType.ToWeakRTRef();
	rdxSBuiltIns *builtins = objm->GetBuiltIns();
	rdxIfcTypeInfo contentsTypeIfc;
	rdxIfcTypeInfo arrayTypeIfc;
	if(objm->TypeIsObjectReference(ttype))
	{
		contentsTypeIfc = objm->GetBasicReferenceTypeInfo();
		arrayTypeIfc = objm->GetBasicReferenceArrayTypeInfo();
	}
	else if(objm->TypeIsInterface(ttype))
	{
		contentsTypeIfc = objm->GetBasicInterfaceTypeInfo();
		arrayTypeIfc = objm->GetBasicInterfaceArrayTypeInfo();
	}
	else if(ttype == builtins->st_StructuredType)
	{
		rdxWeakRTRef(rdxCStructuredType) st = ttype.StaticCast<rdxCStructuredType>();
		if(st->storageSpecifier == rdxSS_RefStruct || st->storageSpecifier == rdxSS_ValStruct || st->storageSpecifier == rdxSS_Enum)
		{
			arrayTypeIfc = objm->GetBasicValueArrayTypeInfo();
			contentsTypeIfc = st->m_native.nativeTypeInfo;
		}
		else
		{
			RDX_LTHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);
		}
	}
	else
	{
		RDX_LTHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);
	}

	rdxXAPI_Create1DArrayCommon(ctx, objm, count, t, domain, 0, stride, contentsTypeIfc, arrayTypeIfc, result);
}

//========================================================================================
rdxIObjectManager *rdxCreateObjectManager(rdxSAllocator alloc, const rdxINativeTypeHost *nth, const rdxICodeProvider *cp)
{
	rdxCObjectManager *om = NULL;
	{
		rdxSOperationContext ctx(NULL);

		om = alloc.CAlloc<rdxCObjectManager>(1, rdxALLOC_LongTerm);
		if(!om)
			return NULL;
		
		new (om) rdxCObjectManager(alloc, nth, cp);

		RDX_TRY(&ctx)
		{
			RDX_PROTECT(&ctx, om->InitializeCoreMutexes(&ctx) );
		}
		RDX_CATCH(&ctx)
		{
			if(om)
				alloc.Free(om);
			return NULL;
		}
		RDX_ENDTRY
	}

	// Mutexes are set up, get a real context and perform remaining init operations
	{
		rdxSOperationContext ctx(om);

		RDX_NTRY(EH2, &ctx)
		{
			RDX_NPROTECT(EH2, &ctx, om->InitializeBuiltInTypes(&ctx) );
			om->CollectGarbage(&ctx);
		}
		RDX_NCATCH(EH2, &ctx)
		{
			om->CollectGarbage(&ctx);
			if(om)
				alloc.Free(om);
			return NULL;
		}
		RDX_NENDTRY(EH2)

	}

	return om;
}

// TODO MUSTFIX: Move me!
rdxSDomainGUID rdxGCInfo::Domain() const
{
	rdxSSerializationTag *serTag = this->SerializationTag();
	if(!serTag)
		return rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime);
	return serTag->gstSymbol.m_domain;
}

// TODO MUSTFIX: Move me!
rdxWeakHdl(rdxCStructuredType) rdxGCInfo::ContainedStructure() const
{
	rdxWeakHdl(rdxCType) cType = this->containerType.ToWeakHdl();
	if(cType.IsNull())
		return rdxWeakHdl(rdxCStructuredType)::Null();
	rdxWeakHdl(rdxCType) cTypeContainerType = cType->ObjectInfo()->containerType.ToWeakHdl();
	const rdxSBuiltIns *builtins = this->ownerObjectManager->GetBuiltIns();

	if(cTypeContainerType == builtins->st_ArrayOfType)
	{
		rdxWeakHdl(rdxCType) contentsType = cType.StaticCast<rdxCArrayOfType>()->type.ToWeakHdl();
		rdxWeakHdl(rdxCType) contentsTypeType = contentsType->ObjectInfo()->containerType.ToWeakHdl();
		if(contentsTypeType == builtins->st_StructuredType)
		{
			rdxWeakHdl(rdxCStructuredType) contentsST = contentsType.StaticCast<rdxCStructuredType>();
			switch(contentsST->storageSpecifier)
			{
			case rdxSS_RefStruct:
			case rdxSS_ValStruct:
			case rdxSS_Enum:
				return contentsST;
				break;
			};
		}
	}
	else if(cTypeContainerType == builtins->st_StructuredType)
	{
		return cType.StaticCast<rdxCStructuredType>();
	}
	return rdxWeakHdl(rdxCStructuredType)::Null();
}



void rdxGCInfo::Release(const rdxSAllocator &alloc)
{
	void *dataRoot = typeInfo.TypeFuncs().GetRefToObjectHeadFunc()(m_objectDataPtr);
	alloc.Free(dataRoot);
	alloc.Free(this);
}


RDX_IMPLEMENT_COMPLEX_NATIVE_CLASS(rdxCObject, (rdxETIF_NoFlags));

RDX_IMPLEMENT_COMPLEX_NATIVE_CLASS(rdxCStructContainer, (rdxETIF_VisitReferences | rdxETIF_BoxFlag));
RDX_IMPLEMENT_COMPLEX_NATIVE_CLASS(rdxCArrayContainer, (rdxETIF_VisitReferences | rdxETIF_ArrayFlag));

////////////////////////////////////////////////////////////////////////
RDX_IMPLEMENT_COMPLEX_NATIVE_CLASS(rdxCType, rdxETIF_NoFlags);

RDX_BEGIN_PROPERTY_LOOKUP_CLASS(rdxCType)
RDX_END_PROPERTY_LOOKUP

rdxCType::rdxCType(rdxIObjectManager *objm, rdxGCInfo *gci)
	: rdxCObject(objm, gci)
{
}

////////////////////////////////////////////////////////////////////////
RDX_IMPLEMENT_COMPLEX_NATIVE_CLASS(rdxCArrayOfType, (rdxETIF_NoFlags));

RDX_BEGIN_PROPERTY_LOOKUP_CLASS(rdxCArrayOfType)
	RDX_DEFINE_LOOKUP_PROPERTY(type)
	RDX_DEFINE_LOOKUP_PROPERTY(numDimensions)
	RDX_DEFINE_LOOKUP_PROPERTY(isConstant)
RDX_END_PROPERTY_LOOKUP

rdxCArrayOfType::rdxCArrayOfType(rdxIObjectManager *objm, rdxGCInfo *gci)
	: rdxCType(objm, gci)
{
}

////////////////////////////////////////////////////////////////////////
RDX_IMPLEMENT_COMPLEX_NATIVE_CLASS(rdxCDelegateType, (rdxETIF_NoFlags));

RDX_BEGIN_PROPERTY_LOOKUP_CLASS(rdxCDelegateType)
	RDX_DEFINE_LOOKUP_PROPERTY(parameters)
	RDX_DEFINE_LOOKUP_PROPERTY(returnTypes)
RDX_END_PROPERTY_LOOKUP

rdxCDelegateType::rdxCDelegateType(rdxIObjectManager *objm, rdxGCInfo *gci)
	: rdxCType(objm, gci)
{
}
