--[[
 Copyright (C) 2011-2013 Eric Lasota

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
]]


--immediate_rtp: la[0].ca[0].rtp
--immediate_ptr: la[0].ca[0].p
--call: ca[2]
--callinterface: ca[4]
--newinstance: ca[1]
--jinherits: ca[2]
--assertinherits: ca[2]


-- Values stored in the local or parameter frames can use a cache system that allows them to be shared across multiple
-- instructions.
-- retrieveValue: Returns a variable containing a value on the frame.  Should ONLY be used for values that could be on the frame prior to the instruction.
-- storeValue: Returns a variable allowing storage to the frame.  Should ONLY be used for values that will be on the frame after the instruction.
--
-- Macros:
-- CALLMETHOD
-- TICK

local CONFIG_ALLOW_VOLATILE = false

local varType
local retrieveValue
local storeValue
local frameVarName
local frameAddress

local typedRuntimePointerSize
local runtimePointerSize
local addressSize
local addressAlign
local boolSize
local boolAlign
local stackValueSize
local headerF
local outF

local compiledMethodsSet = { }
local compiledFunctionNames = { }
local compiledFunctionInstrCounts = { }
local compiledFunctionInsertIndexes = { }
local compiledFunctionInstrInserts = { }
local compiledFunctionForbiddenResumes = { }

local valueSizedTypes = { }

local unionTypes =
{
	["UInt8"] = { size = "1", uName = "u8",		isUInt = true },
	["UInt16"] = { size = "2", uName = "u16",	isUInt = true },
	["UInt32"] = { size = "4", uName = "u32",	isUInt = true },
	["UInt64"] = { size = "8", uName = "u64",	isUInt = true },
	["Int8"] = { size = "1", uName = "i8",		isInt = true },
	["Int16"] = { size = "2", uName = "i16",	isInt = true },
	["Int32"] = { size = "4", uName = "i32",	isInt = true },
	["Int64"] = { size = "8", uName = "i64",	isInt = true },
	["Float32"] = { size = "4", uName = "f32",	isFloat = true },
	["Float64"] = { size = "8", uName = "f64",	isFloat = true },
}

local resumeOffsettingInstructions =
{
	ILOP_debuginfo = true,
}

local nextMarkingInstructions =
{
	ILOP_call = true,
	ILOP_calldelegatebp = true,
	ILOP_calldelegateprv = true,
	ILOP_callvirtual = true,
	ILOP_callinterface = true,
	ILOP_tick = true,
}

local currentMarkingInstructions =
{
	ILOP_call = true,
	ILOP_calldelegateprv = true,
	ILOP_calldelegatebp = true,
	ILOP_callvirtual = true,
	ILOP_callinterface = true,
	ILOP_newinstance = true,
}

local instrnumMarkingInstructions =
{
	ILOP_jump = true,
	ILOP_jtrue = true,
	ILOP_jfalse = true,
	ILOP_jinherits = true,
	ILOP_jeq_f = true,
	ILOP_jeq_p = true,
	ILOP_jne_f = true,
	ILOP_jne_p = true,
}

local srcMarkingInstructions =
{
	ILOP_ilt = true,
	ILOP_igt = true,
	ILOP_ile = true,
	ILOP_ige = true,
	ILOP_ieq = true,
	ILOP_ine = true,

	ILOP_flt = true,
	ILOP_fgt = true,
	ILOP_fle = true,
	ILOP_fge = true,
	ILOP_feq = true,
	ILOP_fne = true,
}

local caseMarkingInstructions =
{
	ILOP_switch = true,
	ILOP_switch_ptr = true,
}

local exitInstrMarkingInstructions =
{
	ILOP_iteratearray = true,
	ILOP_iteratearraysub = true,
}

local codeInsertionPoints = { }
local forbiddenResumes = { }
local numInsertionPoints = 0
local currentFunctionID = 0
local cacheTagNumber = 0

local activeValues = { }

markCurrentInstruction = function(instr)
	instr.active = true
	flushFrame(instr)
end

markNextInstruction = function(instr)
	instr.nextActive = true
end

forbidResumeOnNext = function(instr)
	instr.forbidResumeOnNext = true
end


varType = function(size)
	if valueSizedTypes[size] then
		return "RuntimeValue"..size
	end

	local numAddress
	if math ~= nil then
		numAddress = math.floor(size / addressSize)
	else
		-- Integer build
		numAddress = size / addressSize
	end

	headerF:write("union RuntimeValue"..size.."\n")
	headerF:write("{\n")
	headerF:write("\tUInt8 bytes["..size.."];\n")
	if numAddress > 0 then
		headerF:write("\tLargeInt addrs["..numAddress.."];\n")
	end
	for tName,tData in pairs(unionTypes) do
		if tData.size == size then
			headerF:write("\t"..tName.." "..tData.uName..";\n")
		end
	end

	-- Write the volatile copy operator

	headerF:write("inline RuntimeValue"..size.." &operator =(const volatile RuntimeValue"..size.." &rs)\n")
	headerF:write("{\n")

	if numAddress > 0 then
		headerF:write("	const volatile LargeInt *volAddress = rs.addrs;\n")
		headerF:write("	for(LargeInt i=0;i<"..numAddress..";i++)\n")
		headerF:write("		this->addrs[i] = volAddress[i];\n")
	end

	local addressBytes = (numAddress * 4)
	local trailingBytes = size - addressBytes
	if trailingBytes > 0 then
		headerF:write("	const volatile UInt8 *volBytes = rs.bytes + "..addressBytes..";\n")
		headerF:write("	for(LargeInt i=0;i<"..trailingBytes..";i++)\n")
		headerF:write("		this->bytes[i+"..addressBytes.."] = volBytes[i];\n")
	end

	headerF:write("	return *this;\n")
	headerF:write("}\n")

	headerF:write("};\n")

	valueSizedTypes[size] = true
	return "RuntimeValue"..size
end

frameVarName = function(loc, size, prv)
	local locConverted = string.gsub(loc, "-", "_")
	local sizeConverted = string.gsub(size, "-", "_")

	if prv then
		return "__prv_offs"..locConverted.."_sz"..sizeConverted
	end
	return "__bp_offs"..locConverted.."_sz"..sizeConverted
end

cacheTag = function()
	cacheTagNumber = cacheTagNumber + 1
	return cacheTagNumber
end

retrieveValue = function(instr, loc, size, forget, prv)
	local shortVarName = frameVarName(loc, size, prv)

	if not activeValues[shortVarName] then
		local varName = shortVarName.."_"..cacheTag()
		local vt = varType(size)
		instr:prependCode("\t"..vt.." "..varName..";\n")
		instr:prependCode("\t{\n")
		instr:prependCode("\t\tconst void *src = "..frameAddress(prv and "prv" or "bp", loc, true)..";\n")
		instr:prependCode("\t\t"..varName.." = *reinterpret_cast<const "..vt.." *>(src);\n")
		instr:prependCode("\t}\n")
		activeValues[shortVarName] = { loc = loc, size = size, dirty = false, varName = varName, prv = prv }
	end
	local rv = activeValues[shortVarName].varName

	if forget then
		activeValues[shortVarName] = nil
	end

	return rv
end

storeValue = function(instr, loc, size, prv)
	local shortVarName = frameVarName(loc, size, prv)

	if not activeValues[shortVarName] then
		local vt = varType(size)
		local varName = shortVarName.."_"..cacheTag()
		instr:prependCode("\t"..vt.." "..varName..";\n")
		activeValues[shortVarName] = { loc = loc, size = size, dirty = true, varName = varName, prv = prv }
	else
		activeValues[shortVarName].dirty = true
	end
	return activeValues[shortVarName].varName
end

forgetValue = function(instr, loc, size)
	local shortVarName = frameVarName(loc, size)

	activeValues[shortVarName] = nil
end


-- Enforces that all stored values are on the stack frame before
-- If "preserve" is true, then the values won't be reloaded.  This should be used if the instruction can not be jumped to.
-- If "conditional" is true, then values will be saved again the next time they're flushed.  This should be used if the flush isn't guaranteed.
flushFrame = function(instr, preserve, conditional)
	local sortedKeys = { }
	local i = 0

	for k in pairs(activeValues) do
		i = i + 1
		sortedKeys[i] = k
	end

	table.sort(sortedKeys)

	for _,varKey in ipairs(sortedKeys) do
		local varData = activeValues[varKey]
		if varData.dirty then
			local addedCode = ("\t*reinterpret_cast<"..varType(varData.size).." *>("..frameAddress(varData.prv and "prv" or "bp", varData.loc, false)..") = "..varData.varName..";\n")
			if conditional then
				instr:appendCode(addedCode)
			else
				instr.flush = instr.flush..addedCode
			end
			varData.dirty = nil
		end
		varKey,varData = next(activeValues)
	end

	if not preserve then
		activeValues = { }
		instr.perforate = true
	end
end


frameAddress = function(reg, offset, isConst)
	local c
	if isConst then
		c = "const "
	else
		c = ""
	end
	return "(static_cast<"..c.."void *>(static_cast<"..c.." UInt8 *>("..reg..") + ("..offset..")))"
end

function svOffset(sp, varSize)
	sp = tonumber(sp)
	varSize = tonumber(varSize)
	while varSize > 0 do
		sp = sp + stackValueSize
		varSize = varSize - stackValueSize
	end
	return tostring(sp)
end


local function unaryArithmeticOp(operator, typeTag)
	return function(instr)
		local sp = instr.src
		local src = retrieveValue(instr, instr.src, instr.p2, true)
		local dest = storeValue(instr, instr.dest, instr.p2)

		for _,t in pairs(unionTypes) do
			if t.size == instr.p2 and t[typeTag] then
				instr:appendCode("\t"..dest.."."..t.uName.." = "..operator.."("..src.."."..t.uName..");\n")
			end
		end
	end
end

local function conversionOp(srcTypeTag, destTypeTag)
	return function(instr)
		local sp = instr.src
		local src = retrieveValue(instr, instr.src, instr.p2, true)
		local dest = storeValue(instr, instr.dest, instr.p3)

		for tName,t in pairs(unionTypes) do
			if t.size == instr.p3 and t[destTypeTag] then
				instr:appendCode("\t"..dest.."."..t.uName.." = static_cast<"..tName..">(")
			end
		end

		for tName,t in pairs(unionTypes) do
			if t.size == instr.p2 and t[srcTypeTag] then
				instr:appendCode("\t\t"..src.."."..t.uName..");\n")
			end
		end
	end
end

local function comparisonOp(operator, typeTag)
	return function(instr)
		flushFrame(instr, true)

		-- src is actually instrnum
		-- dest is actually src
		local sp = instr.src
		local rsSrc = retrieveValue(instr, instr.dest, instr.p2, true)
		local lsSrc = retrieveValue(instr, svOffset(instr.dest, instr.p2), instr.p2, true)

		for _,t in pairs(unionTypes) do
			if t.size == instr.p2 and t[typeTag] then
				instr:appendCode("\tif("..lsSrc.."."..t.uName.." "..operator.." "..rsSrc.."."..t.uName..")\n")
				instr:appendCode("\t\tgoto __func"..currentFunctionID.."_instr"..instr.src..";\n")
			end
		end
	end
end

local function binaryArithmeticOp(operator, typeTag, checkForZero)
	return function(instr)
		if checkForZero then
			flushFrame(instr, true)
		end

		local sp = instr.src
		local rsSrc = retrieveValue(instr, instr.src, instr.p2, true)
		local lsSrc = retrieveValue(instr, svOffset(instr.src, instr.p2), instr.p2, true)

		local dest = storeValue(instr, instr.dest, instr.p2)

		for _,t in pairs(unionTypes) do
			if t.size == instr.p2 and t[typeTag] then
				if checkForZero then
					instr:appendCode("\tif(!"..rsSrc.."."..t.uName..")\n")
					instr:appendCode("\t\tDIVIDEBYZEROEXCEPTION("..instr.instructionNumber..");\n")
				end
				instr:appendCode("\t"..dest.."."..t.uName.." = "..lsSrc.."."..t.uName.." "..operator.." "..rsSrc.."."..t.uName..";\n")
			end
		end
	end
end

function AddCallMethod(instr)
	instr:appendCode("\t\tbool shouldContinue = false;\n")
	instr:appendCode("\t\tint methodStatus = CallMethod(ctx, objm, invokedMethod, "..instr.instructionNumber..", "..instr.frameoffs..", "..instr.prvoffs..", thread, instrTable, bp, providerDictionary, shouldContinue);\n")
	instr:appendCode("\t\tif(!shouldContinue)\n")
	instr:appendCode("\t\t\treturn methodStatus;\n")
end


local opcodeEmissions =
{
	ILOP_debuginfo = function(instr)
		instr:appendCode("// "..instr.filename.." [line "..instr.line.."]\n")
	end,

	ILOP_move = function(instr)
		-- Possible flags: destParentFrame, destDeref, srcParentFrame, srcDeref, transient, srcTypeDefault
		local vt = varType(instr.size)

		local volatileRead = (CONFIG_ALLOW_VOLATILE and instr.flags.object)

		local code = ""
		code = code.."\t{\n"
		code = code.."\t\tconst "..(volatileRead and "volatile " or "")..vt.." *src;\n"
		code = code.."\t\t"..vt.." *dest;\n"
		if instr.flags.srcAbsolute then
			code = code.."\t\tsrc = static_cast<const StructuredType *>(resArgs["..instr.srcType.."])->_native.actualDefaultValue;\n"
		else
			if instr.flags.srcDeref then
				code = code.."\t\tconst RuntimePointer<void> *srcPtr = &"..retrieveValue(instr, instr.src, runtimePointerSize, instr.flags.transient, instr.flags.srcParentFrame)..".rtp;\n"
				code = code.."\t\tsrc = static_cast<const "..(volatileRead and "volatile " or "")..vt.." *>(srcPtr->valueRef);\n"
			else
				code = code.."\t\tsrc = reinterpret_cast<const "..vt.." *>(&"..retrieveValue(instr, instr.src, instr.size, instr.flags.transient, instr.flags.srcParentFrame)..");\n"
			end
		end

		if instr.flags.destDeref then
			code = code.."\t\tconst RuntimePointer<void> *destPtr = &"..retrieveValue(instr, instr.dest, runtimePointerSize, true, instr.flags.destParentFrame)..".rtp;\n"
			code = code.."\t\tdest = static_cast<"..vt.." *>(destPtr->valueRef);\n"
		else
			code = code.."\t\tdest = reinterpret_cast<"..vt.." *>(&"..storeValue(instr, instr.dest, instr.size, instr.flags.destParentFrame)..");\n"
		end

		code = code.."\t\t*dest = *src;\n"
		code = code.."\t}\n"

		instr:appendCode(code)
	end,

	ILOP_tovarying = function(instr)
		local rtpLoc = retrieveValue(instr, instr.src, runtimePointerSize, true)
		local storeLoc = storeValue(instr, instr.dest, typedRuntimePointerSize)

		instr:appendCode("\t"..storeLoc..".trtp.rtp = "..rtpLoc..".rtp;\n")
		instr:appendCode("\t"..storeLoc..".trtp.type = static_cast<const Type *>(resArgs["..instr.type.."]);\n")
	end,

	ILOP_tovarying_static = function(instr)
		local rtpLoc = retrieveValue(instr, instr.loc, runtimePointerSize, true)
		local storeLoc = storeValue(instr, instr.loc, typedRuntimePointerSize)

		instr:appendCode("\t"..storeLoc..".trtp.rtp = "..rtpLoc..";\n")
		instr:appendCode("\t"..storeLoc..".trtp.type = static_cast<const Type *>(resArgs["..instr.type.."]);\n")
	end,

	ILOP_pinl = function(instr)
		flushFrame(instr)
		local storeLoc = storeValue(instr, instr.dest, runtimePointerSize)
		instr:appendCode("\t"..storeLoc..".rtp.objectRef = &thread;\n")
		instr:appendCode("\t"..storeLoc..".rtp.valueRef = "..frameAddress("bp", instr.offset)..";\n")
	end,

	ILOP_pinp = function(instr)
		flushFrame(instr)
		local storeLoc = storeValue(instr, instr.dest, runtimePointerSize)
		instr:appendCode("\t"..storeLoc..".rtp.objectRef = &thread;\n")
		instr:appendCode("\t"..storeLoc..".rtp.valueRef = "..frameAddress("prv", instr.offset)..";\n")
	end,

	ILOP_incptr = function(instr)
		local storeLoc = storeValue(instr, instr.loc, runtimePointerSize)
		instr:appendCode("\t"..storeLoc..".rtp.valueRef = reinterpret_cast<UInt8 *>("..storeLoc..".rtp.valueRef) + ("..instr.offset..");\n")
	end,

	ILOP_objinterior_notnull = function(instr)
		local objLoc = retrieveValue(instr, instr.src, addressSize, true)
		local storeLoc = storeValue(instr, instr.dest, runtimePointerSize)
		instr:appendCode("\t"..storeLoc..".rtp.objectRef = "..objLoc..".p;\n")
		instr:appendCode("\t"..storeLoc..".rtp.valueRef = reinterpret_cast<UInt8 *>("..objLoc..".p) + ("..instr.offset..");\n")
	end,

	ILOP_objinterior_notnull_persist = function(instr)
		local objLoc = retrieveValue(instr, instr.src, addressSize)
		local storeLoc = storeValue(instr, instr.dest, runtimePointerSize)
		instr:appendCode("\t"..storeLoc..".rtp.objectRef = "..objLoc..".p;\n")
		instr:appendCode("\t"..storeLoc..".rtp.valueRef = reinterpret_cast<UInt8 *>("..objLoc..".p) + ("..instr.offset..");\n")
	end,

	ILOP_objinterior = function(instr)
		flushFrame(instr)

		local objLoc = retrieveValue(instr, instr.src, addressSize, true)

		instr:appendCode("\tNULLCHECK("..objLoc..".p, "..instr.instructionNumber..");\n")

		local storeLoc = storeValue(instr, instr.dest, runtimePointerSize)
		instr:appendCode("\t"..storeLoc..".rtp.objectRef = "..objLoc..".p;\n")
		instr:appendCode("\t"..storeLoc..".rtp.valueRef = reinterpret_cast<UInt8 *>("..objLoc..".p) + ("..instr.offset..");\n")
	end,

	ILOP_verifynotnull = function(instr)
		local objLoc = retrieveValue(instr, instr.loc, addressSize, false)
		instr:appendCode("\tNULLCHECK("..objLoc..".p, "..instr.instructionNumber..");\n")
	end,

	ILOP_iteratearraysub = function(instr)
		flushFrame(instr)

		instr.jumpTarget = instr.exitInstr

		local eType = varType(instr.size)
		local arrayLoc = retrieveValue(instr, instr.array, addressSize, false)
		local indexLoc = retrieveValue(instr, instr.index, addressSize, false)
		local destLoc = retrieveValue(instr, instr.dest, instr.size, false)

		-- Touch anything modified by this
		storeValue(instr, instr.index, addressSize)
		storeValue(instr, instr.dest, instr.size)

		local subs = { }

		local subIdxCount = tonumber(instr.subidxcount)
		local subIdxLoc = tonumber(instr.subidxloc)
		local addrSizeNum = tonumber(addressSize)
		for i=1,subIdxCount do
			local location = tostring(subIdxLoc - (i - 1) * addrSizeNum)
			subs[i] = retrieveValue(instr, location, addressSize, false)
			storeValue(instr, location, addressSize)
		end
		
		-- We don't need to do any conditional flushes here because the stack is flushed before
		-- this instruction and no changes occur unless all verification succeeds

		local code = ""
		code = code.."\t{\n"
		code = code.."\t\tconst "..eType.." *arrayRef = static_cast<const "..eType.." *>("..arrayLoc..".p);\n"
		code = code.."\t\tNULLCHECK("..arrayLoc..".p, "..instr.instructionNumber..");\n"
		code = code.."\t\tLargeInt numElements = GCInfo::From("..arrayLoc..".p)->numElements;\n"
		code = code.."\t\tLargeInt nextIndex = ("..indexLoc..".li) + 1;\n"
		code = code.."\t\tif(nextIndex < 0 || nextIndex >= numElements)\n"
		code = code.."\t\t\tgoto __func"..currentFunctionID.."_instr"..instr.exitInstr..";\n"
		code = code.."\t\t"..destLoc.." = arrayRef[nextIndex];\n"
		code = code.."\t\tif(GCInfo::From(arrayRef)->numDimensions != "..instr.subidxcount..")\n"
		code = code.."\t\t\tOUTOFBOUNDS("..instr.instructionNumber..");\n"
		code = code.."\t"..indexLoc..".li = nextIndex;\n"
		code = code.."\t\tconst LargeInt *dimensions = GCInfo::From(arrayRef)->dimensions;\n"

		for i=1,subIdxCount do
			if i == subIdxCount then
				code = code.."\t\t++("..subs[i]..").li;\n"
			else
				code = code.."\t\tif(dimensions["..(i-1).."] == ++("..subs[i]..").li)\n"
				code = code.."\t\t{\n"
				code = code.."\t\t"..subs[i]..".li = 0;\n"
			end
		end
		for i=1,subIdxCount-1 do
				code = code.."\t\t}\n"
		end
		code = code.."\t}\n"

		instr:appendCode(code)
	end,

	ILOP_iteratearray = function(instr)
		flushFrame(instr)

		instr.jumpTarget = instr.exitInstr

		local eType = varType(instr.size)
		local arrayLoc = retrieveValue(instr, instr.array, addressSize, false)
		local indexLoc = retrieveValue(instr, instr.index, addressSize, false)
		local destLoc = retrieveValue(instr, instr.dest, instr.size, false)

		-- Touch anything modified by this
		storeValue(instr, instr.index, addressSize)
		storeValue(instr, instr.dest, instr.size)

		local code = ""
		code = code.."\t{\n"
		code = code.."\t\tconst "..eType.." *arrayRef = static_cast<const "..eType.." *>("..arrayLoc..".p);\n"
		code = code.."\t\tNULLCHECK("..arrayLoc..".p, "..instr.instructionNumber..");\n"
		code = code.."\t\tLargeInt numElements = GCInfo::From("..arrayLoc..".p)->numElements;\n"
		code = code.."\t\tLargeInt nextIndex = ("..indexLoc..".li) + 1;\n"
		code = code.."\t\tif(nextIndex < 0 || nextIndex >= numElements)\n"
		code = code.."\t\t\tgoto __func"..currentFunctionID.."_instr"..instr.exitInstr..";\n"
		code = code.."\t\t"..indexLoc..".li = nextIndex;\n"
		code = code.."\t\t"..destLoc.." = arrayRef[nextIndex];\n"
		code = code.."\t}\n"
		instr:appendCode(code)
	end,

	ILOP_immediate = function(instr)
		-- FIX ME
		local storeLoc = storeValue(instr, instr.dest, instr.size)
		local offset = 0
		local nValues = #instr.value
		local addrSizeNum = tonumber(addressSize)
		while (nValues - offset) >= addrSizeNum do
			instr:appendCode("\t"..storeLoc..".addrs["..(offset/addressSize).."] = PackAddress(")
			for i=offset+1,offset+addressSize do
				if i ~= offset+1 then
					instr:appendCode(",")
				end
				instr:appendCode(instr.value[i])
			end
			instr:appendCode(");\n")
			offset = offset + addrSizeNum
		end

		while offset ~= nValues do
			instr:appendCode("\t"..storeLoc..".bytes["..offset.."] = "..instr.value[offset+1]..";\n")
			offset = offset + 1
		end
	end,

	ILOP_immediate_ptr = function(instr)
		local storeLoc = storeValue(instr, instr.dest, addressSize)
		instr:appendCode("\t"..storeLoc..".cp = resArgs["..instr.res.."];\n")
	end,

	ILOP_immediate_rtp = function(instr)
		local storeLoc = storeValue(instr, instr.dest, instr.size)
		instr:appendCode("\t"..storeLoc..".rtp.objectRef = resArgs["..instr.res.."];\n")
		instr:appendCode("\t"..storeLoc..".rtp.valueRef = static_cast<UInt8 *>(resArgs["..instr.res.."]) + "..instr.resOffset..";\n")
	end,

	ILOP_arrayindex = function(instr)
		flushFrame(instr, true)
		local objLoc = retrieveValue(instr, instr.arraysrc, addressSize, true)

		instr:appendCode("\t{\n")
		instr:appendCode("\t\tvoid *dataLoc = RDX_RuntimeUtilities_ArrayIndex("..objLoc..".p, reinterpret_cast<const RuntimeStackValue *>("..frameAddress("bp", instr.indexsrc, true).."));\n")
		instr:appendCode("\t\tif(dataLoc == NULL)\n")
		instr:appendCode("\t\t\tOUTOFBOUNDS("..instr.instructionNumber..");\n")

		local storeLoc = storeValue(instr, instr.dest, runtimePointerSize)
		instr:appendCode("\t\t"..storeLoc..".rtp.objectRef = "..objLoc..".p;\n")
		instr:appendCode("\t\t"..storeLoc..".rtp.valueRef = reinterpret_cast<UInt8 *>(dataLoc) + ("..instr.offset..");\n")
		instr:appendCode("\t}\n")
	end,

	ILOP_call = function(instr)
		markCurrentInstruction(instr)
		flushFrame(instr)

		local native = (instr.isnative and "true" or "false")
		local notNative = (instr.isnative and "false" or "true")

		instr:appendCode("\t{\n")
		instr:appendCode("\t\tconst Method *invokedMethod = static_cast<const Method *>(resArgs["..instr.method.."]);\n")
		if instr.isnative then
			instr:appendCode("\t\tbool shouldContinue = false;\n")
			instr:appendCode("\t\tint methodStatus = CallMethodNative(ctx, objm, invokedMethod, "..instr.instructionNumber..", "..instr.frameoffs..", "..instr.prvoffs..", thread, instrTable, bp, shouldContinue);\n")
			instr:appendCode("\t\tif(!shouldContinue)\n")
			instr:appendCode("\t\t\treturn methodStatus;\n")
		else
			instr:appendCode("\t\treturn CallMethodNotNative(ctx, objm, invokedMethod, "..instr.instructionNumber..", "..instr.frameoffs..", "..instr.prvoffs..", thread, instrTable, bp, providerDictionary);\n")
		end
		instr:appendCode("\t}\n")

		markNextInstruction(instr)
	end,

	ILOP_calldelegatebp = function(instr)
		markCurrentInstruction(instr)
		local methodLoc = retrieveValue(instr, instr.methodsrc, addressSize, false, false)
		flushFrame(instr)

		instr:appendCode("\t{\n")
		instr:appendCode("\t\tconst Method *invokedMethod = static_cast<const Method *>("..methodLoc..".p);\n")
		instr:appendCode("\t\tNULLCHECK(invokedMethod, "..instr.instructionNumber..");\n")
		AddCallMethod(instr)
		instr:appendCode("\t}\n")

		markNextInstruction(instr)
	end,

	ILOP_calldelegateprv = function(instr)
		markCurrentInstruction(instr)
		local methodLoc = retrieveValue(instr, instr.methodsrc, addressSize, false, true)
		flushFrame(instr)

		instr:appendCode("\t{\n")
		instr:appendCode("\t\tconst Method *invokedMethod = static_cast<const Method *>("..methodLoc..".p);\n")
		instr:appendCode("\t\tNULLCHECK(invokedMethod, "..instr.instructionNumber..");\n")
		AddCallMethod(instr)
		instr:appendCode("\t}\n")

		markNextInstruction(instr)
	end,

	ILOP_callvirtual = function(instr)
		markCurrentInstruction(instr)
		flushFrame(instr)
		local objLoc = retrieveValue(instr, instr.objsrc, addressSize, true)

		instr:appendCode("\tNULLCHECK("..objLoc..".p, "..instr.instructionNumber..");\n")
		instr:appendCode("\t{\n")
		instr:appendCode("\t\tconst Method *invokedMethod = static_cast<const StructuredType *>(GCInfo::From("..objLoc..".p)->containerType)->virtualMethods["..instr.vstindex.."];\n")
		AddCallMethod(instr)
		instr:appendCode("\t}\n")

		markNextInstruction(instr)
	end,

	ILOP_callinterface = function(instr)
		markCurrentInstruction(instr)
		flushFrame(instr)
		local objLoc = retrieveValue(instr, instr.objsrc, addressSize, true)

		instr:appendCode("\t{\n")
		instr:appendCode("\t\tconst Method *imethod = static_cast<const Method *>(resArgs["..instr.method.."]);\n")
		instr:appendCode("\t\tconst Type *interfaceType = imethod->parameters[imethod->thisParameterOffset - 1].type;\n")
		instr:appendCode("\t\tconst InterfaceImplementation *impl = static_cast<const StructuredType *>(GCInfo::From("..objLoc..".p)->containerType)->interfaces;\n");
		instr:appendCode("\t\twhile(impl->type != interfaceType)\n")
		instr:appendCode("\t\t\timpl++;\n")
		instr:appendCode("\t\tconst Method *invokedMethod =\n")
		instr:appendCode("\t\t\tstatic_cast<const StructuredType*>(GCInfo::From("..objLoc..".p)->containerType)->virtualMethods[("..instr.vstindex..") + impl->vftOffset];\n")
		AddCallMethod(instr)
		instr:appendCode("\t}\n")

		markNextInstruction(instr)
	end,

	ILOP_zero = function(instr)
		local vt = varType(instr.size)
		local storeLoc = storeValue(instr, instr.loc, instr.size)
		instr:appendCode("\tmemset(&"..storeLoc..", 0, "..instr.size..");\n")
	end,

	ILOP_newinstance = function(instr)
		flushFrame(instr)
		markCurrentInstruction(instr)

		local storeLoc = storeValue(instr, instr.loc, addressSize)

		instr:appendCode("\t{\n")
		instr:appendCode("\t\tt->frame.ip = instrTable + (("..instr.instructionNumber..") + 1);\n")
		instr:appendCode("\t\tconst Type *t = static_cast<const Type *>(resArgs["..instr.type.."]);\n")
		instr:appendCode("\t\tvoid *obj = RDX_RuntimeUtilities_NewObjectInstance(ctx, objm, t, reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8 *>(bp) + ("..instr.dimsrc..")), "..instr.ndim..");\n")
		instr:appendCode("\t\tNULLCHECK(obj, "..instr.instructionNumber..");\n")
		instr:appendCode("\t\t"..storeLoc..".p = obj;\n")
		instr:appendCode("\t}\n")
	end,

	ILOP_exit = function(instr)
		flushFrame(instr)
		instr:appendCode("\tEXITFRAME;\n")
	end,

	ILOP_jump = function(instr)
		flushFrame(instr)
		instr.jumpTarget = instr.instrnum
		instr:appendCode("\tgoto __func"..currentFunctionID.."_instr"..instr.instrnum..";\n")
	end,

	ILOP_jtrue = function(instr)
		flushFrame(instr, true)
		local boolLoc = retrieveValue(instr, instr.src, boolSize, true)
		instr:appendCode("\tif("..boolLoc..".bo == TrueValue)\n")
		instr:appendCode("\t\tgoto __func"..currentFunctionID.."_instr"..instr.instrnum..";\n");
	end,

	ILOP_jfalse = function(instr)
		flushFrame(instr, true)
		local boolLoc = retrieveValue(instr, instr.src, boolSize, true)
		instr:appendCode("\tif("..boolLoc..".bo == FalseValue)\n")
		instr:appendCode("\t\tgoto __func"..currentFunctionID.."_instr"..instr.instrnum..";\n");
	end,

	ILOP_jinherits = function(instr)
		flushFrame(instr, true)

		local instanceLoc = retrieveValue(instr, instr.src, addressSize, true)

		instr:appendCode("\tNULLCHECK("..instanceLoc..".p, "..instr.instructionNumber..");\n")
		instr:appendCode("\tif(objm->TypesCompatible(GCInfo::From("..instanceLoc..".p)->containerType, static_cast<Type *>(resArgs["..instr.exType.."])))\n")
		instr:appendCode("\t\tgoto __func"..currentFunctionID.."_instr"..instr.instrnum..";\n");
	end,

	ILOP_tick = function(instr)
		flushFrame(instr)
		markNextInstruction(instr)
		instr:appendCode("\tTICK("..instr.instructionNumber..");\n")
	end,

	ILOP_hash_f = function(instr)
	end,

	ILOP_hash_p = function(instr)
	end,

	ILOP_assertenum = function(instr)
		--markNextInstruction(instr)
		flushFrame(instr, true)

		local instanceLoc = retrieveValue(instr, instr.src, addressSize, true)

		instr:appendCode("\tif(!objm->EnumCompatible("..instanceLoc..".ev, static_cast<const StructuredType *>(resArgs["..instr.type.."])->enumerants))\n")
		instr:appendCode("\t\tINCOMPATIBLECONVERSION("..instr.instructionNumber..");\n");
	end,

	ILOP_assertinherits = function(instr)
		--markNextInstruction(instr)
		flushFrame(instr, true)

		local instanceLoc = retrieveValue(instr, instr.src, addressSize, true)

		instr:appendCode("\tNULLCHECK("..instanceLoc..".p, "..instr.instructionNumber..");\n")
		instr:appendCode("\tif(!objm->ObjectCompatible("..instanceLoc..".p, static_cast<const Type *>(resArgs["..instr.type.."])))\n")
		instr:appendCode("\t\tINCOMPATIBLECONVERSION("..instr.instructionNumber..");\n");
	end,

	ILOP_jeq_f = function(instr)
		flushFrame(instr)
		instr:appendCode("\tif(!memcmp(static_cast<const UInt8 *>(bp) + ("..instr.src1.."), static_cast<const UInt8 *>(bp) + ("..instr.src2.."), "..instr.size.."))\n")
		instr:appendCode("\t\tgoto __func"..currentFunctionID.."_instr"..instr.instrnum..";\n")
		instr.jumpTarget = instr.instrnum
	end,

	ILOP_jeq_p = function(instr)
		flushFrame(instr, true)

		local p1 = retrieveValue(instr, instr.src1, runtimePointerSize, true);
		local p2 = retrieveValue(instr, instr.src2, runtimePointerSize, true);

		instr:appendCode("\tif(!memcmp("..p1..".valueRef, "..p2..".valueRef, "..instr.size.."))\n")
		instr:appendCode("\t\tgoto __func"..currentFunctionID.."_instr"..instr.instrnum..";\n")
		instr.jumpTarget = instr.instrnum

		flushFrame(instr)
	end,

	ILOP_jne_f = function(instr)
		flushFrame(instr)
		instr:appendCode("\tif(memcmp(static_cast<const UInt8 *>(bp) + ("..instr.src1.."), static_cast<const UInt8 *>(bp) + ("..instr.src2.."), "..instr.size.."))\n")
		instr:appendCode("\t\tgoto __func"..currentFunctionID.."_instr"..instr.instrnum..";\n")
		instr.jumpTarget = instr.instrnum
	end,

	ILOP_jne_p = function(instr)
		flushFrame(instr, true)

		local p1 = retrieveValue(instr, instr.src1, runtimePointerSize, true);
		local p2 = retrieveValue(instr, instr.src2, runtimePointerSize, true);

		instr:appendCode("\tif(memcmp("..p1..".valueRef, "..p2..".valueRef, "..instr.size.."))\n")
		instr:appendCode("\t\tgoto __func"..currentFunctionID.."_instr"..instr.instrnum..";\n")
		instr.jumpTarget = instr.instrnum

		flushFrame(instr)
	end,

	ILOP_xnullref = function(instr)
		flushFrame(instr)
		instr:appendCode("\tNULLREFEXCEPTION("..instr.instructionNumber..");\n")
	end,

	ILOP_catch = function(instr)
		flushFrame(instr)
		instr:appendCode("\tINVALIDOPERATIONEXCEPTION("..instr.instructionNumber..");\n")
		markNextInstruction(instr)
	end,

	ILOP_fatal = function(instr)
		flushFrame(instr)
		instr:appendCode("\tINVALIDOPERATIONEXCEPTION("..instr.instructionNumber..");\n")
	end,

	ILOP_throw = function(instr)
		flushFrame(instr, true)

		local exceptionInstance = retrieveValue(instr, instr.src, addressSize, true)
		instr:appendCode("\tTHROWEXCEPTION("..exceptionInstance..".p, "..instr.instructionNumber..");\n")
	end,

	ILOP_hardenstack = function(instr)
		flushFrame(instr)
	end,
	
	ILOP_switch = function(instr)
		flushFrame(instr, true)

		local switchVal = retrieveValue(instr, instr.src, instr.size, true)
		instr:appendCode("\t{\n")
		instr:appendCode("\t\tconst void *pv = &"..switchVal..";\n")
		instr:appendCode("\t\tconst UInt8 *caseBytes = static_cast<const UInt8 *>(resArgs["..instr.cases.."]);\n")
		for i=1,instr.numCases do
			local iz = i - 1
			instr:appendCode("\t\tif(!memcmp(caseBytes + "..(iz * instr.size)..", pv, "..instr.size.."))\n");
			instr:appendCode("\t\t\tgoto __func"..currentFunctionID.."_instr"..(instr.instructionNumber + i)..";\n")
		end
		instr:appendCode("\t\tgoto __func"..currentFunctionID.."_instr"..(instr.instructionNumber + instr.numCases + 1)..";\n")
		instr:appendCode("\t}\n")
	end,
	
	ILOP_switch_ptr = function(instr)
		flushFrame(instr, true)

		local switchVal = retrieveValue(instr, instr.src, instr.size, true)
		instr:appendCode("\t{\n")
		instr:appendCode("\t\tconst void *pv = "..switchVal..".rtp.valueRef;\n")
		instr:appendCode("\t\tconst UInt8 *caseBytes = static_cast<const UInt8 *>(resArgs["..instr.cases.."]);\n")
		for i=1,instr.numCases do
			local iz = i - 1
			instr:appendCode("\t\tif(!memcmp(caseBytes + "..(iz * instr.size)..", pv, "..instr.size.."))\n");
			instr:appendCode("\t\t\tgoto __func"..currentFunctionID.."_instr"..(instr.instructionNumber + i)..";\n")
		end
		instr:appendCode("\t\tgoto __func"..currentFunctionID.."_instr"..(instr.instructionNumber + instr.numCases + 1)..";\n")
		instr:appendCode("\t}\n")
	end,

	----------------------------------------------------------------------------------------
	-- INTRINSICS

	ILOP_iadd = binaryArithmeticOp("+", "isInt"),
	ILOP_isub = binaryArithmeticOp("-", "isInt"),
	ILOP_imul = binaryArithmeticOp("*", "isInt"),
	ILOP_idiv = binaryArithmeticOp("/", "isInt", true),
	ILOP_imod = binaryArithmeticOp("%", "isInt", true),

	ILOP_ineg = unaryArithmeticOp("-", "isInt"),

	ILOP_isx = conversionOp("isInt", "isInt"),
	ILOP_itof = conversionOp("isInt", "isFloat"),

	ILOP_ilt = comparisonOp("<",  "isInt"),
	ILOP_igt = comparisonOp(">",  "isInt"),
	ILOP_ile = comparisonOp("<=", "isInt"),
	ILOP_ige = comparisonOp(">=", "isInt"),
	ILOP_ieq = comparisonOp("==", "isInt"),
	ILOP_ine = comparisonOp("!=", "isInt"),

	ILOP_fadd = binaryArithmeticOp("+", "isFloat"),
	ILOP_fsub = binaryArithmeticOp("-", "isFloat"),
	ILOP_fmul = binaryArithmeticOp("*", "isFloat"),
	ILOP_fdiv = binaryArithmeticOp("/", "isFloat"),
	ILOP_fneg = unaryArithmeticOp("-", "isFloat"),
	ILOP_ftoi = conversionOp("isFloat", "isInt"),

	ILOP_flt = comparisonOp("<",  "isFloat"),
	ILOP_fgt = comparisonOp(">",  "isFloat"),
	ILOP_fle = comparisonOp("<=", "isFloat"),
	ILOP_fge = comparisonOp(">=", "isFloat"),
	ILOP_feq = comparisonOp("==", "isFloat"),
	ILOP_fne = comparisonOp("!=", "isFloat"),
}

function compileMethod(methodStr)
	local method = assert(loadstring(methodStr))()

	if compiledMethodsSet[method.name] then
		-- Duplicate
		return
	end
	compiledMethodsSet[method.name] = true

	currentFunctionID = currentFunctionID + 1

	compiledFunctionInstrCounts[currentFunctionID] = #method.instructions
	compiledFunctionNames[currentFunctionID] = method.name

	outF:write("\n\n\t// ***** "..method.name.."\n")
	outF:write("static int f"..currentFunctionID.."(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {\n")
	outF:write("void *bp = thread->frame.bp;\n")
	outF:write("void *prv = thread->frame.prv;\n")
	outF:write("const Method *m = thread->frame.method;\n")
	outF:write("void **resArgs = m->resArgs;\n")
	outF:write("LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;\n")
	outF:write("goto __func"..currentFunctionID.."_start;\n")
	outF:write("__func"..currentFunctionID.."_instr0:\n")
	outF:write("{\n")

	for idx,instr in ipairs(method.instructions) do
		if currentMarkingInstructions[instr.opcode] then
			instr.active = true
		end
		if nextMarkingInstructions[instr.opcode] then
			if method.instructions[idx+1] then
				method.instructions[idx+1].active = true
			end
		end
		if instrnumMarkingInstructions[instr.opcode] then
			method.instructions[tonumber(instr.instrnum)+1].active = true
		end
		if srcMarkingInstructions[instr.opcode] then
			method.instructions[tonumber(instr.src)+1].active = true
		end
		if exitInstrMarkingInstructions[instr.opcode] then
			method.instructions[tonumber(instr.exitInstr)+1].active = true
		end
		if caseMarkingInstructions[instr.opcode] then
			for c=1,instr.numCases+1 do						-- +1 for the fallthrough case
				method.instructions[idx + c].active = true
			end
		end
	end

	for instrNum,instr in ipairs(method.instructions) do
		instr.code = ""
		instr.preCode = ""
		instr.flush = ""
		instr.instructionNumber = instrNum - 1

		instr.appendCode = function(self, code)
			self.code = self.code..code
		end
		instr.prependCode = function(self, code)
			self.preCode = self.preCode..code
		end

		-- Force a flush on anything that's a valid jump target
		if instr.active then
			flushFrame(instr)
		end

		assert(opcodeEmissions[instr.opcode], instr.opcode)
		opcodeEmissions[instr.opcode](instr)

		if resumeOffsettingInstructions[instr.opcode] then
			instr.nextActive = instr.active
		end

		if instr.nextActive then
			method.instructions[instrNum+1].active = true
		end
	end

	-- Output code
	for idx,instr in ipairs(method.instructions) do
		outF:write("// instr "..(idx-1).."\n")
		outF:write(instr.flush)
		if instr.perforate then
			outF:write("}\n")
		end
		if instr.active then
			outF:write("__func"..currentFunctionID.."_instr"..(idx-1)..":\n")
		end
		if instr.perforate then
			outF:write("{\n")
		end
		outF:write(instr.preCode)
		outF:write(instr.code)
	end

	outF:write("}\n")

	local activeInstructionSet = { }
	local forbiddenResumeSet = { }

	outF:write("__func"..currentFunctionID.."_start:\n")
	outF:write("switch(startInstruction)\n")
	outF:write("{\n")
	for idx,instr in ipairs(method.instructions) do
		if instr.active or idx == 1 then
			outF:write("case "..(idx-1)..": goto __func"..currentFunctionID.."_instr"..(idx-1)..";\n")
			activeInstructionSet[idx] = true
		end
		if instr.forbidResumeOnNext then
			forbiddenResumeSet[idx] = true
		end
	end
	outF:write("};\n")
	compiledFunctionInstrInserts[currentFunctionID] = activeInstructionSet
	compiledFunctionForbiddenResumes[currentFunctionID] = forbiddenResumeSet

	outF:write("\nreturn 0;\n")
	outF:write("\n} // ************* END FUNC\n")

end

function setSizes(addrSize, rtpSize, trtpSize, addrAlign, bSize, bAlign, rtsvSize, evSize)
	unionTypes["void*"] = { size = addrSize, uName = "p" }
	unionTypes["const void*"] = { size = addrSize, uName = "cp" }
	unionTypes["LargeInt"] = { size = addrSize, uName = "li" }
	unionTypes["RuntimePointer<void>"] = { size = rtpSize, uName = "rtp" }
	unionTypes["Bool"] = { size = bSize, uName = "bo" }
	unionTypes["TypedRuntimePointer"] = { size = trtpSize, uName = "trtp" }
	unionTypes["EnumValue"] = { size = evSize, uName = "ev" }

	addressSize = addrSize
	runtimePointerSize = rtpSize
	typedRuntimePointerSize = trtpSize
	addressAlign = addrAlign
	boolSize = bSize
	boolAlign = bAlign
	stackValueSize = tonumber(rtsvSize)
	enumValueSize = evSize
end

outF = assert(io.open(arg[1]..arg[2]..".cpp", "wb"))
headerF = assert(io.open(arg[1]..arg[2]..".hpp", "wb"))
outF:write("#include \""..arg[5].."rdx/rdx.h\"\n")   --pragma warning(disable:4702)\t// Unreachable code\n")
outF:write("#include \""..arg[5].."rdx/rdx_pccm.hpp\"\n")   --pragma warning(disable:4702)\t// Unreachable code\n")
outF:write("#include \""..arg[5].."rdx/rdx_objectmanagement.hpp\"\n")
outF:write("#include \""..arg[5].."rdx/rdx_ilcomp.hpp\"\n")
outF:write("#include \""..arg[5].."rdx/rdx_runtime.hpp\"\n")
outF:write("#include \""..arg[5].."rdx/rdx_lut.hpp\"\n")

outF:write("using namespace RDX;\n")
outF:write("using namespace RDX::ObjectManagement;\n")
outF:write("using namespace RDX::Programmability;\n")
outF:write("using namespace RDX::Programmability::RuntimeUtilities;\n")

outF:write("#include \""..arg[2]..".hpp\"\n")
outF:write("static int ThrowException(RuntimeThread *t, Exception *e, const void *ip)\n")
outF:write("{\n")
outF:write("	t->ex = e;\n")
outF:write("	t->frame.ip = ip;\n")
outF:write("	return RuntimeState::Exception;\n")
outF:write("}\n")
outF:write("#define THROWEXCEPTION(e, instrNum) return ThrowException(thread, static_cast<Exception*>(e), instrTable + instrNum + 1)\n")
outF:write("#define NULLCHECK(v, instrNum) if(v == NULL) NULLREFEXCEPTION(instrNum)\n")
outF:write("#define EXITFRAME do { thread->frame.bp = reinterpret_cast<RuntimeStackFrame *>(bp); return RuntimeState::AbandonFrame; } while(0)\n")

outF:write("#define INVALIDOPERATIONEXCEPTION(instrNum) THROWEXCEPTION(providerDictionary[X_InvalidOperationException], instrNum)\n")
outF:write("#define INCOMPATIBLECONVERSION(instrNum) THROWEXCEPTION(providerDictionary[X_IncompatibleConversionException], instrNum)\n")
outF:write("#define NULLREFEXCEPTION(instrNum) THROWEXCEPTION(providerDictionary[X_NullReferenceException], instrNum)\n")
outF:write("#define DIVIDEBYZEROEXCEPTION(instrNum) THROWEXCEPTION(providerDictionary[X_DivideByZeroException], instrNum)\n")
outF:write("#define OUTOFBOUNDS(instrNum) THROWEXCEPTION(providerDictionary[X_IndexOutOfBoundsException], instrNum)\n")

outF:write("#include \""..arg[5].."rdx/rdx_pccm_private.hpp\"\n")

outF:write("#define TICK(instrNum)	do {\\\n")
outF:write("		if((--thread->timeout) == 0)\\\n")
outF:write("		{\\\n")
outF:write("			thread->frame.ip = instrTable + (instrNum + 1);\\\n")
outF:write("			return RuntimeState::TimedOut;\\\n")
outF:write("		}\\\n")
outF:write("	} while(0)\n")


local inScript = assert(loadfile(arg[3]))

inScript()

outF:write("// GIT format: Positive = -(value) to function encode, resume allowed\n")
outF:write("//             Negative = -(value) to function encode, resume forbidden\n")
outF:write("//             Zero     = Invalid, execution never stops or resumes at this point\n")
do
	local insertIndex = 0
	outF:write("static LargeInt globalInstructionTable[] = {")
	-- Output encoded instr table
	for fid,instrCount in ipairs(compiledFunctionInstrCounts) do
		outF:write("\n\t// f"..fid..", "..compiledFunctionNames[fid])
		compiledFunctionInsertIndexes[fid] = insertIndex
		local activeInstrSet = compiledFunctionInstrInserts[fid]
		local forbiddenResumeSet = compiledFunctionForbiddenResumes[fid]

		local toBreak = 0
		for i=0,instrCount do
			if toBreak == 0 then
				outF:write("\n\t")
				toBreak = 31
			end
			toBreak = toBreak - 1
			if activeInstrSet[i] then
				if forbiddenResumeSet[i] then
					outF:write("-"..i..", ")
				else
					outF:write(i..", ")
				end
			else
				outF:write("0, ")
			end
		end
		insertIndex = insertIndex + instrCount + 1
	end
	outF:write("\n};\n")
end

do
	outF:write("static StaticLookupTable<StaticLookupStringKey<char, char>, PrecompiledFunctionInfo>::Entry functionTable[] =\n")
	outF:write("{\n")
	for fid,ii in ipairs(compiledFunctionInsertIndexes) do
		outF:write("\t{ \""..compiledFunctionNames[fid].."\", { "..fid.." } },\n")
	end
	outF:write("};\n")


	outF:write("static void InitializeCompiledRDX()\n")
	outF:write("{\n")
	for fid,ii in ipairs(compiledFunctionInsertIndexes) do
		outF:write("\tglobalInstructionTable["..ii.."] = reinterpret_cast<const char *>(f"..fid..") - reinterpret_cast<const char *>(NULL);\n")
		outF:write("\tfunctionTable["..(fid-1).."].value.compiledInstructions = globalInstructionTable + "..(ii + 1)..";\n")
	end
	outF:write("}\n")
	outF:write("static AutoRunFunction compileInitializer(InitializeCompiledRDX);\n")

	-- This needs to happen after InitializeCompiledRDX because the LUT will be shuffled into hash lookup order by the LUT initializer
	outF:write("static RDX::StaticLookupTable<RDX::StaticLookupStringKey<char, char>, PrecompiledFunctionInfo> functionLookup(functionTable, sizeof(functionTable)/sizeof(functionTable[0]));\n")

	-- Write the export function
	do
		local indentation = ""
		local current
		current = arg[4]
		while true do
			local prefix, suffix = string.match(current, "(.-)::(.+)")
			if suffix == nil then
				break
			end

			outF:write(indentation.."namespace "..prefix.."\n")
			outF:write(indentation.."{\n")

			indentation = indentation.."\t"
			current = suffix
		end

		outF:write(indentation.."PrecompiledCodeModule "..current.."(globalInstructionTable, sizeof(globalInstructionTable), &functionLookup);\n")
		while #indentation > 0 do
			indentation = string.sub(indentation, 1, #indentation-1)
			outF:write(indentation.."}\n")
		end
	end

	-- Write address pack func
	headerF:write("inline LargeInt PackAddress(")
	for i=1,tonumber(addressSize) do
		if i ~= 1 then
			headerF:write(", ")
		end
		headerF:write("UInt8 v"..i)
	end
	headerF:write(")\n")
	headerF:write("{\n")
	headerF:write("\tunion { LargeInt addr; UInt8 b8["..addressSize.."]; } u;\n")
	for i=1,tonumber(addressSize) do
		headerF:write("\tu.b8["..(i-1).."] = v"..i..";\n")
	end
	headerF:write("\treturn u.addr;\n")
	headerF:write("}\n\n")

end

outF:close()