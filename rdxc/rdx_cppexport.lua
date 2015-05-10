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
local GetCPPType
local declTypeTranslations =
{
	["class"] = "class",
	["struct"] = "struct",
	["enum"] = "struct",
	["interface"] = "struct",
}

local declTypePredeclares =
{
	["class"] = "RDX_DECLARE_COMPLEX_NATIVE_CLASS",
	["struct"] = "RDX_DECLARE_COMPLEX_NATIVE_STRUCT",
}

local userPropertyTranslations =
{
	["nativechangesduringload"] = "STUF_NativeChangesDuringLoad",
	["nativesafetoserialize"] = "STUF_NativeSafeToSerialize",
}

local methodNameTranslations =
{
	["#coerce"] = "Coerce",
	["__neg"] = "Negate",
	["__mul"] = "Multiply",
	["__mod"] = "Modulo",
	["__div"] = "Divide",
	["__sub"] = "Subtract",
	["__add"] = "Add",
	["__eq"] = "Equal",
	["__ne"] = "NotEqual",
	["__lt"] = "Less",
	["__le"] = "LessOrEqual",
	["__gt"] = "Greater",
	["__ge"] = "GreaterOrEqual",
	["__index"] = "Index",
	["__setindex"] = "SetIndex",
}

local cppTranslateReservedSet = set
{
	"alignas", "alignof", "auto", "bool", "char", "compl", "constexpr", "const_cast", "decltype", "dynamic_cast",
	"double", "dynamic_cast", "export", "extern", "float", "friend", "goto", "inline", "int", "long", "mutable",
	"noexcept", "nullptr", "operator", "register", "reinterpret_cast", "short", "signed", "sizeof", "static_assert",
	"static_cast", "template", "thread_local", "typeid", "typename", "union", "unsigned", "volatile", "wchar_t",
	
	-- Annoying things from stdlib that cause unwanted behavior
	"EOF",

	-- Internally used names
	"Enum",
}

setglobal("CPPUnreserve", function(sym)
	if cppTranslateReservedSet[sym] then
		return sym.."_"
	end
	return sym
end )

local enumPassThroughOperators = { "!=", "==", ">=", "<=", ">", "<" }

setglobal("AttribAssert", function(value, attrib)
	if not value then
		cerror(attrib.sourceNode, "MalformedAttributeTag")
	end
end )

local function shorthash(str)
	return string.sub(RDXC.Native.sha256(str), 1, 8)
end

local function computeguid(str)
	return RDXC.Native.computeguid(str)
end

setglobal("ComputeSymbolGUID", function(sym)
	local domain, objName = splitResName(sym)
	return computeguid(domain).."_"..computeguid(objName)
end )


local CPPAttrib = function(obj, attrib)
	return obj.attribTags and obj.attribTags.cpp and obj.attribTags.cpp[attrib]
end

setglobal("DetermineObjectName", function(cs, obj)
	if obj.cpp == nil then
		obj.cpp = { }
	end

	if obj.cpp.name ~= nil or (obj.type == "CNamespace" and obj.name == nil) then
		return
	end
	
	obj.cpp.insideType = false

	-- Determine the owner
	local owner
	local prefix = obj.type

	if obj.type == "CStructuredType" then
		owner = obj.namespace.owner
		prefix = "C"
	else
		owner = obj.owner
		if owner then
			if owner.type == "CNamespace" then
				prefix = "N"
			end
		else
			prefix = "U"
		end
	end

	if owner then
		if owner.createdBy then
			owner = owner.createdBy
		end
		DetermineObjectName(cs, owner)

		if owner.cpp.insideType or
			(owner.type == "CStructuredType" and (owner.declType == "struct" or owner.declType == "enum" or owner.declType == "class")) then
			obj.cpp.insideType = true
		end
	end
	
	local objName = obj.name
	if obj.isFromTemplate or obj.isPrivateType then
		objName = "Tp_"..shorthash(objName)
		prefix = "T"..prefix
	elseif obj.privateTypeName then
		objName = "Pt_"..shorthash(obj.privateTypeName)
		prefix = "P"..prefix
	end

	if CPPAttrib(obj, "name") then
		AttribAssert(obj.attribTags.cpp.name[1], obj.attribTags.cpp.name)

		obj.cpp.name = obj.attribTags.cpp.name[1].string
		obj.cpp.classname = obj.cpp.name
		assert(type(obj.cpp.name) == "string")
	else
		if owner ~= nil and owner.cpp.name then
			if not owner.cpp.rnNamespace then
				cerror(owner.attribTags.cpp.sourceNode, "SymbolInsideNamedCppObject")
			end

			obj.cpp.name = owner.cpp.name.."::"..objName
			obj.cpp.classname = owner.cpp.rnNamespace.."::"..prefix..objName
			obj.cpp.rnNamespace = owner.cpp.rnNamespace.."::N"..objName
		else
			if cppConfig.exportNamespace then
				obj.cpp.name = "::"..cppConfig.exportNamespace.."::"..objName
			else
				obj.cpp.name = "::"..objName
			end
			obj.cpp.classname = "::"..objName
			obj.cpp.rnNamespace = "::_RDX_CPPX::N"..objName
		end
	end
	assert(string.sub(obj.cpp.name, 1, 2) == "::" or obj.cpp.name == "void", obj.cpp.name)
end )

setglobal("DetermineObjectCPPPath", function(cs, obj)
	DetermineObjectName(cs, obj)

	if not obj.cpp.pathEvaluated then
		if CPPAttrib(obj, "header") then
			obj.cpp.headerPath = obj.attribTags.cpp.header[1].string
			obj.cpp.protoPath = obj.cpp.headerPath
			if obj.attribTags.cpp.coreheader then
				obj.cpp.coreHeader = true
			end
		elseif CPPAttrib(obj, "builtin") then
		else
			-- Compute path from name
			local fullPath = ""
			local current
			assert(string.sub(obj.cpp.name, 1, 2) == "::", obj.cpp.name)
			current = string.sub(obj.cpp.name, 3, #obj.cpp.name)
			while true do
				local prefix, suffix = string.match(current, "(.-)::(.+)")
				assert(prefix ~= "", current)
				if suffix == nil then
					break
				end

				if not CPPAttrib(obj, "coreheader") then
					RDXC.Native.mkdir(cppConfig.exportPath.."/"..fullPath..prefix)
				end
				fullPath = fullPath..prefix.."/"
				current = suffix
			end
			obj.cpp.headerPath = fullPath..current..".hpp"
			obj.cpp.protoPath = fullPath..current..".Proto.hpp"
			obj.cpp.cppPath = fullPath..current..".cpp"
			obj.cpp.pccmPath = fullPath..current..".PCCM.cpp"
		end

		obj.cpp.pathEvaluated = true
	end
end )

local SplitPath = function(path)
	local current = path
	local decomposed = { }
	while true do
		local prefix, suffix = string.match(current, "(.-)/(.+)");
		assert(prefix ~= "")
		if suffix == nil then
			break;
		end
		decomposed[#decomposed+1] = prefix
		current = suffix
	end
	decomposed[#decomposed+1] = current
	return decomposed
end

local RelativeHeaderPathOld = function(cs, sourceObj, targetObj)
	local sourcePath = sourceObj

	if type(sourcePath) == "table" then
		sourcePath = sourceObj.cpp.headerPath
	end

	local targetPath = targetObj

	if type(targetObj) == "table" then
		DetermineObjectCPPPath(cs, targetObj)

		targetPath = targetObj.cpp.headerPath
		if targetObj.cpp.coreHeader then
			targetPath = ""
		end
	end

	local targetDecomposed = SplitPath(targetPath)
	local sourceDecomposed = SplitPath(sourcePath)
	
	local deepestMatch = 0
	for i=1,#targetDecomposed do
		if targetDecomposed[i] ~= sourceDecomposed[i] then
			break
		end
		deepestMatch = i
	end
	
	local relativePath = ""
	for i=1,(#sourceDecomposed - deepestMatch - 1) do
		relativePath = relativePath.."../"
	end
	
	for i=deepestMatch+1,#targetDecomposed do
		relativePath = relativePath..targetDecomposed[i].."/"
	end
	relativePath = string.sub(relativePath, 1, #relativePath-1)		-- Clip final /

	if type(targetObj) == "table" and targetObj.cpp.coreHeader then
		relativePath = relativePath..cppConfig.rdxPath.."/"..targetObj.cpp.headerPath
	end

	return relativePath
end

local RelativeHeaderPath = function(cs, sourceObj, targetObj, needProto)
	if type(targetObj) == "table" then
		DetermineObjectCPPPath(cs, targetObj)
		if needProto then
			return assert(targetObj.cpp.protoPath)
		else
			return assert(targetObj.cpp.headerPath)
		end
	else
		return targetObj
	end
end

-- Adds a dependency that must be fully declared before this type is usable, such as a struct or a class extension
local AddHardDependency = function(cs, dep, obj)
	if dep.softDepSet[obj] then
		dep.softDepSet[obj] = nil
	end
	if dep.protoDepSet[obj] then
		dep.protoDepSet[obj] = nil
	end
	dep.hardDepSet[obj] = true
end

-- Adds a dependency that might only be used for reference types.  Hard dependencies always override soft dependencies
local function AddSoftDependency(cs, dep, obj)
	if dep.hardDepSet[obj] then
		return
	end
	if dep.protoDepSet[obj] then
		return
	end
	if obj.type == "CArrayOfType" then
		AddSoftDependency(cs, dep, obj.containedType)
	else
		dep.softDepSet[obj] = true
	end
end

local function AddProtoDependency(cs, dep, obj)
	if dep.hardDepSet[obj] then
		return
	end
	if dep.protoDepSet[obj] then
		dep.protoDepSet[obj] = nil
	end
	if obj.type == "CArrayOfType" then
		AddSoftDependency(cs, dep, obj.containedType)
	else
		dep.protoDepSet[obj] = true
	end
end

local AddSoftDependencyName = function(cs, dep, name, declType)
	dep.softDepNameSet[name] = { declType = declType }
end

local ObjNoCode = function(obj)
	return CPPAttrib(obj, "nocode")
end

local ResolveDependencies = function(cs, sourceObj, depSet)
	-- Resolve hard dependency header paths
	for dep in pairs(depSet.hardDepSet) do
		if not ObjNoCode(dep) then
			local headerPath = RelativeHeaderPath(cs, sourceObj, dep)
			depSet.headerSet[headerPath] = true
		end
	end
	
	depSet.hardDepSet = nil

	-- Resolve soft dependency paths
	for dep in pairs(depSet.softDepSet) do
		DetermineObjectCPPPath(cs, dep)
		if not ObjNoCode(dep) then
			local isBuiltin = CPPAttrib(dep, "builtin")
			if isBuiltin then
				depSet.headerSet[RelativeHeaderPath(cs, sourceObj, dep)] = true
			else
				depSet.softDepNameSet[dep.cpp.classname] = { declType = dep.declType }
			end
		end
	end

	depSet.softDepSet = nil

	-- Resolve proto paths
	for dep in pairs(depSet.protoDepSet) do
		DetermineObjectCPPPath(cs, dep)
		if not ObjNoCode(dep) then
			local headerPath = RelativeHeaderPath(cs, sourceObj, dep, true)
			depSet.headerSet[headerPath] = true
		end
	end

	depSet.protoDepSet = nil
end

GetCPPType = function(cs, dependencies, obj, useWeak)
	if obj.type == "CStructuredType" then
		if obj.declType == "struct" or obj.declType == "class" or obj.declType == "enum" or obj.declType == "interface" then
			DetermineObjectName(cs, obj)
			local symName = obj.cpp.classname
			if obj.declType == "class" then
				--if CPPAttrib(obj, "alwaysconst") then
				--	symName = symName.." const"
				--end
				if useWeak then
					symName = "rdxWeakRTRef("..symName..")"
				else
					symName = "rdxTracedRTRef("..symName..")"
				end
			elseif obj.declType == "interface" then
				if useWeak then
					symName = "rdxWeakIRef("..symName..")"
				else
					symName = "rdxTracedIRef("..symName..")"
				end
			end
			return symName
		else
			assert(false)
		end
	elseif obj.type == "CArrayOfType" then
		if dependencies then
			AddSoftDependency(cs, dependencies, obj.containedType)
		end
		local arrayName = GetCPPType(cs, dependencies, obj.containedType)
		--if obj.isConst then
		--	arrayName = arrayName.." const"
		--end
		if useWeak then
			arrayName = "rdxWeakArrayRTRef("..arrayName..")"
		else
			arrayName = "rdxTracedArrayRTRef("..arrayName..")"
		end
		return arrayName
	elseif obj.type == "CDelegateType" then
		return GetCPPType(cs, dependencies, cs.gst["Core.RDX.Method"], useWeak)
	else
		assert(false)
	end
end

local TypeIsByRef = function(t)
	return t.type == "CStructuredType" and t.declType == "struct" and not t.byVal
end

-- Builds C++ return value, parameter expectations, and static flag for a method
local GetCPPFunction = function(cs, dependencies, method)
	local returnType = "void"
	local returnTypeRefs = method.returnTypes.typeReferences
	local numReturnTypes = #returnTypeRefs
	local rvInline = false

	AddSoftDependencyName(cs, dependencies, "::rdxSExportedCallEnvironment", "struct")
	
	local paramList
	local paramPass

	if method.isNative then
		paramList = "(rdxSExportedCallEnvironment &callEnv"
		paramPass = "callEnv"
		for rvi,rtRef in ipairs(returnTypeRefs) do
			local rt = rtRef.refType
			local cppType = GetCPPType(cs, dependencies, rt)
			AddSoftDependency(cs, dependencies, rt)

			if method.name.string ~= "#coerce" and numReturnTypes == 1 and not TypeIsByRef(rt) then
				returnType = GetCPPType(cs, dependencies, rt, true)
			else
				rvInline = true
				paramList = paramList..", "..cppType.." & r_"..rvi

				if TypeIsByRef(rt) then
					paramPass = paramPass..", *static_cast<"..cppType.." *>(rv"..rvi.."->Modify())"
				else
					paramPass = paramPass..", *rv"..rvi
				end
			end
		end

		for paramIdx,param in ipairs(method.actualParameterList.parameters) do
			if paramIdx ~= method.thisParameterIndex then
				local paramType = param.type.refType
				local cppType = GetCPPType(cs, dependencies, paramType, true)
				AddSoftDependency(cs, dependencies, paramType)

				if TypeIsByRef(paramType) then
					local constness = param.isConst and " const" or ""
					paramPass = paramPass..", *static_cast<"..cppType..constness.." *>(p"..paramIdx.."->Modify())"
					cppType = cppType..constness.." &"
				else
					if VTypeIsObjectReference(paramType) then
						paramPass = paramPass..", p"..paramIdx.."->ToWeakRTRef()"
					else
						paramPass = paramPass..", *p"..paramIdx
					end
				end
				paramList = paramList..", "..cppType.." p_"..param.name.string
			end
		end
		paramList = paramList..")"
	else
		paramList = "(::rdxPCCMCallEnvironment &callEnv)"
		paramPass = "callEnv"
	end

	return returnType, paramList, paramPass, (not method.thisParameterIndex), rvInline
end

local ExportTypeProto = function(cs, obj)
	local f = io.open(cppConfig.exportPath.."/"..obj.cpp.protoPath, "w")

	DetermineObjectName(cs, obj)

	local headerTagHash = "__RDX_CPPX_PROTO_"..shorthash(obj.longName).."__"
	local objectName = obj.cpp.classname
	
	f:write("#ifndef "..headerTagHash.."\n")
	f:write("#define "..headerTagHash.."\n\n")

	if obj.declType == "class" then
		DetermineObjectCPPPath(cs, obj)
		DetermineObjectName(cs, obj.parentType)
		local headerPath = RelativeHeaderPath(cs, obj, obj.parentType, true)
		f:write("#include \""..headerPath.."\"  // (ExportTypeProto)\n")
	end

	f:write("\n")

	do
		local ns, sym = string.match(objectName, "::(.+::)(.+)")
		if sym == nil then
			ns = ""
			sym = objectName
		end
		
		local indentLevel = 0

		-- Descend into the current symbol's namespace
		local currentNamespace = ""
		local indentLevel = 0
		local subNS = ns
		while subNS ~= "" do
			local nextLevel
			nextLevel, subNS = string.match(subNS, "(.-::)(.*)")

			local namespaceName = string.sub(nextLevel, 1, #nextLevel - 2)
			assert(namespaceName ~= "")
			for i=1,indentLevel do
				f:write("\t")
			end
			f:write("namespace "..namespaceName.."\n")
			for i=1,indentLevel do
				f:write("\t")
			end
			f:write("{\n")
			indentLevel = indentLevel + 1
		end

		for i=1,indentLevel do
			f:write("\t")
		end

		f:write(declTypeTranslations[obj.declType].." "..sym..";\n")

		while indentLevel > 0 do
			indentLevel = indentLevel - 1
			for i=1,indentLevel do
				f:write("\t")
			end
			f:write("}\n")
		end

		if obj.declType == "class" then
			DetermineObjectName(cs, obj.parentType)
			f:write("RDX_DECLARE_SUPER("..obj.cpp.classname..", "..obj.parentType.cpp.classname..");\n")
		elseif obj.declType == "interface" then
			f:write("RDX_DECLARE_SUPER("..obj.cpp.classname..", ::rdxSObjectInterfaceImplementation);\n")
		end
	end

	f:write("\n#endif\n")
end

local ExportTypeHeader = function(cs, obj)
	local headerTagHash = "__RDX_CPPX_HEADER_"..shorthash(obj.longName).."__"
	local dependencies = { headerSet = { }, systemHeaderSet = { }, hardDepSet = { }, softDepSet = { }, protoDepSet = { }, softDepNameSet = { }, dependencyTree = { } }
	local firstProperty = 1

	if obj.parentType and not ObjNoCode(obj.parentType) then
		AddHardDependency(cs, dependencies, obj.parentType)
		firstProperty = #obj.parentType.properties + 1
	end
	
	do
		local numInterfaces = #obj.interfaces
		local numParentInterfaces = 0
		if obj.parentType then
			numParentInterfaces = #obj.parentType.interfaces
		end
		for i=numParentInterfaces+1,numInterfaces do
			local ifc = obj.interfaces[i].interfaceType
			AddHardDependency(cs, dependencies, ifc)
		end
	end

	-- Resolve any property dependencies
	for i=firstProperty,#obj.properties do
		local prop = obj.properties[i]
		local propType = prop.typeOf.refType

		if propType.type == "CStructuredType" and (propType.declType == "struct" or propType.declType == "enum") then
			AddHardDependency(cs, dependencies, propType)
		else
			AddProtoDependency(cs, dependencies, propType)
		end
	end
	
	local childTypeDefs = { }
	local childMethods = { }
	
	-- Find child classes and methods
	local symbols = obj.namespace.symbols
	
	for sym,childObj in pairs(symbols) do
		if childObj.type == "CStructuredType" and (childObj.declType == "class" or childObj.declType == "struct" or childObj.declType == "interface" or childObj.declType == "enum") and not childObj.isTemplate then
			DetermineObjectName(cs, childObj)
			AddSoftDependency(cs, dependencies, childObj)
			childTypeDefs[#childTypeDefs+1] = { internalSymbol = CPPUnreserve(sym), typeName = childObj.cpp.classname, visibility = childObj.visibility }
		elseif childObj.type == "CMethodGroup" then
			for _,overload in ipairs(childObj.overloads) do
				if overload.definedByType == obj then
					local returnType, parameters, paramPass, isStatic, rvInline = GetCPPFunction(cs, dependencies, overload)
					local methodSymbol = sym
					if methodNameTranslations[methodSymbol] then
						methodSymbol = methodNameTranslations[methodSymbol]
					end
					if overload.cpp == nil then
						overload.cpp = { }
					end
					overload.cpp.marshalMethodInfo =
					{
						internalSymbol = CPPUnreserve(methodSymbol),
						returnType = returnType,
						parameters = parameters,
						visibility = childObj.visibility,
						isStatic = isStatic,
						paramPass = paramPass,
						rvInline = rvInline,
						isConst = overload.isConst,
						isNative = overload.isNative,
					}
					overload.cpp.rvInline = rvInline
					childMethods[#childMethods+1] = overload.cpp.marshalMethodInfo
				end
			end
		end
	end
	
	table.sort(childTypeDefs, function(a, b)
		return a.internalSymbol < b.internalSymbol
	end )

	local childPropertyDefs = { }

	-- Scan all properties
	for i=firstProperty,#obj.properties do
		local prop = obj.properties[i]
		local propType = prop.typeOf.refType

		local cppType = GetCPPType(cs, dependencies, propType)
		childPropertyDefs[#childPropertyDefs+1] = { internalSymbol = CPPUnreserve(prop.name), typeName = cppType, visibility = prop.visibility }
	end

	obj.cpp.childPropertyDefs = childPropertyDefs

	if CPPAttrib(obj, "hascustomtrace") then
		obj.cpp.hasTypeProcessor = true
		obj.cpp.hasCustomTrace = true
	end
	if CPPAttrib(obj, "hasfinalizer") then
		obj.cpp.hasTypeProcessor = true
		obj.cpp.hasFinalizer = true
	end
	if CPPAttrib(obj, "hasonload") then
		obj.cpp.hasTypeProcessor = true
		obj.cpp.hasOnLoad = true
	end
	if CPPAttrib(obj, "hasserializer") then
		obj.cpp.hasSerializer = true
	end

	local rdxIncludePath = RelativeHeaderPath(cs, obj, "")..cppConfig.rdxPath

	if obj.cpp.hasTypeProcessor then
		dependencies.headerSet[rdxIncludePath.."/rdx_basictypes.hpp"] = true	-- For ScanID
		dependencies.headerSet[rdxIncludePath.."/rdx_gcslots.hpp"] = true		-- For GCLink
		AddSoftDependencyName(cs, dependencies, "::RDX::ObjectManagement::IObjectManager", "struct")
		AddSoftDependencyName(cs, dependencies, "::RDX::ObjectManagement::ISerializer", "struct")
		AddSoftDependencyName(cs, dependencies, "::RDX::OperationContext", "struct")
	end

	if obj.cpp.hasSerializer then
		AddSoftDependencyName(cs, dependencies, "::RDX::OperationContext", "struct")
		AddSoftDependencyName(cs, dependencies, "::RDX::ObjectManagement::IObjectManager", "struct")
		AddSoftDependencyName(cs, dependencies, "::RDX::IO::ITextDeserializer", "struct")
		AddSoftDependencyName(cs, dependencies, "::RDX::IO::IFileStream", "struct")
		AddSoftDependencyName(cs, dependencies, "::RDX::ObjectManagement::IPackageHost", "struct")
		AddSoftDependencyName(cs, dependencies, "::RDX::ObjectManagement::Package", "struct")
	end

	if CPPAttrib(obj, "softdependencies") then
		local depTable = CPPAttrib(obj, "softdependencies")
		for i=1,#depTable,2 do
			local depName = depTable[i]
			local depDeclType = depTable[i+1]
			if depDeclType == nil then
				cerror(obj.attribTags.cpp.sourceNode, "MalformedSoftDependencies")
			end
			AddSoftDependencyName(cs, dependencies, depName.string, depDeclType.string)
		end
	end

	ResolveDependencies(cs, obj, dependencies)
	
	local f = io.open(cppConfig.exportPath.."/"..obj.cpp.headerPath, "w")
	f:write("#ifndef "..headerTagHash.."\n")
	f:write("#define "..headerTagHash.."\n")
	f:write("\n")
	f:write("// Internal name: "..obj.prettyName.."\n")
	f:write("\n")
	f:write("#include \""..rdxIncludePath.."/rdx_pccm_api.hpp\"\n")

	if obj.type == "CStructuredType" and obj.declType == "enum" then
		dependencies.headerSet[rdxIncludePath.."/rdx_basictypes.hpp"] = true
	end

	if CPPAttrib(obj, "rdxheaders") then
		for _,header in ipairs(CPPAttrib(obj, "rdxheaders")) do
			dependencies.headerSet[rdxIncludePath.."/"..header.string] = true
		end
	end

	if CPPAttrib(obj, "relativeheaders") then
		for _,header in ipairs(CPPAttrib(obj, "relativeheaders")) do
			dependencies.headerSet[header.string] = true
		end
	end

	if CPPAttrib(obj, "systemheaders") then
		for _,header in ipairs(CPPAttrib(obj, "systemheaders")) do
			dependencies.systemHeaderSet[header.string] = true
		end
	end

	-- Include the object's own proto
	do
		DetermineObjectCPPPath(cs, obj)
		local selfProto = obj.cpp.protoPath
		if not dependencies.headerSet[selfProto] then
			f:write("#include \""..selfProto.."\"\n")
		end
	end

	for headerPath in sortedpairs(dependencies.systemHeaderSet) do
		f:write("#include <"..headerPath..">\n")
	end

	for headerPath in sortedpairs(dependencies.headerSet) do
		f:write("#include \""..headerPath.."\"   // (ExportTypeHeader HeaderSet)\n")
	end

	f:write("\n")
	
	local indentLevel = 0
	local currentNamespace = ""

	local writeIndent = function()
		for i=1,indentLevel do f:write("\t") end
	end

	local transitionNamespace = function(symbolPath)
		assert(string.sub(symbolPath, 1, 2) == "::")
		symbolPath = string.sub(symbolPath, 3, #symbolPath)
		local ns, sym
		ns, sym = string.match(symbolPath, "(.+::)(.+)")
		if sym == nil then
			ns = ""
			sym = symbolPath
		end
		
		-- Drop back on namespaces
		while string.sub(ns, 1, #currentNamespace) ~= currentNamespace do
			indentLevel = indentLevel - 1
			writeIndent()
			f:write("}\n")
			currentNamespace = string.match(currentNamespace, "(.+::).-::")
			if currentNamespace == nil then
				currentNamespace = ""
			end
		end
		
		-- Descend into the current symbol's namespace
		local subNS = string.sub(ns, #currentNamespace+1, #ns)
		while currentNamespace ~= ns do
			local nextLevel
			nextLevel, subNS = string.match(subNS, "(.-::)(.*)")

			writeIndent()
			local namespaceName = string.sub(nextLevel, 1, #nextLevel - 2)
			assert(namespaceName ~= "")
			f:write("namespace "..namespaceName.."\n")
			writeIndent()
			f:write("{\n")
			indentLevel = indentLevel + 1

			currentNamespace = currentNamespace..nextLevel
		end
		
		return sym
	end

	-- Forward declare the plugin classes
	transitionNamespace("::_RDX_CPPX::Plug")
	writeIndent() f:write("struct PluginGlue;\n")
	writeIndent() f:write("struct PCCMGlue;\n")
	if obj.cpp.hasTypeProcessor then
		writeIndent() f:write("struct PluginTypeProcessor_"..shorthash(obj.longName)..";\n")
	end
	if obj.cpp.hasSerializer then
		writeIndent() f:write("struct PluginSerializer_"..shorthash(obj.longName)..";\n")
	end


	for symbolPath, dep in sortedpairs(dependencies.softDepNameSet) do
		local sym = transitionNamespace(symbolPath)
		writeIndent()
		f:write(declTypeTranslations[dep.declType].." "..sym..";\n")
	end
	
	DetermineObjectCPPPath(cs, obj)
	local classSymbol = transitionNamespace(obj.cpp.classname)


	-- Export the main class def
	writeIndent() f:write(declTypeTranslations[obj.declType].." "..classSymbol)
	if obj.parentType and not ObjNoCode(obj.parentType) then
		f:write(" : public "..obj.parentType.cpp.classname)
	--elseif obj.type == "CStructuredType" and obj.declType == "enum" then
	--	f:write(" : public RDX::EnumValue")
	end
	if obj.interfaces ~= nil then
		local numInterfaces = #obj.interfaces
		local numParentInterfaces = 0
		if obj.parentType ~= nil then
			numParentInterfaces = #obj.parentType.interfaces
		end
		for i=numParentInterfaces+1,numInterfaces do
			local ifc = obj.interfaces[i].interfaceType
			DetermineObjectName(cs, ifc)
			f:write(", public "..ifc.cpp.classname)
		end
	end
	
	if obj.type == "CStructuredType" and obj.declType == "interface" then
		f:write(" : public ::rdxSObjectInterfaceImplementation")
	end
	if obj.type == "CStructuredType" and obj.declType == "enum" then
		f:write(" : public ::rdxSRuntimeEnum")
	end
	f:write("\n")
	writeIndent() f:write("{\n")


	writeIndent() f:write("\tfriend struct ::_RDX_CPPX::PluginGlue;\n")
	writeIndent() f:write("\tfriend struct ::_RDX_CPPX::PCCMGlue;\n")

	if obj.cpp.hasTypeProcessor then
		writeIndent() f:write("\tfriend struct ::_RDX_CPPX::PluginTypeProcessor_"..shorthash(obj.longName)..";\n")
	end
	if obj.cpp.hasSerializer then
		writeIndent() f:write("\tfriend struct ::_RDX_CPPX::PluginSerializer_"..shorthash(obj.longName)..";\n")
	end

	if CPPAttrib(obj, "nativeproperties") then
		writeIndent() f:write("\tpublic: struct NativeProperties\n")
		writeIndent() f:write("\t{\n")
		for _,nf in ipairs(CPPAttrib(obj, "nativeproperties")) do
			writeIndent() f:write("\t\t"..nf.string..";\n")
		end
		writeIndent() f:write("\t} _native;\n\n")

		obj.cpp.hasNativeFields = true
	end

	if CPPAttrib(obj, "nativemethods") then
		for _,nf in ipairs(CPPAttrib(obj, "nativemethods")) do
			writeIndent() f:write("\tprivate: "..nf.string..";\n")
		end
	end

	if obj.cpp.hasCustomTrace then
		writeIndent() f:write("\tprivate: void MarkDependencies(::RDX::ObjectManagement::IObjectManager *objm, bool markNative, ::RDX::ObjectManagement::ISerializer *ser, ::RDX::ObjectManagement::ScanID scanID, ::RDX::ObjectManagement::GCLink gcl) const;\n")
	end
	if obj.cpp.hasFinalizer then
		writeIndent() f:write("\tprivate: void Finalize(::RDX::ObjectManagement::IObjectManager *objm);\n")
	end
	if obj.cpp.hasOnLoad then
		writeIndent() f:write("\tprivate: bool OnLoad(::RDX::OperationContext *ctx, ::RDX::ObjectManagement::IObjectManager *objm);\n")
	end
	if obj.cpp.hasSerializer then
		writeIndent() f:write("\tprivate: void DeserializeTextInstance(::RDX::OperationContext *ctx, ::RDX::ObjectManagement::IObjectManager *objm, ::RDX::IO::ITextDeserializer *td, ::RDX::ObjectManagement::IPackageHost *host, ::RDX::ObjectManagement::Package *pkg);\n")
		writeIndent() f:write("\tprivate: void DeserializeBinaryInstance(::RDX::OperationContext *ctx, ::RDX::ObjectManagement::IObjectManager *objm, ::RDX::IO::IFileStream *fs, ::RDX::ObjectManagement::IPackageHost *host, ::RDX::ObjectManagement::Package *pkg);\n")
		writeIndent() f:write("\tprivate: void SerializeBinaryInstance(::RDX::ObjectManagement::IObjectManager *objm, ::RDX::IO::IFileStream *fs) const;\n")
		writeIndent() f:write("\tprivate: void SerializeTextInstance(::RDX::ObjectManagement::IObjectManager *objm, ::RDX::IO::IFileStream *fs) const;\n")
	end

	-- Write typedefs
	for _,td in ipairs(childTypeDefs) do
		writeIndent() f:write("\t"..td.visibility..": typedef "..td.typeName.." "..td.internalSymbol..";\n")
	end
	-- Write properties
	for _,p in ipairs(childPropertyDefs) do
		local sym = p.internalSymbol
		writeIndent() f:write("\t"..p.visibility..": "..p.typeName.." "..p.internalSymbol..";\n")
	end
	-- Write methods
	for _,m in ipairs(childMethods) do
		local constness = (m.isConst and " const" or "")
		if m.isNative then
			writeIndent() f:write("\t"..m.visibility..": "..(m.isStatic and "static ")..m.returnType.." "..m.internalSymbol..m.parameters..constness..";\n")
		end
	end
	-- Write enumerants
	if obj.enumerants ~= nil and #obj.enumerants > 0 then
		writeIndent() f:write("public:\n")
		--for _,pto in ipairs(enumPassThroughOperators) do
		--	writeIndent() f:write("\tinline bool operator "..pto.."(const "..classSymbol.." &rs) const { return (this->m_value "..pto.." rs.m_value); }\n")
		--end

		writeIndent() f:write("\tenum Enum\n")
		writeIndent() f:write("\t{\n")
		for _,e in ipairs(obj.enumerants) do
			writeIndent() f:write("\t\t"..CPPUnreserve(e.name).." = "..e.value..",\n")
		end
		writeIndent() f:write("\t};\n")
		writeIndent() f:write("\tinline "..classSymbol.." () { }\n")
		writeIndent() f:write("\tinline "..classSymbol.." (const Enum &rs) : rdxSRuntimeEnum(static_cast<RDX_ENUM_TYPE>(rs)) { }\n")
		writeIndent() f:write("\tinline "..classSymbol.." (const "..classSymbol.." &rs) : rdxSRuntimeEnum(rs) { }\n")
		--writeIndent() f:write("\tinline "..classSymbol.." (const RDX_ENUM_TYPE &rs) { this->m_value = rs; }\n")
		writeIndent() f:write("\tinline "..classSymbol.." & operator =(const Enum &rs) { this->m_value = static_cast<RDX_ENUM_TYPE>(rs); return *this; }\n")
		writeIndent() f:write("\tinline operator Enum () const { return static_cast<Enum>(this->m_value); }\n")
		writeIndent() f:write("\tinline "..classSymbol.." &operator =(const "..classSymbol.." &rs) { this->m_value = rs.m_value; return *this; }\n")
		local enumCompareOperators = { "==", "!=", ">", "<", ">=", "<=" }
		for _,op in ipairs(enumCompareOperators) do
			writeIndent() f:write("\tinline bool operator "..op.."(const "..classSymbol.." &rs) { return this->m_value "..op.." rs.m_value; }\n")
			writeIndent() f:write("\tinline bool operator "..op.."(const Enum &rs) { return this->m_value "..op.." static_cast<RDX_ENUM_TYPE>(rs); }\n")
		end
		writeIndent() f:write("private:\n")
		writeIndent() f:write("\tinline "..classSymbol.." (const rdxSRuntimeEnum &rs) { this->m_value = rs.Value(); }\n")
		writeIndent() f:write("\tinline "..classSymbol.." &operator =(const rdxSRuntimeEnum &rs) { this->m_value = rs.Value(); return *this; }\n")
	end

	if obj.cpp.hasNativeFields then
		f:write("\n")
		writeIndent() f:write("\tRDX_DECLARE_PROPERTY_LOOKUP;\n")
		writeIndent() f:write("\t"..classSymbol.."(rdxIObjectManager *objm, rdxGCInfo *info)\n")
		if obj.parentType and not ObjNoCode(obj.parentType) then
			writeIndent() f:write("\t\t: "..obj.parentType.cpp.classname.."(objm, info)\n")
		end
		writeIndent() f:write("\t{\n")
		writeIndent() f:write("\t}\n")
	end

	writeIndent() f:write("};\n")

	-- Write the base typedef
	if not obj.cpp.insideType then
		local sym = transitionNamespace(obj.cpp.name)
		writeIndent() f:write("typedef "..obj.cpp.classname.." "..sym..";\n")
	end

	transitionNamespace("::")
	
	if obj.cpp.hasNativeFields then
		if obj.declType == "class" then
			f:write("RDX_DECLARE_COMPLEX_NATIVE_CLASS("..obj.cpp.classname..");\n")
		elseif obj.declType == "struct" then
			f:write("RDX_DECLARE_COMPLEX_NATIVE_STRUCT("..obj.cpp.classname..");\n")
		end
	end

	f:write("#endif\n")
	f:close()
end

local ExportObject = function(cs, obj)
	if obj.cpp and obj.cpp.exported == true then
		return
	end

	if obj.type ~= "CStructuredType" or obj.isTemplate then
		return
	end

	obj.cpp = { }
	local cppTags = (obj.attribTags and obj.attribTags.cpp) or { }
	
	if not cppTags.header and not cppTags.nocode then
		-- Determine header path
		DetermineObjectName(cs, obj)
		DetermineObjectCPPPath(cs, obj)
		
		if obj.type == "CStructuredType" then
			ExportTypeProto(cs, obj)
			ExportTypeHeader(cs, obj)
		end
	end

	obj.cpp.exported = true
end

setglobal("CPPExportSymbol", function(cs, sym)
	if not cppConfig then
		return
	end

	local obj = cs.gst[sym]
	if obj == nil then
		return
	end
	
	ExportObject(cs, obj)
end )

setglobal("BinObjectsByGUID", function(objectList)
	local domainBins = { }
	for _,obj in ipairs(objectList) do
		local domain, objName = splitResName(obj.longName)
		local domainGUID = computeguid(domain)
		local objGUID = computeguid(objName)
		local bin = domainBins[domainGUID]
		if bin == nil then
			bin = { }
			domainBins[domainGUID] = bin
		end
		bin[objGUID] = obj
	end
	return domainBins
end )

setglobal("CPPExportPlugin", function(cs, includedSymbols)
	local callbackParams = "(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv)"

	if not cppConfig then
		return
	end

	local f = io.open(cppConfig.exportPath.."/"..cppConfig.pluginPath..".cpp", "wb")

	-- Write standard includes
	local rdxIncludePath = RelativeHeaderPath(cs, cppConfig.pluginPath, "")..cppConfig.rdxPath

	local criticalHeaders = {
		"rdx_lut.hpp", "rdx_basictypes.hpp", "rdx_marshal.hpp", "rdx_programmability.hpp"
	}

	local criticalHeadersSet = { }

	f:write("#include \"rdx_plugin_api.hpp\"\n")
	for _,ch in ipairs(criticalHeaders) do
		local fullPath = rdxIncludePath.."/"..ch
		f:write("#include \""..fullPath.."\"    // (CPPExportPlugin CriticalHeader)\n")
		criticalHeadersSet[fullPath] = true
	end

	local importedHeaders = { }

	local userPropObjects = { }
	local marshalMethods = { }
	local serializerObjects = { }
	local typeProcessorObjects = { }
	local marshalNTIs = { }

	-- Write all dependencies
	for sym in sortedpairs(includedSymbols) do
		local obj = cs.gst[sym]
		if obj ~= nil and obj.cpp then
			if obj.cpp.headerPath and (obj.type ~= "CStructuredType" or obj.declType ~= "interface") then
				importedHeaders[RelativeHeaderPath(cs, cppConfig.pluginPath, obj)] = true
			end

			if obj.cpp.hasSerializer then
				serializerObjects[#serializerObjects+1] = obj
			end
			if obj.cpp.hasTypeProcessor then
				typeProcessorObjects[#typeProcessorObjects+1] = obj
			end

			local hasUserProps = false
			if obj.cpp.hasNativeFields then
				hasUserProps = true
			end

			for k in pairs(userPropertyTranslations) do
				if CPPAttrib(obj, k) then
					hasUserProps = true
				end
			end

			if hasUserProps or obj.cpp.hasSerializer or obj.cpp.hasTypeProcessor then
				userPropObjects[#userPropObjects+1] = obj
				marshalNTIs[#marshalNTIs+1] = obj
			end

			if obj.cpp.marshalMethodInfo and obj.isNative then
				marshalMethods[#marshalMethods+1] = obj

				if obj.thisParameterIndex then
					local pt = obj.actualParameterList.parameters[obj.thisParameterIndex].type.refType
				end
			end
		end
	end
	for h in sortedpairs(importedHeaders) do
		if not criticalHeadersSet[h] then
			f:write("#include \""..h.."\"\n")
		end
	end
	f:write("\n")

	-- Write marshal functions
	print("Number of marshalMethods: "..#marshalMethods)
	for msNum,method in ipairs(marshalMethods) do
		if method.isNative then
			local domain, objName = splitResName(method.longName)
			local domainGUID = computeguid(domain)
			local objGUID = computeguid(objName)

			f:write("template<>\n")
			f:write("int ::_RDX_CPPX::PluginGlue::CallMethod<0x"..domainGUID.."ULL, 0x"..objGUID.."ULL>"..callbackParams.."\n")
			f:write("{\n")

			local returnTypeRefs = method.returnTypes.typeReferences
			local numReturnTypes = #returnTypeRefs
			local hasReturnValues = (numReturnTypes > 0)
			local hasParameters = (#method.actualParameterList.parameters > 0)

			if numReturnTypes > 0 then
				for rvi=#returnTypeRefs,1,-1 do
					local rt = returnTypeRefs[rvi].refType

					if TypeIsByRef(rt) then
						f:write("\trdxWeakTypelessOffsetRTRef *rv"..rvi..";\n")
					else
						f:write("\t"..GetCPPType(cs, nil, rt).." *rv"..rvi..";\n")
					end
					if rvi == #returnTypeRefs then
						f:write("\trdxPluginUtils::SetInitialRV(rv"..rvi..", prv);\n")
					else
						f:write("\trdxPluginUtils::LinkMarshalRV(rv"..rvi..", rv"..(rvi + 1)..");\n")
					end
				end
			end

			if hasParameters then
				local params = method.actualParameterList.parameters
				for paramIdx=1,#params do
					local param = params[paramIdx]
					local paramType = param.type.refType
					local cppType = GetCPPType(cs, nil, paramType)
					local constness = param.isConst and " const" or ""

					if TypeIsByRef(paramType) then
						f:write("\trdxWeakTypelessOffsetRTRef *p"..paramIdx..";\n")
					else
						f:write("\t"..GetCPPType(cs, nil, paramType).." *p"..paramIdx..";\n")
					end

					f:write("\trdxPluginUtils::LinkMarshalParam(p"..paramIdx..", ")
					if paramIdx == 1 then
						f:write("prv")
					else
						f:write("p"..(paramIdx - 1))
					end
					f:write(");\n")
				end
			end

			f:write("\trdxSExportedCallEnvironment callEnv;\n")
			f:write("\tcallEnv.ctx = ctx;\n")
			f:write("\tcallEnv.thread = thread.Modify();\n")
			f:write("\tcallEnv.objm = objm;\n")
			f:write("\tcallEnv.status = rdxRS_Active;\n")

			local marshalInfo = method.cpp.marshalMethodInfo
			-- Emit the actual call
			local returnsRef = false
			f:write("\t")
			if hasReturnValues and not marshalInfo.rvInline then
				local rt = method.returnTypes.typeReferences[1].refType

				if TypeIsByRef(rt) then
					f:write("*static_cast<"..GetCPPType(cs, nil, rt).." *>(rv1->Modify()) = ")
				else
					if VTypeIsObjectReference(rt) then
						returnsRef = true
					end

					f:write("*rv1 = ")
				end
			end

			if method.thisParameterIndex then
				local pt = method.actualParameterList.parameters[method.thisParameterIndex].type.refType

				if TypeIsByRef(pt) then
					local constness = method.isConst and " const" or ""
					f:write("static_cast<"..GetCPPType(cs, nil, pt)..constness.." *>(p"..method.thisParameterIndex.."->Modify())->")
				else
					local isObjRef = VTypeIsObjectReference(pt)
					if isObjRef then
						f:write("(*")
					end
					f:write("p"..method.thisParameterIndex)
					if isObjRef then
						f:write(")")
					end
					f:write("->")
				end
			else
				f:write(method.definedByType.cpp.classname.."::")
			end

			f:write(marshalInfo.internalSymbol.."("..marshalInfo.paramPass)
			if returnsRef then
				f:write(".ToWeakRTRef()")
			end
			f:write(");\n")

			f:write("\treturn callEnv.status;\n")
			f:write("}\n\n")
		end
	end

	for idx,obj in ipairs(typeProcessorObjects) do
		local sh = shorthash(obj.longName)
		f:write("struct ::_RDX_CPPX::PluginTypeProcessor_"..sh.." : public RDX::ObjectManagement::ITypeProcessor\n")
		f:write("{\n")
		f:write("\tvoid MarkDependencies(RDX::ObjectManagement::IObjectManager *objm, void *obj, const RDX::Programmability::StructuredType *st, bool markNative, RDX::ObjectManagement::ISerializer *ser, RDX::ObjectManagement::ScanID scanID, RDX::ObjectManagement::GCLink gcl) const\n")
		f:write("\t{\n")
		if obj.cpp.hasCustomTrace then
			f:write("\t\tstatic_cast<const "..obj.cpp.classname.." *>(obj)->MarkDependencies(objm, markNative, ser, scanID, gcl);\n")
		end
		f:write("\t}\n")
		f:write("\tvoid Finalize(void *obj, RDX::ObjectManagement::IObjectManager *objm) const\n")
		f:write("\t{\n")
		if obj.cpp.hasFinalizer then
			f:write("\t\tstatic_cast<"..obj.cpp.classname.." *>(obj)->Finalize(objm);\n")
		end
		f:write("\t}\n")
		f:write("\tbool OnLoad(RDX::OperationContext *ctx, void *obj, RDX::ObjectManagement::IObjectManager *objm) const\n")
		f:write("\t{\n")
		if obj.cpp.hasOnLoad then
			f:write("\t\treturn static_cast<"..obj.cpp.classname.." *>(obj)->OnLoad(ctx, objm);\n")
		else
			f:write("\t\treturn true;\n")
		end
		f:write("\t}\n")

		f:write("};\n")
		f:write("static ::_RDX_CPPX::PluginTypeProcessor_"..sh.." s_typeProcessor_"..sh..";\n\n")
	end


	for idx,obj in ipairs(serializerObjects) do
		local sh = shorthash(obj.longName)
		f:write("struct ::_RDX_CPPX::PluginSerializer_"..sh.." : public RDX::ObjectManagement::ITypeSerializer\n")
		f:write("{\n")
		f:write("\tvirtual void DeserializeTextInstance(RDX::OperationContext *ctx, RDX::ObjectManagement::IObjectManager *objm, void *instance, RDX::IO::ITextDeserializer *td, RDX::ObjectManagement::IPackageHost *host, RDX::ObjectManagement::Package *pkg) const\n")
		f:write("\t{\n")
		f:write("\t\tstatic_cast<"..obj.cpp.classname.." *>(instance)->DeserializeTextInstance(ctx, objm, td, host, pkg);\n")
		f:write("\t}\n")
		f:write("\tvirtual void DeserializeBinaryInstance(RDX::OperationContext *ctx, RDX::ObjectManagement::IObjectManager *objm, void *instance, RDX::IO::IFileStream *fs, RDX::ObjectManagement::IPackageHost *host, RDX::ObjectManagement::Package *pkg) const\n")
		f:write("\t{\n")
		f:write("\t\tstatic_cast<"..obj.cpp.classname.." *>(instance)->DeserializeBinaryInstance(ctx, objm, fs, host, pkg);\n")
		f:write("\t}\n")
		f:write("\tvirtual void SerializeBinaryInstance(RDX::ObjectManagement::IObjectManager *objm, const void *obj, RDX::IO::IFileStream *fs) const\n")
		f:write("\t{\n")
		f:write("\t\tstatic_cast<const "..obj.cpp.classname.." *>(obj)->SerializeBinaryInstance(objm, fs);\n")
		f:write("\t}\n")
		f:write("\tvirtual void SerializeTextInstance(RDX::ObjectManagement::IObjectManager *objm, const void *obj, RDX::IO::IFileStream *fs) const\n")
		f:write("\t{\n")
		f:write("\t\tstatic_cast<const "..obj.cpp.classname.." *>(obj)->SerializeTextInstance(objm, fs);\n")
		f:write("\t}\n")
		f:write("};\n")
		f:write("static ::_RDX_CPPX::PluginSerializer_"..sh.." s_serializer_"..sh..";\n\n")
	end

	if #userPropObjects > 0 and false then
		f:write("static ::RDX::StaticLookupTable<::RDX::StaticLookupStringKey<char, ::RDX::Char>, ::RDX::Programmability::StructuredType::NativeProperties::UserProperties>::Entry s_nativeFieldLookups[] =\n")
		f:write("{\n")
		for _,obj in ipairs(userPropObjects) do
			f:write("\t{ RDX_STATIC_STRING(\""..obj.longName.."\"), { ")

			if obj.cpp.hasTypeProcessor then
				f:write("&::s_typeProcessor_"..shorthash(obj.longName))
			else
				f:write("NULL")
			end

			if obj.cpp.hasSerializer then
				f:write(", &::s_serializer_"..shorthash(obj.longName))
			else
				f:write(", NULL")
			end

			if obj.cpp.hasNativeFields then
				f:write(", sizeof("..obj.cpp.classname.."::NativeProperties), RDX_ALIGNOF("..obj.cpp.classname.."::NativeProperties)")
			else
				f:write(", 0, 1")
			end

			-- Write flags
			f:write(", 0")
			for upName,upTranslate in pairs(userPropertyTranslations) do
				if CPPAttrib(obj, upName) then
					f:write(" | ::RDX::Programmability::StructuredType::NativeProperties::UserProperties::UserFlags::"..upTranslate)
				end
			end

			f:write(" } },\n")
		end
		f:write("};\n")
		f:write("static ::RDX::StaticLookupTable<::RDX::StaticLookupStringKey<char, ::RDX::Char>, ::RDX::Programmability::StructuredType::NativeProperties::UserProperties> s_nativeFieldLUT(::s_nativeFieldLookups, sizeof(::s_nativeFieldLookups)/sizeof(::s_nativeFieldLookups[0]));\n")
	end

	if #marshalMethods > 0 then
		local domainBins = BinObjectsByGUID(marshalMethods)
		for domainGUID,objects in sortedpairs(domainBins) do
			f:write("static rdxSPluginObjectFunctionLookup s_funcLookup_"..domainGUID.."[] =\n")
			f:write("{\n")
			for objectGUID,obj in sortedpairs(objects) do
				f:write("\t{ 0x"..objectGUID.."ULL, ::_RDX_CPPX::PluginGlue::CallMethod<0x"..domainGUID.."ULL, 0x"..objectGUID.."ULL> },\n")
			end
			f:write("};\n")
			f:write("\n")
		end
		
		f:write("static rdxSPluginDomainFunctionLookup s_domainFuncLookup[] =\n")
		f:write("{\n")
		for domainGUID in sortedpairs(domainBins) do
			f:write("\t{ 0x"..domainGUID.."ULL, sizeof(s_funcLookup_"..domainGUID..") / sizeof(s_funcLookup_"..domainGUID.."[0]), s_funcLookup_"..domainGUID.." },\n")
		end
		f:write("\t{ 0, 0, RDX_CNULL }\n")
		f:write("};\n")
		f:write("\n")
	end

	if #marshalNTIs > 0 then
		local domainBins = BinObjectsByGUID(marshalNTIs)
		for domainGUID,objects in sortedpairs(domainBins) do
			f:write("static rdxSPluginObjectNTILookup s_ntiLookup_"..domainGUID.."[] =\n")
			f:write("{\n")
			for objectGUID,obj in sortedpairs(objects) do
				local cppType = obj.cpp.classname
				f:write("\t{ 0x"..objectGUID.."ULL, ")
				f:write("rdxSAutoTypeInfo<"..cppType..">::TypeInfoInterface(), ")
				f:write(cppType.."::GetPropertyOffset },\n")
			end
			f:write("};\n")
			f:write("\n")
		end
		
		f:write("static rdxSPluginDomainNTILookup s_domainNTILookup[] =\n")
		f:write("{\n")
		for domainGUID in sortedpairs(domainBins) do
			f:write("\t{ 0x"..domainGUID.."ULL, sizeof(s_ntiLookup_"..domainGUID..") / sizeof(s_ntiLookup_"..domainGUID.."[0]), s_ntiLookup_"..domainGUID.." },\n")
		end
		f:write("\t{ 0, 0, RDX_CNULL }\n")
		f:write("};\n")
		f:write("\n")
		
		for _,obj in ipairs(marshalNTIs) do
			if obj.declType == "struct" then
				f:write("RDX_IMPLEMENT_COMPLEX_NATIVE_STRUCT("..obj.cpp.classname..", (rdxETIF_NoFlags));\n")
			elseif obj.declType == "class" then
				f:write("RDX_IMPLEMENT_COMPLEX_NATIVE_CLASS("..obj.cpp.classname..", (rdxETIF_NoFlags));\n")
			end

			if obj.declType == "struct" or obj.declType == "class" then
				f:write("RDX_BEGIN_PROPERTY_LOOKUP_STRUCT("..obj.cpp.classname..")\n")
				
				for _,cpd in ipairs(obj.cpp.childPropertyDefs) do
					f:write("\tRDX_DEFINE_LOOKUP_PROPERTY("..cpd.internalSymbol..")\n")
				end
				f:write("RDX_END_PROPERTY_LOOKUP\n\n")
			end
		end
	end

	-- Export the plugin structure
	if #userPropObjects > 0 and false then
		f:write("static void s_SetTypeProperties(rdxIObjectManager *objm, rdxCStructuredType *st) const\n")
		f:write("{\n")
		f:write("\tconst rdxCString *str = ::RDX::ObjectManagement::GCInfo::From(st)->gstSymbol;\n")
		f:write("\tif(str == NULL)\n")
		f:write("\t\treturn;\n")
		f:write("\t::RDX::StaticLookupStringKey<char, ::RDX::Char> strKey(str->AsChars(), str->Length());\n")
		f:write("\tconst ::RDX::Programmability::StructuredType::NativeProperties::UserProperties *nfLookup = ::s_nativeFieldLUT.Lookup(strKey);\n")
		f:write("\tif(nfLookup != NULL)\n")
		f:write("\t\tst->_native.user = *nfLookup;\n")
		f:write("}\n")
		f:write("\n")
	end
	f:write("static rdxSPluginExport s_plugin =\n")
	f:write("{\n")
	if #marshalMethods > 0 then
		f:write("\ts_domainFuncLookup,\n")
	else
		f:write("\tRDX_CNULL,\n")
	end
	if #marshalNTIs > 0 then
		f:write("\ts_domainNTILookup,\n")
	else
		f:write("\tRDX_CNULL,\n")
	end
	f:write("};\n")
	f:write("\n")
	f:write("const rdxSPluginExport *"..cppConfig.pluginName.."()\n")
	f:write("{\n")
	f:write("\treturn &::s_plugin;\n")
	f:write("}\n")
	
	-- Write PCCM
	local methodExportBuilder = { }
	print("Marshal methods")

	local guidToSymbol = { }
	
	for sym,obj in pairs(cs.gst) do
		guidToSymbol[ComputeSymbolGUID(sym)] = sym
	end

	f:close()

	if exportPCCM then
		local pccmSymbols = { }
		for sym in sortedpairs(includedSymbols) do
			local obj = cs.gst[sym]
			if obj ~= nil and obj.type == "CMethod" and (not obj.isNative) and (not obj.isAbstract) then
				CompilePCCM(cs, obj, guidToSymbol)

				local domain, objName = splitResName(obj.longName)
				domain = computeguid(domain)
				objName = computeguid(objName)
				
				if pccmSymbols[domain] == nil then
					pccmSymbols[domain] = { }
				end
				pccmSymbols[domain][objName] = true
			end
		end

		local glueFilePath = cppConfig.exportPath.."/"..cppConfig.pccmGluePath..".cpp"
		local glueF = io.open(glueFilePath, "wb")
		glueF:write("#include <string.h>\n")
		glueF:write("#include <stdlib.h>\n")
		glueF:write("#include \"rdx_pccm_api.hpp\"\n")
		glueF:write("\n")
		--[[
		glueF:write("extern \"C\" RDX_DLLEXPORT rdxPCCMCallback "..cppConfig.pccmGlueName.."(rdxLargeUInt index)\n")
		glueF:write("{\n")
		glueF:write("\trdxUInt64 domainU64;\n")
		glueF:write("\trdxUInt64 objectU64;\n")
		for i=1,8 do
			local ci = i - 1
			glueF:write("\tdomainU64 |= static_cast<rdxUInt64>(guid->m_domain.m_bytes["..ci.."]) << "..((7 - ci) * 8)..";\n")
		end
		for i=1,8 do
			local ci = i - 1
			glueF:write("\tobjectU64 |= static_cast<rdxUInt64>(guid->m_bytes["..ci.."]) << "..((7 - ci) * 8)..";\n")
		end
		glueF:write("\tswitch(domainU64)\n")
		glueF:write("\t{\n")
		for domainGUID,objects in sortedpairs(pccmSymbols) do
			glueF:write("\tcase 0x"..domainGUID.."ULL:\n")
			glueF:write("\t\t{\n")
			glueF:write("\t\t\tswitch(objectU64)\n")
			glueF:write("\t\t\t{\n")
			for objectGUID in sortedpairs(objects) do
				glueF:write("\t\t\tcase 0x"..objectGUID.."ULL:\n")
				glueF:write("\t\t\t\treturn _RDX_CPPX::PCCMGlue::Call<0x"..domainGUID.."ULL, 0x"..objectGUID.."ULL>;\n")
			end
			glueF:write("\t\t\t};\n")
			glueF:write("\t\t}\n")
			glueF:write("\t\tbreak;\n")
		end
		glueF:write("\t};\n")
		glueF:write("\treturn RDX_CNULL;\n")
		glueF:write("}\n")
		]]--
		for domainGUID,objects in sortedpairs(pccmSymbols) do
			glueF:write("static const rdxSPCCMObjectIndex domainObj_"..domainGUID.."[] =\n")
			glueF:write("{\n")
			for objectGUID in sortedpairs(objects) do
				glueF:write("\t{ 0x"..objectGUID.."ULL, _RDX_CPPX::PCCMGlue::Call<0x"..domainGUID.."ULL, 0x"..objectGUID.."ULL> },\n")
			end
			glueF:write("};\n")
			glueF:write("\n")
		end
		glueF:write("static const rdxSPCCMDomainIndex exportedCallbacks[] =\n")
		glueF:write("{\n")
		for domainGUID,objects in sortedpairs(pccmSymbols) do
			local domainObjStr = "domainObj_"..domainGUID
			glueF:write("\t{ 0x"..domainGUID.."ULL, sizeof("..domainObjStr..") / sizeof("..domainObjStr.."[0]), "..domainObjStr.." },\n")
		end
		glueF:write("\t{ 0, RDX_CNULL }\n")
		glueF:write("};\n")
		glueF:write("\n")
		glueF:write("const rdxSPCCMDomainIndex *"..cppConfig.pccmGlueName.."()\n")
		glueF:write("{\n")
		glueF:write("\treturn exportedCallbacks;\n")
		glueF:write("}\n")
		glueF:close()

		do
			local clusterSize = cppConfig.clusterSize
			local clusterNum = 0
			local clusterLoad = 0
			local clusterF
			for domainGUID,objects in sortedpairs(pccmSymbols) do
				for objectGUID in sortedpairs(objects) do
					if clusterLoad == cppConfig.clusterSize then
						clusterLoad = 0
					end
					if clusterLoad == 0 then
						if clusterF ~= nil then
							clusterF:close()
						end
						clusterF = io.open(cppConfig.exportPath.."/"..cppConfig.clusterPath.."_"..clusterNum..".cpp", "wb")
						clusterNum = clusterNum + 1
					end
					clusterLoad = clusterLoad + 1
					clusterF:write("#include \"RDXPCCM/method_"..domainGUID.."_"..objectGUID..".cpp\"\n")
				end
			end
			if clusterF ~= nil then
				clusterF:close()
			end
		end
	end
end )
