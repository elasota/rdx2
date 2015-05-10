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
#include "rdx_programmability.hpp"
#include "rdx_objectmanagement.hpp"
#include "rdx_ilcomp.hpp"
#include "rdx_runtime.hpp"
#include "rdx_longflow.hpp"
#include "rdx_io.hpp"
#include "rdx_varying.hpp"
#include "rdx_builtins.hpp"

// Attempts to expand a frame from frameBase to store a new BP and have space for stackSize bytes of additional storage
// Outputs newBP if successful, returns "false" if not enough space
bool rdxExpandFrame(void *stackBase, void *frameBase, rdxLargeUInt stackSize, rdxSRuntimeStackFrame **newBP, bool hasInsertionPoint, bool hasNativeFrame)
{
	rdxLargeUInt frameHeadSize;
	if(hasInsertionPoint)
	{
		if(hasNativeFrame)
			frameHeadSize = rdxPaddedSize(sizeof(rdxRuntimeStackFrameReentrant), RDX_MAX_ALIGNMENT);
		else
			frameHeadSize = rdxPaddedSize(sizeof(rdxRuntimeStackFrameRoot), RDX_MAX_ALIGNMENT);
	}
	else
		frameHeadSize = rdxPaddedSize(sizeof(rdxSRuntimeStackFrame), RDX_MAX_ALIGNMENT);

	rdxLargeUInt capacity = static_cast<rdxLargeUInt>(reinterpret_cast<const rdxUInt8*>(frameBase) - reinterpret_cast<const rdxUInt8*>(stackBase));

	if(capacity < frameHeadSize)
		return false;
	capacity -= frameHeadSize;

	if(capacity < stackSize)
		return false;

	*newBP = reinterpret_cast<rdxSRuntimeStackFrame*>(reinterpret_cast<rdxUInt8*>(frameBase) - frameHeadSize);

	return true;
}

bool rdxEnterMethodInline(rdxIObjectManager *objm, rdxWeakRTRef(rdxCMethod) method, rdxSRuntimeStackFrame *currentFrame, void *frameBase,
	void *stackBase, rdxURuntimeStackValue *prv, rdxSRuntimeStackFrame *newFrame)
{
	rdxSRuntimeStackFrame *frame;

	if(!rdxExpandFrame(stackBase, frameBase, method->m_native.frameCapacity, &frame, false, false))
		return false;

	// Copy the previous frame attributes
	*frame = *currentFrame;
	newFrame->method = method;
	newFrame->ip = RDX_CNULL;
	newFrame->prv = prv;
	newFrame->bp = frame;

	return true;
}

bool rdxEnterMethodRoot(rdxIObjectManager *objm, rdxWeakRTRef(rdxCMethod) method, rdxSRuntimeStackFrame *currentFrame, void *frameBase, void *stackBase,
	rdxURuntimeStackValue *prv, rdxSRuntimeStackFrame *newFrame, void *recordedInsertionPoint, bool throughNative, rdxWeakRTRef(rdxCMethod) viaMethod, const rdxSRuntimeStackFrame *subFrame)
{
	rdxSRuntimeStackFrame *frame;

	if(!rdxExpandFrame(stackBase, frameBase, method->m_native.frameCapacity, &frame, true, throughNative))
		return false;

	// Copy the previous frame attributes
	*frame = *currentFrame;

	rdxRuntimeStackFrameReentrant *extendedFrame = static_cast<rdxRuntimeStackFrameReentrant *>(frame);
	extendedFrame->insertionPoint = recordedInsertionPoint;
	extendedFrame->aboveNative = throughNative;
	if(throughNative)
	{
		extendedFrame->nativeMethod = viaMethod;
		extendedFrame->subNativeFrame = *subFrame;
	}
				
	newFrame->method = method;
	newFrame->ip = RDX_CNULL;
	newFrame->prv = prv;
	newFrame->bp = frame;

	return true;
}


rdxCRef(rdxCObject) RDX_DECL_API rdxNewObjectInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCType) t, const void *dimensionsPtr, rdxLargeInt dimensionsStride, rdxLargeUInt numDimensions)
{
	rdxLargeUInt totalElements = 1;
	for(rdxLargeUInt i=0;i<numDimensions;i++)
	{
		rdxLargeUInt d = *reinterpret_cast<const rdxLargeUInt*>(static_cast<const rdxUInt8*>(dimensionsPtr) + static_cast<rdxLargeInt>(i) * dimensionsStride);
		if(!rdxCheckMulOverflowU(d, totalElements))
			return rdxCRef(rdxCObject)::Null();

		if(!rdxCheckMulOverflowU(totalElements, d))
			RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);

		totalElements *= d;
	}

	RDX_TRY(ctx)
	{
		rdxSObjectGUID guid;
		guid.m_domain = rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime);
		memset(guid.m_bytes, 0, rdxSObjectGUID::GUID_SIZE);

		rdxCRef(rdxCObject) obj;
		RDX_PROTECT_ASSIGN(ctx, obj, objm->CreateInitialObject(ctx, t, totalElements, guid));

		objm->InitializeObject(obj.ToWeakRTRef(), 0, false);

		if(numDimensions > 0)
		{
			rdxCArrayContainer *arrayContainer = obj.ToWeakRTRef().StaticCast<rdxCArrayContainer>().Modify();
			for(rdxLargeUInt i=0;i<numDimensions;i++)
			{
				rdxLargeUInt d = *reinterpret_cast<const rdxLargeUInt*>(static_cast<const rdxUInt8*>(dimensionsPtr) + static_cast<rdxLargeInt>(i) * dimensionsStride);
				arrayContainer->SetDimension(i, d);
			}
		}

		return obj;
	}
	RDX_CATCH(ctx)
	{
		return rdxCRef(rdxCObject)::Null();
	}
	RDX_ENDTRY
}

#if 0

namespace RDX
{
	namespace Programmability
	{
		using namespace RDX::ILCompiler;
		using namespace RDX::ILOpcodes;
		using namespace RDX::ObjectManagement;
		using namespace RDX::Utility;

		static bool PlaceBytes(UInt8 *mask, rdxLargeInt offset, rdxLargeInt count, rdxLargeInt capacity)
		{
			if(count > capacity || capacity - count < offset)
				return false;

			if(offset < 0)
				return false;

			while( count && (offset & 0x7) )
			{
				UInt8 maskBit = static_cast<UInt8>(1 << (offset & 0x7));
				if(mask[offset >> 3] & maskBit)
					return false;
				mask[offset >> 3] |= maskBit;
				offset++;
				count--;
			}

			while(count >= 8)
			{
				if(mask[offset >> 3])
					return false;
				mask[offset >> 3] = 0xff;
				offset += 8;
				count -= 8;
			}

			while(count)
			{
				UInt8 maskBit = static_cast<UInt8>(1 << (offset & 0x7));
				if(mask[offset >> 3] & maskBit)
					return false;
				mask[offset >> 3] |= maskBit;
				offset++;
				count--;
			}

			return true;
		}

		bool RuntimeThread::Recover(rdxSOperationContext *ctx)
		{
			RDX_TRY(ctx)
			{
				RuntimeTrace trace;
				if(!Trace(trace))
					return false;
				do
				{
					const Method *method = trace.method;
					const GCInfo *methodGCI = GCInfo::From(method);

					if(method->_native.exHandlers)
					{
						rdxLargeInt numHandlers = GCInfo::From(method->_native.exHandlers)->numElements;
						for(rdxLargeInt i=0;i<numHandlers;i++)
						{
							// Check in reverse order, the last one covering this instruction is the one to use
							const ILCompiler::ExceptionHandlerJournal *ehj = method->_native.exHandlers + numHandlers - 1 - i;
							if(ehj->startInstruction <= trace.currentILInstruction && ehj->endInstruction >= trace.currentILInstruction)
							{
								rdxSRuntimeStackFrame *mutableTraceBP = const_cast<rdxSRuntimeStackFrame *>(trace.bp);
								this->frame.bp = mutableTraceBP;
								this->frame.method = trace.method;
								this->frame.prv = trace.prv;
								RDX_PROTECT_ASSIGN(ctx, this->frame.ip, trace.method->_native.instrNumToIP(ctx, trace.method, ehj->handlerInstruction, NULL) );
								*(reinterpret_cast<void**>(reinterpret_cast<UInt8*>(mutableTraceBP) + trace.method->_native.exceptionInsertionOffset)) = this->ex;
								this->ex = NULL;

								return true;
							}
						}
					}
					else if(method->_native.isNativeCall)
					{
						// Exception propagated to a native call, can't go any further.  The native call will have to call Collapse.
						return false;
					}
				} while(trace.TraceParent(trace));

				return false;
			}
			RDX_CATCH(ctx)
			{
				RDX_RETHROWV(ctx, false);
			}
			RDX_ENDTRY
		}

		int RuntimeThread::Resume(rdxSOperationContext *ctx, int timeout)
		{
			if(this->ex)
				RDX_LTHROWV(ctx, RDX_ERROR_RESUMED_UNRECOVERED_THREAD, RuntimeState::Exception);
			Threading::AtomicWrite(&this->timeout, timeout);
			return this->frame.method->_native.resumeThread(ctx, ownerObjectManager, this);
		}
		
		// Forces the timeout counter for the thread to 1, attempting to make it time out as soon as possible
		void RuntimeThread::ForceTimeout() RDX_POSSIBLY_VOLATILE
		{
			Threading::AtomicWrite(&timeout, 1);
		}

		

		


		// Thread processor
		void ThreadProcessor::Finalize(void *obj, rdxIObjectManager *objm) const
		{
			RuntimeThread *rt = static_cast<RuntimeThread*>(obj);
			if(rt->stackBytes)
				objm->GetAllocator()->Free(rt->stackBytes);
		}

		static bool PushFrame(rdxLargeInt &offset, rdxLargeInt size)
		{
			if(size > offset)
				return false;
			if(!CheckAddOverflow(size, rdxALIGN_RuntimeStackValue - 1))
				return false;
			rdxLargeInt paddedSize = size + (rdxALIGN_RuntimeStackValue - 1);
			paddedSize -= paddedSize % rdxALIGN_RuntimeStackValue;
			if(paddedSize > offset)
				return false;
			offset -= paddedSize;

			return true;
		}

		static bool ValidatePointer(rdxIObjectManager *objm, const void *site, const StructuredType *siteStructure,
			const Type *siteType, rdxLargeInt nElements, bool targetConstant, bool pointerIsConstant, rdxLargeInt offset, const Type *expectedType)
		{
			if(offset < 0)
				return false;
			rdxLargeInt elementSize = Utility::PaddedSize(sizeof(void*), RDX_ALIGN_POINTER);

			if(siteStructure)
				elementSize = siteStructure->_native.size;

			rdxLargeInt index = offset / elementSize;
			if(index >= nElements)
				return false;

			rdxLargeInt elementShift = index * elementSize;
			offset -= elementShift;
			site = reinterpret_cast<const UInt8*>(site) + offset;

			while(true)
			{
				if(!siteStructure)
				{
					// Value is a reference
					if(offset)
						return false;	// Needs to be an exact hit
					return objm->TypesCompatible(siteType, expectedType);
				}

				// Value is a structure
				if(offset == 0 && siteStructure == expectedType)
				{
					// Hit the site exactly, this is OK
					if(pointerIsConstant == false && targetConstant)
						return false;	// Const violation
					return true;
				}

				// Missed, try searching properties
				bool foundSpanningProperty = false;
				if(siteStructure->properties)
				{
					rdxLargeInt nProperties = GCInfo::From(siteStructure->properties)->numElements;
					for(rdxLargeInt i=0;i<nProperties;i++)
					{
						const Property *p = siteStructure->properties + i;
						rdxLargeInt pOffset = siteStructure->_native.propertyOffsets[i];
						if(pOffset > offset)
							return false;	// No match

						if(i == nProperties-1 || offset < siteStructure->_native.propertyOffsets[i+1])
						{
							// Offset's probably in this parameter
							rdxLargeInt shift = siteStructure->_native.propertyOffsets[i];
							const Type *newType = p->type;
							if(!newType)
								return false;	// Corrupt or something

							site = reinterpret_cast<const UInt8*>(site) + shift;
							offset -= shift;
							siteType = newType;
							siteStructure = NULL;
							if(p->isConstant != FalseValue)
								targetConstant = true;
							if(GCInfo::From(newType)->containerType == objm->GetBuiltIns()->st_StructuredType)
							{
								const StructuredType *st = static_cast<const StructuredType*>(newType);
								if(st->storageSpecifier == EnumValue::From(StructuredType::SS_ValStruct) ||
									st->storageSpecifier == EnumValue::From(StructuredType::SS_RefStruct))
									siteStructure = st;
							}

							foundSpanningProperty = true;
							break;
						}
					}
				}
				
				if(!foundSpanningProperty)
					return false;
			}

			return false;
		}

		bool ThreadProcessor::OnLoad(rdxSOperationContext *ctx, void *obj, rdxIObjectManager *objm) const
		{
			RDX_TRY(ctx)
			{
				RuntimeThread *t = static_cast<RuntimeThread*>(obj);
				if(t->deserializationState.deserializing)
				{
					rdxLargeInt numFrames = GCInfo::From(t->deserializationState.storedFrames)->numElements;
					rdxLargeInt numJournals = GCInfo::From(t->deserializationState.storedJournals)->numElements;
					
					// Make sure all frames are loaded
					for(rdxLargeInt i=0;i<numFrames;i++)
					{
						RuntimeThread::DeserializationState::StoredFrame *frame = t->deserializationState.storedFrames + i;

						if(frame->method == NULL)
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
						frame->method = static_cast<const Method *>(objm->ResolveShellableReference(frame->method));
						if(!frame->method)
							RDX_STHROW(ctx, RDX_ERROR_UNRESOLVED_SYMBOL_REFERENCE);

						if(GCInfo::From(frame->method)->gcflags & GCInfo::GCOF_Unprocessed)
							return false;
					}
					
					// Parse out all frames, verify and resolve journals
					rdxLargeInt prvOffset = 0;
					rdxLargeInt parameterBaseOffset = t->stackCapacity;
					rdxLargeInt frameOffset = 0;
					rdxLargeInt journalIndex = 0;
					rdxLargeInt numStoredJournals = 0;
					if(t->deserializationState.storedJournals)
						numStoredJournals = GCInfo::From(t->deserializationState.storedJournals)->numElements;

					// Previous frame state
					const Method *lastMethod = NULL;
					rdxSRuntimeStackFrame *lastFrame = NULL;
					const void *lastIP = NULL;
					rdxURuntimeStackValue *lastPRV = NULL;

					rdxLargeInt scannedJournal = 0;
					for(rdxLargeInt i=0;i<numFrames;i++)
					{
						const RuntimeThread::DeserializationState::StoredFrame *frame = t->deserializationState.storedFrames + i;

						const Method *method = frame->method;
						if(!method)
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

						if(!method->_native.numILInstructions)
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

						if(frame->currentILInstruction < 0 || frame->currentILInstruction >= method->_native.numILInstructions)
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

						// Regardless of the condition executed here, the end result must be that prvOffset and parameterBaseOffset are set
						if(i == 0)
						{
							// First frame, this needs to have parameters 
							if(method->returnTypes)
							{
								rdxLargeInt numReturns = GCInfo::From(method->returnTypes)->numElements;
								
								for(rdxLargeInt r=0;r<numReturns;r++)
								{
									const Type *returnType = method->returnTypes[r];
									if(!returnType)
										RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

									if(GCInfo::From(returnType)->containerType == objm->GetBuiltIns()->st_StructuredType)
									{
										const StructuredType *st = static_cast<const StructuredType *>(returnType);
										if(st->storageSpecifier == EnumValue::From(StructuredType::SS_RefStruct))
											RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);		// Can't have pointer return values at root level
									}
									
									rdxLargeInt size, align;
									RDX_PROTECT(ctx, objm->TypeValueSize(ctx, returnType, size, align));
									
									if(align > rdxALIGN_RuntimeStackValue)
										RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);			// Unalignable value on the stack

									bool overflowed;
									size = Utility::PaddedSize(size, rdxALIGN_RuntimeStackValue, overflowed);
									if(overflowed)
										RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
									if(!PushFrame(parameterBaseOffset, size))
										RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
								}
							}

							prvOffset = parameterBaseOffset;

							rdxLargeInt numParams = 0;
							if(method->parameters)
								numParams = GCInfo::From(method->parameters)->numElements;
							for(rdxLargeInt p=0;p<numParams;p++)
							{
								const MethodParameter *parameter = method->parameters + p;
								if(!parameter->type)
									RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

								rdxLargeInt size, align;
								if(GCInfo::From(parameter->type)->containerType == objm->GetBuiltIns()->st_StructuredType
									&& static_cast<const StructuredType *>(parameter->type)->storageSpecifier == EnumValue::From(StructuredType::SS_RefStruct))
								{
									size = sizeof(RuntimePointer<void>);
									align = ALIGN_RuntimePointer;
								}
								else
								{
									RDX_PROTECT(ctx, objm->TypeValueSize(ctx, parameter->type, size, align));
								}

								if(align > rdxALIGN_RuntimeStackValue)
									RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);			// Unalignable value on the stack

								bool overflowed;
								size = Utility::PaddedSize(size, rdxALIGN_RuntimeStackValue, overflowed);

								if(!PushFrame(parameterBaseOffset, size))
									RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
							}

							// Determine padding
							rdxLargeInt padding = parameterBaseOffset % RDX_MAX_ALIGNMENT;
							if(!PushFrame(parameterBaseOffset, padding))
								RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
						}
						else
						{
							const RuntimeThread::DeserializationState::StoredFrame *invokingFrame = t->deserializationState.storedFrames + i - 1;
							// Resolve actual values
							const Method *calledMethod = NULL;

							const ILCallPoint *callPoints = invokingFrame->method->_native.callPoints;
							const GCInfo *invokingMethodGCI = GCInfo::From(invokingFrame->method);
							rdxLargeInt numCallPoints = 0;
							rdxLargeInt currentInstrNum = invokingFrame->currentILInstruction;
							const ILCallPoint *callPoint = NULL;

							if(callPoints)
								numCallPoints = GCInfo::From(callPoints)->numElements;

							for(rdxLargeInt instri=0;instri<numCallPoints;instri++)
							{
								if(callPoints[instri].instrNum == currentInstrNum)
								{
									callPoint = callPoints + instri;
									break;
								}
							}

							if(!callPoint)
								RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

							switch(callPoint->ilOpcode)
							{
							case ILOP_call:
								calledMethod = callPoint->args.call.func;
								break;
							case ILOP_calldelegatebp:
								calledMethod = *reinterpret_cast<const Method *const*>(reinterpret_cast<const UInt8 *>(lastFrame) + callPoint->args.callDelegate.offset);
								break;
							case ILOP_calldelegateprv:
								calledMethod = *reinterpret_cast<const Method *const*>(t->stackBytes + prvOffset + callPoint->args.callDelegate.offset);
								break;
							case ILOP_callvirtual:
								{
									const void *thisObject = *reinterpret_cast<const void *const*>(reinterpret_cast<const UInt8 *>(lastFrame) + callPoint->args.callVirtual.bpOffset);
									if(!thisObject)
										RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
									const Type *ot = GCInfo::From(thisObject)->containerType;
									if(GCInfo::From(ot)->containerType != objm->GetBuiltIns()->st_StructuredType)
										RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

									const StructuredType *st = static_cast<const StructuredType*>(ot);
									if(!st->virtualMethods)
										RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
									rdxLargeInt numVirtualMethods = GCInfo::From(st->virtualMethods)->numElements;
									rdxLargeInt vidx = callPoint->args.callVirtual.vftOffset;
									if(vidx < 0 || vidx >= numVirtualMethods)
										RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
									calledMethod = st->virtualMethods[vidx];
								}
								break;
							case ILOP_callinterface:
								{
									const void *thisObject = *reinterpret_cast<const void *const*>(reinterpret_cast<const UInt8 *>(lastFrame) + callPoint->args.callInterface.bpOffset);
									if(!thisObject)
										RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
									const Type *ot = GCInfo::From(thisObject)->containerType;
									if(GCInfo::From(ot)->containerType != objm->GetBuiltIns()->st_StructuredType)
										RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

									const StructuredType *st = static_cast<const StructuredType*>(ot);
									if(!st->virtualMethods)
										RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
									rdxLargeInt numVirtualMethods = GCInfo::From(st->virtualMethods)->numElements;

									const StructuredType *soughtInterface = callPoint->args.callInterface.soughtInterface;

									rdxLargeInt numInterfaces = 0;
									if(st->interfaces)
										numInterfaces = GCInfo::From(st->interfaces)->numElements;
									rdxLargeInt vidx = -1;
									for(rdxLargeInt intfi=0;intfi<numInterfaces;intfi++)
									{
										if(st->interfaces[intfi].type == soughtInterface)
										{
											vidx = static_cast<rdxLargeInt>(st->interfaces[intfi].vftOffset) + callPoint->args.callInterface.vftOffset;
											break;
										}
									}

									if(vidx < 0 || vidx >= numVirtualMethods)
										RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
									calledMethod = st->virtualMethods[vidx];
								}
								break;
							default:
								RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
							};

							// Was this the actual method called?
							if(calledMethod != frame->method)
								RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

							// Determine where the frame should be
							parameterBaseOffset = frameOffset + callPoint->paramBaseOffset;
							prvOffset = frameOffset + callPoint->prvOffset;
						}
						
						bool isFirstFrame = (i == 0);
						
						// Lay down the frame
						rdxLargeInt paddedFrameSize = Utility::PaddedSize(sizeof(rdxSRuntimeStackFrame), RDX_MAX_ALIGNMENT);
						if(isFirstFrame)
							paddedFrameSize = Utility::PaddedSize(sizeof(RuntimeStackFrameRoot), RDX_MAX_ALIGNMENT);
						
						frameOffset = parameterBaseOffset;
						if(!PushFrame(frameOffset, paddedFrameSize))
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

						if(!PlaceBytes(t->deserializationState.stackOccupationBits, frameOffset, paddedFrameSize, t->stackCapacity))
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

						const RuntimeThread::DeserializationState::StoredFrame *sf = t->deserializationState.storedFrames + i;

						// Make sure this is an exact match
						if(sf->bpOffset != frameOffset || sf->prvOffset != prvOffset)
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);


						// Inject the frame
						InstrNumToIPCallback instrToIP = objm->GetCodeProvider()->GetInstrNumToIPCallback();

						bool resumeAllowed;
						const void *ip;
						// Frames below the current call need an IP of the instruction after the current one
						// The current frame on the other hand is suspended on the current instruction, since that represents its current stack state
						RDX_PROTECT_ASSIGN(ctx, ip, instrToIP(ctx, sf->method, sf->currentILInstruction + ((i == numFrames-1) ? 0 : 1), &resumeAllowed));
						if(!ip || !resumeAllowed)
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

						rdxSRuntimeStackFrame *rtframe = reinterpret_cast<rdxSRuntimeStackFrame*>(t->stackBytes + frameOffset);
						rtframe->bp = lastFrame;
						rtframe->ip = lastIP;
						rtframe->prv = lastPRV;
						rtframe->method = lastMethod;
						if(isFirstFrame)
						{
							RuntimeStackFrameRoot *rootFrame = static_cast<RuntimeStackFrameRoot *>(rtframe);
							rootFrame->insertionPoint = t->stackBytes + t->stackCapacity;
							rootFrame->aboveNative = false;
						}

						rdxURuntimeStackValue *prv = reinterpret_cast<rdxURuntimeStackValue*>(t->stackBytes + prvOffset);

						// Fix up and verify journals
						if(method->_native.numJournals)
						{
							rdxLargeInt instrNum = sf->currentILInstruction;

							rdxLargeInt numFrameJournals = method->_native.numJournals;
							const UInt8 *cjBytes = method->_native.compactedJournals;
							for(rdxLargeInt j=0;j<numFrameJournals;j++)
							{
								StackJournal sj;
								cjBytes += sj.Decompress(cjBytes);

								if(sj.isParameter && i != 0)
									continue;	// Looking for parameters, but it isn't the first frame
								if(!sj.isParameter && (sj.startInstruction >= instrNum || sj.endInstruction < instrNum))
									continue;	// Journal doesn't exist

								if(journalIndex == numStoredJournals)
									RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
								const RuntimeThread::DeserializationState::StoredJournal *storedJournal = t->deserializationState.storedJournals + journalIndex;
								journalIndex++;
								
								void *journalValue;
								if(sj.isParameter)
									journalValue = reinterpret_cast<void*>(reinterpret_cast<UInt8*>(prv) + sj.offset);
								else
									journalValue = reinterpret_cast<void*>(reinterpret_cast<UInt8*>(rtframe) + sj.offset);

								if(storedJournal->stackOffset != reinterpret_cast<const UInt8*>(journalValue) - reinterpret_cast<const UInt8*>(t->stackBytes))
									RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

								if(sj.isPointer || sj.isVarying)
								{
									if((storedJournal->isPointer != FalseValue) != sj.isPointer)
										RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
									if((storedJournal->isVarying != FalseValue) != sj.isVarying)
										RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

									RuntimePointer<void> *rtp = reinterpret_cast<RuntimePointer<void> *>(journalValue);
									if(!rtp->objectRef)
										RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
									rtp->objectRef = objm->ResolveShellableReference(rtp->objectRef);
									if(!rtp->objectRef)
										RDX_STHROW(ctx, RDX_ERROR_UNRESOLVED_SYMBOL_REFERENCE);

									rdxLargeInt pointerResolveNElements = 0;
									void *pointerResolveInitialSite = NULL;
									const StructuredType *pointerResolveInitialStructure = NULL;
									const Type *pointerResolveInitialType = NULL;
									bool pointerResolveConstant = false;

									switch(sj.pointerSourceType)
									{
									case PST_Object:
										pointerResolveInitialSite = rtp->objectRef;
										pointerResolveInitialType = 
											pointerResolveInitialStructure = GCInfo::From(rtp->objectRef)->containedStructure;
										pointerResolveNElements = GCInfo::From(rtp->objectRef)->numElements;
										pointerResolveConstant = objm->ObjectIsConstant(rtp->objectRef);
										break;
									case PST_PinnedLocal:
										{
											if(rtp->objectRef != t)
												RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

											bool found = false;
											const UInt8 *searchCJBytes = method->_native.compactedJournals;
											for(rdxLargeInt k=0;k<numFrameJournals;k++)
											{
												StackJournal searchSJ;
												searchCJBytes += searchSJ.Decompress(searchCJBytes);

												if(!searchSJ.isParameter && searchSJ.startInstruction < instrNum && searchSJ.endInstruction >= instrNum &&
													searchSJ.offset == sj.offset)
												{
													pointerResolveInitialSite = reinterpret_cast<UInt8*>(rtframe) + searchSJ.offset;
													pointerResolveInitialType = 
														pointerResolveInitialStructure = static_cast<const StructuredType*>(searchSJ.vType);	// Pinned locals are always structures
													pointerResolveNElements = 1;
													pointerResolveConstant = searchSJ.isConstant;
													found = true;
													break;
												}
											}
											if(!found)
												RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);
										}
										break;
									case PST_PinnedParameter:
									case PST_OffsetParameter:
										{
											// Pinned locals and parameters are guaranteed to be structures
											if(rtp->objectRef != t)
												RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

											bool found = false;
											const UInt8 *searchCJBytes = method->_native.compactedJournals;
											for(rdxLargeInt k=0;k<numFrameJournals;k++)
											{
												StackJournal searchSJ;
												searchCJBytes += searchSJ.Decompress(searchCJBytes);

												if(searchSJ.isParameter && searchSJ.startInstruction < instrNum && searchSJ.endInstruction >= instrNum &&
													searchSJ.offset == sj.offset)
												{
													pointerResolveInitialType = 
														pointerResolveInitialStructure = static_cast<const StructuredType*>(searchSJ.vType);
													if(sj.pointerSource == PST_PinnedParameter)
														pointerResolveInitialSite = reinterpret_cast<UInt8*>(prv) + searchSJ.offset;
													else
													{
														pointerResolveInitialSite = reinterpret_cast<const RuntimePointer<void> *>(reinterpret_cast<const UInt8*>(prv) + searchSJ.offset)->valueRef;
														// This might not be a structure
														if(pointerResolveInitialStructure->storageSpecifier != EnumValue::From(StructuredType::SS_RefStruct) &&
															pointerResolveInitialStructure->storageSpecifier != EnumValue::From(StructuredType::SS_ValStruct))
															pointerResolveInitialStructure = NULL;
													}
													pointerResolveNElements = 1;
													pointerResolveConstant = searchSJ.isConstant;
													found = true;
													break;
												}
											}
											if(!found)
												RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);
										}
										break;
									default:
										RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);
									};

									// Decode the offset from the valueRef, validate that this pointer is good, then relocate it relative to the initial site
									rdxLargeInt initialOffset = reinterpret_cast<const UInt8*>(rtp->valueRef) - reinterpret_cast<const UInt8*>(NULL);
									if(!ValidatePointer(objm, pointerResolveInitialSite, pointerResolveInitialStructure, pointerResolveInitialType, pointerResolveNElements,
										pointerResolveConstant, sj.isConstant, initialOffset, sj.vType))
										RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
									rtp->valueRef = reinterpret_cast<UInt8*>(pointerResolveInitialSite) + initialOffset;

									if(sj.isVarying)
										reinterpret_cast<TypedRuntimePointer*>(rtp)->type = sj.vType;
								}
								else
								{
									// Value is not a pointer
									if(objm->TypeIsObjectReference xxxx FIX INTERFACE REFS!!(storedJournal->vType))
									{
										// Make sure this is an object reference we're looking at
										if(!objm->TypeIsObjectReference(sj.vType))
											RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

										void **pObjRef = reinterpret_cast<void**>(journalValue);
										if(*pObjRef)
										{
											void *resolved = objm->ResolveShellableReference(*pObjRef);
											if(!resolved)
												RDX_STHROW(ctx, RDX_ERROR_UNRESOLVED_SYMBOL_REFERENCE);
											if(sj.vType == NULL)
												RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);	// Non-NULL assigned to a NULL value
											if(!objm->ObjectCompatible(resolved, sj.vType))
											{
												const GCInfo *resolvedInfo = GCInfo::From(GCInfo::From(resolved)->containerType);
												const GCInfo *sjInfo = GCInfo::From(sj.vType);
												RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
											}
											*pObjRef = resolved;
										}
										else
										{
											// TODO: Double-check that this is correct
											// Make sure that this isn't NULL past the point that this can permissibly be NULL
											if(sj.notNullInstruction < sf->currentILInstruction)
												RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
										}
									}
									else
									{
										// This isn't polymorphic so it has to match exactly
										if(storedJournal->vType != sj.vType)
											RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
										if(GCInfo::From(sj.vType)->containerType == objm->GetBuiltIns()->st_StructuredType)
										{
											const StructuredType *st = static_cast<const StructuredType *>(sj.vType);
											if(st->_native.numContainedReferences)
											{
												rdxLargeInt nRefs = st->_native.numContainedReferences;
												for(rdxLargeInt refi=0;refi<nRefs;refi++)
												{
													const StructuredType::NativeProperties::ContainedReference *ref = st->_native.containedReferences + refi;
													void **refLoc = reinterpret_cast<void**>(reinterpret_cast<UInt8*>(journalValue) + ref->offset);

													void *resolved = *refLoc;
													if(resolved)
													{
														resolved = objm->ResolveShellableReference(*refLoc);
														if(!resolved)
															RDX_STHROW(ctx, RDX_ERROR_UNRESOLVED_SYMBOL_REFERENCE);
														if(!objm->ObjectCompatible(resolved, ref->requiredType))
															RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
														*refLoc = resolved;
													}	// Resolved
												}	// i<nRefs
											}	// numContainedReferences
										}	// Type == structured
									}	// Not object reference
								}	// Not pointer
							}	// i<numFrameJournals
						}	// method->journal

						lastFrame = rtframe;
						lastIP = ip;
						lastPRV = reinterpret_cast<rdxURuntimeStackValue*>(t->stackBytes + prvOffset);
						lastMethod = sf->method;
					}

					// Make sure we're fully synced
					if(journalIndex != numStoredJournals)
						RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);


					t->frame.bp = lastFrame;
					t->frame.ip = lastIP;
					t->frame.prv = lastPRV;
					t->frame.method = lastMethod;

					t->deserializationState.storedFrames = NULL;
					t->deserializationState.storedJournals = NULL;
					t->deserializationState.stackOccupationBits = NULL;
					t->deserializationState.deserializing = false;
				}
				return true;
			}
			RDX_CATCH(ctx)
			{
				RDX_RETHROWV(ctx, false);
			}
			RDX_ENDTRY
		}


		void ThreadProcessor::MarkDependencies(rdxIObjectManager *objm, void *obj, const StructuredType *objST, bool markNative, ISerializer *ser, ScanID scanID, GCLink gcl) const
		{
			RuntimeThread *t = static_cast<RuntimeThread*>(obj);
			objm->SerializeIncludeObject(ser, t->ex, gcl);

			if(t->stackBytes)
			{
				if(t->deserializationState.deserializing)
				{
					// This isn't fully loaded and verified, use the preload storage to find values
					objm->SerializeIncludeObject(ser, t->deserializationState.stackOccupationBits, gcl);
					objm->SerializeIncludeObject(ser, t->deserializationState.storedFrames, gcl);
					objm->SerializeIncludeObject(ser, t->deserializationState.storedJournals, gcl);

					rdxLargeInt numFrames = GCInfo::From(t->deserializationState.storedFrames)->numElements;
					rdxLargeInt numJournals = GCInfo::From(t->deserializationState.storedJournals)->numElements;
					for(rdxLargeInt i=0;i<numFrames;i++)
					{
						const RuntimeThread::DeserializationState::StoredFrame *sf = t->deserializationState.storedFrames + i;
						objm->SerializeIncludeObject(ser, sf->method, gcl);
					}
					for(rdxLargeInt i=0;i<numJournals;i++)
					{
						const RuntimeThread::DeserializationState::StoredJournal *sj = t->deserializationState.storedJournals + i;
						objm->SerializeIncludeObject(ser, sj->vType, gcl);

						const GCInfo *checkedInfo = GCInfo::From(sj->vType);
						if(sj->vType)
						{
							if(sj->isPointer)
								objm->SerializeIncludeObject(ser, reinterpret_cast<const RuntimePointer<void> *>(t->stackBytes + sj->stackOffset)->objectRef, gcl);
							else if(sj->isVarying)
							{
								const TypedRuntimePointer *trp = reinterpret_cast<const TypedRuntimePointer*>(t->stackBytes + sj->stackOffset);
								objm->SerializeIncludeObject(ser, trp->rtp.objectRef, gcl);
								objm->SerializeIncludeObject(ser, trp->type, gcl);
							}
							else
							{
								if(objm->TypeIsObjectReference(sj->vType))
									objm->SerializeIncludeObject(ser, *reinterpret_cast<void**>(t->stackBytes + sj->stackOffset), gcl);
								else if(GCInfo::From(sj->vType)->containerType == objm->GetBuiltIns()->st_StructuredType)
								{
									const StructuredType *st = static_cast<const StructuredType*>(sj->vType);
									if(st->storageSpecifier == EnumValue::From(StructuredType::SS_RefStruct) || st->storageSpecifier == EnumValue::From(StructuredType::SS_ValStruct))
									{
										rdxLargeInt ncr = st->_native.numContainedReferences;
										if(ncr)
										{
											for(rdxLargeInt cri=0;cri<ncr;cri++)
											{
												rdxLargeInt offset = st->_native.containedReferences[cri].offset + sj->stackOffset;
												objm->SerializeIncludeObject(ser, *reinterpret_cast<void**>(t->stackBytes + offset), gcl);
											}
										}
									}
								}
							}
						}
					}

				}
				else
				{
					RuntimeTrace trace;
					if(t->Trace(trace))
					{
						RuntimeTrace nextTrace;
						bool hasParent = false;
						do
						{
							const GCInfo *methodInfo = GCInfo::From(trace.method);
							hasParent = trace.TraceParent(nextTrace);
							objm->SerializeIncludeObject(ser, trace.method, gcl);
							
							bool parentIsNative = hasParent && nextTrace.method->_native.isNativeCall;

							rdxLargeInt numJournal = trace.NumJournalEntries();
							rdxCJournalScanState scanState = trace.ScanJournals();

							for(rdxLargeInt i=0;i<numJournal;i++)
							{
								Journal j = scanState.NextJournal();
								if(j.isActive &&
									(!j.isParameter || !hasParent || parentIsNative))
								{
									objm->SerializeIncludeObject(ser, j.type, gcl);
									if(j.isPointer)
										objm->SerializeIncludeObject(ser, static_cast<const RuntimePointer<void> *>(j.value)->objectRef, gcl);
									else if(j.isVarying)
									{
										const TypedRuntimePointer *trp = static_cast<const TypedRuntimePointer *>(j.value);
										objm->SerializeIncludeObject(ser, trp->rtp.objectRef, gcl);
										objm->SerializeIncludeObject(ser, trp->type, gcl);
									}
									else if(objm->TypeIsObjectReference(j.type))
										objm->SerializeIncludeObject(ser, *static_cast<const void*const*>(j.value), gcl);
									else if(GCInfo::From(j.type)->containerType == objm->GetBuiltIns()->st_StructuredType)
									{
										const StructuredType *st = static_cast<const StructuredType*>(j.type);
										objm->SerializeIncludeStructure(ser, st, j.value, gcl);
									}
								}
							}

							trace = nextTrace;
						} while(hasParent);
					}
					else
					{
						// Couldn't trace, but this thread might have invoked a native method directly, in which case we still need
						// to trace the parameters.  Extract PRV from the precall frame.
						if(t->activeNativeMethod)
						{
							const Method *method = t->activeNativeMethod;
							rdxLargeInt numJournals = method->_native.numJournals;
							const UInt8 *cjBytes = method->_native.compactedJournals;
							const UInt8 *prvBase = reinterpret_cast<const UInt8 *>(t->precallFrame.prv);
							for(rdxLargeInt i=0;i<numJournals;i++)
							{
								StackJournal searchSJ;
								cjBytes += searchSJ.Decompress(cjBytes);
								const void *valueLoc = prvBase + searchSJ.offset;
								if(searchSJ.isPointer)
									objm->SerializeIncludeObject(ser, static_cast<const RuntimePointer<void> *>(valueLoc)->objectRef, gcl);
								else if(searchSJ.isVarying)
								{
									const TypedRuntimePointer *trp = static_cast<const TypedRuntimePointer *>(valueLoc);
									objm->SerializeIncludeObject(ser, trp->rtp.objectRef, gcl);
									objm->SerializeIncludeObject(ser, trp->type, gcl);
								}
								else if(objm->TypeIsObjectReference(searchSJ.vType))
									objm->SerializeIncludeObject(ser, *static_cast<const void*const*>(valueLoc), gcl);
								else if(GCInfo::From(searchSJ.vType)->containerType == objm->GetBuiltIns()->st_StructuredType)
								{
									const StructuredType *st = static_cast<const StructuredType*>(searchSJ.vType);
									objm->SerializeIncludeStructure(ser, st, valueLoc, gcl);
								}
							}
						}	// if(t->activeMethod)
					}	// !if(t->Trace(trace))
				}	// !if(t->deserializationState.deserializing)
			}	// !if(t->stackBytes)
		}
		
		ThreadProcessor			ThreadProcessor::instance;

		// Thread serializer
		// Safety checks:
		// - The stored stack depth must match with the deserialized frames
		// - Frame parents must land on call* instructions and the call must have been capable of invoking the specified method
		// - Pointers to other values in the thread are always one of two cases:
		//   - Offset from a parameter in the current frame
		//   - Offset from a local value.  In these cases, the local must be in the same frame (may be a parameter) and must have a lifespan
		//     that fully includes the lifespan of the pointer
		//
		// Serialized format:
		// LI: Stack size
		// LI: Frame count
		// LI: Journal count
		// Methods
		// Journals
		void ThreadSerializer::DeserializeCommon(rdxSOperationContext *ctx, rdxIObjectManager *objm, void *instance,
			IO::IFileStream *fs, IPackageHost *host, Package *pkg, bool isText) const
		{
			const Type *t_LargeInt = objm->GetBuiltIns()->st_LargeInt;
			const Type *t_Method = objm->GetBuiltIns()->st_Method;
			const Type *t_Type = objm->GetBuiltIns()->st_Type;
			const Type *t_Bool = objm->GetBuiltIns()->st_Bool;
			const Type *t_Object = objm->GetBuiltIns()->st_Object;

			RuntimeThread *thread = static_cast<RuntimeThread*>(instance);

			RDX_TRY(ctx)
			{
#ifndef RDX_JOURNAL_ALL_VALUES
				RDX_STHROW(ctx, RDX::RDX_ERROR_DESERIALIZED_THREAD_WITHOUT_FULL_JOURNALING);
#else
				CRef<const String> str;
				RDX_PROTECT_ASSIGN(ctx, str, objm->CreateStringASCII(ctx, "Core.Exception") );
				CRef<StructuredType> st_Exception = objm->LookupSymbolSimple(ctx, str).Cast<StructuredType>();

				rdxLargeInt stackSize;
				rdxLargeInt frameCount;
				rdxLargeInt journalCount;
				RDX_PROTECT(ctx, DeserializeValue(ctx, objm, pkg, DOMAIN_Runtime, host, fs, isText, t_LargeInt, &stackSize, 0));
				RDX_PROTECT(ctx, DeserializeValue(ctx, objm, pkg, DOMAIN_Runtime, host, fs, isText, t_LargeInt, &frameCount, 0));
				RDX_PROTECT(ctx, DeserializeValue(ctx, objm, pkg, DOMAIN_Runtime, host, fs, isText, t_LargeInt, &journalCount, 0));
				RDX_PROTECT(ctx, DeserializeValue(ctx, objm, pkg, DOMAIN_Runtime, host, fs, isText, st_Exception, &thread->ex, 0));

				if(stackSize < 0 || stackSize >= RDX_LARGEINT_MAX || stackSize % RDX_MAX_ALIGNMENT != 0)
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
				if(frameCount < 0 || frameCount >= RDX_LARGEINT_MAX)
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
				if(journalCount < 0 || journalCount >= RDX_LARGEINT_MAX)
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

				RDX_PROTECT_ASSIGN(ctx, thread->stackBytes, objm->GetAllocator()->CAlloc<UInt8>(stackSize));
				if(!thread->stackBytes)
					RDX_STHROW(ctx, RDX_ERROR_ALLOCATION_FAILED);

				thread->deserializationState.deserializing = true;
				RDX_PROTECT_ASSIGN(ctx, thread->deserializationState.stackOccupationBits, objm->Create1DArray<UInt8>(ctx, stackSize / 8 + 1));
				RDX_PROTECT_ASSIGN(ctx, thread->deserializationState.storedFrames, objm->Create1DArray<RuntimeThread::DeserializationState::StoredFrame>(ctx, frameCount));
				RDX_PROTECT_ASSIGN(ctx, thread->deserializationState.storedJournals, objm->Create1DArray<RuntimeThread::DeserializationState::StoredJournal>(ctx, journalCount));


				thread->ex = NULL;	// TODO: Serialize exception state!
				thread->ownerObjectManager = objm;
				thread->stackCapacity = stackSize;
				Threading::AtomicWrite(&thread->timeout, 0);


				UInt8 *occupyMask = thread->deserializationState.stackOccupationBits;

				for(rdxLargeInt i=0;i<frameCount;i++)
				{
					RuntimeThread::DeserializationState::StoredFrame frame;
					RDX_PROTECT(ctx, DeserializeValue(ctx, objm, pkg, DOMAIN_Runtime, host, fs, isText, t_Method, &frame.method, 0));
					RDX_PROTECT(ctx, DeserializeValue(ctx, objm, pkg, DOMAIN_Runtime, host, fs, isText, t_LargeInt, &frame.currentILInstruction, 0));
					RDX_PROTECT(ctx, DeserializeValue(ctx, objm, pkg, DOMAIN_Runtime, host, fs, isText, t_LargeInt, &frame.bpOffset, 0));
					RDX_PROTECT(ctx, DeserializeValue(ctx, objm, pkg, DOMAIN_Runtime, host, fs, isText, t_LargeInt, &frame.prvOffset, 0));

					thread->deserializationState.storedFrames[i] = frame;
				}


				for(rdxLargeInt i=0;i<journalCount;i++)
				{
					RuntimeThread::DeserializationState::StoredJournal journal;
					void *typeRef;

					RDX_PROTECT(ctx, DeserializeValue(ctx, objm, pkg, DOMAIN_Runtime, host, fs, isText, t_Type, &typeRef, 0));
					RDX_PROTECT(ctx, DeserializeValue(ctx, objm, pkg, DOMAIN_Runtime, host, fs, isText, t_LargeInt, &journal.stackOffset, 0));
					RDX_PROTECT(ctx, DeserializeValue(ctx, objm, pkg, DOMAIN_Runtime, host, fs, isText, t_Bool, &journal.isPointer, 0));
					RDX_PROTECT(ctx, DeserializeValue(ctx, objm, pkg, DOMAIN_Runtime, host, fs, isText, t_Bool, &journal.isVarying, 0));

					if(typeRef)
					{
						typeRef = objm->ResolveShellableReference(typeRef);
						if(!typeRef)
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
						// This needs to be resolved right now, types are critical so it should be available
						if(!objm->TypesCompatible(GCInfo::From(typeRef)->containerType, t_Type))
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
					}
					journal.vType = reinterpret_cast<Type*>(typeRef);

					if(journal.isPointer || journal.isVarying)
					{
						if(!journal.vType)
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
						if(journal.stackOffset % ALIGN_RuntimePointer)
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

						if(!PlaceBytes(occupyMask, journal.stackOffset, sizeof(RuntimePointer<void>), stackSize))
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

						RDX_PROTECT(ctx, DeserializeValue(ctx, objm, pkg, DOMAIN_Runtime, host, fs, isText, t_Object, thread->stackBytes + journal.stackOffset + RDX_OFFSETOF(RuntimePointer<void>, objectRef), 0));
						RDX_PROTECT(ctx, DeserializeValue(ctx, objm, pkg, DOMAIN_Runtime, host, fs, isText, t_LargeInt, thread->stackBytes + journal.stackOffset + RDX_OFFSETOF(RuntimePointer<void>, valueRef), 0));
					}
					else
					{
						if(journal.vType)
						{
							rdxLargeInt size;
							rdxLargeInt align;
							RDX_PROTECT(ctx, objm->TypeValueSize(ctx, journal.vType, size, align));

							if(journal.stackOffset % align)
								RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
							if(!PlaceBytes(occupyMask, journal.stackOffset, size, stackSize))
								RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
							RDX_PROTECT(ctx, DeserializeValue(ctx, objm, pkg, DOMAIN_Runtime, host, fs, isText, journal.vType, thread->stackBytes + journal.stackOffset, 0));
						}
						else
						{
							void *nullRef;
							RDX_PROTECT(ctx, DeserializeValue(ctx, objm, pkg, DOMAIN_Runtime, host, fs, isText, t_Object, &nullRef, 0));

							if(nullRef)
								RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

							if(!PlaceBytes(occupyMask, journal.stackOffset, sizeof(void*), stackSize))
								RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
							memset(thread->stackBytes + journal.stackOffset, 0, sizeof(void*));
						}
					}

					thread->deserializationState.storedJournals[i] = journal;
				}
#endif
			}
			RDX_CATCH(ctx)
			{
				RDX_RETHROW(ctx);
			}
			RDX_ENDTRY
		}


		void ThreadSerializer::DeserializeTextInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, void *instance, IO::ITextDeserializer *td, IPackageHost *host, Package *pkg) const
		{
			DeserializeCommon(ctx, objm, instance, td, host, pkg, true);
		}

		void ThreadSerializer::DeserializeBinaryInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, void *instance, IO::IFileStream *reader, IPackageHost *host, Package *pkg) const
		{
			DeserializeCommon(ctx, objm, instance, reader, host, pkg, false);
		}

		void ThreadSerializer::SerializeBinaryInstance(rdxIObjectManager *objm, const void *obj, IO::IFileStream *fs, const SSIDTable *ssidTable) const
		{
			SerializeCommon(objm, obj, fs, false, ssidTable);
		}

		void ThreadSerializer::SerializeTextInstance(rdxIObjectManager *objm, const void *obj, IO::IFileStream *fs, const SSIDTable *ssidTable) const
		{
			SerializeCommon(objm, obj, fs, true, ssidTable);
		}

		void ThreadSerializer::SerializeCommon(rdxIObjectManager *objm, const void *obj, IO::IFileStream *fs, bool isText, const SSIDTable *ssidTable) const
		{
			const ITypeSerializer *lis = objm->GetBuiltIns()->st_LargeInt->_native.user.typeSerializer;
			//const ITypeSerializer *strs = objm->GetBuiltIns()->st_LargeInt->_native.user.typeSerializer;

			rdxLargeInt numFrames = 0;
			rdxLargeInt numJournals = 0;
			const RuntimeThread *thread = static_cast<const RuntimeThread *>(obj);

			if(isText)
				fs->WriteBytes("\n", 1);

			// Output the stack size
			WriteValue(objm, fs, isText, ssidTable, objm->GetBuiltIns()->st_LargeInt, &thread->stackCapacity);

			// Count frames and valid journals
			for(const rdxSRuntimeStackFrame *frame = &thread->frame;frame->method;frame = frame->bp)
			{
				numFrames++;
				bool parametersAllowed = (frame->bp->method == NULL);

				RuntimeTrace trace;
				frame->Trace(trace);
				rdxLargeInt nJournals = trace.NumJournalEntries();
				rdxCJournalScanState scanState = trace.ScanJournals();
				for(rdxLargeInt i=0;i<nJournals;i++)
				{
					Journal j = scanState.NextJournal();
					if(j.isActive)
					{
						if(j.isParameter && !parametersAllowed)
							continue;
						numJournals++;
					}
				}
			}

			// Write frame and journal counts
			WriteValue(objm, fs, isText, ssidTable, objm->GetBuiltIns()->st_LargeInt, &numFrames);
			WriteValue(objm, fs, isText, ssidTable, objm->GetBuiltIns()->st_LargeInt, &numJournals);
			WriteValue(objm, fs, isText, ssidTable, objm->GetBuiltIns()->st_Object, &thread->ex);

			// Emit each frame
			for(rdxLargeInt frameNum=0;frameNum<numFrames;frameNum++)
			{
				// Drill down to the target frame
				const rdxSRuntimeStackFrame *frame = &thread->frame;
				{
					rdxLargeInt steps = numFrames - 1 - frameNum;
					for(rdxLargeInt i=0;i<steps;i++)
						frame = frame->bp;
				}

				RuntimeTrace trace;
				frame->Trace(trace);

				RuntimeThread::DeserializationState::StoredFrame sf;
				sf.method = trace.method;
				sf.currentILInstruction = trace.currentILInstruction;
				sf.bpOffset = reinterpret_cast<const UInt8 *>(frame->bp) - thread->stackBytes;
				sf.prvOffset = reinterpret_cast<const UInt8 *>(frame->prv) - thread->stackBytes;
				
				if(isText)
					fs->WriteBytes("\n", 1);
				WriteValue(objm, fs, isText, ssidTable, objm->GetBuiltIns()->st_Method, &sf.method);
				WriteValue(objm, fs, isText, ssidTable, objm->GetBuiltIns()->st_LargeInt, &sf.currentILInstruction);
				WriteValue(objm, fs, isText, ssidTable, objm->GetBuiltIns()->st_LargeInt, &sf.bpOffset);
				WriteValue(objm, fs, isText, ssidTable, objm->GetBuiltIns()->st_LargeInt, &sf.prvOffset);
			}

			// Emit journal values
			for(rdxLargeInt frameNum=0;frameNum<numFrames;frameNum++)
			{
				// Drill down to the target frame
				const rdxSRuntimeStackFrame *frame = &thread->frame;
				{
					rdxLargeInt steps = numFrames - 1 - frameNum;
					for(rdxLargeInt i=0;i<steps;i++)
						frame = frame->bp;
				}

				RuntimeTrace trace;
				frame->Trace(trace);
				
				rdxLargeInt nJournals = trace.NumJournalEntries();
				rdxCJournalScanState scanState = trace.ScanJournals();
				for(rdxLargeInt i=0;i<nJournals;i++)
				{
					Journal j = scanState.NextJournal();
					if(j.isActive)
					{
						if(j.isParameter && frameNum != 0)
							continue;
						
						RuntimeThread::DeserializationState::StoredJournal sj;
						if(j.isPointer)
							sj.isPointer = TrueValue;
						else
							sj.isPointer = FalseValue;
						if(j.isVarying)
							sj.isVarying = TrueValue;
						else
							sj.isVarying = FalseValue;
						sj.stackOffset = reinterpret_cast<const UInt8 *>(j.value) - thread->stackBytes;
						sj.vType = j.type;

						if(isText)
							fs->WriteBytes("\n", 1);
						WriteValue(objm, fs, isText, ssidTable, objm->GetBuiltIns()->st_Type, &sj.vType);
						WriteValue(objm, fs, isText, ssidTable, objm->GetBuiltIns()->st_LargeInt, &sj.stackOffset);
						WriteValue(objm, fs, isText, ssidTable, objm->GetBuiltIns()->st_Bool, &sj.isPointer);
						WriteValue(objm, fs, isText, ssidTable, objm->GetBuiltIns()->st_Bool, &sj.isVarying);

						if(isText)
							fs->WriteBytes("\n", 1);

						if(j.isPointer || j.isVarying)
						{
							const RuntimePointer<void> *ptr = reinterpret_cast<const RuntimePointer<void> *>(j.value);
							rdxLargeInt offset;

							switch(j.pointerSourceType)
							{
							case PST_Object:
								offset = reinterpret_cast<const UInt8 *>(ptr->valueRef) - reinterpret_cast<const UInt8 *>(ptr->objectRef);
								break;
							case PST_PinnedLocal:
							case PST_PinnedParameter:
								offset = reinterpret_cast<const UInt8 *>(ptr->valueRef) - reinterpret_cast<const UInt8 *>(j.pointerSource);
								break;
							case PST_OffsetParameter:
								offset = reinterpret_cast<const UInt8 *>(ptr->valueRef) - reinterpret_cast<const UInt8 *>(reinterpret_cast<const RuntimePointer<void> *>(j.pointerSource)->valueRef);
								break;
							};

							WriteValue(objm, fs, isText, ssidTable, objm->GetBuiltIns()->st_Object, &ptr->objectRef);
							WriteValue(objm, fs, isText, ssidTable, objm->GetBuiltIns()->st_LargeInt, &offset);
						}
						else
							WriteValue(objm, fs, isText, ssidTable, sj.vType, j.value);
					}
				}
			}
		}


		ThreadSerializer		ThreadSerializer::instance;

		namespace RuntimeUtilities
		{

			
			void *ArrayIndex(void *o, const rdxURuntimeStackValue *rsv)
			{
				rdxLargeInt stride = GCInfo::From(o)->elementSize;
				rdxLargeInt numDimensions = GCInfo::From(o)->numDimensions;
				rdxLargeInt element = 0;
				const rdxLargeInt *dimensions = GCInfo::From(o)->dimensions;

				for(rdxLargeInt i=0;i<numDimensions;i++)
				{
					element *= dimensions[i];
					rdxLargeInt idx = rsv[numDimensions - 1 - i].asLargeInt;
					if(idx < 0 || idx >= dimensions[i])
						return NULL;
					element += idx;
				}
				return reinterpret_cast<UInt8*>(o) + element*stride;
			}

			// Enters a method, stores prevFrame attributes and will return to them when the method exits
			// currentFrame: Frame state to restore after the method exits
			// frameBase:    RDX_MAX_ALIGNMENT-aligned address that is below any values stored in the current frame
			// stackBase:    Minimum address that the stack can be pushed to
			bool EnterMethod(rdxIObjectManager *objm, const Method *m, rdxSRuntimeStackFrame *currentFrame, void *frameBase,
				void *stackBase, rdxURuntimeStackValue *prv, rdxSRuntimeStackFrame *newFrame, void *recordedInsertionPoint, bool throughNative, const Method *viaNativeMethod)
			{
				rdxSRuntimeStackFrame *frame;

				if(!ExpandFrame(stackBase, frameBase, m->_native.frameCapacity, &frame, recordedInsertionPoint != NULL, throughNative))
					return false;

				// Copy the previous frame attributes
				*frame = *currentFrame;
				if(recordedInsertionPoint)
					static_cast<RuntimeStackFrameRoot *>(frame)->insertionPoint = recordedInsertionPoint;

				newFrame->method = m;
				newFrame->ip = static_cast<const ILInstruction *>(m->_native.nativeInstructions);
				newFrame->prv = prv;
				newFrame->bp = frame;

				return true;
			}

		}
	}
}

#endif

void rdxCRuntimeThread::Reset()
{
	memset(&this->frame, 0, sizeof(this->frame));
	memset(&this->precallFrame, 0, sizeof(this->precallFrame));
	this->activeNativeMethod = rdxWeakRTRef(rdxCMethod)::Null();
	this->insertionPoint = this->stackBytes + this->stackCapacity;
}

void rdxCRuntimeThread::Precall(rdxSOperationContext *ctx, void **ppFramePointer, rdxLargeUInt sizeOfParameters, rdxLargeUInt sizeOfReturnValues)
{
	rdxLargeUInt combinedSpace = rdxPaddedSize(sizeOfParameters + sizeOfReturnValues, RDX_MAX_ALIGNMENT);
	rdxUInt8 *insertionBytes = static_cast<rdxUInt8 *>(this->insertionPoint);
	rdxLargeUInt capacity = static_cast<rdxLargeUInt>(insertionBytes - this->stackBytes);
	if(capacity < combinedSpace)
		RDX_LTHROW(ctx, RDX_ERROR_PRECALL_STACK_OVERFLOW);

	this->precallFrame.bp = reinterpret_cast<rdxSRuntimeStackFrame*>(insertionBytes - combinedSpace);
	this->precallFrame.prv = reinterpret_cast<rdxURuntimeStackValue*>(insertionBytes - sizeOfReturnValues);
	this->precallFrame.method = rdxWeakRTRef(rdxCMethod)::Null();
	this->precallFrame.ip = NULL;

	*ppFramePointer = insertionBytes - sizeOfReturnValues - sizeOfParameters;
}

void rdxCRuntimeThread::VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
{
	visitor->VisitReference(objm, this->ex);
	visitor->VisitReference(objm, this->activeNativeMethod);

	if(this->stackBytes)
	{
		if(this->deserializationState.deserializing)
		{
			// This isn't fully loaded and verified, use the preload storage to find values
			visitor->VisitReference(objm, this->deserializationState.storedFrames);
			visitor->VisitReference(objm, this->deserializationState.storedJournals);

			rdxLargeUInt numFrames = this->deserializationState.storedFrames->NumElements();
			rdxLargeUInt numJournals = this->deserializationState.storedJournals->NumElements();
			for(rdxLargeUInt i=0;i<numFrames;i++)
			{
				rdxCRuntimeThread::DeserializationState::StoredFrame &sf = deserializationState.storedFrames->Element(i);
				visitor->VisitReference(objm, sf.method);
			}
			for(rdxLargeUInt i=0;i<numJournals;i++)
			{
				rdxCRuntimeThread::DeserializationState::StoredJournal &sj = this->deserializationState.storedJournals->Element(i);
				visitor->VisitReference(objm, sj.vType);

				if(sj.vType.IsNotNull())
				{
					void *stackAtOffset = this->stackBytes + sj.stackOffset;
					if(sj.isPointer)
						visitor->VisitReference(objm, *reinterpret_cast<rdxWeakTypelessOffsetRTRef*>(stackAtOffset));
					else if(sj.isVarying)
					{
						rdxSVarying *trp = reinterpret_cast<rdxSVarying*>(stackAtOffset);
						visitor->VisitReference(objm, trp->rtp);
						visitor->VisitReference(objm, trp->type);
					}
					else
					{
						if(objm->TypeIsObjectReference(sj.vType.ToWeakRTRef()))
							visitor->VisitReference(objm, *reinterpret_cast<rdxTracedRTRef(rdxCObject)*>(stackAtOffset));
						else if(objm->TypeIsInterface(sj.vType.ToWeakRTRef()))
							visitor->VisitReference(objm, *reinterpret_cast<rdxTracedTypelessIRef*>(stackAtOffset));
						else if(sj.vType->ObjectInfo()->containerType == objm->GetBuiltIns()->st_StructuredType)
						{
							rdxWeakRTRef(rdxCStructuredType) st = sj.vType.ToWeakRTRef().StaticCast<rdxCStructuredType>();
							if(st->storageSpecifier == rdxSS_RefStruct || st->storageSpecifier == rdxSS_ValStruct)
							{
								rdxLargeUInt ncr = st->m_native.numContainedReferences;
								if(ncr)
								{
									for(rdxLargeUInt cri=0;cri<ncr;cri++)
									{
										rdxLargeUInt offset = st->m_native.containedReferences->Element(cri).offset + sj.stackOffset;
										visitor->VisitReference(objm, *reinterpret_cast<rdxTracedRTRef(rdxCObject)*>(this->stackBytes + offset));
									}
								}
							}
						}
					}
				}
			}

		}
		else
		{
			rdxSRuntimeTrace trace;
			if(this->Trace(trace))
			{
				rdxSRuntimeTrace nextTrace;
				bool hasParent = false;
				do
				{
					hasParent = trace.TraceParent(nextTrace);
					visitor->VisitReference(objm, *trace.methodStackLoc);
							
					bool parentIsNative = hasParent && nextTrace.method->m_native.isNativeCall;

					rdxLargeUInt numJournal = trace.NumJournalEntries();
					rdxCJournalScanState scanState = trace.ScanJournals();

					for(rdxLargeUInt i=0;i<numJournal;i++)
					{
						rdxSJournal j = scanState.NextJournal();
						if(j.isActive &&
							(!j.isParameter || !hasParent || parentIsNative))
						{
							// TODO MUSTFIX: Need to properly relocate journal types
							visitor->VisitReference(objm, j.type);
							if(j.isPointer)
								visitor->VisitReference(objm, *static_cast<rdxWeakTypelessOffsetRTRef*>(j.value));
							else if(j.isVarying)
							{
								rdxSVarying *trp = static_cast<rdxSVarying *>(j.value);
								visitor->VisitReference(objm, trp->rtp);
								visitor->VisitReference(objm, trp->type);
							}
							else if(objm->TypeIsObjectReference(j.type))
								visitor->VisitReference(objm, *static_cast<rdxTracedRTRef(rdxCObject)*>(j.value));
							else if(objm->TypeIsInterface(j.type))
								visitor->VisitReference(objm, *static_cast<rdxTracedTypelessIRef*>(j.value));
							else if(j.type->ObjectInfo()->containerType == objm->GetBuiltIns()->st_StructuredType)
							{
								rdxWeakRTRef(rdxCStructuredType) st = j.type.StaticCast<rdxCStructuredType>();
								objm->VisitStructureReferences(j.value, visitor, visitNonSerializable, st->m_native.nativeTypeInfo, st, 0, 1);
							}
						}
					}

					trace = nextTrace;
				} while(hasParent);
			}
			else
			{
				// Couldn't trace, but this thread might have invoked a native method directly, in which case we still need
				// to trace the parameters.  Extract PRV from the precall frame.
				if(this->activeNativeMethod.IsNotNull())
				{
					rdxWeakRTRef(rdxCMethod) method = this->activeNativeMethod.ToWeakRTRef();
					rdxLargeUInt numJournals = method->m_native.numJournals;
					rdxWeakOffsetHdl(rdxUInt8) cjBytes = method->m_native.compactedJournals->OffsetElementRTRef(0).ToHdl();
					rdxUInt8 *prvBase = reinterpret_cast<rdxUInt8 *>(this->precallFrame.prv);
					for(rdxLargeUInt i=0;i<numJournals;i++)
					{
						rdxSStackJournal searchSJ;
						rdxLargeUInt nDecompressed;
						searchSJ.Decompress(&cjBytes, &nDecompressed);
						cjBytes += nDecompressed;
						void *valueLoc = prvBase + searchSJ.offset;
						if(searchSJ.isPointer)
							visitor->VisitReference(objm, *static_cast<rdxWeakTypelessOffsetRTRef*>(valueLoc));
						else if(searchSJ.isVarying)
						{
							rdxSVarying *trp = static_cast<rdxSVarying*>(valueLoc);
							visitor->VisitReference(objm, trp->rtp);
							visitor->VisitReference(objm, trp->type);
						}
						else if(objm->TypeIsObjectReference(searchSJ.vType))
							visitor->VisitReference(objm, *static_cast<rdxTracedRTRef(rdxCObject)*>(valueLoc));
						else if(objm->TypeIsInterface(searchSJ.vType))
							visitor->VisitReference(objm, *static_cast<rdxTracedTypelessIRef*>(valueLoc));
						else if(searchSJ.vType->ObjectInfo()->containerType == objm->GetBuiltIns()->st_StructuredType)
						{
							rdxWeakRTRef(rdxCStructuredType) st = searchSJ.vType.StaticCast<rdxCStructuredType>();
							objm->VisitStructureReferences(valueLoc, visitor, visitNonSerializable, st->m_native.nativeTypeInfo, st, 0, 1);
						}
					}
				}	// if(t->activeMethod)
			}	// !if(t->Trace(trace))
		}	// !if(t->deserializationState.deserializing)
	}	// !if(t->stackBytes)}
}

bool rdxCRuntimeThread::Trace(rdxSRuntimeTrace &outTrace) const
{
	return frame.Trace(outTrace);
}

///////////////////////////////////////////////////////////////////////////////
rdxCJournalScanState::rdxCJournalScanState(rdxWeakArrayRTRef(rdxUInt8) data, const void *bp, void *prv, rdxLargeUInt currentILInstruction)
{
	m_data = data->OffsetElementRTRef(0).ToHdl();
	m_prv = prv;
	m_bp = bp;
	m_currentILInstruction = currentILInstruction;
}

rdxSJournal rdxCJournalScanState::NextJournal()
{
	rdxSJournal j;
	rdxSStackJournal sj;
			
	rdxWeakOffsetHdl(rdxUInt8) cjBytes = m_data;
	rdxLargeUInt nDecompressed;
	sj.Decompress(&cjBytes, &nDecompressed);
	m_data = cjBytes + nDecompressed;

	j.isActive = sj.isParameter || (
			(m_currentILInstruction > sj.startInstruction &&
			m_currentILInstruction <= sj.endInstruction)
		);
	j.isParameter = sj.isParameter;
	j.isPointer = sj.isPointer;
	j.isVarying = sj.isVarying;
	j.type = sj.vType;
	j.name = sj.name;

	if(j.isParameter)
		j.value = reinterpret_cast<rdxUInt8*>(m_prv) + sj.offset;
	else
		j.value = const_cast<rdxUInt8 *>(reinterpret_cast<const rdxUInt8*>(m_bp)) + sj.offset;
	j.pointerSourceType = sj.pointerSourceType;

	switch(j.pointerSourceType)
	{
	case rdxPST_Object:
		j.pointerSource = NULL;
		break;
	case rdxPST_PinnedLocal:
		j.pointerSource = reinterpret_cast<const rdxUInt8*>(m_bp) + sj.pointerSource;
		break;
	case rdxPST_PinnedParameter:
	case rdxPST_OffsetParameter:
		j.pointerSource = reinterpret_cast<const rdxUInt8*>(m_prv) + sj.pointerSource;
		break;
	};

	return j;
}

///////////////////////////////////////////////////////////////////////////////
void rdxSRuntimeTrace::GetFileNameAndLineNumber(rdxWeakHdl(rdxCString) &outFilename, rdxLargeUInt &outLineNumber) const
{
	rdxLargeUInt ili = this->currentILInstruction;
	outFilename = rdxWeakHdl(rdxCString)::Null();
	outLineNumber = 0;

	if(this->method.IsNull())
		return;		// No info

	rdxWeakArrayRTRef(rdxSILDebugInfo) debugInfo = this->method->m_native.debugInfo.ToWeakRTRef();
	rdxLargeUInt numDebugInfos = 0;
	if(debugInfo.IsNotNull())
		numDebugInfos = debugInfo->NumElements();
	for(rdxLargeUInt i=0;i<numDebugInfos;i++)
	{
		if(debugInfo->Element(i).firstInstruction <= ili)
		{
			outFilename = debugInfo->Element(i).filename;
			outLineNumber = debugInfo->Element(i).line;
		}
	}
}

rdxSJournal rdxSRuntimeTrace::JournalEntry(rdxLargeUInt idx) const
{
	rdxCJournalScanState scanState(this->method->m_native.compactedJournals.ToWeakRTRef(), this->bp, this->prv, this->currentILInstruction);
	rdxSJournal j = scanState.NextJournal();
	for(rdxLargeUInt i=1;i<idx;i++)
		j = scanState.NextJournal();
	return j;
}

rdxCJournalScanState rdxSRuntimeTrace::ScanJournals() const
{
	return rdxCJournalScanState(this->method->m_native.compactedJournals.ToWeakRTRef(), this->bp, this->prv, this->currentILInstruction);
}

rdxLargeUInt rdxSRuntimeTrace::NumJournalEntries() const
{
	if(this->method.IsNull())
		return 0;
	return this->method->m_native.numJournals;
}

bool rdxSRuntimeTrace::TraceParent(rdxSRuntimeTrace &outTrace) const
{
	return bp->Trace(outTrace);
}

///////////////////////////////////////////////////////////////////////////////
bool rdxSRuntimeStackFrame::Trace(rdxSRuntimeTrace &outTrace) const
{
	if(this->method.IsNull())
	{
		const rdxRuntimeStackFrameReentrant *expandedFrame = static_cast<const rdxRuntimeStackFrameReentrant *>(this);
		if(!expandedFrame->aboveNative)
			return false;

		outTrace.bp = &expandedFrame->subNativeFrame;
		outTrace.method = expandedFrame->nativeMethod;
		outTrace.methodStackLoc = const_cast<rdxTracedRTRef(rdxCMethod)*>(&expandedFrame->nativeMethod);
		outTrace.prv = expandedFrame->prv;
		outTrace.currentILInstruction = 0;
		outTrace.ip = NULL;
		return true;
	}

	outTrace.bp = this->bp;
	outTrace.prv = this->prv;
	outTrace.method = this->method;
	outTrace.methodStackLoc = const_cast<rdxTracedRTRef(rdxCMethod)*>(&this->method);
	outTrace.ip = this->ip;
	outTrace.currentILInstruction = this->method->m_native.ipToCurrentInstruction(this->method.ToWeakHdl(), this->ip);
	return true;
}