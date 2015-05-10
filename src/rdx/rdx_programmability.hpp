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
#ifndef __RDX_PROGRAMMABILITY_HPP__
#define __RDX_PROGRAMMABILITY_HPP__

#include <cstddef>

#include "rdx_processing.hpp"
#include "rdx_pragmas.hpp"
#include "rdx_utility.hpp"
#include "rdx_ilopcodes.hpp"
#include "rdx_callbacks.hpp"
#include "rdx_typeprocessor.hpp"
#include "rdx_steal.hpp"
#include "rdx_packagereference.hpp"
#include "rdx_pccm_callback.hpp"
#include "rdx_charspan.hpp"

struct rdxSOperationContext;

struct rdxITypeProcessor;
struct rdxITypeSerializer;
struct rdxGCInfo;
struct rdxIObjectManager;

struct rdxSILInstruction;
struct rdxSILCallPoint;
struct rdxSILDebugInfo;
union rdxUILOpCompactArg;
union rdxUILOpLargeArg;
struct rdxSStackJournal;
struct rdxSExceptionHandlerJournal;
struct rdxSMILStackAction;
struct rdxSMarkupInstruction;

class rdxCStructuredType;
class rdxCMethod;
class rdxCRuntimeThread;
union rdxURuntimeStackValue;
class rdxCPrecompiledCodeModule;

struct rdxSPackageReference;

typedef rdxBool rdxSerializationDetector;

	
class rdxCString : public rdxCObject
{
	friend class rdxCObjectManager;

	struct NativeProperties
	{
		rdxLargeUInt					length;
		rdxTracedArrayRTRef(rdxChar)	characters;
	} m_native;

public:
	inline rdxCString(rdxIObjectManager *objm, rdxGCInfo *info)
		: rdxCObject(objm, info)
	{
	}

	inline bool Equal(const char *str) const
	{
		rdxLargeUInt i;
		const rdxChar *carray = m_native.characters->ArrayData();
		for(i=0;str[i];i++)
			if(carray[i] != static_cast<rdxChar>(str[i]))
				return false;
		if(i != m_native.length)
			return false;
		return true;
	}

	inline bool Equal(const rdxSCharSpan &charSpan) const
	{
		return charSpan.Length() == m_native.length && !memcmp(charSpan.Chars(), m_native.characters->ArrayData(), m_native.length * sizeof(rdxChar));
	}

	inline bool StartsWith(const char *str) const
	{
		const rdxChar *carray = m_native.characters->ArrayData();
		for(rdxLargeUInt i=0;str[i];i++)
			if(carray[i] != static_cast<rdxChar>(str[i]))
				return false;
		return true;
	}

	inline bool StartsWith(rdxWeakRTRef(rdxCString) str) const
	{
		if(str.IsNull())
			return false;

		rdxLargeUInt rslen = str->m_native.length;

		const rdxChar *carray1 = m_native.characters->ArrayData();
		const rdxChar *carray2 = str->m_native.characters->ArrayData();

		if(m_native.length < rslen)
			return false;

		for(rdxLargeUInt i=0;i<rslen;i++)
			if(carray1[i] != carray2[i])
				return false;
		return true;
	}

	inline rdxArrayCRef(rdxChar) AsChars() const
	{
		if(this == NULL)
			return rdxArrayCRef(rdxChar)::Null();
		return this->m_native.characters.ToCRef();
	}

	inline rdxWeakArrayRTRef(rdxChar) AsCharsRTRef() const
	{
		if(this == NULL)
			return rdxWeakArrayRTRef(rdxChar)::Null();
		return this->m_native.characters.ToWeakRTRef();
	}

	inline rdxSCharSpan AsCharSpan() const
	{
		if(this == NULL)
			return rdxSCharSpan();
		return rdxSCharSpan(m_native.characters->ArrayData(), m_native.length);
	}

	inline rdxLargeUInt Length() const
	{
		return this->m_native.length;
	}

	// TODO MUSTFIX: strings must not be loadable
	void VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable);

private:
	inline void Initialize(rdxWeakArrayRTRef(rdxChar) chars, rdxLargeUInt length)
	{
		m_native.characters = chars;
		m_native.length = length;
	}
};
RDX_DECLARE_COMPLEX_NATIVE_CLASS(rdxCString);

class rdxCType : public rdxCObject
{
public:
	explicit rdxCType(rdxIObjectManager *objm, rdxGCInfo *info);

	RDX_DECLARE_PROPERTY_LOOKUP;
};
RDX_DECLARE_COMPLEX_NATIVE_CLASS(rdxCType);

struct rdxSMethodParameter
{
	RDX_DECLARE_PROPERTY_LOOKUP;

	rdxTracedRTRef(rdxCType)	type;
	rdxBool						isConstant;
	rdxBool						isNotNull;
};
RDX_DECLARE_COMPLEX_NATIVE_STRUCT(rdxSMethodParameter);


class rdxCArrayOfType : public rdxCType
{
	RDX_DECLARE_PROPERTY_LOOKUP;

public:
	explicit rdxCArrayOfType(rdxIObjectManager *objm, rdxGCInfo *info);

	struct
	{
		rdxIfcTypeInfo		arrayTypeInfo;
	} m_native;

	rdxTracedRTRef(rdxCType)	type;
	rdxLargeUInt				numDimensions;
	rdxBool						isConstant;

	// TODO MUSTFIX: ArrayOfTypes must not be loadable
	
};
RDX_DECLARE_COMPLEX_NATIVE_CLASS(rdxCArrayOfType);

class rdxCDelegateType : public rdxCType
{
	RDX_DECLARE_PROPERTY_LOOKUP;

public:
	explicit rdxCDelegateType(rdxIObjectManager *objm, rdxGCInfo *info);

	rdxTracedArrayRTRef(rdxSMethodParameter)		parameters;
	rdxTracedArrayRTRef(rdxTracedRTRef(rdxCType))	returnTypes;
};
RDX_DECLARE_COMPLEX_NATIVE_CLASS(rdxCDelegateType);

class rdxCException : public rdxCObject
{
	RDX_DECLARE_PROPERTY_LOOKUP;

public:
	explicit rdxCException(rdxIObjectManager *objm, rdxGCInfo *info);

	rdxTracedRTRef(rdxCException)				innerException;
};
RDX_DECLARE_COMPLEX_NATIVE_CLASS(rdxCException);

struct rdxSInstructionFileInfo
{
	RDX_DECLARE_PROPERTY_LOOKUP;

	rdxTracedRTRef(rdxCString)	filename;	// NULL if the same as the previous filename
	rdxUInt						line;
	rdxUInt						firstInstruction;
	
};
RDX_DECLARE_COMPLEX_NATIVE_STRUCT(rdxSInstructionFileInfo);

class rdxCMethod : public rdxCObject
{
	RDX_DECLARE_PROPERTY_LOOKUP;

public:
	struct NativeProperties
	{
		rdxTracedArrayRTRef(rdxSILInstruction)		ilinstructions;
		rdxTracedArrayRTRef(rdxUInt8)				ilResumeFlags;
		rdxTracedArrayRTRef(rdxSILCallPoint)		callPoints;
		rdxTracedArrayRTRef(rdxSILDebugInfo)		debugInfo;
		rdxTracedArrayRTRef(rdxUILOpCompactArg)		compactArgs;
		rdxTracedArrayRTRef(rdxUILOpLargeArg)		largeArgs;
		rdxTracedArrayRTRef(rdxSMILStackAction)		milStackActions;
		rdxTracedArrayRTRef(rdxSMarkupInstruction)	markupInstructions;
		rdxLargeUInt								numILInstructions;

		void										*optimizedInstructions;

		const rdxCPrecompiledCodeModule				*precompiledCodeModule;
		rdxPCCMCallback								pccmCallback;

		rdxTracedArrayRTRef(rdxUInt8)							compactedJournals;
		rdxLargeUInt											numJournals;
		rdxTracedRTRef(rdxCArray<rdxSExceptionHandlerJournal>)	exHandlers;
		rdxTracedRTRef(rdxCArray<rdxLargeUInt>)					translation1;
		rdxTracedRTRef(rdxCObject)								translation2;

		rdxNativeCallback							nativeCall;
		bool										isNativeCall;
		rdxIPToCurrentInstructionCallback			ipToCurrentInstruction;
		rdxInstrNumToIPCallback						instrNumToIP;
		rdxResumeThreadCallback						resumeThread;

		bool										intrinsicStateChecked;

		bool										isIntrinsic;
		rdxEILOpcode								opcode;
		rdxLargeInt									p1, p2;
		bool										neverFails;
		bool										isBranching;
		rdxEILOpcode								falseCheckOpcode;

		rdxLargeUInt								frameCapacity;				// Maximum amount of space this frame can consume.  Aligned to RDX_MAX_ALIGNMENT.
		rdxLargeInt									thisParameterInvokeOffset;	// Where "this" will be if inserted from a precall
	} m_native;

	explicit rdxCMethod(rdxIObjectManager *objm, rdxGCInfo *info);
	~rdxCMethod();

	void DetermineIntrinsicState();

	virtual int ExplicitInvoke(rdxSOperationContext *ctx, rdxWeakHdl(rdxCRuntimeThread) t, rdxLargeUInt timeout) RDX_FINAL;
	virtual int Invoke(rdxSOperationContext *ctx, rdxWeakHdl(rdxCRuntimeThread) t, rdxLargeUInt timeout) RDX_FINAL;

	void VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable);

	bool OnLoad(rdxSOperationContext *ctx, rdxIObjectManager *objm);

	rdxTracedArrayRTRef(rdxSMethodParameter)		parameters;
	rdxTracedArrayRTRef(rdxTracedRTRef(rdxCType))	returnTypes;
	rdxTracedArrayRTRef(rdxByte)					bytecode;
	rdxTracedArrayRTRef(rdxTracedRTRef(rdxCObject))	resArgs;
	rdxTracedArrayRTRef(rdxSInstructionFileInfo)	instructionFileInfos;
	rdxUInt											numInstructions;
	rdxUInt											vftIndex;				// 0 for direct calls, index+1 for virtual
	rdxUInt											thisParameterOffset;	// 0 for direct calls, param+1 for virtual
	rdxBool											isAbstract;

};
RDX_DECLARE_COMPLEX_NATIVE_CLASS(rdxCMethod);

struct rdxSProperty
{
	RDX_DECLARE_PROPERTY_LOOKUP;

	rdxTracedRTRef(rdxCString)	name;
	rdxTracedRTRef(rdxCType)	type;
	rdxBool						isConstant;
	rdxBool						mustBeConstant;
};
RDX_DECLARE_COMPLEX_NATIVE_STRUCT(rdxSProperty);


struct rdxSEnumerant
{
	RDX_DECLARE_PROPERTY_LOOKUP;

	rdxTracedRTRef(rdxCString)	name;
	rdxEnumValue				value;
};
RDX_DECLARE_COMPLEX_NATIVE_STRUCT(rdxSEnumerant);

struct rdxSInterfaceImplementation
{
	RDX_DECLARE_PROPERTY_LOOKUP;

	rdxTracedRTRef(rdxCStructuredType)	type;
	rdxUInt								vftOffset;
};
RDX_DECLARE_COMPLEX_NATIVE_STRUCT(rdxSInterfaceImplementation);

enum rdxEStorageSpecifier
{
	rdxSS_Class,
	rdxSS_RefStruct,
	rdxSS_ValStruct,
	rdxSS_Enum,
	rdxSS_Interface,
};


enum rdxImmediateType
{

	/*
	rdxALIASINGTYPE_Reference,
	rdxALIASINGTYPE_Float64,
	rdxALIASINGTYPE_Float32,
	rdxALIASINGTYPE_RuntimePointer,
	rdxALIASINGTYPE_Varying,
	*/
};

class rdxCStructuredTypeSerializer : public rdxITypeSerializer
{
private:
	void SetDefaultFromPackageRef(rdxSOperationContext *ctx, rdxWeakTypelessOffsetHdl instance, const rdxSPackageReference &pkgRef) const;

public:
	virtual void DeserializeTextInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakTypelessOffsetHdl instance, rdxITextDeserializer *td, rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool &outShouldProcessProperties) const RDX_OVERRIDE;
	virtual void DeserializeBinaryInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakTypelessOffsetHdl instance, rdxIFileStream *reader, rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool &outShouldProcessProperties) const RDX_OVERRIDE;
	virtual void SerializeBinaryInstance(rdxIObjectManager *objm, rdxWeakTypelessOffsetRTRef obj, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties) const RDX_OVERRIDE;
	virtual void SerializeTextInstance(rdxIObjectManager *objm, rdxWeakTypelessOffsetRTRef obj, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties) const RDX_OVERRIDE;

	static rdxCStructuredTypeSerializer instance;
};

// Structured types are any types that contain structured data and aren't simply references to other types of info
class rdxCStructuredType : public rdxCType
{
	RDX_DECLARE_PROPERTY_LOOKUP;

public:
	rdxCStructuredType(rdxIObjectManager *objm, rdxGCInfo *info);
	void VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable);
	bool DuplicateEqual(const rdxCStructuredType &other) const;

	typedef rdxCType super;

	struct NativeProperties
	{
		enum Flags
		{
			STF_StructureEvaluated				= 1,
			STF_FinalDefaultEvaluated			= 2,
			STF_DependencyDefaultsEvaluated		= 4,
			STF_ZeroFill						= 8,	// No complex dependencies, implies DefaultEvaluated and DependencyDefaultsEvaluated
			STF_StructureIsMutable				= 32,	// All members are constant
			STF_AllowBulkSerialize				= 64,	// All members contain only NativeSafeToSerialize structures and no references, this will be bulk serialized
			//STF_HasDefaultValue				= 128,
			STF_InheritedTypeInfo				= 256,
		};

		struct ContainedReference
		{
			typedef void super;

			// Offset is to the rdxCObject base for classes and to the object head for structures
			rdxLargeUInt				offset;
			rdxTracedRTRef(rdxCType)	requiredType;
			bool						mustBeConstant;
			bool						isInterface;

			void VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable);
		};

		struct UserProperties
		{
			// User flags are flags that may be set by things that interact with the runtime
			enum UserFlags
			{
				STUF_AllowDuplicates				= 1,	// Permitted in DOMAIN_Duplicable
				//STUF_NativeCompareDuplicate		= 2,	// Native data may change during load so don't compare it for duplicates (only works with ref types)
				STUF_NativeSafeToSerialize			= 4,	// Native data is safe to serialize as binary AND is in reverse byte order on opposite endian systems
				STUF_SerializeAsReference			= 8,	// Serialize references directly, creating instances not allowed
			};

			rdxUInt8 flags;
			const rdxITypeSerializer	*typeSerializer;
		};

		rdxLargeUInt							numContainedReferences;
		rdxTracedArrayRTRef(ContainedReference)	containedReferences;

		rdxTracedArrayRTRef(rdxLargeUInt)		propertyOffsets;
		rdxTracedArrayRTRef(rdxLargeUInt)		interfaceOffsets;

		// sizeInSubclass should always be the same as size on Microsoft ABI.
		// On Itanium, it may be truncated to remove tail padding.
		rdxLargeUInt						size;
		rdxLargeUInt						sizeInSubclass;
		rdxLargeUInt						alignment;

		rdxTracedRTRef(rdxCObject)			currentDefaultValue;
		rdxTracedRTRef(rdxCObject)			originalDefaultValue;

		rdxSPackageReference				defaultValueReference;

		rdxIfcTypeInfo						nativeTypeInfo;
		bool								(*getPropertyOffsetFunc)(rdxWeakRTRef(rdxCString) name, rdxLargeUInt *outOffset);
				
		UserProperties						user;
		
		rdxUInt16 flags;
	} m_native;

	rdxTracedRTRef(rdxCStructuredType)					parentClass;
	rdxTracedArrayRTRef(rdxSInterfaceImplementation)	interfaces;
	rdxEnumValue										storageSpecifier;
	rdxTracedArrayRTRef(rdxTracedRTRef(rdxCMethod))		virtualMethods;
	rdxTracedArrayRTRef(rdxSProperty)					properties;
	rdxTracedArrayRTRef(rdxSEnumerant)					enumerants;
	rdxBool												isFinal;
	rdxBool												isAbstract;
	rdxBool												isLocalized;

};
RDX_DECLARE_COMPLEX_NATIVE_CLASS(rdxCStructuredType);
RDX_DECLARE_COMPLEX_NATIVE_STRUCT(rdxCStructuredType::NativeProperties::ContainedReference);

#endif
