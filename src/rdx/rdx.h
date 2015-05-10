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
#ifndef __RDX_GFG_H__
#define __RDX_GFG_H__

#include "rdx_pragmas.hpp"
#include "rdx_platform.hpp"
#include "rdx_builtindomain.hpp"
#include "rdx_coretypes.hpp"
#include "rdx_api.hpp"

void rdxInitialize();

struct rdxSOperationContext;

struct rdxSAllocator;
struct rdxIObjectManager;
struct rdxINativeTypeHost;

struct rdxICodeProvider;
struct rdxSRuntimeTrace;
class rdxCPrecompiledCodeModule;
class rdxCString;
struct rdxSJournal;
class rdxCJournalScanState;
struct rdxSRuntimeStackFrame;
union rdxURuntimeStackValue;
struct rdxSDomainGUID;

RDX_DYNLIB_API rdxIObjectManager *RDX_CreateObjectManager(const rdxSAllocator *alloc, const rdxINativeTypeHost *nth, const rdxICodeProvider *cp);
RDX_DYNLIB_API void RDX_ExportMethods(rdxIObjectManager *objm, const char *outBasePath, rdxSDomainGUID domainGUID);
RDX_DYNLIB_API rdxICodeProvider *RDX_CreateInterpreterCodeProvider(const rdxSAllocator *alloc);

#if 0

RDX_DYNLIB_API void RDX_Initialize();

RDX_DYNLIB_API const rdxINativeTypeHost *RDX_GetNumericTypesPlugin();

RDX_DYNLIB_API int RDX_RuntimeTrace_TraceParent(const rdxSRuntimeTrace *trace, rdxSRuntimeTrace *outTrace);
RDX_DYNLIB_API rdxLargeInt RDX_RuntimeTrace_NumJournalEntries(const rdxSRuntimeTrace *trace);
RDX_DYNLIB_API void RDX_RuntimeTrace_JournalEntry(const rdxSRuntimeTrace *trace, rdxLargeInt index, rdxSJournal *outJournal);
RDX_DYNLIB_API void RDX_RuntimeTrace_ScanJournals(const rdxSRuntimeTrace *trace, rdxCJournalScanState *outJournalScanState);
RDX_DYNLIB_API void RDX_RuntimeTrace_GetFileNameAndLineNumber(const rdxSRuntimeTrace *trace, rdxHandle<rdxCString> *outFilename, rdxLargeInt *outLineNumber);

RDX_DYNLIB_API void RDX_JournalScanState_NextJournal(rdxCJournalScanState *jss, rdxSJournal *outJournal);

RDX_DYNLIB_API void RDX_ExportSource(rdxIObjectManager *objm, void *stdFile, int domain);


RDX_DYNLIB_API bool RDX_RuntimeUtilities_ArrayIndex(rdxBaseHandle o, const rdxURuntimeStackValue *rsv, rdxSUntypedOffsetHandle &outHandle);
RDX_DYNLIB_API void RDX_RuntimeUtilities_NewObjectInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm,
	rdxHandle<rdxCType> t, const rdxURuntimeStackValue *dimensions, rdxLargeInt numDimensions, rdxBaseHandle &outHandle);
RDX_DYNLIB_API int RDX_RuntimeUtilities_ExpandFrame(void *stackBase, void *frameBase, rdxLargeInt stackSize, rdxSRuntimeStackFrame **newBP, int hasInsertionPoint, int hasNativeFrame);
RDX_DYNLIB_API int RDX_RuntimeUtilities_EnterMethodInline(rdxIObjectManager *objm, rdxRef<rdxCMethod> m, rdxSRuntimeStackFrame *currentFrame,
	void *frameBase, void *stackBase, rdxURuntimeStackValue *prv, rdxSRuntimeStackFrame *newFrame);
RDX_DYNLIB_API int RDX_RuntimeUtilities_EnterMethodRoot(rdxIObjectManager *objm, rdxRef<rdxCMethod> m, rdxSRuntimeStackFrame *currentFrame, void *frameBase, void *stackBase,
	rdxURuntimeStackValue *prv, rdxSRuntimeStackFrame *newFrame, void *recordedInsertionPoint, int throughNative, rdxRef<rdxCMethod> viaMethod, const rdxSRuntimeStackFrame *subFrame);

#endif

#endif
