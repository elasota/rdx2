#include <stdio.h>
#include "../rdx/rdx_marshal.hpp"
#include "../rdx/rdx.h"
#include "../rdx/rdx_io.hpp"
#include "../rdx/rdx_objectmanagement.hpp"
#include "../rdx/rdx_reftypealiases.hpp"
#include "../rdx/rdx_pragmas.hpp"
#include "../rdx/rdx_marshal.hpp"
#include "../rdx/rdx_plugin_api.hpp"
#include "systemPluginApi.hpp"

struct heapBlock
{
	rdxLargeUInt sentinelA;
	rdxLargeUInt size;
	rdxLargeUInt allocNumber;
	heapBlock *next;
	rdxLargeUInt sentinelB;
};

#define HEAPBLOCK_SIZE	32

struct heapBlock *currentBlock = NULL;

__declspec(align(16)) char hugeBuffer[400000000];

struct heapBlock *firstBlock = NULL;
rdxLargeUInt allocNum = 0;

bool enableHeapAudits = false;

void auditHeap()
{
	if(!enableHeapAudits)
		return;
	heapBlock *block = firstBlock;
	if(!block)
		return;
	int allocsScanned = 0;
	heapBlock *prevBlock;

	while(true)
	{
		if(block->sentinelA != block->sentinelB)
			__asm { int 3 }
		if(block->sentinelA == 0x66666666)
			return;
		heapBlock *nextBlock = reinterpret_cast<heapBlock*>(reinterpret_cast<char*>(block) + HEAPBLOCK_SIZE + block->size);
		if(block->next != nextBlock)
			__asm { int 3 }
		rdxUInt8 *bytes = reinterpret_cast<rdxUInt8*>(block) + HEAPBLOCK_SIZE;
		if(block->sentinelA == 0x44444444)
			for(rdxLargeUInt i=0;i<block->size;i++)
				if(bytes[i] != 0xfe)
					__asm { int 3 }
		prevBlock = block;
		block = nextBlock;
		allocsScanned++;
	}
}

void *myReallocReal(void *opaque, void *ptr, rdxLargeUInt sz, rdxLargeUInt align, rdxEAllocUsage allocUsage)
{
	//if(_heapchk() != _HEAPOK)
	//	rdxDebugBreak(rdxBREAKCAUSE_CriticalInternalFailure);
	return _aligned_realloc(ptr, sz, align);
}

void *myRealloc(void *opaque, void *ptr, rdxLargeUInt sz, rdxLargeUInt align, rdxEAllocUsage allocUsage)
{
	auditHeap();

	sz += (sz + 31);
	sz -= sz % 32;

	heapBlock *oldBlock = NULL;
	heapBlock *oldFirstBlock = firstBlock;
	if(!firstBlock)
	{
		memset(hugeBuffer, 0x66, sizeof(hugeBuffer));
		currentBlock = firstBlock = reinterpret_cast<heapBlock*>(hugeBuffer);
	}
	
	if(ptr != NULL)
		oldBlock = reinterpret_cast<heapBlock*>(reinterpret_cast<char*>(ptr) - HEAPBLOCK_SIZE);

	void *outPtr = NULL;

	if(sz != 0)
	{
		heapBlock *newBlock = currentBlock;
		if(currentBlock->sentinelA != 0x66666666 || currentBlock->sentinelB != 0x66666666)
		{
			__asm { int 3 }
		}
		newBlock->size = sz;
		newBlock->allocNumber = allocNum++;
		newBlock->sentinelA = newBlock->sentinelB = 0x33333333;
		currentBlock = reinterpret_cast<heapBlock*>(reinterpret_cast<char*>(newBlock) + HEAPBLOCK_SIZE + sz);
		newBlock->next = currentBlock;

		outPtr = reinterpret_cast<char*>(newBlock) + HEAPBLOCK_SIZE;

		memset(reinterpret_cast<char*>(newBlock) + HEAPBLOCK_SIZE, 0x77, sz);

		if(oldBlock)
		{
			if(oldBlock->sentinelA != 0x33333333 || oldBlock->sentinelB != 0x33333333)
			{
				__asm { int 3 }
			}
			rdxLargeUInt copySize = oldBlock->size;
			if(copySize > sz)
				copySize = sz;
			memcpy(outPtr, reinterpret_cast<char*>(oldBlock) + HEAPBLOCK_SIZE, copySize);
		}
	}
	if(oldBlock)
	{
		if(oldBlock->sentinelA != 0x33333333 || oldBlock->sentinelB != 0x33333333)
		{
			__asm { int 3 }
		}
		oldBlock->sentinelA = oldBlock->sentinelB = 0x44444444;
		memset(reinterpret_cast<char*>(oldBlock) + HEAPBLOCK_SIZE, 0xfe, oldBlock->size);
	}
	auditHeap();

	return outPtr;
}

class MyFileStream : public rdxIFileStream
{
private:
	FILE *m_f;
	bool m_aborted;

public:
	MyFileStream(FILE *f)
	{
		m_f = f;
		m_aborted = false;
	}
	
	virtual rdxLargeUInt WriteBytes(const void *src, rdxLargeUInt numBytes)
	{
		if(!m_aborted)
			return fwrite(src, 1, numBytes, m_f);
		return 0;
	}

	virtual rdxLargeUInt ReadBytes(void *dest, rdxLargeUInt numBytes)
	{
		if(!m_aborted)
			return fread(dest, 1, numBytes, m_f);
		return 0;
	}

	virtual void SeekStart(rdxLargeUInt offset)
	{
		if(!m_aborted)
			if(fseek(m_f, static_cast<long>(offset), SEEK_SET))
				m_aborted = true;
	}

	virtual void SeekEnd(rdxLargeInt offset)
	{
		if(!m_aborted)
			if(fseek(m_f, static_cast<long>(offset), SEEK_END))
				m_aborted = true;
	}

	virtual void SeekCurrent(rdxLargeInt offset)
	{
		if(!m_aborted)
			if(fseek(m_f, static_cast<long>(offset), SEEK_CUR))
				m_aborted = true;
	}

	virtual void SeekForward(rdxLargeUInt offset)
	{
		if(!m_aborted)
			if(fseek(m_f, static_cast<long>(offset), SEEK_CUR))
				m_aborted = true;
	}

	virtual rdxLargeUInt Tell()
	{
		if(m_aborted)
			return 0;
		return static_cast<size_t>(ftell(m_f));
	}

	virtual void Close()
	{
		fclose(m_f);
		delete this;
	}

	virtual bool HasAborted() const
	{
		return m_aborted;
	}

	virtual void Abort()
	{
		m_aborted = true;
	}
};

class MyPackageHost : public rdxIPackageHost
{
public:
	
	virtual rdxIFileStream *StreamForDomain(rdxIObjectManager *om, rdxSDomainGUID domain, bool write, bool &isText)
	{
		if(!write)
		{
			FILE *f = NULL;
			if(domain == rdxSDomainGUID::FromName("Core"))
				f = fopen("../rdxclasses/Core.rxt", "rb");
			else if(domain == rdxSDomainGUID::FromName("Apps.MyApp"))
				f = fopen("../rdxclasses/Apps/MyApp.rxt", "rb");
			else if(domain == rdxSDomainGUID::FromName("Apps.Common"))
				f = fopen("../rdxclasses/Apps/Common.rxt", "rb");
			else if(domain == rdxSDomainGUID::FromName("RDX.Compiler"))
				f = fopen("../rdxclasses/RDX/Compiler.rxt", "rb");
			isText = true;
			if(f)
				return new MyFileStream(f);
			return NULL;
		}
		return NULL;
	}

	virtual bool DomainsVisible(rdxSDomainGUID sourceDomain, rdxSDomainGUID destDomain)
	{
		return true;
	}

	virtual bool DomainCanContainMethods(rdxSDomainGUID domain)
	{
		return true;
	}
};

class MyNTH : public rdxINativeTypeHost
{
	rdxIfcTypeInfo TypeInfoForType(rdxWeakRTRef(rdxCStructuredType) st) const RDX_OVERRIDE
	{
		if(st.IsNotNull() && st->ObjectInfo()->SerializationTag())
		{
			rdxSObjectGUID stGUID = st->ObjectInfo()->SerializationTag()->gstSymbol;
			// TODO: Delete this
			rdxUInt64 codedDomain = 0;
			rdxUInt64 codedObject = 0;
			for(rdxLargeUInt i=0;i<rdxSDomainGUID::GUID_SIZE;i++)
				codedDomain = ((codedDomain << 8) | (stGUID.m_domain.m_bytes[i]));
			for(rdxLargeUInt i=0;i<rdxSObjectGUID::GUID_SIZE;i++)
				codedObject = ((codedObject << 8) | (stGUID.m_bytes[i]));

			rdxLargeUInt numPlugins = rdxTestPluginApi->NumPlugins();
			for(rdxLargeUInt i=0;i<numPlugins;i++)
			{
				const rdxSPluginExport *plugin = rdxTestPluginApi->GetAPI(i);
				if(plugin && plugin->ntiLookups)
				{
					const rdxSPluginDomainNTILookup *domainLookup = plugin->ntiLookups;
					while(domainLookup->objects)
					{
						if(domainLookup)
						{
							if(domainLookup->domainGUID == codedDomain)
							{
								for(rdxLargeUInt obj=0;obj<domainLookup->numObjects;obj++)
								{
									if(domainLookup->objects[obj].objectGUID == codedObject)
									{
										// TODO MUSTFIX: Dumb hack
										st->m_native.getPropertyOffsetFunc = domainLookup->objects[obj].getPropertyOffsetFunc;
										return domainLookup->objects[obj].typeInfo;
									}
								}
								break;
							}
						}
						domainLookup++;
					}
				}
			}
		}

		rdxIfcTypeInfo ti;
		ti.fetchFunc = RDX_CNULL;
		return ti;
	}

	static int RDX_DECL_API DummyCallback(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
	{
		rdxDebugBreak(rdxBREAKCAUSE_Unimplemented);
		return 0;
	}

	virtual rdxNativeCallback HookMethod(rdxSObjectGUID methodGUID) const RDX_OVERRIDE
	{
		// TODO: Delete this
		rdxUInt64 codedDomain = 0;
		rdxUInt64 codedObject = 0;
		for(rdxLargeUInt i=0;i<rdxSDomainGUID::GUID_SIZE;i++)
			codedDomain = ((codedDomain << 8) | (methodGUID.m_domain.m_bytes[i]));
		for(rdxLargeUInt i=0;i<rdxSObjectGUID::GUID_SIZE;i++)
			codedObject = ((codedObject << 8) | (methodGUID.m_bytes[i]));

		rdxLargeUInt numPlugins = rdxTestPluginApi->NumPlugins();
		for(rdxLargeUInt i=0;i<numPlugins;i++)
		{
			const rdxSPluginExport *plugin = rdxTestPluginApi->GetAPI(i);
			if(plugin && plugin->functionLookups)
			{
				const rdxSPluginDomainFunctionLookup *domainLookup = plugin->functionLookups;
				while(domainLookup->objects)
				{
					if(domainLookup)
					{
						if(domainLookup->domainGUID == codedDomain)
						{
							for(rdxLargeUInt obj=0;obj<domainLookup->numObjects;obj++)
							{
								if(domainLookup->objects[obj].objectGUID == codedObject)
									return domainLookup->objects[obj].callMethod;
							}
							break;
						}
					}
					domainLookup++;
				}
			}
		}
		return DummyCallback;
	}

	virtual rdxLargeUInt NumPlugins() const RDX_OVERRIDE
	{
		return rdxTestPluginApi->NumPlugins();
	}

	virtual void GetPlugin(rdxLargeUInt pluginIndex, const rdxSPCCMDomainIndex **pPCCM, const rdxSPluginExport **pPlugin) const RDX_OVERRIDE
	{
		*pPlugin = rdxTestPluginApi->GetAPI(pluginIndex);
		*pPCCM = rdxTestPluginApi->GetPCCM(pluginIndex);
	}
};

#include "../rdxcompression/rdx_comp.hpp"

int main(int argc, const char **argv)
{
	MyPackageHost myPackageHost;
	rdxSAllocator alloc;
	alloc.opaque = NULL;
	alloc.reallocFunc = myReallocReal;
	alloc.auditHeapFunc = auditHeap;
	MyNTH nth;

	rdxTestPluginApi->Init();

	//RDX_Initialize();
	rdxICodeProvider *cp = RDX_CreateInterpreterCodeProvider(&alloc);
	rdxIObjectManager *objm = RDX_CreateObjectManager(&alloc, &nth, cp);

	if(false)
	{
		rdxSOperationContext myContext(objm);
		rdxCRef(rdxCPackage) package1 = objm->LoadPackage(&myContext, rdxSDomainGUID::FromName("Apps.Common"), &myPackageHost);
		rdxCRef(rdxCPackage) package2 = objm->LoadPackage(&myContext, rdxSDomainGUID::FromName("Apps.MyApp"), &myPackageHost);
		rdxCRef(rdxCPackage) packageCompiler = objm->LoadPackage(&myContext, rdxSDomainGUID::FromName("RDX.Compiler"), &myPackageHost);
		
		RDX_ExportMethods(objm, "rdxmidcode", rdxSDomainGUID::FromName("RDX.Compiler"));
		RDX_ExportMethods(objm, "rdxmidcode", rdxSDomainGUID::FromName("Core"));
		RDX_ExportMethods(objm, "rdxmidcode", rdxSDomainGUID::FromName("Apps.MyApp"));
		RDX_ExportMethods(objm, "rdxmidcode", rdxSDomainGUID::FromName("Apps.Common"));
	}
	if(false)
	{
		rdxSOperationContext myContext(objm);

		rdxSObjectGUID mainID = rdxSObjectGUID::FromObjectName("Apps.MyApp", "MyApp/methods/main(#Core.string[C])");
		rdxCRef(rdxCMethod) method = objm->LookupSymbol(&myContext, mainID, &myPackageHost).StaticCast<rdxCMethod>();

		enableHeapAudits = true;
		rdxCRef(rdxCRuntimeThread) thread;
		thread = objm->CreateThread(&myContext, 32768);

		rdxCStackViewPrecall stackView(thread->stackBytes + thread->stackCapacity);
		rdxCStackObjectRefProxy<rdxCArray<rdxTracedRTRef(rdxCString)> > args(stackView, rdxSTACKVIEWTYPE_Parameter);
		rdxCStackValueProxy<rdxUInt> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);
		void *fp;
		thread->Precall(&myContext, &fp, stackView.ParameterSize(), stackView.ReturnValueSize());

		args.Modify() = rdxWeakArrayRTRef(rdxTracedRTRef(rdxCString))::Null();
		method->Invoke(&myContext, thread.ToWeakHdl(), 10000);
	}
	if(true)
	{
		rdxSOperationContext myContext(objm);

		rdxSObjectGUID mainID = rdxSObjectGUID::FromObjectName("RDX.Compiler", "Compiler/methods/TempTest()");
		rdxCRef(rdxCMethod) method = objm->LookupSymbol(&myContext, mainID, &myPackageHost).StaticCast<rdxCMethod>();

		rdxCRef(rdxCRuntimeThread) thread;
		thread = objm->CreateThread(&myContext, 32768);

		rdxCStackViewPrecall stackView(thread->stackBytes + thread->stackCapacity);
		void *fp;
		thread->Precall(&myContext, &fp, stackView.ParameterSize(), stackView.ReturnValueSize());
		method->Invoke(&myContext, thread.ToWeakHdl(), 1000000);
	}
	return 0;
}
