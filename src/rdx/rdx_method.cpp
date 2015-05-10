#include "rdx_programmability.hpp"
#include "rdx_intrinsics.hpp"
#include "rdx_builtins.hpp"


//////////////////////////////////////////////////////////////////////
RDX_IMPLEMENT_COMPLEX_NATIVE_CLASS(rdxCMethod, (rdxETIF_VisitReferences | rdxETIF_OnLoad));

RDX_BEGIN_PROPERTY_LOOKUP_CLASS(rdxCMethod)
	RDX_DEFINE_LOOKUP_PROPERTY(parameters)
	RDX_DEFINE_LOOKUP_PROPERTY(returnTypes)
	RDX_DEFINE_LOOKUP_PROPERTY(bytecode)
	RDX_DEFINE_LOOKUP_PROPERTY(resArgs)
	RDX_DEFINE_LOOKUP_PROPERTY(instructionFileInfos)
	RDX_DEFINE_LOOKUP_PROPERTY(numInstructions)
	RDX_DEFINE_LOOKUP_PROPERTY(vftIndex)
	RDX_DEFINE_LOOKUP_PROPERTY(thisParameterOffset)
	RDX_DEFINE_LOOKUP_PROPERTY(isAbstract)
RDX_END_PROPERTY_LOOKUP

//////////////////////////////////////////////////////////////////////
RDX_IMPLEMENT_COMPLEX_NATIVE_STRUCT(rdxSMethodParameter, (rdxETIF_NoFlags));

RDX_BEGIN_PROPERTY_LOOKUP_STRUCT(rdxSMethodParameter)
	RDX_DEFINE_LOOKUP_PROPERTY(type)
	RDX_DEFINE_LOOKUP_PROPERTY(isConstant)
	RDX_DEFINE_LOOKUP_PROPERTY(isNotNull)
RDX_END_PROPERTY_LOOKUP

//////////////////////////////////////////////////////////////////////
RDX_IMPLEMENT_COMPLEX_NATIVE_STRUCT(rdxSInstructionFileInfo, (rdxETIF_NoFlags));

RDX_BEGIN_PROPERTY_LOOKUP_STRUCT(rdxSInstructionFileInfo)
	RDX_DEFINE_LOOKUP_PROPERTY(filename)
	RDX_DEFINE_LOOKUP_PROPERTY(line)
	RDX_DEFINE_LOOKUP_PROPERTY(firstInstruction)
RDX_END_PROPERTY_LOOKUP


rdxCMethod::rdxCMethod(rdxIObjectManager *objm, rdxGCInfo *gci)
	: rdxCObject(objm, gci)
{
}

rdxCMethod::~rdxCMethod()
{
	// TODO MUSTFIX
	//if(m_native.nativeInstructions)
	//	objm->GetCodeProvider()->ReleaseExecutable(objm, m);
}

void rdxCMethod::VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
{
	if(visitNonSerializable)
	{
		visitor->VisitReference(objm, m_native.ilinstructions);
		visitor->VisitReference(objm, m_native.ilResumeFlags);
		visitor->VisitReference(objm, m_native.compactArgs);
		visitor->VisitReference(objm, m_native.largeArgs);
		visitor->VisitReference(objm, m_native.callPoints);
		visitor->VisitReference(objm, m_native.debugInfo);
		visitor->VisitReference(objm, m_native.translation1);
		visitor->VisitReference(objm, m_native.translation2);
		visitor->VisitReference(objm, m_native.compactedJournals);
		visitor->VisitReference(objm, m_native.exHandlers);
		visitor->VisitReference(objm, m_native.milStackActions);
		visitor->VisitReference(objm, m_native.markupInstructions);
	}
}

bool rdxCMethod::OnLoad(rdxSOperationContext *ctx, rdxIObjectManager *objm)
{
	RDX_TRY(ctx)
	{
		if(this->parameters.IsNull())
			RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

		if(this->vftIndex)
		{
			// Make sure that this will call the expected method when invoked
			if(this->vftIndex < 0)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
			if(this->parameters.IsNull())
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

			if(this->thisParameterOffset <= 0 || this->thisParameterOffset > this->parameters->NumElements())
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

			// Determine "this" type
			rdxWeakHdl(rdxCType) thisType = this->parameters->Element(this->thisParameterOffset - 1).type.ToWeakHdl();
			if(thisType.IsNull())
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
			if(thisType->ObjectInfo()->containerType != objm->GetBuiltIns()->st_StructuredType)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

			// Verify that the type this will invoke will actually call this method
			rdxWeakHdl(rdxCStructuredType) thisST = thisType.StaticCast<rdxCStructuredType>();
			if(thisST->storageSpecifier != rdxSS_Interface &&
				thisST->storageSpecifier != rdxSS_Class)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

			if(thisST->virtualMethods.IsNull())
				RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_VTABLE);
			if(this->vftIndex <= 0 || this->vftIndex > thisST->virtualMethods->NumElements())
				RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_VTABLE);
			if(thisST->virtualMethods->Element(this->vftIndex - 1) != rdxWeakRTRef(rdxCMethod)::FromPtr(this))
				RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_VTABLE);
		}

		// Validate that the parameters are valid
		rdxLargeUInt numParameters = this->parameters->NumElements();
		for(rdxLargeUInt i=0;i<numParameters;i++)
		{
			rdxWeakOffsetHdl(rdxSMethodParameter) param = this->parameters->OffsetElementRTRef(i).ToHdl();
			// IsNotNull is only valid for reference types
			if(param->isNotNull && !objm->TypeIsObjectReference(param->type.ToWeakRTRef()) && !objm->TypeIsInterface(param->type.ToWeakRTRef()))
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
		}

		rdxNativeCallback ncb = this->m_native.nativeCall;
		if(!ncb)
		{
			rdxSSerializationTag *serTag = this->ObjectInfo()->SerializationTag();
			if(serTag && !serTag->isAnonymous && this->bytecode.IsNull())
			{
				const rdxNativeCallback *icb = NULL;
				
				icb = rdxLookupBuiltin(serTag->gstSymbol);

				if(icb)
					ncb = *icb;
				else if(!this->isAbstract)
				{
					const rdxINativeTypeHost *nativeTypeHost = objm->GetTypeHost();
					if(nativeTypeHost != RDX_CNULL)
						ncb = nativeTypeHost->HookMethod(serTag->gstSymbol);
				}

				if(ncb)
				{
					this->m_native.nativeCall = ncb;
					this->m_native.isNativeCall = true;
				}
				else if(!this->isAbstract)
					RDX_STHROW(ctx, RDX_ERROR_NATIVE_METHOD_UNBOUND);
			}
			else
			{
				if(this->bytecode.IsNotNull())
				{
					if(this->isAbstract)
						RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
				}
				else
					if(!this->isAbstract)
						RDX_STHROW(ctx, RDX_ERROR_NATIVE_METHOD_UNBOUND);
			}

			RDX_PROTECT(ctx, rdxCompileMethod(ctx, rdxWeakHdl(rdxCMethod)::FromPtr(this), objm) );
		}

		return true;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROWV(ctx, false);
	}
	RDX_ENDTRY
}

void rdxCMethod::DetermineIntrinsicState()
{
	rdxSSerializationTag *serTag = this->ObjectInfo()->SerializationTag();
	if(!serTag || serTag->isAnonymous)
		return;

	if(m_native.intrinsicStateChecked)
		return;

	m_native.intrinsicStateChecked = true;
	const rdxSNativeFunction *nf = rdxLookupIntrinsicFunction(serTag->gstSymbol);

	if(nf)
	{
		m_native.isIntrinsic = true;
		m_native.falseCheckOpcode = nf->falseCheckOpcode;
		m_native.isBranching = nf->isBranching;
		m_native.neverFails = nf->neverFails;
		m_native.opcode = nf->opcode;
		m_native.p1 = nf->p1;
		m_native.p2 = nf->p2;
	}
}

int rdxCMethod::ExplicitInvoke(rdxSOperationContext *ctx, rdxWeakHdl(rdxCRuntimeThread) t, rdxLargeUInt timeout)
{
	t->timeout.WriteUnfenced(timeout);
	const rdxICodeProvider *cp = ctx->objm->GetCodeProvider();
	// TODO: Don't use a hdl...
	return cp->RunMethod(ctx, t->ownerObjectManager, rdxWeakHdl(rdxCMethod)::FromPtr(this), t, t->precallFrame.prv);
}

inline int rdxCMethod::Invoke(rdxSOperationContext *ctx, rdxWeakHdl(rdxCRuntimeThread) t, rdxLargeUInt timeout)
{
	RDX_TRY(ctx)
	{
		// pParametersAndReturnValues is only used to allow compatibility with RDX_MARSHAL macros
		t->timeout.WriteBeforeLater(timeout);
		rdxWeakHdl(rdxCMethod) callMethod = rdxWeakHdl(rdxCMethod)::FromPtr(this);

		if(this->vftIndex != 0)
		{
			rdxWeakRTRef(rdxCObject) objInstance = reinterpret_cast<const rdxTracedRTRef(rdxCObject)*>(t->stackBytes + t->stackCapacity + this->m_native.thisParameterInvokeOffset)->ToWeakRTRef();
			if(objInstance.IsNull())
				RDX_STHROW(ctx, RDX_ERROR_INVALID_PARAMETER);
			callMethod = objInstance->ObjectInfo()->containerType.StaticCast<rdxCStructuredType>()->virtualMethods->Element(this->vftIndex - 1);
		}

		const rdxICodeProvider *cp = callMethod->ObjectInfo()->ownerObjectManager->GetCodeProvider();
		return cp->RunMethod(ctx, t->ownerObjectManager, callMethod.ToWeakHdl(), t, t->precallFrame.prv);
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROWV(ctx, 0);
	}
	RDX_ENDTRY
}
