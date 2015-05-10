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
#ifndef __RDX_RUNTIME_HPP__
#define __RDX_RUNTIME_HPP__

#include "rdx_processing.hpp"
#include "rdx_gcslots.hpp"
#include "rdx_typeprocessor.hpp"

/*
Stack frame layout:
   <RDX_MAX_ALIGNMENT padding> <Opstack> <Locals> <Previous frame data>
   ^-- FrameBase (on calls, location varies)      ^--- BP (aligned to RDX_MAX_ALIGNMENT)

    <Parameters> <Return values>
	             ^--- PRV (aligned to ALIGN_RuntimeStackValue)
*/


template<class _TindexCharType, class _TactualCharType> class rdxSStaticLookupStringKey;
template<class _Tkey, class _TlookupType> class rdxCStaticLookupTable;

struct rdxIObjectManager;
struct rdxISerializer;
struct rdxIPackageHost;

class rdxCMethod;
struct rdxSRuntimeStackFrame;

enum rdxEPointerSourceType
{
	rdxPST_Object			= 0,
	rdxPST_PinnedLocal		= 1,
	rdxPST_PinnedParameter	= 2,
	rdxPST_OffsetParameter	= 3,
	rdxPST_Parameter		= 4,	// Is an actual parameter
	rdxPST_Invalid			= 5,
};

struct rdxSJournal
{
	void					*value;
	rdxCRef(rdxCType)		type;
	bool					isActive;
	bool					isParameter;
	bool					isPointer;
	bool					isVarying;
	const void				*pointerSource;		// Other value in the frame that this pointer was based on
	rdxCRef(rdxCString)		name;
	rdxEPointerSourceType	pointerSourceType;
};

class rdxCJournalScanState
{
private:
	rdxLargeUInt				m_currentILInstruction;
	rdxWeakOffsetHdl(rdxUInt8)	m_data;
	const void					*m_bp;
	void						*m_prv;
public:
	rdxCJournalScanState(rdxWeakArrayRTRef(rdxUInt8) data, const void *bp, void *prv, rdxLargeUInt currentILInstruction);
	rdxSJournal NextJournal();
};

struct rdxSRuntimeTrace
{
	rdxCRef(rdxCMethod)				method;
	rdxLargeUInt					currentILInstruction;
	rdxURuntimeStackValue			*prv;
	const rdxSRuntimeStackFrame		*bp;
	const void						*ip;

	rdxTracedRTRef(rdxCMethod)		*methodStackLoc;		// For reference visiting

	bool TraceParent(rdxSRuntimeTrace &outTrace) const;
	rdxLargeUInt NumJournalEntries() const;
	rdxSJournal JournalEntry(rdxLargeUInt index) const;
	rdxCJournalScanState ScanJournals() const;
	void GetFileNameAndLineNumber(rdxWeakHdl(rdxCString) &outFilename, rdxLargeUInt &outLineNumber) const;
};

struct rdxSRuntimeStackFrame
{
	rdxTracedRTRef(rdxCMethod)	method;
	const void					*ip;				// Location to resume at (in compiled code, NOT IL, must always match up with the patch list)
	rdxURuntimeStackValue		*prv;
	rdxSRuntimeStackFrame		*bp;

	bool Trace(rdxSRuntimeTrace &outTrace) const;
};

// Root-level frame, only appears at points initiated from the API
struct rdxRuntimeStackFrameRoot : public rdxSRuntimeStackFrame
{
	void	*insertionPoint;	// Location to insert new calls into the stack at.  If this is the end of the stack, no more information is present
	bool	aboveNative;		// True if this call is above a native call
};

// Root-level frame from reentry, present if insertionPoint isn't the end of the stack
struct rdxRuntimeStackFrameReentrant : public rdxRuntimeStackFrameRoot
{
	rdxTracedRTRef(rdxCMethod)	nativeMethod;
	rdxSRuntimeStackFrame		subNativeFrame;
};

class rdxCRuntimeThread : public rdxCObject
{
	RDX_DECLARE_PROPERTY_LOOKUP;

public:
	rdxCRuntimeThread(rdxIObjectManager *objm, rdxGCInfo *info);

	rdxSRuntimeStackFrame				frame;
	rdxSRuntimeStackFrame				precallFrame;		// Filled by Precall, copied to frame on invoke
	rdxTracedRTRef(rdxCMethod)			activeNativeMethod;	// Currently active native method, set to NULL if it's below an interpreter stack
	rdxTracedRTRef(rdxCException)		ex;
	rdxLargeUInt						stackCapacity;
	rdxIObjectManager					*ownerObjectManager;
	void								*insertionPoint;

	struct DeserializationState
	{
		typedef void super;

		struct StoredJournal
		{
			typedef void super;

			rdxTracedRTRef(rdxCType)	vType;			// Expected type
			rdxLargeInt					stackOffset;	// Absolute offset into the stack
			rdxBool						isPointer;
			rdxBool						isVarying;

			void VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable);
		};

		struct StoredFrame
		{
			typedef void super;

			rdxTracedRTRef(rdxCMethod)	method;
			rdxLargeInt					currentILInstruction;
			rdxLargeInt					bpOffset;
			rdxLargeInt					prvOffset;

			void VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable);
		};

		bool										deserializing;
		rdxTracedRTRef(rdxCArray<StoredJournal>)	storedJournals;
		rdxTracedRTRef(rdxCArray<StoredFrame>)		storedFrames;
		rdxUInt8									*stackOccupationBits;

		void VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable);
	};

	rdxAtomicUInt						timeout;

	DeserializationState				deserializationState;

	rdxUInt8							*stackBytes;

	bool Trace(rdxSRuntimeTrace &outTrace) const;
	virtual void Precall(rdxSOperationContext *ctx, void **ppFramePointer, rdxLargeUInt sizeOfParameters, rdxLargeUInt sizeOfReturnValues);

	bool Recover(rdxSOperationContext *ctx);				// Attempts to recover the current frame from an exceptional state to an exception handler
	int Resume(rdxSOperationContext *ctx, int timeout);		// Resumes an active thread
	void Reset();

	// Forces the timeout counter for the thread to 1, attempting to make it time out as soon as possible
	void ForceTimeout() RDX_POSSIBLY_VOLATILE;

	void VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable);
};
RDX_DECLARE_COMPLEX_NATIVE_CLASS(rdxCRuntimeThread);
RDX_DECLARE_COMPLEX_NATIVE_STRUCT(rdxCRuntimeThread::DeserializationState);
RDX_DECLARE_COMPLEX_NATIVE_STRUCT(rdxCRuntimeThread::DeserializationState::StoredJournal);
RDX_DECLARE_COMPLEX_NATIVE_STRUCT(rdxCRuntimeThread::DeserializationState::StoredFrame);

class rdxCThreadSerializer : public rdxITypeSerializer
{
public:
	void DeserializeTextInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakTypelessOffsetHdl instance, rdxITextDeserializer *td, rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool &outShouldProcessProperties) const RDX_OVERRIDE;
	void DeserializeBinaryInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakTypelessOffsetHdl instance, rdxIFileStream *reader, rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool &outShouldProcessProperties) const RDX_OVERRIDE;

	void SerializeBinaryInstance(rdxIObjectManager *objm, rdxWeakTypelessOffsetRTRef obj, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties) const RDX_OVERRIDE;
	void SerializeTextInstance(rdxIObjectManager *objm, rdxWeakTypelessOffsetRTRef obj, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties) const RDX_OVERRIDE;

	void SerializeCommon(rdxIObjectManager *objm, rdxWeakTypelessOffsetRTRef obj, rdxIFileStream *fs, bool isText, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties);
	void DeserializeCommon(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakOffsetHdl(void) instance,
		rdxIFileStream *fs, rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool isText, bool &outShouldProcessProperties) const;

	static rdxCThreadSerializer instance;
};

enum rdxERuntimeState
{
	rdxRS_AbandonFrame	= 2,
	rdxRS_Active		= 1,
	rdxRS_TimedOut		= 0,
	rdxRS_Suspended		= -1,
	rdxRS_Exception		= -2,
	rdxRS_Exited		= -3,
};

struct rdxICodeProvider
{
	virtual void CreateExecutable(rdxIObjectManager *objm, rdxSOperationContext *ctx, rdxWeakHdl(rdxCMethod) m) const = 0;
	virtual void ReleaseExecutable(rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m) const = 0;
	virtual void InitializeSymbolDictionary(rdxSOperationContext *ctx, rdxIObjectManager *objm) const = 0;
	virtual int RunMethod(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, rdxURuntimeStackValue *prv) const = 0;

	virtual rdxIPToCurrentInstructionCallback GetIPToCurrentInstructionCallback() const = 0;
	virtual rdxInstrNumToIPCallback GetInstrNumToIPCallback() const = 0;
	virtual rdxResumeThreadCallback GetResumeThreadCallback(rdxIObjectManager *objm) const = 0;
	virtual void Shutdown() = 0;
};

struct rdxSPrecompiledFunctionInfo
{
	rdxLargeInt functionNum;
	const void *compiledInstructions;
};

class rdxCPrecompiledCodeModule
{
public:
	typedef int (*PrecompiledFunction)(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCRuntimeThread) thread, const rdxLargeUInt *instrTable, rdxWeakRTRef(rdxTracedRTRef(void)) providerDictionary);

private:
	const void *m_instructionTable;
	rdxLargeUInt m_instructionTableSize;
	const rdxCStaticLookupTable<rdxSStaticLookupStringKey<char, char>, rdxSPrecompiledFunctionInfo> *m_functionLookupTable;

public:
	rdxCPrecompiledCodeModule(const void *instrTable, rdxLargeUInt instrTableSize, const rdxCStaticLookupTable<rdxSStaticLookupStringKey<char, char>, rdxSPrecompiledFunctionInfo> *functionLookupTable);

	const void *IPForMethod(rdxWeakHdl(rdxCMethod) method) const;
	bool CanRunIP(rdxWeakHdl(rdxCMethod) method, const void *ip) const;
	const rdxSPrecompiledFunctionInfo *GetFunctionInfo(rdxWeakHdl(rdxCString) str) const;
	static rdxLargeUInt IPToCurrentInstruction(rdxWeakHdl(rdxCMethod) method, const void *ip);
	static const void *InstrNumToIP(rdxSOperationContext* ctx, rdxWeakHdl(rdxCMethod) method, rdxLargeUInt instrNum, bool *resumeAllowed);
	static int ResumeThread(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakRTRef(rdxCRuntimeThread) t, rdxPCCMCallback pccmCallback);
	static int RunNativeMethod(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) method, rdxWeakHdl(rdxCRuntimeThread) thread, rdxURuntimeStackValue *prv);
};

rdxCRef(rdxCObject) RDX_DECL_API rdxNewObjectInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCType) t, const void *dimensionsPtr, rdxLargeInt dimensionsStride, rdxLargeUInt numDimensions);

bool rdxArrayIndex(rdxWeakRTRef(void) o, const rdxURuntimeStackValue *rsv, rdxWeakOffsetRTRef(void) &outLooseReference);
bool rdxExpandFrame(void *stackBase, void *frameBase, rdxLargeUInt stackSize, rdxSRuntimeStackFrame **newBP, bool hasInsertionPoint, bool hasNativeFrame);
bool rdxEnterMethodInline(rdxIObjectManager *objm, rdxWeakRTRef(rdxCMethod) method, rdxSRuntimeStackFrame *currentFrame, void *frameBase,
	void *stackBase, rdxURuntimeStackValue *prv, rdxSRuntimeStackFrame *newFrame);
bool rdxEnterMethodRoot(rdxIObjectManager *objm, rdxWeakRTRef(rdxCMethod) method, rdxSRuntimeStackFrame *currentFrame, void *frameBase, void *stackBase,
	rdxURuntimeStackValue *prv, rdxSRuntimeStackFrame *newFrame, void *recordedInsertionPoint, bool throughNative, rdxWeakRTRef(rdxCMethod) viaMethod, const rdxSRuntimeStackFrame *subFrame);

class rdxCAutoRunFunction
{
public:
	inline rdxCAutoRunFunction(void (*initializer)())
	{
		initializer();
	}
};

enum rdxECommonExceptions
{
	rdxX_NullReferenceException,
	rdxX_IndexOutOfBoundsException,
	rdxX_AllocationFailureException,
	rdxX_IncompatibleConversionException,
	rdxX_InvalidOperationException,
	rdxX_UnspecifiedException,
	rdxX_StackOverflowException,
	rdxX_DivideByZeroException,

	rdxX_NumCommonExceptions,

	rdxX_NoException = rdxX_NumCommonExceptions,
};

#include "rdx_threading.hpp"

	
inline rdxCPrecompiledCodeModule::rdxCPrecompiledCodeModule(const void *instrTable, rdxLargeUInt instrTableSize, const rdxCStaticLookupTable<rdxSStaticLookupStringKey<char, char>, rdxSPrecompiledFunctionInfo> *functionLookupTable)
{
	m_instructionTable = instrTable;
	m_instructionTableSize = instrTableSize;
	m_functionLookupTable = functionLookupTable;
}

#endif
