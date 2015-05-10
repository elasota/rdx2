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
#include "rdx_intrinsics.hpp"
#include "rdx_programmability.hpp"
#include "rdx_lut.hpp"
#include "rdx_marshal.hpp"
#include "rdx_runtime.hpp"
#include "rdx_objectmanagement.hpp"
#include "rdx_blockcopy.hpp"
#include "rdx_operators.hpp"

#define NTHROW_EXCEPTION(lbl, name)	\
	do\
	{\
		RDX_NTRY(lbl, ctx)\
		{\
			RDX_PROTECT_ASSIGN(ctx, thread->ex, objm->LookupSymbolSimple(ctx, name).ToWeakHdl().StaticCast<rdxCException>());\
			return rdxRS_Exception;\
		}\
		RDX_NCATCH(lbl, ctx)\
		{\
			RDX_RETHROWV(ctx, rdxRS_Exception);\
		}\
		RDX_NENDTRY(lbl)\
	} while(0)

#define THROW_EXCEPTION(name)	NTHROW_EXCEPTION(EH, name)

template<class TReturn, class TLeft, class TRight, template<class TOpReturn, class TOpLeft, class TOpRight> class TOperation>
int RDX_DECL_API rdxArithmeticOpFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackValueProxy<TLeft> left(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<TRight> right(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<TReturn> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);
	returnValue.Modify() = TOperation<TReturn, TLeft, TRight>::Operate(left.Value(), right.Value());
	return rdxRS_Active;
}

template<class TReturn, class TValue, template<class TOpReturn, class TOpValue> class TOperation>
int RDX_DECL_API rdxArithmeticUnaryOpFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackValueProxy<TValue> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<TReturn> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);
	returnValue.Modify() = TOperation<TReturn, TValue>::Operate(self.Value());
	return rdxRS_Active;
}


template<class TReturn, class TLeft, class TRight, template<class TOpReturn, class TOpLeft, class TOpRight> class TOperation>
int RDX_DECL_API rdxArithmeticOpZeroCheckedFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackValueProxy<TLeft> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<TRight> v(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<TReturn> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);
	TRight rightV = v.Value();
	TLeft leftV = v.Value();
	if(rightV == static_cast<TRight>(0))
		THROW_EXCEPTION(rdxSObjectGUID::FromObjectName("Core", "RDX.DivideByZeroException.instance"));
	returnValue.Modify() = TOperation<TReturn, TLeft, TRight>::Operate(leftV, rightV);
	return rdxRS_Active;
}

template<class TLeft, class TRight, template<class TOpReturn, class TOpLeft, class TOpRight> class TOperation>
int RDX_DECL_API rdxCompareOpFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackValueProxy<TLeft> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<TRight> v(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<rdxBool> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);
	returnValue.Modify() = (TOperation<bool, TLeft, TRight>::Operate(self.Value(), v.Value()) ? rdxTrueValue : rdxFalseValue);
	return rdxRS_Active;\
}

template<class TDest, class TSource>
static int RDX_DECL_API rdxCastOpFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackValueProxy<TSource> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<TDest> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);
	returnValue.Modify() = static_cast<TDest>(self.Value());
	return rdxRS_Active;
}

template<class TValue>
static int RDX_DECL_API rdxHashFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackValueProxy<TValue> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<rdxHashValue> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);
	returnValue.Modify() = rdxHashBytes(&self.Value(), sizeof(TValue));
	return rdxRS_Active;
}

template<class T>
static int RDX_DECL_API rdxIntToStringOpFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxChar digitStack[sizeof(T)*3+1];
	rdxChar result[sizeof(T)*3+1];
	rdxLargeUInt digits = 0;
	rdxLargeUInt chars = 0;
	bool negative = false;
	rdxChar *out = result;

	rdxCStackValueProxy<T> vparam(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackObjectRefProxy<rdxCString> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);

	T v = vparam.Value();

	if(v < 0)
	{
		*out++ = static_cast<rdxChar>('-');
		chars++;
		negative = true;
	}

	do
	{
		if(negative)
			digitStack[digits++] = static_cast<rdxChar>(0 - (v % T(10)) + '0');
		else
			digitStack[digits++] = static_cast<rdxChar>((v % T(10)) + '0');
		v /= T(10);
	} while(v);

	while(digits)
	{
		*out++ = digitStack[--digits];
		chars++;
	}

	RDX_TRY(ctx)
	{
		rdxWeakRTRef(rdxCString) strRef;
		RDX_PROTECT_ASSIGN(ctx, strRef, objm->CreateString(ctx, result, true, chars));
		returnValue.Modify() = strRef;
	}
	RDX_CATCH(ctx)
	{
		returnValue.Modify() = rdxWeakRTRef(rdxCString)::Null();
		NTHROW_EXCEPTION(EH2, rdxSObjectGUID::FromObjectName("Core", "RDX.InternalFailureException.instance"));
	}
	RDX_ENDTRY

	return rdxRS_Active;
}

//Core.hashcode/methods/ToLargeUInt()
static int RDX_DECL_API rdxHashCodeToLargeUIntFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackValueProxy<rdxHashValue> hash(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<rdxLargeUInt> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);
	returnValue.Modify() = static_cast<rdxUInt>(hash.Value());

	return rdxRS_Active;
}

static int RDX_DECL_API rdxGetCurrentThreadFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackObjectRefProxy<rdxCRuntimeThread> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);

	returnValue.Modify() = thread;
	return rdxRS_Active;
}

static int RDX_DECL_API rdxObjectGetTypeFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackObjectRefProxy<rdxCObject> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackObjectRefProxy<rdxCType> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);

	returnValue.Modify() = self.Value()->ObjectInfo()->containerType;
	return rdxRS_Active;
}

static int RDX_DECL_API rdxObjectCanConvertToFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackObjectRefProxy<rdxCObject> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackObjectRefProxy<rdxCType> targetType(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<rdxBool> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);

	returnValue.Modify() = (objm->ObjectCompatible(self.Value().ToWeakRTRef(), targetType.Value().ToWeakRTRef())) ? rdxTrueValue : rdxFalseValue;
	return rdxRS_Active;
}

static int RDX_DECL_API rdxObjectGetHashCodeFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackObjectRefProxy<rdxCObject> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<rdxHashValue> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);

	rdxBaseHdl::PODType hdlPOD = self.Value().ToWeakHdl().GetPOD();
	returnValue.Modify() = rdxHashBytes(&hdlPOD, sizeof(hdlPOD));
	return rdxRS_Active;
}

// TODO MUSTFIX: Move this...
static int RDX_DECL_API rdxArrayBlockCopyFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackObjectRefProxy<rdxCArrayContainer> selfSV(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<rdxLargeUInt> srcStartSV(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackObjectRefProxy<rdxCArrayContainer> destSV(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<rdxLargeUInt> destStartSV(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<rdxLargeUInt> countSV(stackView, rdxSTACKVIEWTYPE_Parameter);

	rdxWeakHdl(rdxCArrayOfType) srcAOT = selfSV.Value()->ObjectInfo()->containerType.StaticCast<rdxCArrayOfType>();
	rdxWeakHdl(rdxCArrayOfType) destAOT = destSV.Value()->ObjectInfo()->containerType.StaticCast<rdxCArrayOfType>();
	const rdxLargeUInt nSrcElements = selfSV.Value()->NumElements();
	const rdxLargeUInt nDestElements = destSV.Value()->NumElements();
	const rdxLargeUInt stride = selfSV.Value()->Stride();

	if(!objm->TypesCompatible(srcAOT.ToWeakRTRef(), destAOT.ToWeakRTRef()))
	{
		THROW_EXCEPTION(rdxSObjectGUID::FromObjectName("Core", "RDX.IncompatibleConversionException.instance"));
	}

	if(countSV.Value() == 0)
		return rdxRS_Active;

	if(countSV.Value() > nSrcElements || countSV.Value() > nDestElements
		|| (nSrcElements - countSV.Value()) < srcStartSV.Value() || (nDestElements - countSV.Value()) < destStartSV.Value())
	{
		NTHROW_EXCEPTION(EH2, rdxSObjectGUID::FromObjectName("Core", "RDX.IndexOutOfBoundsException.instance"));
	}

	rdxBlockMove(static_cast<rdxUInt8*>(destSV.Value()->ModifyRawData()) + destStartSV.Value() * stride,
		static_cast<const rdxUInt8*>(selfSV.Value()->GetRawData()) + srcStartSV.Value() * stride,
		countSV.Value() * stride);
	return rdxRS_Active;
}

// rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, rdxCStackView stackView
static int rdxArrayCopyBaseFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCRuntimeThread) thread, const rdxWeakRTRef(rdxCArrayContainer) src, rdxTracedRTRef(rdxCArrayContainer) &outArray, bool becomeConst, bool becomeNonConst)
{
	RDX_TRY(ctx)
	{
		const rdxGCInfo *selfInfo = src->ObjectInfo();
		rdxIfcTypeInfo selfTypeInfo = selfInfo->containerType->ObjectInfo()->typeInfo;

		rdxCRef(rdxCArrayOfType) newType;
		newType = selfInfo->containerType.StaticCast<rdxCArrayOfType>();
		if(becomeConst)
		{
			RDX_PROTECT_ASSIGN(ctx, newType, objm->CreateArrayType(ctx, newType->type.ToWeakHdl(), newType->numDimensions, false, selfTypeInfo));
		}
		else if(becomeNonConst)
		{
			RDX_PROTECT_ASSIGN(ctx, newType, objm->CreateArrayType(ctx, newType->type.ToWeakHdl(), newType->numDimensions, false, selfTypeInfo));
		}

		rdxCRef(rdxCArrayContainer) newArray;
		RDX_PROTECT_ASSIGN(ctx, newArray, objm->CreateArrayContainer(ctx, src->Stride(), src->NumElements(), src->NumDimensions(), rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime), 0, newType.ToWeakHdl(), selfTypeInfo));
		rdxLargeUInt dimOffset;
		RDX_PROTECT(ctx, rdxCArrayContainer::ComputeContainerSize(ctx, src->Stride(), src->NumElements() + src->Overflow(), src->NumDimensions(), &dimOffset, RDX_CNULL));
		newArray->InitializeArray(src->NumElements(), src->Overflow(), src->Stride(), dimOffset, src->ContentsTypeInfo());

		rdxLargeUInt numDimensions = src->NumDimensions();
		for(rdxLargeUInt i=0;i<src->NumDimensions();i++)
			newArray->SetDimension(i, src->Dimension(i));
		rdxBlockCopy(newArray.Modify(), src.Data(), src->Stride() * src->NumElements());
		outArray = newArray.ToWeakRTRef();
	}
	RDX_CATCH(ctx)
	{
		NTHROW_EXCEPTION(EH2, rdxSObjectGUID::FromObjectName("Core", "RDX.AllocationFailureException.instance"));
	}
	RDX_ENDTRY

	return rdxRS_Active;
}

static int RDX_DECL_API rdxArrayDimensionFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackObjectRefProxy<rdxCArrayContainer> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<rdxLargeUInt> dimension(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<rdxLargeUInt> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);

	if(dimension.Value() >= self.Value()->NumDimensions())
	{
		THROW_EXCEPTION(rdxSObjectGUID::FromObjectName("Core", "RDX.IndexOutOfBoundsException.instance"));
	}
	returnValue.Modify() = self.Value()->Dimension(dimension.Value());
	return rdxRS_Active;
}

static int RDX_DECL_API rdxArrayLengthFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackObjectRefProxy<rdxCArrayContainer> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<rdxLargeUInt> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);

	returnValue.Modify() = self.Value()->NumElements();
	return rdxRS_Active;
}

static int RDX_DECL_API rdxArrayCloneFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackObjectRefProxy<rdxCArrayContainer> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackObjectRefProxy<rdxCArrayContainer> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);

	return rdxArrayCopyBaseFunc(ctx, objm, thread, self.Value().ToWeakRTRef(), returnValue.Modify(), false, false);
}

static int RDX_DECL_API rdxArrayToConstFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackObjectRefProxy<rdxCArrayContainer> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackObjectRefProxy<rdxCArrayContainer> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);

	return rdxArrayCopyBaseFunc(ctx, objm, thread, self.Value().ToWeakRTRef(), returnValue.Modify(), true, false);
}

static int RDX_DECL_API rdxArrayToNonConstFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackObjectRefProxy<rdxCArrayContainer> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackObjectRefProxy<rdxCArrayContainer> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);

	return rdxArrayCopyBaseFunc(ctx, objm, thread, self.Value().ToWeakRTRef(), returnValue.Modify(), false, true);
}

static int RDX_DECL_API rdxWriteLineFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackObjectRefProxy<rdxCString> str(stackView, rdxSTACKVIEWTYPE_Parameter);

	if(str.Value().IsNull())
		wprintf(L"Null\n");
	else
	{
		const wchar_t *ws = str.Value()->AsChars()->ArrayData();
		_putws(str.Value()->AsChars()->ArrayData());
	}

	return rdxRS_Active;
}


// Todo: Reimplement with GUID
/*
static int RDX_DECL_API objectGetGlobalSymbol(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, rdxCStackView stackView)
{
	rdxCStackObjectRefProxy<void> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackObjectRefProxy<rdxCString> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);

	returnValue.Set(self.Value().ObjectInfo()->gstSymbol.ToRef());
	return rdxRS_Active;
}
*/

static int RDX_DECL_API rdxStringCharsFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackObjectRefProxy<rdxCString> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackObjectRefProxy<rdxCArray<rdxChar> > returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);

	returnValue.Modify() = self.Value()->AsChars();
	return rdxRS_Active;
}

// TODO: Change this to use largeuint or restrict string size
static int RDX_DECL_API rdxStringLengthFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackObjectRefProxy<rdxCString> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<rdxLargeUInt> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);

	returnValue.Modify() = static_cast<rdxUInt>(self.Value()->Length());
	return rdxRS_Active;
}

static int RDX_DECL_API rdxStringConcatFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackObjectRefProxy<rdxCString> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackObjectRefProxy<rdxCString> rs(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackObjectRefProxy<rdxCString> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);

	RDX_TRY(ctx)
	{
		rdxWeakRTRef(rdxCString) concatenated;
		RDX_PROTECT_ASSIGN(ctx, concatenated, objm->CreateStringConcatenated(ctx, self.Value().ToWeakHdl(), rs.Value().ToWeakHdl()).ToWeakRTRef());
		returnValue.Modify() = concatenated;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROWV(ctx, rdxRS_Exception);
	}
	RDX_ENDTRY

	return rdxRS_Active;
}

static int RDX_DECL_API rdxStringSubstringFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackObjectRefProxy<rdxCString> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<rdxLargeUInt> offsetParam(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackObjectRefProxy<rdxCString> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);

	rdxLargeUInt offset = offsetParam.Value();
	const rdxLargeUInt nChars = self.Value()->Length();

	if(offset > nChars)
		offset = nChars;

	const rdxLargeUInt length = nChars - offset;

	if(offset == 0)
	{
		returnValue.Modify() = self.Value();
		return rdxRS_Active;
	}

	RDX_TRY(ctx)
	{
		rdxWeakRTRef(rdxCString) truncated;
		RDX_PROTECT_ASSIGN(ctx, truncated, objm->CreateString(ctx, self.Value()->AsChars()->OffsetElementRTRef(offset).ToHdl(), true, length));
		returnValue.Modify() = truncated;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROWV(ctx, rdxRS_Exception);
	}
	RDX_ENDTRY

	return rdxRS_Active;
}

static int RDX_DECL_API rdxStringSubstringWithLengthFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackObjectRefProxy<rdxCString> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<rdxLargeUInt> offsetParam(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<rdxLargeUInt> lengthParam(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackObjectRefProxy<rdxCString> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);
	
	rdxLargeUInt length = lengthParam.Value();
	rdxLargeUInt offset = offsetParam.Value();
	const rdxLargeUInt nChars = self.Value()->Length();

	if(offset > nChars)
		offset = nChars;

	if(length < 0)
		length = 0;
	else if(length > nChars - offset)
		length = nChars - offset;

	if(offset == 0 && length == nChars)
	{
		returnValue.Modify() = self.Value();
		return rdxRS_Active;
	}

	RDX_TRY(ctx)
	{
		rdxWeakRTRef(rdxCString) truncated;
		RDX_PROTECT_ASSIGN(ctx, truncated, objm->CreateString(ctx, self.Value()->AsChars()->OffsetElementRTRef(offset).ToHdl(), true, length));
		returnValue.Modify() = truncated;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROWV(ctx, rdxRS_Exception);
	}
	RDX_ENDTRY

	return rdxRS_Active;
}

static int RDX_DECL_API rdxStringFromCharsFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackObjectRefProxy<rdxCArray<rdxChar> > chars(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<rdxLargeUInt> offsetParam(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<rdxLargeUInt> lengthParam(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackObjectRefProxy<rdxCString> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);

	rdxLargeUInt length = lengthParam.Value();
	rdxLargeUInt offset = offsetParam.Value();
	const rdxLargeUInt nChars = chars.Value()->NumElements();

	if(offset > nChars)
		offset = nChars;

	if(length < 0)
		length = 0;
	else if(length > nChars - offset)
		length = nChars - offset;

	RDX_TRY(ctx)
	{
		rdxWeakRTRef(rdxCString) str;
		RDX_PROTECT_ASSIGN(ctx, str, objm->CreateString(ctx, chars.Value()->OffsetElementRTRef(offset).ToHdl(), true, length));
		returnValue.Modify() = str;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROWV(ctx, rdxRS_Exception);
	}
	RDX_ENDTRY

	return rdxRS_Active;
}

static int RDX_DECL_API rdxStringIndexFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackObjectRefProxy<rdxCString> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<rdxLargeUInt> index(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackValueProxy<rdxChar> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);

	rdxWeakRTRef(rdxCString) str = self.Value().ToWeakRTRef();
	const rdxLargeUInt nChars = str->Length();
	const rdxLargeUInt offset = index.Value();

	if(offset >= nChars)
		THROW_EXCEPTION(rdxSObjectGUID::FromObjectName("Core", "RDX.IndexOutOfBoundsException.instance"));

	returnValue.Modify() = str->AsChars()->Element(offset);
	return rdxRS_Active;
}

static int RDX_DECL_API rdxCharToStringFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackValueProxy<rdxChar> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackObjectRefProxy<rdxCString> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);

	rdxChar cv = self.Value();

	RDX_TRY(ctx)
	{
		rdxWeakRTRef(rdxCString) str;
		RDX_PROTECT_ASSIGN(ctx, str, objm->CreateString(ctx, &cv, true, 1));
		returnValue.Modify() = str;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROWV(ctx, rdxRS_Exception);
	}
	RDX_ENDTRY

	return rdxRS_Active;
}

static int RDX_DECL_API rdxBoolToStringFunc(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)
{
	rdxCStackViewCallback stackView(prv);
	rdxCStackValueProxy<rdxChar> self(stackView, rdxSTACKVIEWTYPE_Parameter);
	rdxCStackObjectRefProxy<rdxCString> returnValue(stackView, rdxSTACKVIEWTYPE_ReturnValue);

	RDX_TRY(ctx)
	{
		const rdxChar *pv = (self.Value() == rdxFalseValue) ? RDX_STATIC_STRING("false") : RDX_STATIC_STRING("true");
		rdxWeakRTRef(rdxCString) str;
		RDX_PROTECT_ASSIGN(ctx, str, objm->CreateString(ctx, pv).ToWeakRTRef());
		returnValue.Modify() = str;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROWV(ctx, rdxRS_Exception);
	}
	RDX_ENDTRY

	return rdxRS_Active;
}


typedef rdxCStaticLookupTable<rdxSObjectGUID, rdxNativeCallback> rdxBuiltinLUT;

static rdxBuiltinLUT::Entry bi_builtins[] =
{
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/#coerce(Core.int)"),		rdxCastOpFunc<rdxInt, rdxLargeInt> },

	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__add(Core.largeint)"),	rdxArithmeticOpFunc<rdxLargeInt, rdxLargeInt, rdxLargeInt, rdxCOpProxyAdd> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__sub(Core.largeint)"),	rdxArithmeticOpFunc<rdxLargeInt, rdxLargeInt, rdxLargeInt, rdxCOpProxySub> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__mul(Core.largeint)"),	rdxArithmeticOpFunc<rdxLargeInt, rdxLargeInt, rdxLargeInt, rdxCOpProxyMul> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__div(Core.largeint)"),	rdxArithmeticOpZeroCheckedFunc<rdxLargeInt, rdxLargeInt, rdxLargeInt, rdxCOpProxyDiv> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__mod(Core.largeint)"),	rdxArithmeticOpZeroCheckedFunc<rdxLargeInt, rdxLargeInt, rdxLargeInt, rdxCOpProxyMod> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__neg()"),				rdxArithmeticUnaryOpFunc<rdxLargeInt, rdxLargeInt, rdxCOpProxyNegate> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/#coerce(Core.string)"),	rdxIntToStringOpFunc<rdxLargeInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/#coerce(Core.long)"),	rdxCastOpFunc<rdxLong, rdxLargeInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/#coerce(Core.int)"),		rdxCastOpFunc<rdxInt, rdxLargeInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/#coerce(Core.short)"),	rdxCastOpFunc<rdxShort, rdxLargeInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/#coerce(Core.largeuint)"),	rdxCastOpFunc<rdxLargeUInt, rdxLargeInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/#coerce(Core.ulong)"),	rdxCastOpFunc<rdxULong, rdxLargeInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/#coerce(Core.uint)"),	rdxCastOpFunc<rdxUInt, rdxLargeInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/#coerce(Core.ushort)"),	rdxCastOpFunc<rdxUShort, rdxLargeInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/#coerce(Core.byte)"),	rdxCastOpFunc<rdxByte, rdxLargeInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/#coerce(Core.double)"),	rdxCastOpFunc<rdxDouble, rdxLargeInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/#coerce(Core.float)"),	rdxCastOpFunc<rdxFloat, rdxLargeInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/GetHashCode()"),			rdxHashFunc<rdxLargeInt> },

	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__lt(Core.largeint)"),	rdxCompareOpFunc<rdxLargeInt, rdxLargeInt, rdxCOpProxyLT> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__gt(Core.largeint)"),	rdxCompareOpFunc<rdxLargeInt, rdxLargeInt, rdxCOpProxyGT> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__le(Core.largeint)"),	rdxCompareOpFunc<rdxLargeInt, rdxLargeInt, rdxCOpProxyLE> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__ge(Core.largeint)"),	rdxCompareOpFunc<rdxLargeInt, rdxLargeInt, rdxCOpProxyGE> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__eq(Core.largeint)"),	rdxCompareOpFunc<rdxLargeInt, rdxLargeInt, rdxCOpProxyEqual> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__ne(Core.largeint)"),	rdxCompareOpFunc<rdxLargeInt, rdxLargeInt, rdxCOpProxyNE> },

	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__add(Core.largeuint)"),rdxArithmeticOpFunc<rdxLargeUInt, rdxLargeUInt, rdxLargeUInt, rdxCOpProxyAdd> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__sub(Core.largeuint)"),rdxArithmeticOpFunc<rdxLargeUInt, rdxLargeUInt, rdxLargeUInt, rdxCOpProxySub> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__mul(Core.largeuint)"),rdxArithmeticOpFunc<rdxLargeUInt, rdxLargeUInt, rdxLargeUInt, rdxCOpProxyMul> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__div(Core.largeuint)"),rdxArithmeticOpZeroCheckedFunc<rdxLargeUInt, rdxLargeUInt, rdxLargeUInt, rdxCOpProxyDiv> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__mod(Core.largeuint)"),rdxArithmeticOpZeroCheckedFunc<rdxLargeUInt, rdxLargeUInt, rdxLargeUInt, rdxCOpProxyMod> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/#coerce(Core.string)"),	rdxIntToStringOpFunc<rdxLargeUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/#coerce(Core.long)"),	rdxCastOpFunc<rdxLong, rdxLargeUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/#coerce(Core.int)"),		rdxCastOpFunc<rdxInt, rdxLargeUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/#coerce(Core.short)"),	rdxCastOpFunc<rdxShort, rdxLargeUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/#coerce(Core.ulong)"),	rdxCastOpFunc<rdxULong, rdxLargeUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/#coerce(Core.largeint)"),rdxCastOpFunc<rdxLargeInt, rdxLargeUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/#coerce(Core.uint)"),	rdxCastOpFunc<rdxUInt, rdxLargeUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/#coerce(Core.ushort)"),	rdxCastOpFunc<rdxUShort, rdxLargeUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/#coerce(Core.byte)"),	rdxCastOpFunc<rdxByte, rdxLargeUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/#coerce(Core.double)"),	rdxCastOpFunc<rdxDouble, rdxLargeUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/#coerce(Core.float)"),	rdxCastOpFunc<rdxFloat, rdxLargeUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__lt(Core.largeuint)"),	rdxCompareOpFunc<rdxLargeUInt, rdxLargeUInt, rdxCOpProxyLT> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__gt(Core.largeuint)"),	rdxCompareOpFunc<rdxLargeUInt, rdxLargeUInt, rdxCOpProxyGT> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__le(Core.largeuint)"),	rdxCompareOpFunc<rdxLargeUInt, rdxLargeUInt, rdxCOpProxyLE> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__ge(Core.largeuint)"),	rdxCompareOpFunc<rdxLargeUInt, rdxLargeUInt, rdxCOpProxyGE> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__eq(Core.largeuint)"),	rdxCompareOpFunc<rdxLargeUInt, rdxLargeUInt, rdxCOpProxyEqual> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__ne(Core.largeuint)"),	rdxCompareOpFunc<rdxLargeUInt, rdxLargeUInt, rdxCOpProxyNE> },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/GetHashCode()"),		rdxHashFunc<rdxLargeUInt> },

	{ rdxSObjectGUID::FromObjectName("Core", "long/methods/#coerce(Core.string)"),		rdxIntToStringOpFunc<rdxLong> },
	{ rdxSObjectGUID::FromObjectName("Core", "long/methods/#coerce(Core.int)"),			rdxCastOpFunc<rdxInt, rdxLong> },
	{ rdxSObjectGUID::FromObjectName("Core", "long/methods/#coerce(Core.short)"),		rdxCastOpFunc<rdxShort, rdxLong> },
	{ rdxSObjectGUID::FromObjectName("Core", "long/methods/#coerce(Core.largeuint)"),	rdxCastOpFunc<rdxLargeUInt, rdxLong> },
	{ rdxSObjectGUID::FromObjectName("Core", "long/methods/#coerce(Core.ulong)"),		rdxCastOpFunc<rdxULong, rdxLong> },
	{ rdxSObjectGUID::FromObjectName("Core", "long/methods/#coerce(Core.largeint)"),	rdxCastOpFunc<rdxLargeInt, rdxLong> },
	{ rdxSObjectGUID::FromObjectName("Core", "long/methods/#coerce(Core.uint)"),		rdxCastOpFunc<rdxUInt, rdxLong> },
	{ rdxSObjectGUID::FromObjectName("Core", "long/methods/#coerce(Core.ushort)"),		rdxCastOpFunc<rdxUShort, rdxLong> },
	{ rdxSObjectGUID::FromObjectName("Core", "long/methods/#coerce(Core.byte)"),		rdxCastOpFunc<rdxByte, rdxLong> },
	{ rdxSObjectGUID::FromObjectName("Core", "long/methods/#coerce(Core.double)"),		rdxCastOpFunc<rdxDouble, rdxLong> },
	{ rdxSObjectGUID::FromObjectName("Core", "long/methods/#coerce(Core.float)"),		rdxCastOpFunc<rdxFloat, rdxLong> },
	{ rdxSObjectGUID::FromObjectName("Core", "long/methods/GetHashCode()"),				rdxHashFunc<rdxLong> },

	{ rdxSObjectGUID::FromObjectName("Core", "ulong/methods/#coerce(Core.string)"),		rdxIntToStringOpFunc<rdxULong> },
	{ rdxSObjectGUID::FromObjectName("Core", "ulong/methods/#coerce(Core.long)"),		rdxCastOpFunc<rdxLong, rdxULong> },
	{ rdxSObjectGUID::FromObjectName("Core", "ulong/methods/#coerce(Core.int)"),		rdxCastOpFunc<rdxInt, rdxULong> },
	{ rdxSObjectGUID::FromObjectName("Core", "ulong/methods/#coerce(Core.short)"),		rdxCastOpFunc<rdxShort, rdxULong> },
	{ rdxSObjectGUID::FromObjectName("Core", "ulong/methods/#coerce(Core.largeuint)"),	rdxCastOpFunc<rdxLargeUInt, rdxULong> },
	{ rdxSObjectGUID::FromObjectName("Core", "ulong/methods/#coerce(Core.largeint)"),	rdxCastOpFunc<rdxLargeInt, rdxULong> },
	{ rdxSObjectGUID::FromObjectName("Core", "ulong/methods/#coerce(Core.uint)"),		rdxCastOpFunc<rdxUInt, rdxULong> },
	{ rdxSObjectGUID::FromObjectName("Core", "ulong/methods/#coerce(Core.ushort)"),		rdxCastOpFunc<rdxUShort, rdxULong> },
	{ rdxSObjectGUID::FromObjectName("Core", "ulong/methods/#coerce(Core.byte)"),		rdxCastOpFunc<rdxByte, rdxULong> },
	{ rdxSObjectGUID::FromObjectName("Core", "ulong/methods/#coerce(Core.double)"),		rdxCastOpFunc<rdxDouble, rdxULong> },
	{ rdxSObjectGUID::FromObjectName("Core", "ulong/methods/#coerce(Core.float)"),		rdxCastOpFunc<rdxFloat, rdxULong> },
	{ rdxSObjectGUID::FromObjectName("Core", "ulong/methods/GetHashCode()"),			rdxHashFunc<rdxULong> },

	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__add(Core.int)"),			rdxArithmeticOpFunc<rdxInt, rdxInt, rdxInt, rdxCOpProxyAdd> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__sub(Core.int)"),			rdxArithmeticOpFunc<rdxInt, rdxInt, rdxInt, rdxCOpProxySub> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__mul(Core.int)"),			rdxArithmeticOpFunc<rdxInt, rdxInt, rdxInt, rdxCOpProxyMul> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__div(Core.int)"),			rdxArithmeticOpZeroCheckedFunc<rdxInt, rdxInt, rdxInt, rdxCOpProxyDiv> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__mod(Core.int)"),			rdxArithmeticOpZeroCheckedFunc<rdxInt, rdxInt, rdxInt, rdxCOpProxyMod> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__neg()"),					rdxArithmeticUnaryOpFunc<rdxInt, rdxInt, rdxCOpProxyNegate> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/#coerce(Core.long)"),			rdxCastOpFunc<rdxLong, rdxInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/#coerce(Core.short)"),		rdxCastOpFunc<rdxShort, rdxInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/#coerce(Core.largeuint)"),	rdxCastOpFunc<rdxLargeUInt, rdxInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/#coerce(Core.ulong)"),		rdxCastOpFunc<rdxULong, rdxInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/#coerce(Core.largeint)"),		rdxCastOpFunc<rdxLargeInt, rdxInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/#coerce(Core.uint)"),			rdxCastOpFunc<rdxUInt, rdxInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/#coerce(Core.ushort)"),		rdxCastOpFunc<rdxUShort, rdxInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/#coerce(Core.byte)"),			rdxCastOpFunc<rdxByte, rdxInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/#coerce(Core.double)"),		rdxCastOpFunc<rdxDouble, rdxInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/#coerce(Core.float)"),		rdxCastOpFunc<rdxFloat, rdxInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/#coerce(Core.string)"),		rdxIntToStringOpFunc<rdxInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__lt(Core.int)"),				rdxCompareOpFunc<rdxInt, rdxInt, rdxCOpProxyLT> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__gt(Core.int)"),				rdxCompareOpFunc<rdxInt, rdxInt, rdxCOpProxyGT> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__le(Core.int)"),				rdxCompareOpFunc<rdxInt, rdxInt, rdxCOpProxyLE> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__ge(Core.int)"),				rdxCompareOpFunc<rdxInt, rdxInt, rdxCOpProxyGE> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__eq(Core.int)"),				rdxCompareOpFunc<rdxInt, rdxInt, rdxCOpProxyEqual> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__ne(Core.int)"),				rdxCompareOpFunc<rdxInt, rdxInt, rdxCOpProxyNE> },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/GetHashCode()"),				rdxHashFunc<rdxInt> },
	

	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__add(Core.uint)"),			rdxArithmeticOpFunc<rdxUInt, rdxUInt, rdxUInt, rdxCOpProxyAdd> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__sub(Core.uint)"),			rdxArithmeticOpFunc<rdxUInt, rdxUInt, rdxUInt, rdxCOpProxySub> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__mul(Core.uint)"),			rdxArithmeticOpFunc<rdxUInt, rdxUInt, rdxUInt, rdxCOpProxyMul> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__div(Core.uint)"),			rdxArithmeticOpZeroCheckedFunc<rdxUInt, rdxUInt, rdxUInt, rdxCOpProxyDiv> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__mod(Core.uint)"),			rdxArithmeticOpZeroCheckedFunc<rdxUInt, rdxUInt, rdxUInt, rdxCOpProxyMod> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.long)"),		rdxCastOpFunc<rdxLong, rdxUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.int)"),			rdxCastOpFunc<rdxInt, rdxUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.short)"),		rdxCastOpFunc<rdxShort, rdxUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.largeuint)"),	rdxCastOpFunc<rdxLargeUInt, rdxUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.ulong)"),		rdxCastOpFunc<rdxULong, rdxUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.largeint)"),	rdxCastOpFunc<rdxLargeInt, rdxUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.ushort)"),		rdxCastOpFunc<rdxUShort, rdxUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.byte)"),		rdxCastOpFunc<rdxByte, rdxUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.double)"),		rdxCastOpFunc<rdxDouble, rdxUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.float)"),		rdxCastOpFunc<rdxFloat, rdxUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.string)"),		rdxIntToStringOpFunc<rdxUInt> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__lt(Core.uint)"),			rdxCompareOpFunc<rdxUInt, rdxUInt, rdxCOpProxyLT> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__gt(Core.uint)"),			rdxCompareOpFunc<rdxUInt, rdxUInt, rdxCOpProxyGT> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__le(Core.uint)"),			rdxCompareOpFunc<rdxUInt, rdxUInt, rdxCOpProxyLE> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__ge(Core.uint)"),			rdxCompareOpFunc<rdxUInt, rdxUInt, rdxCOpProxyGE> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__eq(Core.uint)"),			rdxCompareOpFunc<rdxUInt, rdxUInt, rdxCOpProxyEqual> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__ne(Core.uint)"),			rdxCompareOpFunc<rdxUInt, rdxUInt, rdxCOpProxyNE> },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/GetHashCode()"),				rdxHashFunc<rdxUInt> },
	
	{ rdxSObjectGUID::FromObjectName("Core", "short/methods/#coerce(Core.long)"),		rdxCastOpFunc<rdxLong, rdxShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "short/methods/#coerce(Core.int)"),		rdxCastOpFunc<rdxInt, rdxShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "short/methods/#coerce(Core.largeuint)"),	rdxCastOpFunc<rdxLargeUInt, rdxShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "short/methods/#coerce(Core.ulong)"),		rdxCastOpFunc<rdxULong, rdxShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "short/methods/#coerce(Core.largeint)"),	rdxCastOpFunc<rdxLargeInt, rdxShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "short/methods/#coerce(Core.uint)"),		rdxCastOpFunc<rdxUInt, rdxShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "short/methods/#coerce(Core.ushort)"),		rdxCastOpFunc<rdxUShort, rdxShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "short/methods/#coerce(Core.byte)"),		rdxCastOpFunc<rdxByte, rdxShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "short/methods/#coerce(Core.double)"),		rdxCastOpFunc<rdxDouble, rdxShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "short/methods/#coerce(Core.float)"),		rdxCastOpFunc<rdxFloat, rdxShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "short/methods/#coerce(Core.string)"),		rdxIntToStringOpFunc<rdxShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "short/methods/GetHashCode()"),			rdxHashFunc<rdxShort> },
	
	{ rdxSObjectGUID::FromObjectName("Core", "ushort/methods/#coerce(Core.long)"),		rdxCastOpFunc<rdxLong, rdxUShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "ushort/methods/#coerce(Core.int)"),		rdxCastOpFunc<rdxInt, rdxUShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "ushort/methods/#coerce(Core.short)"),		rdxCastOpFunc<rdxShort, rdxUShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "ushort/methods/#coerce(Core.largeuint)"),	rdxCastOpFunc<rdxLargeUInt, rdxUShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "ushort/methods/#coerce(Core.ulong)"),		rdxCastOpFunc<rdxULong, rdxUShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "ushort/methods/#coerce(Core.largeint)"),	rdxCastOpFunc<rdxLargeInt, rdxUShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "ushort/methods/#coerce(Core.uint)"),		rdxCastOpFunc<rdxUInt, rdxUShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "ushort/methods/#coerce(Core.byte)"),		rdxCastOpFunc<rdxByte, rdxUShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "ushort/methods/#coerce(Core.double)"),	rdxCastOpFunc<rdxDouble, rdxUShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "ushort/methods/#coerce(Core.float)"),		rdxCastOpFunc<rdxFloat, rdxUShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "ushort/methods/#coerce(Core.string)"),	rdxIntToStringOpFunc<rdxUShort> },
	{ rdxSObjectGUID::FromObjectName("Core", "ushort/methods/GetHashCode()"),			rdxHashFunc<rdxUShort> },

	{ rdxSObjectGUID::FromObjectName("Core", "byte/methods/#coerce(Core.long)"),		rdxCastOpFunc<rdxLong, rdxByte> },
	{ rdxSObjectGUID::FromObjectName("Core", "byte/methods/#coerce(Core.int)"),			rdxCastOpFunc<rdxInt, rdxByte> },
	{ rdxSObjectGUID::FromObjectName("Core", "byte/methods/#coerce(Core.short)"),		rdxCastOpFunc<rdxShort, rdxByte> },
	{ rdxSObjectGUID::FromObjectName("Core", "byte/methods/#coerce(Core.largeuint)"),	rdxCastOpFunc<rdxLargeUInt, rdxByte> },
	{ rdxSObjectGUID::FromObjectName("Core", "byte/methods/#coerce(Core.ulong)"),		rdxCastOpFunc<rdxULong, rdxByte> },
	{ rdxSObjectGUID::FromObjectName("Core", "byte/methods/#coerce(Core.largeint)"),	rdxCastOpFunc<rdxLargeInt, rdxByte> },
	{ rdxSObjectGUID::FromObjectName("Core", "byte/methods/#coerce(Core.uint)"),		rdxCastOpFunc<rdxUInt, rdxByte> },
	{ rdxSObjectGUID::FromObjectName("Core", "byte/methods/#coerce(Core.ushort)"),		rdxCastOpFunc<rdxUShort, rdxByte> },
	{ rdxSObjectGUID::FromObjectName("Core", "byte/methods/#coerce(Core.byte)"),		rdxCastOpFunc<rdxByte, rdxByte> },
	{ rdxSObjectGUID::FromObjectName("Core", "byte/methods/#coerce(Core.double)"),		rdxCastOpFunc<rdxDouble, rdxByte> },
	{ rdxSObjectGUID::FromObjectName("Core", "byte/methods/#coerce(Core.float)"),		rdxCastOpFunc<rdxFloat, rdxByte> },
	{ rdxSObjectGUID::FromObjectName("Core", "byte/methods/#coerce(Core.string)"),		rdxIntToStringOpFunc<rdxByte> },
	{ rdxSObjectGUID::FromObjectName("Core", "byte/methods/GetHashCode()"),				rdxHashFunc<rdxByte> },

	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__add(Core.float)"),		rdxArithmeticOpFunc<rdxFloat, rdxFloat, rdxFloat, rdxCOpProxyAdd> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__sub(Core.float)"),		rdxArithmeticOpFunc<rdxFloat, rdxFloat, rdxFloat, rdxCOpProxySub> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__mul(Core.float)"),		rdxArithmeticOpFunc<rdxFloat, rdxFloat, rdxFloat, rdxCOpProxyMul> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__div(Core.float)"),		rdxArithmeticOpZeroCheckedFunc<rdxFloat, rdxFloat, rdxFloat, rdxCOpProxyDiv> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__neg()"),					rdxArithmeticUnaryOpFunc<rdxFloat, rdxFloat, rdxCOpProxyNegate> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__lt(Core.float)"),			rdxCompareOpFunc<rdxInt, rdxInt, rdxCOpProxyLT> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__gt(Core.float)"),			rdxCompareOpFunc<rdxInt, rdxInt, rdxCOpProxyGT> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__le(Core.float)"),			rdxCompareOpFunc<rdxInt, rdxInt, rdxCOpProxyLE> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__ge(Core.float)"),			rdxCompareOpFunc<rdxInt, rdxInt, rdxCOpProxyGE> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__eq(Core.float)"),			rdxCompareOpFunc<rdxInt, rdxInt, rdxCOpProxyEqual> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__ne(Core.float)"),			rdxCompareOpFunc<rdxInt, rdxInt, rdxCOpProxyNE> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/#coerce(Core.long)"),		rdxCastOpFunc<rdxLong, rdxFloat> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/#coerce(Core.int)"),		rdxCastOpFunc<rdxInt, rdxFloat> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/#coerce(Core.short)"),		rdxCastOpFunc<rdxShort, rdxFloat> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/#coerce(Core.largeuint)"),	rdxCastOpFunc<rdxLargeUInt, rdxFloat> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/#coerce(Core.ulong)"),		rdxCastOpFunc<rdxULong, rdxFloat> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/#coerce(Core.largeint)"),	rdxCastOpFunc<rdxLargeInt, rdxFloat> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/#coerce(Core.uint)"),		rdxCastOpFunc<rdxUInt, rdxFloat> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/#coerce(Core.ushort)"),		rdxCastOpFunc<rdxUShort, rdxFloat> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/#coerce(Core.byte)"),		rdxCastOpFunc<rdxByte, rdxFloat> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/#coerce(Core.double)"),		rdxCastOpFunc<rdxDouble, rdxFloat> },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/GetHashCode()"),			rdxHashFunc<rdxFloat> },

	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__add(Core.double)"),		rdxArithmeticOpFunc<rdxDouble, rdxDouble, rdxDouble, rdxCOpProxyAdd> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__sub(Core.double)"),		rdxArithmeticOpFunc<rdxDouble, rdxDouble, rdxDouble, rdxCOpProxySub> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__mul(Core.double)"),		rdxArithmeticOpFunc<rdxDouble, rdxDouble, rdxDouble, rdxCOpProxyMul> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__div(Core.double)"),		rdxArithmeticOpZeroCheckedFunc<rdxDouble, rdxDouble, rdxDouble, rdxCOpProxyDiv> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__neg()"),					rdxArithmeticUnaryOpFunc<rdxDouble, rdxDouble, rdxCOpProxyNegate> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__lt(Core.double)"),		rdxCompareOpFunc<rdxInt, rdxInt, rdxCOpProxyLT> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__gt(Core.double)"),		rdxCompareOpFunc<rdxInt, rdxInt, rdxCOpProxyGT> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__le(Core.double)"),		rdxCompareOpFunc<rdxInt, rdxInt, rdxCOpProxyLE> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__ge(Core.double)"),		rdxCompareOpFunc<rdxInt, rdxInt, rdxCOpProxyGE> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__eq(Core.double)"),		rdxCompareOpFunc<rdxInt, rdxInt, rdxCOpProxyEqual> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__ne(Core.double)"),		rdxCompareOpFunc<rdxInt, rdxInt, rdxCOpProxyNE> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/#coerce(Core.long)"),		rdxCastOpFunc<rdxLong, rdxDouble> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/#coerce(Core.int)"),		rdxCastOpFunc<rdxInt, rdxDouble> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/#coerce(Core.short)"),		rdxCastOpFunc<rdxShort, rdxDouble> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/#coerce(Core.largeuint)"),	rdxCastOpFunc<rdxLargeUInt, rdxDouble> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/#coerce(Core.ulong)"),		rdxCastOpFunc<rdxULong, rdxDouble> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/#coerce(Core.largeint)"),	rdxCastOpFunc<rdxLargeInt, rdxDouble> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/#coerce(Core.uint)"),		rdxCastOpFunc<rdxUInt, rdxDouble> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/#coerce(Core.ushort)"),	rdxCastOpFunc<rdxUShort, rdxDouble> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/#coerce(Core.byte)"),		rdxCastOpFunc<rdxByte, rdxDouble> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/#coerce(Core.float)"),		rdxCastOpFunc<rdxFloat, rdxDouble> },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/GetHashCode()"),			rdxHashFunc<rdxDouble> },

	{ rdxSObjectGUID::FromObjectName("Core", "Object/methods/GetType()"),				rdxObjectGetTypeFunc },
	{ rdxSObjectGUID::FromObjectName("Core", "Object/methods/CanConvertTo(notnull Core.RDX.Type)"),
																						rdxObjectCanConvertToFunc },
	{ rdxSObjectGUID::FromObjectName("Core", "Object/methods/GetHashCode()"),			rdxObjectGetHashCodeFunc },
	//{ rdxSObjectGUID::FromObjectName("Core", "Object/methods/GlobalSymbol()"),		rdxObjectGetGlobalSymbolFunc },

	{ rdxSObjectGUID::FromObjectName("Core", "string/methods/Chars()"),					rdxStringCharsFunc },
	{ rdxSObjectGUID::FromObjectName("Core", "string/methods/Length()"),				rdxStringLengthFunc },
	{ rdxSObjectGUID::FromObjectName("Core", "string/methods/__add(Core.string)"),		rdxStringConcatFunc },
	{ rdxSObjectGUID::FromObjectName("Core", "string/methods/Substring(Core.largeuint)"),rdxStringSubstringFunc },
	{ rdxSObjectGUID::FromObjectName("Core", "string/methods/Substring(Core.largeuint,Core.largeuint)"),
																						rdxStringSubstringWithLengthFunc },
	{ rdxSObjectGUID::FromObjectName("Core", "string/methods/__index(Core.largeuint)"),	rdxStringIndexFunc },
	{ rdxSObjectGUID::FromObjectName("Core", "string/methods/FromChars(notnull #Core.char[C],Core.largeuint,Core.largeuint)"),
																						rdxStringFromCharsFunc },
	{ rdxSObjectGUID::FromObjectName("Core", "char/methods/#coerce(Core.string)"),		rdxCharToStringFunc },
	{ rdxSObjectGUID::FromObjectName("Core", "char/methods/GetHashCode()"),				rdxHashFunc<rdxChar> },

	{ rdxSObjectGUID::FromObjectName("Core", "bool/methods/#coerce(Core.string)"),		rdxBoolToStringFunc },
	{ rdxSObjectGUID::FromObjectName("Core", "bool/methods/GetHashCode()"),				rdxHashFunc<rdxBool> },

	{ rdxSObjectGUID::FromObjectName("Core", "RDX.Thread/methods/GetCurrent()"),		rdxGetCurrentThreadFunc },

	{ rdxSObjectGUID::FromObjectName("Core", "hashcode/methods/ToLargeUInt()"),			rdxHashCodeToLargeUIntFunc },

	// TO BE MOVED
	{ rdxSObjectGUID::FromObjectName("Core", "Array/methods/BlockCopy(Core.largeuint,notnull Core.Array,Core.largeuint,Core.largeuint)"),
																						rdxArrayBlockCopyFunc },
	{ rdxSObjectGUID::FromObjectName("Core", "Array/methods/Clone()"),					rdxArrayCloneFunc },
	{ rdxSObjectGUID::FromObjectName("Core", "Array/methods/Dimension(Core.largeuint)"),rdxArrayDimensionFunc },
	{ rdxSObjectGUID::FromObjectName("Core", "Array/methods/Length()"),					rdxArrayLengthFunc },
	{ rdxSObjectGUID::FromObjectName("Core", "Array/methods/ToConst()"),				rdxArrayToConstFunc },
	{ rdxSObjectGUID::FromObjectName("Core", "Array/methods/ToNonConst()"),				rdxArrayToNonConstFunc },

	// TODO MUSTFIX: Move
	{ rdxSObjectGUID::FromObjectName("Apps.Common", "Console/methods/WriteLine(Core.string)"),rdxWriteLineFunc },
};

rdxBuiltinLUT bi_lut(bi_builtins, sizeof(bi_builtins)/sizeof(bi_builtins[0]));


typedef rdxCStaticLookupTable<rdxSObjectGUID, rdxSNativeFunction> rdxCIntrinsicLUT;

static rdxCIntrinsicLUT::Entry i_intrinsics[] =
{
	//functionName																			opcode			p2					p3					neverFails		isBranching		falseCheckOpcode
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/#coerce(Core.int)"),		{	rdxILOP_isx,	sizeof(rdxLargeInt),sizeof(rdxInt),		true } },

	//functionName																			opcode			p2					p3					neverFails		isBranching		falseCheckOpcode
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__add(Core.int)"),			{	rdxILOP_iadd,	sizeof(rdxInt),		0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__sub(Core.int)"),			{	rdxILOP_isub,	sizeof(rdxInt),		0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__mul(Core.int)"),			{	rdxILOP_imul,	sizeof(rdxInt),		0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__div(Core.int)"),			{	rdxILOP_idiv,	sizeof(rdxInt),		0,					false } },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__mod(Core.int)"),			{	rdxILOP_imod,	sizeof(rdxInt),		0,					false } },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__neg()"),					{	rdxILOP_ineg,	sizeof(rdxInt),		0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/#coerce(Core.largeint)"),		{	rdxILOP_isx,	sizeof(rdxInt),		sizeof(rdxLargeInt),true } },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/#coerce(Core.long)"),			{	rdxILOP_isx,	sizeof(rdxInt),		sizeof(rdxLong),	true } },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/#coerce(Core.short)"),		{	rdxILOP_isx,	sizeof(rdxInt),		sizeof(rdxShort),	true } },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/#coerce(Core.double)"),		{	rdxILOP_itof,	sizeof(rdxInt),		sizeof(rdxDouble),	true } },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/#coerce(Core.float)"),		{	rdxILOP_itof,	sizeof(rdxInt),		sizeof(rdxFloat),	true } },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__lt(Core.int)"),				{	rdxILOP_ilt,	sizeof(rdxInt),		0,					true,			true,			rdxILOP_ige } },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__gt(Core.int)"),				{	rdxILOP_igt,	sizeof(rdxInt),		0,					true,			true,			rdxILOP_ile } },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__le(Core.int)"),				{	rdxILOP_ile,	sizeof(rdxInt),		0,					true,			true,			rdxILOP_igt } },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__ge(Core.int)"),				{	rdxILOP_ige,	sizeof(rdxInt),		0,					true,			true,			rdxILOP_ilt } },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__eq(Core.int)"),				{	rdxILOP_ieq,	sizeof(rdxInt),		0,					true,			true,			rdxILOP_ine } },
	{ rdxSObjectGUID::FromObjectName("Core", "int/methods/__ne(Core.int)"),				{	rdxILOP_ine,	sizeof(rdxInt),		0,					true,			true,			rdxILOP_ieq } },

	//functionName																			opcode			p2					p3					neverFails		isBranching		falseCheckOpcode
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__add(Core.int)"),			{	rdxILOP_iadd,	sizeof(rdxUInt),	0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__sub(Core.int)"),			{	rdxILOP_isub,	sizeof(rdxUInt),	0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__mul(Core.int)"),			{	rdxILOP_imul,	sizeof(rdxUInt),	0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__div(Core.int)"),			{	rdxILOP_idiv,	sizeof(rdxUInt),	0,					false } },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__mod(Core.int)"),			{	rdxILOP_imod,	sizeof(rdxUInt),	0,					false } },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.float)"),		{	rdxILOP_iutof,	sizeof(rdxUInt),	sizeof(rdxFloat),	true } },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.double)"),		{	rdxILOP_iutof,	sizeof(rdxUInt),	sizeof(rdxDouble),	true } },

	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.long)"),		{	rdxILOP_izx,	sizeof(rdxUInt),	sizeof(rdxLong),	true } },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.int)"),			{	rdxILOP_izx,	sizeof(rdxUInt),	sizeof(rdxInt),		true } },

	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.short)"),		{	rdxILOP_izx,	sizeof(rdxUInt),	sizeof(rdxShort),	true } },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.largeuint)"),	{	rdxILOP_izx,	sizeof(rdxUInt),	sizeof(rdxLargeUInt),true } },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.ulong)"),		{	rdxILOP_izx,	sizeof(rdxUInt),	sizeof(rdxULong),	true } },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.largeint)"),	{	rdxILOP_izx,	sizeof(rdxUInt),	sizeof(rdxLargeInt),true } },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.ushort)"),		{	rdxILOP_izx,	sizeof(rdxUInt),	sizeof(rdxUShort),	true } },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/#coerce(Core.byte)"),		{	rdxILOP_izx,	sizeof(rdxUInt),	sizeof(rdxByte),	true } },

	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__lt(Core.uint)"),			{	rdxILOP_iult,	sizeof(rdxUInt),	0,					true,			true,			rdxILOP_iuge } },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__gt(Core.uint)"),			{	rdxILOP_iugt,	sizeof(rdxUInt),	0,					true,			true,			rdxILOP_iule } },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__le(Core.uint)"),			{	rdxILOP_iule,	sizeof(rdxUInt),	0,					true,			true,			rdxILOP_iugt } },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__ge(Core.uint)"),			{	rdxILOP_iuge,	sizeof(rdxUInt),	0,					true,			true,			rdxILOP_iult } },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__eq(Core.uint)"),			{	rdxILOP_ieq,	sizeof(rdxUInt),	0,					true,			true,			rdxILOP_ine } },
	{ rdxSObjectGUID::FromObjectName("Core", "uint/methods/__ne(Core.uint)"),			{	rdxILOP_ine,	sizeof(rdxUInt),	0,					true,			true,			rdxILOP_ieq } },

	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__add(Core.largeint)"),	{	rdxILOP_iadd,	sizeof(rdxLargeInt),0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__sub(Core.largeint)"),	{	rdxILOP_isub,	sizeof(rdxLargeInt),0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__mul(Core.largeint)"),	{	rdxILOP_imul,	sizeof(rdxLargeInt),0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__div(Core.largeint)"),	{	rdxILOP_idiv,	sizeof(rdxLargeInt),0,					false } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__mod(Core.largeint)"),	{	rdxILOP_imod,	sizeof(rdxLargeInt),0,					false } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__neg()"),				{	rdxILOP_ineg,	sizeof(rdxLargeInt),0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__lt(Core.largeint)"),	{	rdxILOP_ilt,	sizeof(rdxLargeInt),0,					true,			true,			rdxILOP_ige } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__gt(Core.largeint)"),	{	rdxILOP_igt,	sizeof(rdxLargeInt),0,					true,			true,			rdxILOP_ile } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__le(Core.largeint)"),	{	rdxILOP_ile,	sizeof(rdxLargeInt),0,					true,			true,			rdxILOP_igt } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__ge(Core.largeint)"),	{	rdxILOP_ige,	sizeof(rdxLargeInt),0,					true,			true,			rdxILOP_ilt } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__eq(Core.largeint)"),	{	rdxILOP_ieq,	sizeof(rdxLargeInt),0,					true,			true,			rdxILOP_ine } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeint/methods/__ne(Core.largeint)"),	{	rdxILOP_ine,	sizeof(rdxLargeInt),0,					true,			true,			rdxILOP_ieq } },

	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__add(Core.largeuint)"),{	rdxILOP_iadd,	sizeof(rdxLargeUInt),0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__sub(Core.largeuint)"),{	rdxILOP_isub,	sizeof(rdxLargeUInt),0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__mul(Core.largeuint)"),{	rdxILOP_imul,	sizeof(rdxLargeUInt),0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__div(Core.largeuint)"),{	rdxILOP_iudiv,	sizeof(rdxLargeUInt),0,					false } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__mod(Core.largeuint)"),{	rdxILOP_iumod,	sizeof(rdxLargeUInt),0,					false } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__lt(Core.largeuint)"),	{	rdxILOP_iult,	sizeof(rdxLargeUInt),0,					true,			true,			rdxILOP_iuge } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__gt(Core.largeuint)"),	{	rdxILOP_iugt,	sizeof(rdxLargeUInt),0,					true,			true,			rdxILOP_iule } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__le(Core.largeuint)"),	{	rdxILOP_iule,	sizeof(rdxLargeUInt),0,					true,			true,			rdxILOP_iugt } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__ge(Core.largeuint)"),	{	rdxILOP_iuge,	sizeof(rdxLargeUInt),0,					true,			true,			rdxILOP_iult } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__eq(Core.largeuint)"),	{	rdxILOP_ieq,	sizeof(rdxLargeUInt),0,					true,			true,			rdxILOP_ine } },
	{ rdxSObjectGUID::FromObjectName("Core", "largeuint/methods/__ne(Core.largeuint)"),	{	rdxILOP_ine,	sizeof(rdxLargeUInt),0,					true,			true,			rdxILOP_ieq } },

	//functionName																			opcode			p2					p3					neverFails		isBranching		falseCheckOpcode
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__add(Core.float)"),		{	rdxILOP_fadd,	sizeof(rdxFloat),	0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__sub(Core.float)"),		{	rdxILOP_fsub,	sizeof(rdxFloat),	0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__mul(Core.float)"),		{	rdxILOP_fmul,	sizeof(rdxFloat),	0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__div(Core.float)"),		{	rdxILOP_fdiv,	sizeof(rdxFloat),	0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__neg()"),					{	rdxILOP_fneg,	sizeof(rdxFloat),	0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__lt(Core.float)"),			{	rdxILOP_flt,	sizeof(rdxFloat),	0,					true,			true,			rdxILOP_fge } },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__gt(Core.float)"),			{	rdxILOP_fgt,	sizeof(rdxFloat),	0,					true,			true,			rdxILOP_fle } },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__le(Core.float)"),			{	rdxILOP_fle,	sizeof(rdxFloat),	0,					true,			true,			rdxILOP_fgt } },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__ge(Core.float)"),			{	rdxILOP_fge,	sizeof(rdxFloat),	0,					true,			true,			rdxILOP_flt } },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__eq(Core.float)"),			{	rdxILOP_feq,	sizeof(rdxFloat),	0,					true,			true,			rdxILOP_fne } },
	{ rdxSObjectGUID::FromObjectName("Core", "float/methods/__ne(Core.float)"),			{	rdxILOP_fne,	sizeof(rdxFloat),	0,					true,			true,			rdxILOP_feq } },

	//functionName																			opcode			p2					p3					neverFails		isBranching		falseCheckOpcode
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__add(Core.double)"),		{	rdxILOP_fadd,	sizeof(rdxDouble),	0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__sub(Core.double)"),		{	rdxILOP_fsub,	sizeof(rdxDouble),	0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__mul(Core.double)"),		{	rdxILOP_fmul,	sizeof(rdxDouble),	0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__div(Core.double)"),		{	rdxILOP_fdiv,	sizeof(rdxDouble),	0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__neg()"),					{	rdxILOP_fneg,	sizeof(rdxDouble),	0,					true } },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__lt(Core.double)"),		{	rdxILOP_flt,	sizeof(rdxDouble),	0,					true,			true,			rdxILOP_fge } },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__gt(Core.double)"),		{	rdxILOP_fgt,	sizeof(rdxDouble),	0,					true,			true,			rdxILOP_fle } },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__le(Core.double)"),		{	rdxILOP_fle,	sizeof(rdxDouble),	0,					true,			true,			rdxILOP_fgt } },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__ge(Core.double)"),		{	rdxILOP_fge,	sizeof(rdxDouble),	0,					true,			true,			rdxILOP_flt } },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__eq(Core.double)"),		{	rdxILOP_feq,	sizeof(rdxDouble),	0,					true,			true,			rdxILOP_fne } },
	{ rdxSObjectGUID::FromObjectName("Core", "double/methods/__ne(Core.double)"),		{	rdxILOP_fne,	sizeof(rdxDouble),	0,					true,			true,			rdxILOP_feq } },

	{ rdxSObjectGUID::FromObjectName("Core", "Object/methods/CanConvertTo(notnull Core.RDX.Type)"),
																						{	rdxILOP_jccp,	0,					0,						false,			true,			rdxILOP_jccf } },
};
static rdxCIntrinsicLUT i_lut(i_intrinsics, sizeof(i_intrinsics)/sizeof(i_intrinsics[0]));

const rdxSNativeFunction *rdxLookupIntrinsicFunction(rdxSObjectGUID guid)
{
	return i_lut.Lookup(guid);
}

const rdxNativeCallback *rdxLookupBuiltin(rdxSObjectGUID name)
{
	return bi_lut.Lookup(name);
}

