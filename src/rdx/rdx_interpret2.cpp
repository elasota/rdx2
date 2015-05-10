#include "rdx_interpret.hpp"
#include "rdx_pccm_api.hpp"
#include "rdx_pccm_env.hpp"
#include "rdx_ilcomp.hpp"

enum rdxEInterpretMoveFlag
{
	rdxEIMF_DestPRV				= 1,	// Destination stack loc is PRV-relative
	rdxEIMF_DestDeref			= 2,	// Destination is a RTP
	rdxEIMF_SourcePRV			= 4,	// Source stack loc is PRV-relative
	rdxEIMF_SourceDeref			= 8,	// Source stack loc is PRV-relative
	rdxEIMF_SourceTypeDefault	= 16,	// Source is inside of a boxed type default
};

namespace rdxInterpretOps
{
	enum EOpCode
	{
		EOP_Move,
		EOP_MoveAndMark,		// Move a structure and mark at destination
		EOP_MoveMarkVisit,		// Move a structure, mark, and run visit at destination
		EOP_MoveInterfaceRef,
		EOP_MoveObjectRef,
		EOP_MoveRTP,
		EOP_PinLocal,
	};

	struct InterpretOp
	{
		rdxUInt8 opcode;
	};

	struct PinLocal : public InterpretOp
	{
		rdxLargeInt dest, loc;
	};

	struct Move : public InterpretOp
	{
		rdxUInt8 moveFlags;
		rdxLargeInt src, dest;
		rdxLargeUInt size;
	};

	struct MoveAndMark : public InterpretOp
	{
		rdxUInt8 moveFlags;
		rdxLargeInt src, dest;
		rdxLargeUInt size;
		rdxWeakArrayHdl(rdxCStructuredType::NativeProperties::ContainedReference) markList;
	};

	struct MoveMarkVisit : public InterpretOp
	{
		rdxUInt8 moveFlags;
		rdxLargeInt src, dest;
		rdxLargeUInt size;
		rdxWeakArrayHdl(rdxCStructuredType::NativeProperties::ContainedReference) markList;
		rdxIfcTypeFuncs::VisitReferencesFunc vrf;
	};

	struct MoveInterfaceReference : public InterpretOp
	{
		rdxUInt8 moveFlags;
		rdxLargeInt src, dest;
	};

	struct MoveObjectReference : public InterpretOp
	{
		rdxUInt8 moveFlags;
		rdxLargeInt src, dest;
	};

	struct MoveRTP : public InterpretOp
	{
		rdxUInt8 moveFlags;
		rdxLargeInt src, dest;
	};
};

//================================================================================
int RDX_DECL_API rdxResumeThread(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakRTRef(rdxCRuntimeThread) thread)
{
	int status = rdxRS_Active;

	// Cycle through code until the status turns into an exit code
	while(status > 0)
	{
		rdxWeakRTRef(rdxCMethod) method;	// FIXME: Bad constructor
		method = thread->frame.method;

		if(method.IsNull())
			return rdxRS_Exited;

		rdxPCCMCallback pccmCallback = method->m_native.pccmCallback;
		if(pccmCallback)
			status = rdxCPrecompiledCodeModule::ResumeThread(ctx, objm, thread, pccmCallback);
		else
		{
			rdxDebugBreak(rdxBREAKCAUSE_Unimplemented);	status = 0;
			//status = rdxResumeThreadInterpreted(ctx, objm, thread);
		}

		if(status == rdxRS_AbandonFrame)
		{
			// Code signalled to abandon the frame, determine the type of frame being entered
			rdxSRuntimeStackFrame *rsf = thread->frame.bp;
			if(rsf->method.IsNotNull())
			{
				// Standard frame
				thread->frame = *rsf;
				status = rdxRS_Active;
			}
			else
			{
				// Root-level call frame
				rdxRuntimeStackFrameRoot *rframe = static_cast<rdxRuntimeStackFrameReentrant *>(rsf);
				thread->insertionPoint = rframe->insertionPoint;
				if(rframe->aboveNative)
				{
					// Reentrant frame
					rdxRuntimeStackFrameReentrant *ref = static_cast<rdxRuntimeStackFrameReentrant *>(rframe);
					thread->activeNativeMethod = ref->nativeMethod;
					thread->frame = ref->subNativeFrame;
					return rdxRS_Exited;
				}
				else
				{
					// Absolute root frame
					thread->activeNativeMethod = rdxWeakRTRef(rdxCMethod)::Null();
					thread->Reset();
					return rdxRS_Exited;
				}
			}
		}
	}

	return status;
}

//====================================================================================
rdxCInterpreterCodeProvider::rdxCInterpreterCodeProvider(const rdxSAllocator &alloc)
{
	m_alloc = alloc;
}

void rdxCInterpreterCodeProvider::CreateExecutable(rdxIObjectManager *objm, rdxSOperationContext *ctx, rdxWeakHdl(rdxCMethod) m) const
{
	if(const rdxSSerializationTag *serTag = m->ObjectInfo()->SerializationTag())
	{
		rdxSObjectGUID objectGUID = serTag->gstSymbol;
		rdxUInt64 codedDomain = 0;
		rdxUInt64 codedObject = 0;
		for(rdxLargeUInt i=0;i<rdxSDomainGUID::GUID_SIZE;i++)
			codedDomain = ((codedDomain << 8) | (objectGUID.m_domain.m_bytes[i]));
		for(rdxLargeUInt i=0;i<rdxSObjectGUID::GUID_SIZE;i++)
			codedObject = ((codedObject << 8) | (objectGUID.m_bytes[i]));

		const rdxINativeTypeHost *nth = objm->GetTypeHost();
		rdxLargeUInt nPlugins = nth->NumPlugins();
		for(rdxLargeUInt i=0;i<nPlugins;i++)
		{
			const rdxSPCCMDomainIndex *pccm;
			const rdxSPluginExport *plugin;
			nth->GetPlugin(i, &pccm, &plugin);
			if(pccm != RDX_CNULL)
			{
				while(pccm->objects != RDX_CNULL)
				{
					if(pccm->codedDomainGUID == codedDomain)
					{
						rdxLargeUInt nObjects = pccm->objCount;
						// TODO: Symbols are probably evenly distributed, so a simple initial guess will probably be good
						for(rdxLargeUInt obji=0;obji<nObjects;obji++)
						{
							if(pccm->objects[obji].codedObjectGUID == codedObject)
							{
								m->m_native.pccmCallback = pccm->objects[obji].callback;
								return;
							}
						}
					}
					pccm++;
				}
			}
		}
	}
	// TODO: This must be fatal until interpreter is ready
	RDX_TRY(ctx)
	{
		RDX_PROTECT(ctx, CreateInterpreterCode(m));
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
	rdxDebugBreak(rdxBREAKCAUSE_Unimplemented);
}

void rdxCInterpreterCodeProvider::ReleaseExecutable(rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m) const
{
	rdxDebugBreak(rdxBREAKCAUSE_Unimplemented);
}

void rdxCInterpreterCodeProvider::InitializeSymbolDictionary(rdxSOperationContext *ctx, rdxIObjectManager *objm) const
{
}

int rdxCInterpreterCodeProvider::RunMethod(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, rdxURuntimeStackValue *prv) const
{
	rdxWeakRTRef(rdxCMethod) viaMethod;	// FIXME: Bad constructor
	viaMethod = thread->activeNativeMethod;
	int status;

	if(m->m_native.isNativeCall)
	{
		thread->activeNativeMethod = m.ToWeakRTRef();
		status = m->m_native.nativeCall(ctx, objm, m, thread, prv);
	}
	else
	{
		const void *stackBottom = thread->stackBytes + thread->stackCapacity;
		bool methodEntered;
		{
			methodEntered = rdxEnterMethodRoot(objm, m.ToWeakRTRef(), &thread->precallFrame, thread->precallFrame.bp, thread->stackBytes, prv, &thread->frame, thread->insertionPoint, (thread->insertionPoint != stackBottom), viaMethod.ToWeakRTRef(), &thread->frame);
		}
		if(!methodEntered)
		{
			rdxDebugBreak(rdxBREAKCAUSE_Unimplemented);
			/*
			rdxRef<rdxCException> stackOverflowException = objm->GetBuiltIns()->providerDictionary.ToHandle()[rdxX_StackOverflowException].StaticCast<rdxCException>();

			{
				rdxCDataView<rdxHandle<rdxCRuntimeThread> > threadView(thread);
				threadView->ex = stackOverflowException.ToHandle();
			}
			*/
			return rdxRS_Exception;
		}
		
		thread->activeNativeMethod = rdxWeakRTRef(rdxCMethod)::Null();
		status = rdxResumeThread(ctx, objm, thread);
	}
	return status;
}

static rdxLargeUInt RDX_DECL_API StaticInterpretIPToCurrentInstruction(rdxWeakHdl(rdxCMethod) method, const void *ip)
{
	return static_cast<rdxLargeUInt>(static_cast<const rdxUInt8*>(ip) - static_cast<const rdxUInt8*>(RDX_CNULL) - 1);
}

const void *RDX_DECL_API StaticInstrNumToIP(rdxSOperationContext *ctx, rdxWeakHdl(rdxCMethod) method, rdxLargeUInt instrNum, bool *resumeAllowed)
{
	const rdxUInt8 *ip = static_cast<const rdxUInt8*>(RDX_CNULL) + instrNum;
	if(resumeAllowed)
	{
		bool flag = ((method->m_native.ilResumeFlags->Element(instrNum/8) & (1 << (instrNum & 0x7))) != 0);
		*resumeAllowed = flag;
	}
	return ip;
}

rdxIPToCurrentInstructionCallback rdxCInterpreterCodeProvider::GetIPToCurrentInstructionCallback() const
{
	return StaticInterpretIPToCurrentInstruction;
}

rdxInstrNumToIPCallback rdxCInterpreterCodeProvider::GetInstrNumToIPCallback() const
{
	return StaticInstrNumToIP;
}

rdxResumeThreadCallback rdxCInterpreterCodeProvider::GetResumeThreadCallback(rdxIObjectManager *objm) const
{
	return RDX_CNULL;
}

void rdxCInterpreterCodeProvider::Shutdown()
{
	m_alloc.Free(this);
}

class rdxCInterpreterCompiler
{
public:
	rdxCInterpreterCompiler(const rdxSILInstruction *ili, rdxLargeUInt numILInstructions, const rdxSMarkupInstruction *mop, rdxLargeUInt numMOP, const rdxUILOpCompactArg *ca, const rdxUILOpLargeArg *la);

	void ExecutePass(rdxSOperationContext *ctx);
	void DischargeMOP(rdxSOperationContext *ctx, rdxLargeUInt instrNumPlusOne);
	void DischargeIncrProps(rdxSOperationContext *ctx, rdxLargeUInt instrNumPlusOne, rdxLargeInt &offset);

private:
	const rdxSMarkupInstruction *m_currentMOP;

	const rdxSILInstruction *m_baseILI;
	const rdxSMarkupInstruction *m_baseMOP;
	const rdxSMILStackAction *m_baseStackActions;
	const rdxUILOpCompactArg *m_compactArgs;
	const rdxUILOpLargeArg *m_largeArgs;
	rdxLargeInt *m_stackActionLocations;
	rdxLargeUInt m_numILI;
	rdxLargeUInt m_numMOP;
};

void rdxCInterpreterCodeProvider::CompileInterpreterCode(rdxSOperationContext *ctx, rdxWeakHdl(rdxCMethod) m)
{
	RDX_TRY(ctx)
	{
		const rdxSMILStackAction *stackActions = RDX_CNULL;
		if(m->m_native.milStackActions.IsNotNull())
			stackActions = m->m_native.milStackActions->ArrayData();
		const rdxSILInstruction *ili = m->m_native.ilinstructions->ArrayData();
		const rdxSMarkupInstruction *mop = RDX_CNULL;
		rdxLargeUInt numMOP = 0;
		rdxLargeUInt numILInstructions = m->m_native.numILInstructions;
		if(m->m_native.markupInstructions.IsNotNull())
		{
			mop = m->m_native.markupInstructions->ArrayData();
			numMOP = m->m_native.markupInstructions->NumElements();
		}

		const rdxUILOpCompactArg *compactArgs = RDX_CNULL;
		const rdxUILOpLargeArg *largeArgs = RDX_CNULL;

		if(m->m_native.compactArgs.IsNotNull())
			compactArgs = m->m_native.compactArgs->ArrayData();
		if(m->m_native.largeArgs.IsNotNull())
			largeArgs = m->m_native.largeArgs->ArrayData();

		rdxCInterpreterCompiler compiler(ili, numILInstructions, mop, numMOP, compactArgs, largeArgs);

		RDX_PROTECT(ctx, compiler.ExecutePass(ctx));
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

rdxCInterpreterCompiler::rdxCInterpreterCompiler(const rdxSILInstruction *ili, rdxLargeUInt numILInstructions, const rdxSMarkupInstruction *mop, rdxLargeUInt numMOP, const rdxUILOpCompactArg *ca, const rdxUILOpLargeArg *la)
	: m_baseILI(ili)
	, m_numILI(numILInstructions)
	, m_baseMOP(mop)
	, m_numMOP(numMOP)
	, m_currentMOP(RDX_CNULL)
	, m_compactArgs(ca)
	, m_largeArgs(la)
	, m_stackActionLocations(RDX_CNULL)
{
}

void rdxCInterpreterCompiler::ExecutePass(rdxSOperationContext *ctx)
{
	RDX_TRY(ctx)
	{
		RDX_PROTECT(ctx, DischargeMOP(ctx, 0));

		for(rdxLargeUInt iNum=0;iNum<m_numILI;iNum++)
		{
			const rdxSILInstruction *ili = this->m_baseILI + iNum;
			const rdxUILOpCompactArg *ca = this->m_compactArgs + ili->argOffs;
			const rdxUILOpLargeArg *la = this->m_largeArgs + ili->argOffs;

			switch(ili->opcode)
			{
			case rdxILOP_debuginfo:
				break;
			case rdxILOP_move:
				{
					rdxLargeUInt flags = ca[0].lui;
					rdxLargeUInt sourceSA = ca[1].lui;
					rdxLargeUInt destSA = ca[2].lui;
					rdxWeakHdl(rdxCObject) copyType = rdxWeakHdl(rdxCObject)(rdxObjRef_CSignal_BaseRef, rdxBaseHdl(rdxBaseRef_ConSignal_POD, ca[3].p));

					rdxLargeUInt copySize;
					RDX_PROTECT(ctx, DetermineMoveSize(ctx, copyType.StaticCast<rdxCType>()));

					rdxInterpretOps::EOpCode opcode;

					rdxUInt8 moveFlags;
					if(flags & RDX_ILOP_MOVE_DEST_PARENT_FRAME)
						moveFlags |= rdxEIMF_DestPRV;
					if(flags & RDX_ILOP_MOVE_DEST_DEREF)
						moveFlags |= rdxEIMF_DestDeref;
					if(flags & RDX_ILOP_MOVE_SRC_PARENT_FRAME)
						moveFlags |= rdxEIMF_SourcePRV;
					if(flags & RDX_ILOP_MOVE_SRC_DEREF)
						moveFlags |= rdxEIMF_SourceDeref;
					if(flags & RDX_ILOP_MOVE_SRC_TYPE_DEFAULT)
						moveFlags |= rdxEIMF_SourceTypeDefault;

					rdxLargeInt src, dest;

					if(flags & RDX_ILOP_MOVE_SRC_TYPE_DEFAULT)
						src = reinterpret_cast<rdxUInt8*>(ca[1].p) - reinterpret_cast<rdxUInt8*>(RDX_CNULL);
					else
						src = m_stackActionLocations[sourceSA];

					dest = m_stackActionLocations[sourceSA];

					RDX_PROTECT(ctx, DischargeIncrProps(ctx, iNum+1, dest));

					if(flags & RDX_ILOP_MOVE_SRC_CONTAINS_REFS)
					{
						rdxIfcTypeFuncs::VisitReferencesFunc vrf = RDX_CNULL;
						rdxWeakRTRef(rdxCStructuredType) copyST = copyType.ToWeakRTRef().StaticCast<rdxCStructuredType>();

						if(copyST->m_native.nativeTypeInfo.IsNotNull())
						{
							rdxIfcTypeFuncs typeFuncs = copyST->m_native.nativeTypeInfo.TypeFuncs();
							if(typeFuncs.IsNotNull())
								vrf = typeFuncs.GetVisitReferencesFunc();
						}

						rdxWeakArrayHdl(rdxCStructuredType::NativeProperties::ContainedReference) crArray = copyST->m_native.containedReferences.ToWeakHdl();

						if(vrf == RDX_CNULL)
						{
							rdxInterpretOps::MoveAndMark instr;
							instr.opcode = rdxInterpretOps::EOP_MoveAndMark;
							instr.src = src;
							instr.dest = dest;
							instr.moveFlags = moveFlags;
							instr.size = copySize;
							instr.markList = crArray;
							EmitInstr(instr);
						}
						else
						{
							rdxInterpretOps::MoveMarkVisit instr;
							instr.opcode = rdxInterpretOps::EOP_MoveMarkVisit;
							instr.src = src;
							instr.dest = dest;
							instr.moveFlags = moveFlags;
							instr.size = copySize;
							instr.markList = crArray;
							instr.vrf = vrf;
							EmitInstr(instr);
						}
					}
					else if(flags & RDX_ILOP_MOVE_SRC_IS_CLASS_REF)
					{
						rdxInterpretOps::MoveObjectReference instr;
						instr.opcode = rdxInterpretOps::EOP_MoveObjectRef;
						instr.src = src;
						instr.dest = dest;
						instr.moveFlags = moveFlags;
						EmitInstr(instr);
					}
					else if(flags & RDX_ILOP_MOVE_SRC_IS_INTERFACE_REF)
					{
						rdxInterpretOps::MoveInterfaceReference instr;
						instr.opcode = rdxInterpretOps::EOP_MoveInterfaceRef;
						instr.src = src;
						instr.dest = dest;
						instr.moveFlags = moveFlags;
						EmitInstr(instr);
					}
					else if(flags & RDX_ILOP_MOVE_IS_RTP)
					{
						rdxInterpretOps::MoveRTP instr;
						instr.opcode = rdxInterpretOps::EOP_MoveRTP;
						instr.src = src;
						instr.dest = dest;
						instr.moveFlags = moveFlags;
						EmitInstr(instr);
					}
					else
					{
						rdxInterpretOps::Move instr;
						instr.opcode = rdxInterpretOps::EOP_MoveRTP;
						instr.src = src;
						instr.dest = dest;
						instr.moveFlags = moveFlags;
						instr.size = copySize;
						EmitInstr(instr);
					}
				}
				break;
			case rdxILOP_clone:
				{
					rdxLargeUInt srcSA = ca[0].lui;
					rdxLargeUInt destSA = ca[1].lui;
					rdxLargeUInt moveSize = StackActionMoveSize(srcSA);

					rdxInterpretOps::Move instr;
					instr.opcode = rdxInterpretOps::EOP_Move;
					instr.src = m_stackActionLocations[srcSA];
					instr.dest = m_stackActionLocations[destSA];
					instr.moveFlags = 0;
					instr.size = StackActionMoveSize(srcSA);
					EmitInstr(instr);
				}
				break;
			case rdxILOP_pinl:
				{
					rdxLargeUInt destSA = ca[0].lui;
					rdxLargeUInt locSA = ca[1].lui;

					rdxInterpretOps::PinLocal instr;
					instr.opcode = rdxInterpretOps::EOP_PinLocal;
					instr.dest = m_stackActionLocations[destSA];
					instr.src = m_stackActionLocations[srcSA];
					EmitInstr(instr);
				}
				break;
			case rdxILOP_pushdefault:	// TODO MUSTFIX: Implement
			case rdxILOP_tovarying:	// TODO MUSTFIX: Implement
			default:
				RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);	// Unhandled op
			};
		}
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

void rdxCInterpreterCompiler::DischargeMOP(rdxSOperationContext *ctx, rdxLargeUInt instrNumPlusOne)
{
	while(m_currentMOP != RDX_CNULL && m_currentMOP->instructionLink == instrNumPlusOne)
	{
		rdxEMarkupOpcode mopcode = m_currentMOP->opcode;

		switch(mopcode)
		{
		case rdxMOP_addshell:
		case rdxMOP_pop:
		case rdxMOP_incrprop:
		case rdxMOP_moveprop:
		case rdxMOP_unshell:
		case rdxMOP_movesa:
			break;
		default:
			RDX_LTHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);	// Unimplemented mop?
		};
		m_currentMOP++;
		if(m_currentMOP - m_baseMOP == m_numMOP)
			m_currentMOP = RDX_CNULL;
	}
}

void rdxCInterpreterCompiler::DischargeIncrProps(rdxSOperationContext *ctx, rdxLargeUInt instrNumPlusOne, rdxLargeInt &offset)
{
	while(m_currentMOP != RDX_CNULL && (m_currentMOP->opcode == rdxMOP_incrprop || m_currentMOP->opcode == rdxMOP_moveprop) && m_currentMOP->instructionLink == instrNumPlusOne)
	{
		const rdxUILOpCompactArg *ca = this->m_compactArgs + m_currentMOP->argOffs;
		rdxBaseHdl stBaseHdl(rdxBaseRef_ConSignal_POD, ca[1].p);
		rdxWeakHdl(rdxCStructuredType) st(rdxObjRef_CSignal_BaseRef, stBaseHdl);
		rdxLargeUInt propertyIndex = ca[0].lui;

		rdxLargeUInt propOffset = st->m_native.propertyOffsets->Element(propertyIndex);
		offset += static_cast<rdxLargeInt>(propOffset);

		m_currentMOP++;
		if(m_currentMOP - m_baseMOP == m_numMOP)
			m_currentMOP = RDX_CNULL;
	}
}

