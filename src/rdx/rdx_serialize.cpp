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
#include <stdio.h>

#include "rdx.h"
#include "rdx_objectmanagement.hpp"
#include "rdx_objectmanagement_internal.hpp"
#include "rdx_programmability.hpp"
#include "rdx_hashmap.hpp"
#include "rdx_io.hpp"
#include "rdx_lut.hpp"
#include "rdx_pragmas.hpp"
#include "rdx_constants.hpp"
#include "rdx_package.hpp"
#include "rdx_loadshell.hpp"
#include "rdx_lutdecls.hpp"
#include "rdx_coretypeattribs.hpp"
#include "rdx_typeprocessordefs.hpp"

RDX_IMPLEMENT_COMPLEX_NATIVE_CLASS(rdxCPackage, rdxETIF_VisitReferences);
RDX_IMPLEMENT_COMPLEX_NATIVE_STRUCT(rdxSPackageManifestImport, rdxETIF_VisitReferences);
RDX_IMPLEMENT_COMPLEX_NATIVE_STRUCT(rdxSPackageManifestLocal, rdxETIF_VisitReferences);
RDX_IMPLEMENT_COMPLEX_NATIVE_STRUCT(rdxSPackageManifestString, rdxETIF_VisitReferences);
RDX_IMPLEMENT_COMPLEX_NATIVE_STRUCT(rdxSPackageArrayDef, rdxETIF_VisitReferences);

RDX_IMPLEMENT_COMPLEX_NATIVE_STRUCT(rdxSLoadShell, rdxETIF_NoFlags);

void rdxCPackage::VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
{
	if(!visitNonSerializable)
		return;

	visitor->VisitReference(objm, m_native.importedSymbols);
	visitor->VisitReference(objm, m_native.localSymbols);
	visitor->VisitReference(objm, m_native.strings);
	visitor->VisitReference(objm, m_native.firstObject);
	visitor->VisitReference(objm, m_native.arrayDefs);
}


class rdxCTextFileParser : public rdxITextDeserializer
{
public:
	static const rdxLargeUInt BUFFER_SIZE = 1024;
	static const rdxLargeUInt BUFFER_FRONTLOAD = 1024;
	static const rdxLargeUInt STATIC_STRING_SIZE = 1024;

private:
	unsigned char m_backlog[BUFFER_SIZE+BUFFER_FRONTLOAD];
	unsigned char *m_debugLocalized;
	rdxLargeUInt m_bufferOffset;
	rdxLargeUInt m_position;
	rdxLargeUInt m_backlogAvailable;

	rdxIFileStream *m_stream;

	void RefillBacklog()
	{
		if(m_backlogAvailable < BUFFER_FRONTLOAD)
		{
			if(m_backlogAvailable)
				memmove(m_backlog, m_backlog + m_bufferOffset, static_cast<rdxLargeUInt>(m_backlogAvailable));
			m_bufferOffset = 0;
			m_backlogAvailable += m_stream->ReadBytes(m_backlog + m_backlogAvailable, BUFFER_SIZE+BUFFER_FRONTLOAD - m_backlogAvailable);
		}
	}

public:
	rdxLargeUInt WriteBytes(const void *src, rdxLargeUInt numBytes) RDX_OVERRIDE
	{
		return m_stream->WriteBytes(src, numBytes);
	}

	void Close() RDX_OVERRIDE
	{
		// This doesn't close because it's on top of a separately-managed stream
	}

	rdxLargeUInt Tell() RDX_OVERRIDE
	{
		return m_position;
	}

	rdxLargeUInt PeekBytes(rdxByte *dest, rdxLargeUInt count)
	{
		if(m_backlogAvailable < count)
			return PeekBytes(dest, m_backlogAvailable);

		rdxBlockCopy(dest, m_backlog + m_bufferOffset, static_cast<size_t>(count));

		return count;
	}

	void FinishSeek()
	{
		m_position = m_stream->Tell();
		m_backlogAvailable = 0;
		m_bufferOffset = 0;
		RefillBacklog();
	}

	void SeekEnd(rdxLargeInt offset) RDX_OVERRIDE
	{
		m_stream->SeekEnd(offset);
		FinishSeek();
	}

	void SeekCurrent(rdxLargeInt offset) RDX_OVERRIDE
	{
		m_stream->SeekCurrent(offset);
		FinishSeek();
	}

	void SeekForward(rdxLargeUInt offset) RDX_OVERRIDE
	{
		m_stream->SeekForward(offset);
		FinishSeek();
	}

	void SeekStart(rdxLargeUInt offset) RDX_OVERRIDE
	{
		m_stream->SeekStart(offset);
		FinishSeek();
	}

	rdxLargeUInt ReadBytes(void *dest, rdxLargeUInt count) RDX_OVERRIDE
	{
		rdxLargeUInt sz = PeekBytes(static_cast<rdxUInt8 *>(dest), count);

		m_backlogAvailable -= sz;
		m_bufferOffset += sz;

		if(sz < count)
			sz = sz + m_stream->ReadBytes(static_cast<rdxUInt8 *>(dest) + sz, count - sz);
		m_position += sz;

		RefillBacklog();

		return sz;
	}

	static bool IsPunctuation(rdxUInt8 b)
	{
		return b == ':' || b == '{' || b == '}' || b == ',';
	}

	static bool IsWhitespace(rdxUInt8 b)
	{
		return b <= ' ';
	}

	bool CheckToken(const char *str)
	{
		rdxLargeUInt len = strlen(str);

		m_debugLocalized = m_backlog + m_bufferOffset;
		if(m_backlogAvailable >= len && !memcmp(m_backlog + m_bufferOffset, str, static_cast<size_t>(len)) &&
			(
				(m_backlogAvailable == len || IsWhitespace(m_backlog[m_bufferOffset + len]) || IsPunctuation(m_backlog[m_bufferOffset + len])
			)))
			return true;
		return false;
	}

	static void FlushBytes(rdxSOperationContext *ctx, rdxIObjectManager *om, const rdxByte *staticString, rdxLargeUInt appendLength, rdxArrayCRef(rdxUInt8) *pbytes)
	{
		RDX_TRY(ctx)
		{
			rdxLargeUInt currentCapacity = 0;
			if(pbytes->IsNotNull())
				currentCapacity = pbytes->Data()->NumElements();

			rdxArrayCRef(rdxUInt8) newBytes;
			RDX_PROTECT_ASSIGN(ctx, newBytes, rdxCInternalObjectFactory::Create1DArray<rdxUInt8>(ctx, om, currentCapacity + appendLength));
			rdxUInt8 *outBytesView = newBytes->ArrayModify();
			if(currentCapacity)
				rdxBlockCopy(outBytesView, pbytes->Data(), currentCapacity + appendLength);
			if(appendLength)
				rdxBlockCopy(outBytesView + currentCapacity, staticString, appendLength);
			*pbytes = newBytes;
		}
		RDX_CATCH(ctx)
		{
			RDX_RETHROW(ctx);
		}
		RDX_ENDTRY
	}

	void ParseToken(rdxSOperationContext *ctx, rdxIObjectManager *objm, bool *pIsString, SCompactToken *outToken = RDX_CNULL) RDX_OVERRIDE
	{
		rdxByte staticString[STATIC_STRING_SIZE];
		rdxArrayCRef(rdxUInt8) bytes = rdxArrayCRef(rdxUInt8)::Null();
		rdxUInt8 b;
		bool parseAsString = false;

		RDX_TRY(ctx)
		{
			while(true)
			{
				if(!PeekBytes(&b, 1))
					RDX_LTHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
				if(!IsWhitespace(b))
					break;
				ReadBytes(&b, 1);
			}

			PeekBytes(&b, 1);
			if(b == '\'')
			{
				parseAsString = true;
				ReadBytes(&b, 1);
			}

			rdxLargeUInt loadedStatic = 0;
			bool firstCharacter = true;

			while(1)
			{
				if(!PeekBytes(&b, 1))
				{
					if(parseAsString)
						RDX_LTHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
					break;
				}

				if(parseAsString && b == '\'')
				{
					ReadBytes(&b, 1);
					break;
				}
				else if(!parseAsString && (IsWhitespace(b) || (IsPunctuation(b) && !firstCharacter)))
					break;

				firstCharacter = false;

				if(b == '\\')
				{
					unsigned char sequence[3];
					if(!PeekBytes(sequence, 2))
						RDX_LTHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

					if(sequence[1] == '\\' || sequence[1] == '\'')
					{
						b = sequence[1];
						ReadBytes(sequence, 2);
					}
					else if(sequence[1] >= '0' && sequence[1] <= '9')
					{
						if(!PeekBytes(sequence, 3))
							RDX_LTHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
						if(sequence[2] < '0' || sequence[2] > '9')
							RDX_LTHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
						b = static_cast<rdxUInt8>((sequence[1]-'0')*10 + (sequence[2]-'0'));
						ReadBytes(sequence, 3);
					}
					else
						RDX_LTHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
				}
				else
					ReadBytes(&b, 1);

				if(outToken != RDX_CNULL)
				{
					if(loadedStatic == STATIC_STRING_SIZE)
					{
						RDX_PROTECT(ctx, FlushBytes(ctx, objm, staticString, loadedStatic, &bytes) );
						loadedStatic = 0;
					}

					staticString[loadedStatic++] = b;
				}
			}

			// Skip further whitespace
			while(1)
			{
				if(!PeekBytes(&b, 1))
					break;
				if(!IsWhitespace(b))
					break;
				ReadBytes(&b, 1);
			}

			*pIsString = parseAsString;

			if(outToken)
			{
				rdxCRef(rdxCString) str;
				if(bytes.IsNotNull())
				{
					RDX_PROTECT(ctx, FlushBytes(ctx, objm, staticString, loadedStatic, &bytes) );
					RDX_PROTECT_ASSIGN(ctx, str, objm->CreateStringUTF8(ctx, bytes->OffsetElementRTRef(0).ToHdl(), true, bytes->NumElements()));
					outToken->SetStrChars(str.ToWeakRTRef());
				}
				else
				{
					// Count chars
					// TODO: Hoist UTF8 counter...
					rdxLargeUInt numChars = 0;
					{
						rdxLargeUInt numBytes = loadedStatic;
						const void *byteBuf = staticString;
						while(numBytes > 0)
						{
							if(rdxDecodeUTF8Char(&byteBuf, &numBytes) == rdxCHAR_Invalid)
								RDX_LTHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
							numChars++;
						}
					}

					if(numChars <= rdxITextDeserializer::COMPACT_TOKEN_SIZE)
					{
						rdxChar *outChars = outToken->InitCompactChars(numChars);
						rdxLargeUInt numBytes = loadedStatic;
						const void *byteBuf = staticString;
						while(numBytes > 0)
						{
							*outChars++ = rdxDecodeUTF8Char(&byteBuf, &numBytes);
						}
					}
					else
					{
						RDX_PROTECT_ASSIGN(ctx, str, objm->CreateStringUTF8(ctx, staticString, true, loadedStatic) );
						outToken->SetStrChars(str.ToWeakRTRef());
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

	void SkipToken(rdxSOperationContext *ctx) RDX_OVERRIDE
	{
		bool b;
		ParseToken(ctx, NULL, &b, false);
	}

	explicit rdxCTextFileParser(rdxIFileStream *stream)
	{
		m_stream = stream;
	}

	bool HasAborted() const RDX_OVERRIDE
	{
		return m_stream->HasAborted();
	}

	void Abort()
	{
		m_stream->Abort();
	}
};


void rdxSPackageManifestImport::VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
{
	if(!visitNonSerializable)
		return;

	visitor->VisitReference(objm, this->resolution);
}

void rdxSPackageManifestLocal::VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
{
	if(!visitNonSerializable)
		return;
		
	visitor->VisitReference(objm, this->resolvedObject);
	visitor->VisitReference(objm, this->resolvedType);
}

void rdxSPackageManifestString::VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
{
	if(!visitNonSerializable)
		return;

	visitor->VisitReference(objm, this->resolvedStr);
}

void rdxSPackageArrayDef::VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
{
	if(!visitNonSerializable)
		return;

	visitor->VisitReference(objm, this->resolvedArrayType);
}


static void rdxParseTextManifest(rdxSOperationContext *ctx, rdxIObjectManager *objManager, rdxIFileStream *stream, rdxWeakHdl(rdxCPackage) pkg)
{
	bool errored = false;
	int eCode = 0;

	rdxLargeUInt numCriticalResources = 0;
	rdxLargeUInt numNonCriticalResources = 0;
	rdxLargeUInt numResources = 0;
	rdxLargeUInt numImports = 0;
	rdxLargeUInt numArrayDefs = 0;
	bool isString;

	rdxCObjectManager::ResourceHashMap *localResources = static_cast<rdxCObjectManager::ResourceHashMap*>(objManager->GetLocalResourcesMap());
	rdxCObjectManager::ResourceHashMap *importedResources = static_cast<rdxCObjectManager::ResourceHashMap*>(objManager->GetImportedResourcesMap());
	rdxCObjectManager::ArrayDefHashMap *arrayDefs = static_cast<rdxCObjectManager::ArrayDefHashMap*>(objManager->GetArrayDefMap());

	const rdxITypeSerializer *luiSerializer = objManager->GetBuiltIns()->st_LargeUInt->m_native.user.typeSerializer;

	RDX_TRY(ctx)
	{
		rdxCTextFileParser tfp(stream);
		tfp.SeekStart(0);
		rdxUInt8 b;

		while(tfp.PeekBytes(&b, 1))
		{
			if(tfp.CheckToken("arraydef"))
			{
				bool isConstant = false;
				RDX_PROTECT(ctx, tfp.SkipToken(ctx) );
				
				rdxITextDeserializer::SCompactToken str1, str2;
				RDX_PROTECT(ctx, tfp.ParseToken(ctx, objManager, &isString, &str1) );
				RDX_PROTECT(ctx, tfp.ParseToken(ctx, objManager, &isString, &str2) );

				rdxSObjectGUID arrayDefName = objManager->ComputeObjectGUID(objManager->ComputeDomainGUID(str1.GetCharSpan()), str2.GetCharSpan());
				
				// Parse off parent def
				RDX_PROTECT(ctx, tfp.ParseToken(ctx, objManager, &isString, &str1) );
				RDX_PROTECT(ctx, tfp.ParseToken(ctx, objManager, &isString, &str2) );

				rdxSDomainGUID parentDomain = objManager->ComputeDomainGUID(str1.GetCharSpan());
				rdxSObjectGUID parentGUID = objManager->ComputeObjectGUID(parentDomain, str2.GetCharSpan());

				// Parse off dimension count
				RDX_PROTECT(ctx, tfp.ParseToken(ctx, objManager, &isString, &str1) );
				
				if(str1.GetCharSpan().Equal("const"))
				{
					isConstant = true;
					RDX_PROTECT(ctx, tfp.ParseToken(ctx, objManager, &isString, &str1) );
				}

				rdxLargeUInt numDimensions;
				rdxDecodeString(str1.GetCharSpan().Chars(), numDimensions);

				rdxSArrayDefPrototype proto;
				proto.containedTypeGUID = parentGUID;
				proto.isConstant = isConstant;
				proto.numDimensions = numDimensions;
				proto.tableIndex = numArrayDefs++;

				RDX_PROTECT(ctx, arrayDefs->Insert(ctx, &arrayDefName, &proto));
			}
			else if(tfp.CheckToken("import"))
			{
				rdxITextDeserializer::SCompactToken str1, str2;
				RDX_PROTECT(ctx, tfp.SkipToken(ctx) );
				RDX_PROTECT(ctx, tfp.ParseToken(ctx, objManager, &isString, &str1) );
				RDX_PROTECT(ctx, tfp.ParseToken(ctx, objManager, &isString, &str2) );
						
				rdxSLoadShell shell;
				shell.tableIndex = numImports++;
				shell.typeName = rdxSObjectGUID::Invalid();
				rdxSDomainGUID domain = objManager->ComputeDomainGUID(str1.GetCharSpan());
				rdxSObjectGUID objectGUID = objManager->ComputeObjectGUID(domain, str2.GetCharSpan());
				RDX_PROTECT(ctx, importedResources->Insert(ctx, &objectGUID, &shell) );
			}
			else if(tfp.CheckToken("def"))
			{
				rdxITextDeserializer::SCompactToken str1, str2;
				RDX_PROTECT(ctx, tfp.SkipToken(ctx) );
				RDX_PROTECT(ctx, tfp.ParseToken(ctx, objManager, &isString, &str1) );
				RDX_PROTECT(ctx, tfp.ParseToken(ctx, objManager, &isString, &str2) );
				
				// TODO MUSTFIX: Add GUID null checks here
				rdxSDomainGUID defDomain = objManager->ComputeDomainGUID(str1.GetCharSpan());
				rdxSObjectGUID defName = objManager->ComputeObjectGUID(defDomain, str2.GetCharSpan());
						
				rdxSLoadShell shell;
				shell.isAnonymous = false;
				shell.isConstant = false;
				shell.isCloaked = false;
				
				RDX_PROTECT(ctx, tfp.ParseToken(ctx, objManager, &isString, &str1) );

				while(!isString)
				{
					rdxSCharSpan charSpan = str1.GetCharSpan();
					if(charSpan.Equal("anonymous"))
						shell.isAnonymous = true;
					else if(charSpan.Equal("cloaked"))
						shell.isCloaked = true;
					else if(charSpan.Equal("const"))
						shell.isConstant = true;
					else
						RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

					RDX_PROTECT(ctx, tfp.ParseToken(ctx, objManager, &isString, &str1) );
				}

				RDX_PROTECT(ctx, tfp.ParseToken(ctx, objManager, &isString, &str2) );

				rdxSDomainGUID typeDomain = objManager->ComputeDomainGUID(str1.GetCharSpan());
				rdxSObjectGUID typeName = objManager->ComputeObjectGUID(typeDomain, str2.GetCharSpan());

				shell.typeName = typeName;
				shell.tableIndex = numResources++;

				RDX_PROTECT(ctx, tfp.ParseToken(ctx, objManager, &isString, &str1) );

				shell.fileOffset = tfp.Tell();

				rdxDecodeString(str1.GetCharSpan().Chars(), shell.numElements);

				// TODO MUSTFIX: Check for collision
				RDX_PROTECT(ctx, localResources->Insert(ctx, &defName, &shell) );
			}
			else
				RDX_PROTECT(ctx, tfp.SkipToken(ctx) );
		}

		{
			pkg->m_native.bucketCounts.numArrayDefs = numArrayDefs;
			pkg->m_native.bucketCounts.numImports = numImports;
			pkg->m_native.bucketCounts.numLocalSymbols = numResources;
			pkg->m_native.bucketCounts.numStrings = 0;

			RDX_PROTECT_ASSIGN(ctx, pkg->m_native.localSymbols, rdxCInternalObjectFactory::Create1DArray<rdxSPackageManifestLocal>(ctx, objManager, numResources, rdxWeakHdl(rdxCArrayOfType)::Null(), rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime)).ToWeakRTRef() );
			RDX_PROTECT_ASSIGN(ctx, pkg->m_native.importedSymbols, rdxCInternalObjectFactory::Create1DArray<rdxSPackageManifestImport>(ctx, objManager, numImports, rdxWeakHdl(rdxCArrayOfType)::Null(), rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime)).ToWeakRTRef() );
			RDX_PROTECT_ASSIGN(ctx, pkg->m_native.arrayDefs, rdxCInternalObjectFactory::Create1DArray<rdxSPackageArrayDef>(ctx, objManager, numArrayDefs, rdxWeakHdl(rdxCArrayOfType)::Null(), rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime)).ToWeakRTRef() );
		}

		// Create manifest symbols
		if(localResources->m_native.untracedEntries)
		{
			for(rdxLargeUInt i=0;i<localResources->m_native.numEntries;i++)
			{
				rdxSHashEntry<rdxSObjectGUID, rdxSLoadShell> *e = localResources->m_native.untracedEntries + i;

				if(e->e.elementType > rdxSHashElement::HMT_ContainsData)
				{
					rdxSPackageManifestLocal ml;

					ml.fileOffset = e->v.fileOffset;
					ml.flags = 0;

					ml.resolvedObject = rdxWeakRTRef(rdxCObject)::Null();
					ml.resolvedType = rdxWeakRTRef(rdxCType)::Null();
					ml.objectGUID = e->k;
					ml.tempTypeGUID = e->v.typeName;
					ml.numElements = e->v.numElements;
					ml.duplicateChecked = false;

					if(e->v.isAnonymous)
						ml.flags |= rdxSPackageManifestLocal::PMLF_Anonymous;
					if(e->v.isConstant)
						ml.flags |= rdxSPackageManifestLocal::PMLF_Constant;
					if(e->v.isCloaked)
						ml.flags |= rdxSPackageManifestLocal::PMLF_Cloaked;

					rdxLargeUInt idx = e->v.tableIndex;
					pkg->m_native.localSymbols->Element(idx) = ml;
				}
			}
		}

		if(importedResources->m_native.untracedEntries)
		{
			for(rdxLargeUInt i=0;i<importedResources->m_native.numEntries;i++)
			{
				rdxSHashEntry<rdxSObjectGUID, rdxSLoadShell> *e = importedResources->m_native.untracedEntries + i;

				if(e->e.elementType > rdxSHashElement::HMT_ContainsData)
				{
					rdxSPackageManifestImport mi;

					mi.resolution = rdxWeakRTRef(rdxCObject)::Null();
					mi.objectGUID = e->k;

					rdxLargeUInt idx = e->v.tableIndex;
					pkg->m_native.importedSymbols->Element(idx) = mi;
					e->v.tableIndex = idx;
				}
			}
		}

		// Create array defs
		if(arrayDefs->m_native.untracedEntries)
		{
			for(rdxLargeUInt i=0;i<arrayDefs->m_native.numEntries;i++)
			{
				rdxSHashEntry<rdxSObjectGUID, rdxSArrayDefPrototype> *e = arrayDefs->m_native.untracedEntries + i;

				if(e->e.elementType > rdxSHashElement::HMT_ContainsData)
				{
					rdxSPackageArrayDef adef;

					adef.internalGUID = e->k;
					adef.tempContainedTypeGUID = e->v.containedTypeGUID;
					adef.isConstant = e->v.isConstant;
					adef.numDimensions = e->v.numDimensions;
					adef.resolvedArrayType = rdxWeakRTRef(rdxCArrayOfType)::Null();

					rdxLargeUInt idx = e->v.tableIndex;
					pkg->m_native.arrayDefs->Element(idx) = adef;
				}
			}
		}

		// All tables are now fully indexed, so we can do lookups
		rdxLargeUInt nArrayDefs = pkg->m_native.arrayDefs->NumElements();
		for(rdxLargeUInt i=0;i<nArrayDefs;i++)
		{
			rdxWeakOffsetRTRef(rdxSPackageArrayDef) adef = pkg->m_native.arrayDefs->OffsetElementRTRef(i);
			rdxSLoadShell shell;
			rdxSArrayDefPrototype arrayProto;
			
			// TODO: Duplicate code...	
			if(importedResources->GetElement(&adef->tempContainedTypeGUID, &shell))
			{
				adef->pkgRef.index = shell.tableIndex;
				adef->pkgRef.symbolLoc = rdxPSL_Imported;
			}
			else if(localResources->GetElement(&adef->tempContainedTypeGUID, &shell))
			{
				adef->pkgRef.index = shell.tableIndex;
				adef->pkgRef.symbolLoc = rdxPSL_Local;
			}
			else if(arrayDefs->GetElement(&adef->tempContainedTypeGUID, &arrayProto))
			{
				adef->pkgRef.index = arrayProto.tableIndex;
				adef->pkgRef.symbolLoc = rdxPSL_Array;
			}
			else
				RDX_STHROW(ctx, RDX_ERROR_UNRESOLVED_SYMBOL_REFERENCE);
		}

		rdxLargeUInt nLocalSymbols = pkg->m_native.localSymbols->NumElements();
		for(rdxLargeUInt i=0;i<nLocalSymbols;i++)
		{
			rdxSPackageManifestLocal *ml = &pkg->m_native.localSymbols->Element(i);
			rdxSLoadShell shell;
			rdxSArrayDefPrototype arrayProto;

			// Match the type name
			{
				ml->resolvedType = rdxWeakRTRef(rdxCType)::Null();
				if(importedResources->GetElement(&ml->tempTypeGUID, &shell))
				{
					ml->typePkgRef.index = shell.tableIndex;
					ml->typePkgRef.symbolLoc = rdxPSL_Imported;
				}
				else if(localResources->GetElement(&ml->tempTypeGUID, &shell))
				{
					ml->typePkgRef.index = shell.tableIndex;
					ml->typePkgRef.symbolLoc = rdxPSL_Local;
				}
				else if(arrayDefs->GetElement(&ml->tempTypeGUID, &arrayProto))
				{
					ml->typePkgRef.index = arrayProto.tableIndex;
					ml->typePkgRef.symbolLoc = rdxPSL_Array;
				}
				else
					RDX_STHROW(ctx, RDX_ERROR_UNRESOLVED_SYMBOL_REFERENCE);
			}
		}

		localResources->Clear();
		importedResources->Clear();
		arrayDefs->Clear();
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

struct rdxSBinaryHeader
{
	char magic[16];
	rdxSBinaryBucketCounts bucketCounts;
};

static rdxLargeUInt rdxReadBinaryLargeUInt(rdxSOperationContext *ctx, rdxIFileStream *fs)
{
	rdxLargeUInt li = 0;
	bool overflowed = false;
	bool readFailed = true;
	if(!fs->ReadConverted<rdxBinPackageLargeUInt, rdxLargeUInt>(&li, overflowed, readFailed))
	{
		if(overflowed)
			RDX_LTHROWV(ctx, RDX_ERROR_INTEGER_OVERFLOW, static_cast<rdxLargeUInt>(0));
		if(readFailed)
			RDX_LTHROWV(ctx, RDX_ERROR_PACKAGE_CORRUPTED, static_cast<rdxLargeUInt>(0));
	}
	return li;
}

static rdxSObjectGUID rdxReadBinaryObjectGUID(rdxSOperationContext *ctx, rdxIFileStream *fs)
{
	rdxSObjectGUID guid;
	if(fs->ReadBytes(guid.m_domain.m_bytes, sizeof(guid.m_domain.m_bytes)) != sizeof(guid.m_domain.m_bytes))
		RDX_LTHROWV(ctx, RDX_ERROR_PACKAGE_CORRUPTED, rdxSObjectGUID::Invalid());
	if(fs->ReadBytes(guid.m_bytes, sizeof(guid.m_bytes)) != sizeof(guid.m_bytes))
		RDX_LTHROWV(ctx, RDX_ERROR_PACKAGE_CORRUPTED, rdxSObjectGUID::Invalid());
	return guid;
}

static rdxLargeInt rdxReadBinaryLargeInt(rdxSOperationContext *ctx, rdxIFileStream *fs)
{
	rdxLargeInt li = 0;
	bool overflowed = false;
	bool readFailed = true;
	if(!fs->ReadConverted<rdxBinPackageLargeInt, rdxLargeInt>(&li, overflowed, readFailed))
	{
		if(overflowed)
			RDX_LTHROWV(ctx, RDX_ERROR_INTEGER_OVERFLOW, static_cast<rdxLargeInt>(0));
		if(readFailed)
			RDX_LTHROWV(ctx, RDX_ERROR_PACKAGE_CORRUPTED, static_cast<rdxLargeInt>(0));
	}
	return li;
}

static void rdxReadBinarySymbolIndex(rdxSOperationContext *ctx, rdxIFileStream *stream, rdxSPackageReference *outPkgRef, const rdxSBinaryBucketCounts *bucketCounts)
{
	rdxUInt8 locB;
	if(stream->ReadBytes(&locB, 1) != 1)
		RDX_LTHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
	if(locB > rdxPSL_Count)
		RDX_LTHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
	if(locB == rdxPSL_Null)
	{
		outPkgRef->symbolLoc = rdxPSL_Null;
		outPkgRef->index = 0;
	}
	else
	{
		RDX_TRY(ctx)
		{
			rdxLargeUInt symIndexU;
			RDX_PROTECT_ASSIGN(ctx, symIndexU, rdxReadBinaryLargeUInt(ctx, stream));

			switch(locB)
			{
			case rdxPSL_Array:
				{
					if(symIndexU >= bucketCounts->numArrayDefs)
						RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
				}
				break;
			case rdxPSL_Imported:
				{
					if(symIndexU >= bucketCounts->numImports)
						RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
				}
				break;
			case rdxPSL_Local:
				{
					if(symIndexU >= bucketCounts->numLocalSymbols)
						RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
				}
				break;
			case rdxPSL_String:
				{
					if(symIndexU >= bucketCounts->numStrings)
						RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
				}
				break;
			default:
				RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);
			};
			
			outPkgRef->index = 0;
		}
		RDX_CATCH(ctx)
		{
			RDX_RETHROW(ctx);
		}
		RDX_ENDTRY
	}

	outPkgRef->symbolLoc = static_cast<rdxEPackageSymbolLoc>(locB);
}

void rdxParseBinaryManifest(rdxSOperationContext *ctx, rdxIObjectManager *objManager, rdxIFileStream *stream, rdxWeakHdl(rdxCPackage) pkg)
{
	bool errored = false;
	int eCode = 0;

	rdxLargeUInt numCriticalResources = 0;
	rdxLargeUInt numNonCriticalResources = 0;
	rdxLargeUInt numResources = 0;
	rdxLargeUInt numExports = 0;
	rdxLargeUInt numImports = 0;

	RDX_TRY(ctx)
	{
		rdxSBinaryHeader header;
		{
			if(stream->ReadBytes(header.magic, 16) != 16)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
			RDX_PROTECT_ASSIGN(ctx, header.bucketCounts.numImports, rdxReadBinaryLargeUInt(ctx, stream));
			RDX_PROTECT_ASSIGN(ctx, header.bucketCounts.numLocalSymbols, rdxReadBinaryLargeUInt(ctx, stream));
			RDX_PROTECT_ASSIGN(ctx, header.bucketCounts.numStrings, rdxReadBinaryLargeUInt(ctx, stream));
			RDX_PROTECT_ASSIGN(ctx, header.bucketCounts.numArrayDefs, rdxReadBinaryLargeUInt(ctx, stream));
		}

		if(memcmp(header.magic, RDX_MAGIC, 16))
			RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

		RDX_PROTECT_ASSIGN(ctx, pkg->m_native.importedSymbols, rdxCInternalObjectFactory::Create1DArray<rdxSPackageManifestImport>(ctx, objManager, header.bucketCounts.numImports));
		RDX_PROTECT_ASSIGN(ctx, pkg->m_native.localSymbols, rdxCInternalObjectFactory::Create1DArray<rdxSPackageManifestLocal>(ctx, objManager, header.bucketCounts.numLocalSymbols));

		rdxCRef(rdxCArrayOfType) aot_String;
		RDX_PROTECT_ASSIGN(ctx, aot_String, objManager->CreateArrayType(ctx, objManager->GetBuiltIns()->st_String.ToWeakHdl(), 1, true, rdxSAutoTypeInfo<rdxCArray<rdxTracedRTRef(rdxCString)> >::TypeInfoInterface()));
		RDX_PROTECT_ASSIGN(ctx, pkg->m_native.strings, rdxCInternalObjectFactory::Create1DArray<rdxTracedRTRef(rdxCString)>(ctx, objManager, header.bucketCounts.numStrings, aot_String.ToWeakHdl(), rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime)));
		RDX_PROTECT_ASSIGN(ctx, pkg->m_native.arrayDefs, rdxCInternalObjectFactory::Create1DArray<rdxSPackageArrayDef>(ctx, objManager, header.bucketCounts.numArrayDefs).ToWeakRTRef());
				
		const rdxITypeSerializer *strs = objManager->GetBuiltIns()->st_String->m_native.user.typeSerializer;

		// Read strings
		for(rdxLargeUInt i=0;i<header.bucketCounts.numStrings;i++)
		{
			rdxUInt8 characters[1024];
			rdxLargeUInt nUTF8Bytes = 0;

			RDX_PROTECT_ASSIGN(ctx, nUTF8Bytes, rdxReadBinaryLargeUInt(ctx, stream));

			if(nUTF8Bytes <= sizeof(characters))
			{
				if(stream->ReadBytes(characters, nUTF8Bytes) != nUTF8Bytes)
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

				RDX_PROTECT_ASSIGN(ctx, pkg->m_native.strings->Element(i), objManager->CreateStringUTF8(ctx, characters, true, nUTF8Bytes).ToWeakRTRef());
			}
			else
			{
				rdxArrayCRef(rdxUInt8) tempChars;
				RDX_PROTECT_ASSIGN(ctx, tempChars, rdxCInternalObjectFactory::Create1DArray<rdxUInt8>(ctx, objManager, nUTF8Bytes));

				if(stream->ReadBytes(tempChars.Modify(), nUTF8Bytes) != nUTF8Bytes)
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

				RDX_PROTECT_ASSIGN(ctx, pkg->m_native.strings->Element(i), objManager->CreateStringUTF8(ctx, tempChars->OffsetElementRTRef(0).ToHdl(), true, nUTF8Bytes));
			}
		}
		
		// Read array defs
		for(rdxLargeUInt i=0;i<header.bucketCounts.numArrayDefs;i++)
		{
			rdxSPackageArrayDef adef;
			RDX_PROTECT(ctx, rdxReadBinarySymbolIndex(ctx, stream, &adef.pkgRef, &header.bucketCounts));
			if(adef.pkgRef.symbolLoc == rdxPSL_String)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
			
			rdxUInt8 isConstantB;
			if(stream->ReadBytes(&isConstantB, 1) != 1)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
			adef.isConstant = (isConstantB != 0);

			RDX_PROTECT_ASSIGN(ctx, adef.numDimensions, rdxReadBinaryLargeUInt(ctx, stream));
		}

		for(rdxLargeUInt i=0;i<header.bucketCounts.numImports;i++)
		{
			rdxCRef(rdxCString) str;

			rdxWeakOffsetHdl(rdxSPackageManifestImport) mi = pkg->m_native.importedSymbols->OffsetElementRTRef(i).ToHdl();
					
			// Deserialize name
			RDX_PROTECT_ASSIGN(ctx, mi->objectGUID, rdxReadBinaryObjectGUID(ctx, stream));
			mi->resolution = rdxWeakRTRef(rdxCObject)::Null();
		}
				
		for(rdxLargeUInt i=0;i<header.bucketCounts.numLocalSymbols;i++)
		{
			rdxWeakOffsetHdl(rdxSPackageManifestLocal) ml = pkg->m_native.localSymbols->OffsetElementRTRef(i).ToHdl();
			RDX_PROTECT_ASSIGN(ctx, ml->fileOffset, rdxReadBinaryLargeUInt(ctx, stream));
			if(ml->fileOffset < 0)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
					
			rdxLargeUInt typeSymbolID = 0;
			RDX_PROTECT_ASSIGN(ctx, typeSymbolID, rdxReadBinaryLargeUInt(ctx, stream));

			rdxUInt8 typeSymbolLocB;
			if(stream->ReadBytes(&typeSymbolLocB, 1) != 1)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

			
			RDX_PROTECT(ctx, rdxReadBinarySymbolIndex(ctx, stream, &ml->typePkgRef, &header.bucketCounts));
			if(ml->typePkgRef.symbolLoc != rdxPSL_Imported
				&& ml->typePkgRef.symbolLoc != rdxPSL_Local
				&& ml->typePkgRef.symbolLoc != rdxPSL_Array)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

			RDX_PROTECT_ASSIGN(ctx, ml->numElements, rdxReadBinaryLargeUInt(ctx, stream));

			ml->flags = 0;
			stream->ReadSwappableBytes(&ml->flags, sizeof(ml->flags));
			ml->flags &= rdxSPackageManifestLocal::PMLF_Anonymous;
			
			if(!(ml->flags & rdxSPackageManifestLocal::PMLF_Anonymous))
				RDX_PROTECT_ASSIGN(ctx, ml->objectGUID, rdxReadBinaryObjectGUID(ctx, stream));
		}
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

void rdxDeserializeReference(rdxSOperationContext *ctx, rdxIObjectManager *objManager, rdxWeakHdl(rdxCPackage) pkg,
	rdxIPackageHost *host, rdxIFileStream *fs, bool isText, bool permitString, rdxCRef(rdxCString) *outStr, rdxSPackageReference *outPkgRef)
{
	RDX_TRY(ctx)
	{
		outPkgRef->index = 0;

		if(isText)
		{
			rdxITextDeserializer *tfp = static_cast<rdxITextDeserializer*>(fs);
			if(tfp->CheckToken("null"))
			{
				outPkgRef->symbolLoc = rdxPSL_Null;
				RDX_PROTECT(ctx, tfp->SkipToken(ctx) );

				return;
			}
			if(tfp->CheckToken("res"))
			{
				bool isString;

				rdxITextDeserializer::SCompactToken resDomain;
				rdxITextDeserializer::SCompactToken resName;

				RDX_PROTECT(ctx, tfp->SkipToken(ctx) );

				RDX_PROTECT(ctx, tfp->ParseToken(ctx, objManager, &isString, &resDomain) );
				if(!isString)
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
				RDX_PROTECT(ctx, tfp->ParseToken(ctx, objManager, &isString, &resName) );
				if(!isString)
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

				rdxSDomainGUID domain = objManager->ComputeDomainGUID(resDomain.GetCharSpan());
				rdxSObjectGUID objectGUID = objManager->ComputeObjectGUID(domain, resName.GetCharSpan());

				rdxLargeUInt niRefs = pkg->m_native.importedSymbols->NumElements();
				for(rdxLargeUInt i=0;i<niRefs;i++)
				{
					if(pkg->m_native.importedSymbols->Element(i).objectGUID == objectGUID)
					{
						outPkgRef->symbolLoc = rdxPSL_Imported;
						outPkgRef->index = i;
						return;
					}
				}
				
				rdxLargeUInt nlRefs = pkg->m_native.localSymbols->NumElements();
				for(rdxLargeUInt i=0;i<nlRefs;i++)
				{
					if(pkg->m_native.localSymbols->Element(i).objectGUID == objectGUID)
					{
						outPkgRef->symbolLoc = rdxPSL_Local;
						outPkgRef->index = i;
						return;
					}
				}
				
				rdxLargeUInt nadefs = pkg->m_native.arrayDefs->NumElements();
				for(rdxLargeUInt i=0;i<nadefs;i++)
				{
					if(pkg->m_native.arrayDefs->Element(i).internalGUID == objectGUID)
					{
						outPkgRef->symbolLoc = rdxPSL_Array;
						outPkgRef->index = i;
						return;
					}
				}

				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
			}

			// Try to parse this as a string.  This is destructive, so any other parse attempts will need to reset the stream
			if(permitString)
			{
				rdxITextDeserializer::SCompactToken str;
				bool isString;
				RDX_PROTECT(ctx, tfp->ParseToken(ctx, objManager, &isString, &str) );

				if(!isString)
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
				outPkgRef->symbolLoc = rdxPSL_String;
				rdxSCharSpan charSpan(str.GetCharSpan());
				// TODO MUSTFIX: This is dumb if the char span is already backed by a str object
				RDX_PROTECT_ASSIGN(ctx, *outStr, objManager->CreateString(ctx, charSpan.Chars(), true, charSpan.Length()));
			}
			return;
		}
		else
		{
			// Binary
			rdxSPackageReference pkgRef;
			RDX_PROTECT(ctx, rdxReadBinarySymbolIndex(ctx, fs, &pkgRef, &pkg->m_native.bucketCounts));
			
			if(pkgRef.symbolLoc == rdxPSL_String)
			{
				if(!permitString)
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
				if(outStr != NULL)
				{
					rdxWeakHdl(rdxCObject) strRef;
					RDX_PROTECT(ctx, pkgRef.ConvertToReference(ctx, pkg, false, &strRef));
					*outStr = strRef.StaticCast<rdxCString>();
				}
			}
			*outPkgRef = pkgRef;
		}
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

void rdxDeserializeBulk(rdxSOperationContext *ctx, rdxIObjectManager *objManager, rdxWeakHdl(rdxCPackage) pkg,
	rdxSDomainGUID domain, rdxIPackageHost *host, rdxIFileStream *fs, rdxWeakHdl(rdxCStructuredType) st, rdxWeakTypelessOffsetHdl output, rdxLargeUInt numElements, int depth);

static void rdxDeserializeArray(rdxSOperationContext *ctx, rdxIObjectManager *objManager, rdxWeakHdl(rdxCPackage) pkg,
	rdxSDomainGUID domain, rdxIPackageHost *host, rdxIFileStream *fs, bool isText, rdxWeakHdl(rdxCArrayOfType) aot,
	rdxLargeUInt expectedNumElements, rdxWeakHdl(rdxCArrayContainer) arrayObj)
{
	RDX_TRY(ctx)
	{
		bool isString;
		rdxLargeUInt totalElements;

		// Read in dimensions
		totalElements = 1;
		
		if(aot->numDimensions == 0)
		{
			RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
		}
		else if(aot->numDimensions == 1)
		{
			totalElements = expectedNumElements;
		}
		else
		{
			for(rdxLargeUInt i=0;i<aot->numDimensions;i++)
			{
				rdxLargeUInt d;

				if(isText)
				{
					rdxITextDeserializer::SCompactToken str1;
					RDX_PROTECT(ctx, static_cast<rdxITextDeserializer*>(fs)->ParseToken(ctx, objManager, &isString, &str1) );
					rdxDecodeString(str1.GetCharSpan().Chars(), d);
				}
				else
				{
					RDX_PROTECT_ASSIGN(ctx, d, rdxReadBinaryLargeUInt(ctx, fs));
				}

				arrayObj->SetDimension(i, d);

				if(!rdxCheckMulOverflowU(totalElements, d))
					RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);

				totalElements *= d;
			}

			if(totalElements != expectedNumElements)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
		}

		rdxWeakHdl(rdxCType) subType = aot->type.ToWeakHdl();
		rdxLargeUInt elementSize = 0;

		if(subType.IsNull())
			RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);

		elementSize = objManager->TypeElementSize(aot.ToWeakRTRef());
		if(!elementSize)
			RDX_STHROW(ctx, RDX_ERROR_INTERNAL_BAD_TYPE);

		RDX_PROTECT(ctx, objManager->InitializeObject(arrayObj.ToWeakRTRef(), 0, false));

		RDX_PROTECT(ctx, objManager->AddUnloadedObject(ctx, arrayObj.ToWeakHdl()));

		rdxWeakOffsetHdl(rdxUInt8) byteBase = arrayObj.ReinterpretCast<rdxCArray<rdxUInt8> >()->OffsetElementRTRef(0).ToHdl();

		// Parse out elements
		if(isText)
		{
			rdxITextDeserializer *tfp = static_cast<rdxITextDeserializer*>(fs);
			if(!tfp->CheckToken("{")) RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
			RDX_PROTECT(ctx, tfp->SkipToken(ctx));
		}

		if(isText == false
			&& (subType->ObjectInfo()->containerType == objManager->GetBuiltIns()->st_StructuredType)
			&& (subType.StaticCast<rdxCStructuredType>()->m_native.flags & rdxCStructuredType::NativeProperties::STF_AllowBulkSerialize)
			&& !objManager->TypeIsObjectReference(subType.ToWeakRTRef())
			&& !objManager->TypeIsInterface(subType.ToWeakRTRef()))
		{
			// Bulk serializable values
			RDX_PROTECT(ctx, rdxDeserializeBulk(ctx, objManager, pkg, domain, host, fs, subType.StaticCast<rdxCStructuredType>(), byteBase, totalElements, 0));
		}
		else
		{
			for(rdxLargeUInt i=0;i<totalElements;i++)
			{
				RDX_PROTECT(ctx, rdxDeserializeValue(ctx, objManager, pkg, domain, host, fs, isText, aot->type.ToWeakHdl(), byteBase, 0));
				byteBase += elementSize;

				if(isText)
				{
					rdxITextDeserializer *tfp = static_cast<rdxITextDeserializer*>(fs);
					if( tfp->CheckToken(",") )
						tfp->ParseToken(ctx, NULL, &isString, false);
					else if(i != totalElements-1)
						RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
				}
			}
		}

		if(isText)
		{
			rdxITextDeserializer *tfp = static_cast<rdxITextDeserializer*>(fs);
			if(!tfp->CheckToken("}")) RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
			RDX_PROTECT(ctx, tfp->SkipToken(ctx));
		}
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

void rdxDeserializeBulk(rdxSOperationContext *ctx, rdxIObjectManager *objManager, rdxWeakHdl(rdxCPackage) pkg,
	rdxSDomainGUID domain, rdxIPackageHost *host, rdxIFileStream *fs, rdxWeakHdl(rdxCStructuredType) st, rdxWeakTypelessOffsetHdl output, rdxLargeUInt numElements, int depth)
{
	RDX_TRY(ctx)
	{
		rdxLargeUInt stride = st->m_native.size;

		bool swap = rdxIFileStream::ShouldByteSwap();

		if(!swap)
		{
			rdxLargeUInt bulkSize = numElements * stride;
			if(fs->ReadBytes(output.Modify(), bulkSize) != bulkSize)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
			return;
		}

		while(numElements--)
		{
			if(st->storageSpecifier == rdxSS_Enum)
			{
				rdxEnumValue v;
				if(fs->ReadSwappableBytes(&v, sizeof(rdxEnumValue)) != sizeof(rdxEnumValue))
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
				*output.StaticCast<rdxEnumValue>().Modify() = v;
			}
			else if(st->storageSpecifier == rdxSS_Class ||
				st->storageSpecifier == rdxSS_RefStruct ||
				st->storageSpecifier == rdxSS_ValStruct)
			{
				// Parse out properties

				rdxLargeUInt nProperties = 0;
				rdxLargeUInt propertyOffset = 0;
				if(st->properties.IsNotNull())
					nProperties = st->properties->NumElements();
				rdxWeakArrayHdl(rdxSProperty) properties = st->properties.ToWeakHdl();
				rdxWeakArrayHdl(rdxLargeUInt) offsets = st->m_native.propertyOffsets.ToWeakHdl();

				for(rdxLargeUInt i=0;i<nProperties;i++)
				{
#ifdef RDX_ENABLE_PADDED_BULK_SERIALIZE
					fs->SeekForward(offsets[i] - propertyOffset);
					propertyOffset = offsets[i] + properties[i].type.StaticCast<rdxCStructuredType>()->m_native.size;
#endif
					rdxWeakTypelessOffsetHdl propertyLocation = output.StaticCast<rdxCArray<rdxUInt8> >()->OffsetElementRTRef(offsets->Element(i)).ToHdl();
					RDX_PROTECT(ctx, rdxDeserializeValue(ctx, objManager, pkg, domain, host, fs, false, properties->Element(i).type.ToWeakHdl(), propertyLocation, depth + 1));
				}

#ifdef RDX_ENABLE_PADDED_BULK_SERIALIZE
				fs->SeekForward(st->m_native.size - propertyOffset);
#endif
			}
			else
			{
				RDX_STHROW(ctx, RDX_ERROR_INTERNAL_BAD_TYPE);
			}
		}
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

static void rdxDeserializeStructure(rdxSOperationContext *ctx, rdxIObjectManager *objManager, rdxWeakHdl(rdxCPackage) pkg,
	rdxSDomainGUID domain, rdxIPackageHost *host, rdxIFileStream *fs, bool isText, rdxWeakHdl(rdxCStructuredType) st, rdxWeakTypelessOffsetHdl output, int depth)
{
	bool isString;

	RDX_TRY(ctx)
	{
		if(depth >= RDX_MAX_RECURSION_DEPTH)
			RDX_STHROW(ctx, RDX_ERROR_RECURSION_LIMIT_EXCEEDED);

		if(st->m_native.user.typeSerializer)
		{
			bool shouldProcessProperties = true;
			if(isText)
			{
				RDX_PROTECT(ctx, st->m_native.user.typeSerializer->DeserializeTextInstance(ctx, objManager, output, static_cast<rdxITextDeserializer*>(fs), host, pkg, shouldProcessProperties) );
			}
			else
			{
				RDX_PROTECT(ctx, st->m_native.user.typeSerializer->DeserializeBinaryInstance(ctx, objManager, output, fs, host, pkg, shouldProcessProperties) );
			}
			if(!shouldProcessProperties)
				return;
		}

		if(st->storageSpecifier == rdxSS_Enum)
		{
			// Find the enum
			if(isText)
			{
				rdxITextDeserializer *tfp = static_cast<rdxITextDeserializer*>(fs);

				rdxITextDeserializer::SCompactToken token;
				RDX_PROTECT(ctx, tfp->ParseToken(ctx, objManager, &isString, &token) );
				if(!isString)
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

				rdxWeakArrayRTRef(rdxSEnumerant) enumerants = st->enumerants.ToWeakRTRef();
				if(enumerants.IsNotNull())
				{
					rdxLargeUInt nEnums = enumerants->NumElements();

					for(rdxLargeUInt i=0;i<nEnums;i++)
					{
						rdxWeakRTRef(rdxCString) enumName = enumerants->Element(i).name.ToWeakRTRef();
						if(enumName.IsNull())
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
						if(enumName->Equal(token.GetCharSpan()))
						{
							*output.StaticCast<rdxEnumValue>().Modify() = enumerants->Element(i).value;
							return;
						}
					}
				}
				RDX_STHROW(ctx, RDX_ERROR_UNRESOLVED_SYMBOL_REFERENCE);
			}
			else
			{
				rdxEnumValue v;
				if(fs->ReadSwappableBytes(&v, sizeof(rdxEnumValue)) != sizeof(rdxEnumValue))
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

#ifdef RDX_VALIDATE_DESERIALIZED_ENUMS
				rdxWeakArrayRTRef(rdxSEnumerant) enumerants = st->enumerants.ToWeakRTRef();
				if(enumerants.IsNotNull())
				{
					rdxLargeUInt nEnums = enumerants->NumElements();

					for(rdxLargeUInt i=0;i<nEnums;i++)
					{
						if(enumerants->Element(i).value == v)
						{
							*output.StaticCast<rdxEnumValue>().Modify() = v;
							return;
						}
					}
				}

				RDX_STHROW(ctx, RDX_ERROR_UNRESOLVED_SYMBOL_REFERENCE);
#else
				*output.StaticCast<rdxEnumValue>().Modify() = v;
#endif
			}
		}

		// Parse out properties
		if(isText)
		{
			rdxITextDeserializer *tfp = static_cast<rdxITextDeserializer*>(fs);
			if(!tfp->CheckToken("{"))
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
			RDX_PROTECT(ctx, tfp->SkipToken(ctx));

			while(!tfp->CheckToken("}"))
			{
				rdxITextDeserializer::SCompactToken propertyName;
				rdxITextDeserializer::SCompactToken propertyValue;
					
				RDX_PROTECT(ctx, tfp->ParseToken(ctx, objManager, &isString, &propertyName) );

				if(!tfp->CheckToken(":")) RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
				RDX_PROTECT(ctx, tfp->SkipToken(ctx));

				// Figure out which property this is
				rdxLargeUInt propertyIndex = 0;
				rdxLargeUInt nProperties = 0;
				bool propertyIndexFound = false;
				if(st->properties.IsNotNull())
					nProperties = st->properties->NumElements();
				for(rdxLargeUInt i=0;i<nProperties;i++)
				{
					rdxWeakRTRef(rdxCString) checkPropName = st->properties->Element(i).name.ToWeakRTRef();
					if(checkPropName.IsNotNull() && checkPropName->Equal(propertyName.GetCharSpan()))
					{
						propertyIndex = i;
						propertyIndexFound = true;
						break;
					}
				}

				if(!propertyIndexFound)
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

				rdxWeakOffsetHdl(rdxSProperty) p = st->properties->OffsetElementRTRef(propertyIndex).ToHdl();
				rdxWeakTypelessOffsetHdl propertyLocation = output.ReinterpretCast<rdxUInt8>() + st->m_native.propertyOffsets->Element(propertyIndex);

				RDX_PROTECT(ctx, rdxDeserializeValue(ctx, objManager, pkg, domain, host, fs, isText, p->type.ToWeakHdl(), propertyLocation, depth + 1));

				if( tfp->CheckToken(",") )
					tfp->ParseToken(ctx, NULL, &isString, false);
				else if(!tfp->CheckToken("}"))
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
			}
			RDX_PROTECT(ctx, tfp->SkipToken(ctx) );
		}
		else
		{
			// Binary, all properties are serialized
			bool bulk = ((st->m_native.flags & rdxCStructuredType::NativeProperties::STF_AllowBulkSerialize) != 0);

			if(bulk)
			{
				RDX_PROTECT(ctx, rdxDeserializeBulk(ctx, objManager, pkg, domain, host, fs, st, output, 1, depth + 1));
			}
			else
			{
				rdxLargeUInt nProperties = 0;
				if(st->properties.IsNotNull())
					nProperties = st->properties->NumElements();
				for(rdxLargeUInt i=0;i<nProperties;i++)
				{
					rdxWeakTypelessOffsetHdl propertyLocation = output.ReinterpretCast<rdxUInt8>() + st->m_native.propertyOffsets->Element(i);
					rdxWeakOffsetHdl(rdxSProperty) p = st->properties->OffsetElementRTRef(i).ToHdl();
					RDX_PROTECT(ctx, rdxDeserializeValue(ctx, objManager, pkg, domain, host, fs, isText, p->type.ToWeakHdl(), propertyLocation, depth + 1));
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

void rdxDeserializeValue(rdxSOperationContext *ctx, rdxIObjectManager *objManager, rdxWeakHdl(rdxCPackage) pkg,
	rdxSDomainGUID domain, rdxIPackageHost *host, rdxIFileStream *fs, bool isText, rdxWeakHdl(rdxCType) t, rdxWeakTypelessOffsetHdl propertyLocation, int depth)
{
	RDX_TRY(ctx)
	{
		if(objManager->TypeIsObjectReference(t.ToWeakRTRef()) || objManager->TypeIsInterface(t.ToWeakRTRef()))
		{
			rdxWeakHdl(rdxCObject) objRef;
			rdxWeakIRef(rdxSObjectInterfaceImplementation) iRef;
			rdxCRef(rdxCString) str;
			rdxSPackageReference pkgRef;
			RDX_PROTECT(ctx, rdxDeserializeReference(ctx, objManager, pkg, host, fs, isText, true, &str, &pkgRef) );
			if(pkgRef.symbolLoc == rdxPSL_String)
				objRef = str;
			else
				RDX_PROTECT(ctx, pkgRef.ConvertToReference(ctx, pkg, false, &objRef));

			if(objRef.IsNotNull())
			{
				if(objManager->TypeIsObjectReference(t.ToWeakRTRef()))
				{
					if(!objManager->ObjectCompatible(objRef.ToWeakRTRef(), t.ToWeakRTRef()) )
						RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_CONVERSION);
				}
				else if(objManager->TypeIsInterface(t.ToWeakRTRef()))
				{
					iRef = objManager->FindInterface(objRef.ToWeakRTRef(), t.StaticCast<rdxCStructuredType>().ToWeakRTRef());
					if(iRef.IsNull())
						RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_CONVERSION);
				}
				else
					RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);
			}

			if(objManager->TypeIsInterface(t.ToWeakRTRef()))
				*propertyLocation.StaticCast<rdxTracedIRef(rdxSObjectInterfaceImplementation)>().Modify() = iRef;
			else
				*propertyLocation.StaticCast<rdxTracedRTRef(rdxCObject)>().Modify() = objRef;
		}
		else
			RDX_PROTECT(ctx, rdxDeserializeStructure(ctx, objManager, pkg, domain, host, fs, isText, t.StaticCast<rdxCStructuredType>(), propertyLocation, depth) );
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

bool rdxCanDeserializeObject(rdxIObjectManager *objManager, rdxWeakHdl(rdxCType) t, rdxWeakRTRef(rdxCObject) unloadedInstance)
{
	rdxWeakHdl(rdxCStructuredType) containedStructure;

	if(t == objManager->GetBuiltIns()->st_Varying)
		return false;	// Can never deserialize varyings

	if(t->ObjectInfo()->containerType == objManager->GetBuiltIns()->st_ArrayOfType)
	{
		rdxWeakHdl(rdxCType) subType = t.StaticCast<rdxCArrayOfType>()->type.ToWeakHdl();

		// TODO MUSTFIX: Can this break?
		if(subType->ObjectInfo()->containerType == objManager->GetBuiltIns()->st_ArrayOfType)
			return true;
		else if(subType->ObjectInfo()->containerType == objManager->GetBuiltIns()->st_DelegateType)
			return true;
		else if(subType->ObjectInfo()->containerType == objManager->GetBuiltIns()->st_StructuredType)
		{
			containedStructure = subType.StaticCast<rdxCStructuredType>();

			switch(containedStructure->storageSpecifier)
			{
			case rdxSS_Class:
			case rdxSS_Enum:
			case rdxSS_Interface:
				return true;	// Dumb or ref types
			default:
				break;
			};
		}
		else
		{
			return false;	// Not a valid type
		}
	}
	else if(t->ObjectInfo()->containerType == objManager->GetBuiltIns()->st_StructuredType)
	{
		containedStructure = t.StaticCast<rdxCStructuredType>();
		if(containedStructure->isAbstract)
			return false;	// Can't deserialize abstract types
	}
	else
		return false;	// Not a valid type

	if(!(containedStructure->m_native.flags & rdxCStructuredType::NativeProperties::STF_StructureEvaluated))
		return false;	// Don't have the structure layout

	if(containedStructure->m_native.flags & rdxCStructuredType::NativeProperties::STF_FinalDefaultEvaluated)
		return true;
	if( (containedStructure->m_native.flags & rdxCStructuredType::NativeProperties::STF_DependencyDefaultsEvaluated)
		&& containedStructure->m_native.currentDefaultValue == unloadedInstance)
		return true;	// This is the default and it's being loaded, go ahead
	return false;		// Can't determine the default yet
}

bool rdxDeserializeObject(rdxSOperationContext *ctx, rdxCObjectManager *objManager, rdxWeakHdl(rdxCPackage) pkg,
	rdxWeakHdl(rdxCObject) targetContainer, rdxWeakOffsetHdl(rdxSPackageManifestLocal) mfl,
	rdxSDomainGUID domain, rdxIPackageHost *host, rdxIFileStream *stream, bool isText)
{
	rdxCRef(rdxCType) objType = mfl->resolvedType.ToCRef();

	//printf("Deserializing %s:%s\n", mfl->objectGUID.m_domain.DebugStr(), mfl->objectGUID.DebugStr());

	RDX_TRY(ctx)
	{
		if(objType.IsNull())
			RDX_STHROW(ctx, RDX_ERROR_UNRESOLVED_SYMBOL_REFERENCE);

		rdxIFileStream *streamToUse;
		rdxCTextFileParser textParser(stream);
		if(isText)
			streamToUse = &textParser;
		else
			streamToUse = stream;
		streamToUse->SeekStart(mfl->fileOffset);

		if(!rdxCanDeserializeObject(objManager, objType.ToWeakHdl(), targetContainer.ToWeakRTRef()))
			return false;

		if(objType->ObjectInfo()->containerType == objManager->GetBuiltIns()->st_StructuredType)
		{
			rdxWeakHdl(rdxCStructuredType) objST = objType.ToWeakHdl().StaticCast<rdxCStructuredType>();

			if(mfl->numElements != 1)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

			if(domain != rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime) && domain != rdxSDomainGUID::Builtin(rdxDOMAIN_Duplicable))
			{
				rdxSSerializationTag *tag = targetContainer->ObjectInfo()->SerializationTag();
				if(tag)
					tag->package = pkg;
			}

			// TODO: Fix?
			//if(!(mfl->flags & rdxSPackageManifestLocal::PMLF_Anonymous) && !(objST->m_native.flags & rdxCStructuredType::NativeProperties::STF_StructureIsMutable))
			//	pkg->m_native.persistence = rdxCPackage::PERS_Persistent;

			if(mfl->objectGUID.m_domain == rdxSDomainGUID::Builtin(rdxDOMAIN_Duplicable))
			{
				if((objST->m_native.user.flags & rdxCStructuredType::NativeProperties::UserProperties::STUF_AllowDuplicates) == 0)
				{
					RDX_STHROW(ctx, RDX_ERROR_FORBIDDEN_SYMBOL_DUPLICATION);
				}
			}

			// If this is a non-reference type and the resource is non-constant, then the resource is mutable even if the fields are constant, so this package is non-persistent
			// TODO: Fix?
			//if(!objManager->TypeIsObjectReference(objST.ToWeakRTRef()) && !(mfl->flags & rdxSPackageManifestLocal::PMLF_Constant))
			//	pkg->,m_native.persistence = Package::PERS_Persistent;

			if(mfl->flags & rdxSPackageManifestLocal::PMLF_Constant)
				targetContainer->ObjectInfo()->objectFlags |= rdxGCInfo::GCOF_ConstantStructure;

			RDX_PROTECT(ctx, objManager->AddUnloadedObject(ctx, targetContainer));
			RDX_PROTECT(ctx, rdxDeserializeStructure(ctx, objManager, pkg, domain, host, streamToUse, isText, objST, targetContainer.NoOffset(), 0) );
		}
		else if(objType->ObjectInfo()->containerType == objManager->GetBuiltIns()->st_ArrayOfType)
		{
			rdxWeakHdl(rdxCArrayOfType) objAOT = objType.ToWeakHdl().StaticCast<rdxCArrayOfType>();

			// TODO: Fix?
			//if(!objAOT->isConstant)
			//	pkg->m_native.persistence = rdxCPackage::PERS_Persistent;

			if(mfl->objectGUID.m_domain == rdxSDomainGUID::Builtin(rdxDOMAIN_Duplicable))
			{
				// Duplicable arrays must be constant
				if(objAOT->isConstant == rdxFalseValue)
					RDX_STHROW(ctx, RDX_ERROR_FORBIDDEN_SYMBOL_DUPLICATION);
			}
			RDX_PROTECT(ctx, rdxDeserializeArray(ctx, objManager, pkg, mfl->objectGUID.m_domain, host, streamToUse, isText, objType.ToWeakHdl().StaticCast<rdxCArrayOfType>(), mfl->numElements, targetContainer.StaticCast<rdxCArrayContainer>()) );
		}
		else
			RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

		return true;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROWV(ctx, false);
	}
	RDX_ENDTRY
}

rdxCRef(rdxCPackage) rdxDeserializePackage(rdxSOperationContext *ctx, rdxIObjectManager *objManager, rdxSDomainGUID domain,
	rdxIPackageHost *host, rdxIFileStream *stream, bool isText)
{
	rdxCRef(rdxCPackage) pkg = rdxCRef(rdxCPackage)::Null();
	RDX_TRY(ctx)
	{
		RDX_PROTECT_ASSIGN(ctx, pkg, rdxCInternalObjectFactory::CreateObject<rdxCPackage>(ctx, objManager));
		pkg->m_native.domain = domain;

		if(isText)
			RDX_PROTECT(ctx, rdxParseTextManifest(ctx, objManager, stream, pkg.ToWeakHdl()) );
		else
			RDX_PROTECT(ctx, rdxParseBinaryManifest(ctx, objManager, stream, pkg.ToWeakHdl()) );

		// Determine the domain of all local symbols in this package
		rdxLargeUInt nls = pkg->m_native.localSymbols->NumElements();
		for(rdxLargeUInt i=0;i<nls;i++)
		{
			rdxSDomainGUID symbolDomain = rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime);
			rdxWeakOffsetHdl(rdxSPackageManifestLocal) ml = pkg->m_native.localSymbols->OffsetElementRTRef(i).ToHdl();

			// TODO: Is this actually possible?
			if(!(ml->flags & rdxSPackageManifestLocal::PMLF_Anonymous))
			{
				// Not an anonymous object, this needs to actually be storable in this package
				symbolDomain = ml->objectGUID.m_domain;
				if(symbolDomain != rdxSDomainGUID::Builtin(rdxDOMAIN_Duplicable) && symbolDomain != domain)
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
			}

			// TODO: Is this necessary?
			ml->objectGUID.m_domain = symbolDomain;
		}
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROWV(ctx, rdxCRef(rdxCPackage)::Null());
	}
	RDX_ENDTRY

	return pkg;
}

// =======================================================================================
// Writing
void rdxWriteHeader(rdxIObjectManager *objm, rdxIFileStream *fs, bool isText, rdxLargeUInt numLocalSymbols, rdxLargeUInt numImports, rdxLargeUInt numStrings, rdxLargeUInt numArrayDefs)
{
	if(!isText)
	{
		const char *magic = RDX_MAGIC;
		fs->WriteBytes(magic, 16);
		fs->WriteConverted<rdxLargeUInt, rdxBinPackageLargeUInt>(numImports);
		fs->WriteConverted<rdxLargeUInt, rdxBinPackageLargeUInt>(numLocalSymbols);
		fs->WriteConverted<rdxLargeUInt, rdxBinPackageLargeUInt>(numStrings);
		fs->WriteConverted<rdxLargeUInt, rdxBinPackageLargeUInt>(numArrayDefs);
	}
}

static void rdxWriteObjectGUID(rdxIFileStream *fs, bool isText, rdxSObjectGUID guid)
{
	if(isText)
	{
		static const char *nibbles = "0123456789abcdef";

		char domainHex[sizeof(guid.m_bytes) * 2];
		char objectNameHex[sizeof(guid.m_bytes) * 2];

		for(rdxLargeUInt i=0;i<sizeof(guid.m_domain.m_bytes);i++)
		{
			domainHex[i*2+0] = nibbles[(guid.m_domain.m_bytes[i] >> 4) & 0xf];
			domainHex[i*2+1] = nibbles[guid.m_domain.m_bytes[i] & 0xf];
		}

		for(rdxLargeUInt i=0;i<sizeof(guid.m_bytes);i++)
		{
			objectNameHex[i*2+0] = nibbles[(guid.m_bytes[i] >> 4) & 0xf];
			objectNameHex[i*2+1] = nibbles[guid.m_bytes[i] & 0xf];
		}

		fs->WriteBytes("'{", 2);
		fs->WriteBytes(domainHex, sizeof(domainHex));
		fs->WriteBytes("}' '{", 5);
		fs->WriteBytes(objectNameHex, sizeof(objectNameHex));
		fs->WriteBytes("}'", 2);
	}
	else
	{
		fs->WriteBytes(guid.m_domain.m_bytes, sizeof(guid.m_domain.m_bytes));
		fs->WriteBytes(guid.m_bytes, sizeof(guid.m_bytes));
	}
}

static void rdxWriteLargeUInt(rdxIFileStream *fs, bool isText, rdxLargeUInt v)
{
	if(isText)
	{
		rdxChar chars[RDX_MAX_ENCODED_NUMBER_SIZE];
		rdxUInt8 encoded[RDX_MAX_ENCODED_NUMBER_SIZE];

		rdxEncodeString(chars, v);
		rdxLargeUInt len;
		for(len=0;chars[len];len++)
			encoded[len] = static_cast<rdxUInt8>(chars[len]);
		fs->WriteBytes(" ", 1);
		fs->WriteBytes(encoded, len);
	}
	else
	{
		fs->WriteConverted<rdxLargeUInt, rdxBinPackageLargeUInt>(v);
	}
}

static rdxSObjectGUID rdxGUIDForRuntimeObject(rdxLargeUInt ssid)
{
	// SSID should be non-zero, since 0 is NULL
	rdxSObjectGUID guid;
	guid.m_domain = rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime);
	for(rdxLargeUInt i=0;i<sizeof(guid.m_bytes);i++)
	{
		guid.m_bytes[i] = static_cast<rdxUInt8>(ssid & 0xff);
		ssid >>= 8;
	}
	return guid;
}

void rdxWriteImport(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxIFileStream *fs, bool isText, const rdxCSSIDTable *ssidTable, rdxSObjectGUID symbolName)
{
	// Serialize name
	if(isText)
	{
		fs->WriteBytes("import ", 7);
		rdxWriteObjectGUID(fs, isText, symbolName);
		fs->WriteBytes("\n", 1);
	}
	else
		rdxWriteObjectGUID(fs, isText, symbolName);
}

void rdxReserveObjectExport(rdxIObjectManager *objm, rdxIFileStream *fs, bool isText, const rdxCSSIDTable *ssidTable, rdxWeakHdl(rdxCObject) obj)
{
	const rdxITypeSerializer *strs = objm->GetBuiltIns()->st_String->m_native.user.typeSerializer;

	if(!isText)
	{
		rdxGCInfo *info = obj->ObjectInfo();

		rdxBinPackageLargeUInt offset = 0;
		fs->WriteBytes(&offset, sizeof(rdxBinPackageLargeUInt));

		{
			const rdxCStaticLookupPODKey<rdxBaseHdl::PODType> containerInfoKey(info->containerType.ToWeakHdl().GetPOD());
			fs->WriteConverted<rdxLargeUInt, rdxBinPackageLargeUInt>(*(ssidTable->Lookup(containerInfoKey)));
		}

		rdxUInt32 flags = 0;
		fs->WriteSwappableBytes(&flags, sizeof(flags));

		rdxSSerializationTag *serTag = info->SerializationTag();
		if(serTag && !serTag->isAnonymous)
			rdxWriteObjectGUID(fs, false, rdxSObjectGUID::Invalid());
	}
}

void rdxWriteObjectExport(rdxIObjectManager *objm, rdxIFileStream *fs, bool isText, rdxLargeUInt *catalogOffset, rdxLargeUInt objectOffset, rdxWeakRTRef(rdxCObject) obj)
{
	if(!isText)
	{
		rdxSDomainGUID *dummyDomain = NULL;

		rdxLargeUInt resumeLoc = fs->Tell();
		fs->SeekStart(*catalogOffset);
		fs->WriteConverted<rdxLargeUInt, rdxBinPackageLargeUInt>(objectOffset);
		// Offset, SSID, flags
		(*catalogOffset) += sizeof(rdxBinPackageLargeUInt) + sizeof(dummyDomain->m_bytes) + sizeof(rdxBinPackageLargeUInt) + sizeof(rdxUInt32);
		{
			rdxGCInfo *gcinfo = obj->ObjectInfo();
			rdxSSerializationTag *serTag = gcinfo->SerializationTag();
			if(serTag && !serTag->isAnonymous)
			{
				rdxSObjectGUID *dummyObject = NULL;
				(*catalogOffset) += sizeof(dummyObject->m_bytes);
			}
		}
		fs->SeekStart(resumeLoc);
	}
}
		
void rdxWriteReference(rdxIObjectManager *objm, rdxIFileStream *fs, bool isText, const rdxCSSIDTable *ssidTable, rdxWeakHdl(rdxCObject) obj, bool resTag)
{
	const rdxITypeSerializer *strs = objm->GetBuiltIns()->st_String->m_native.user.typeSerializer;
	if(isText)
	{
		if(obj.IsNotNull())
		{
			const rdxGCInfo *info = obj->ObjectInfo();
			if(info->containerType == objm->GetBuiltIns()->st_String)
				objm->SerializeTextString(obj.ToWeakRTRef().StaticCast<rdxCString>(), fs);
			else
			{
				if(resTag)
					fs->WriteBytes(" res ", 5);

				rdxSSerializationTag *serTag = info->SerializationTag();
				if(serTag && !serTag->isAnonymous)
					rdxWriteObjectGUID(fs, true, serTag->gstSymbol);
				else
				{
					rdxCStaticLookupPODKey<rdxBaseHdl::PODType> infoKey(obj.GetPOD());

					rdxSObjectGUID guid = rdxGUIDForRuntimeObject(*ssidTable->Lookup(infoKey));
					rdxWriteObjectGUID(fs, true, guid);
				}
			}
		}
		else
			fs->WriteBytes(" null", 5);
	}
	else
	{
		if(obj.IsNotNull())
		{
			rdxCStaticLookupPODKey<rdxBaseHdl::PODType> infoKey(obj.GetPOD());
			fs->WriteConverted<rdxLargeUInt, rdxBinPackageLargeUInt>(*(ssidTable->Lookup(infoKey)));
		}
		else
		{
			rdxBinPackageLargeUInt i = 0;
			fs->WriteSwappableBytes(&i, sizeof(rdxBinPackageLargeUInt));
		}
	}
}

void rdxWriteStructure(rdxIObjectManager *objm, rdxIFileStream *fs, bool isText, const rdxCSSIDTable *ssidTable, rdxWeakHdl(rdxCStructuredType) st, rdxWeakTypelessOffsetRTRef obj, int depth);

void rdxWriteValue(rdxIObjectManager *objm, rdxIFileStream *fs, bool isText, const rdxCSSIDTable *ssidTable, rdxWeakHdl(rdxCType) t, rdxWeakTypelessOffsetRTRef obj, int depth)
{
	if(objm->TypeIsObjectReference(t.ToWeakRTRef()))
		rdxWriteReference(objm, fs, isText, ssidTable, obj.StaticCast<rdxTracedRTRef(rdxCObject)>()->ToWeakHdl(), true);
	else if(objm->TypeIsInterface(t.ToWeakRTRef()))
	{
		const rdxSObjectInterfaceImplementation *oii = obj.StaticCast<rdxTracedTypelessIRef>().Data()->GetPOD();
		rdxWeakHdl(rdxCObject) obj = rdxWeakHdl(rdxCObject)::Null();
		if(oii != RDX_CNULL)
			obj = rdxWeakHdl(rdxCObject)(rdxObjRef_CSignal_DataPointer, oii->GetImplementingObject());
		rdxWriteReference(objm, fs, isText, ssidTable, obj, true);
	}
	else
		rdxWriteStructure(objm, fs, isText, ssidTable, t.StaticCast<rdxCStructuredType>(), obj, depth);
}

void rdxWriteValue(rdxIObjectManager *objm, rdxIFileStream *fs, bool isText, const rdxCSSIDTable *ssidTable, rdxWeakHdl(rdxCType) t, rdxWeakTypelessOffsetRTRef obj)
{
	rdxWriteValue(objm, fs, isText, ssidTable, t, obj, 0);
}

void rdxWriteBulk(rdxIObjectManager *objm, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, rdxWeakHdl(rdxCStructuredType) st, rdxLargeUInt count, rdxWeakTypelessOffsetRTRef obj, int depth)
{
	rdxLargeUInt stride = st->m_native.size;
	bool shouldSwap = rdxIFileStream::ShouldByteSwap();

	if(!rdxIFileStream::ShouldByteSwap())
	{
		fs->WriteBytes(obj.Data(), stride * count);
		return;
	}

	rdxUInt8 zeroPad = 0;

	// Target is padded and byte swapped
	rdxWeakArrayHdl(rdxSProperty) properties = st->properties.ToWeakHdl();
	rdxWeakArrayHdl(rdxLargeUInt) offsets = st->m_native.propertyOffsets.ToWeakHdl();

	rdxLargeUInt nProperties = 0;
	if(st->properties.IsNotNull())
		nProperties = properties->NumElements();

	while(count--)
	{
		if(st->storageSpecifier == rdxSS_Enum)
		{
			fs->WriteSwappedBytes(obj.Data(), sizeof(rdxEnumValue));
		}
		else if(st->storageSpecifier == rdxSS_Class ||
			st->storageSpecifier == rdxSS_RefStruct ||
			st->storageSpecifier == rdxSS_ValStruct)
		{
			// TODO MUSTFIX: Make sure properties that are out of order aren't bulk serializable
			rdxLargeUInt propertyOffset = 0;
			for(rdxLargeUInt i=0;i<nProperties;i++)
			{
#ifdef RDX_ENABLE_PADDED_BULK_SERIALIZE
				while(propertyOffset < offsets[i])
				{
					fs->WriteBytes(&zeroPad, 1);
					propertyOffset++;
				}
				propertyOffset += properties[i].type.StaticCast<rdxCStructuredType>()->m_native.size;
#endif
				rdxWriteValue(objm, fs, false, ssidTable, properties->Element(i).type.ToWeakHdl(), obj.StaticCast<rdxUInt8>() + offsets->Element(i), depth);
			}

#ifdef RDX_ENABLE_PADDED_BULK_SERIALIZE
			while(propertyOffset < stride)
			{
				fs->WriteBytes(&zeroPad, 1);
				propertyOffset++;
			}
#endif

			obj = obj.StaticCast<rdxUInt8>() + stride;
		}
		else
			fs->Abort();
	}
}

void rdxWriteStructure(rdxIObjectManager *objm, rdxIFileStream *fs, bool isText, const rdxCSSIDTable *ssidTable, rdxWeakHdl(rdxCStructuredType) st, rdxWeakTypelessOffsetRTRef obj, int depth)
{
	const rdxITypeSerializer *strs = objm->GetBuiltIns()->st_String->m_native.user.typeSerializer;
	if(depth > RDX_MAX_RECURSION_DEPTH)
	{
		fs->Abort();
		return;
	}

	if(st->m_native.user.typeSerializer)
	{
		bool shouldProcessProperties = true;
		if(isText)
		{
			st->m_native.user.typeSerializer->SerializeTextInstance(objm, obj.ToRTRef(), fs, ssidTable, shouldProcessProperties);
		}
		else
		{
			st->m_native.user.typeSerializer->SerializeBinaryInstance(objm, obj.ToRTRef(), fs, ssidTable, shouldProcessProperties);
		}
		if(!shouldProcessProperties)
			return;
	}

	if(st->storageSpecifier == rdxSS_Enum)
	{
		// Find the enum
		if(isText)
		{
			rdxWeakArrayHdl(rdxSEnumerant) enumerants = st->enumerants.ToWeakHdl();
			rdxLargeUInt nEnums = enumerants->NumElements();
			rdxEnumValue v = *obj.StaticCast<rdxEnumValue>().Data();
			rdxWeakHdl(rdxCString) s = rdxWeakHdl(rdxCString)::Null();

			for(rdxLargeUInt i=0;i<nEnums;i++)
			{
				if(enumerants->Element(i).value == v)
				{
					s = enumerants->Element(i).name;
					break;
				}
			}

			objm->SerializeTextString(s.ToWeakRTRef(), fs);

			return;
		}
		else
		{
			fs->WriteSwappableBytes(obj.Data(), sizeof(rdxEnumValue)); 
			return;
		}
	}

	// Write properties
	if(isText)
	{
		fs->WriteBytes("{\n", 2);
	}
			
	rdxLargeUInt nProperties = 0;
	if(st->properties.IsNotNull())
		nProperties = st->properties->NumElements();

	if(isText)
	{
		for(rdxLargeUInt i=0;i<nProperties;i++)
		{
			objm->SerializeTextString(st->properties->Element(i).name.ToWeakRTRef(), fs);
			fs->WriteBytes(" : ", 3);

			rdxWeakOffsetHdl(rdxSProperty) p = st->properties->OffsetElementRTRef(i).ToHdl();
			rdxWeakTypelessOffsetRTRef propertyLocation = obj.ReinterpretCast<rdxUInt8>() + st->m_native.propertyOffsets->Element(i);

			rdxWriteValue(objm, fs, isText, ssidTable, p->type.ToWeakHdl(), propertyLocation, depth + 1);

			fs->WriteBytes(",\n", 2);
		}
		fs->WriteBytes("}\n", 2);
	}
	else
	{
		bool bulk = ((st->m_native.flags & rdxCStructuredType::NativeProperties::STF_AllowBulkSerialize) != 0);

		if(bulk)
		{
			rdxWriteBulk(objm, fs, ssidTable, st, 1, obj, depth + 1);
		}
		else
		{
			rdxWeakArrayHdl(rdxSProperty) properties = st->properties.ToWeakHdl();
			rdxWeakArrayHdl(rdxLargeUInt) offsets = st->m_native.propertyOffsets.ToWeakHdl();

			for(rdxLargeUInt i=0;i<nProperties;i++)
			{
				rdxWeakTypelessOffsetRTRef propertyLocation = obj.StaticCast<rdxUInt8>() + offsets->Element(i);
				rdxWriteValue(objm, fs, isText, ssidTable, properties->Element(i).type.ToWeakHdl(), propertyLocation, depth + 1);
			}
		}
	}
}

void rdxWriteArray(rdxIObjectManager *objm, rdxIFileStream *fs, bool isText, const rdxCSSIDTable *ssidTable, rdxWeakHdl(rdxCArrayOfType) aot, rdxWeakRTRef(rdxCArrayContainer) obj)
{
	if(aot->numDimensions > 1)
	{
		for(rdxLargeUInt i=0;i<aot->numDimensions;i++)
		{
			rdxLargeUInt d = obj->Dimension(i);

			if(isText)
			{
				rdxWriteLargeUInt(fs, true, d);
				fs->WriteBytes(" ", 1);
			}
			else
			{
				rdxWriteLargeUInt(fs, false, d);
			}
		}
	}

	rdxWeakHdl(rdxCType) subType = aot->type.ToWeakHdl();
	rdxLargeUInt elementSize = objm->TypeElementSize(aot.ToWeakRTRef());

	rdxWeakOffsetRTRef(rdxUInt8) byteBase = obj.ReinterpretCast<rdxCArray<rdxUInt8> >()->OffsetElementRTRef(0);

	// Write out elements
	if(isText)
		fs->WriteBytes("{\n", 2);

	rdxLargeUInt totalElements = obj->NumElements();
	if(isText == false
		&& (subType->ObjectInfo()->containerType == objm->GetBuiltIns()->st_StructuredType)
		&& (subType.StaticCast<rdxCStructuredType>()->m_native.flags & rdxCStructuredType::NativeProperties::STF_AllowBulkSerialize)
		&& !objm->TypeIsObjectReference(subType.ToWeakRTRef())
		&& !objm->TypeIsInterface(subType.ToWeakRTRef())
		)
	{
		// Bulk serializable values
		rdxWriteBulk(objm, fs, ssidTable, subType.StaticCast<rdxCStructuredType>(), totalElements, byteBase, 0);
	}
	else
	{
		for(rdxLargeUInt i=0;i<totalElements;i++)
		{
			rdxWriteValue(objm, fs, isText, ssidTable, subType, byteBase, 0);
			byteBase += elementSize;

			if(isText)
				fs->WriteBytes(",\n", 2);
		}
	}
	if(isText)
		fs->WriteBytes("}", 1);
}

void rdxWriteObject(rdxIObjectManager *objm, rdxIFileStream *fs, bool isText, const rdxCSSIDTable *ssidTable, rdxWeakRTRef(rdxCObject) obj)
{
	const rdxGCInfo *objInfo = obj->ObjectInfo();
	rdxWeakHdl(rdxCType) objType = objInfo->containerType.ToWeakHdl();

	bool isAnonymous = (objInfo->SerializationTag() == NULL);

	if(isText)
	{
		fs->WriteBytes("\n\ndef ", 6);
		rdxWriteReference(objm, fs, isText, ssidTable, obj.ToWeakHdl(), false);
		fs->WriteBytes(" ", 1);
		if(isAnonymous)
			fs->WriteBytes("anonymous ", 10);
		if(objInfo->objectFlags & rdxGCInfo::GCOF_ConstantStructure)
			fs->WriteBytes("const ", 6);
		rdxWriteReference(objm, fs, isText, ssidTable, objType, false);
		fs->WriteBytes(" ", 1);
	}

	if(objType->ObjectInfo()->containerType == objm->GetBuiltIns()->st_StructuredType)
	{
		rdxWeakHdl(rdxCStructuredType) objST = objType.StaticCast<rdxCStructuredType>();

		rdxWriteStructure(objm, fs, isText, ssidTable, objST, obj.NoOffset(), 0);
	}
	else if(objType->ObjectInfo()->containerType == objm->GetBuiltIns()->st_ArrayOfType)
	{
		rdxWriteArray(objm, fs, isText, ssidTable, objType.StaticCast<rdxCArrayOfType>(), obj.StaticCast<rdxCArrayContainer>());
	}
	else
		fs->Abort();	// Don't know what happened, but this shouldn't be allowed
}

void rdxSPackageReference::ConvertToReference(rdxSOperationContext *ctx, rdxWeakHdl(rdxCPackage) pkg, bool allowCloaked, rdxWeakHdl(rdxCObject) *outRef)
{
	RDX_TRY(ctx)
	{
		rdxWeakHdl(rdxCObject) ref;
		switch(this->symbolLoc)
		{
		case rdxPSL_Null:
			ref = rdxWeakHdl(rdxCObject)::Null();
			break;
		case rdxPSL_String:
			ref = pkg->m_native.strings->Element(this->index);
			break;
		case rdxPSL_Array:
			ref = pkg->m_native.arrayDefs->Element(this->index).resolvedArrayType.ToWeakHdl();
			break;
		case rdxPSL_Imported:
			ref = pkg->m_native.importedSymbols->Element(this->index).resolution.ToWeakHdl();
			break;
		case rdxPSL_Local:
			ref = pkg->m_native.localSymbols->Element(this->index).resolvedObject.ToWeakHdl();
			break;
		default:
			RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);
		}

		if(ref.IsNotNull() && (ref->ObjectInfo()->objectFlags & rdxGCInfo::GCOF_Cloaked))
			RDX_STHROW(ctx, RDX_ERROR_UNRESOLVED_SYMBOL_REFERENCE);
		*outRef = ref;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}
