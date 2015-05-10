#include <new>
#include "rdx.h"
#include "rdx_objectmanagement.hpp"
#include "rdx_interpret.hpp"
#include "rdx_plugin.hpp"
#include "rdx_ilcomp.hpp"
#include "rdx_runtime.hpp"


RDX_DYNLIB_API rdxIObjectManager *RDX_CreateObjectManager(const rdxSAllocator *alloc, const rdxINativeTypeHost *nth, const rdxICodeProvider *cp)
{
	return rdxCreateObjectManager(*alloc, nth, cp);
}

RDX_DYNLIB_API rdxICodeProvider *RDX_CreateInterpreterCodeProvider(const rdxSAllocator *alloc)
{
	rdxCInterpreterCodeProvider *cp = alloc->CAlloc<rdxCInterpreterCodeProvider>(1, rdxALLOC_LongTerm);
	if(!cp)
		return NULL;
	new (cp) rdxCInterpreterCodeProvider(*alloc);
	return cp;
}

#if 0

RDX_DYNLIB_API void RDX_Initialize()
{
	rdxInitialize();
}

RDX_DYNLIB_API const rdxINativeTypeHost *RDX_GetNumericTypesPlugin()
{
	return rdxGetNumericTypesPlugin();
}

RDX_DYNLIB_API int RDX_RuntimeTrace_TraceParent(const rdxSRuntimeTrace *trace, rdxSRuntimeTrace *outTrace)
{
	return trace->TraceParent(*outTrace) ? 1 : 0;
}

RDX_DYNLIB_API rdxLargeInt RDX_RuntimeTrace_NumJournalEntries(const rdxSRuntimeTrace *trace)
{
	return trace->NumJournalEntries();
}

RDX_DYNLIB_API void RDX_RuntimeTrace_JournalEntry(const rdxSRuntimeTrace *trace, rdxLargeUInt index, rdxSJournal *outJournal)
{
	*outJournal = trace->JournalEntry(index);
}

RDX_DYNLIB_API void RDX_RuntimeTrace_ScanJournals(const rdxSRuntimeTrace *trace, rdxCJournalScanState *outJournalScanState)
{
	*outJournalScanState = trace->ScanJournals();
}

RDX_DYNLIB_API void RDX_RuntimeTrace_GetFileNameAndLineNumber(const rdxSRuntimeTrace *trace, rdxHdl<rdxCString> *outFilename, rdxLargeUInt *outLineNumber)
{
	trace->GetFileNameAndLineNumber(*outFilename, *outLineNumber);
}

RDX_DYNLIB_API void RDX_JournalScanState_NextJournal(rdxCJournalScanState *jss, rdxSJournal *outJournal)
{
	*outJournal = jss->NextJournal();
}

RDX_DYNLIB_API void RDX_ExportSource(rdxIObjectManager *objm, void *stdFile, int domain)
{
	rdxExportSource(objm, stdFile, static_cast<rdxEDomain>(domain));
}

RDX_DYNLIB_API bool RDX_RuntimeUtilities_ArrayIndex(const RDXBASEREF(void) &o, const rdxURuntimeStackValue *rsv, rdxSUntypedOffsetHandle &outHandle)
{
	rdxSUntypedOffsetRef outRef;
	rdxRTRef<
	bool result = rdxArrayIndex(o, rsv, outRef);
	outHandle = outRef.ToHandle();
	return result;
}

RDX_DYNLIB_API void RDX_RuntimeUtilities_NewObjectInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm,
	rdxHandle<rdxCType> t, const rdxURuntimeStackValue *dimensions, rdxLargeInt numDimensions, rdxBaseHandle &outHandle)
{
	outHandle = rdxNewObjectInstance(ctx, objm, t, dimensions, numDimensions).ToHandle().BaseHandle();
}

RDX_DYNLIB_API int RDX_RuntimeUtilities_ExpandFrame(void *stackBase, void *frameBase, rdxLargeInt stackSize, rdxSRuntimeStackFrame **newBP, int hasInsertionPoint, int hasNativeFrame)
{
	return rdxExpandFrame(stackBase, frameBase, stackSize, newBP, hasInsertionPoint != 0, hasNativeFrame != 0) ? 1 : 0;
}

RDX_DYNLIB_API int RDX_RuntimeUtilities_EnterMethodInline(rdxIObjectManager *objm, rdxRef<rdxCMethod> m, rdxSRuntimeStackFrame *currentFrame,
	void *frameBase, void *stackBase, rdxURuntimeStackValue *prv, rdxSRuntimeStackFrame *newFrame)
{
	return rdxEnterMethodInline(objm, m, currentFrame, frameBase, stackBase, prv, newFrame) ? 1 : 0;
}

RDX_DYNLIB_API int RDX_RuntimeUtilities_EnterMethodRoot(rdxIObjectManager *objm, rdxRef<rdxCMethod> m, rdxSRuntimeStackFrame *currentFrame, void *frameBase, void *stackBase,
	rdxURuntimeStackValue *prv, rdxSRuntimeStackFrame *newFrame, void *recordedInsertionPoint, int throughNative, rdxRef<rdxCMethod> viaMethod, const rdxSRuntimeStackFrame *subFrame)
{
	return rdxEnterMethodRoot(objm, m, currentFrame, frameBase, stackBase, prv, newFrame, recordedInsertionPoint, throughNative != 0, viaMethod, subFrame);
}

#endif
