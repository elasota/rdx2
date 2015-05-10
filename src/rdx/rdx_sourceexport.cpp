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

#include "rdx_objectmanagement.hpp"
#include "rdx_ilcomp.hpp"
#include "rdx_builtins.hpp"
#include "rdx_guid.hpp"

static void rdxSEOutNumberU(FILE *f, rdxLargeUInt lui)
{
	if(lui >= 10)
		rdxSEOutNumberU(f, lui / 10);

	fputc('0' + static_cast<int>(lui % 10), f);
}

static void rdxSEOutNumberS(FILE *f, rdxLargeInt li)
{
	if(li >= 0)
		rdxSEOutNumberU(f, static_cast<rdxLargeUInt>(li));
	else if(li < 0)
	{
		fputc('-', f);
		rdxSEOutNumberU(f, static_cast<rdxLargeUInt>(-(li / 10)));
		rdxSEOutNumberU(f, static_cast<rdxLargeUInt>(-(li % 10)));
	}
}

static void rdxSEOutLUI(FILE *f, const char *argParam, rdxLargeUInt lui)
{
	fprintf(f, "%s = \"", argParam);
	rdxSEOutNumberU(f, lui);
	fprintf(f, "\", ");
}

static void rdxSEOutLI(FILE *f, const char *argParam, rdxLargeInt li)
{
	fprintf(f, "%s = \"", argParam);
	rdxSEOutNumberS(f, li);
	fprintf(f, "\", ");
}

static void rdxSEOutRes(FILE *f, const char *argParam, rdxBaseHdl::PODType hdl, rdxWeakRTRef(rdxCMethod) method)
{
	rdxWeakRTRef(rdxCObject) p(rdxObjRef_CSignal_DataPointer, rdxRefConverter<rdxCObject>::Convert(hdl));

	rdxLargeUInt index = 0;
	rdxWeakArrayRTRef(rdxTracedRTRef(rdxCObject)) resArgs = method->resArgs.ToWeakRTRef();
	while(resArgs->Element(index) != p)
		index++;
	rdxSEOutLUI(f, argParam, index);
}

static void rdxSEOutBytes(FILE *f, const char *varName, const void *ptr, rdxLargeUInt nBytes)
{
	union
	{
		rdxUInt32 ui;
		rdxUInt8 b[4];
	} u;
	u.ui = 0;
	u.b[0] = 1;
	bool systemIsLittleEndian = (u.ui == 1);

	// Always store in little endian
	const rdxUInt8 *bptr = static_cast<const rdxUInt8 *>(ptr);
	fprintf(f, "%s = { ", varName);
	for(rdxLargeUInt i=0;i<nBytes;i++)
	{
		rdxUInt8 b = systemIsLittleEndian ? bptr[i] : bptr[nBytes - 1 - i];
		fprintf(f, "%i, ", b);
	}
	fprintf(f, "}, ");
}

static void rdxSEOutTypeContents(FILE *f, rdxWeakRTRef(rdxCType) type, rdxIObjectManager *objm)
{
	const rdxSBuiltIns *builtins = objm->GetBuiltIns();
	rdxWeakRTRef(rdxCType) containerType = type->ObjectInfo()->containerType.ToWeakRTRef();
	if(containerType == builtins->st_ArrayOfType)
	{
		rdxWeakRTRef(rdxCArrayOfType) aot = type.StaticCast<rdxCArrayOfType>();
		fprintf(f, "{ specialType = \"arrayDef\", contentsType = ");
		rdxSEOutTypeContents(f, aot->type.ToWeakRTRef(), objm);
		fprintf(f, ", isConstant = %s, ", aot->isConstant ? "true" : "false");
		fputs("numDimensions = \"", f);
		rdxSEOutNumberU(f, aot->numDimensions);
		fprintf(f, "\" } ");
	}
	else if(containerType == builtins->st_DelegateType ||
		containerType == builtins->st_StructuredType)
	{
		const char *nibbles = "0123456789abcdef";
		rdxSObjectGUID objectGUID = type->ObjectInfo()->SerializationTag()->gstSymbol;
		fputc('\"', f);
		for(rdxLargeUInt i=0;i<rdxSDomainGUID::GUID_SIZE;i++)
		{
			rdxUInt8 gbyte = objectGUID.m_domain.m_bytes[i];
			fputc(nibbles[(gbyte & 0xf0) >> 4], f);
			fputc(nibbles[gbyte & 0xf], f);
		}
		fputc('_', f);
		for(rdxLargeUInt i=0;i<rdxSObjectGUID::GUID_SIZE;i++)
		{
			rdxUInt8 gbyte = objectGUID.m_bytes[i];
			fputc(nibbles[(gbyte & 0xf0) >> 4], f);
			fputc(nibbles[gbyte & 0xf], f);
		}
		fputc('\"', f);
	}
}

static void rdxSEOutTypeRef(FILE *f, const char *varName, rdxBaseHdl::PODType type, rdxIObjectManager *objm)
{
	if(type == RDX_CNULL)
		fprintf(f, "%s = { specialType = \"null\" }, ", varName);
	else
	{
		fprintf(f, "%s = ", varName);
		rdxWeakRTRef(rdxCType) typeRTRef(rdxObjRef_CSignal_DataPointer, static_cast<rdxCType*>(rdxRefConverter<rdxCObject>::Convert(type)));
		rdxSEOutTypeContents(f, typeRTRef, objm);
		fprintf(f, ", ");
	}
}

void rdxExportMethod(rdxIObjectManager *objm, void *pf, rdxWeakRTRef(rdxCMethod) method)
{
	rdxWeakArrayRTRef(rdxSILInstruction) instructions = method->m_native.ilinstructions.ToWeakRTRef();
	FILE *f = static_cast<FILE*>(pf);
	
	fprintf(f, "local stackActions = {\n");

	rdxWeakArrayRTRef(rdxSMILStackAction) stackActions = method->m_native.milStackActions.ToWeakRTRef();
	for(rdxLargeUInt i=0;i<stackActions->NumElements();i++)
	{
		const rdxSMILStackAction &sa = stackActions->Element(i);

		fputs("    { actionType = \"", f);
		switch(sa.actionType)
		{
		case rdxEMILSAT_PushParentFrameReturnValue:
			fputs("pushParentFrameReturnValue", f);
			break;
		case rdxEMILSAT_PushParentFrameParameter:
			fputs("pushParentFrameParameter", f);
			break;
		case rdxEMILSAT_PushLocal:
			fputs("pushLocal", f);
			break;
		case rdxEMILSAT_PushOpstackValue:
			fputs("pushOpstackValue", f);
			break;
		case rdxEMILSAT_PushOpstackPtr:
			fputs("pushOpstackPtr", f);
			break;
		case rdxEMILSAT_PushOpstackVarying:
			fputs("pushOpstackVarying", f);
			break;
		case rdxEMILSAT_Pop:
			fputs("pop", f);
			break;
		};
		fputs("\", ", f);
		rdxSEOutTypeRef(f, "type", sa.valueType.GetPOD(), objm);
		fputs(" },\n", f);
	}
	fputs("}\n\n", f);
	
	const rdxUILOpCompactArg *compactArgs = RDX_CNULL;
	const rdxUILOpLargeArg *largeArgs = RDX_CNULL;
	if(method->m_native.compactArgs.IsNotNull())
		compactArgs = method->m_native.compactArgs->ArrayData();
	if(method->m_native.largeArgs.IsNotNull())
		largeArgs = method->m_native.largeArgs->ArrayData();

	//if(instructions.IsNotNull())
	{
		rdxLargeUInt numInstr = instructions->NumElements();

		fprintf(f, "local instructions = {\n");
		for(rdxLargeUInt i=0;i<numInstr;i++)
		{
			const rdxSILInstruction &ili = instructions->Element(i);
			const rdxUILOpCompactArg *argCA = compactArgs + ili.argOffs;
			const rdxUILOpLargeArg *argLA = largeArgs + ili.argOffs;

			fprintf(f, "\t\t{ opcode = \"%s\", ", rdxILOpcodeNames[ili.opcode]);

			if(method->m_native.ilResumeFlags->Element(i / 8) & (1 << (i % 8)))
				fprintf(f, "isResumable = true, ");

			switch(ili.opcode)
			{
				case rdxILOP_debuginfo:
				{
					const rdxCString *stringRef = static_cast<const rdxCString*>(rdxRefConverter<rdxCObject>::Convert(argCA[0].p));
					if(stringRef != RDX_CNULL)
					{
						const rdxChar *filenameChars = stringRef->AsChars()->ArrayData();
						fprintf(f, "filename = \"");
						for(rdxLargeInt j=0;filenameChars[j];j++)
							fputc(static_cast<int>(filenameChars[j]), f);
						fprintf(f, "\", ");
					}
				}
				rdxSEOutLUI(f, "line", argCA[1].lui);
				break;
			case rdxILOP_move:
			case rdxILOP_pushdefault:
				if(ili.opcode == rdxILOP_move)
					rdxSEOutLUI(f, "src", argCA[1].lui);
				else if(ili.opcode == rdxILOP_pushdefault)
					rdxSEOutRes(f, "src", argCA[1].p, method);

				rdxSEOutLI(f, "dest", argCA[2].lui);
				rdxSEOutTypeRef(f, "type", argCA[3].p, objm);

				{
					rdxLargeUInt flags = argCA[0].lui;
					fputs("flags = { ", f);
					if(flags & RDX_ILOP_MOVE_DEST_PARENT_FRAME)
						fputs("destParentFrame = true, ", f);
					if(flags & RDX_ILOP_MOVE_DEST_DEREF)
						fputs("destDeref = true, ", f);
					if(flags & RDX_ILOP_MOVE_DEST_OBJECT)
						fputs("destObject = true, ", f);
					if(flags & RDX_ILOP_MOVE_DEST_DESTROY)
						fputs("destDestroy = true, ", f);
					if(flags & RDX_ILOP_MOVE_SRC_PARENT_FRAME)
						fputs("srcParentFrame = true, ", f);
					if(flags & RDX_ILOP_MOVE_SRC_DEREF)
						fputs("srcDeref = true, ", f);
					if(flags & RDX_ILOP_MOVE_SRC_TRANSIENT)
						fputs("srcTransient = true, ", f);
					if(flags & RDX_ILOP_MOVE_SRC_TYPE_DEFAULT)
						fputs("srcTypeDefault = true, ", f);
					if(flags & RDX_ILOP_MOVE_SRC_OBJECT)
						fputs("object = true, ", f);
					if(flags & RDX_ILOP_MOVE_SRC_IS_CLASS_REF)
						fputs("isClassRef = true, ", f);
					if(flags & RDX_ILOP_MOVE_SRC_IS_INTERFACE_REF)
						fputs("isInterfaceRef = true, ", f);
					if(flags & RDX_ILOP_MOVE_SRC_CONTAINS_REFS)
						fputs("containsRefs = true, ", f);
					if(flags & RDX_ILOP_MOVE_IS_RTP)
						fputs("isRTP = true, ", f);
					if(flags & RDX_ILOP_MOVE_DEST_OPSTACK)
						fputs("destOpstack = true, ", f);
					if(flags & RDX_ILOP_MOVE_DEST_TRANSIENT)
						fputs("destTransient = true, ", f);
					if(flags & RDX_ILOP_MOVE_DEST_INITIALIZE)
						fputs("destInit = true, ", f);
					
					fputs("}, ", f);
				}
				break;
			case rdxILOP_clone:
				rdxSEOutLUI(f, "src", argCA[0].lui);
				rdxSEOutLUI(f, "dest", argCA[1].lui);
				break;
			case rdxILOP_tovarying:
				rdxSEOutLI(f, "dest", argCA[0].lui);
				rdxSEOutRes(f, "type", argCA[1].p, method);
				rdxSEOutLI(f, "src", argCA[2].li);
				break;
			case rdxILOP_pinl:
				rdxSEOutLI(f, "dest", argCA[0].li);
				rdxSEOutLI(f, "src", argCA[1].li);
				break;
			case rdxILOP_ptrprop:
				rdxSEOutLI(f, "loc", argCA[0].li);
				rdxSEOutTypeRef(f, "type", argCA[1].p, objm);
				rdxSEOutLI(f, "propIndex", argCA[2].lui);
				rdxSEOutLI(f, "oldSA", argCA[3].lui);
				break;
			case rdxILOP_changeproperty:
				rdxSEOutTypeRef(f, "type", argCA[0].p, objm);
				rdxSEOutLI(f, "oldProperty", argCA[1].lui);
				rdxSEOutLI(f, "newProperty", argCA[2].lui);
				rdxSEOutLUI(f, "loc", argCA[3].lui);
				break;
			case rdxILOP_objproperty:
			case rdxILOP_objproperty_notnull:
			case rdxILOP_objproperty_notnull_persist:
				rdxSEOutLI(f, "src", argCA[0].lui);
				rdxSEOutLI(f, "dest", argCA[1].lui);
				rdxSEOutTypeRef(f, "type", argCA[2].p, objm);
				rdxSEOutLI(f, "property", argCA[3].lui);
				break;
			case rdxILOP_immediate:
				rdxSEOutBytes(f, "value", &argLA[0], argLA[2].ca[0].lui);
				rdxSEOutLI(f, "dest", argLA[1].ca[0].li);
				rdxSEOutTypeRef(f, "type", argLA[1].ca[1].p, objm);
				break;
			case rdxILOP_immediate_ptr:
				rdxSEOutLI(f, "dest", argLA[1].ca[0].lui);
				rdxSEOutRes(f, "res", argLA[0].ca[0].p, method);
				break;
			case rdxILOP_immediate_rtp:
				rdxSEOutLI(f, "dest", argLA[1].ca[0].lui);
				rdxSEOutRes(f, "res", argLA[0].rtp.hdl, method);
				break;
			case rdxILOP_arrayindex:
				rdxSEOutLI(f, "indexsrc", argCA[0].lui);
				rdxSEOutLI(f, "arraysrc", argCA[1].lui);
				rdxSEOutLI(f, "dest", argCA[2].lui);
				rdxSEOutLI(f, "numdimensions", argCA[3].lui);
				break;
						
			case rdxILOP_call:
				rdxSEOutLI(f, "op0", argCA[0].li);
				rdxSEOutLI(f, "op1", argCA[1].li);
				rdxSEOutRes(f, "method", argCA[2].p, method);
				fprintf(f, "isnative = %s, ", static_cast<const rdxCMethod *>(rdxRefConverter<rdxCObject>::Convert(argCA[2].p))->m_native.isNativeCall ? "true" : "false");
				rdxSEOutLI(f, "parambase", argCA[3].lui);
				rdxSEOutLI(f, "numparams", argCA[4].lui);
				rdxSEOutLI(f, "numreturns", argCA[5].lui);
				break;
			case rdxILOP_calldelegatebp:
			case rdxILOP_calldelegateprv:
				rdxSEOutLI(f, "parambase", argCA[3].lui);
				rdxSEOutLI(f, "numparams", argCA[4].lui);
				rdxSEOutLI(f, "numreturns", argCA[5].lui);
				rdxSEOutLI(f, "methodsrc", argCA[6].lui);
				break;
			case rdxILOP_callvirtual:
				rdxSEOutLI(f, "vftindex", argCA[3].lui);
				rdxSEOutLI(f, "parambase", argCA[4].lui);
				rdxSEOutLI(f, "numparams", argCA[5].lui);
				rdxSEOutLI(f, "numreturns", argCA[6].lui);
				rdxSEOutLI(f, "objsrc", argCA[7].lui);
				break;
			case rdxILOP_callinterface:
				rdxSEOutLI(f, "vftindex", argCA[3].li);
				rdxSEOutLI(f, "parambase", argCA[4].li);
				rdxSEOutLI(f, "numparams", argCA[5].li);
				rdxSEOutLI(f, "numreturns", argCA[6].li);
				rdxSEOutLI(f, "objsrc", argCA[7].li);
				rdxSEOutTypeRef(f, "interface", argCA[8].p, objm);
				rdxSEOutRes(f, "method", argCA[9].p, method);
				break;
			case rdxILOP_verifynotnull:
				rdxSEOutLI(f, "loc", argCA[0].lui);
				rdxSEOutLI(f, "instroffset", argCA[1].li);
				break;
			case rdxILOP_zero_op:
			case rdxILOP_zero_local:
				rdxSEOutLI(f, "loc", argCA[0].li);
				rdxSEOutTypeRef(f, "type", argCA[1].p, objm);
				break;
			case rdxILOP_newinstance:
			case rdxILOP_newinstance_local:
				rdxSEOutLI(f, "loc", argCA[0].li);
				rdxSEOutRes(f, "type", argCA[1].p, method);
				rdxSEOutLI(f, "dimsrc", argCA[2].li);
				rdxSEOutLI(f, "ndim", argCA[3].li);
				break;
			case rdxILOP_exit:
				break;

			case rdxILOP_jump:
				rdxSEOutLI(f, "instrnum", argCA[0].li);
				break;
			case rdxILOP_jtrue:
			case rdxILOP_jfalse:
			case rdxILOP_jnullo:
			case rdxILOP_jnotnullo:
			case rdxILOP_jnulli:
			case rdxILOP_jnotnulli:
				rdxSEOutLI(f, "instrnum", argCA[0].li);
				rdxSEOutLI(f, "src", argCA[1].li);
				break;
			case rdxILOP_jinherits:
				rdxSEOutLI(f, "instrnum", argCA[0].li);
				rdxSEOutLI(f, "src", argCA[1].li);
				rdxSEOutRes(f, "exType", argCA[2].p, method);
				break;

			case rdxILOP_tick:
				break;
			case rdxILOP_assertenum:
				rdxSEOutLUI(f, "src", argCA[0].lui);
				rdxSEOutRes(f, "type", argCA[1].p, method);
				rdxSEOutLUI(f, "dest", argCA[2].lui);
				break;
			case rdxILOP_assertinherits:
			case rdxILOP_rcast_otoi:
			case rdxILOP_rcast_itoi:
			case rdxILOP_rcast_itoo:
				rdxSEOutLUI(f, "src", argCA[0].lui);
				rdxSEOutRes(f, "type", argCA[1].p, method);
				rdxSEOutLUI(f, "dest", argCA[2].lui);
				break;
			case rdxILOP_rcast_otoi_direct:
				rdxSEOutLUI(f, "src", argCA[0].lui);
				rdxSEOutTypeRef(f, "type", argCA[1].p, objm);
				rdxSEOutLUI(f, "dest", argCA[2].lui);
				rdxSEOutLUI(f, "ifcOffset", argCA[3].lui);
				break;
			case rdxILOP_jeq_f:
			case rdxILOP_jeq_p:
			case rdxILOP_jne_f:
			case rdxILOP_jne_p:
				rdxSEOutLI(f, "instrnum", argCA[0].li);
				rdxSEOutLI(f, "src1", argCA[1].li);
				rdxSEOutLI(f, "src2", argCA[2].li);
				rdxSEOutTypeRef(f, "type", argCA[3].p, objm);
				break;

			case rdxILOP_xnullref:
				break;
			case rdxILOP_catch:
				break;
			case rdxILOP_fatal:
				break;
			case rdxILOP_throw:
				rdxSEOutLI(f, "src", argCA[0].li);
				break;

			case rdxILOP_hardenstack:
				break;

			case rdxILOP_switch:
				rdxSEOutLI(f, "src", argCA[0].li);
				rdxSEOutRes(f, "cases", argCA[1].p, method);
				rdxSEOutLI(f, "numCases", argCA[2].li);
				rdxSEOutTypeRef(f, "type", argCA[3].p, objm);
				break;

			case rdxILOP_switch_ptr:
				rdxSEOutLI(f, "src", argCA[0].li);
				rdxSEOutRes(f, "cases", argCA[1].p, method);
				rdxSEOutLI(f, "numCases", argCA[2].li);
				rdxSEOutTypeRef(f, "type", argCA[3].p, objm);
				break;

			case rdxILOP_iteratearray:
				rdxSEOutLI(f, "array", argCA[0].li);
				rdxSEOutLI(f, "index", argCA[1].li);
				rdxSEOutLI(f, "dest", argCA[2].li);
				rdxSEOutLI(f, "exitInstr", argCA[3].li);
				rdxSEOutTypeRef(f, "subscriptType", argCA[4].p, objm);
				break;
							
			case rdxILOP_iteratearraysub:
				rdxSEOutLI(f, "array", argCA[0].li);
				rdxSEOutLI(f, "index", argCA[1].li);
				rdxSEOutLI(f, "dest", argCA[2].li);
				rdxSEOutLI(f, "exitInstr", argCA[3].li);
				rdxSEOutTypeRef(f, "subscriptType", argCA[4].p, objm);
				rdxSEOutLI(f, "subidxloc", argCA[5].li);
				rdxSEOutLI(f, "subidxcount", argCA[6].li);
				break;

			default:
				rdxSEOutLI(f, "p0", argCA[0].li);
				rdxSEOutLI(f, "p1", argCA[1].li);
				rdxSEOutLI(f, "p2", argCA[2].li);
				break;
			}
					
			fprintf(f, "},\n");
		}
		fprintf(f, "}\n\n");
	}
	
	fputs("local markupInstructions = {\n", f);
	rdxWeakArrayRTRef(rdxSMarkupInstruction) mops = method->m_native.markupInstructions.ToWeakRTRef();
	for(rdxLargeUInt i=0;i<mops->NumElements();i++)
	{
		const rdxSMarkupInstruction &mop = mops->Element(i);

		const rdxUILOpCompactArg *argCA = compactArgs + mop.argOffs;
		const rdxUILOpLargeArg *argLA = largeArgs + mop.argOffs;

		fprintf(f, "\t{ linkInstr = %u, ", static_cast<rdxUInt32>(mop.instructionLink));
		switch(mop.opcode)
		{
		case rdxMOP_incrprop:
			fputs("opcode = \"incrprop\", propIndex = \"", f);
			rdxSEOutNumberU(f, argCA[0].lui);
			fputs("\", ", f);
			rdxSEOutTypeRef(f, "type", argCA[1].p, objm);
			rdxSEOutLUI(f, "src", argCA[2].lui);
			rdxSEOutLUI(f, "dest", argCA[3].lui);
			break;
		case rdxMOP_moveprop:
			fputs("opcode = \"moveprop\", propIndex = \"", f);
			rdxSEOutNumberU(f, argCA[0].lui);
			fputs("\", ", f);
			rdxSEOutTypeRef(f, "type", argCA[1].p, objm);
			break;
		case rdxMOP_addshell:
			fputs("opcode = \"addshell\", ", f);
			rdxSEOutTypeRef(f, "type", argCA[0].p, objm);
			break;
		case rdxMOP_pop:
			fputs("opcode = \"pop\", ", f);
			rdxSEOutLUI(f, "loc", argCA[0].lui);
			break;
		case rdxMOP_unshell:
			fputs("opcode = \"unshell\", ", f);
			rdxSEOutLUI(f, "loc", argCA[0].lui);
			break;
		case rdxMOP_movesa:
			fputs("opcode = \"movesa\", ", f);
			rdxSEOutLUI(f, "src", argCA[0].lui);
			rdxSEOutLUI(f, "dest", argCA[1].lui);
			break;
		};
		fputs(" },\n", f);
	}
	fputs("}\n\n", f);
	fprintf(f, "return stackActions, instructions, markupInstructions\n");
}


RDX_DYNLIB_API void RDX_ExportMethods(rdxIObjectManager *objm, const char *outBasePath, rdxSDomainGUID domainGUID)
{
	rdxSAllocator *alloc = objm->GetAllocator();

	char *pathBuffer = alloc->CAlloc<char>(strlen(outBasePath) + 256 + (rdxSObjectGUID::GUID_SIZE + rdxSDomainGUID::GUID_SIZE) * 2, rdxALLOC_ShortTerm);
	if(!pathBuffer)
		return;

	for(rdxWeakRTRef(rdxCObject) obj = objm->FirstLiveObject().ToWeakRTRef(); obj.IsNotNull(); obj = objm->NextLiveObject(obj.ToWeakRTRef()).ToWeakHdl())
	{
		const rdxGCInfo *gci = obj->ObjectInfo();
		if(gci->containerType != objm->GetBuiltIns()->st_Method)
			continue;

		if(gci->Domain() != domainGUID
#ifdef RDX_ALLOW_PRECOMPILED_DUPLICATES
			&& gci->Domain() != rdxSDomainGUID::Builtin(rdxDOMAIN_Duplicable)
#endif
			)
			continue;

		rdxWeakRTRef(rdxCMethod) method = obj.StaticCast<rdxCMethod>();
		if(method->bytecode.IsNull())
			continue;

		const rdxSSerializationTag *serTag = gci->SerializationTag();
		if(!serTag)
			continue;

		strcpy(pathBuffer, outBasePath);
		strcat(pathBuffer, "/method_");
		rdxLargeUInt bufEnd = strlen(pathBuffer);

		const char *nibbles = "0123456789abcdef";
		const rdxUInt8 *domainBytes = serTag->gstSymbol.m_domain.m_bytes;
		const rdxUInt8 *guidBytes = serTag->gstSymbol.m_bytes;

		for(rdxLargeUInt i=0;i<rdxSDomainGUID::GUID_SIZE;i++)
		{
			rdxUInt8 gbyte = domainBytes[i];
			pathBuffer[bufEnd++] = nibbles[(gbyte & 0xf0) >> 4];
			pathBuffer[bufEnd++] = nibbles[gbyte & 0xf];
		}
		pathBuffer[bufEnd++] = '_';
		for(rdxLargeUInt i=0;i<rdxSObjectGUID::GUID_SIZE;i++)
		{
			rdxUInt8 gbyte = guidBytes[i];
			pathBuffer[bufEnd++] = nibbles[(gbyte & 0xf0) >> 4];
			pathBuffer[bufEnd++] = nibbles[gbyte & 0xf];
		}
		pathBuffer[bufEnd] = '\0';
		strcat(pathBuffer, ".rdxil");

		FILE *f = fopen(pathBuffer, "wb");
		if(f)
		{
			rdxExportMethod(objm, f, obj.StaticCast<rdxCMethod>());
			fclose(f);
		}
	}
	alloc->Free(pathBuffer);
}
