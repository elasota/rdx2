local ResolveTypeDef
local CodedTypeValue

local tempID

-- Resumable flags

local resumeOffsettingInstructions =
{
	debuginfo = true,
}

local nextMarkingInstructions =
{
	call = true,
	calldelegatebp = true,
	calldelegateprv = true,
	callvirtual = true,
	callinterface = true,
}

--[[
local currentMarkingInstructions =
{
	call = true,
	calldelegateprv = true,
	calldelegatebp = true,
	callvirtual = true,
	callinterface = true,
	newinstance = true,
}
]]--

local instrnumMarkingInstructions =
{
	jump = true,
	jtrue = true,
	jfalse = true,
	jnulli = true,
	jnullo = true,
	jnotnulli = true,
	jnotnullo = true,
	jinherits = true,
	jeq_f = true,
	jeq_p = true,
	jne_f = true,
	jne_p = true,
}

local srcMarkingInstructions =
{
	ilt = true,
	igt = true,
	ile = true,
	ige = true,
	ieq = true,
	ine = true,

	flt = true,
	fgt = true,
	fle = true,
	fge = true,
	feq = true,
	fne = true,
}

local markingBranchIntrinsics =
{
	jccf = true,
	jccp = true,
}

local caseMarkingInstructions =
{
	switch = true,
	switch_ptr = true,
}

local exitInstrMarkingInstructions =
{
	iteratearray = true,
	iteratearraysub = true,
}

-- Emission instruction sets
local castInstructions =
{
	["isx"] = true,
	["izx"] = true,
	["ftoi"] = true,
	["iutof"] = true,
	["itof"] = true,
}

local binaryArithInstructions =
{
	["iadd"] = "+",
	["isub"] = "-",
	["imul"] = "*",

	["fadd"] = "+",
	["fsub"] = "-",
	["fmul"] = "*",
	["fdiv"] = "/",
}

local checkedBinaryArithInstructions =
{
	["idiv"] = true,
	["imod"] = true,
	["iudiv"] = true,
	["iumod"] = true,
}

local unaryArithInstructions =
{
	["fneg"] = "-",
	["ineg"] = "-",
}

local simpleCmpInstructions =
{
	["ilt"] = "<",
	["igt"] = ">",
	["ile"] = "<=",
	["ige"] = ">=",
	["ieq"] = "==",
	["ine"] = "!=",

	["iult"] = "<",
	["iugt"] = ">",
	["iule"] = "<=",
	["iuge"] = ">=",
	
	["flt"] = "<",
	["fgt"] = ">",
	["fle"] = "<=",
	["fge"] = ">=",
	["feq"] = "==",
	["fne"] = "!=",
}

local CodedTypeProperty = function(cs, guidToSymbol, typeDef, propIndex)
	if type(typeDef) == "string" then
		local vType = cs.gst[guidToSymbol[typeDef]]
		if vType.type == "CStructuredType" then
			if vType.declType == "struct" or vType.declType == "class" then
				return vType.properties[tonumber(propIndex) + 1]
			end
		end
	end
	assert(false)
end

local CodedTypeIsObjRef = function(cs, guidToSymbol, typeDef)
	if type(typeDef) == "table" then
		if typeDef.specialType == "arrayDef" then
			return true
		elseif typeDef.specialType == "null" then
			return true
		else
			assert(false)
		end
	elseif type(typeDef) == "string" then
		local vType = cs.gst[guidToSymbol[typeDef]]

		if vType.type == "CStructuredType" then
			if vType.declType == "enum" then
				return false
			elseif vType.declType == "interface" then
				return true
			elseif vType.declType == "struct" then
				return false
			elseif vType.declType == "class" then
				return true
			else
				assert(false)
			end
		elseif vType.type == "CStaticDelegateType" then
			return true
		elseif vType.type == "CBoundDelegateType" then
			return true
		else
			assert(false)
		end
	else
		assert(false)
	end
	assert(false)
end

local CodedTypeIsRefStruct = function(cs, guidToSymbol, typeDef)
	if type(typeDef) == "string" then
		return VTypeIsRefStruct(cs.gst[guidToSymbol[typeDef]])
	end
	return false
end

local CodedTypeRefHeader

CodedTypeRefHeader = function(cs, guidToSymbol, typeDef)
	if type(typeDef) == "table" then
		if typeDef.specialType == "arrayDef" then
			return CodedTypeRefHeader(cs, guidToSymbol, typeDef.contentsType)
		elseif typeDef.specialType == "null" then
			return nil
		else
			assert(false)
		end
	elseif type(typeDef) == "string" then
		local vType = cs.gst[guidToSymbol[typeDef]]

		if vType.type == "CStructuredType" then
			local declType = vType.declType
			if declType == "enum" then
				return nil
			elseif declType == "interface" or declType == "struct" or declType == "class" then
				DetermineObjectCPPPath(cs, vType)
				return assert(vType.cpp.headerPath)
			else
				assert(false)
			end
		elseif vType.type == "CStaticDelegateType" then
			return nil
		else
			assert(false)
		end
	else
		assert(false)
	end
	assert(false)
end

CodedTypeValue = function(cs, guidToSymbol, typeDef, isRTP, isVarying, form)
	if isVarying then
		return "::rdxSVarying"
	end

	if isRTP then
		if type(typeDef) == "table" and typeDef.specialType == "null" then
			return "::rdxWeakTypelessOffsetRTRef"
		end
		return "rdxWeakOffsetRTRef("..CodedTypeValue(cs, guidToSymbol, typeDef, false, false, "TracedRTRef")..")"
	end

	if type(typeDef) == "table" then
		if typeDef.specialType == "arrayDef" then
			local contentsType = CodedTypeValue(cs, guidToSymbol, typeDef.contentsType, false, false, "TracedRTRef")
			if form == "TracedRTRef" then
				return "rdxTracedArrayRTRef("..contentsType..")"
			elseif form == "WeakRTRef" then
				return "rdxWeakArrayRTRef("..contentsType..")"
			elseif form == "InternalClass" then
				return "::rdxCArray<"..contentsType.."> "
			end
			assert(false, "Bad form")
		elseif typeDef.specialType == "null" then
			if form == "TracedRTRef" then
				return "rdxTracedRTRef(::rdxCObject)"
			elseif form == "WeakRTRef" then
				return "rdxWeakRTRef(::rdxCObject)"
			elseif form == "InternalClass" then
				return "::rdxCObject"
			else
				assert(false)
			end
		else
			assert(false)
		end
	elseif type(typeDef) == "string" then
		local vType = cs.gst[guidToSymbol[typeDef]]

		if vType.type == "CStructuredType" then
			if vType.declType == "enum" then
				return "::rdxSRuntimeEnum"
			elseif vType.declType == "interface" then
				if form == "TracedRTRef" then
					return "rdxTracedIRef("..vType.cpp.classname..")"
				elseif form == "WeakRTRef" then
					return "rdxWeakIRef("..vType.cpp.classname..")"
				elseif form == "InternalClass" then
					return vType.cpp.classname
				else
					assert(false)
				end
			elseif vType.declType == "struct" then
				DetermineObjectName(cs, vType)
				return assert(vType.cpp.classname)
			elseif vType.declType == "class" then
				DetermineObjectName(cs, vType)
				if form == "TracedRTRef" then
					return assert("rdxTracedRTRef("..vType.cpp.classname..")")
				elseif form == "WeakRTRef" then
					return assert("rdxWeakRTRef("..vType.cpp.classname..")")
				elseif form == "InternalClass" then
					return vType.cpp.classname
				end
			else
				assert(false)
			end
			print("WTF3")
		elseif vType.type == "CStaticDelegateType" then
			if form == "TracedRTRef" then
				return "rdxTracedRTRef(::rdxCMethod)"
			elseif form == "WeakRTRef" then
				return "rdxWeakRTRef(::rdxCMethod)"
			else
				assert(false)
			end
		else
			assert(false)
		end
		print("WTF1")
	else
		print(type(typeDef))
		assert(false)
	end
	print("FELL THROUGH (???)")
	assert(false)
end


local CodedTypeStackStorage = function(cs, guidToSymbol, typeDef)
	if type(typeDef) == "table" then
		if typeDef.specialType == "arrayDef" then
			return "rdxTracedRTRef(::rdxCArrayContainer)"
		elseif typeDef.specialType == "null" then
			return "rdxTracedRTRef(::rdxCObject)"
		end
	elseif type(typeDef) == "string" then
		local vType = cs.gst[guidToSymbol[typeDef]]

		if vType.type == "CStructuredType" then
			if vType.declType == "enum" then
				return "::rdxSRuntimeEnum"
			elseif vType.declType == "interface" then
				return "rdxTracedIRef(rdxSObjectInterfaceImplementation)"
			elseif vType.declType == "struct" then
				DetermineObjectName(cs, vType)
				return assert(vType.cpp.classname)
			elseif vType.declType == "class" then
				return "::rdxTracedRTRef(rdxCObject)"
			end
		elseif vType.type == "CStaticDelegateType" then
			return "::rdxTracedRTRef(rdxCMethod)"
		elseif vType.type == "CBoundDelegateType" then
			return "::rdxTracedRTRef(rdxCObject)"
		end
	else
		print(typeDef)
	end
	assert(false)
end


setglobal("CompilePCCM", function(cs, obj, guidToSymbol)
	print("Compiling PCCM for "..obj.longName)
	RDXC.Native.mkdir(cppConfig.exportPath.."/RDXPCCM")
	local methodGUID = ComputeSymbolGUID(obj.longName)
	print("Midcode: "..methodGUID.." = symbol "..guidToSymbol[methodGUID])
	local rdxilScript = loadcacheablefile(cppConfig.midcodePath.."/method_"..ComputeSymbolGUID(obj.longName)..".rdxil")

	local symDomain, symObjName = splitResName(obj.longName)
	local symDomainGUID = RDXC.Native.computeguid(symDomain)
	local symObjNameGUID = RDXC.Native.computeguid(symObjName)

	assert(rdxilScript)
	
	local stackActions, instructions, markupInstructions = rdxilScript()
	local methodFilePath = cppConfig.exportPath.."/RDXPCCM/method_"..methodGUID..".cpp"

	local f = io.open(methodFilePath, "wb")

	local headerPathSet = { }
	for _,sa in ipairs(stackActions) do
		if sa.type then
			local headerPath = CodedTypeRefHeader(cs, guidToSymbol, sa.type)
			if headerPath ~= nil then
				headerPathSet[headerPath] = true
			end
		end
	end

	print("guid: "..methodGUID)
	tempID = methodGUID

	f:write("template<>\n")
	f:write("void _RDX_CPPX::PCCMGlue::Call<0x"..symDomainGUID.."ULL, 0x"..symObjNameGUID.."ULL>(rdxPCCMCallEnvironment &pccmEnvironment)\n")
	f:write("{\n")
	f:write("\trdxUInt8 *prvBytes = static_cast<rdxUInt8*>(pccmEnvironment.GetPRV());\n")
	f:write("\trdxUInt8 *bpBytes = static_cast<rdxUInt8*>(pccmEnvironment.GetBP());\n")
	f:write("\tconst rdxCMethod *callMethod = RDX_CNULL;\n")
	f:write("\tconst rdxCObject *callObjSrc;\n")
	f:write("\trdxLargeUInt callVFTIndex;\n")
	f:write("\trdxLargeUInt callMethodRes;\n")
	f:write("\trdxLargeInt callPRVBaseOffset;\n")
	f:write("\trdxLargeInt callStackBaseOffset;\n")
	f:write("\trdxLargeUInt callInstr;\n")
	f:write("\tvoid *callIfcStackLoc;\n")
	f:write("\tgoto func_entry;\n")
	f:write("call_call:\n")
	f:write("\t{\n")
	f:write("\t\tcallMethod = pccmEnvironment.GetMethodRes(callMethodRes);\n")
	f:write("\t\tgoto call_enter;\n")
	f:write("\t}\n")
	f:write("call_callvirtual:\n")
	f:write("\t{\n")
	f:write("\t\tcallMethod = pccmEnvironment.GetVirtualMethod(callObjSrc, callVFTIndex);\n")
	f:write("\t\tgoto call_enter;\n")
	f:write("\t}\n")
	f:write("call_callinterface:\n")
	f:write("\t{\n")
	f:write("\t\tcallMethod = pccmEnvironment.GetInterfaceCallMethod(callIfcStackLoc, callVFTIndex);\n")
	-- NOTE: Fallthrough
	f:write("\t}\n")
	f:write("\tcall_enter:\n")
	f:write("\t{\n")
	f:write("\t\tpccmEnvironment.EnterMethod(callMethod, RDX_PCCM_BPSTACK(callPRVBaseOffset), RDX_PCCM_BPSTACK(callStackBaseOffset), callInstr);\n")
	f:write("\t\treturn;\n")
	f:write("\t}\n")
	
	local vRegs = { }
	local vRegList = { }
	do
		f:write("\tenum ValueStorageLocation\n")
		f:write("\t{\n")
		f:write("\t\tVSL_BASE = 0,\n")
		local vRegStackRoot = { regID = "VSL_BASE" }
		local vRegStack = vRegStackRoot
		local lastRVReg = "VSL_BASE"
		local transitionedBP = false
		local balance = 0

		local virtualRegisters = { }
		for index,sa in ipairs(stackActions) do
			local regID = "VSL_"..(index - 1)
			local actionType = sa.actionType
			if actionType == "pop" then
				balance = balance - 1
				vRegStack = vRegStack.next
			else
				local prevReg = vRegStack.regID
				if actionType == "pushParentFrameReturnValue" or actionType == "pushParentFrameParameter" then
					vRegStack = { regID = regID, next = vRegStack, isAlive = true, type = sa.type, isPRV = true, isPinned = true, isAlive = true }
					vRegList[#vRegList+1] = vRegStack
					if CodedTypeIsRefStruct(cs, guidToSymbol, sa.type) then
						vRegStack.isRTP = true
						f:write("\t\tRDX_PCCM_VSL_CHAINOP("..regID..", "..prevReg..", ::rdxWeakTypelessOffsetRTRef),\n")
					else
						f:write("\t\tRDX_PCCM_VSL_CHAINOP("..regID..", "..prevReg..", "..CodedTypeStackStorage(cs, guidToSymbol, sa.type).."),\n")
					end
					if actionType == "pushParentFrameReturnValue" then
						lastRVReg = regID
					end
				else
					balance = balance + 1
					-- BP-based stack values
					if not transitionedBP then
						transitionedBP = true
						vRegStack = vRegStackRoot
					end

					local prevReg = vRegStack.regID
					vRegStack = { regID = regID, next = vRegStack, type = sa.type }
					vRegList[#vRegList+1] = vRegStack
					
					if actionType == "pushLocal" then
						f:write("\t\tRDX_PCCM_VSL_CHAINLOCAL("..regID..", "..prevReg..", "..CodedTypeStackStorage(cs, guidToSymbol, sa.type).."),\n")
						vRegStack.isLocal = true
					else
						if actionType == "pushOpstackValue" then
							vRegStack.isOp = true
							assert(regID)
							assert(prevReg)
							assert(CodedTypeStackStorage(cs, guidToSymbol, sa.type))
							f:write("\t\tRDX_PCCM_VSL_CHAINOP("..regID..", "..prevReg..", "..CodedTypeStackStorage(cs, guidToSymbol, sa.type).."),\n")
						elseif actionType == "pushOpstackPtr" then
							vRegStack.isOp = true
							vRegStack.isRTP = true
							f:write("\t\tRDX_PCCM_VSL_CHAINOP("..regID..", "..prevReg..", ::rdxWeakTypelessOffsetRTRef),\n")
						elseif actionType == "pushOpstackVarying" then
							vRegStack.isOp = true
							vRegStack.isVarying = true
							f:write("\t\tRDX_PCCM_VSL_CHAINOP("..regID..", "..prevReg..", ::rdxSVarying),\n")
						end
					end
				end
			end

			vRegs[index - 1] = vRegStack
		end
		assert(balance == 0)
		f:write("\t\tVSL_RV = "..lastRVReg..",\n")
		f:write("};\n")
	end
	
	-- Mark instructions
	for iNum,instr in ipairs(instructions) do
		if instr.opcode == "tick" and cppConfig.markTicks then
			instructions[iNum+1].isResumable = true
		end
		if nextMarkingInstructions[instr.opcode] then
			if instructions[iNum+1] then
				instructions[iNum+1].isResumable = true
			end
		end
		if instrnumMarkingInstructions[instr.opcode] then
			instructions[tonumber(instr.instrnum)+1].isResumable = true
		end
		if srcMarkingInstructions[instr.opcode] then
			instructions[tonumber(instr.p0)+1].isResumable = true
		end
		if exitInstrMarkingInstructions[instr.opcode] then
			print(instr.opcode)
			instructions[tonumber(instr.exitInstr)+1].isResumable = true
		end
		if caseMarkingInstructions[instr.opcode] then
			for c=1,instr.numCases+1 do						-- +1 for the fallthrough case
				instructions[iNum + c].isResumable = true
			end
		end
		if markingBranchIntrinsics[instr.opcode] or simpleCmpInstructions[instr.opcode] then
			instructions[tonumber(instr.p0)+1].isResumable = true
		end
	end

	-- Entry point instruction is always resumable
	instructions[1].isResumable = true

	for iNum,instr in ipairs(instructions) do
		if resumeOffsettingInstructions[instr.opcode] then
			instr.nextResumable = instr.isResumable
		end

		if instr.nextResumable then
			instructions[iNum+1].isResumable = true
		end
	end

	for iNum,instr in ipairs(instructions) do
		if instr.opcode == "pinl" then
			vRegs[tonumber(instr.src)].isPinned = true
			print("Pinned: "..instr.src)
		end
	end

	local VRegStackLoc = function(vReg)
		if vReg.isPRV then
			return "RDX_PCCM_PRVSTACK("..vReg.regID.." - VSL_RV)"
		else
			return "RDX_PCCM_BPSTACK("..vReg.regID..")"
		end
	end

	local VRegAddr
	
	local VRegValue = function(vReg, isConst)
		if type(vReg.type) == "table" and vReg.type.specialType == "null" and not vReg.isRTP then
			assert(not vReg.isPinned)
			return "/*reg_"..vReg.regID.."*/"
		end

		if vReg.isPinned then
			return "(*"..VRegAddr(vReg, isConst)..")"
		else
			return "reg_"..vReg.regID
		end
	end

	local VRegStackAddr = function(vReg, isConst)
		local stackType = CodedTypeValue(cs, guidToSymbol, vReg.type, vReg.isRTP, vReg.isVarying, "TracedRTRef")
		if methodGUID == "2c6b409c2c6b409c_01ea9e0cc993bc5d" and stackType == "::_RDX_CPPX::NCore::NCollections::NTp_4ee71f6e::CEnumerator" then
			print("Type: "..tostring(vReg.type))
			print("isRTP: "..tostring(vReg.isRTP))
			print("isVarying: "..tostring(vReg.isVarying))
			print("regID: "..tostring(vReg.regID))
			assert(false)
		end
		
		return "(static_cast<"..stackType.." *"..(isConst and " const" or "")..">("..VRegStackLoc(vReg).."))"
	end

	VRegAddr = function(vReg, isConst)
		if vReg.isPinned then
			return VRegStackAddr(vReg, isConst)
		else
			return "(&"..VRegValue(vReg, isConst)..")"
		end
	end

	local CreateVRegLocal = function(vReg)
		assert(type(vReg) == "table")
		if not vReg.isPinned and not vReg.isLocalCreated then
			if type(vReg.type) == "table" and vReg.type.specialType == "null" and not vReg.isRTP then
				-- Don't create null locals
			else
				local regType = CodedTypeValue(cs, guidToSymbol, vReg.type, vReg.isRTP, vReg.isVarying, "WeakRTRef")
				f:write("\t"..regType.." reg_"..vReg.regID..";\n")
			end
		end
		vReg.isLocalCreated = true
	end

	local LoadVReg = function(vReg)
		assert(type(vReg) == "table")
		CreateVRegLocal(vReg)
		if not vReg.isLoaded then
			if not vReg.isPinned then
				if type(vReg.type) == "table" and vReg.type.specialType == "null" and not vReg.isRTP then
					-- TODO: There are some rare cases where a null gets flushed to stack and reloaded
					-- It might be better to try avoiding the reload entirely
				else
					local regType = CodedTypeValue(cs, guidToSymbol, vReg.type, vReg.isRTP, vReg.isVarying, "WeakRTRef")
					local stackType = CodedTypeValue(cs, guidToSymbol, vReg.type, vReg.isRTP, vReg.isVarying, "TracedRTRef")
					f:write("\treg_"..vReg.regID.." = *"..VRegStackAddr(vReg, true)..";\n")
				end
			end
			vReg.isLoaded = true
		end
	end

	local OpenVReg = function(vReg)
		assert(type(vReg) == "table")
		assert(not vReg.isAlive, "VReg is already alive: "..vReg.regID)
		vReg.isAlive = true
	end

	local DestroyVReg = function(vReg)
		assert(type(vReg) == "table")
		assert(not vReg.isPRV)
		vReg.isDirty = false
		vReg.isLoaded = false
		vReg.isAlive = false
	end

	local WriteVReg = function(vReg)
		assert(type(vReg) == "table")
		assert(vReg.isAlive)
		CreateVRegLocal(vReg)
		vReg.isLoaded = true
		vReg.isDirty = true
	end

	local VRegForIndex = function(index)
		local rv = vRegs[tonumber(index)]
		if rv == nil then
			assert(false, "Failed to find index: "..tostring(index))
		end
		return rv
	end
	
	local currentMOPIndex = 0
	local currentMOP

	local FindMOPPropertyTranslation = function(iNum, reg)
		local scanMOP = currentMOP
		local scanMOPIndex = currentMOPIndex
		while scanMOP ~= nil and tonumber(scanMOP.linkInstr) == iNum do
			if scanMOP.opcode == "incrprop" or scanMOP.opcode == "moveprop" then
				if reg == scanMOP.src then
					reg = assert(scanMOP.dest)
				end
			end
			scanMOPIndex = scanMOPIndex + 1
			scanMOP = markupInstructions[scanMOPIndex + 1]
		end
		return reg
	end

	local DischargeIncrProps = function(iNum)
		while currentMOP ~= nil and (currentMOP.opcode == "incrprop" or currentMOP.opcode == "moveprop") and tonumber(currentMOP.linkInstr) == iNum do
			local headerPath = CodedTypeRefHeader(cs, guidToSymbol, currentMOP.type)
			headerPathSet[headerPath] = true
			local prop = CodedTypeProperty(cs, guidToSymbol, currentMOP.type, currentMOP.propIndex)
			f:write(".")
			f:write(CPPUnreserve(prop.name))
			currentMOPIndex = currentMOPIndex + 1
			currentMOP = markupInstructions[currentMOPIndex + 1]
		end
	end

	local EmitPassiveConversions = function(cs, guidToSymbol, srcVReg, destType)
		if type(srcVReg.type) == "table" and srcVReg.type.specialType == "null" and not srcVReg.isRTP then
			--local isInterface = false
			--if type(destType) == "string" then
			--	local vType = cs.gst[guidToSymbol[destType]]
			--	if vType.type == "CStructuredType" and vType.declType == "interface" then
			--		isInterface = true
			--	end
			--end
			--if isInterface then
			--	f:write(".ToWeakRTRef().StaticCast<::rdxCObject>()")
			--else
			--	local headerPath = CodedTypeRefHeader(cs, guidToSymbol, destType)
			--	headerPathSet[headerPath] = true
			--	f:write(".ToWeakRTRef().StaticCast<"..CodedTypeValue(cs, guidToSymbol, destType, false, false, "InternalClass")..">()")
			--end
			local nullDestType = CodedTypeValue(cs, guidToSymbol, destType, false, false, "WeakRTRef")
			f:write(nullDestType.."::Null()")
		elseif type(srcVReg.type) == "string" then
			if srcVReg.type ~= destType then
				local srcType = cs.gst[guidToSymbol[srcVReg.type]]
				if srcType.type == "CStructuredType" and srcType.declType == "enum" then
					f:write(".Value()")
				end
			end
		end
	end

	local FlushStack = function()
		f:write("\t// Flushing\n")
		for _,vReg in ipairs(vRegList) do
			if vReg.isDirty then
				if not vReg.isPinned then
					if vReg.markRTPOnFlush then
						f:write("\trdxPCCMUtils::MarkRTP(reg_"..vReg.regID..");\n")
					end
					if type(vReg.type) == "table" and vReg.type.specialType == "null" and not vReg.isRTP then
						f:write("\tmemset("..VRegStackAddr(vReg, false)..", 0, sizeof(rdxTracedRTRef(rdxCObject)));\n")
					else
						f:write("\t*"..VRegStackAddr(vReg, false).." = reg_"..vReg.regID..";\n")
					end
				end
				vReg.isDirty = false
			end
		end
	end

	local UnloadRegs = function()
		for _,vReg in ipairs(vRegList) do
			if not vReg.isPRV then
				vReg.isLocalCreated = false
				vReg.isLoaded = false
			end
		end
	end

	f:write("\tdo {\n")

	local DischargeMOP = function(iNum)
		while currentMOP ~= nil and tonumber(currentMOP.linkInstr) == iNum do
			local mopcode = currentMOP.opcode
			if mopcode == "addshell" then
			elseif mopcode == "pop" then
				DestroyVReg(VRegForIndex(currentMOP.loc))
			elseif mopcode == "unshell" then
				OpenVReg(VRegForIndex(currentMOP.loc))
			elseif mopcode == "movesa" then
				local srcVReg = VRegForIndex(currentMOP.src)
				local destVReg = VRegForIndex(currentMOP.dest)
				OpenVReg(destVReg)
				WriteVReg(destVReg)
				LoadVReg(srcVReg)
				DestroyVReg(srcVReg)
				f:write("\t"..VRegValue(destVReg).." = "..VRegValue(srcVReg)..";\n")
			else
				-- Undigested MOP
				assert(false, "Unknown out-of-band MOP: "..mopcode)
			end
			currentMOPIndex = currentMOPIndex + 1
			currentMOP = markupInstructions[currentMOPIndex + 1]
		end
	end
	
	-- Write instructions
	currentMOP = markupInstructions[1]
	DischargeMOP(0)
	for iNum,instr in ipairs(instructions) do
		if instr.isResumable then
			FlushStack()
			UnloadRegs()
			f:write("} while(false);\n")
			f:write("instr_"..(iNum - 1)..": do {\n")
		end
		currentMOP = markupInstructions[currentMOPIndex + 1]
		f:write("\t// op: "..instr.opcode.."\n")
		local opcode = instr.opcode
		if opcode == "debuginfo" then
			f:write("\t// "..instr.filename.." ["..instr.line.."]\n")
		elseif opcode == "move" then
			local moveDest
			local moveSrc
			local srcVReg = VRegForIndex(instr.src)
			local destVReg = VRegForIndex(instr.dest)

			if instr.flags.destOpstack and not instr.flags.destDestroy and not instr.flags.destDeref then
				OpenVReg(destVReg)
			elseif instr.flags.destInit then
				OpenVReg(destVReg)
			end

			WriteVReg(VRegForIndex(instr.dest))

			local destVReg = VRegForIndex(instr.dest)
			-- Special case for newinstanceset emits
			if instr.flags.destDeref and destVReg.isRTP and type(destVReg.type) == "table" and destVReg.type.specialType == "null" then
				local copyType = CodedTypeValue(cs, guidToSymbol, instr.type, false, false, "TracedRTRef")
				moveDest = "(*(static_cast<"..copyType.." * const>("..VRegValue(destVReg, false)..".Modify())))"
			else
				moveDest = VRegValue(destVReg, false)
				if instr.flags.destDeref then
					moveDest = "(*"..moveDest..".Modify())"
				end
			end

			-- TODO: Property MOP
			moveSrc =  VRegValue(VRegForIndex(instr.src), true)
			LoadVReg(srcVReg)

			if instr.flags.srcTransient then
				DestroyVReg(VRegForIndex(instr.src))
			end

			if instr.flags.destTransient then
				DestroyVReg(VRegForIndex(instr.dest))
			end

			if instr.flags.srcDeref then
				moveSrc = "(*"..moveSrc..".Data())"
			end

			f:write("\t"..moveDest)

			DischargeIncrProps(iNum)
			f:write(" = "..moveSrc)

			EmitPassiveConversions(cs, guidToSymbol, srcVReg, instr.type)
			f:write(";\n")
		elseif opcode == "pushdefault" then
			assert(false)
		elseif opcode == "clone" then
			OpenVReg(VRegForIndex(instr.dest))
			LoadVReg(VRegForIndex(instr.src))
			WriteVReg(VRegForIndex(instr.dest))
			f:write("\treg_"..vRegs[tonumber(instr.dest)].regID.." = reg_"..vRegs[tonumber(instr.src)].regID..";\n")
		elseif opcode == "tovarying" then
			assert(false)
		elseif opcode == "pinl" then
			local destVReg = VRegForIndex(FindMOPPropertyTranslation(iNum, instr.dest))
			local destType = CodedTypeValue(cs, guidToSymbol, destVReg.type, destVReg.isRTP, destVReg.isVarying, "WeakRTRef")
			OpenVReg(destVReg)
			WriteVReg(destVReg)
			f:write("\t\t"..VRegValue(destVReg, false).." = "..destType.."(rdxObjRef_CSignal_DataPointer, pccmEnvironment.GetStackRoot(), &("..VRegValue(VRegForIndex(instr.src), false))
			DischargeIncrProps(iNum)
			f:write("));\n")
		elseif opcode == "ptrprop" then
			local prop = CodedTypeProperty(cs, guidToSymbol, instr.type, instr.propIndex)
			local destVReg = VRegForIndex(FindMOPPropertyTranslation(iNum, instr.loc))
			local destType = CodedTypeValue(cs, guidToSymbol, destVReg.type, destVReg.isRTP, destVReg.isVarying, "WeakRTRef")
			local srcVReg = VRegForIndex(instr.oldSA)
			LoadVReg(srcVReg)
			DestroyVReg(srcVReg)
			OpenVReg(destVReg)
			WriteVReg(destVReg)
			f:write("\t"..VRegValue(destVReg, false).." = "..destType.."(rdxObjRef_CSignal_DataPointer, "..VRegValue(VRegForIndex(instr.oldSA), false)..".GetObjRef()->Modify(), &"..VRegValue(srcVReg, false).."->"..CPPUnreserve(prop.name))
			DischargeIncrProps(iNum)
			f:write(");\n")
		elseif opcode == "changeproperty" then
			local vreg = VRegForIndex(instr.loc)
			local stType = CodedTypeValue(cs, guidToSymbol, instr.type, false, false, "InternalClass")
			local oldProp = CodedTypeProperty(cs, guidToSymbol, instr.type, instr.oldProperty)
			local newProp = CodedTypeProperty(cs, guidToSymbol, instr.type, instr.newProperty)
			f:write("\trdxPCCMUtils::ChangePtrProperty("..VRegValue(vreg, false)..", &"..stType.."::"..CPPUnreserve(oldProp.name)..", &"..stType.."::"..CPPUnreserve(newProp.name)..");\n")
		elseif opcode == "objproperty" or opcode == "objproperty_notnull" or opcode == "objproperty_notnull_persist" then
			local prop = CodedTypeProperty(cs, guidToSymbol, instr.type, instr.property)
			local destVReg = VRegForIndex(FindMOPPropertyTranslation(iNum, instr.dest))
			local destType = CodedTypeValue(cs, guidToSymbol, destVReg.type, destVReg.isRTP, destVReg.isVarying, "WeakRTRef")
			local srcVReg = VRegForIndex(instr.src)
			local srcInternalType = CodedTypeValue(cs, guidToSymbol, srcVReg.type, srcVReg.isRTP, srcVReg.isVarying, "InternalClass")
			LoadVReg(VRegForIndex(instr.src))
			if opcode ~= "objproperty_notnull_persist" then
				DestroyVReg(VRegForIndex(instr.src))
			end
			OpenVReg(destVReg)
			WriteVReg(destVReg)
			f:write("\t{\n")
			f:write("\t\t"..srcInternalType.." * objRef = "..VRegValue(VRegForIndex(instr.src), false)..".Modify();\n")
			f:write("\t\t"..VRegValue(destVReg, false).." = "..destType.."(rdxObjRef_CSignal_DataPointer, objRef, &objRef->"..CPPUnreserve(prop.name))
			DischargeIncrProps(iNum)
			f:write(");\n\t}\n")
			destVReg.markRTPOnFlush = true
		elseif opcode == "immediate" then
			OpenVReg(VRegForIndex(instr.dest))
			WriteVReg(VRegForIndex(instr.dest))
			f:write("\trdxPCCMSetImmediate("..VRegValue(VRegForIndex(instr.dest)))
			for _,v in ipairs(instr.value) do
				f:write(", ")
				f:write(v)
			end
			f:write(");\n")
		elseif opcode == "immediate_ptr" then
			local destVReg = VRegForIndex(instr.dest)
			OpenVReg(destVReg)
			WriteVReg(destVReg)
			local destInternalType = CodedTypeValue(cs, guidToSymbol, destVReg.type, destVReg.isRTP, destVReg.isVarying, "InternalClass")
			f:write("\t"..VRegValue(destVReg).." = pccmEnvironment.ImmediateResObj("..instr.res..").StaticCast<"..destInternalType..">();\n")
		elseif opcode == "immediate_rtp" then
			local destVReg = VRegForIndex(instr.dest)
			OpenVReg(destVReg)
			WriteVReg(destVReg)
			f:write("\t\tpccmEnvironment.ImmediateRTP("..VRegValue(destVReg)..", "..instr.res..");\n")
		elseif opcode == "arrayindex" then
			local indexRegs = { }
			local vRegChain = { }
			local numDimensions = tonumber(instr.numdimensions)
			local arrayVReg = VRegForIndex(instr.arraysrc)
			local destVReg = VRegForIndex(FindMOPPropertyTranslation(iNum, instr.dest))
			local destType = CodedTypeValue(cs, guidToSymbol, destVReg.type, destVReg.isRTP, destVReg.isVarying, "WeakRTRef")
			do
				local indexStack = VRegForIndex(instr.indexsrc)
				for i=1,numDimensions do
					LoadVReg(indexStack)
					vRegChain[numDimensions + 1 - i] = indexStack
					indexStack = indexStack.next
				end
			end
			LoadVReg(arrayVReg)
			for i=1,numDimensions do
				DestroyVReg(vRegChain[i])
			end
			DestroyVReg(arrayVReg)
			FlushStack()
			OpenVReg(destVReg)
			WriteVReg(destVReg)

			local arrayInternalType = CodedTypeValue(cs, guidToSymbol, arrayVReg.type, arrayVReg.isRTP, arrayVReg.isVarying, "InternalClass")
			
			f:write("\t{\n")
			f:write("\t\t"..arrayInternalType.." * arrayRef = "..VRegValue(arrayVReg, true)..".Modify();\n")
			f:write("\t\tif(arrayRef == RDX_CNULL)\n")
			f:write("\t\t\tpccmEnvironment.SetStandardException(rdxX_NullReferenceException, "..(iNum - 1)..");\n")
			
			
			for i=1,numDimensions do
				f:write("\t\t::rdxLargeUInt arrayDim_"..(i - 1).." = "..VRegValue(vRegChain[i])..";\n")
			end
			
			f:write("\t\tif(")
			for i=1,numDimensions do
				if i ~= 1 then
					f:write(" || ")
				end
				f:write("arrayDim_"..(i - 1).." >= arrayRef->")
				if numDimensions == 1 then
					f:write("NumElements()")
				else
					f:write("Dimension("..(i - 1)..")")
				end
			end
			f:write(")\n")
			f:write("\t\t\tpccmEnvironment.SetStandardException(rdxX_IndexOutOfBoundsException, "..(iNum - 1)..");\n")
			f:write("\t\t::rdxLargeUInt arrayIndex = arrayDim_0;\n")
			for i=2,numDimensions do
				f:write("arrayIndex = arrayIndex * arrayRef->Dimension("..(i - 2).." + arrayDim_"..(i - 1)..";\n")
			end
			
			f:write("\t\t"..VRegValue(destVReg, false).." = "..destType.."(rdxObjRef_CSignal_DataPointer, arrayRef, &(arrayRef->Element(arrayIndex)")
			DischargeIncrProps(iNum)
			f:write("));\n")
			f:write("\t}\n")
			destVReg.markRTPOnFlush = true
		elseif opcode == "call" or opcode == "calldelegatebp" or opcode == "calldelegateprv" or opcode == "callvirtual" or opcode == "callinterface" then
			FlushStack()
			f:write("\t{\n")
			
			f:write("\t\tcallInstr = "..(iNum - 1)..";\n")
			if opcode == "call" then
				f:write("\t\tcallMethodRes = "..instr.method..";\n")
			elseif opcode == "callvirtual" then
				LoadVReg(VRegForIndex(instr.objsrc))
				f:write("\t\tcallObjSrc = "..VRegValue(VRegForIndex(instr.objsrc), true)..".Data();\n")
				f:write("\t\tcallVFTIndex = "..instr.vftindex..";\n")
			elseif opcode == "callinterface" then
				-- No load because callinterface needs to deinterface the parameter
				f:write("\t\tcallIfcStackLoc = "..VRegStackLoc(VRegForIndex(instr.objsrc))..";\n")
				f:write("\t\tcallVFTIndex = "..instr.vftindex..";\n")
			else
				-- TODO MUSTFIX: implement calldelegate
				assert(false, "Unimplemented call: "..opcode)
			end

			local prvVReg = VRegForIndex(instr.parambase)
			for i=1,tonumber(instr.numparams) do
				local nextVReg = prvVReg.next
				DestroyVReg(prvVReg)
				prvVReg = nextVReg
			end
			local paramBaseVReg = VRegForIndex(instr.parambase)
			f:write("\t\tcallPRVBaseOffset = (static_cast<rdxLargeInt>("..prvVReg.regID.." + static_cast<rdxLargeInt>(rdxALIGN_RuntimeStackValue) - 1) & ~(static_cast<rdxLargeInt>(rdxALIGN_RuntimeStackValue) - 1));\n")
			f:write("\t\tcallStackBaseOffset = "..paramBaseVReg.regID..";\n")
			f:write("\t\tgoto call_"..opcode..";\n")
			f:write("\t}\n")
		elseif opcode == "verifynotnull" then
			LoadVReg(VRegForIndex(instr.loc))
			f:write("\tif("..VRegValue(VRegForIndex(instr.loc), true)..".IsNull())\n")
			f:write("\t{\n")
			f:write("\t\tpccmEnvironment.SetStandardException(rdxX_NullReferenceException, "..(iNum - 1 + tonumber(instr.instroffset))..");\n")
			f:write("\t\treturn;\n")
			f:write("\t}\n")
		elseif opcode == "zero_op" then
			OpenVReg(VRegForIndex(instr.loc))
			WriteVReg(VRegForIndex(instr.loc))
			f:write("\t/* Set null on vreg "..instr.loc.."*/\n")
			-- Null vregs are resolved in-place
		elseif opcode == "zero_local" then
			OpenVReg(VRegForIndex(instr.loc))
			WriteVReg(VRegForIndex(instr.loc))
			f:write("\tmemset("..VRegAddr(VRegForIndex(instr.loc), false)..", 0, sizeof("..CodedTypeValue(cs, guidToSymbol, instr.type, instr.isRTP, instr.isVarying, "TracedRTRef").."));\n")
		elseif opcode == "newinstance" or opcode == "newinstance_local" then
			local instVReg = VRegForIndex(instr.loc)
			local nDim = tonumber(instr.ndim)
			if opcode == "newinstance" then
				OpenVReg(instVReg)
			else
				assert(instVReg.isAlive)
			end
			WriteVReg(instVReg)
			do
				local dimStackVReg = VRegForIndex(instr.dimsrc)
				for i=1,nDim do
					LoadVReg(dimStackVReg)
					DestroyVReg(dimStackVReg)
					dimStackVReg = dimStackVReg.next
				end
			end
				
			f:write("\t{\n")
			if nDim ~= 0 then
				f:write("\t\t::rdxLargeUInt dimensions["..instr.ndim.."];\n")
				local dimStackVReg = VRegForIndex(instr.dimsrc)
				for i=1,nDim do
					f:write("\t\tdimensions["..(nDim - i).."] = "..VRegValue(dimStackVReg, true)..";\n")
					dimStackVReg = dimStackVReg.next
				end
			end
			f:write("\t\trdxCObject *obj;\n")
			f:write("\t\tif(!pccmEnvironment.CreateInstanceFromRes(&obj, "..instr.type..", ")
			if nDim == 0 then
				f:write("RDX_CNULL")
			else
				f:write("dimensions")
			end
			f:write(", "..instr.ndim..", "..(iNum - 1).."))\n")
			f:write("\t\t\treturn;\n")
			local instClass = CodedTypeValue(cs, guidToSymbol, instVReg.type, instVReg.isRTP, instVReg.isVarying, "InternalClass")
			f:write("\t\t"..VRegValue(instVReg, false).." = rdxWeakRTRef("..instClass..")(rdxObjRef_CSignal_DataPointer, static_cast<"..instClass.."*>(obj));\n")
			f:write("\t}\n")
		elseif opcode == "exit" then
			f:write("\tpccmEnvironment.ExitFunction();\n")
			f:write("\treturn;\n")
		elseif opcode == "jump" then
			FlushStack()
			f:write("\tgoto instr_"..instr.instrnum..";\n")
		elseif opcode == "jtrue" then
			LoadVReg(VRegForIndex(instr.src))
			local testValue = VRegValue(VRegForIndex(instr.src), true) 
			DestroyVReg(VRegForIndex(instr.src))
			FlushStack()
			f:write("\tif("..testValue.." != rdxFalseValue)\n")
			f:write("\t\tgoto instr_"..instr.instrnum..";\n")
		elseif opcode == "jfalse" then
			LoadVReg(VRegForIndex(instr.src))
			local testValue = VRegValue(VRegForIndex(instr.src), true) 
			DestroyVReg(VRegForIndex(instr.src))
			FlushStack()
			f:write("\tif("..testValue.." == rdxFalseValue)\n")
			f:write("\t\tgoto instr_"..instr.instrnum..";\n")
		elseif opcode == "jnullo" or opcode == "jnulli" then
			LoadVReg(VRegForIndex(instr.src))
			local testValue = VRegValue(VRegForIndex(instr.src), true) 
			DestroyVReg(VRegForIndex(instr.src))
			FlushStack()
			f:write("\tif("..testValue..".IsNull())\n")
			f:write("\t\tgoto instr_"..instr.instrnum..";\n")
		elseif opcode == "jnotnullo" or opcode == "jnotnulli" then
			LoadVReg(VRegForIndex(instr.src))
			local testValue = VRegValue(VRegForIndex(instr.src), true) 
			DestroyVReg(VRegForIndex(instr.src))
			FlushStack()
			f:write("\tif("..testValue..".IsNotNull())\n")
			f:write("\t\tgoto instr_"..instr.instrnum..";\n")
		elseif opcode == "jinherits" then
			LoadVReg(VRegForIndex(instr.src))
			local testValue = VRegValue(VRegForIndex(instr.src), true) 
			FlushStack()
			f:write("\tif(pccmEnvironment.CheckInherits("..VRegValue(VRegForIndex(instr.src), true)..".Data(), "..instr.exType.."))\n")
			f:write("\t\tgoto instr_"..instr.instrnum..";\n")
		elseif opcode == "tick" then
			f:write("\t// UNIMPLEMENTED: "..opcode.."\n")
		elseif opcode == "assertenum" then
			FlushStack()
			local srcVReg = VRegForIndex(instr.src)
			local destVReg = VRegForIndex(instr.dest)
			LoadVReg(srcVReg)
			DestroyVReg(srcVReg)
			OpenVReg(destVReg)
			WriteVReg(destVReg)
			f:write("\tif(!pccmEnvironment.CheckEnum("..VRegValue(srcVReg, true)..", "..instr.type.."))\n")
			f:write("\t{\n")
			f:write("\t\tpccmEnvironment.SetStandardException(rdxX_IncompatibleConversionException, "..(iNum - 1)..");\n")
			f:write("\t\treturn;\n")
			f:write("\t}\n")
			f:write("\t"..VRegValue(destVReg, false).." = "..VRegValue(srcVReg, true)..";\n")
		elseif opcode == "assertinherits" then
			FlushStack()
			local srcVReg = VRegForIndex(instr.src)
			local destVReg = VRegForIndex(instr.dest)
			LoadVReg(srcVReg)
			DestroyVReg(srcVReg)
			OpenVReg(destVReg)
			WriteVReg(destVReg)
			local destInternalType = CodedTypeValue(cs, guidToSymbol, destVReg.type, destVReg.isRTP, destVReg.isVarying, "InternalClass")
			f:write("\tif(!pccmEnvironment.CheckInherits("..VRegValue(srcVReg, true)..".Data(), "..instr.type.."))\n")
			f:write("\t{\n")
			f:write("\t\tpccmEnvironment.SetStandardException(rdxX_IncompatibleConversionException, "..(iNum - 1)..");\n")
			f:write("\t\treturn;\n")
			f:write("\t}\n")
			--f:write("\t"..VRegValue(destVReg, false).." = "..VRegValue(srcVReg, true)..".StaticCast<"..destInternalType..">();\n")
			f:write("\t"..VRegValue(destVReg, false).." = rdxWeakRTRef("..destInternalType..")(rdxObjRef_CSignal_DataPointer, static_cast<"..destInternalType.."*>("..VRegValue(srcVReg, true)..".Modify()));\n")
		elseif opcode == "rcast_itoi" then
			FlushStack()
			local srcVReg = VRegForIndex(instr.src)
			local destVReg = VRegForIndex(instr.dest)
			LoadVReg(srcVReg)
			DestroyVReg(srcVReg)
			OpenVReg(destVReg)
			WriteVReg(destVReg)
			local destInternalType = CodedTypeValue(cs, guidToSymbol, destVReg.type, destVReg.isRTP, destVReg.isVarying, "InternalClass")
			f:write("\t{\n")
			f:write("\t\tbool succeeded;\n")
			f:write("\t\t"..VRegValue(destVReg, false).." = pccmEnvironment.RCast_IToI("..VRegValue(srcVReg, true)..".GetBaseRef().GetPOD(), "..instr.type..", "..(iNum - 1)..", succeeded).StaticCast<"..destInternalType..">();\n")
			f:write("\t\tif(!succeeded)\n")
			f:write("\t\t\treturn;\n")
			f:write("\t}\n")
		elseif opcode == "rcast_otoi" then
			FlushStack()
			local srcVReg = VRegForIndex(instr.src)
			local destVReg = VRegForIndex(instr.dest)
			LoadVReg(srcVReg)
			DestroyVReg(srcVReg)
			OpenVReg(destVReg)
			WriteVReg(destVReg)
			local destInternalType = CodedTypeValue(cs, guidToSymbol, destVReg.type, destVReg.isRTP, destVReg.isVarying, "InternalClass")
			f:write("\t{\n")
			f:write("\t\tbool succeeded;\n")
			f:write("\t\t"..VRegValue(destVReg, false).." = pccmEnvironment.RCast_OToI("..VRegValue(srcVReg, true)..".GetBaseRef().GetPOD(), "..instr.type..", "..(iNum - 1)..", succeeded).StaticCast<"..destInternalType..">();\n")
			f:write("\t\tif(!succeeded)\n")
			f:write("\t\t\treturn;\n")
			f:write("\t}\n")
		elseif opcode == "rcast_otoi_direct" then
			local srcVReg = VRegForIndex(instr.src)
			local destVReg = VRegForIndex(instr.dest)
			LoadVReg(srcVReg)
			DestroyVReg(srcVReg)
			OpenVReg(destVReg)
			WriteVReg(destVReg)
			local destInternalType = CodedTypeValue(cs, guidToSymbol, destVReg.type, destVReg.isRTP, destVReg.isVarying, "InternalClass")
			f:write("\t"..VRegValue(destVReg, false).." = "..VRegValue(srcVReg, true)..".StaticCastToInterface<"..destInternalType..">();\n")
		elseif opcode == "rcast_itoo" then
			FlushStack()
			local srcVReg = VRegForIndex(instr.src)
			local destVReg = VRegForIndex(instr.dest)
			LoadVReg(srcVReg)
			DestroyVReg(srcVReg)
			OpenVReg(destVReg)
			WriteVReg(destVReg)
			local destInternalType = CodedTypeValue(cs, guidToSymbol, destVReg.type, destVReg.isRTP, destVReg.isVarying, "InternalClass")
			f:write("\t{\n")
			f:write("\t\tbool succeeded;\n")
			f:write("\t\t"..VRegValue(destVReg, false).." = pccmEnvironment.RCast_IToO("..VRegValue(srcVReg, true)..".GetBaseRef().GetPOD(), "..instr.type..", "..(iNum - 1)..", succeeded).StaticCast<"..destInternalType..">();\n")
			f:write("\t\tif(!succeeded)\n")
			f:write("\t\t\treturn;\n")
			f:write("\t}\n")
		elseif opcode == "jeq_f" or opcode == "jeq_p" then
			local src1VReg = VRegForIndex(instr.src1)
			local src2VReg = VRegForIndex(instr.src2)
			LoadVReg(src1VReg)
			LoadVReg(src2VReg)
			DestroyVReg(src1VReg)
			DestroyVReg(src2VReg)
			f:write("\tif("..VRegValue(src1VReg, true).." == "..VRegValue(src2VReg, true)..")\n")
			f:write("\t\tgoto instr_"..instr.instrnum..";\n")
		elseif opcode == "jne_f" or opcode == "jne_p" then
			local src1VReg = VRegForIndex(instr.src1)
			local src2VReg = VRegForIndex(instr.src2)
			LoadVReg(src1VReg)
			LoadVReg(src2VReg)
			DestroyVReg(src1VReg)
			DestroyVReg(src2VReg)
			f:write("\tif("..VRegValue(src1VReg, true).." != "..VRegValue(src2VReg, true)..")\n")
			f:write("\t\tgoto instr_"..instr.instrnum..";\n")
		elseif opcode == "xnullref" then
			f:write("\tpccmEnvironment.SetStandardException(rdxX_NullReferenceException, "..(iNum - 1)..");\n")
			f:write("\tif(true) return;\n")
		elseif opcode == "catch" then
			f:write("\tpccmEnvironment.SetStandardException(rdxX_InvalidOperationException, "..(iNum - 1)..");\n")
			f:write("\tif(true) return;\n")
		elseif opcode == "fatal" then
			f:write("\tpccmEnvironment.SetStandardException(rdxX_InvalidOperationException, "..(iNum - 1)..");\n")
			f:write("\tif(true) return;\n")
		elseif opcode == "throw" then
			local exVReg = VRegForIndex(instr.src)
			LoadVReg(exVReg)
			f:write("\tpccmEnvironment.SetException("..VRegValue(exVReg, true)..".Modify(), "..(iNum - 1)..");\n")
			f:write("\tif(true) return;\n")
			DestroyVReg(VRegForIndex(instr.src))
		elseif opcode == "hardenstack" then
			FlushStack()
		elseif opcode == "switch" then
			local srcVReg = VRegForIndex(instr.src)
			LoadVReg(srcVReg)
			DestroyVReg(srcVReg)
			FlushStack()
			local switchFunc = "SwitchScan"
			local convertSuffix = ""
			if CodedTypeIsObjRef(cs, guidToSymbol, srcVReg.type) then
				switchFunc = "SwitchScanObjRef"
				convertSuffix = ".Data()"
			end
			f:write("\tpccmEnvironment."..switchFunc.."("..VRegValue(srcVReg, true)..convertSuffix..", "..instr.cases..", "..instr.numCases..", "..(iNum - 1)..");\n")
			f:write("\tgoto func_entry;\n")
		elseif opcode == "switch_ptr" then
			local srcVReg = VRegForIndex(instr.src)
			LoadVReg(srcVReg)
			DestroyVReg(srcVReg)
			FlushStack()
			f:write("\tpccmEnvironment.SwitchScanPtr("..VRegValue(srcVReg, true)..", "..instr.cases..", "..instr.numCases..", "..(iNum - 1)..");\n")
			f:write("\tgoto func_entry;\n")
		elseif checkedBinaryArithInstructions[opcode] then
			local rsVReg = VRegForIndex(instr.p0)
			local lsVReg = rsVReg.next
			local resultVReg = lsVReg.next
			LoadVReg(rsVReg)
			LoadVReg(lsVReg)
			local rsValue = VRegValue(rsVReg, true)
			local lsValue = VRegValue(lsVReg, true)
			DestroyVReg(rsVReg)
			DestroyVReg(lsVReg)
			FlushStack()
			OpenVReg(resultVReg)
			WriteVReg(resultVReg)
			f:write("\t{\n")
			f:write("\t\trdxECommonExceptions ex = rdxPCCMUtils::CheckedOp_"..opcode.."("..VRegValue(resultVReg, false)..", "..lsValue..", "..rsValue..");\n")
			f:write("\t\tif(ex != rdxX_NumCommonExceptions)\n")
			f:write("\t\t{\n")
			f:write("\t\t\tpccmEnvironment.SetStandardException(ex, "..(iNum - 1)..");\n")
			f:write("\t\t\treturn;\n")
			f:write("\t\t}\n")
			f:write("\t}\n")
		elseif binaryArithInstructions[opcode] then
			local rsVReg = VRegForIndex(instr.p0)
			local lsVReg = rsVReg.next
			local resultVReg = lsVReg.next
			LoadVReg(rsVReg)
			LoadVReg(lsVReg)
			local rsValue = VRegValue(rsVReg, true)
			local lsValue = VRegValue(lsVReg, true)
			DestroyVReg(rsVReg)
			DestroyVReg(lsVReg)
			OpenVReg(resultVReg)
			WriteVReg(resultVReg)
			f:write("\t"..VRegValue(resultVReg, false).." = ("..lsValue..") "..binaryArithInstructions[opcode].." ("..rsValue..");\n")
		elseif unaryArithInstructions[opcode] then
			local unaryOperator = unaryArithInstructions[opcode]
			local valueVReg = VRegForIndex(instr.p0)
			local resultVReg = valueVReg.next
			LoadVReg(valueVReg)
			local valueValue = VRegValue(valueVReg, true)
			DestroyVReg(valueVReg)
			OpenVReg(resultVReg)
			WriteVReg(resultVReg)
			f:write("\t"..VRegValue(resultVReg, false).." = "..unaryOperator.."("..valueValue..");\n")
		elseif castInstructions[opcode] then
			local valueVReg = VRegForIndex(instr.p0)
			local resultVReg = valueVReg.next
			LoadVReg(valueVReg)
			local valueValue = VRegValue(valueVReg, true)
			DestroyVReg(valueVReg)
			OpenVReg(resultVReg)
			WriteVReg(resultVReg)
			local targetType = CodedTypeValue(cs, guidToSymbol, resultVReg.type, false, false, "WeakRTRef")
			f:write("\t"..VRegValue(resultVReg, false).." = static_cast<"..targetType..">("..VRegValue(valueVReg, true))
			EmitPassiveConversions(cs, guidToSymbol, valueVReg, targetType)
			f:write(");\n")
		elseif simpleCmpInstructions[opcode] then
			local rsVReg = VRegForIndex(instr.p1)
			local lsVReg = rsVReg.next
			LoadVReg(rsVReg)
			LoadVReg(lsVReg)
			local rsValue = VRegValue(rsVReg, true)
			local lsValue = VRegValue(lsVReg, true)
			DestroyVReg(rsVReg)
			DestroyVReg(lsVReg)
			FlushStack()
			f:write("\tif(("..lsValue..") "..simpleCmpInstructions[opcode].." ("..rsValue.."))\n")
			f:write("\t\tgoto instr_"..instr.p0..";\n")
		elseif opcode == "jccp" or opcode == "jccf" then
			local rsVReg = VRegForIndex(instr.p1)
			local lsVReg = rsVReg.next
			LoadVReg(rsVReg)
			LoadVReg(lsVReg)
			local rsValue = VRegValue(rsVReg, true)
			local lsValue = VRegValue(lsVReg, true)
			DestroyVReg(rsVReg)
			DestroyVReg(lsVReg)
			FlushStack()
			f:write("\tif(rdxPCCMUtils::JCCOp_"..opcode.."(pccmEnvironment, ("..lsValue.."), ("..rsValue..")))\n")
			f:write("\t\tgoto instr_"..instr.p0..";\n")
		elseif opcode == "iteratearray" then
			local arrayVReg = VRegForIndex(instr.array)
			local indexVReg = VRegForIndex(instr.index)
			local destVReg = VRegForIndex(instr.dest)
			local templateType = CodedTypeValue(cs, guidToSymbol, instr.subscriptType, false, false, "TracedRTRef")
			LoadVReg(indexVReg)
			WriteVReg(indexVReg)
			WriteVReg(destVReg)
			LoadVReg(arrayVReg)
			f:write("\t{\n")
			f:write("\t\tbool shouldExit = false;\n")
			f:write("\t\trdxECommonExceptions ex = rdxPCCMUtils::IterateArray("..VRegAddr(arrayVReg, true)..", "..VRegAddr(indexVReg, false)..", "..VRegAddr(destVReg, false)..", shouldExit);\n")
			f:write("\t\tif(ex != rdxX_NumCommonExceptions)\n")
			f:write("\t\t{\n")
			f:write("\t\t\tpccmEnvironment.SetStandardException(ex, "..(iNum - 1)..");\n")
			f:write("\t\t\treturn;\n")
			f:write("\t\t}\n")
			f:write("\t\tif(shouldExit)\n")
			f:write("\t\t\tgoto instr_"..instr.exitInstr..";\n")
			f:write("\t}\n")
		else
			f:write("\t// UNIMPLEMENTED: "..opcode.."\n")
			assert(false, "Unimplemented op: "..opcode)
		end

		DischargeMOP(iNum)
	end

	for _,vReg in ipairs(vRegList) do
		if vReg.isAlive and not vReg.isPRV then
			assert(false, "VReg "..vReg.regID.." leaked")
		elseif vReg.isPRV and not vReg.isAlive then
			assert(false, "VReg "..vReg.regID.." destroyed")
		end
	end

	f:write("} while(false);\n")
	f:write("func_entry:\n")
	f:write("\tswitch(pccmEnvironment.GetResumeInstruction())\n")
	f:write("\t{\n")
	for iNum,instr in ipairs(instructions) do
		if instr.isResumable then
			f:write("\tcase "..(iNum - 1)..": goto instr_"..(iNum - 1)..";\n")
		end
	end
	f:write("\t};\n")
	f:write("}\n")
	f:close()

	-- Prepend dependencies
	f = io.open(methodFilePath, "rb")
	local existingContents = f:read("*a")
	f:close()

	f = io.open(methodFilePath, "wb")
	f:write("#include \"rdx_pccm_api.hpp\"\n")
	for path in sortedpairs(headerPathSet) do
		f:write("#include \""..path.."\"\n")
	end
	f:write("\n")
	f:write(existingContents)
	f:close()
end )
