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
local AddRelevantSymbol

local duplicatableNamespaces =
{
	"#",
}

local encodedArgCats =
{
	int1 = "intArgs",
	int2 = "intArgs",
	intVar = "intArgs",
	str1 = "stringArgs",
	str2 = "stringArgs",
	res1 = "resArgs",
}

-- Emitters return the number of emitted values, and the emitted bytes for the line
local emitEscapedRes = function(cs, bcPacker, a, resTable)
	if not resTable.knownRes[a] then
		resTable.resources[#resTable.resources + 1] = a
		resTable.knownRes[a] = #resTable.resources
	end
	RDXC.Native.bytecodePackerAddInteger(bcPacker, resTable.knownRes[a] - 1)
end

local encodedArgCatEmitters =
{
	stringArgs = function(cs, bcPacker, a, resTable)
		if a ~= nil then
			emitEscapedRes(cs, bcPacker, "'"..escapestring(tostring(a)).."'", resTable)
		else
			emitEscapedRes(cs, bcPacker, "null", resTable)
		end
	end,
	intArgs = function(cs, bcPacker, a, resTable)
		if type(a) == "table" then
			RDXC.Native.bytecodePackerAddInteger(bcPacker, #a)
			
			for _,i in ipairs(a) do
				RDXC.Native.bytecodePackerAddInteger(bcPacker, i)
			end
			return
		end

		RDXC.Native.bytecodePackerAddInteger(bcPacker, a)
	end,
	resArgs = function(cs, bcPacker, a, resTable)
		if a == nil then
			emitEscapedRes(cs, bcPacker, "null", resTable)
		else
			AddRelevantSymbol(cs, a)
			emitEscapedRes(cs, bcPacker, "res "..encodeRes(a), resTable)
		end
	end,
}

local opcodeNames =
{
	"startbarrier",
	"endbarrier",
	"throw",
	"try",
	"catch",
	"trycatch",
	"jump",
	"jumpif",
	"jumpifnot",
	"jumpiftrue",
	"jumpiffalse",
	"jumpifnull",
	"jumpifnotnull",
	"jumpifequal",
	"jumpifnotequal",
	"call",
	"callvirtual",
	"calldelegate",
	"alloclocal",
	"createlocal",
	"removelocal",
	"pushempty",
	"newinstance",
	"newinstanceset",
	"null",
	"pinlocal",
	"tovarying",
	"arrayindex",
	"property",
	"move",
	"load",
	"clone",
	"pop",
	"cast",
	"localref",
	"return",
	"hash",
	"res",
	"constant",
	"constant_str",
	"switch",
	"iteratearray",
}

local opcodeNumbers = { }

for idx,op in ipairs(opcodeNames) do
	opcodeNumbers[op] = idx - 1
end

local opcodeArgs =
{
	startbarrier = { "int1" },
	endbarrier = { },
	throw = { },
	try = { "int1", "int2" },
	catch = { "res1" },
	trycatch = { "int1", "res1" },
	jump = { "int1" },
	jumpif = { "int1", "res1" },
	jumpifnot = { "int1", "res1" },
	jumpiftrue = { "int1" },
	jumpiffalse = { "int1" },
	jumpifnull = { "int1" },
	jumpifnotnull = { "int1" },
	jumpifequal = { "int1" },
	jumpifnotequal = { "int1" },
	call = { "res1" },
	callvirtual = { "res1" },
	calldelegate = { "res1" },
	alloclocal = { "res1" },
	createlocal = { "res1" },
	removelocal = { },
	pushempty = { "res1" },
	newinstance = { "int1", "res1" },
	newinstanceset = { "int1", "intVar", "res1" },
	null = { },
	pinlocal = { },
	tovarying = { },
	arrayindex = { "int1" },
	property = { "int1" },
	move = { },
	load = { },
	clone = { "int1", "int2" },
	pop = { },
	cast = { "res1" },
	localref = { "int1" },
	["return"] = { "int1" },
	hash = { },
	res = { "res1" },
	constant = { "int1", "res1" },
	constant_str = { "str1", "res1" },
	switch = { "int1", "res1" },
	iteratearray = { "int1", "int2" },
}

setglobal("escapestring", function(str)
	local rv = ""
	for i=1,#str do
		local b = string.byte(str, i)
		if b < 32 then
			if b < 10 then
				rv = rv.."\\0"..b
			else
				rv = rv.."\\"..b
			end
		elseif b == 92 then
			rv = rv.."\\\\"
		elseif b == 39 then
			rv = rv.."\\'"
		else
			rv = rv..string.sub(str, i, i)
		end
	end

	return rv
end )

local includedNamespaces



local function AddRelevantSymbolSet(cs, dependencySet)
	for k in pairs(dependencySet) do
		AddRelevantSymbol(cs, k)
	end
end

local function SymbolIncluded(sym)
	for _,nsname in ipairs(includedNamespaces) do
		if string.sub(sym, 1, #nsname) == nsname then
			return true
		end
	end
	return false
end

local function SymbolIsDuplicate(sym)
	for _,nsname in ipairs(duplicatableNamespaces) do
		if string.sub(sym, 1, #nsname) == nsname then
			return true
		end
	end
	return false
end

local function SymbolIsRelevant(sym)
	return SymbolIsDuplicate(sym) or SymbolIncluded(sym)
end

local function ForceEmitArrayOfType(cs, f, tn)
	local atn = "#"..tn.."[C]"
	if cs.gst[atn] then
		return
	end

	assert(cs.gst[tn])

	cs.gst[atn] = {
		type = "CArrayOfType",
		containedType = cs.gst[tn],
		dimensions = 1,
		isCompiled = true,
		longName = atn,
		isConst = true,
	}

	AddRelevantSymbol(cs, tn)
end

local function EmitArrayOfType(cs, f, aot)
	AddRelevantSymbol(cs, aot.containedType.longName)

	AddRelevantSymbol(cs, "Core.RDX.ArrayOfType")
	f:write("arraydef "..encodeRes(aot.longName))

	f:write(" "..encodeRes(aot.containedType.longName))
	if aot.isConst then
		f:write(" const")
	end
	f:write(" "..aot.dimensions)
	f:write("\n\n")
end

local function OpcodeIndex(cs, opname)
	local opcodeEnumerants = cs.gst["Core.RDX.Method.Opcode"].enumerants
end

local function EmitInstructions(cs, f, instructions, methodName, resTable)
	AddRelevantSymbol(cs, "#Core.byte[C]")
	ForceEmitArrayOfType(cs, f, "Core.byte")
	AddRelevantSymbol(cs, "#Core.Object[C]")
	ForceEmitArrayOfType(cs, f, "Core.Object")

	resTable.debugInfoAvailable = false

	local debugInfos = { }

	local lastFilename = nil
	local lastLine = nil
	local currentDebugInfo = nil
	
	local bcPacker = RDXC.Native.createBytecodePacker()

	for instrIndex,instr in ipairs(instructions) do
		if instr.res1 ~= nil then
			AddRelevantSymbol(cs, instr.res1)
		end

		-- Determine debug info and packed args
		RDXC.Native.bytecodePackerAddInteger(bcPacker, opcodeNumbers[instr.op])
		for _,argName in ipairs(opcodeArgs[instr.op]) do
			local argCat = encodedArgCats[argName]
			local catEmitter = encodedArgCatEmitters[argCat]
			catEmitter(cs, bcPacker, instr[argName], resTable)
		end

		if instr.filename then
			lastFilename = instr.filename
		end

		if instr.line then
			lastLine = instr.line
		end

		if emitDebugInfo and (instr.filename or instr.line) then
			debugInfos[#debugInfos+1] = currentDebugInfo
			currentDebugInfo = { filename = lastFilename, line = instr.line, firstInstruction = instrIndex - 1 }
		end
	end
	
	local finishedBytecode = RDXC.Native.finishBytecodePacker(bcPacker)

	if methodName == "Core.Collections.HashSetBase/methods/RemoveKey(Core.largeuint)" then
		print("BCPacker Size: "..#finishedBytecode)
	end
	
	f:write("def "..encodeRes(methodName.."/bytecode").." "..encodeRes("#Core.byte[C]").." "..#finishedBytecode.."\n")
	f:write("{\n")
	f:write(RDXC.Native.makeBytecodeASCII(finishedBytecode))
	f:write("}\n\n")

	debugInfos[#debugInfos+1] = currentDebugInfo


	if #debugInfos ~= 0 then
		resTable.debugInfoAvailable = true

		AddRelevantSymbol(cs, "#Core.RDX.InstructionFileInfo[C]")
		ForceEmitArrayOfType(cs, f, "Core.RDX.InstructionFileInfo")

		f:write("def "..encodeRes(methodName.."/debugInfo").." "..encodeRes("#Core.RDX.InstructionFileInfo[C]").." "..#debugInfos.."\n")
		f:write("{\n")
		for _,debugInfo in ipairs(debugInfos) do
			f:write("\t{\n")
			assert(debugInfo.filename, "No file info for instruction "..debugInfo.firstInstruction)
			f:write("\t\tfilename : '"..escapestring(debugInfo.filename).."',\n")
			f:write("\t\tline : "..debugInfo.line..",\n")
			f:write("\t\tfirstInstruction : "..debugInfo.firstInstruction..",\n")
			f:write("\t},\n")
		end
		f:write("}\n\n")
	end

	if #resTable.resources ~= 0 then
		f:write("def "..encodeRes(methodName.."/resArgs").." "..encodeRes("#Core.Object[C]").." "..#resTable.resources.."\n")
		f:write("{\n")
		for _,res in ipairs(resTable.resources) do
			f:write("\t"..res..",\n")
		end
		f:write("}\n\n")
	end
end

local function EmitMethod(cs, f, m, pass)
	AddRelevantSymbol(cs, m.actualParameterList.longName)
	AddRelevantSymbol(cs, m.returnTypes.longName)

	if asmDumpFile and pass == nil then
		asmDumpFile:write("method "..m.longName.."\n")
		asmDumpFile:write("\tparameters : "..m.actualParameterList.prettyName.."\n")
		asmDumpFile:write("\treturnTypes : "..m.returnTypes.longName.."\n")
		asmDumpFile:write("\tvftIndex : "..(m.vftIndex or -1).."\n")
		if m.thisParameterOffset then
			asmDumpFile:write("\tthisParameterOffset : "..(m.thisParameterOffset).."\n")
		end
		if m.compiledInstructions then
			asmDumpFile:write("instructions:\n")
			for instrNum,instr in ipairs(m.compiledInstructions) do
				if instr.filename then
					asmDumpFile:write("--- File '"..instr.filename.."'\n")
				end
				if instr.line then
					asmDumpFile:write("--- Line "..instr.line.."\n")
				end

				asmDumpFile:write(instrNum.."\t: ")
				asmDumpFile:write(instr.op)
				if instr.str1 then
					asmDumpFile:write(" <str1: '")
					asmDumpFile:write(instr.str1)
					asmDumpFile:write("'>")
				end
				if instr.int1 then
					asmDumpFile:write(" <int1: ")
					asmDumpFile:write(instr.int1)
					asmDumpFile:write(">")
				end
				if instr.intVar then
					asmDumpFile:write(" <intVar: ")
					for idx,v in ipairs(instr.intVar) do
						if idx ~= 1 then
							asmDumpFile:write(", ")
						end
						asmDumpFile:write(v)
					end
					asmDumpFile:write(">")
				end
				if instr.int2 then
					asmDumpFile:write(" <int2: '")
					asmDumpFile:write(instr.int2)
					asmDumpFile:write("'>")
				end
				if instr.res1 then
					asmDumpFile:write(" <res1: res '")
					asmDumpFile:write(instr.res1)
					asmDumpFile:write("'>")
				end
				asmDumpFile:write("\n")
			end
		end
		asmDumpFile:write("\n")
	end

	local packsNonEmpty = { }
	local debugInfoAvailable = false
	local resTable = { knownRes = { }, resources = { } }

	if m.compiledInstructions then
		EmitInstructions(cs, f, m.compiledInstructions, m.longName, resTable)

		debugInfoAvailable = resTable.debugInfoAvailable
	end

	AddRelevantSymbol(cs, "Core.RDX.Method")
	f:write("def "..encodeRes(m.longName).." "..encodeRes("Core.RDX.Method").." 1\n")
	f:write("{\n")
	f:write("\tparameters : res "..encodeRes(m.actualParameterList.longName)..",\n")
	f:write("\treturnTypes : res "..encodeRes(m.returnTypes.longName)..",\n")
	if m.vftIndex then
		f:write("\tvftIndex : "..(m.vftIndex)..",\n")
	end
	if m.thisParameterOffset then
		f:write("\tthisParameterOffset : "..(m.thisParameterOffset)..",\n")
	end
	if m.compiledInstructions then
		f:write("\tbytecode : res "..encodeRes(m.longName.."/bytecode")..",\n")
		f:write("\tnumInstructions : "..#m.compiledInstructions..",\n")
	else
		f:write("\tbytecode : null,\n")
	end
	if m.isAbstract then
		f:write("\tisAbstract : true,\n")
	end

	if #resTable.resources ~= 0 then
		f:write("\tresArgs : res "..encodeRes(m.longName.."/resArgs")..",\n")
	end
	if debugInfoAvailable then
		f:write("\tinstructionFileInfos : res "..encodeRes(m.longName.."/debugInfo")..",\n")
	end

	f:write("}\n\n")
end

local function EmitEnumerants(cs, f, st)
	AddRelevantSymbol(cs, "#Core.RDX.Enumerant[C]")
	ForceEmitArrayOfType(cs, f, "Core.RDX.Enumerant")

	f:write("def "..encodeRes(st.longName.."/enumerants").." "..encodeRes("#Core.RDX.Enumerant[C]").." "..#st.enumerants.."\n")
	f:write("{\n")

	for _,e in ipairs(st.enumerants) do
		f:write("\t{\n")
		f:write("\t\tname : '"..e.name.."',\n")
		f:write("\t\tvalue : "..e.value..",\n")
		f:write("\t},\n")
	end
	f:write("}\n\n")
end


local function EmitInterfaces(cs, f, st)
	for _,i in ipairs(st.interfaces) do
		AddRelevantSymbol(cs, i.interfaceType.longName)
	end

	ForceEmitArrayOfType(cs, f, "Core.RDX.InterfaceImplementation")

	AddRelevantSymbol(cs, "#Core.RDX.InterfaceImplementation[C]")
	f:write("def "..encodeRes(st.longName.."/interfaces").." "..encodeRes("#Core.RDX.InterfaceImplementation[C]").." "..#st.interfaces.."\n")
	f:write("{\n")

	for _,i in ipairs(st.interfaces) do
		AddRelevantSymbol(cs, i.interfaceType.longName)

		f:write("\t{\n")
		f:write("\t\ttype : res "..encodeRes(i.interfaceType.longName)..",\n")
		f:write("\t\tvftOffset : "..(i.vftOffset-1)..",\n")
		f:write("\t},\n")
	end

	f:write("}\n\n")
end

local function EmitProperties(cs, f, st)
	for _,p in ipairs(st.properties) do
		AddRelevantSymbol(cs, p.typeOf.refType.longName)
	end

	ForceEmitArrayOfType(cs, f, "Core.RDX.Property")

	AddRelevantSymbol(cs, "#Core.RDX.Property[C]")
	f:write("def "..encodeRes(st.longName.."/properties").." "..encodeRes("#Core.RDX.Property[C]").." "..#st.properties.."\n")
	f:write("{\n")

	for _,p in ipairs(st.properties) do
		f:write("\t{\n")
		if p.name == nil then
			f:write("\t\tname : null,\n")
		else
			f:write("\t\tname : '"..p.name.."',\n")
		end
		if p.isConst then
			f:write("\t\tisConstant : true,\n")
		end
		f:write("\t\ttype : res "..encodeRes(p.typeOf.refType.longName)..",\n")
		if p.mustBeConst then
			f:write("\t\tmustBeConstant : true,\n")
		end
		f:write("\t},\n")
	end

	f:write("}\n\n")
end

local function EmitVFT(cs, f, st)
	ForceEmitArrayOfType(cs, f, "Core.RDX.Method")

	AddRelevantSymbol(cs, "#Core.RDX.Method[C]")
	f:write("def "..encodeRes(st.longName.."/vft").." "..encodeRes("#Core.RDX.Method[C]").." "..#st.virtualMethods.."\n")
	f:write("{\n")
	for _,m in ipairs(st.virtualMethods) do
		AddRelevantSymbol(cs, m.longName)
		f:write("\tres "..encodeRes(m.longName)..",\n")
	end
	f:write("}\n\n")
end

local function EmitDelegateType(cs, f, dt, pass)
	AddRelevantSymbol(cs, dt.longName)
	AddRelevantSymbol(cs, "Core.RDX.DelegateType")
	AddRelevantSymbol(cs, dt.actualParameterList.longName)
	AddRelevantSymbol(cs, dt.returnTypes.longName)

	-- NOTE: refName might not be the same as longName, but this might not be a real delegate, it might
	-- be a fake delegate from EmitBoundDelegateType
	if pass == 1 or pass == nil then
		f:write("def "..encodeRes(dt.longName).." "..encodeRes("Core.RDX.DelegateType").." 1\n")
		f:write("{\n")
		f:write("\tparameters : res "..encodeRes(dt.actualParameterList.longName)..",\n")
		f:write("\treturnTypes : res "..encodeRes(dt.returnTypes.longName)..",\n")
		f:write("}\n\n")
	end
end

local function EmitBoundDelegateType(cs, f, bdt, pass)
	AddRelevantSymbol(cs, bdt.longName)
	AddRelevantSymbol(cs, bdt.actualParameterList.longName)
	AddRelevantSymbol(cs, bdt.returnTypes.longName)
	AddRelevantSymbol(cs, "Core.RDX.StructuredType")
	ForceEmitArrayOfType(cs, f, "Core.RDX.Method")
	AddRelevantSymbol(cs, "#Core.RDX.Method[C]")

	if pass == 1 or pass == nil then
		f:write("def "..encodeRes(bdt.longName).." "..encodeRes("Core.RDX.StructuredType").." 1\n")
		f:write("{\n")

		f:write("\tstorageSpecifier : 'SS_Class',\n")
		f:write("\tvirtualMethods : res "..encodeRes(bdt.longName.."/vft")..",\n")
		f:write("\tisAbstract : true,\n")
		f:write("}\n\n")

		f:write("def "..encodeRes(bdt.longName.."/vft").." "..encodeRes("#Core.RDX.Method[C]").." 1\n")
		f:write("{\n")
		f:write("\tres "..encodeRes(bdt.invokeName).."\n")
		f:write("}\n\n")

		f:write("def "..encodeRes(bdt.invokeName).." "..encodeRes("Core.RDX.Method").." 1\n")
		f:write("{\n")
		f:write("\tparameters : res "..encodeRes(bdt.actualParameterList.longName)..",\n")
		f:write("\treturnTypes : res "..encodeRes(bdt.returnTypes.longName)..",\n")
		f:write("\tvftIndex : 1,\n")
		f:write("\tthisParameterOffset : 1,\n")
		f:write("\tisAbstract : true,\n")
		f:write("}\n\n")
	end
end

local function EmitBoundDelegateMarshal(cs, f, marshal, pass)
	AddRelevantSymbol(cs, marshal.longName)
	AddRelevantSymbol(cs, marshal.returnTypes.longName)
	if marshal.actualParameterList.longName == nil then
		print("PList wasn't compiled?")
		print(marshal.actualParameterList.type)
		print(marshal.actualParameterList.isCompiled)
	end
	AddRelevantSymbol(cs, marshal.actualParameterList.longName)
	AddRelevantSymbol(cs, marshal.owner.longName)
	AddRelevantSymbol(cs, "Core.RDX.StructuredType")
	ForceEmitArrayOfType(cs, f, "Core.RDX.Method")
	AddRelevantSymbol(cs, "#Core.RDX.Method[C]")

	if marshal.thisType then
		AddRelevantSymbol(cs, marshal.thisType.longName)
		ForceEmitArrayOfType(cs, f, "Core.RDX.Property")
		AddRelevantSymbol(cs, "#Core.RDX.Property[C]")
	end

	if pass == 1 or pass == nil then
		f:write("def "..encodeRes(marshal.longName).." "..encodeRes("Core.RDX.StructuredType").." 1\n")
		f:write("{\n")
		f:write("\tparentClass : res "..encodeRes(marshal.owner.longName)..",\n")
		f:write("\tstorageSpecifier : 'SS_Class',\n")
		f:write("\tvirtualMethods : res "..encodeRes(marshal.longName.."/vft")..",\n")
		if marshal.thisType then
			f:write("\tproperties : res "..encodeRes(marshal.longName.."/properties")..",\n")
		end
		f:write("\tisFinal : true,\n")
		f:write("}\n\n")

		f:write("def "..encodeRes(marshal.longName.."/vft").." "..encodeRes("#Core.RDX.Method[C]").." 1\n")
		f:write("{\n")
		f:write("\tres "..encodeRes(marshal.invokeName)..",\n")
		f:write("}\n\n")

		if marshal.thisType then
			f:write("def "..encodeRes(marshal.longName.."/properties").." "..encodeRes("#Core.RDX.Property[C]").." 1\n")
			f:write("{\n")
			f:write("\t{\n")
			f:write("\t\tname : '.DelegateValue',\n")
			f:write("\t\ttype : res "..encodeRes(marshal.thisType.longName)..",\n")
			f:write("\t}\n")
			f:write("}\n\n")
		end

		local resTable = { knownRes = { }, resources = { } }
		EmitInstructions(cs, f, marshal.instructions, marshal.invokeName, resTable)

		f:write("def "..encodeRes(marshal.invokeName).." "..encodeRes("Core.RDX.Method").." 1\n")
		f:write("{\n")
		f:write("\tparameters : res "..encodeRes(marshal.actualParameterList.longName)..",\n")
		f:write("\treturnTypes : res "..encodeRes(marshal.returnTypes.longName)..",\n")
		f:write("\tnumInstructions : "..#marshal.instructions..",\n")
		f:write("\tbytecode : res "..encodeRes(marshal.invokeName.."/bytecode")..",\n")
		f:write("\tvftIndex : 1,\n")
		f:write("\tthisParameterOffset : 1,\n")

		if #resTable.resources ~= 0 then
			f:write("\tresArgs : res "..encodeRes(marshal.invokeName.."/resArgs")..",\n")
		end
		f:write("}\n")
	end
end

local function EmitStructuredType(cs, f, st, pass)
	local emitVFT
	local emitProperties
	local emitInterfaces

	if st.isTemplate then
		return
	end

	AddRelevantSymbol(cs, st.longName)

	AddRelevantSymbol(cs, "Core.RDX.StructuredType")

	if pass == 1 or pass == nil then
		f:write("def "..encodeRes(st.longName).." "..encodeRes("Core.RDX.StructuredType").." 1\n")
		
		if st.defaultValue then
			AddRelevantSymbol(cs, st.defaultValue.longName)
			AddRelevantSymbolSet(cs, st.defaultDependencySet)
			f:write("\tdefaultValue : res "..encodeRes(st.defaultValue.longName).."\n")
		else
			f:write("\tdefaultValue : null\n")
		end
		
		f:write("{\n")

		if st.baseClass == nil then
			f:write("\tparentClass : null,\n")
		else
			AddRelevantSymbol(cs, st.baseClass.longName)
			f:write("\tparentClass : res "..encodeRes(st.baseClass.longName)..",\n")
		end

		if st.declType == "struct" then
			if st.byVal then
				f:write("\tstorageSpecifier : 'SS_ValStruct',\n")
			else
				f:write("\tstorageSpecifier : 'SS_RefStruct',\n")
			end
			f:write("\tenumerants : null,\n")
		elseif st.declType == "interface" then
			f:write("\tstorageSpecifier : 'SS_Interface',\n")
			f:write("\tenumerants : null,\n")
		elseif st.declType == "class" then
			f:write("\tstorageSpecifier : 'SS_Class',\n")
			f:write("\tenumerants : null,\n")
		elseif st.declType == "enum" then
			f:write("\tstorageSpecifier : 'SS_Enum',\n")
			f:write("\tenumerants : res "..encodeRes(st.longName.."/enumerants")..",\n")
		else
			error("Unsupported ST decl type");
		end
	end

	if #st.interfaces ~= 0 then
		local typeInterfaces = st.aliasInterfaces
		if typeInterfaces == nil then
			typeInterfaces = st
			emitInterfaces = true
		end
		AddRelevantSymbol(cs, typeInterfaces.longName.."/interfaces")

		if pass == 1 then
			f:write("\tinterfaces : res "..encodeRes(typeInterfaces.longName.."/interfaces")..",\n")
		end
	else
		if pass == 1 then
			f:write("\tinterfaces : null,\n")
		end
	end

	if #st.virtualMethods ~= 0 then
		local typeVFT = st.aliasVFT
		if typeVFT == nil then
			typeVFT = st
			emitVFT = true
		end
		AddRelevantSymbol(cs, typeVFT.longName.."/vft")
		if pass == 1 then
			f:write("\tvirtualMethods : res "..encodeRes(typeVFT.longName.."/vft")..",\n")
		end
	else
		if pass == 1 then
			f:write("\tvirtualMethods : null,\n")
		end
	end

	if #st.properties == 0 then
		if pass == 1 then
			f:write("\tproperties : null,\n")
		end
	else
		local typeProperties = st.aliasProperties
		if typeProperties == nil then
			typeProperties = st
			emitProperties = true
		end

		AddRelevantSymbol(cs, typeProperties.longName.."/properties")
		if pass == 1 then
			f:write("\tproperties : res "..encodeRes(typeProperties.longName.."/properties")..",\n")
		end
	end

	if pass == 1 then
		if st.isFinal then
			f:write("\tisFinal : true,\n")
		end
		if st.isAbstract then
			f:write("\tisAbstract : true,\n")
		end
		if st.isLocalized then
			f:write("\tisLocalized : true,\n")
		end

		f:write("}\n\n")
	end

	if pass == 2 or pass == nil then
		if emitInterfaces then
			EmitInterfaces(cs, f, st)
		end

		if emitVFT then
			EmitVFT(cs, f, st)
		end
	end

	if pass == 1 or pass == nil then
		if emitProperties then
			EmitProperties(cs, f, st)
		end

		if st.declType == "enum" then
			EmitEnumerants(cs, f, st)
		end
	end
end

local function EmitTypeTuple(cs, f, tt)
	ForceEmitArrayOfType(cs, f, "Core.RDX.Type")

	AddRelevantSymbol(cs, "#Core.RDX.Type[C]")
	f:write("def "..encodeRes(tt.longName).." "..encodeRes("#Core.RDX.Type[C]").." "..#tt.typeReferences.."\n")
	f:write("{\n")
	for _,tr in ipairs(tt.typeReferences) do
		AddRelevantSymbol(cs, tr.refType.longName)
		f:write("\tres "..encodeRes(tr.refType.longName)..",\n")
	end
	f:write("}\n\n")
end

local function EmitParameterList(cs, f, pl)
	ForceEmitArrayOfType(cs, f, "Core.RDX.MethodParameter")

	AddRelevantSymbol(cs, "#Core.RDX.MethodParameter[C]")
	f:write("def "..encodeRes(pl.longName).." "..encodeRes("#Core.RDX.MethodParameter[C]").." "..#pl.parameters.."\n")
	f:write("{\n")
	for _,p in ipairs(pl.parameters) do
		f:write("\t{\n")
		AddRelevantSymbol(cs, p.type.refType.longName)
		f:write("\t\ttype : res "..encodeRes(p.type.refType.longName)..",\n")
		if p.isNotNull then
			f:write("\t\tisNotNull : true,\n")
			f:write("\t\tisConstant : true,\n")
		elseif p.isConst then
			f:write("\t\tisConstant : true,\n")
		end
		
		f:write("\t},\n")
	end
	f:write("}\n\n")
end


local function EmitDefaultInstance(cs, f, di)
	AddRelevantSymbol(cs, di.typeOf)

	local anonTag = ""

	if di.isAnonymous and not SymbolIsDuplicate(di.longName) then
		anonTag = "anonymous cloaked "
	end

	f:write("def "..encodeRes(di.longName).." "..anonTag..encodeRes(di.typeOf))

	if di.dimensions then
		if #di.dimensions > 1 then
			local total = 1
			for _,d in ipairs(di.dimensions) do
				total = total * d
			end
			f:write(" "..total)
		elseif #di.dimensions == 0 then
			f:write(" 1")	-- ???
		end
		for _,d in ipairs(di.dimensions) do
			f:write(" "..d)
		end
	else
		f:write(" 1")
	end

	f:write("\n")

	f:write(di.value)
	f:write("\n\n")
end



local function EmitStaticInstance(cs, f, si)
	local vType = si.typeOf.refType

	AddRelevantSymbol(cs, vType.longName)
	local defLine = "def "..encodeRes(si.longName)

	if si.isAnonymous and not SymbolIsDuplicate(si.longName) then
		defLine = defLine.."anonymous "
	end
	if si.isConst then
		defLine = defLine.."const "
	end

	defLine = defLine.." "..encodeRes(vType.longName)

	f:write(defLine)

	if si.initializer and si.initializer.defaultDimensions and #si.initializer.defaultDimensions ~= 0 then
		if #si.initializer.defaultDimensions > 1 then
			local total = 1
			for _,d in ipairs(si.initializer.defaultDimensions) do
				total = total * d
			end
			f:write(" "..total)
		end
		for _,d in ipairs(si.initializer.defaultDimensions) do
			f:write(" "..d)
		end
	else
		f:write(" 1")
	end

	f:write("\n")
	if si.initializer then
		AddRelevantSymbolSet(cs, si.initializer.defaultDependencySet)

		local needBraces = false
			--(string.sub(tostring(si.initializer.defaultValue), 1, 1) ~= "{")
		if needBraces then
			f:write("{\n")
			f:write("\t"..si.initializer.defaultValue.."\n")
			f:write("}")
		else
			f:write(si.initializer.defaultValue)
		end
	else
		f:write("{\n")
		f:write("}")
	end
	f:write("\n\n")
end

local function EmitSymbol(cs, f, sym, pass)
	local v = cs.gst[sym]

	if v ~= nil then
		if v.type == "CMethod" then
			if pass == 3 or pass == nil then
				EmitMethod(cs, f, v, pass)
			end
		elseif v.type == "CParameterList" then
			if pass == 3 or pass == nil then
				EmitParameterList(cs, f, v)
			end
		elseif v.type == "CStaticDelegateType" then
			if pass == 1 or pass == nil then
				EmitDelegateType(cs, f, v, pass)
			end
		elseif v.type == "CBoundDelegateType" then
			if pass == 1 or pass == nil then
				EmitBoundDelegateType(cs, f, v, pass)
			end
		elseif v.type == "CBoundDelegateMarshal" then
			if pass == 1 or pass == nil then
				EmitBoundDelegateMarshal(cs, f, v, pass)
			end
		elseif v.type == "CStructuredType" then
			if pass == 1 or pass == 2 or pass == nil then
				EmitStructuredType(cs, f, v, pass)
			end
		elseif v.type == "CTypeTuple" then
			if pass == 3 or pass == nil then
				EmitTypeTuple(cs, f, v)
			end
		elseif v.type == "CArrayOfType" then
			if pass == 1 or pass == nil then
				EmitArrayOfType(cs, f, v)
			end
		elseif v.type == "CDefaultInstance" then
			if pass == 3 or pass == nil then
				EmitDefaultInstance(cs, f, v)
			end
		elseif v.type == "CStaticInstance" then
			if pass == 3 or pass == nil then
				EmitStaticInstance(cs, f, v)
			end
		else
			error("Couldn't emit symbol of type "..v.type.." from the GST")
		end
	end
end

AddRelevantSymbol = function(cs, sym)
	if cs.markedSymbols[sym] then
		return
	end

	cs.markedSymbols[sym] = true

	if not SymbolIsRelevant(sym) then
		return
	end

	cs.includedSymbols[sym] = true

	local blackhole = {
		write = function() end,
		seek = function() return 0 end,
	}

	EmitSymbol(cs, blackhole, sym)
end

setglobal("CommitPackageRegistry", function()
	local processedRegistry = { }

	processedRegistry[#processedRegistry+1] = {
		prefix = "#",
		named = "#"
	}

	for _,packageName in ipairs(packageRegistry) do
		processedRegistry[#processedRegistry+1] = {
			prefix = packageName..".",
			named = packageName
		}
	end

	setglobal("packageRegistry", processedRegistry)
end )

setglobal("splitResName", function(resStr)
	for _,packageDef in ipairs(packageRegistry) do
		if string.sub(resStr, 1, #packageDef.prefix) == packageDef.prefix then
			return packageDef.named, string.sub(resStr, #packageDef.prefix + 1, #resStr)
		end
	end

	ncerror("PackageNotDefined", resStr)
end )

setglobal("encodeRes", function(resStr)
	local pkgName, resName = splitResName(resStr)
	return "'"..pkgName.."' '"..resName.."'"
end )

RDXC.Export = function(cs, f, iNamespaces, symbolDumpFileName)
	includedNamespaces = iNamespaces
	assert(iNamespaces)

	local keys = { }

	for k in pairs(cs.gst) do
		keys[#keys+1] = k
	end

	table.sort(keys)

	local typeTypes = set { "CStructuredType", "CArrayOfType" }
	local importedTypes = {
		"Core.RDX.Method",
		"Core.RDX.Property",
		"Core.RDX.ArrayOfType",
		"Core.RDX.StructuredType",
	}
	local exportedTypes = {
		"#Core.RDX.Property[C]",
		"#Core.RDX.Method[C]",
	}

	cs.includedSymbols = { }
	cs.markedSymbols = { }

	for _,k in ipairs(keys) do
		local v = cs.gst[k]

		if SymbolIncluded(k) then
			AddRelevantSymbol(cs, k)
		end
	end

	local importedPackagesSet = { }
	local packagesAppended = { }	-- With appended . for performance

	for idx,pkgName in ipairs(packages) do
		packagesAppended[idx] = pkgName.."."
	end

	for k in pairs(cs.markedSymbols) do
		if not cs.includedSymbols[k] then
			if cs.gst[k] ~= nil and cs.gst[k].isAnonymous then
				ncerror("ReferencedAnonymous", k)
			end
			f:write("import "..encodeRes(k).."\n")
		end
	end

	for emitPass=1,3 do
		for k in pairs(cs.includedSymbols) do
			EmitSymbol(cs, f, k, emitPass)
		end
	end
	
	if symbolDumpFileName then
		local symbols = { }
		for symbol in pairs(cs.markedSymbols) do
			if cs.includedSymbols[symbol] then
				symbols[#symbols+1] = symbol
			end
		end
		
		table.sort(symbols)

		local f = io.open(symbolDumpFileName, "w")
		f:write("Symbols that were included in this package:\n\n")
		for _,symbol in ipairs(symbols) do
			f:write(symbol.."\n")
		end
		f:close()
	end
	
	if cppConfig then
		for symbol in pairs(cs.includedSymbols) do
			CPPExportSymbol(cs, symbol)
		end

		CPPExportPlugin(cs, cs.includedSymbols)
	end
end
