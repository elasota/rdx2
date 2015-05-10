#ifndef __RDX_BUILTINS_HPP__
#define __RDX_BUILTINS_HPP__

#include "rdx_reftypealiases.hpp"
#include "rdx_reftypedefs.hpp"
#include "rdx_programmability.hpp"

struct rdxSBuiltIns
{
	rdxCRef(rdxCStructuredType) st_ArrayOfType;
	rdxCRef(rdxCStructuredType) st_DelegateType;
	rdxCRef(rdxCStructuredType) st_Object;
	rdxCRef(rdxCStructuredType) st_Thread;
	rdxCRef(rdxCStructuredType) st_Array;
	rdxCRef(rdxCStructuredType) st_Type;
	rdxCRef(rdxCStructuredType) st_Bool;
	rdxCRef(rdxCStructuredType) st_LargeInt;
	rdxCRef(rdxCStructuredType) st_LargeUInt;
	rdxCRef(rdxCStructuredType) st_Int;
	rdxCRef(rdxCStructuredType) st_UInt;
	rdxCRef(rdxCStructuredType) st_Short;
	rdxCRef(rdxCStructuredType) st_UShort;
	rdxCRef(rdxCStructuredType) st_Long;
	rdxCRef(rdxCStructuredType) st_ULong;
	rdxCRef(rdxCStructuredType) st_Float;
	rdxCRef(rdxCStructuredType) st_Double;
	rdxCRef(rdxCStructuredType) st_HashValue;
	rdxCRef(rdxCStructuredType) st_Byte;
	rdxCRef(rdxCStructuredType) st_Char;
	rdxCRef(rdxCStructuredType) st_Varying;
	rdxCRef(rdxCStructuredType) st_String;
	rdxCRef(rdxCStructuredType) st_MethodParameter;
	rdxCRef(rdxCStructuredType) st_Property;
	rdxCRef(rdxCStructuredType) st_Enumerant;
	rdxCRef(rdxCStructuredType) st_StorageSpecifier;
	rdxCRef(rdxCStructuredType) st_StructuredType;
	rdxCRef(rdxCStructuredType) st_Method;
	rdxCRef(rdxCStructuredType) st_InstructionFileInfo;
	rdxCRef(rdxCStructuredType) st_InterfaceImplementation;

	rdxArrayCRef(rdxSEnumerant) e_StorageSpecifierEnumerants;

	rdxCRef(rdxCArrayOfType) aot_MethodParameter;
	rdxCRef(rdxCArrayOfType) aot_Type;
	rdxCRef(rdxCArrayOfType) aot_StructuredType;
	rdxCRef(rdxCArrayOfType) aot_Enumerant;
	rdxCRef(rdxCArrayOfType) aot_Property;
	rdxCRef(rdxCArrayOfType) aot_Method;
	rdxCRef(rdxCArrayOfType) aot_InterfaceImplementation;
	rdxCRef(rdxCArrayOfType) aot_Object;
	rdxCRef(rdxCArrayOfType) aot_String;
	rdxCRef(rdxCArrayOfType) aot_Int;
	rdxCRef(rdxCArrayOfType) aot_UInt;
	rdxCRef(rdxCArrayOfType) aot_Char;
	rdxCRef(rdxCArrayOfType) aot_Byte;
	rdxCRef(rdxCArrayOfType) aot_InstructionFileInfo;

	rdxArrayCRef(rdxSProperty) p_MethodProperties;
	rdxArrayCRef(rdxSProperty) p_EnumerantProperties;
	rdxArrayCRef(rdxSProperty) p_MethodParameterProperties;
	rdxArrayCRef(rdxSProperty) p_TypeProperties;
	rdxArrayCRef(rdxSProperty) p_PropertyProperties;
	rdxArrayCRef(rdxSProperty) p_StructuredTypeProperties;
	rdxArrayCRef(rdxSProperty) p_ArrayOfTypeProperties;
	rdxArrayCRef(rdxSProperty) p_DelegateTypeProperties;
	rdxArrayCRef(rdxSProperty) p_InterfaceImplementationProperties;
	rdxArrayCRef(rdxSProperty) p_InstructionFileInfoProperties;

	// ======================================================================
	// These are only guaranteed to be set after a load
	rdxArrayCRef(rdxTracedRTRef(void)) providerDictionary;

	rdxLargeUInt providerDictionaryTracedSymbolCount;
};

#endif
