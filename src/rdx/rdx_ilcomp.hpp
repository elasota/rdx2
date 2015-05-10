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
#ifndef __RDX_ILCOMP_HPP__
#define __RDX_ILCOMP_HPP__

#include "rdx_basictypes.hpp"
#include "rdx_programmability.hpp"
#include "rdx_intrinsics.hpp"
#include "rdx_ilopcodes.hpp"
#include "rdx_runtime.hpp"

extern const char *rdxILOpcodeNames[];

class rdxCMethod;
struct rdxIObjectManager;
struct rdxSOperationContext;
class rdxCString;

void rdxCompileMethod(rdxSOperationContext *ctx, rdxWeakHdl(rdxCMethod) m, rdxIObjectManager *objm);
void rdxJournalNativeMethodParameters(rdxSOperationContext *ctx, rdxWeakHdl(rdxCMethod) method, rdxIObjectManager *objm);
void rdxCompactJournals(rdxSOperationContext *ctx, rdxWeakHdl(rdxCMethod) m, rdxWeakArrayHdl(rdxSStackJournal) journals, rdxLargeUInt numJournals, rdxIObjectManager *objm, rdxLargeUInt &outCompactSize, rdxLargeUInt &outNumDropped);

union rdxUILOpCompactArg
{
	rdxLargeInt li;
	rdxLargeUInt lui;
	rdxBaseHdl::PODType p;
	rdxUInt32 u32;
	rdxUInt16 u16;
	rdxUInt8 u8;

	rdxFloat f;
	rdxChar c;
	rdxInt si;
	rdxShort s;
	rdxByte b;
	rdxEnumValue ev;
	rdxBool bo;
	rdxImmediateType it;
};
RDX_DECLARE_SIMPLE_NATIVE_STRUCT(rdxUILOpCompactArg);

union rdxUILOpLargeArg
{
	rdxDouble dbl;
	rdxLong l;
	rdxOffsetHdlPOD rtp;
	rdxUInt64 u64;
	rdxUILOpCompactArg ca[2];
};
RDX_DECLARE_SIMPLE_NATIVE_STRUCT(rdxUILOpLargeArg);

enum rdxEMILStackActionType
{
	rdxEMILSAT_Speculative,
	
	rdxEMILSAT_PushParentFrameReturnValue,
	rdxEMILSAT_PushParentFrameParameter,
	rdxEMILSAT_PushLocal,
	rdxEMILSAT_PushOpstackValue,
	rdxEMILSAT_PushOpstackPtr,
	rdxEMILSAT_PushOpstackVarying,
	rdxEMILSAT_Pop,
};

struct rdxSMILStackAction
{
	rdxEMILStackActionType actionType;
	rdxWeakHdl(rdxCType) valueType;
};
RDX_DECLARE_SIMPLE_NATIVE_STRUCT(rdxSMILStackAction);

// The stack journal tracks the lifespan of objects on the stack at points where execution may stop.
// Out-of-frame exceptions always cause a timeslice to stop with STATUS_EXCEPTION, except the root frame which
// receives STATUS_FATAL instead.
// Reverse-direction jumps will also decrement the timeslice counter, causing STATUS_SUSPENDED if too many occur

//                                                May throw     Slice timer behavior      ILOP_tick inserted    Next instruction patched
// - Calls to functions                           Yes           Decrement                 Yes                   Yes
// - Jumps that go backwards                      No            Decrement if backwards    Yes                   No
// - Throws                                       Yes           Suspend                   No                    Yes
// - Array index lookups                          Yes           None                      No                    Yes
// - Object NULL checks                           Yes           None                      No                    Yes
// - Pointer mutability verification              Yes           None                      No                    Yes
// - Polymorphic casts                            Yes           None                      Yes                   Yes
struct rdxSStackJournal
{
private:
	inline static rdxLargeUInt PushRef(rdxUInt8 **bytesOutH, const rdxWeakHdl(rdxCObject) &p)
	{
		if(bytesOutH)
		{
			rdxBaseHdl::PODType tempPOD = p.GetPOD();
			memcpy(*bytesOutH, &tempPOD, sizeof(rdxBaseHdl::PODType));
			(*bytesOutH) += sizeof(rdxBaseHdl::PODType);
		}
		return sizeof(rdxBaseHdl::PODType);
	}

	inline static rdxWeakHdl(rdxCObject) PopRef(rdxUInt8 const** bytesInH, rdxLargeUInt &nBytes)
	{
		rdxBaseHdl::PODType hdl;
		const rdxUInt8 *bytesIn = *bytesInH;
		memcpy(&hdl, *bytesInH, sizeof(rdxBaseHdl::PODType));

		(*bytesInH) += sizeof(rdxBaseHdl::PODType);
		nBytes += sizeof(rdxBaseHdl::PODType);
		return rdxWeakHdl(rdxCObject)::FromPtr(rdxRefConverter<rdxCObject>::Convert(hdl));
	}

	inline static rdxLargeUInt PushSInt(rdxUInt8 **bytesOutH, const rdxLargeInt &vp)
	{
		rdxLargeInt v = vp;
		rdxUInt8 bytes[sizeof(rdxLargeInt)*2];
		rdxLargeUInt nBytes = 0;

		while(true)
		{
			rdxInt8 extendedSmall = static_cast<rdxInt8>((v & 0x7f) << 1);
			bytes[nBytes++] = static_cast<rdxUInt8>(v & 0x7f);
			if(extendedSmall / 2 != v)
			{
				bytes[nBytes-1] |= 0x80;
				v >>= 7;
			}
			else
				break;
		}

		if(bytesOutH)
		{
			rdxUInt8 *outB = *bytesOutH;
			for(rdxLargeUInt i=0;i<nBytes;i++)
				outB[i] = bytes[i];
			*bytesOutH = outB + nBytes;
		}

		return nBytes;
	}

	inline static rdxLargeUInt PushUInt(rdxUInt8 **bytesOutH, const rdxLargeUInt &vp)
	{
		rdxLargeUInt v = vp;
		rdxUInt8 bytes[sizeof(rdxLargeUInt)*2];
		rdxLargeUInt nBytes = 0;

		while(true)
		{
			bytes[nBytes++] = static_cast<rdxUInt8>(v & 0x7f);
			if((v & 0x7f) != v)
			{
				bytes[nBytes-1] |= 0x80;
				v >>= 7;
			}
			else
				break;
		}

		if(bytesOutH)
		{
			rdxUInt8 *outB = *bytesOutH;
			for(rdxLargeUInt i=0;i<nBytes;i++)
				outB[i] = bytes[i];
			*bytesOutH = outB + nBytes;
		}

		return nBytes;
	}

	inline static rdxLargeInt PopSInt(rdxUInt8 const** bytesInH, rdxLargeUInt &nBytes)
	{
		rdxLargeUInt bitOffs = 0;
		rdxLargeInt dec = 0;
		const rdxUInt8 *bytesIn = *bytesInH;
		while(true)
		{
			nBytes++;
			rdxUInt8 b = *bytesIn;
			++bytesIn;
			dec |= ((static_cast<rdxLargeInt>(b) & 0x7f) << bitOffs);
			bitOffs += 7;

			if((b & 0x80) == 0)
				break;
		}
		rdxLargeUInt headRoom = static_cast<rdxLargeInt>(sizeof(rdxLargeInt) * 8) - bitOffs;
		// Sign extend
		if(headRoom > 0)
		{
			dec <<= headRoom;
			dec >>= headRoom;
		}
		*bytesInH = bytesIn;
		return dec;
	}
	
	inline static rdxLargeUInt PopUInt(rdxUInt8 const** bytesInH, rdxLargeUInt &nBytes)
	{
		rdxLargeUInt bitOffs = 0;
		rdxLargeUInt dec = 0;
		const rdxUInt8 *bytesIn = *bytesInH;

		while(true)
		{
			nBytes++;
			rdxUInt8 b = *bytesIn;
			++bytesIn;
			dec |= ((static_cast<rdxLargeInt>(b) & 0x7f) << bitOffs);
			bitOffs += 7;

			if((b & 0x80) == 0)
				break;
		}
		*bytesInH = bytesIn;
		return dec;
	}

public:
	rdxWeakHdl(rdxCType) vType;
	rdxLargeInt offset;
	rdxLargeUInt startInstruction;		// Instruction that creates this value.  The value starts existing on the NEXT instruction.
	rdxLargeUInt endInstruction;		// Instruction that destroys this value. The value stops existing on the NEXT instruction.
	bool isPointer;
	bool isVarying;
	bool isConstant;

	bool isNotNull;						// Can be guaranteed to be non-NULL after NullCheckOffset
	rdxLargeUInt notNullInstruction;	// IL instruction that enforces this value as non-NULL.  The value is guaranteed to be non-NULL on the NEXT instruction.

	bool isParameter;					// Offset is relative to PRV instead of BP, only trace if stack goes into native code
	bool isFromLocal;					// For integrity enforcement when serializing copy-to-pointer returns
	bool isTraceable;					// Contains traceable references
	rdxWeakHdl(rdxCString) name;		// For debugging
	rdxLargeInt pointerSource;			// Offset in the same frame that this is from if it's a thread-relative pointer
	rdxEPointerSourceType pointerSourceType;

	inline void Compress(rdxWeakOffsetHdl(rdxUInt8) *bytesOut, rdxLargeUInt *nBytesOut) const
	{
		rdxUInt8 *bytesOutPtr;
		rdxUInt8 **bytesOutH = NULL;
		if(bytesOut)
		{
			bytesOutPtr = bytesOut->Modify();
			bytesOutH = &bytesOutPtr;
		}
		
		rdxLargeUInt nBytes = 0;
		nBytes += PushRef(bytesOutH, vType);
		nBytes += PushRef(bytesOutH, name);
		nBytes += PushSInt(bytesOutH, offset);
		nBytes += PushUInt(bytesOutH, startInstruction);
		nBytes += PushUInt(bytesOutH, endInstruction);
		nBytes += PushUInt(bytesOutH, notNullInstruction);
		nBytes += PushSInt(bytesOutH, pointerSource);
		nBytes += PushUInt(bytesOutH, pointerSourceType);
		rdxLargeUInt flags = 0;
		if(isPointer) flags |= 1;
		if(isVarying) flags |= 2;
		if(isConstant) flags |= 4;
		if(isNotNull) flags |= 8;
		if(isParameter) flags |= 16;
		if(isFromLocal) flags |= 32;
		if(isTraceable) flags |= 64;
		nBytes += PushUInt(bytesOutH, flags);

		if(nBytesOut)
			*nBytesOut = nBytes;
	}

	inline void Decompress(const rdxWeakOffsetHdl(rdxUInt8) *bytesIn, rdxLargeUInt *nBytesOut)
	{
		rdxUInt8 const* bytesInPtr;
		rdxUInt8 const** bytesInH = RDX_CNULL;
		if(bytesIn)
		{
			bytesInPtr = bytesIn->Data();
			bytesInH = &bytesInPtr;
		}
		rdxLargeUInt nBytes = 0;
		vType = PopRef(bytesInH, nBytes).StaticCast<rdxCType>();
		name = PopRef(bytesInH, nBytes).StaticCast<rdxCString>();
		offset = PopSInt(bytesInH, nBytes);
		startInstruction = PopUInt(bytesInH, nBytes);
		endInstruction = PopUInt(bytesInH, nBytes);
		notNullInstruction = PopUInt(bytesInH, nBytes);
		pointerSource = PopSInt(bytesInH, nBytes);
		pointerSourceType = static_cast<rdxEPointerSourceType>(PopUInt(bytesInH, nBytes));
		rdxLargeUInt flags = PopUInt(bytesInH, nBytes);

		isPointer = ((flags & 1) != 0);
		isVarying = ((flags & 2) != 0);
		isConstant = ((flags & 4) != 0);
		isNotNull = ((flags & 8) != 0);
		isParameter = ((flags & 16) != 0);
		isFromLocal = ((flags & 32) != 0);
		isTraceable = ((flags & 64) != 0);

		if(nBytesOut)
			*nBytesOut = nBytes;
	}
};
RDX_DECLARE_SIMPLE_NATIVE_STRUCT(rdxSStackJournal);

struct rdxSExceptionHandlerJournal
{
	rdxLargeUInt startInstruction;		// First instruction covered by this
	rdxLargeUInt endInstruction;		// Last instruction covered by this
	rdxLargeUInt handlerInstruction;	// Handler instruction to jump to
	rdxLargeInt catchStackOffset;
};
RDX_DECLARE_SIMPLE_NATIVE_STRUCT(rdxSExceptionHandlerJournal);

struct rdxSILInstruction
{
	rdxEILOpcode opcode;
	rdxLargeUInt argOffs;
};
RDX_DECLARE_SIMPLE_NATIVE_STRUCT(rdxSILInstruction);

struct rdxSMarkupInstruction
{
	rdxEMarkupOpcode opcode;
	rdxLargeUInt instructionLink;
	rdxLargeUInt argOffs;
};
RDX_DECLARE_SIMPLE_NATIVE_STRUCT(rdxSMarkupInstruction);

struct rdxSExceptionRecoveryLocation
{
	rdxLargeUInt instrNum;
	rdxLargeInt bpOffset;
};
RDX_DECLARE_SIMPLE_NATIVE_STRUCT(rdxSExceptionRecoveryLocation);

struct rdxSILDebugInfo
{
	rdxLargeUInt firstInstruction;
	rdxWeakHdl(rdxCString) filename;
	rdxLargeUInt line;
};
RDX_DECLARE_SIMPLE_NATIVE_STRUCT(rdxSILDebugInfo);


// Call points are reduced information about where call points are in a function, used to validate deserialized frames
struct rdxSILCallPoint
{
	struct CallArgs
	{
		rdxBaseHdl::PODType func;
	};
			
	struct CallDelegateArgs
	{
		rdxLargeInt offset;
		bool bpRelative;
	};

	struct CallVirtualArgs
	{
		rdxLargeInt bpOffset;
		rdxLargeUInt vftOffset;
	};

	struct CallInterfaceArgs
	{
		rdxLargeInt bpOffset;
		rdxLargeInt vftOffset;
		rdxBaseHdl::PODType soughtInterface;
	};

	rdxEILOpcode ilOpcode;
	rdxLargeUInt instrNum;
	rdxLargeInt paramBaseOffset;
	rdxLargeInt prvOffset;

	union
	{
		CallArgs call;
		CallDelegateArgs callDelegate;
		CallVirtualArgs callVirtual;
		CallInterfaceArgs callInterface;
	} args;
};
RDX_DECLARE_SIMPLE_NATIVE_STRUCT(rdxSILCallPoint);

void rdxExportSource(rdxIObjectManager *objm, void *pf, rdxSDomainGUID domain);

#endif
