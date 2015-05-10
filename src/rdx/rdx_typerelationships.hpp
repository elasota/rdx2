#ifndef __RDX_TYPERELATIONSHIPS_HPP__
#define __RDX_TYPERELATIONSHIPS_HPP__

#include "rdx_coretypes.hpp"
#include "rdx_superresolver.hpp"

class rdxCObject;

RDX_DECLARE_EXPLICIT_SUPER(class, rdxCArrayContainer, rdxCObject);
RDX_DECLARE_EXPLICIT_SUPER(class, rdxCType, rdxCObject);
RDX_DECLARE_EXPLICIT_SUPER(class, rdxCString, rdxCObject);
//RDX_DECLARE_EXPLICIT_SUPER(struct, rdxSMethodParameter, rdxCObject);
RDX_DECLARE_EXPLICIT_SUPER(class, rdxCException, rdxCObject);
RDX_DECLARE_EXPLICIT_SUPER(class, rdxCStructuredType, rdxCType);
RDX_DECLARE_EXPLICIT_SUPER(class, rdxCArrayOfType, rdxCType);
RDX_DECLARE_EXPLICIT_SUPER(class, rdxCDelegateType, rdxCType);
RDX_DECLARE_EXPLICIT_SUPER(class, rdxCMethod, rdxCObject);
RDX_DECLARE_EXPLICIT_SUPER(class, rdxCPackage, rdxCObject);
RDX_DECLARE_EXPLICIT_SUPER(class, rdxCRuntimeThread, rdxCObject);
RDX_DECLARE_EXPLICIT_SUPER(class, rdxCStructContainer, rdxCObject);

/*

#ifdef RDX_WCHAR_T_IS_NATIVE
RDX_DECLARE_EXPLICIT_SUPER_NATIVE(wchar_t, void);
#endif

RDX_DECLARE_EXPLICIT_SUPER_NATIVE(rdxUInt8, void);
RDX_DECLARE_EXPLICIT_SUPER_NATIVE(rdxUInt16, void);
RDX_DECLARE_EXPLICIT_SUPER_NATIVE(rdxUInt32, void);
RDX_DECLARE_EXPLICIT_SUPER_NATIVE(rdxUInt64, void);
RDX_DECLARE_EXPLICIT_SUPER_NATIVE(rdxInt8, void);
RDX_DECLARE_EXPLICIT_SUPER_NATIVE(rdxInt16, void);
RDX_DECLARE_EXPLICIT_SUPER_NATIVE(rdxInt32, void);
RDX_DECLARE_EXPLICIT_SUPER_NATIVE(rdxInt64, void);
RDX_DECLARE_EXPLICIT_SUPER_NATIVE(rdxFloat32, void);
RDX_DECLARE_EXPLICIT_SUPER_NATIVE(rdxFloat64, void);

RDX_DECLARE_EXPLICIT_SUPER(struct, rdxSILCallPoint, void);
RDX_DECLARE_EXPLICIT_SUPER(class, rdxCType, void);
RDX_DECLARE_EXPLICIT_SUPER(class, rdxCString, void);
RDX_DECLARE_EXPLICIT_SUPER(struct, rdxSMethodParameter, void);
RDX_DECLARE_EXPLICIT_SUPER(class, rdxCException, void);
RDX_DECLARE_EXPLICIT_SUPER(struct, rdxSILInstruction, void);
RDX_DECLARE_EXPLICIT_SUPER(struct, rdxSILDebugInfo, void);
RDX_DECLARE_EXPLICIT_SUPER(union, rdxUILOpCompactArg, void);
RDX_DECLARE_EXPLICIT_SUPER(union, rdxUILOpLargeArg, void);
RDX_DECLARE_EXPLICIT_SUPER(struct, rdxSExceptionHandlerJournal, void);
RDX_DECLARE_EXPLICIT_SUPER(struct, rdxSInstructionFileInfo, void);
RDX_DECLARE_EXPLICIT_SUPER(class, rdxCStructuredType, rdxCType);
RDX_DECLARE_EXPLICIT_SUPER(class, rdxCArrayOfType, rdxCType);
RDX_DECLARE_EXPLICIT_SUPER(class, rdxCDelegateType, rdxCType);
RDX_DECLARE_EXPLICIT_SUPER(struct, rdxSInterfaceImplementation, void);
RDX_DECLARE_EXPLICIT_SUPER(class, rdxCMethod, void);
RDX_DECLARE_EXPLICIT_SUPER(struct, rdxSProperty, void);
RDX_DECLARE_EXPLICIT_SUPER(struct, rdxSEnumerant, void);
RDX_DECLARE_EXPLICIT_SUPER(class, rdxCPackage, void);
RDX_DECLARE_EXPLICIT_SUPER(struct, rdxSLoadShell, void);
RDX_DECLARE_EXPLICIT_SUPER(class, rdxCRuntimeThread, void);

*/

#endif
