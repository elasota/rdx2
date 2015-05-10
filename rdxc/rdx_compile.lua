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

--[[


The code generator operates on one of several access modes:
L: Local variable or parameter
P: Pointer to a value, located on the stack
CP: Constant pointer to a value, located on the stack
I: Undischarged intercept
A: Undischarged array index
R: Value on the stack

]]--

local SIGNAL_UnresolvedExpression = { }
local ST_InterceptType = { longName = "** INTERCEPT TARGET **" }
local ST_SetIndexType = { longName = "** SETINDEX TARGET **" }

local function throw(err)
	error(err)
end

local DumpCompiledInstructions
local InsertDeclarations
local NamespaceResolver
local TypeLongName
local InstanceTypeFromTypeNode
local UnresolvedExpression
local PrivatizedProperty
local CompileExpressionList
local AdjustValueCount
local EmitValues
local EmitParameters
local CompileCodeBlock
local CloseCodeBlock
local ExpressionToType
local ConstructTypeTemplate
local CastMatchability
local SingleExpressionToList
local CompileExpression
local EnforceAccessibility
local MatchMethodCall
local Scope
local DischargeIntercept
local TypeReference
local CompileTypeReference
local CloneMethodGroup
local CompileTypeShells
local LocalVariable
local EmitAssign
local EmitOperateAndAssign
local Constant
local AMIsPointer
local TypeDirectlyCastable
local TypePolymorphicallyCastable
local TypeImplementsInterface
local TypeTuple
local CompileParameterList
local ParameterList
local FlattenMV
local BlockMethod
local EmitAliasableJump

local enumType = "Core.uint"		-- This must match the type specified by RDX_ENUM_TYPE and the "value" field defined by Enumerant in Programmability.rdx
local arrayIndexType = "Core.largeuint"
local stringType = "Core.string"
local varyingType = "Core.varying"

local numCaseCollections = 0

setglobal("RDXT_RESERVED_SYMBOLS", set { "def", "import", "null", "res", "string", "enum" } )

local standardGuaranteeBlocks = { EXCEPTION = "-1", RETURN = "-2", DEFAULT = "0" }

local unaryOperatorMethods = {
	["-"] = "__neg",
}

local binaryOperatorMethods = {
	["*"] = "__mul",
	["*="] = "__mul",
	["%"] = "__mod",
	["%="] = "__mod",
	["/"] = "__div",
	["/="] = "__div",
	["+"] = "__add",
	["+="] = "__add",
	["++"] = "__add",
	["-"] = "__sub",
	["-="] = "__sub",
	["--"] = "__sub",

	["=="] = "__eq",
	["!="] = "__ne",
	["<="] = "__le",
	[">="] = "__ge",
	["<"] = "__lt",
	[">"] = "__gt",
}

local optionalJumpOpcodes = set {
	"jumpif", "jumpifnot", "jumpiftrue", "jumpiffalse", "jumpifequal", "jumpifnotequal",
	"try", "trycatch", "case", "iteratearray", "jumpifnull", "jumpifnotnull",
}

local nonInstructionOpcodes = set {
	"enablepath", "deadcode", "label", "checkpathusage"
}

local localReferencingOpcodes = set { "newinstanceset", "localref", "binddelegate" }


-- Opcodes which will follow str2 if the str1 label option fails, especially invertible tests.
-- Ops not in this list may use str2 for something else
local splittingJumpOpcodes = set {
	"jumpif", "jumpifnot", "jumpiftrue", "jumpiffalse", "jumpifequal", "jumpifnotequal", "jumpifnull", "jumpifnotnull",
}

local parseableTypes = set {
	"Core.string", "Core.int", "Core.uint", "Core.float", "Core.short", "Core.ushort", "Core.byte", "Core.char", "Core.bool"
}

local structuredTypeDeclTypes = set {
	"class",
	"struct",
	"enum",
	"interface"
}

local securityLevels =
{
	public = 1,
	protected = 2,
	private = 3,
}

local matchLevels =
{
	EXACT = 1,
	DIRECT = 2,
	LOSSLESS = 3,
	LOSSY = 4,
	VARYING = 5,
	POLYMORPHIC = 6,
	UNMATCHABLE = 7,
}

local function clonetable(tbl)
	local newTable = { }
	for k,v in pairs(tbl) do
		newTable[k] = v
	end
	return newTable
end

setglobal("cerror", function(node, reason, param1, param2, param3)
	terror(node.line, node.filename, true, "C", reason, param1, param2, param3)
end )

local function cwarning(node, reason, param1, param2, param3)
	terror(node.line, node.filename, false, "W", reason, param1, param2, param3)
end

local function DefaultVisibility(vs)
	if vs == nil then
		return "private"
	end
	return vs.string
end

FlattenMV = function(mv)
	local recursive = false
	for _,expr in ipairs(mv.expressions) do
		if expr.type == "CMultipleValues" then
			recursive = true
		end
	end
	
	if not recursive then
		return mv
	end

	local newExprList = { }
	for _,expr in ipairs(mv.expressions) do
		if expr.type == "CMultipleValues" then
			for _,subExpr in ipairs(expr.expressions) do
				newExprList[#newExprList+1] = subExpr
			end
		else
			newExprList[#newExprList+1] = expr
		end
	end

	return FlattenMV({
		type = "CMultipleValues",
		accessModes = clonetable(mv.accessModes),
		expressions = newExprList,
		vTypes = clonetable(mv.vTypes),
	})
end

local function EmptyExpressionList()
	return {
		type = "CMultipleValues",
		accessModes = { },
		expressions = { },
		vTypes = { },
	}
end

local function AppendExpressionToMV(mv, expr)
	for idx in ipairs(expr.vTypes) do
		mv.vTypes[#mv.vTypes+1] = expr.vTypes[idx]
		mv.accessModes[#mv.accessModes+1] = expr.accessModes[idx]
	end
	mv.expressions[#mv.expressions+1] = expr
end

local function VirtualParseNode(type, baseNode)
	return { type = type, line = baseNode.line, filename = baseNode.filename }
end

local function CombineAccessModes(exprs)
	local recombinedAccessModes = { }
	for _,expr in ipairs(exprs) do
		for _,am in ipairs(expr.accessModes) do
			recombinedAccessModes[#recombinedAccessModes+1] = am
		end
	end
	return recombinedAccessModes
end

local function AuditAccessModes(n)
	for _,am in ipairs(n) do
		assert(am ~= "*" and am ~= "*P")
	end
end

local function ConvertExpression(cs, incriminate, cnode, targetVTypes, targetAccessModes, allowPoly, allowBlackHole)
	local outAccessModes = nil
	
	if targetAccessModes ~= nil then
		outAccessModes = clonetable(targetAccessModes)
	end

	assert(incriminate, "AF1")
	if cnode.accessModes == nil or cnode.accessModes[1] == "I" then
		cerror(incriminate, "ExpectedValueExpression", cnode.type)
	end

	assert(#targetVTypes == #cnode.vTypes, "AF2")

	if cnode.type == "CConstant" and cnode.signal == "Value" then
		-- Coercible const fold
		if cnode.vTypes[1] ~= targetVTypes[1] then
			local coerceName = cnode.vTypes[1].longName.."/methods/#coerce("..targetVTypes[1].longName..")"
			local constFold = RDXC.Native["cf_"..coerceName]

			if constFold then
				local newValue = constFold(cnode.value)
				if newValue == nil then
					cwarning(incriminate, "CouldNotFoldConstant")
				else
					cnode = Constant(cs, targetVTypes[1], newValue, "Value")
				end
			end
		end
	end

	if cnode.type == "CMultipleValues" then
		-- Recurse into subexpressions instead
		local sliceIndex = 1
		local newExpressions = { }

		for _,expr in ipairs(cnode.expressions) do
			local sliceVTypes = { }
			local sliceAccessModes

			sliceAccessModes = { }

			for idx in ipairs(expr.vTypes) do
				sliceVTypes[idx] = targetVTypes[sliceIndex]
				if targetAccessModes then
					sliceAccessModes[idx] = targetAccessModes[sliceIndex]
				else
					sliceAccessModes[idx] = "*"
				end
				sliceIndex = sliceIndex + 1
			end

			local newExpr = ConvertExpression(cs, incriminate, expr, sliceVTypes, sliceAccessModes, allowPoly, allowBlackHole)
			AuditAccessModes(newExpr.accessModes)
			newExpressions[#newExpressions+1] = newExpr
		end

		outAccessModes = CombineAccessModes(newExpressions)

		return FlattenMV ({
			type = "CMultipleValues",
			accessModes = outAccessModes,
			vTypes = targetVTypes,
			expressions = newExpressions,
		})
	end

	local allAcceptable = true
	for idx in ipairs(cnode.accessModes) do
		local amsCompatible = true
		if targetAccessModes ~= nil then
			local tam = targetAccessModes[idx]
			local cnodeAM = cnode.accessModes[idx]

			if tam == "*P" then
				amsCompatible = (cnodeAM == "P" or cnodeAM == "CP")
			elseif tam == "*" then
				amsCompatible = true
			else
				amsCompatible = (cnodeAM == tam)
			end
		end
		if (not amsCompatible) or cnode.vTypes[idx] ~= targetVTypes[idx] then
			allAcceptable = false
			break
		end
	end

	if allAcceptable then
		return cnode	-- Nothing to do
	end

	outAccessModes = clonetable(cnode.accessModes)

	local dismantled = false

	-- Coerces will cause the coerced value to R of the expected type and all non-coerced values after to L from temp stores
	for idx,vt in ipairs(targetVTypes) do
		local isCoerce = false
		local matchability = CastMatchability(cnode.vTypes[idx], vt, allowBlackHole)

		if cnode.accessModes[idx] == "CP" and targetAccessModes ~= nil and targetAccessModes[idx] == "P" then
			cerror(incriminate, "CanNotStripConst")
		end

		if matchability == matchLevels.LOSSLESS or matchability == matchLevels.LOSSY or matchability == matchLevels.POLYMORPHIC then
			outAccessModes[idx] = "R"
			dismantled = true
			if (not allowPoly) and matchability == matchLevels.POLYMORPHIC then
				cerror(incriminate, "PolyNotAllowed", cnode.vTypes[idx].prettyName, vt.prettyName)
			end
		elseif matchability == matchLevels.VARYING then
			outAccessModes[idx] = "V"
			dismantled = true
		elseif matchability == matchLevels.UNMATCHABLE then
			cerror(incriminate, "CouldNotConvert", cnode.vTypes[idx].prettyName, vt.prettyName)
		else
			-- Anything after the dismantled value winds up as L
			if dismantled then
				outAccessModes[idx] = "L"
			end
		end
	end

	for idx,am in ipairs(outAccessModes) do
		assert(am ~= "*")
		if targetAccessModes ~= nil then
			local tam = targetAccessModes[idx]
			if tam == "*P" then
				if outAccessModes[idx] == "L" then
					outAccessModes[idx] = "P"
				end
			elseif tam ~= "*" then
				outAccessModes[idx] = tam
			end
		end
	end
	
	AuditAccessModes(outAccessModes)
	return {
		type = "CConvertExpression",
		expression = cnode,
		accessModes = outAccessModes,
		vTypes = targetVTypes,
		}
end

Constant = function(cs, st, str, signal)
	if type(st) == "string" then
		st = cs.gst[st]
		if st == nil then
			throw(SIGNAL_UnresolvedExpression)
		end
	end

	local accessMode = "R"
	if signal == "Resource" and not VTypeIsObjectReference(st) then
		accessMode = "P"
	end

	return {
		type = "CConstant",
		accessModes = { accessMode },
		vTypes = { st },
		value = str,
		signal = signal,
	}
end

local function MethodDelegation(cs, m, dt)
	return {
		type = "CMethodDelegation",
		accessModes = { "R" },
		vTypes = { dt },
		method = m,
		}
end

local function BoundMethodDelegation(cs, m, bdt, obj)

	return {
		type = "CBoundMethodDelegation",
		accessModes = { "L" },
		vTypes = { bdt },
		method = m,
		object = obj,
		}
end

local function PlaceholderValue(cs, st, accessMode)
	return {
		type = "CPlaceholderValue",
		accessModes = { accessMode or "R" },
		vTypes = { st },
		}
end


local function ObjectProperty(cs, left, member, incriminate)
	assert(incriminate)

	if not member.typeOf.isCompiled then
		throw(SIGNAL_UnresolvedExpression)
	end

	local leftVT = left.vTypes[1]

	left = AdjustValueCount(cs, left, incriminate, 1)
	
	local resultAM

	if leftVT.declType == "struct" or leftVT.declType == "enum" then
		left = ConvertExpression(cs, incriminate, left, left.vTypes, { "*P" }, false )
		resultAM = left.accessModes[1]

		if member.isConst then
			resultAM = "CP"
		end
	elseif leftVT.declType == "class" then
		left = ConvertExpression(cs, incriminate, left, left.vTypes, { "R" }, false )
		resultAM = "P"
	else
		assert(false, "Bad type to get a property from")
	end

	return {
		type = "CObjectProperty",
		object = left,
		property = member,
		accessModes = { resultAM },
		vTypes = { member.typeOf.refType },
	}
end

local function ObjectMethod(cs, left, member, incriminate)
	return {
		type = "CObjectMethod",
		object = left,
		methodGroup = member,
	}
end

local function MemberLookupScope(cs, owner, st)
	local sc = Scope(owner)
	sc.type = "CMemberLookupScope"
	sc.Lookup = function(self, symbol, incriminate)
			assert(incriminate)
			local v = st.namespace.symbols[symbol]
			if v ~= nil then
				if v.type == "CProperty" then
					return ObjectProperty(cs, self.thisLocal, v, incriminate)
				end
				if v.type == "CMethodGroup" and not v.isStatic then
					return ObjectMethod(cs, self.thisLocal, v, incriminate)
				end
			end
			if self.owner ~= nil then
				return self.owner:Lookup(symbol, incriminate)
			end
			return nil
		end

	return sc
	-- sc.thisLocal = ???
end

local function CompiledExprToBoolean(cs, incriminate, expr)
	local boolType = cs.gst["Core.bool"]
	if boolType == nil then
		throw(SIGNAL_UnresolvedExpression)
	end

	expr = AdjustValueCount(cs, expr, incriminate, 1)
	expr = ConvertExpression(cs, incriminate, expr, { boolType }, { "R" }, false )
	return expr
end

local function OperationToMethod(cs, node, scope)
	local leftOperand = AdjustValueCount(cs, CompileExpression(cs, node.operands[1], scope, true), node, 1)
	local operator

	if node.type == "BinaryOperatorNode" then
		local boolType = cs.gst["Core.bool"]
		if boolType == nil then
			throw(SIGNAL_UnresolvedExpression)
		end

		if node.operator.string == "&&" or node.operator.string == "||" then
			local rightOperand = AdjustValueCount(cs, CompileExpression(cs, node.operands[2], scope, true), node, 1)

			local leftExpr = ConvertExpression(cs, node, leftOperand, { boolType }, { "R" }, false )
			local rightExpr = ConvertExpression(cs, node, rightOperand, { boolType }, { "R" }, false )

			local t
			if node.operator.string == "&&" then
				t = "CLogicalAndNode"
			end
			if node.operator.string == "||" then
				t = "CLogicalOrNode"
			end
			
			assert(t)

			-- Fold early-outs
			if leftExpr.type == "CConstant" and leftExpr.signal == "Value" and leftExpr.vTypes[1] == boolType then
				if tostring(leftExpr.value) == "true" then
					if t == "CLogicalAndNode" then
						return rightExpr
					elseif t == "CLogicalOrNode" then
						return leftExpr
					else
						assert(false)
					end
				else
					assert(tostring(leftExpr.value) == "false")
					if t == "CLogicalAndNode" then
						return leftExpr
					elseif t == "CLogicalOrNode" then
						return rightExpr
					else
						assert(false)
					end
				end
			end

			return {
				type = t,
				vTypes = { boolType },
				accessModes = { "R" },
				leftExpr = leftExpr,
				rightExpr = rightExpr,
			}
		end

		operator = binaryOperatorMethods[node.operator.string]
	else
		if node.operator.string == "!" then
			local boolType = cs.gst["Core.bool"]
			if boolType == nil then
				throw(SIGNAL_UnresolvedExpression)
			end

			local expr = ConvertExpression(cs, node, leftOperand, { boolType }, { "R" }, false )

			if expr.type == "CConstant" and expr.signal == "Value" and expr.vTypes[1] == boolType then
				if expr.value == "true" then
					return Constant(cs, "Core.bool", "false", "Value")
				elseif expr.value == "false" then
					return Constant(cs, "Core.bool", "true", "Value")
				else
					assert(false)
				end
			end

			return {
				type = "CLogicalNotNode",
				vTypes = { boolType },
				accessModes = { "R" },
				expr = expr,
			}
		end

		assert(node.type == "UnaryOperatorNode")
		operator = unaryOperatorMethods[node.operator.string]
	end

	assert(operator)

	local mg = nil
	if leftOperand.vTypes[1].type == "CStructuredType" then
		mg = leftOperand.vTypes[1].namespace.symbols[operator]
	end

	if mg == nil then
		if operator == "__eq" or operator == "__ne" then
			local boolType = cs.gst["Core.bool"]
			if boolType == nil then
				throw(SIGNAL_UnresolvedExpression)
			end

			local desiredAccessModes
			if VTypeIsRefStruct(leftOperand.vTypes[1]) then
				desiredAccessModes = { "CP" }
			else
				desiredAccessModes = { "R" }
			end

			local rightOperand = AdjustValueCount(cs, CompileExpression(cs, node.operands[2], scope, true), node, 1)

			leftOperand = ConvertExpression(cs, node, leftOperand, { leftOperand.vTypes[1] }, desiredAccessModes, false)

			local matchability = CastMatchability(leftOperand.vTypes[1], rightOperand.vTypes[1])

			if matchability == matchLevels.DIRECT or matchability == matchLevels.POLYMORPHIC then
				-- Sufficiently similar
				rightOperand = ConvertExpression(cs, node, rightOperand, { rightOperand.vTypes[1] }, desiredAccessModes, false)
			else
				-- Requires a cast
				rightOperand = ConvertExpression(cs, node, rightOperand, { leftOperand.vTypes[1] }, desiredAccessModes, false)
			end

			-- Fold constants
			if leftOperand.type == "CConstant" and leftOperand.signal == "Value" and rightOperand.type == "CConstant" and rightOperand.signal == "Value" then
				local constv
				if leftOperand.value == rightOperand.value then
					constv = (operator == "__eq" and "true" or "false")
				else
					constv = (operator == "__ne" and "true" or "false")
				end
				return Constant(cs, "Core.bool", constv, "Value")
			end

			assert(leftOperand)
			assert(rightOperand)

			return {
				type = "CEqualityCompare",
				leftExpr = leftOperand,
				rightExpr = rightOperand,
				vTypes = { boolType },
				operator = operator,
				accessModes = { "R" },
			}
		end

		cerror(node, "CouldNotResolveOperator", operator)
	end
	if mg.type ~= "CMethodGroup" then
		cerror(node, "OperatorNotMethod", operator)
	end

	local objMethod = {
		type = "CObjectMethod",
		object = leftOperand,
		methodGroup = mg,
	}

	local parameters

	if node.operands[2] then
		local pbase = CompileExpression(cs, node.operands[2], scope, true)
		local padjusted = AdjustValueCount(cs, pbase, node, 1)
		assert(padjusted.accessModes)

		parameters = SingleExpressionToList(padjusted)
		assert(parameters.accessModes)
	else
		parameters = EmptyExpressionList()
	end

	return MatchMethodCall(cs, objMethod, parameters, node)
end

-- Polymorphic converts are possible in any of three scenarios:
-- 1.) A direct cast is possible
-- 2.) A direct cast of the target type to the source type is possible
-- 3.) Target is an interface and source is an object reference
-- This checks for the latter two, so that TypeDirectlyCastable(f,t) and TypePolymorphicallyCastable(f,t) returns the desired result
TypePolymorphicallyCastable = function(fromType, toType)
	if TypeDirectlyCastable(toType, fromType) or TypeImplementsInterface(toType, fromType) then
		return true
	end

	if fromType.type == "CStructuredType" and toType.type == "CStructuredType" then
		if fromType.declType == "interface" then
			return (toType.declType == "interface") or (toType.declType == "class" and not toType.isFinal)
		end
		if toType.declType == "interface" then
			return (fromType.declType == "class" and not fromType.isFinal)
		end
	end
	return false
end

TypeDirectlyCastable = function(fromType, toType)
	if fromType.type == "CArrayOfType" and (toType.longName == "Core.Array" or toType.longName == "Core.Object") then
		return true
	end

	if fromType.type == "CStaticDelegateType" and (toType.longName == "Core.RDX.Method" or toType.longName == "Core.Object") then
		return true
	end

	if fromType.longName == "Core.nullreference" and VTypeIsObjectReference(toType) then
		return true
	end

	-- Check inheritance casts
	if fromType.declType == "class" and toType.declType == "class" then
		local t = fromType
		while t ~= nil do
			if t == toType then
				return true
			end
			t = t.parentType
		end
	end

	if fromType.declType == "enum" and toType.longName == enumType then
		return true
	end
	
	-- This needs to match CastMatchability
	if fromType.type == "CArrayOfType" and toType.type == "CArrayOfType" then
		if fromType.dimensions ~= toType.dimensions then
			return false
		end

		local matchability = CastMatchability(fromType.containedType, toType.containedType)

		if matchability == matchLevels.EXACT and (toType.isConst or not fromType.isConst) then
			return true
		elseif matchability == matchLevels.DIRECT and toType.isConst then
			return true
		end
	end

	if fromType == toType then
		return true
	end

	return false
end

TypeImplementsInterface = function(fromType, toType)
	if fromType.declType == "class" and toType.declType == "interface" then
		for _,i in ipairs(fromType.interfaces) do
			if i.interfaceType == toType then
				return true
			end
		end
	end
	return false
end

local function FindCoerce(fromType, toType, lossless)
	if fromType.type ~= "CStructuredType" then
		return nil
	end

	local mg = fromType.namespace.symbols["#coerce"]
	if mg then
		for _,overload in ipairs(mg.overloads) do
			if overload.isLossless == lossless and overload.returnTypes.typeReferences[1].refType == toType then
				return overload
			end
		end
	end
end

CastMatchability = function(fromType, toType, allowBH)
	-- Black hole
	if allowBH and toType.longName == "Core.nullreference" then
		return matchLevels.DIRECT
	end

	-- Anything to varying
	if toType.longName == varyingType then
		return matchLevels.VARYING
	end

	if fromType.type == "CArrayOfType" and toType.type == "CArrayOfType" then
		if fromType.dimensions ~= toType.dimensions then
			return matchLevels.UNMATCHABLE
		end

		local matchability = CastMatchability(fromType.containedType, toType.containedType)

		if matchability == matchLevels.EXACT then
			if fromType.isConst and not toType.isConst then
				return matchLevels.POLYMORPHIC
			end
			if not fromType.isConst and toType.isConst then
				return matchLevels.DIRECT
			end
			return matchLevels.EXACT
		elseif matchability == matchLevels.DIRECT then
			if not toType.isConst then
				return matchLevels.UNMATCHABLE
			end
			return matchLevels.DIRECT
		elseif matchability == matchLevels.POLYMORPHIC then
			return matchLevels.POLYMORPHIC
		end
	end

	-- Match by name, since some types exist multiple times with the same name (delegates...)
	if fromType.longName == toType.longName then
		return matchLevels.EXACT
	end

	if TypeDirectlyCastable(fromType, toType) then
		return matchLevels.DIRECT
	end

	-- Find interface implementations
	if TypeImplementsInterface(fromType, toType) then
		return matchLevels.LOSSLESS
	end
	
	if fromType.type == "CStructuredType" and fromType.declType == "interface" and toType.longName == "Core.Object" then
		return matchLevels.LOSSLESS
	end

	-- Find coerces
	if fromType.type == "CStaticDelegateType" and toType.type == "CBoundDelegateType"
		and fromType.parameters.longName == toType.parameters.longName
		and fromType.returnTypes.longName == toType.returnTypes.longName then
		return matchLevels.LOSSLESS
	end

	if FindCoerce(fromType, toType, true) then
		return matchLevels.LOSSLESS
	end
	if FindCoerce(fromType, toType, false) then
		return matchLevels.LOSSY
	end

	if TypePolymorphicallyCastable(fromType, toType) then
		return matchLevels.POLYMORPHIC
	end

	return matchLevels.UNMATCHABLE
end

local function MethodMatchability(cs, vTypes, accessModes, method, thisIndex)
	if #vTypes ~= #method.actualParameterList.parameters then
		return matchLevels.UNMATCHABLE
	end

	local overallMatchability = matchLevels.EXACT
	for idx,vt in ipairs(vTypes) do
		local matchability
		local param = method.actualParameterList.parameters[idx]

		if idx == thisIndex then
			matchability = matchLevels.EXACT
		else
			matchability = CastMatchability(vt, param.type.refType)
		end

		if matchability == matchLevels.EXACT and VTypeIsRefStruct(param.type.refType) then
			-- Ref types may match, but still fail on const compatibility
			local am = accessModes[idx]
			if param.isConst then
				-- Passing mutable references to constant parameters is direct
				if am == "P" or am == "L" then
					matchability = matchLevels.DIRECT
				end
			else
				-- Passing constant references to mutable parameters prevents match
				if am == "CP" or am == "A" or am == "I" or am == "R" then
					matchability = matchLevels.UNMATCHABLE
				end
			end
		end

		if matchability > overallMatchability then
			overallMatchability = matchability
		end
	end

	return overallMatchability
end

local function InsertThisExpression(cs, parameters, thisIndex, thisExpr, incriminate)
	local inPVTypes
	local inPAccessModes
	local inExprs
	local outputtedThis = false

	assert(thisIndex)
	assert(parameters.type == "CMultipleValues", "AF6")

	inPVTypes = { }
	inPAccessModes = { }
	inExprs = { }

	for exprIdx,expr in ipairs(parameters.expressions) do
		if (#inPVTypes+1) == thisIndex then
			inPVTypes[thisIndex] = thisExpr.vTypes[1]
			inPAccessModes[thisIndex] = thisExpr.accessModes[1]
			inExprs[#inExprs+1] = thisExpr
			outputtedThis = true
		end

		inExprs[#inExprs+1] = expr
		for vidx,am in ipairs(expr.accessModes) do
			inPVTypes[#inPVTypes+1] = expr.vTypes[vidx]
			inPAccessModes[#inPAccessModes+1] = am
		end
	end

	if (#inPVTypes+1) == thisIndex then
		inPVTypes[thisIndex] = thisExpr.vTypes[1]
		inPAccessModes[thisIndex] = thisExpr.accessModes[1]
		inExprs[#inExprs+1] = thisExpr
		outputtedThis = true
	end

	if not outputtedThis then
		cerror(incriminate, "OverranThisParameter")
	end

	return FlattenMV({
		type = "CMultipleValues",
		accessModes = inPAccessModes,
		vTypes = inPVTypes,
		expressions = inExprs
		})
end


MatchMethodCall = function(cs, left, parameters, incriminate, explicit, delegate)
	local mg
	local thisIndex
	local finalExpressions
	local delegateExpr

	if delegate then
		mg = {
			type = "CDelegateMethodGroup",
			name = delegate.name,
			overloads = { delegate },
		}
		delegateExpr = left

		if delegate.type == "CBoundDelegateType" then
			-- FIXME: thisIndex can't be set because later code assumes it'll be used for an object invoke
			finalExpressions = InsertThisExpression(cs, parameters, 1, delegateExpr, incriminate)
			delegateExpr = nil
		else
			finalExpressions = parameters
		end
	elseif left.type == "CObjectMethod" then
		mg = left.methodGroup

		thisIndex = 1
		if mg.isIntercept and #parameters.expressions > 0 then
			thisIndex = 2
		end
		if mg.isArrayIntercept then
			thisIndex = 2
		end

		finalExpressions = InsertThisExpression(cs, parameters, thisIndex, left.object, incriminate)
		assert(finalExpressions.accessModes)
	elseif left.type == "CMethodGroup" then
		mg = left
		finalExpressions = parameters

		if not mg.isStatic then
			cerror(incriminate, "UnboundInstanceCall")
		end
	else
		assert(false, "AF7: Left type was "..left.type)
	end
	assert(mg.type == "CMethodGroup" or mg.type == "CDelegateMethodGroup", "AF8")

	local matches = {
		[matchLevels.EXACT] = { },
		[matchLevels.DIRECT] = { },
		[matchLevels.LOSSLESS] = { },
		[matchLevels.LOSSY] = { },
		[matchLevels.VARYING] = { }
		}

	local thisVT
	for idx,method in ipairs(mg.overloads) do
		local matchability = MethodMatchability(cs, finalExpressions.vTypes, finalExpressions.accessModes, method, thisIndex)
		local matchGroup = matches[matchability]
		if matchGroup ~= nil then
			matchGroup[#matchGroup+1] = method
		end
	end

	-- Find a match
	for matchLevel,matchGroup in ipairs(matches) do
		if #matchGroup > 0 then
			if #matchGroup > 1 then
				for _,m in ipairs(matchGroup) do
					io.stderr:write(m.longName.."\n")
				end
				cerror(incriminate, "AmbiguousMethodCall")
			end
			if matchLevel == matchLevels.LOSSY then
				cwarning(incriminate, "LossyConversion")
			end

			local method = matchGroup[1]

			local returnVTypes = { }
			local returnAccessModes = { }

			for idx,rt in ipairs(method.returnTypes.typeReferences) do
				returnVTypes[idx] = rt.refType

				if VTypeIsRefStruct(rt.refType) then
					returnAccessModes[idx] = "CP"
				else
					returnAccessModes[idx] = "R"
				end
			end

			local parameterVTypes = { }
			local parameterAccessModes = { }

			for idx,param in ipairs(method.actualParameterList.parameters) do
				local pt = param.type.refType

				if pt.longName == varyingType then
					parameterAccessModes[idx] = "V"
				elseif VTypeIsRefStruct(pt) then
					if param.isConst then
						parameterAccessModes[idx] = "CP"
					else
						parameterAccessModes[idx] = "P"
					end
				else
					parameterAccessModes[idx] = "R"
				end

				parameterVTypes[idx] = pt
			end
			
			-- See if null was passed to any notnull parameter
			do
				local startParameter = 1
				for _,expr in ipairs(finalExpressions.expressions) do
					if expr.type == "CConstant" and expr.vTypes[1].longName == "Core.nullreference" and method.actualParameterList.parameters[startParameter].isNotNull then
						cwarning(incriminate, "NullPassedToNotNull")
					end

					startParameter = startParameter + #expr.vTypes
				end
			end

			local convertedParameters = ConvertExpression(cs, incriminate, finalExpressions, parameterVTypes, parameterAccessModes, false)

			local canFold = true

			for _,p in ipairs(convertedParameters.expressions) do
				if p.type ~= "CConstant" or p.signal ~= "Value" then
					canFold = false
					break
				end
			end

			if canFold and RDXC.Native["cf_"..method.longName] then
				local returnType = returnVTypes[1]
				local parameterValues = { }
				for i,expr in ipairs(convertedParameters.expressions) do
					parameterValues[i] = expr.value
				end

				local newValue = RDXC.Native["cf_"..method.longName](unpack(parameterValues))
				if newValue == nil then
					cwarning(incriminate, "CouldNotFoldConstant")
				else
					return Constant(cs, returnVTypes[1], newValue, "Value")
				end
			end

			if explicit and method.isAbstract then
				cerror(incriminate, "AbstractMethodInvokedExplicitly")
			end

			return {
				type = "CMethodCall",
				method = method,
				explicit = explicit,
				delegateExpr = delegateExpr,
				parameters = convertedParameters,
				vTypes = returnVTypes,
				accessModes = returnAccessModes,
			}
		end
	end
	cerror(incriminate, "CouldNotMatchOverload", mg.name)
end

DischargeIntercept = function(cs, v, incriminate, discharging)
	if v.type ~= "CObjectMethod" or not v.methodGroup.isIntercept then
		-- Not an intercept node
		return v
	end

	if not discharging then
		v.vTypes = { ST_InterceptType }
		v.accessModes = { "I" }
		return v
	end

	-- Execute with no parameters to get the return value
	return MatchMethodCall(cs, v, EmptyExpressionList(), incriminate)
end


local function CompileIndexExpression(cs, node, scope, discharging)
	-- 3 scenarios:
	-- - Array indexes are P-access values
	-- - Index operations on non-arrays are __index when discharging and __setindex when not discharging

	local leftExpr = CompileExpression(cs, node.operands[1], scope, true)

	if leftExpr.accessModes == nil or #leftExpr.accessModes == 0 then
		cerror(node, "ExpectedValueForIndex")
	end

	local leftVType = leftExpr.vTypes[1]
	local indexes = CompileExpressionList(cs, node.operands[2], scope, true)

	if leftVType.type == "CArrayOfType" then
		local arrayIndexST = cs.gst[arrayIndexType]
		if arrayIndexST == nil then
			throw(SIGNAL_UnresolvedExpression)
		end

		if #indexes.accessModes ~= leftVType.dimensions then
			cerror(node, "ArrayIndexCountMismatch", tostring(leftVType.dimensions), tostring(#indexes.expressions))
		end

		local rvLeft = ConvertExpression(cs, node, AdjustValueCount(cs, leftExpr, node, 1), { leftVType }, { "R" }, false )  

		-- Convert all indexes to the array index type and as values
		local arrayIndexVTypes = { }
		local arrayIndexAccessModes = { }
		for _,idx in ipairs(indexes.expressions) do
			arrayIndexVTypes[#arrayIndexVTypes+1] = arrayIndexST
			arrayIndexAccessModes[#arrayIndexAccessModes+1] = "R"
		end

		local resultAM = (leftVType.isConst and "CP" or "P")

		return {
			type = "CArrayIndex",
			operand = rvLeft,
			indexes = ConvertExpression(cs, node, indexes, arrayIndexVTypes, arrayIndexAccessModes, false ),
			vTypes = { leftVType.containedType },
			accessModes = { resultAM },
		}
	elseif leftVType.type == "CStructuredType" then
		if not leftVType.isCompiled or not leftVType.finalizer.isCompiled then
			throw(SIGNAL_UnresolvedExpression)
		end

		local targetAccessMode
		if VTypeIsRefStruct(leftVType) then
			targetAccessMode = "*P"
		else
			targetAccessMode = "R"
		end

		local pvLeft = ConvertExpression(cs, node, AdjustValueCount(cs, leftExpr, node, 1), { leftVType }, { targetAccessMode }, false )

		if discharging then
			local mg = leftVType.namespace.symbols["__index"]

			if mg == nil then
				cerror(node, "CouldNotFindIndexMethod")
			end
			if mg.type ~= "CMethodGroup" then
				cerror(node, "IndexMemberNotMethod")
			end

			local objMethod = {
				type = "CObjectMethod",
				object = pvLeft,
				methodGroup = mg,
			}

			return MatchMethodCall(cs, objMethod, indexes, node)
		else
			if targetAccessMode == "*P" and leftExpr.accessModes[1] == "R" then
				cerror(node, "IndexedObjectWasValue")
			end

			local mg = leftVType.namespace.symbols["__setindex"]

			if mg == nil then
				cerror(node, "CouldNotFindSetIndexMethod")
			end
			if mg.type ~= "CMethodGroup" then
				cerror(node, "SetIndexMemberNotMethod")
			end


			local objMethod = {
				type = "CObjectMethod",
				object = pvLeft,
				methodGroup = mg,
			}

			return {
				type = "CSetIndexCall",
				methodCall = objMethod,
				indexes = indexes,
				accessModes = { "A" },
				vTypes = { ST_SetIndexType },
			}
		end
	else
		assert(false)
	end
end

local function CompileInitializeInstance(cs, expr, scope, parametersNode, incriminate)
	local parameters

	if parametersNode then
		parameters = CompileExpressionList(cs, parametersNode, scope, true)
	else
		parameters = EmptyExpressionList()
	end

	local leftVT = expr.vTypes[1]
	local initMG = leftVT.namespace.symbols["Initialize"]

	if initMG == nil or initMG.type ~= "CMethodGroup" then
		if parametersNode == nil or #parametersNode.expressions == 0 then
			-- Don't bother initializing
			if VTypeIsRefStruct(leftVT) then
				return {
					type = "CAllocateTemporary",
					vTypes = { leftVT },
					accessModes = { "L" },
				}
			else
				return expr
			end
		end
		cerror(incriminate, "TypeDoesNotHaveInitialize")
	end

	local cloneExpr
	local recoveryAM

	local baseExpr
	if VTypeIsObjectReference(leftVT) then
		recoveryAM = "R"
		baseExpr = expr
	elseif VTypeIsRefStruct(leftVT) then
		recoveryAM = "L"
		baseExpr = {
			type = "CAllocateTemporary",
			vTypes = { leftVT },
			accessModes = { "L" },
		}
	else
		cerror(incriminate, "InitializeOnNonReferenceType")
	end

	cloneExpr = {
		type = "CCloneExpression",
		vTypes = { leftVT },
		accessModes = { recoveryAM },
		expression = baseExpr
	}


	local objMethod = ObjectMethod(cs, cloneExpr, initMG, incriminate)

	local mc = MatchMethodCall(cs, objMethod, parameters, incriminate, true)		-- Constructors are always explicit

	if #mc.vTypes ~= 0 then
		cerror(incriminate, "InitializeMethodReturnsValues")
	end

	return {
		type = "CInitializeAndRecover",
		vTypes = { leftVT },
		accessModes = { recoveryAM },
		disallowWrites = true,
		expression = mc
	}
end

local function CompileInitializeArray(cs, expr, scope, initializersNode)
	local leftVT = expr.vTypes[1]
	local exprList = CompileExpressionList(cs, initializersNode, scope, true)

	local targetVType = expr.vTypes[1].containedType
	assert(targetVType)

	local pInitializerNode = {
		type = "CInitializeArray",
		initializers = { },
		dimensions = { },
		localVar = LocalVariable(nil, leftVT, false),
		vTypes = { leftVT },
		accessModes = { "L" },
		incriminateNode = initializersNode,
	}

	local dimCount = 1

	for _,iexpr in ipairs(exprList.expressions) do
		local subExpr = AdjustValueCount(cs, iexpr, initializersNode, 1)
		subExpr = ConvertExpression(cs, initializersNode, subExpr, { targetVType }, nil, false)

		pInitializerNode.initializers[#pInitializerNode.initializers+1] = subExpr
	end


	if expr.parameters == nil then
		if leftVT.dimensions ~= 1 then
			cerror(initializersNode, "ParameterlessInitializerOnMultidimensionalArray")
		end
		local d = #exprList.expressions
		pInitializerNode.dimensions[1] = d
		dimCount = d
	else
		for _,iexpr in ipairs(expr.parameters.expressions) do
			if iexpr.type ~= "CConstant" then
				cerror(initializersNode, "InitializerDimensionNotConstant")
			end
			if iexpr.vTypes[1].longName ~= arrayIndexType then
				cerror(initializersNode, "InitializerDimensionWrongType")
			end

			local d = tonumber(iexpr.value)

			if d < 0 then
				cerror(initializersNode, "NegativeDimension")
			end

			pInitializerNode.dimensions[#pInitializerNode.dimensions+1] = d
			dimCount = dimCount * d
		end

		if dimCount ~= #exprList.expressions then
			cerror(initializersNode, "DimensionInitializerMismatch", tostring(dimCount), tostring(#exprList.expressions))
		end
	end


	return pInitializerNode
end

local function CompileInitializeProperties(cs, expr, scope, initializersNode)
	local leftVT = expr.vTypes[1]

	assert(leftVT.type ~= "CArrayOfType")

	local baseExpr
	if leftVT.declType == "class" or leftVT.declType == "struct" then
		baseExpr = expr
	else
		cerror(initializersNode, "InitializerOnNonStructuredType")
	end

	local mls = MemberLookupScope(cs, nil, leftVT)

	local pInitializerNode = {
		type = "CInitializeProperties",
		initializers = { },
		localVar = LocalVariable(nil, leftVT, false),
		vTypes = { leftVT },
		accessModes = { "L" },
		incriminateNode = initializersNode,
	}

	mls.thisLocal = pInitializerNode.localVar

	local propertiesSet = { }
	for _,p in ipairs(initializersNode.initializers) do
		local pName = p.name.string

		if propertiesSet[pName] then
			cerror(p, "PropertyAlreadyInitialized", pName)
		end
		propertiesSet[pName] = true

		local dest = mls:Lookup(pName, p)
		if dest == nil then
			cerror(p, "InitializerPropertyNotFound", pName)
		end
		if dest.type ~= "CObjectProperty" then
			cerror(p, "InitializedNonProperty")
		end

		local src = AdjustValueCount(cs, CompileExpression(cs, p.expression, scope, true), p.expression, 1)

		-- Intercepts need to be deferred to emission due to some shitty design with the assign emitter
		if dest.vTypes[1] ~= ST_InterceptType then
			src = ConvertExpression(cs, p, src, { dest.vTypes[1] }, nil, false)
		end

		pInitializerNode.initializers[#pInitializerNode.initializers+1] = {
			type = "CPropertyInitializer",
			dest = dest,
			src = src,
			incriminateNode = p,
		}
	end

	return pInitializerNode
end

local function CompileTernary(cs, node, scope)
	assert(node.operands[2])

	local trueExprs = CompileExpression(cs, node.operands[1], scope, true)
	local falseExprs = CompileExpression(cs, node.operands[2], scope, true)

	local numTrue = #trueExprs.vTypes
	local numFalse = #falseExprs.vTypes
	local numExprs = numTrue
	
	if numTrue == 0 or numFalse == 0 then
		cerror(node, "ConditionalExpressionNotValue")
	end

	if numTrue > numFalse then
		trueExprs = AdjustValueCount(cs, trueExprs, node, numFalse)
		numExprs = numFalse
	elseif numFalse > numTrue then
		falseExprs = AdjustValueCount(cs, falseExprs, node, numTrue)
	end
	
	local targetVTypeSet = { }
	local resultAccessModes = { }
	
	for i=1,numExprs do
		local preferVType
		local trueVType = trueExprs.vTypes[i]
		local falseVType = falseExprs.vTypes[i]

		if trueVType.longName == "Core.nullreference" then
			trueVType = cs.gst["Core.Object"]
		end
		if falseVType.longName == "Core.nullreference" then
			falseVType = cs.gst["Core.Object"]
		end
		
		if trueVType == falseVType then
			preferVType = trueVType
		else
			if TypeDirectlyCastable(falseVType, trueVType) then
				preferVType = trueVType
			elseif TypeDirectlyCastable(trueVType, falseVType) then
				preferVType = falseVType
			else
				local reduceSet = { }

				-- See if we can reduce one of the values to the other
				if trueVType.type == "CStructuredType" then
					for _,interf in ipairs(trueVType.interfaces) do
						reduceSet[interf] = true
					end
					local pType = trueVType
					while pType ~= nil do
						reduceSet[pType] = true
						pType = pType.parentType
					end

					for _,interf in ipairs(falseVType.interfaces) do
						if reduceSet[interf.interfaceType] then
							if preferVType ~= nil then
								cerror(node, "TernaryPairResolvesToMultipleInterfaces", tostring(i))
							end
							preferVType = interf.interfaceType
						end
					end
					
					pType = falseVType
					while pType ~= nil do
						if reduceSet[pType] then
							preferVType = pType
							break;
						end
						pType = pType.parentType
					end
				end
				
				if preferVType == nil then
					cerror(node, "TernaryPairNotConvertable", tostring(i))
				end
			end
		end
		targetVTypeSet[i] = preferVType
		resultAccessModes[i] = "R"
		if VTypeIsRefStruct(preferVType) then
			resultAccessModes[i] = "CP"
		end
	end

	trueExprs = ConvertExpression(cs, node, trueExprs, targetVTypeSet, nil, false)
	falseExprs = ConvertExpression(cs, node, falseExprs, targetVTypeSet, nil, false)

	local cond = CompiledExprToBoolean(cs, node, CompileExpression(cs, node.condition, scope, true))

	-- Fold static conditions
	if cond.type == "CConstant" and cond.signal == "Value" and cond.vTypes[1] == cs.gst["Core.bool"] then
		if tostring(cond.value) == "true" then
			return trueExprs
		else
			return falseExprs
		end
	end

	return {
		type = "CTernary",
		trueExprs = trueExprs,
		falseExprs = falseExprs,
		cond = cond,
		vTypes = targetVTypeSet,
		accessModes = resultAccessModes,
		incriminateNode = node
	}
end

local function DelegateMethodGroup(cs, mg, dt, incriminate)
	-- Make sure this is static
	if not mg.isStatic then
		cerror(incriminate, "DelegatedBoundMethod")
	end

	if not dt.isCompiled then
		throw(SIGNAL_UnresolvedExpression)
	end

	-- Find an appropriate overload
	for _,m in ipairs(mg.overloads) do
		if m.parameterList.longName == dt.parameters.longName and
			m.returnTypes.longName == dt.returnTypes.longName then
			return MethodDelegation(cs, m, dt)
		end
	end

	cerror(incriminate, "CouldNotMatchDelegate")
end

local function DelegateBoundMethod(cs, obj, mg, bdt, incriminate)
	if not bdt.isCompiled then
		throw(SIGNAL_UnresolvedExpression)
	end
	
	local objV
	if not mg.isStatic then
		objV = AdjustValueCount(cs, obj, incriminate, 1)
		local bindTargetAM = "R"
		if VTypeIsRefStruct(objV.vTypes[1]) then
			bindTargetAM = "CP"
		end
		objV = ConvertExpression(cs, incriminate, objV, objV.vTypes, { bindTargetAM }, false )
	end
	
	-- Find an appropriate overload
	for _,m in ipairs(mg.overloads) do
		local mParams = m.parameterList
		local dtParams = bdt.parameters
		if m.returnTypes.longName == bdt.returnTypes.longName and #mParams == #dtParams then
			local matched = true
			for idx,mParam in ipairs(mParams) do
				local dtParam = dtParams[idx]
				if idx ~= m.thisParameterOffset and mParam.type.refType ~= dtParam.type.refType then
					matched = false
					break
				end
				if dtParam.isNotNull ~= mParam.isNotNull or dtParam.isConstant ~= mParam.isConstant then
					matched = false
					break
				end
			end

			if matched then
				return BoundMethodDelegation(cs, m, bdt, objV)
			end
		end
	end

	cerror(incriminate, "CouldNotMatchDelegate")
end

-- Enforces that "symbol" in "namespace" is accessible from "scope"
-- If the namespace is a parent of the scope, then the symbol is always accessible
-- Otherwise, the class owning both scopes are found and inheritance is checked
EnforceAccessibility = function(cs, scope, namespace, symbol, incriminate)
	local scopeScan = scope

	while scopeScan ~= nil do
		if scopeScan.type == "CInstanciatedNamespace" and scopeScan.namespace == namespace then
			-- The symbol namespace is a parent of the current namespace, so this is always accessible
			return
		end
		scopeScan = scopeScan.owner
	end

	-- If the owner namespace isn't accessible, then the symbol must be public
	local symbolVisibility = namespace.symbols[symbol].visibility

	if symbolVisibility == "private" then
		cerror(incriminate, "InaccessiblePrivateMember", symbol)
	end

	if symbolVisibility == "protected" then
		local scopeType = nil
		local scopeTypeScan = scope
		while scopeTypeScan ~= nil do
			if scopeTypeScan.type == "CInstanciatedNamespace" then
				if scopeTypeScan.boundType then
					scopeType = scopeTypeScan.boundType
					break
				end
			end
			scopeTypeScan = scopeTypeScan.owner
		end

		while scopeType ~= nil do
			if scopeType.namespace == namespace then
				-- Found a matching type in the inheritance tree
				return
			end
			scopeType = scopeType.baseClass
		end

		cerror(incriminate, "InaccessibleProtectedMember", symbol)
	end
end

-- node: Node to compile
-- scope: Scope that the expression exists in
-- discharging: If true, then this is a read attempt and should discharge intercepts
--
-- This returns a compiled expression, it does NOT emit code.
CompileExpression = function(cs, node, scope, discharging)
	local v

	assert(discharging ~= nil)
	assert(node)

	if node.type == "Name" or node.type == "this" then
		v = scope:Lookup(node.string, node)
		if v == nil then
			cerror(node, "UnresolvedName", node.string)
		end
		
		if v.type == "CLocalVariable" and v.method ~= nil and v.method ~= BlockMethod(scope) then
			cerror(node, "AccessedExternalLocal")
		end

		v = DischargeIntercept(cs, v, node, discharging)
		assert(v, "AF9")
	elseif node.type == "Indirect" then
		local left = CompileExpression(cs, node.operands[1], scope, true)

		if left.type == "CStructuredType" or left.type == "CNamespace" then
			if left.type == "CStructuredType" then
				if not left.isCompiled then
					throw(SIGNAL_UnresolvedExpression)
				end
				left = left.namespace
			end

			local symbol = node.operands[2].string
			v = left.symbols[symbol]
			if v == nil then
				cerror(node.operands[2], "UnresolvedMember", symbol, left.prettyName)
			end

			if (v.type == "CProperty" or v.type == "CMethodGroup") and not v.isStatic then
				cerror(node.operands[2], "ExpectedStaticMember", symbol)
			end

			EnforceAccessibility(cs, scope, left, symbol, node)

			-- Fall through, this could be a type ref
		else --if left.type == "CLocalVariable" or left.type == "CObjectProperty" or left.type == "CStaticInstance" then
			left = AdjustValueCount(cs, left, node, 1)

			if not left.vTypes[1].isCompiled then
				throw(SIGNAL_UnresolvedExpression)
			end

			if left.vTypes[1].type == "CArrayOfType" then
				local arrayType = cs.gst["Core.Array"]
				if arrayType == nil then
					throw(SIGNAL_UnresolvedExpression)
				end
				left = ConvertExpression(cs, node, left, { arrayType }, nil, false )
			elseif left.vTypes[1].type == "CStaticDelegateType" then
				local methodType = cs.gst["Core.RDX.Method"]
				if methodType == nil then
					throw(SIGNAL_UnresolvedExpression)
				end
				left = ConvertExpression(cs, node, left, { methodType }, nil, false )
			end

			local leftVT = left.vTypes[1]
			local namespace = leftVT.namespace

			local symbol = node.operands[2].string
			local right = namespace.symbols[symbol]
			if right == nil then
				-- If this is an interface, try from Core.Object
				if leftVT.type == "CStructuredType" and leftVT.declType == "interface" then
					local objType = cs.gst["Core.Object"]

					if objType == nil then
						throw(SIGNAL_UnresolvedExpression)
					end
					namespace = objType.namespace
					right = namespace.symbols[symbol]
					if right == nil then
						cerror(node.operands[2], "UnresolvedMember", symbol, "Core.Object")
					end
				else
					cerror(node.operands[2], "UnresolvedMember", symbol, leftVT.prettyName)
				end
			end

			EnforceAccessibility(cs, scope, namespace, symbol, node)

			if (right.type == "CProperty" or right.type == "CMethodGroup") and right.isStatic then
				cerror(node.operands[2], "ExpectedInstanceMember", symbol)
			end

			if right.type == "CProperty" then
				return ObjectProperty(cs, left, right, node)
			end
			if right.type == "CMethodGroup" then
				if right.isArrayIntercept then
					cerror(node, "CanNotAccessArrayIntercept", symbol)
				end
				local objMethod = ObjectMethod(cs, left, right, node)
				objMethod.explicit = node.explicit
				return DischargeIntercept(cs, objMethod, node, discharging)
			end
			assert(false, "AF10")
		--else
		--	error("Unimplemented indirection of left type "..left.type)
		end
	elseif node.type == "Invoke" then
		local left = CompileExpression(cs, node.operands[1], scope, true)
		local parameters

		assert(left, "AF11")

		if node.operands[2] == nil then
			parameters = EmptyExpressionList()
		else
			parameters = CompileExpressionList(cs, node.operands[2], scope, true)
		end

		local delegateType = nil
		if left.vTypes and #left.vTypes > 0 then
			local leftVType = left.vTypes[1]
			if leftVType.type == "CStaticDelegateType" or leftVType.type == "CBoundDelegateType" then
				delegateType = left.vTypes[1]
				left = ConvertExpression(cs, node, AdjustValueCount(cs, left, node, 1), { delegateType }, { "L" }, false )
			end
		end

		if left.type ~= "CMethodGroup" and left.type ~= "CObjectMethod" and delegateType == nil then
			cerror(node, "CalledNonMethod")
		end

		if left.isIntercept or left.isArrayIntercept then
			cerror(node, "CalledIntercept")
		end

		if left.type == "CMethodGroup" and not left.isCompiled then
			throw(SIGNAL_UnresolvedExpression)
		end

		assert(parameters.accessModes)
		return MatchMethodCall(cs, left, parameters, node, left.explicit, delegateType)
	elseif node.type == "BinaryOperatorNode" then
		return OperationToMethod(cs, node, scope)
	elseif node.type == "UnaryOperatorNode" then
		return OperationToMethod(cs, node, scope)
	elseif node.type == "null" then
		return Constant(cs, "Core.nullreference", "null", "NullRef")
	elseif node.type == "String" then
		return Constant(cs, "Core.string", node.string, "Value")
	elseif node.type == "true" or node.type == "false" then
		return Constant(cs, "Core.bool", node.type, "Value")
	elseif node.type == "Number" then
		local v, t, what = RDXC.Native.parseNumber(node.string)
		return Constant(cs, t, v, "Value")
	elseif node.type == "Template" then
		local left = CompileExpression(cs, node.operands[1], scope, true)

		if left.type ~= "CStructuredType" then
			cerror(node, "TemplateFromNonStructuredType")
		end

		if not left.isCompiled then
			throw(SIGNAL_UnresolvedExpression)
		end

		if not left.isTemplate then
			cerror(node, "TemplateFromNonStructuredType")
		end

		local templateParameters = { }
		for _,trNode in ipairs(node.operands[2].types) do
			local tArg = TypeReference(cs, trNode, scope, false)
			CompileTypeReference(cs, tArg)

			if (not tArg.isCompiled) or (not tArg.refType.isCompiled) then
				throw(SIGNAL_UnresolvedExpression)
			end

			templateParameters[#templateParameters+1] = tArg.refType
		end

		if #left.uncompiledNode.templateParameters.parameters ~= #templateParameters then
			cerror(node, "TemplateParameterMismatch")
		end
		v = ConstructTypeTemplate(cs, left, templateParameters)

		if not cs.compilingTypes then
			-- This was declared inside a method, reestablish types types
			CompileTypeShells(cs)
			assert(v.isCompiled, "AF12")
		end
	elseif node.type == "CLocalVariable" or
		node.type == "CArrayIndex" or
		node.type == "CSetIndexCall" or
		node.type == "CObjectProperty" or
		node.type == "CStaticInstance" or
		node.type == "CRecycledValues" or
		node.type == "CMethodCall" or
		node.type == "CConstant" or
		node.type == "CTernary" then
		-- Precompiled node
		return node
	elseif node.type == "CObjectMethod" and node.vTypes ~= nil then
		return node
	elseif node.type == "Index" then
		return CompileIndexExpression(cs, node, scope, discharging)
	elseif node.type == "CheckCast" then
		local boolType = cs.gst["Core.bool"]
		if boolType == nil then
			throw(SIGNAL_UnresolvedExpression)
		end

		local left = AdjustValueCount(cs, CompileExpression(cs, node.operands[1], scope, true), node, 1)

		local tr = TypeReference(cs, node.operands[2], scope, true)
		CompileTypeReference(cs, tr)
		if not tr.isCompiled then
			cerror(node, "CastToNonType")
		end

		local leftType = left.vTypes[1]
		local rightType = tr.refType
		if TypeDirectlyCastable(leftType, rightType) then
			-- If this is a guaranteed convert, always accept
			return Constant(cs, "Core.bool", "true", "Value")
		end
		-- If this is a polymorphic convert, then check
		if TypePolymorphicallyCastable(leftType, rightType) then
			if leftType.type == "CStructuredType" and leftType.declType == "interface" then
				local objectClass = cs.gst["Core.Object"]
				left = ConvertExpression(cs, node, left, { objectClass }, { "R" }, false)
				leftType = objectClass
			end
			return {
				type = "CCheckCast",
				expression = ConvertExpression(cs, node, left, { leftType }, { "R" }, true),
				checkType = rightType,
				vTypes = { boolType },
				accessModes = { "R" },
			}
		end
		cwarning(node, "ImpossibleConversionCheck")
		return Constant(cs, "Core.bool", "false", "Value")
	elseif node.type == "Cast" then
		local left = CompileExpression(cs, node.operands[1], scope, true)
		local targetTypeRefs = { }

		if node.multipleTypes then
			for i,t in ipairs(node.operands[2].types) do
				targetTypeRefs[i] = t
			end
		else
			targetTypeRefs[1] = node.operands[2]
		end
		
		local targetVTypes = { }
		local targetAccessModes = { }
		for tri,trNode in ipairs(targetTypeRefs) do
			local tr = TypeReference(cs, trNode, scope, true)
			CompileTypeReference(cs, tr)
			if not tr.isCompiled then
				cerror(node, "CastToNonType")
			end
			local right = tr.refType

			if right.type ~= "CStructuredType" and right.type ~= "CArrayOfType" and right.type ~= "CStaticDelegateType" and right.type ~= "CBoundDelegateType" then
				cerror(node, "CastToNonType")
			end

			if left.type == "CMethodGroup" then
				if node.multipleTypes then
					cerror(node, "DelegatedToMultipleTypes")
				end
				
				if right.type == "CStaticDelegateType" then
					return DelegateMethodGroup(cs, left, right, node)
				elseif right.type == "CBoundDelegateType" then
					return DelegateBoundMethod(cs, nil, left, right, node)
				else
					cerror(node, "BadDelegation", PrettyInternalClasses[right.type])
				end
			end
			
			if left.type == "CObjectMethod" then
				if node.multipleTypes then
					cerror(node, "DelegatedToMultipleTypes")
				end
				if right.type ~= "CBoundDelegateType" then
					cerror(node, "BadDelegation", PrettyInternalClasses[right.type])
				end
				
				return DelegateBoundMethod(cs, left.object, left.methodGroup, right, node)
			end

			if left.accessModes == nil then
				cerror(node, "ExpectedExpression", PrettyInternalClasses[left.type])
			end

			targetAccessModes[tri] = "R"
			if VTypeIsRefStruct(right) then
				targetAccessModes[tri] = "CP"
			end
			
			targetVTypes[tri] = right
		end

		return ConvertExpression(cs, node, AdjustValueCount(cs, left, node, #targetTypeRefs), targetVTypes, targetAccessModes, true )
	elseif node.type == "SingleResultNode" then
		local left = CompileExpression(cs, node.operands[1], scope, true)
		return AdjustValueCount(cs, left, node, 1)
	elseif node.type == "NewInstanceNode" then
		local tr = TypeReference(cs, node.typeSpec, scope, true)
		CompileTypeReference(cs, tr)
		if not tr.isCompiled then
			cerror(node, "NewCreatedNonType")
		end

		local t = tr.refType

		if t.type == "CStructuredType" and t.declType == "interface" then
			cerror(node, "NewInterface")
		end

		if t.isAbstract then
			cerror(node, "NewAbstractType", t.abstractBlame.signature)
		end

		if t.type == "CStaticDelegateType" or t.type == "CBoundDelegateType" then
			cerror(node, "NewDelegate")
		end

		local instance = {
			type = "CNewInstance",
			parameters = tr.specifiedDimensions,
			accessModes = { "R" },
			vTypes = { t },
		}

		if t.type == "CArrayOfType" then
			if node.initializers then
				if node.initializers.type == "PropertyInitializerListNode" then
					cerror(node, "InitializedArrayWithProperties")
				end
				return CompileInitializeArray(cs, instance, scope, node.initializers)
			else
				if instance.parameters == nil then
					cerror(node, "ExpectedDimensionsForArrayCreation")
				end
				return instance
			end
		else
			if node.initializers then
				if node.initializers.type == "ExpressionList" then
					if #node.initializers.expressions == 0 then
						-- Empty expression list, convert to an empty property initializer set
						node.initializers = VirtualParseNode("PropertyInitializerListNode", node.initializers)
						node.initializers.initializers = { }
					else
						cerror(node, "InitializedObjectWithExpressions")
					end
				end
				return CompileInitializeProperties(cs, instance, scope, node.initializers)
			else
				return CompileInitializeInstance(cs, instance, scope, node.initParameters, node)
			end
		end
	elseif node.type == "GenerateHashNode" then
		local left = CompileExpression(cs, node.expression, scope, true)
		left = AdjustValueCount(cs, left, node, 1)

		local desiredAccessMode = "R"
		if VTypeIsRefStruct(left.vTypes[1]) then
			desiredAccessMode = "CP"
		end

		left = ConvertExpression(cs, node, left, left.vTypes, { desiredAccessMode }, false )

		local t = cs.gst["Core.hashcode"]

		if t == nil then
			throw(SIGNAL_UnresolvedExpression)
		end

		return {
			type = "CGenerateHashNode",
			expression = left,
			accessModes = { "R" },
			vTypes = { t }
		}
	elseif node.type == "TypeOfNode" then
		local left = CompileExpression(cs, node.expression, scope, true)

		if left.type == "CStructuredType" or left.type == "CBoundDelegateType" then
			if not left.isCompiled then
				throw(SIGNAL_UnresolvedExpression)
			end
			return Constant(cs, "Core.RDX.StructuredType", left.longName, "Resource")
		elseif left.type == "CArrayOfType" then
			if not left.isCompiled then
				throw(SIGNAL_UnresolvedExpression)
			end
			return Constant(cs, "Core.RDX.ArrayOfType", left.longName, "Resource")
		elseif left.type == "CStaticDelegateType" then
			if not left.isCompiled then
				throw(SIGNAL_UnresolvedExpression)
			end
			return Constant(cs, "Core.RDX.DelegateType", left.longName, "Resource")
		else
			left = AdjustValueCount(cs, left, node, 1)
			if left.vTypes[1].longName == "Core.nullreference" then
				return cs.gst["Core.Object"]
			end
			return left.vTypes[1]
		end
	elseif node.type == "Ternary" then
		return CompileTernary(cs, node, scope)
	elseif node.type == "CreateTupleNode" then
		return CompileExpressionList(cs, node.expression, scope, true)
	else
		error("Unimplemented expression node type "..node.type)
	end

	if v.type == "CTypeReference" then
		if not v.isCompiled then
			throw(SIGNAL_UnresolvedExpression)
		end
		v = v.refType
	end

	return v
end

SingleExpressionToList = function(expr)
	assert(expr.vTypes)
	return FlattenMV({
		type = "CMultipleValues",
		accessModes = clonetable(expr.accessModes),
		expressions = { expr },
		vTypes = clonetable(expr.vTypes),
	})
end

CompileExpressionList = function(cs, node, scope, discharging)
	assert(discharging ~= nil)

	assert(scope, "AF13")
	local vTypes = { }
	local expressions = { }
	local accessModes = { }

	for _,expr in ipairs(node.expressions) do
		local exprNode = CompileExpression(cs, expr, scope, discharging)

		if exprNode.accessModes == nil then
			cerror(node, "ExpectedExpression", exprNode.type)
		end

		expressions[#expressions+1] = exprNode
		for idx,vt in ipairs(exprNode.vTypes) do
			vTypes[#vTypes+1] = vt
			accessModes[#accessModes+1] = exprNode.accessModes[idx]
		end
	end

	assert(vTypes)
	return FlattenMV({
		type = "CMultipleValues",
		accessModes = accessModes,
		expressions = expressions,
		vTypes = vTypes,
	})
end


Scope = function(owner)
	return {
		type = "CScope",
		symbols = { },
		owner = owner,
		InsertUnique = function(self, symbolName, value, incriminate)
			assert(value)
			local lookupSymbol
			if type(symbolName) == "string" then
				lookupSymbol = symbolName
			else
				incriminate = symbolName
				lookupSymbol = symbolName.string
			end

			if self.symbols[lookupSymbol] ~= nil then
				cerror(incriminate, "DuplicateSymbol", lookupSymbol)
			end
			self.symbols[lookupSymbol] = value
		end,
		Lookup = function(self, symbol, incriminate)
			local v = self.symbols[symbol]
			local invisibleSymbolWarning = false
			if v ~= nil then
				if v.invisibleSymbol == true then
					invisibleSymbolWarning = true
				else
					return v
				end
			end

			local fallThrough = nil
			if self.owner ~= nil then
				fallThrough = self.owner:Lookup(symbol, incriminate)
			end

			if fallThrough ~= nil and invisibleSymbolWarning then
				cwarning(incriminate, "InvisibleSymbol", symbol, symbol)
			end

			return fallThrough
		end
	}
end

-- Imported namespaces are a shallow check
local function ImportedNamespace(cs, pathNode)
	return {
		type = "CImportedNamespace",
		Lookup = function(self, symbol, incriminate)
			if self.namespace == nil then
				local ns = cs.globalNamespace
				for _,path in ipairs(pathNode) do
					ns = ns.symbols[path.string]
					if ns == nil then
						cerror(incriminate, "CouldNotResolveNamespace", path.string)
					end
				end
				pathNode = nil
				self.namespace = ns
			end

			return self.namespace.symbols[symbol]
		end,
	}
end

-- InstanciatedNamespaces are namespaces that are referenced by an absolute namespace path
local function InstanciatedNamespace(owner, ns, boundType)
	local insn = Scope(owner)
	insn.type = "CInstanciatedNamespace"
	insn.imports = { }
	insn.namespace = ns
	insn.boundType = boundType

	assert(owner == nil or owner.type ~= "CNamespace", "AF14")

	insn.Lookup = function(self, symbol, incriminate)
		local v = self.namespace.symbols[symbol]
		if v ~= nil then
			return v
		end

		for _,i in ipairs(self.imports) do
			v = i:Lookup(symbol, incriminate)
			if v ~= nil then
				return v
			end
		end
		if self.owner ~= nil then
			return self.owner:Lookup(symbol, incriminate)
		end
		return nil
	end

	return insn
end

local function Namespace(owner, name, prefix)
	local ns = Scope(owner)
	ns.type = "CNamespace"
	ns.name = name
	ns.prefix = prefix

	return ns
end


local function DelegateType(cs, namespace, scope, node, definedBy)
	local parameters = { }
	local returnTypes = { }
	local isStatic

	if node.accessDescriptor.static then
		isStatic = true
	end

	if node.parameters ~= nil then
		for _,parameterNode in ipairs(node.parameters.parameters) do
			local parameter = {
				type = TypeReference(cs, parameterNode.type, scope),
				isConst = (parameterNode.const ~= nil),
				isNotNull = (parameterNode.notnull ~= nil),
			}
			
			parameters[#parameters+1] = parameter
		end
	end

	for _,typeNode in ipairs(node.returnType.types) do
		returnTypes[#returnTypes+1] = TypeReference(cs, typeNode, scope)
	end

	local dt = {
			name = node.name.string,
			parameters = ParameterList(cs, parameters),
			returnTypes = TypeTuple(cs, returnTypes),
			namespace = namespace,

			uncompiledNode = node,
		}

	if isStatic then
		dt.type = "CStaticDelegateType"
		dt.isStatic = true
		dt.actualParameterList = dt.parameters
	else
		dt.type = "CBoundDelegateType"
		dt.isStatic = false
		dt.methodMarshals = { }

		local actualParams = { true }

		actualParams[1] = {
			type = TypeReference(cs, dt),
			name = { type = "Name", string = "this" },
			isConst = true,
			isNotNull = true,
		}

		for idx,p in ipairs(dt.parameters.parameters) do
			actualParams[idx+1] = p
		end

		local actualParameterList = ParameterList(cs, actualParams)
		dt.actualParameterList = actualParameterList
	end

	return dt
end

local function CompileDelegateType(cs, dt)
	if not dt.parameters.isCompiled then
		return false
	end
	if not dt.returnTypes.isCompiled then
		return false
	end

	local refName = dt.returnTypes.longName..dt.parameters.longName
	local longName

	if dt.isStatic then
		longName = "#DS-"..refName
	else
		longName = "#DB-"..refName
		dt.invokeName = longName.."/invoke"
	end

	dt.prettyName = "delegate "..dt.returnTypes.prettyName
	if not dt.isStatic then
		dt.prettyName = "bound "..dt.prettyName
	end
	dt.prettyName = dt.prettyName..dt.parameters.prettyName
	dt.longName = longName
	dt.isCompiled = true

	cs.gst[dt.longName] = dt

	return true
end


local function MarshalForBoundDelegate(cs, bdt, method)
	local mName = method.longName
	if bdt.methodMarshals[mName] then
		return bdt.methodMarshals[mName]
	end

	-- Doesn't exist
	local marshal = {
		type = "CBoundDelegateMarshal",
		owner = bdt,
		longName = bdt.longName.."/glue/"..method.longName,
		invokeName = bdt.longName.."/glueInvoke/"..method.longName,
		returnTypes = bdt.returnTypes,
		bdt = bdt,
		method = method,
	}

	if method.type == "CMethod" and not method.isStatic then
		marshal.thisType = method.definedByType
	elseif method.type == "CStaticDelegateType" then
		marshal.thisType = method
	else
		assert(method.type == "CMethod" and method.isStatic)
	end

	-- Generate a new parameter list with the marshal as "this"
	local newParameters = { }
	for idx,param in ipairs(bdt.actualParameterList.parameters) do
		newParameters[idx] = param
	end

	local tRef = TypeReference(cs, marshal)
	CompileTypeReference(cs, tRef)

	newParameters[1] = {
		type = tRef,
		name = { type = "Name", string = "this" },
		isConst = true,
		isNotNull = true,
	}
	marshal.actualParameterList = ParameterList(cs, newParameters)
	CompileParameterList(cs, marshal.actualParameterList)
	assert(marshal.actualParameterList.isCompiled)

	bdt.methodMarshals[mName] = marshal
	cs.gst[marshal.longName] = marshal

	local instructions = { }
	local numInstructions = 0
	local addInstr = function(instr)
		numInstructions = numInstructions + 1
		instructions[numInstructions] = instr
	end

	-- NOTE: "method" may actually be a delegate
	local method = marshal.method
	local isStaticDelegate = (method.type == "CStaticDelegateType")

	local refReturnValueStorage = { }	-- [returnValueIndex] = localIndex
	local firstStorageLocal = #marshal.actualParameterList.parameters
	local numStoredLocals = 0
	local numReturnValues
	local delegateValueLocal

	-- Create storage space for reference structs
	for idx,ref in ipairs(method.returnTypes.typeReferences) do
		local rvType = ref.refType
		if VTypeIsRefStruct(rvType) then
			local storedLocal = firstStorageLocal + numStoredLocals
			addInstr( { op = "alloclocal", res1 = rvType.longName } )
			numStoredLocals = numStoredLocals + 1
			refReturnValueStorage[idx] = storedLocal
		end
	end

	if isStaticDelegate then
		delegateValueLocal = firstStorageLocal + numStoredLocals
		numStoredLocals = numStoredLocals + 1

		-- Load "this"
		addInstr( { op = "localref", int1 = 0 } )
		addInstr( { op = "load" } )
		-- Load the delegate
		addInstr( { op = "property", int1 = 0 } )
		addInstr( { op = "load" } )
		-- Store it in a local
		addInstr( { op = "createlocal", res1 = method.longName } )
	end

	-- Push return value space
	for idx,ref in ipairs(method.returnTypes.typeReferences) do
		if refReturnValueStorage[idx] then
			addInstr( { op = "localref", int1 = refReturnValueStorage[idx] } )
			addInstr( { op = "pinlocal" } )
		else
			addInstr( { op = "pushempty", res1 = ref.refType.longName } )
		end
	end

	if isStaticDelegate then
		-- Push local for delegate call
		addInstr( { op = "localref", int1 = delegateValueLocal } )
	end


	-- Push parameters
	local fetchIndex = 1
	for idx,param in ipairs(method.actualParameterList.parameters) do
		if idx == method.thisParameterOffset then
			-- Load "this" value
			addInstr( { op = "localref", int1 = 0 } )
			addInstr( { op = "load" } )
			addInstr( { op = "property", int1 = 0 } )
			if not VTypeIsRefStruct(method.definedByType) then
				addInstr( { op = "load" } )
			end
		else
			addInstr( { op = "localref", int1 = fetchIndex } )
			addInstr( { op = "load" } )
			fetchIndex = fetchIndex + 1
		end
	end

	if isStaticDelegate then
		addInstr( { op = "calldelegate", res1 = method.longName } )
	elseif method.vftIndex then
		addInstr( { op = "callvirtual", res1 = method.longName } )
	else
		addInstr( { op = "call", res1 = method.longName } )
	end
	addInstr( { op = "return", int1 = #method.returnTypes.typeReferences } )

	for i=1,numStoredLocals do
		addInstr( { op = "removelocal" } )
	end

	marshal.instructions = instructions
	marshal.isCompiled = true

	return marshal
end

setglobal("ArrayOfTypeCode", function(numDimensions, isConst)
	return tostring(numDimensions).."/"..tostring(isConst ~= nil and isConst ~= false)
end )

local function StructuredType(cs, namespace, scope, attribTags, node)
	local isTemplate = (node.templateParameters ~= nil)

	local st = {
			type = "CStructuredType",
			isTemplate = isTemplate,
			isCompiled = false,
			namespace = namespace,		-- Namespace containing members of the type, named after the type
			scope = scope,			-- Scope external to the type, for resolving dependencies
			methods = { },
			methodGroups = { },
			virtualMethods = { },
			properties = { },
			arraysOf = { },
			enumerants = { },
			interfaces = { },
			implementedInterfaces = { },
			initializers = { },
			initializersSet = { },
			name = node.name.string,
			attribTags = attribTags,
			--baseClass = nil,

			uncompiledNode = node,
		}

	st.internalScope = InstanciatedNamespace(scope, namespace, st)

	if isTemplate then
		st.templateInstances = { }
	end

	return st
end

ConstructTypeTemplate = function(cs, st, templateParameters)
	local tParameters = ":<"
	local prettyParameters = ":<"

	for idx,tp in ipairs(templateParameters) do
		if idx ~= 1 then
			tParameters = tParameters..","
			prettyParameters = prettyParameters..","
		end
		tParameters = tParameters..tp.longName
		prettyParameters = prettyParameters..tp.prettyName
	end
	tParameters = tParameters..">"
	prettyParameters = prettyParameters..">"

	local longName = "#Tmpl."..st.longName..tParameters
	local prettyName = st.prettyName..prettyParameters

	local constructedType = st.templateInstances[tParameters]

	if constructedType then
		return constructedType
	end
	
	if instancedTemplateLimit == 0 then
		cerror(st.uncompiledNode, "TooManyTemplates")
	end
	instancedTemplateLimit = instancedTemplateLimit - 1

	local namespace = Namespace(st.namespace.owner, st.name..tParameters, "#Tmpl.")
	local templatedScope = Scope(st.scope)
	namespace.isFromTemplate = true

	-- This needs to be already true since we don't have the originating node for cerror
	assert(#templateParameters == #st.uncompiledNode.templateParameters.parameters)

	for idx,tp in ipairs(st.uncompiledNode.templateParameters.parameters) do
		templatedScope:InsertUnique(tp.string, templateParameters[idx], tp)
	end

	local templateST = {
			type = "CStructuredType",
			isTemplate = false,
			isCompiled = false,
			namespace = namespace,		-- Namespace containing members of the type, named after the type
			scope = templatedScope,	-- Scope external to the type, for resolving dependencies
			internalScope = InstanciatedNamespace(templatedScope, namespace),
			methods = { },
			methodGroups = { },
			virtualMethods = { },
			properties = { },
			arraysOf = { },
			name = st.name..tParameters,
			templateParameters = templateParameters,
			interfaces = { },
			implementedInterfaces = { },
			initializers = { },
			initializersSet = { },
			longName = longName,
			prettyName = prettyName,
			--baseClass = nil,

			uncompiledNode = st.uncompiledNode,
		}

	namespace.createdBy = templateST


	-- Add

	st.templateInstances[tParameters] = templateST
	cs.uncompiled[#cs.uncompiled+1] = templateST

	return templateST
end

local function ExtendClass(cs, st, baseST)
	assert(st.baseClass == nil)
	st.baseClass = baseST

	-- Try to alias existing tables
	st.aliasVFT = baseST.aliasVFT
	if st.aliasVFT == nil then
		st.aliasVFT = baseST
	end

	st.aliasProperties = baseST.aliasProperties
	if st.aliasProperties == nil then
		st.aliasProperties = baseST
	end

	st.aliasInterfaces = baseST.aliasInterfaces
	if st.aliasInterfaces == nil then
		st.aliasInterfaces = baseST
	end

	-- Copy everything except private members
	st.implementedInterfaces = clonetable(baseST.implementedInterfaces)
	st.interfaces = clonetable(baseST.interfaces)

	for _,m in ipairs(baseST.methods) do
		assert(m.visibility)
		if m.visibility ~= "private" then
			st.methods[#st.methods+1] = m
		end
	end

	for _,mg in ipairs(baseST.methodGroups) do
		assert(mg.visibility)
		if mg.visibility ~= "private" then
			local clonedGroup = CloneMethodGroup(cs, mg)
			st.methodGroups[#st.methodGroups+1] = clonedGroup
			st.namespace:InsertUnique(mg.name, clonedGroup)
		end
	end

	for _,p in ipairs(baseST.properties) do
		assert(p.visibility)
		if p.visibility == "private" then
			st.properties[#st.properties+1] = PrivatizedProperty(cs, p)
			if p.definedByType == baseST then
				st.aliasProperties = nil	-- This triggered the change, can't recycle the table
			end
		else
			st.properties[#st.properties+1] = p
			st.namespace:InsertUnique(p.name, p)
		end
	end

	for k,v in pairs(baseST.namespace.symbols) do
		if v.type ~= "CProperty" and v.type ~= "CMethodGroup" and v.visibility ~= "private" then
			st.namespace.symbols[k] = v
		end
	end

	st.virtualMethods = clonetable(baseST.virtualMethods)
end

local function AddNamespace(cs, namespace, ins, declarations)
	assert(ins.type == "CInstanciatedNamespace")
	for _,decl in ipairs(declarations) do
		if decl.type == "Namespace" then
			if namespace.symbols[decl.name.string] == nil then
				namespace:InsertUnique(decl.name, Namespace(namespace, decl.name.string))
			end
			local newNS = namespace.symbols[decl.name.string]

			local newINS = InstanciatedNamespace(ins, newNS)
			AddNamespace(cs, newNS, newINS, decl.members.declarations)
		elseif decl.type == "Using" then
			ins.imports[#ins.imports+1] = ImportedNamespace(cs, decl.namespacePath)
		elseif decl.type == "MemberDecl" and structuredTypeDeclTypes[decl.declType.string] then
			local uncompiledType = StructuredType(cs, Namespace(namespace, decl.name.string), ins, decl.attribTags, decl)
			namespace:InsertUnique(decl.name, uncompiledType)
			uncompiledType.namespace.createdBy = uncompiledType

			cs.uncompiled[#cs.uncompiled+1] = uncompiledType
		elseif decl.type == "MemberDecl" and decl.declType.string == "delegate" then
			local uncompiledType = DelegateType(cs, namespace, ins, decl, nil)
			namespace:InsertUnique(decl.name, uncompiledType)
			cs.uncompiled[#cs.uncompiled+1] = uncompiledType
		else
			cerror(decl, "BadDeclInNamespace", decl.type, decl.declType.string)
		end
	end
end

local function MergeParsedFile(cs, pf)
	AddNamespace(cs, cs.globalNamespace, InstanciatedNamespace(nil, cs.globalNamespace), pf.declarations)
end


local function ArrayOfType(cs, st, dimensions, isConst, incriminateNode)
	local aot = {
			type = "CArrayOfType",
			containedType = st,
			dimensions = dimensions,
			isConst = isConst,
			isCompiled = false,
			incriminateNode = incriminateNode,
			arraysOf = { },
		}

	cs.uncompiled[#cs.uncompiled+1] = aot
	return aot
end

local function CompileArrayOfType(cs, aot)
	if not aot.containedType.isCompiled then
		return false
	end

	if aot.containedType.longName == varyingType then
		cerror(aot.incriminateNode, "CreatedArrayOfVarying")
	end

	local longName = "#"..aot.containedType.longName.."["
	if aot.isConst then
		longName = longName.."C"
	end
	for i=2,aot.dimensions do
		longName = longName..","
	end
	longName = longName.."]"

	-- Unwind the AOT to determine the pretty name
	local prettyName = ""
	local unwind = aot
	while unwind.type == "CArrayOfType" do
		if unwind.isConst then
			prettyName = prettyName.." const"
		end
		prettyName = prettyName.."["
		for i=2,unwind.dimensions do
			prettyName = prettyName..","
		end
		prettyName = prettyName.."]"
		unwind = unwind.containedType
	end
	prettyName = unwind.prettyName..prettyName

	aot.prettyName = prettyName
	aot.longName = longName
	aot.isCompiled = true

	cs.gst[longName] = aot

	return true
end

TypeReference = function(cs, node, scope, allowDimensions)
	assert(node)
	local tr = { type = "CTypeReference",
			isCompiled = false,
			node = node,
			scope = scope,
			allowDimensions = allowDimensions
		}
	cs.uncompiled[#cs.uncompiled+1] = tr

	assert(scope == nil or scope.type ~= "CNamespace", "Don't use namespaces as lookup scopes!  Use InstanciatedNamespace")

	return tr
end

ExpressionToType = function(cs, typeNode, scope, allowArrays)
	local expr

	assert(typeNode)
	local succeeded = true
	local succeeded, exception = pcall(function()
		expr = CompileExpression(cs, typeNode, scope, true)
	end )

	if succeeded == false then
		if exception == SIGNAL_UnresolvedExpression then
			return nil
		else
			error(exception)
		end
	end

	if expr.type == "CArrayOfType" then
		if allowArrays then
			return expr
		end
		cerror(typeNode, "ArraysNotAllowed")
	end

	if expr.type ~= "CStructuredType" and expr.type ~= "CStaticDelegateType" and expr.type ~= "CBoundDelegateType" then
		cerror(typeNode, "ExpectedTypeReference", PrettyInternalClasses[expr.type])
		return false
	end

	if expr.isTemplate then
		cerror(typeNode, "NewTemplate")
	end

	return expr
end

local function CompileArrayTypeReference(cs, node, scope, specifyDimensions)
	local subType
	local subTypeNode = node.subType
	local specifiedDimensions

	if subTypeNode.type == "ArrayOfType" then
		subType = CompileArrayTypeReference(cs, subTypeNode, scope, false)
	else
		assert(subTypeNode.type == "Type")
		subType = ExpressionToType(cs, subTypeNode.baseType, scope, true)
	end

	if not subType then
		return nil	-- Unresolved dependency
	end

	-- See if this is cached
	local dimensions = node.dimensions

	if dimensions == nil then
		if not specifyDimensions then
			cerror(node, "UnexpectedDimensions")
		end

		local arrayIndexType = cs.gst[arrayIndexType]
		if arrayIndexType == nil then
			throw(SIGNAL_UnresolvedExpression)
		end

		local convertAccessModes = { }
		local convertVTypes = { }

		specifiedDimensions = CompileExpressionList(cs, node.specifiedDimensions, scope, true)
		if #specifiedDimensions.vTypes == 0 then
			cerror(node, "ExpectedDimensions")
		end

		for idx in ipairs(specifiedDimensions.vTypes) do
			convertAccessModes[idx] = "R"
			convertVTypes[idx] = arrayIndexType
		end

		specifiedDimensions = ConvertExpression(cs, node, specifiedDimensions, convertVTypes, convertAccessModes, false)
		dimensions = #specifiedDimensions.vTypes
	else
		--if specifyDimensions then
		--	cerror(node, "ExpectedDimensions")
		--end
	end

	assert(dimensions, "Need to parse out dimensions")
	
	local arrayVariationCode = ArrayOfTypeCode(dimensions, node.isConst)
	local aot = subType.arraysOf[arrayVariationCode]

	if aot == nil then
		assert(subType)
		aot = ArrayOfType(cs, subType, dimensions, node.isConst, node)
		subType.arraysOf[arrayVariationCode] = aot

		if not cs.compilingTypes then
			-- Declared inside a function
			CompileTypeShells(cs)
		end

		CompileArrayOfType(cs, aot)
	end
	return aot, specifiedDimensions
end

CompileTypeReference = function(cs, tr)
	if tr.node.type == "ArrayOfType" then
		local aot, specifiedDimensions = CompileArrayTypeReference(cs, tr.node, tr.scope, tr.allowDimensions)

		if aot == nil then
			return false	-- Couldn't resolve
		end
		tr.isCompiled = true
		tr.refType = aot
		tr.node = nil
		tr.specifiedDimensions = specifiedDimensions
		return true
	end

	if tr.node.type == "CStructuredType" or tr.node.type == "CBoundDelegateType" or tr.node.type == "CBoundDelegateMarshal" then
		-- Preresolved type reference
		tr.isCompiled = true
		tr.refType = tr.node
		tr.node = nil
		return true
	end

	assert(tr.node.type == "Type")

	-- Arrays are allowed here because the expression could be aliased from a typedef that references an array
	local expr = ExpressionToType(cs, tr.node.baseType, tr.scope, true)

	if not expr then
		return false
	end

	tr.node = nil		-- Don't need the node any more
	tr.isCompiled = true
	tr.refType = expr

	return true
end

local function MethodGroup(cs, name, visibility, isStatic, isIntercept, isArrayIntercept, incriminate)
	local mg = {
			type = "CMethodGroup",
			name = name,
			isCompiled = false,
			isStatic = isStatic,
			isIntercept = isIntercept,
			isArrayIntercept = isArrayIntercept,
			overloads = { },
			visibility = visibility,
			incriminate = incriminate
		}
	cs.uncompiled[#cs.uncompiled+1] = mg
	return mg
end

CloneMethodGroup = function(cs, mg)
	local cloned = clonetable(mg)
	cloned.overloads = clonetable(mg.overloads)
	cloned.isCompiled = false

	cs.uncompiled[#cs.uncompiled+1] = cloned
	return cloned
end


local function CompileMethodGroup(cs, mg)
	-- Make sure every overload is compiled
	for _,overload in ipairs(mg.overloads) do
		if not overload.isCompiled then
			return false
		end
	end
	mg.isCompiled = true

	return true
end


local function CompileTypeTuple(cs, tt)
	local longName = "("
	for _,tr in ipairs(tt.typeReferences) do
		if not tr.isCompiled or tr.refType.longName == nil then
			return false
		end
		if longName ~= "(" then
			longName = longName..","
		end
		longName = longName..tr.refType.longName
	end
	longName = longName..")"

	tt.prettyName = longName
	tt.longName = "#TT-"..longName
	tt.isCompiled = true

	cs.gst[tt.longName] = tt

	return true
end


TypeTuple = function(cs, typeReferences)
	local tt = {
			type = "CTypeTuple",
			isCompiled = false,
			typeReferences = typeReferences,
		}
	cs.uncompiled[#cs.uncompiled+1] = tt
	return tt
end




CompileParameterList = function(cs, pl)
	local longName = "("
	for _,param in ipairs(pl.parameters) do
		if not param.type.isCompiled or param.type.refType.longName == nil then
			return false
		end
		if longName ~= "(" then
			longName = longName..","
		end
		if param.isNotNull then
			longName = longName.."notnull "
		elseif param.isConst then
			longName = longName.."const "
		end

		longName = longName..param.type.refType.longName
	end
	longName = longName..")"

	pl.prettyName = longName
	pl.longName = "#PL-"..longName
	pl.isCompiled = true

	cs.gst[pl.longName] = pl

	return true
end


ParameterList = function(cs, parameters)
	local pl = {
			type = "CParameterList",
			isCompiled = false,
			parameters = parameters,
		}
	cs.uncompiled[#cs.uncompiled+1] = pl
	return pl
end


local function CompileMethod(cs, m)
	local longName
	if not m.definedByType.isCompiled then
		return false
	end
	longName = m.definedByType.longName.."/"..m.grouping.."/"..m.name.string

	if not m.returnTypes.isCompiled then
		return false
	end
	if not m.parameterList.isCompiled then
		return false
	end

	m.signature = m.name.string

	if m.declType.type == "coerce" or m.declType.type == "promote" then
		m.isLossless = (m.declType.type == "promote")
		m.signature = m.signature..m.returnTypes.prettyName
		longName = longName..m.returnTypes.prettyName
	else
		m.signature = m.signature..m.parameterList.prettyName
		longName = longName..m.parameterList.prettyName
	end

	m.returnSignature = m.name.string..m.returnTypes.prettyName

	m.longName = longName
	m.isCompiled = true

	-- Find out what the actual parameters are, including self
	-- These aren't needed for the type resolution phase so it's not important that they be compiled when this finishes
	local actualParameterList

	if m.isStatic then
		actualParameterList = m.parameterList
	else
		local thisParameter = TypeReference(cs, m.definedByType)
		CompileTypeReference(cs, thisParameter)
		assert(thisParameter.isCompiled)

		local thisParameterIndex = 1

		local actualParameters = { }

		if m.isIntercept and #m.parameterList.parameters == 1 then
			thisParameterIndex = 2
		end
		if m.isArrayIntercept then
			thisParameterIndex = 2
		end

		for idx,param in ipairs(m.parameterList.parameters) do
			local insertIdx = idx
			if idx >= thisParameterIndex then
				insertIdx = insertIdx + 1
			end

			actualParameters[insertIdx] = param
		end

		local thisPLParam = {
			type = thisParameter,
			name = { type = "Name", string = "this" },
		}

		if VTypeIsRefStruct(thisParameter.refType) then
			thisPLParam.isConst = m.isConst
		elseif VTypeIsObjectReference(thisParameter.refType) then
			thisPLParam.isNotNull = true
			thisPLParam.isConst = true
		end

		actualParameters[thisParameterIndex] = thisPLParam

		m.thisParameterIndex = thisParameterIndex
		m.thisParameterOffset = thisParameterIndex

		actualParameterList = ParameterList(cs, actualParameters)
		CompileParameterList(cs, actualParameterList)
		assert(actualParameterList.isCompiled)
	end
	
	assert(actualParameterList)
	m.actualParameterList = actualParameterList
	
	for _,p in ipairs(actualParameterList) do
		assert(p.type.refType.longName)
		if p.type.refType.longName == varyingType and not m.isNative then
			cerror(m.name, "VaryingParameterInNonNative")
		end
	end
	for _,t in ipairs(m.returnTypes) do
		if t.refType.longName == varyingType and not m.isNative then
			cerror(m.name, "VaryingReturnType")
		end
	end

	cs.gst[longName] = m

	return true
end

local function Method(cs, name, grouping, definedByType, returnTypes, parameterList, visibility,
	declType, codeBlock, isStatic, isVirtual, isAbstract, isNative, isIntercept, isArrayIntercept, isBranching, isFinal, isConst)
	
	assert(parameterList.type == "CParameterList")
	assert(returnTypes.type == nil)

	local method = {
			type = "CMethod",
			name = name,
			isBranching = isBranching,
			isVirtual = isVirtual,
			isAbstract = isAbstract,
			isNative = isNative,
			isStatic = isStatic,
			isIntercept = isIntercept,
			isArrayIntercept = isArrayIntercept,
			isFinal = isFinal,
			isConst = isConst,
			declType = declType,
			grouping = grouping,
			definedByType = definedByType,
			isCompiled = false,
			parameterList = parameterList,
			returnTypes = TypeTuple(cs, returnTypes),
			codeBlock = codeBlock,
			visibility = visibility,
			numPrivateTypes = 0,
		}
	cs.uncompiled[#cs.uncompiled+1] = method

	if codeBlock then
		cs.uncompiledCode[#cs.uncompiledCode+1] = method
	end

	return method
end


PrivatizedProperty = function(cs, p)
	if p.alias then
		p = p.alias
	end
	local p = {
			type = "CProperty",
			typeOf = p.typeOf,
			isCompiled = true,
			definedByType = p.definedByType,
			visibility = p.visibility,
			name = p.definedByType.longName.."."..p.name,
			nameNode = p.nameNode,
			alias = p,
		}
	return p
end

local function StaticInstance(cs, definedByType, typeOf, visibility, name, isConst, incriminate)
	local si = {
			type = "CStaticInstance",
			isCompiled = false,
			definedByType = definedByType,
			typeOf = typeOf,
			visibility = visibility,
			name = (name and name.string),
			nameNode = name,
			incriminate = incriminate,
			isConst = isConst,
		}
	cs.uncompiled[#cs.uncompiled+1] = si
	return si
end

local function CompileStaticInstance(cs, si)
	if not si.typeOf.isCompiled or not si.definedByType.isCompiled then
		return false
	end
	local vType = si.typeOf.refType

	if not vType.isCompiled or (vType.declType == "CStructuredType" and not vType.finalizer.isCompiled) then
		return false
	end

	si.longName = si.definedByType.longName.."."..si.name


	if VTypeIsObjectReference(vType) then
		si.accessModes = { "R" }
	else
		si.accessModes = { (si.isConst and "CP" or "P") }
	end

	if vType.type == "CStructuredType" and vType.declType == "interface" then
		cerror(si.incriminate, "InterfaceResource")
	end

	si.incriminate = nil
	si.vTypes = { si.typeOf.refType }
	si.isCompiled = true

	cs.gst[si.longName] = si

	return true
end

local function Initializer(cs, node, ownerType, ownerMemberName)
	local i = {
			type = "CInitializer",
			isCompiled = false,
			ownerType = ownerType,
			ownerMemberName = ownerMemberName,
			--ownerMember = ???,
			uncompiledNode = node,
		}
	cs.uncompiled[#cs.uncompiled+1] = i
	return i
end

local function DefaultInstance(cs, vType, diValue, name, dimensions, isAnonymous)
	assert(type(name) == "string")

	local defaultInstance = {
		type = "CDefaultInstance",
		dimensions = dimensions,
		typeOf = vType.longName,
		longName = name,
		value = diValue,
		isAnonymous = isAnonymous,
	}

	cs.defaultInstances[#cs.defaultInstances+1] = defaultInstance
	cs.gst[name] = defaultInstance

	return defaultInstance
end

local function GetStructuredTypeDefault(cs, st)
	local finalValue

	if #st.initializers == 0 then
		return nil
	end

	st.defaultDependencySet = { }

	finalValue = "{\n"

	for idx,init in ipairs(st.initializers) do
		if idx ~= 1 then
			finalValue = finalValue..",\n"
		end

		local pName = init.ownerMember.name
		if RDXT_RESERVED_SYMBOLS[pName] then
			pName = "'"..pName.."'"
		end

		finalValue = finalValue.."\t"..pName.." : "..init.defaultValue

		for k in pairs(init.defaultDependencySet) do
			st.defaultDependencySet[k] = true
		end
	end
	finalValue = finalValue.."\n}"

	return DefaultInstance(cs, st, finalValue, st.longName.."/default", nil, false)
end

-- Returns the packed initialization value
-- vType: vType of the value being initialized
-- expr: Expression to attempt to evaluate
-- forceRef: If true, the expression must be an instance and not a reference to another resource
-- indentLevel: Indentation level
-- base: Prefix of any generated names
-- dependencySet: A set of any dependency names needed by this initializer
-- dimensions: Output array of dimensions
-- incriminate: Node to incriminate if this errors
local function GetInitializationValue(cs, vType, expr, forceRef, indentLevel, base, dependencySet, dimensions, isAnonymous, incriminate)
	assert(incriminate)

	local finalValue

	local indentation, indentation2, lineBreak

	if false then
		indentation = ""
		indentation2 = ""
		lineBreak = " "
	else
		indentation = ""
		for i=1,indentLevel do
			indentation = indentation.."\t"
		end
		indentation2 = indentation.."\t"
		lineBreak = "\n"
	end

	-- Initialization rules:
	--    Enum:        To a CEnumerant
	--    Interface:   To a static instance
	--    Class/array: Static instance or constructor
	--    Struct:      To a constructor unless in parseableTypes, in which case CConstant

	local allowConstructor

	if vType.type == "CStructuredType" and parseableTypes[vType.longName] then
		if expr.type ~= "CConstant" or expr.signal ~= "Value" then
			cerror(incriminate, "NonConstantInitializer")
		end

		if expr.vTypes[1].longName == stringType and not forceRef then
			cerror(incriminate, "StringResource")
		end

		if vType.longName == "Core.string" then
			finalValue = "'"..escapestring(expr.value).."'"
		else
			finalValue = expr.value
		end
	elseif VTypeIsObjectReference(vType) then
		if expr.type == "CConvertExpression" then
			expr = expr.expression
		end

		if CastMatchability(expr.vTypes[1], vType) > matchLevels.DIRECT then
			cerror(incriminate, "InitializerNotCompatible")
		end

		allowConstructor = (vType.type == "CArrayOfType" or (vType.type == "CStructuredType" and vType.declType ~= "interface"))

		if expr.type == "CStaticInstance" then
			if not forceRef then
				cerror(incriminate, "ResourceReferencesResource")
			end
			dependencySet[expr.longName] = true
			finalValue = "res "..encodeRes(expr.longName)
		elseif expr.type == "CConstant" then
			if expr.vTypes[1].longName == "Core.nullreference" then
				finalValue = "null"
			elseif expr.signal == "Resource" then
				finalValue = "res "..encodeRes(expr.value)
			else
				finalValue = expr.value
			end
		elseif expr.type == "CMethodDelegation" then
			finalValue = "res "..encodeRes(expr.method.longName)
		else
			if expr.type ~= "CInitializeProperties" and expr.type ~= "CInitializeArray" then
				cerror(incriminate, "BadPropertyInitializerType", PrettyInternalClasses[expr.type])
			end
		end
	elseif vType.declType == "enum" then
		if expr.type ~= "CConstant" or expr.signal ~= "Enum" or not expr.enumName then
			cerror(incriminate, "BadEnumeratorValueType", PrettyInternalClasses[expr.type])
		end

		finalValue = "'"..escapestring(expr.enumName).."'"
	elseif vType.declType == "struct" then
		allowConstructor = true

		--if expr.type == "CStaticInstance" then
		--	cerror(incriminate, "StaticInstanceInitializer")
		--end
	else
		assert(false)
	end

	if finalValue == nil and allowConstructor then
		if expr.type ~= "CInitializeProperties" and expr.type ~= "CInitializeArray" then
			cerror(incriminate, "ExpectedConstructorInitializer", PrettyInternalClasses[expr.type])
		end

		if forceRef then
			local subDimensions = { }
			local diValue = GetInitializationValue(cs, vType, expr, false, 0, base, dependencySet, subDimensions, isAnonymous, incriminate)

			local defaultInstance = DefaultInstance(cs, vType, diValue, base, subDimensions, isAnonymous)

			dependencySet[defaultInstance.longName] = true

			return "res "..encodeRes(defaultInstance.longName)
		end

		if expr.type == "CInitializeArray" then
			for i,v in ipairs(expr.dimensions) do
				dimensions[i] = v
			end
		end

		finalValue = "{"..lineBreak

		for idx,i in ipairs(expr.initializers) do
			local dest, src

			if idx ~= 1 then
				finalValue = finalValue..","..lineBreak
			end

			local requireRef
			local refType
			local newIncriminate
			local valueExpr
			local repitchName

			if vType.type == "CArrayOfType" then
				refType = vType.containedType
				newIncriminate = expr.incriminateNode
				valueExpr = i
				assert(refType)

				repitchName = tostring(idx-1)

			elseif vType.type == "CStructuredType" then
				if i.dest.type ~= "CObjectProperty" then
					cerror(incriminate, "OnlyPropertyInitializersAllowed")
				end

				dest = i.dest

				refType = dest.property.typeOf.refType
				newIncriminate = i.incriminateNode
				valueExpr = i.src
				repitchName = dest.property.name
			else
				assert(false, "Unimplemented")
			end

			if VTypeIsObjectReference(refType) then
				requireRef = true
			end

			local propertyV = GetInitializationValue(cs, refType, valueExpr, requireRef, indentLevel+1, base.."."..repitchName, dependencySet, dimensions, isAnonymous, newIncriminate)

			if vType.type == "CArrayOfType" then
				finalValue = finalValue..indentation2..propertyV
			elseif vType.type == "CStructuredType" then
				if i.dest.type ~= "CObjectProperty" then
					cerror(incriminate, "OnlyPropertyInitializersAllowed")
				end

				local dest = i.dest

				local pName = dest.property.name
				if RDXT_RESERVED_SYMBOLS[pName] then
					pName = "'"..pName.."'"
				end


				finalValue = finalValue..indentation2..pName.." : "..propertyV
			else
				assert(false, "Unimplemented")
			end
		end

		finalValue = finalValue..lineBreak..indentation.."}"
	end

	assert(finalValue ~= nil)

	return finalValue
end

local function CompileInitializer(cs, i)
	assert(i.uncompiledNode)

	local isAnonymous

	local expr
	local vType

	if not i.ownerType.isCompiled and not i.ownerType.finalizer.isCompiled then
		return nil
	end

	local ownerMember = i.ownerType.namespace.symbols[i.ownerMemberName.string]
	if not ownerMember then
		cerror(i.ownerMemberName, "UnresolvedMemberInitializer", i.ownerMemberName.string)
	end

	if ownerMember.type ~= "CProperty" and ownerMember.type ~= "CStaticInstance" then
		cerror(i.ownerMemberName, "UninitializableMember")
	end

	if not ownerMember.isCompiled or not ownerMember.typeOf.isCompiled then
		return nil
	end
	local vType = ownerMember.typeOf.refType

	if not vType.isCompiled then
		return nil
	end

	local succeeded = true
	local exprOriginalType
	local succeeded, exception = pcall(function()
		expr = CompileExpression(cs, i.uncompiledNode, i.ownerType.internalScope, true)
		expr = AdjustValueCount(cs, expr, i.uncompiledNode, 1)
		exprOriginalType = expr.vTypes[1]
		expr = ConvertExpression(cs, i.uncompiledNode, expr, { vType }, nil, false )
	end )

	if succeeded == false then
		if exception == SIGNAL_UnresolvedExpression then
			return false
		else
			error(exception)
		end
	end

	local requireRef
	local baseName
	if VTypeIsObjectReference(vType) then
		requireRef = true
	end

	local firstIndent = 1
	if ownerMember.type == "CStaticInstance" then
		firstIndent = 0
		requireRef = false
		ownerMember.initializer = i
		baseName = ownerMember.longName
		isAnonymous = ownerMember.isAnonymous

		-- Static instance types must be exact matches
		if vType ~= exprOriginalType then
			cerror(i.ownerMemberName, "ResourceInstanceIncompatible")
		end
	elseif ownerMember.type == "CProperty" then
		local init = i.ownerType.initializersSet[ownerMember.name]
		if init and init.ownerType == i.ownerType then
			cerror(i.uncompiledNode, "MemberAlreadyHasDefault", ownerMember.name)
		end
		i.ownerType.initializersSet[ownerMember.name] = i
		i.ownerType.initializers[#i.ownerType.initializers+1] = i
		baseName = i.ownerType.longName.."/default."..ownerMember.name
	end

	i.defaultDependencySet = { }
	i.defaultDimensions = { }
	i.defaultValue = GetInitializationValue(cs, vType, expr, requireRef, firstIndent, baseName, i.defaultDependencySet, i.defaultDimensions, isAnonymous, i.uncompiledNode)
	i.ownerMember = ownerMember
	i.isCompiled = true

	i.uncompiledNode = nil

	return true
end


local function Property(cs, definedByType, typeOf, visibility, isConst, mustBeConst, name)
	local p = {
			type = "CProperty",
			isCompiled = false,
			definedByType = definedByType,
			typeOf = typeOf,
			visibility = visibility,
			name = (name and name.string),
			nameNode = name,
			isConst = isConst,
			mustBeConst = mustBeConst,
		}
	cs.uncompiled[#cs.uncompiled+1] = p
	return p
end

local function CompileProperty(cs, p)
	if not p.typeOf.isCompiled or not p.definedByType.isCompiled then
		return false
	end
	if p.typeOf.refType.longName == varyingType then
		cerror(p.name, "VaryingProperty")
	end
	--p.longName = p.definedByType.longName.."/properties/"..p.name
	p.isCompiled = true
	return true
end

local function TypeFinalizer(cs, st)
	local tf = {
			type = "CTypeFinalizer",
			st = st,
			isCompiled = false,
		}

	cs.uncompiled[#cs.uncompiled+1] = tf
	return tf
end

local function CompileTypeFinalizer(cs, tf)
	local st = tf.st

	local methodsBySignature = { }
	local virtualOffsetsBySignature = { }
	local deletedMethods = { }

	-- Make sure all methods are compiled
	for _,method in ipairs(st.methods) do
		if not method.isCompiled then
			return false, method.nameNode
		end
	end

	-- Make sure all interfaces are compiled and finalized
	for _,i in ipairs(st.interfaces) do
		local ist = i.interfaceType
		if not ist.isCompiled or not ist.finalizer.isCompiled then
			return false, i.incriminateNode
		end
	end
	
	-- Go back through method signatures and handle overloads
	for _,method in ipairs(st.methods) do
		local methodSignature = method.signature

		local collision = methodsBySignature[methodSignature]
		local virtualIndex = #st.virtualMethods+1
		
		-- See if there's an existing method with this name
		if collision then
			if collision.definedByType == method.definedByType then
				cerror(method.name, "DuplicatedMethod")
			end

			if collision.isVirtual then
				if method.isStatic then
					cerror(method.name, "OverridedVirtualWithStatic")
				end

				if collision.returnSignature ~= method.returnSignature then
					cerror(method.name, "OverrideHasDifferentReturn")
				end

				if not method.isVirtual and not method.isFinal then
					cerror(method.name, "InvalidOverrideFlags")
				end

				method.vftIndex = collision.vftIndex
				method.isOverriding = true

				-- Replace all entries in the vtable
				for vidx,vmethod in ipairs(st.virtualMethods) do
					if vmethod == collision then
						st.virtualMethods[vidx] = method
						st.aliasVFT = nil
					end
				end

				virtualIndex = nil
			end

			deletedMethods[collision] = true
		end

		if method.isFinal and not method.isOverriding then
			cerror(method.name, "FinalMethodDoesNotOverride")
		end

		if method.definedByType == st then
			-- New virtual method
			if method.isVirtual and virtualIndex ~= nil then
				method.vftIndex = virtualIndex
				st.virtualMethods[virtualIndex] = method
				st.aliasVFT = nil
			end
		end

		if method.isVirtual then
			method.isCalledVirtual = true
		end

		methodsBySignature[methodSignature] = method
	end

	-- Rebuild method groups
	for _,mg in ipairs(st.methodGroups) do
		local rebuilt = { }
		for _,m in ipairs(mg.overloads) do
			if not deletedMethods[m] then
				rebuilt[#rebuilt+1] = m
			end
		end
		mg.overloads = rebuilt
	end

	-- Rebuild the main methods list
	local rebuilt = { }
	for _,m in ipairs(st.methods) do
		if not deletedMethods[m] then
			rebuilt[#rebuilt+1] = m
		end
	end
	st.methods = rebuilt

	for _,i in ipairs(st.interfaces) do
		if i.vftOffset == nil then
			i.vftOffset = #st.virtualMethods+1

			for _,m in ipairs(i.interfaceType.methods) do
				local localMethod = methodsBySignature[m.signature]
				if localMethod == nil then
					cerror(i.incriminateNode, "InterfaceMethodMissing", m.signature, i.interfaceType.longName)
				end
				if localMethod.returnTypes.longName ~= m.returnTypes.longName then
					cerror(i.incriminateNode, "InterfaceReturnTypeMismatch", m.signature)
				end
				st.virtualMethods[#st.virtualMethods+1] = localMethod
			end
			st.aliasInterfaces = nil	-- Can't recycle the interfaces table
			st.aliasVFT = nil
		end
	end

	for _,m in ipairs(st.virtualMethods) do
		if m.isAbstract then
			st.isAbstract = true
			st.abstractBlame = m
			break
		end
	end

	tf.isCompiled = true

	return true
end

local function InsertProperties(cs, t, propertyNode)
	local visibility = DefaultVisibility(propertyNode.accessDescriptor.visibility)
	local isStatic = (propertyNode.declType.type == "resource")
	local isConst = (propertyNode.accessDescriptor.const ~= nil)
	local mustBeConst = (propertyNode.accessDescriptor.mustbeconst ~= nil)

	if propertyNode.accessDescriptor.static then
		cerror(propertyNode, "StaticProperty")
	end

	if propertyNode.initializers and #propertyNode.initializers.expressions ~= #propertyNode.declList.declarations then
		cerror(propertyNode, "PropertyInitializerCountMismatch")
	end

	for idx,decl in ipairs(propertyNode.declList.declarations) do
		local typeRef = TypeReference(cs, decl.type, t.internalScope)
		local initializer = nil

		if propertyNode.initializers then
			initializer = Initializer(cs, propertyNode.initializers.expressions[idx], t, decl.name)
		end

		if isStatic then
			local si = StaticInstance(cs, t, typeRef, visibility, decl.name, isConst, propertyNode)
			si.hasInitializer = (propertyNode.initializers ~= nil)
			si.isAnonymous = (propertyNode.accessDescriptor.anonymous ~= nil)
			t.namespace:InsertUnique(decl.name, si)
		else
			local p = Property(cs, t, typeRef, visibility, isConst, mustBeConst, decl.name)

			t.properties[#t.properties+1] = p
			p.propertyIndex = #t.properties
			t.namespace:InsertUnique(decl.name, p)
		end
	end

	-- Don't alias properties any more
	t.aliasProperties = nil
end

local function InsertTypedef(cs, t, tdNode)
	local typeRef = TypeReference(cs, tdNode.specifiedType, t.internalScope)
	typeRef.visibility = DefaultVisibility(tdNode.accessDescriptor.visibility)
	t.namespace:InsertUnique(tdNode.name, typeRef)
end

local function InsertStructuredType(cs, t, decl)
	local uncompiledType = StructuredType(cs, Namespace(t.namespace, decl.name.string), t.internalScope, decl.attribTags, decl)
	t.namespace:InsertUnique(decl.name, uncompiledType)

	cs.uncompiled[#cs.uncompiled+1] = uncompiledType
end

local function InsertMethod(cs, t, methodNode)
	local methodName
	local methodNameNode
	local visibility = DefaultVisibility(methodNode.accessDescriptor.visibility)
	local isStatic = (methodNode.accessDescriptor.static ~= nil)
	local isIntercept = (methodNode.accessDescriptor.intercept ~= nil)
	local mg

	if methodNode.declType.type == "coerce" or methodNode.declType.type == "promote" then
		local n = VirtualParseNode("Name", methodNode)
		n.string = "#coerce"
		methodNameNode = n
	else
		methodNameNode = methodNode.name
	end

	methodName = methodNameNode.string

	local isArrayIntercept = (methodName == "__setindex")

	if t.namespace.symbols[methodName] ~= nil then
		mg = t.namespace.symbols[methodName]
		if mg.type ~= "CMethodGroup" then
			cerror(methodNode, "MethodCollidesWithNonMethod")
		end
		if mg.visibility ~= visibility then
			cerror(methodNode, "MethodVisibilityMismatch")
		end
		if mg.isStatic ~= isStatic then
			cerror(methodNode, "MethodStaticMismatch")
		end
		if mg.isIntercept ~= isIntercept then
			cerror(methodNode, "MethodInterceptMismatch")
		end
	else
		mg = MethodGroup(cs, methodNameNode.string, visibility, isStatic, isIntercept, isArrayIntercept, methodNameNode)
		t.namespace:InsertUnique(methodNameNode, mg)
		t.methodGroups[#t.methodGroups+1] = mg
	end
	
	-- Convert parameters
	local parameters = { }
	local returnTypes = { }

	if methodNode.declType.type == "coerce" or methodNode.declType.type == "promote" then
		if #methodNode.returnType.types ~= 1 then
			cerror(methodNode, "CoerceDoesNotReturnOneType")
		end
	end

	for _,typeNode in ipairs(methodNode.returnType.types) do
		returnTypes[#returnTypes+1] = TypeReference(cs, typeNode, t.internalScope)
	end

	if methodNode.parameters ~= nil then
		for _,parameterNode in ipairs(methodNode.parameters.parameters) do
			local param = { }
			parameters[#parameters+1] = param
			param.type = TypeReference(cs, parameterNode.type, t.internalScope)
			param.name = parameterNode.name
			param.isConst = (parameterNode.const ~= nil)
			param.isNotNull = (parameterNode.notnull ~= nil)
			if param.isNotNull then
				param.isConst = true
			end
		end
	end

	if isIntercept then
		if not ((#parameters == 1 and #returnTypes == 0) or (#parameters == 0 and #returnTypes == 1)) then
			cerror(methodNode, "InvalidInterceptFormat")
		end
	end

	local isStatic = methodNode.accessDescriptor.static ~= nil
	local isVirtual = methodNode.accessDescriptor.virtual ~= nil
	local isAbstract = methodNode.accessDescriptor.abstract ~= nil
	local isBranching = methodNode.accessDescriptor.branching ~= nil
	local isNative = methodNode.accessDescriptor.native ~= nil
	local isFinal = methodNode.accessDescriptor.final ~= nil
	local isConst = methodNode.accessDescriptor.const ~= nil

	if isAbstract and not isVirtual then
		cerror(methodNode, "NonVirtualAbstract")
	end

	if t.declType == "interface" then
		if isStatic then
			cerror(methodNode, "StaticMethodInInterface")
		end
		if isVirtual then
			cerror(methodNode, "VirtualMethodInInterface")
		end
		if isAbstract then
			cerror(methodNode, "AbstractMethodInInterface")
		end
		isAbstract = true
		isVirtual = true
	elseif t.declType == "struct" then
		if isVirtual then
			cerror(methodNode, "VirtualMethodInStructure")
		end
	end

	if isAbstract and methodNode.codeBlockCacheID then
		cerror(methodNode, "AbstractMethodHasCode")
	end
	if (not isAbstract) and (not isNative) and (not methodNode.codeBlockCacheID) then
		cerror(methodNode, "MethodMissingCode")
	end


	if isArrayIntercept and #returnTypes ~= 0 then
		cerror(methodNode, "SetIndexWithReturnValue")
	end

	local parameterList = ParameterList(cs, parameters)

	local method = Method(cs, methodNameNode, "methods", t, returnTypes, parameterList, visibility, methodNode.declType, methodNode.codeBlockCacheID,
				isStatic, isVirtual, isAbstract, isNative, isIntercept, isArrayIntercept, isBranching, isFinal, isConst)

	t.methods[#t.methods+1] = method
	mg.overloads[#mg.overloads+1] = method
end

local function InterfaceImplementation(cs, t, incriminate)
	assert(t)
	return {
		type = "CInterfaceImplementation",
		interfaceType = t,
		incriminateNode = incriminate,
		--vftOffset = ???
	}
end

-- Returns true if any progress is made on the type
local function CompileType(cs, t)
	local madeProgress = false

	t.visibility = DefaultVisibility(t.uncompiledNode.accessDescriptor.visibility)
	t.declType = t.uncompiledNode.declType.string
	t.byVal = (t.uncompiledNode.accessDescriptor.byVal ~= nil)
	t.mustBeRef = (t.uncompiledNode.accessDescriptor.mustBeRef ~= nil)
	t.isFinal = (t.uncompiledNode.accessDescriptor.final ~= nil)
	t.isAbstract = (t.uncompiledNode.accessDescriptor.abstract ~= nil)
	t.isLocalized = (t.uncompiledNode.accessDescriptor.localized ~= nil)
	
	local prefixes = { }

	local longName = t.uncompiledNode.name.string
	local ns = t.namespace.owner
	while ns ~= nil do
		if ns.name ~= nil then
			prefixes[#prefixes+1] = ns.prefix
			longName = ns.name.."."..longName
		end
		ns = ns.owner
	end
	
	local prettyName = longName
	
	-- Unwind prefixes
	while #prefixes > 0 do
		local pfidx = #prefixes
		longName = prefixes[pfidx]..longName
		prefixes[pfidx] = nil
	end

	if t.templateParameters then
		prettyName = prettyName..":<"
		longName = "#Tmpl."..longName..":<"
		for idx,tp in ipairs(t.templateParameters) do
			if idx ~= 1 then
				longName = longName..","
				prettyName = prettyName..","
			end
			longName = longName..tp.longName
			prettyName = prettyName..tp.prettyName
		end
		longName = longName..">"
		prettyName = prettyName..">"
		t.isFromTemplate = true
	end

	t.longName = longName
	t.prettyName = prettyName

	if not t.isTemplate then
		local parentClass = nil
		local interfaces = { }

		if t.declType == "class" and t.uncompiledNode.parent == nil and longName ~= "Core.Object" then
			parentClass = cs.gst["Core.Object"]
			if parentClass == nil then
				return false	-- Core.Object not defined yet
			end
		elseif t.uncompiledNode.parent then
			local parentClassRef = TypeReference(cs,  t.uncompiledNode.parent, t.scope, false)
			CompileTypeReference(cs, parentClassRef)
			
			if not parentClassRef.isCompiled then
				return false	-- Unresolved
			end

			parentClass = parentClassRef.refType
		end

		-- Check now to avoid partial compilation
		if t.uncompiledNode.interfaces then
			for _,iexpr in ipairs(t.uncompiledNode.interfaces.expressions) do
				local interfaceType = ExpressionToType(cs, iexpr, t.scope)
				if interfaceType == nil then
					return false
				end

				if not interfaceType.isCompiled then
					return false
				end

				interfaces[#interfaces+1] = InterfaceImplementation(cs, interfaceType, iexpr)
			end
		end

		if parentClass then
			if parentClass.type ~= "CStructuredType" then
				cerror(t.uncompiledNode, "ExtendedNonClass")
			end

			if not parentClass.isCompiled or not parentClass.finalizer.isCompiled then
				-- Can't extend until the type is fully defined with a property layout and vtable
				return false
			end

			-- Won't have a declType until this is compiled
			if parentClass.declType ~= "class" then
				cerror(t.uncompiledNode, "ExtendedNonClass")
			end

			if parentClass.isTemplate then
				cerror(t.uncompiledNode, "ExtendedTemplate")
			end

			if t.declType ~= "class" then
				cerror(t.uncompiledNode, "NonClassExtended")
			end

			if parentClass.isFinal then
				cerror(t.uncompiledNode, "ExtendedFinalClass")
			end

			t.parentType = parentClass

			-- Import everything
			ExtendClass(cs, t, parentClass)
		end

		-- Insert new interfaces
		local implementedInterfaces = { }
		for _,i in ipairs(interfaces) do
			if i.interfaceType.type ~= "CStructuredType" or i.interfaceType.declType ~= "interface" then
				cerror(t.uncompiledNode, "ImplementedNonInterface", i.interfaceType.longName)
			end

			if implementedInterfaces[i.interfaceType] then
				cerror(t.uncompiledNode, "DuplicateImplementations", i.interfaceType.name.string)
			end
			if not t.implementedInterfaces[i.interfaceType] then
				t.interfaces[#t.interfaces+1] = i
				t.implementedInterfaces[i.interfaceType] = true
			end
			implementedInterfaces[i.interfaceType] = true
		end

		-- Insert new members
		if t.uncompiledNode.typeMembers then
			for _,decl in ipairs(t.uncompiledNode.typeMembers.members) do
				if decl.type == "DefaultDeclList" then
					for _,ddecl in ipairs(decl.defaultDecls) do
						Initializer(cs, ddecl.expression, t, ddecl.fieldName)
					end
				elseif decl.type == "MemberDecl" then
					if decl.declType.type == "function" or decl.declType.type == "coerce" or decl.declType.type == "promote" then
						InsertMethod(cs, t, decl)
					elseif decl.declType.type == "property" or decl.declType.type == "resource" then
						InsertProperties(cs, t, decl)
					elseif decl.declType.type == "typedef" then
						InsertTypedef(cs, t, decl)
					elseif structuredTypeDeclTypes[decl.declType.type] then
						InsertStructuredType(cs, t, decl)
					elseif decl.declType.type == "delegate" then
						local uncompiledType = DelegateType(cs, t.namespace, t.internalScope, decl, t)
						t.namespace:InsertUnique(decl.name, uncompiledType)
						cs.uncompiled[#cs.uncompiled+1] = uncompiledType
					else
						cerror(decl, "UnsupportedDeclType", decl.declType.type)
					end
				else
					cerror(decl, "UnsupportedTypeMemberType", decl.declType.type)
				end
			end
		end

		if t.uncompiledNode.enumerants then
			local enumIndex = -1
			local zeroDefined = false

			local usedValues = { }
			local finalEnumerants = { }
			local finalInserts = { }

			for _,e in ipairs(t.uncompiledNode.enumerants.enumerants) do
				if e.initializer then
					local initializer
					local succeeded, exception = pcall(function()
						initializer  = CompileExpression(cs, e.initializer, t.internalScope, true)
					end )

					if succeeded == false then
						if exception == SIGNAL_UnresolvedExpression then
							return nil
						else
							error(exception)
						end
					end
					
					if initializer.type ~= "CConstant" or initializer.signal ~= "Value" or initializer.vTypes[1].longName ~= enumType then
						cerror(e.initializer, "EnumInitializerNotInteger")
					end
					enumIndex = tonumber(initializer.value)
				else
					enumIndex = enumIndex + 1
				end
				local constVal = Constant(cs, t, enumIndex, "Enum")
				constVal.enumName = e.name.string

				if usedValues[enumIndex] then
					cerror(e.name, "DuplicateEnumValue", tostring(enumIndex), e.name.string)
				end
				usedValues[enumIndex] = true

				if enumIndex == 0 then
					zeroDefined = true
				end

				finalEnumerants[#finalEnumerants+1] = {
					type = "CEnumerant",
					name = e.name.string,
					value = enumIndex,
					constVal = constVal,
					nameToken = e.name,
				}
			end
			
			-- Sort enumerants
			table.sort(finalEnumerants, function(a, b)
					return a.value < b.value
				end )

			t.enumerants = finalEnumerants

			for _,fe in ipairs(finalEnumerants) do
				t.namespace:InsertUnique(fe.name, fe.constVal, fe.nameToken)
			end

			if not zeroDefined then
				cerror(t.uncompiledNode, "MissingZeroEnumerant")
			end
		end

		t.uncompiledNode = nil	-- Don't need this any more
	end

	t.isCompiled = true
	t.finalizer = TypeFinalizer(cs, t)

	cs.gst[longName] = t

	return true
end

local function CheckCircularity(cs, branch, root, incriminate)
	if branch == root then
		cerror(incriminate, "VerifiedCircularDependency", root.prettyName)
	end
	if branch == nil then
		branch = root
		if branch.type ~= "CStructuredType" or (branch.declType ~= "struct" and branch.declType ~= "class") then
			return
		end
	end
	
	if branch.parentType ~= nil then
		CheckCircularity(cs, branch.parentType, root, incriminate)
	end

	for _,p in ipairs(branch.properties) do
		local pt = p.typeOf.refType
		if pt.type == "CStructuredType" and pt.declType == "struct" then
			CheckCircularity(cs, pt, root, incriminate)
		end
	end
end

local function CheckByVals(cs, st, incriminate)
	if st.mustBeRef then
		cerror(incriminate, "UnalignableByVal")
	end

	for _,p in ipairs(st.properties) do
		local pt = p.typeOf.refType
		if pt.type == "CStructuredType" and pt.declType == "struct" then
			CheckByVals(cs, pt, incriminate)
		end
	end
end

CompileTypeShells = function(cs)
	local newTypes = { }

	if #cs.uncompiled == 0 then
		return
	end

	cs.compilingTypes = true

	while true do
		local uncompiled = cs.uncompiled

		if #uncompiled == 0 then
			break
		end

		local compiledOK
		local madeAnyProgress = false
		local incriminate = nil
		cs.uncompiled = { }

		for _,u in ipairs(uncompiled) do
			compiledOK = false

			if u.isCompiled then
				-- Precompiled
				compiledOK = true
			elseif u.type == "CStructuredType" then
				incriminate = u.uncompiledNode
				compiledOK = CompileType(cs, u)

				if compiledOK then
					newTypes[#newTypes+1] = { st = u, incriminate = incriminate }
				end
			elseif u.type == "CMethodGroup" then
				incriminate = u.incriminate
				compiledOK = CompileMethodGroup(cs, u)
			elseif u.type == "CMethod" then
				incriminate = u.name
				compiledOK = CompileMethod(cs, u)
			elseif u.type == "CTypeTuple" then
				compiledOK = CompileTypeTuple(cs, u)
			elseif u.type == "CParameterList" then
				compiledOK = CompileParameterList(cs, u)
			elseif u.type == "CTypeReference" then
				incriminate = u.node
				compiledOK = CompileTypeReference(cs, u)
			elseif u.type == "CProperty" then
				incriminate = u.nameNode
				compiledOK = CompileProperty(cs, u)
			elseif u.type == "CStaticInstance" then
				incriminate = u.nameNode
				compiledOK = CompileStaticInstance(cs, u)
			elseif u.type == "CTypeFinalizer" then
				compiledOK, incriminate = CompileTypeFinalizer(cs, u)
			elseif u.type == "CArrayOfType" then
				incriminate = u.incriminateNode
				compiledOK = CompileArrayOfType(cs, u)
			elseif u.type == "CStaticDelegateType" or u.type == "CBoundDelegateType" then
				incriminate = u.incriminateNode
				compiledOK = CompileDelegateType(cs, u)
			elseif u.type == "CInitializer" then
				incriminate = u.uncompilednode
				compiledOK = CompileInitializer(cs, u)
			else
				error("Unknown uncompiled node type: "..u.type)
			end

			if compiledOK == true then
				madeAnyProgress = true
			else
				-- Reinsert and try again
				cs.uncompiled[#cs.uncompiled+1] = u
			end
		end

		if madeAnyProgress == false then
			cerror(incriminate, "CircularDependency")
		end
	end
	cs.compilingTypes = false
	
	-- Enforce non-circularity
	for _,nt in ipairs(newTypes) do
		local st = nt.st
		CheckCircularity(cs, nil, st, nt.incriminate)
		if st.type == "CStructuredType" and st.declType == "struct" and st.byVal then
			CheckByVals(cs, st, nt.incriminate)
		end
	end
end

-- -----------------------------------------------------------------------------------------------
LocalVariable = function(method, rType, isPointer, isConstant, note)
	assert(type(rType) == "table")
	assert(rType.type == "CStructuredType" or rType.type == "CArrayOfType" or rType.type == "CStaticDelegateType" or rType.type == "CBoundDelegateType")
	assert(method == nil or method.type == "CMethod")

	return {
		type = "CLocalVariable",
		isPointer = isPointer,
		method = method,
		accessModes = { isPointer and (isConstant and "CP" or "P") or "L" },
		vTypes = { rType },
		note = note,
		--stackIndex = ?
		}
end


local function CodeBlock(cs, ownerNamespace, ownerScope, initialLocalIndex)
	local cb = Scope(ownerScope)
	cb.type = "CCodeBlock"
	cb.locals = { }
	cb.CreateLocal = function(self, ces, localVar, name, hide)
		assert(ces.opstackIndex == 0)	-- Can't create locals in the middle of an operation except as temporaries

		localVar.stackIndex = ces.localCount
		ces.localCount = ces.localCount + 1

		self.locals[#self.locals+1] = localVar
		if name ~= nil then
			-- See if this is in a parent block
			local checkBlock = self.owner
			while checkBlock ~= nil and checkBlock.type == "CCodeBlock" do
				if checkBlock.symbols[name.string] ~= nil then
					cwarning(name, "MaskedLocal", name.string)
				end
				checkBlock = checkBlock.owner
			end
			assert(checkBlock ~= nil and (checkBlock.type == "CMemberLookupScope" or checkBlock.type == "CInstanciatedNamespace"), checkBlock.type)
			self:InsertUnique(name.string, localVar, name)

			if hide then
				localVar.invisibleSymbol = true
			end
		end
	end

	return cb
end


local function GuaranteeOuterBlock(cs, ces, ownerNamespace, ownerScope, initialLocalIndex)
	local ob = CodeBlock(cs, ownerNamespace, ownerScope, initialLocalIndex)
	ob.labelPrefix = ces:CreateLabel()
	ob.returnPath = ob.labelPrefix.."_greturn"
	ob.defaultPath = ob.labelPrefix.."_gdefaultpath"
	ob.codePaths = { ob.returnPath, ob.defaultPath }
	ob.codePathEscapes = { false, false }	-- Label to jump to after running the guaranteed statements
	ob.labelToCodePath = { }
	ob.guaranteeLocals = 0
	
	return ob
end

local function FindGuaranteeingBlock(cs, block)
	while not block.isRootLevel do
		block = block.owner
		if block == nil then
			break
		end
		if block.isGuaranteeInner then
			return block.owner
		end
	end
	return nil
end

local function StartGuaranteeInnerBlock(cs, ces, outerBlock)
	local lbl = outerBlock.labelPrefix
	local catchLabel = lbl.."_gcatch"
	local endTryLabel = lbl.."_gendtry"
	local method = BlockMethod(outerBlock)
	
	local ownerGuarantee = FindGuaranteeingBlock(cs, outerBlock)
	
	
	if ownerGuarantee == nil then
		outerBlock.carriedExceptionLocal = LocalVariable(method, cs.gst["Core.Exception"], false)
		ces:AddInstruction( { op = "alloclocal", res1 = "Core.Exception", str1 = "guarantee carried exception" } )
		outerBlock:CreateLocal(ces, outerBlock.carriedExceptionLocal)
		
		outerBlock.codePathLocal = LocalVariable(method, cs.gst["Core.int"], false)
		ces:AddInstruction( { op = "alloclocal", res1 = "Core.int", str1 = "guarantee code path" } )
		outerBlock:CreateLocal(ces, outerBlock.codePathLocal)
		
		outerBlock.guaranteeLocals = 2
		
		outerBlock.returnValueHolders = { }
	
		for idx,ref in ipairs(method.returnTypes.typeReferences) do
			local returnType = ref.refType
			local rvHolder = LocalVariable(method, returnType, false)
			ces:AddInstruction( { op = "alloclocal", res1 = "Core.int", str1 = "guarantee return value "..idx } )
			outerBlock:CreateLocal(ces, rvHolder)
			outerBlock.returnValueHolders[#outerBlock.returnValueHolders + 1] = rvHolder
			outerBlock.guaranteeLocals = outerBlock.guaranteeLocals + 1
		end
	else
		outerBlock.carriedExceptionLocal = ownerGuarantee.carriedExceptionLocal
		outerBlock.codePathLocal = ownerGuarantee.codePathLocal
		outerBlock.returnValueHolders = ownerGuarantee.returnValueHolders
	end

	ces:AddInstruction( { op = "try", str1 = catchLabel, str2 = endTryLabel } )
	ces:Discharge()
	
	local innerBlock = CodeBlock(cs, nil, outerBlock, outerBlock.localIndex)
	innerBlock.isGuaranteeInner = true

	return innerBlock
end

local function CloseGuaranteeInnerBlock(cs, ces, innerBlock)
	local outerBlock = innerBlock.owner
	local lbl = outerBlock.labelPrefix
	ces:Discharge()

	-- Fall-through code goes to the default path
	ces:AddInstruction( { op = "label", str1 = lbl.."_gendtry" } )
	ces:AddInstruction( { op = "enablepath", str1 = outerBlock.defaultPath } )
	ces:AddConstantInstruction("Core.int", standardGuaranteeBlocks.DEFAULT, "Value")
	ces:AddInstruction( { op = "localref", int1 = outerBlock.codePathLocal.stackIndex } )
	ces:AddInstruction( { op = "move" } )
	ces:AddInstruction( { op = "jump", str1 = lbl.."_gfinally" } )
	
	-- Exceptions go to the exception-carrying path
	ces:AddInstruction( { op = "label", str1 = lbl.."_gcatch" } )
	ces:AddInstruction( { op = "catch", res1 = "Core.Exception" } )
	ces:AddInstruction( { op = "localref", int1 = outerBlock.carriedExceptionLocal.stackIndex } )
	ces:AddInstruction( { op = "move" } )
	ces:AddConstantInstruction("Core.int", standardGuaranteeBlocks.EXCEPTION, "Value")
	ces:AddInstruction( { op = "localref", int1 = outerBlock.codePathLocal.stackIndex } )
	ces:AddInstruction( { op = "move" } )
	ces:AddInstruction( { op = "jump", str1 = lbl.."_gfinally" } )
	ces:Discharge()

	-- Emit flow path labels
	for pathIdx,path in ipairs(outerBlock.codePaths) do
		local pathName = path
		ces:AddInstruction( { op = "label", str1 = pathName } )
		ces:AddInstruction( { op = "enablepath", str1 = pathName } )
		ces:AddConstantInstruction("Core.int", tostring(pathIdx), "Value")
		ces:AddInstruction( { op = "localref", int1 = outerBlock.codePathLocal.stackIndex } )
		ces:AddInstruction( { op = "move" } )
		ces:AddInstruction( { op = "jump", str1 = lbl.."_gfinally" } )
	end

	ces:AddInstruction( { op = "label", str1 = lbl.."_gfinally" } )
	ces:Discharge()
end

local function AddLabelToGuarantee(cs, block, label)
	local codePathIdx = block.labelToCodePath[label]
	if codePathIdx == nil then
		codePathIdx = #block.codePaths + 1
		block.codePaths[codePathIdx] = block.labelPrefix.."_gpath_"..label
		block.codePathEscapes[codePathIdx] = label
		block.labelToCodePath[label] = codePathIdx
	end
	return assert(block.codePaths[codePathIdx])
end

local function CloseGuaranteeOuterBlock(cs, ces, block)
	local lbl = block.labelPrefix
	ces:Discharge()
	local owner = FindGuaranteeingBlock(cs, block)

	-- Carry exceptions
	ces:AddInstruction( { op = "localref", int1 = block.codePathLocal.stackIndex } )
	ces:AddInstruction( { op = "load" } )
	ces:AddConstantInstruction("Core.int", standardGuaranteeBlocks.EXCEPTION, "Value")
	ces:AddInstruction( { op = "jumpifnotequal", str1 = lbl.."_gnocarry", str2 = lbl.."_gcarry" } )
	ces:AddInstruction( { op = "label", str1 = lbl.."_gcarry" } )
	ces:AddInstruction( { op = "localref", int1 = block.carriedExceptionLocal.stackIndex } )
	ces:AddInstruction( { op = "load" } )
	ces:AddInstruction( { op = "throw" } )
	ces:AddInstruction( { op = "label", str1 = lbl.."_gnocarry" } )
	
	-- Emit code exit paths
	for pathIdx,path in ipairs(block.codePaths) do
		ces:AddInstruction( { op = "checkpathusage", str1 = path.."_gpathunused", str2 = path } )
		ces:AddInstruction( { op = "localref", int1 = block.codePathLocal.stackIndex } )
		ces:AddInstruction( { op = "load" } )
		ces:AddConstantInstruction("Core.int", tostring(pathIdx), "Value")
		ces:AddInstruction( { op = "jumpifnotequal", str1 = path.."_gpathunused", str2 = path.."_gpathtaken" } )
		ces:AddInstruction( { op = "label", str1 = path.."_gpathtaken" } )
		if path == block.returnPath then
			-- Special code for return path
			if owner then
				-- Has an owner, the value holders should be populated
				ces:AddInstruction( { op = "jump", str1 = owner.returnPath } )
			else
				-- No owner, exit the function
				for _,rvLocal in ipairs(block.returnValueHolders) do
					ces:AddInstruction( { op = "localref", int1 = rvLocal.stackIndex } )
				end
				ces:AddInstruction( { op = "return", int1 = #block.returnValueHolders } )
			end
		elseif path == block.defaultPath then
			ces:AddInstruction( { op = "jump", str1 = block.defaultPath } )
		else
			EmitAliasableJump(cs, ces, block, block.codePathEscapes[pathIdx])
		end
		ces:AddInstruction( { op = "label", str1 = path.."_gpathunused" } )
	end
	ces:AddInstruction( { op = "deadcode" } )

	for i=1,block.guaranteeLocals do
		ces:AddInstruction( { op = "removelocal" } )
	end
	
	ces:AddInstruction( { op = "label", str1 = block.defaultPath } )
	ces:Discharge()
end


AdjustValueCount = function(cs, cnode, incriminate, goalCount, noTruncationWarning)
	if cnode.vTypes == nil then
		cerror(incriminate, "ExpectedValue")
	end
	if #cnode.vTypes < goalCount then
		cerror(incriminate, "TooFewValues", tostring(goalCount), tostring(#cnode.vTypes))
	end
	if #cnode.vTypes == goalCount then
		return cnode	-- Nothing to do
	end

	local vTypes = { }
	local accessModes = { }
	for i=1,goalCount do
		vTypes[i] = cnode.vTypes[i]
		accessModes[i] = cnode.accessModes[i]
	end

	if (not noTruncationWarning) and cnode.type == "CMultipleValues" then
		local firstExprIndex = 1
		for idx,expr in ipairs(cnode.expressions) do
			if firstExprIndex > goalCount then
				cwarning(incriminate, "TruncatedValue")
				break
			end
			firstExprIndex = firstExprIndex + #expr.vTypes
		end
	end

	return {
		type = "CAdjustValueCount",
		accessModes = accessModes,
		vTypes = vTypes,
		expression = cnode
		}
end

local function EmitMethodCall(ces, m, explicit, delegateExpr)
	if delegateExpr then
		ces:AddInstruction( { op = "calldelegate", res1 = m.longName } )
	elseif m.vftIndex and not explicit then
		ces:AddInstruction( { op = "callvirtual", res1 = m.longName } )
	elseif m.type == "CBoundDelegateMarshal" then
		ces:AddInstruction( { op = "callvirtual", res1 = m.invokeName } )
	else
		assert(m.type == "CMethod")
		ces:AddInstruction( { op = "call", res1 = m.longName } )
	end
end

setglobal("VTypeIsObjectReference", function(vType)
	if vType.type == "CStructuredType" and (vType.declType == "class" or vType.declType == "interface") then
		return true
	end
	if vType.type == "CArrayOfType" or vType.type == "CStaticDelegateType" then
		return true
	end
	return false
end )

AMIsPointer = function(am)
	return (am == "P" or am == "CP" or am == "*P")
end

setglobal("VTypeIsRefStruct", function(vType)
	if vType.type == "CStructuredType" and vType.declType == "struct" and not vType.byVal then
		return true
	else
		return false
	end
end )

local function PushShellSpace(ces, vType)
	if vType.type == "CStructuredType" and VTypeIsRefStruct(vType) then
		-- This requires a temporary
		local temp = ces:AddTemporary(vType)
		ces:AddInstruction( { op = "localref", int1 = temp.stackIndex } )
		ces:AddInstruction( { op = "pinlocal" } )
		ces:Push(1, temp)
	else
		ces:AddInstruction( { op = "pushempty", res1 = vType.longName } )
		ces:Push(1)
	end
end

-- Emits expression, auto-converts if necessary
-- This can only actually convert one value, the one before the first value being dismantled, and it may not convert that either

-- Because R-values can contain multiple values, they may be partially "dismantled" to convert arguments:
-- Conversion types that can be done in-place (polymorphic cast, P-to-R) dismantle everything after them
-- Conversion types that require a previous stack entry (coerce) dismantle everything after them and will dismantle themselves if not first
-- R-to-P dismantles itself and everything after
-- Direct casts do not dismantle
local function EmitConvertedExpression(cs, expr, targetVTypes, targetAccessModes, ces)
	assert(#expr.vTypes == #targetVTypes)
	assert(#expr.vTypes ~= 0)
	assert(#expr.accessModes == #expr.vTypes)

	local firstDismantle
	local convertOperation

	for idx,svt in ipairs(expr.vTypes) do
		local sam = expr.accessModes[idx]
		local tvt = targetVTypes[idx]
		local tam

		if targetAccessModes then
			tam = targetAccessModes[idx]
			assert(tam)
		end

		if tam == "*" then
			tam = nil
		end

		--assert(tam ~= "L")

		local matchability = CastMatchability(svt, tvt, true)

		if matchability == matchLevels.EXACT or matchability == matchLevels.DIRECT then
			if sam == tam or tam == nil then
				-- Nothing to do
			elseif AMIsPointer(sam) and (tam == "CP" or tam == "*P") then
				-- Nothing to do
			elseif sam == "L" and AMIsPointer(tam) then
				convertOperation = "PinL"
				firstDismantle = idx+1
				break
			elseif sam == "L" and tam == "R" then
				convertOperation = "LoadL"
				firstDismantle = idx+1
				break
			elseif AMIsPointer(sam) and tam == "R" then
				convertOperation = "PtoR"
				firstDismantle = idx+1
				break
			elseif sam == "R" and AMIsPointer(tam) then
				firstDismantle = idx	-- RtoP is a dismantle followed by a secondary PinL
				break
			else
				-- NOTE: Errors are never caught at this point, filter them out in ConvertExpression!
				assert(false, "Access modes unconvertible, failed to properly convert "..svt.longName.." to "..tvt.longName..", match level "..matchability..", access modes "..sam.."-->"..tam)
			end
		elseif matchability == matchLevels.LOSSLESS or matchability == matchLevels.LOSSY then
			if svt.type == "CStaticDelegateType" and tvt.type == "CBoundDelegateType" then
				convertOperation = "BindStaticDelegate"
				firstDismantle = idx+1
				break
			else
				assert(svt.type == "CStructuredType")
				if idx == 1 then
					if matchability == matchLevels.LOSSLESS and tvt.type == "CStructuredType" and TypeImplementsInterface(svt, tvt) then
						convertOperation = "PolymorphicCast"
					elseif matchability == matchLevels.LOSSLESS and svt.declType == "interface" and tvt.longName == "Core.Object" then
						convertOperation = "PolymorphicCast"
					else
						convertOperation = "Coerce"
					end
					firstDismantle = idx+1
					break
				else
					firstDismantle = idx	-- Can only coerce the first parameter, anything else gets dismantled
				end
			end
			break
		elseif matchability == matchLevels.POLYMORPHIC then
			convertOperation = "PolymorphicCast"
			firstDismantle = idx+1
			break
		elseif matchability == matchLevels.VARYING then
			if sam == "L" and tam == "V" then
				convertOperation = "PinLV"
				firstDismantle = idx+1
			elseif sam == "P" and tam == "V" then
				convertOperation = "PtoV"
				firstDismantle = idx+1
			elseif sam == "R" and tam == "V" then
				firstDismantle = idx
				break
			else
				io.stderr:write("SAM: "..sam.."  TAM: "..tam.."\n")
				assert(false)
			end
		else
			assert(false)
		end
	end

	local cidx
	if firstDismantle ~= nil then
		cidx = firstDismantle - 1
	end

	-- Leave space for coerce
	if convertOperation == "Coerce" then
		-- Add space for the coercion
		PushShellSpace(ces, targetVTypes[1])
	end

	-- Emit the actual values
	EmitValues(cs, expr, ces, true)

	if convertOperation == "PinL" then
		ces:AddInstruction( { op = "pinlocal" } )
		return
	elseif convertOperation == "LoadL" then
		ces:AddInstruction( { op = "load" } )

		-- Cycle the opstack to remove the reference to this local
		ces:Pop()
		ces:Push(1)

		return
	elseif convertOperation == "PinLV" then
		ces:AddInstruction( { op = "pinlocal" } )
		ces:AddInstruction( { op = "tovarying" } )
		return
	end


	local dismantledTemporaries = { }

	-- Dismantle everything else
	if firstDismantle then
		local dismantlingIndex = #expr.vTypes
		while dismantlingIndex >= firstDismantle do
			local temp = ces:AddTemporary(expr.vTypes[dismantlingIndex])
			ces:AddInstruction( { op = "localref", int1 = temp.stackIndex } )
			if expr.accessModes[dismantlingIndex] == "R" then
				ces:AddInstruction( { op = "move" } )
				ces:Pop()
			elseif expr.accessModes[dismantlingIndex] == "P" then
				ces:AddInstruction( { op = "move" } )
				ces:Pop()
			else
				assert(false, "COMPILER ERROR: Dismantled a localref, localrefs can only be singular expressions")
			end

			dismantledTemporaries[dismantlingIndex] = temp
			dismantlingIndex = dismantlingIndex - 1
		end
	end


	if convertOperation ~= nil then
		if convertOperation == "Coerce" then
			local fromType = expr.vTypes[cidx]
			local toType = targetVTypes[cidx]
			local tam

			if targetAccessModes == nil then
				tam = "*"
			else
				tam = targetAccessModes[cidx]
			end

			local coerceMethod = FindCoerce(fromType, toType, true)
			if coerceMethod == nil then
				coerceMethod = FindCoerce(fromType, toType, false)
				assert(coerceMethod)
			end

			local rt = coerceMethod.actualParameterList.parameters[1].type.refType

			if VTypeIsRefStruct(rt) then
				-- To P
				if expr.accessModes[cidx] == "L" then
					ces:AddInstruction( { op = "pinlocal" } )
				elseif expr.accessModes[cidx] == "R" then
					local temp = ces:AddTemporary(rt)
					ces:AddInstruction( { op = "localref", int1 = temp.stackIndex } )
					ces:AddInstruction( { op = "move" } )
					ces:Pop()
					ces:AddInstruction( { op = "localref", int1 = temp.stackIndex } )
					ces:AddInstruction( { op = "pinlocal" } )
					ces:Push(1, temp)
				end
			else
				-- To R
				if expr.accessModes[cidx] == "L" or
					AMIsPointer(expr.accessModes[cidx]) then
					ces:AddInstruction( { op = "load" } )

					-- Cycle the opstack to remove the reference to this local
					ces:Pop()
					ces:Push(1)
				else
					assert(expr.accessModes[cidx] == "R", "Expr wasn't R, it was "..expr.accessModes[cidx])
				end
			end

			EmitMethodCall(ces, coerceMethod)
			ces:Pop()

			if VTypeIsRefStruct(toType) then
				assert(tam == "P" or tam == "*")
			else
				-- Value struct on the stack
				if tam == "P" then
					local temp = ces:AddTemporary(toType)
					ces:AddInstruction( { op = "localref", int1 = temp.stackIndex } )
					ces:AddInstruction( { op = "move" } )
					ces:AddInstruction( { op = "localref", int1 = temp.stackIndex } )
					ces:AddInstruction( { op = "pinlocal" } )
					ces:Pop()
					ces:Push(1, temp)
				else
					assert(tam == "R" or tam == "*")
				end
			end
		elseif convertOperation == "PtoR" then
			ces:AddInstruction( { op = "load" } )

			-- Cycle the opstack to remove the reference to this local
			ces:Pop()
			ces:Push(1)
		elseif convertOperation == "PtoV" then
			ces:AddInstruction( { op = "tovarying" } )
		elseif convertOperation == "PolymorphicCast" then
			local exprAM = expr.accessModes[cidx]

			if exprAM == "P" or exprAM == "CP" or exprAM == "L" then
				-- Load from pointer or local if needed
				ces:AddInstruction( { op = "load" } )

				-- Cycle the opstack to remove the reference to this local
				ces:Pop()
				ces:Push(1)
			else
				assert(exprAM == "R")
			end
			ces:AddInstruction( { op = "cast", res1 = targetVTypes[cidx].longName } )

			local tam = targetAccessModes[cidx]
			assert(tam == "R" or tam == "*" or tam == nil, "Polymorphic cast requested to a non-R-value")
		elseif convertOperation == "BindStaticDelegate" then
			local fromType = expr.vTypes[cidx]
			local toType = targetVTypes[cidx]
			local tam

			if targetAccessModes == nil then
				tam = "*"
			else
				tam = targetAccessModes[cidx]
			end

			if expr.accessModes[cidx] == "L" or
				expr.accessModes[cidx] == "P" then
				ces:AddInstruction( { op = "load" } )

				-- Cycle the opstack to remove the reference to this local
				ces:Pop()
				ces:Push(1)
			else
				assert(expr.accessModes[cidx] == "R")
			end

			local bdTemp = ces:AddTemporary(toType)
			local glue = MarshalForBoundDelegate(cs, toType, fromType)
			ces:AddInstruction( { op = "newinstanceset", int1 = bdTemp.stackIndex, intVar = { "0" }, res1 = glue.longName } )
			ces:Pop()
			ces:AddInstruction( { op = "localref", int1 = bdTemp.stackIndex } )
			ces:Push(1, bdTemp)

			if tam == "P" then
				ces:AddInstruction( { op = "pinlocal" } )
			else
				assert(tam == "R" or tam == "*")
			end
		else
			assert(false)
		end
	end

	if firstDismantle then
		for i=firstDismantle,#expr.vTypes do
			-- Re-emit dismantled values
			local tam
			if targetAccessModes then
				tam = targetAccessModes[i]
			else
				tam = "*"
			end
			EmitConvertedExpression(cs, dismantledTemporaries[i], { targetVTypes[i] }, { tam }, ces)
		end
	end
end


local EmitBooleanToLogical = function(cs, expr, ces, trueLabel, falseLabel)
	local boolType = cs.gst["Core.bool"]

	assert(boolType)
	assert(expr.vTypes[1] == boolType)
	assert(expr.accessModes[1] == "R")

	if expr.type == "CConstant" and expr.vTypes[1] == boolType then
		-- Optimize out static branches
		if tostring(expr.value) == "true" then
			ces:AddInstruction( { op = "jump", str1 = trueLabel } )
		else
			assert(tostring(expr.value) == "false")
			ces:AddInstruction( { op = "jump", str1 = falseLabel } )
		end
	end

	EmitValues(cs, expr, ces, false)
	ces:AddInstruction( { op = "jumpiftrue", str1 = trueLabel, str2 = falseLabel } )
	ces:Pop()
end

local function EmitLogical(cs, expr, ces, trueLabel, falseLabel)
	if expr.type == "CMethodCall" and expr.method.isBranching then
		EmitValues(cs, expr.parameters, ces)

		ces:AddInstruction( { op = "jumpif", res1 = expr.method.longName, str1 = trueLabel, str2 = falseLabel } )
		ces:Pop(#expr.method.actualParameterList.parameters)
	elseif expr.type == "CLogicalNotNode" then
		EmitLogical(cs, expr.expr, ces, falseLabel, trueLabel)
	elseif expr.type == "CLogicalAndNode" then
		local evaluateSecondLabel = ces:CreateLabel().."_and"

		EmitLogical(cs, expr.leftExpr, ces, evaluateSecondLabel, falseLabel)
		ces:AddInstruction( { op = "label", str1 = evaluateSecondLabel } )
		EmitLogical(cs, expr.rightExpr, ces, trueLabel, falseLabel)
	elseif expr.type == "CLogicalOrNode" then
		local evaluateSecondLabel = ces:CreateLabel().."_or"

		EmitLogical(cs, expr.leftExpr, ces, trueLabel, evaluateSecondLabel)
		ces:AddInstruction( { op = "label", str1 = evaluateSecondLabel } )
		EmitLogical(cs, expr.rightExpr, ces, trueLabel, falseLabel)
	elseif expr.type == "CEqualityCompare" then
		local nullCheckExpr
		if expr.leftExpr.type == "CConstant" and expr.leftExpr.signal == "NullRef" then
			nullCheckExpr = expr.rightExpr
		elseif expr.rightExpr.type == "CConstant" and expr.rightExpr.signal == "NullRef" then
			nullCheckExpr = expr.leftExpr
		end
		
		if nullCheckExpr ~= nil then
			if nullCheckExpr.type == "CConstant" and nullCheckExpr.signal == "NullRef" then
				ces:AddInstruction( { op = "jump", str1 = trueLabel } )
			else
				EmitValues(cs, nullCheckExpr, ces, true)

				local instrOp
				if expr.operator == "__eq" then
					instrOp = "jumpifnull"
				elseif expr.operator == "__ne" then
					instrOp = "jumpifnotnull"
				else
					assert(false)
				end
				ces:AddInstruction( { op = instrOp, str1 = trueLabel, str2 = falseLabel } )
				ces:Pop(1)
			end
		else
			EmitValues(cs, expr.leftExpr, ces, true)
			EmitValues(cs, expr.rightExpr, ces, true)

			local instrOp
			if expr.operator == "__eq" then
				instrOp = "jumpifequal"
			elseif expr.operator == "__ne" then
				instrOp = "jumpifnotequal"
			else
				assert(false)
			end

			ces:AddInstruction( { op = instrOp, str1 = trueLabel, str2 = falseLabel } )
			ces:Pop(2)
		end
	elseif expr.type == "CCheckCast" then
		EmitValues(cs, expr.expression, ces, false)
		ces:AddInstruction( { op = "res", res1 = expr.checkType.longName } )
		ces:AddInstruction( { op = "jumpif", res1 = "Core.Object/methods/CanConvertTo(notnull Core.RDX.Type)", str1 = trueLabel, str2 = falseLabel } )
		ces:Pop(1)
	else
		EmitBooleanToLogical(cs, expr, ces, trueLabel, falseLabel)
	end
end

local EmitLogicalToBoolean = function(cs, expr, ces)
	ces:AddInstruction( { op = "pushempty", res1 = "Core.bool" } )
	ces:Push(1)

	ces:AddInstruction( { op = "startbarrier", int1 = 1 } )

	local lbl = ces:CreateLabel()
	local trueLabel = lbl.."_true"
	local falseLabel = lbl.."_false"

	EmitLogical(cs, expr, ces, trueLabel, falseLabel)

	ces:AddInstruction( { op = "label", str1 = trueLabel } )
	ces:AddConstantInstruction("Core.bool", "true", "Value")
	ces:Push(1)
	ces:AddInstruction( { op = "return", int1 = 1 } )
	ces:Pop()
	ces:AddInstruction( { op = "label", str1 = falseLabel } )
	ces:AddConstantInstruction("Core.bool", "false", "Value")
	ces:Push(1)
	ces:AddInstruction( { op = "return", int1 = 1 } )
	ces:Pop()
	ces:AddInstruction( { op = "endbarrier" } )
end

local function EmitArrayElementInitialize(cs, expr, arrayIndex, indexExpressions, dimIndex, deadTable, ces, iidx)
	local subD = expr.dimensions[dimIndex+1]

	for i=1,expr.dimensions[dimIndex] do
		indexExpressions[dimIndex] = Constant(cs, arrayIndexType, i-1, "Value")

		local d
		if subD then
			iidx = EmitArrayElementInitialize(cs, expr, arrayIndex, indexExpressions, dimIndex+1, deadTable, ces, iidx)
		else
			local src = SingleExpressionToList(expr.initializers[iidx])

			EmitAssign(cs, arrayIndex, src, deadTable, ces, expr.incriminateNode)
			iidx = iidx + 1
		end
	end

	return iidx
end

local function EmitArrayInitialize(cs, expr, ces)
	local aist = cs.gst[arrayIndexType]
	local indexVTypes = { }
	local indexAccessModes = { }
	local initTarget = expr.localVar
	local exportTarget = initTarget
	local initAsConst = false

	for idx=1,#expr.dimensions do
		indexVTypes[idx] = aist
		indexAccessModes[idx] = "R"
	end

	local indexValues = FlattenMV({
		type = "CMultipleValues",
		expressions = { },
		accessModes = indexAccessModes,
		vTypes = indexVTypes,
	})

	assert(expr.vTypes[1].containedType)
	
	local arrayVType = expr.localVar.vTypes[1]
	local createVType = arrayVType
	if arrayVType.isConst then
		-- Need to form as the non-const the non-const version of this array
		local arrayVariationCode = ArrayOfTypeCode(arrayVType.dimensions, false)

		createVType = arrayVType.containedType.arraysOf[arrayVariationCode]
		if createVType == nil then
			createVType = ArrayOfType(cs, arrayVType.containedType, arrayVType.dimensions, false, arrayVType.incriminateNode)
			arrayVType.containedType.arraysOf[arrayVariationCode] = createVType

			CompileArrayOfType(cs, createVType)
		end

		initTarget = ces:AddTemporary(createVType)
		initTarget.holdOpenTemp = true
	end

	local arrayIndex = {
		type = "CArrayIndex",
		operand = ConvertExpression(cs, expr.incriminateNode, initTarget, { createVType }, { "R" }, false ),
		indexes = indexValues,
		vTypes = { expr.vTypes[1].containedType },
		accessModes = { "P" },
	}

	EmitArrayElementInitialize(cs, expr, SingleExpressionToList(arrayIndex), indexValues.expressions, 1, { }, ces, 1)

	if arrayVType.isConst then
		-- Convert to constant array
		ces:AddInstruction( { op = "pushempty", res1 = "Core.Array" } )
		ces:AddInstruction( { op = "localref", int1 = initTarget.stackIndex } )
		ces:AddInstruction( { op = "load" } )
		ces:AddInstruction( { op = "call", res1 = "Core.Array/methods/ToConst()" } )
		ces:AddInstruction( { op = "cast", res1 = arrayVType.longName } )
		ces:AddInstruction( { op = "localref", int1 = exportTarget.stackIndex } )
		ces:AddInstruction( { op = "move" } )

		-- Cycle the temporary
		initTarget.holdOpenTemp = false
		ces:Push(1, initTarget)
		ces:Pop(1)
	end

end

-- Pushes values from an expression to the stack.  The expression must only contain R and P values
EmitValues = function(cs, expr, ces, allowLocals)

	if expr.type == "CMultipleValues" then
		for _,subExpr in ipairs(expr.expressions) do
			assert(subExpr.type ~= "CMultipleValues")	-- Parameters and expression lists should always be flat
			EmitValues(cs, subExpr, ces, allowLocals)
		end
		return
	end

	if not allowLocals then
		for _,am in ipairs(expr.accessModes) do
			assert(am == "P" or am == "CP" or am == "R" or am == "V", "Unexpected access mode "..tostring(am).." in expr type "..expr.type)
		end
	end

	if expr.type == "CLocalVariable" then
		assert(expr.stackIndex)
		ces:AddInstruction( { op = "localref", int1 = expr.stackIndex } )
		if expr.isPointer then
			-- Move pointer to stack
			ces:AddInstruction( { op = "load" } )
			ces:Push()
		else
			ces:Push(1, expr)
		end
	elseif expr.type == "CConvertExpression" then
		-- TEMPORARY: No convert!
		EmitConvertedExpression(cs, expr.expression, expr.vTypes, expr.accessModes, ces)
	elseif expr.type == "CMethodCall" then
		local method = expr.method

		if method.isBranching then
			EmitLogicalToBoolean(cs, expr, ces)
		else
			if #expr.method.returnTypes.typeReferences ~= 0 then
				for _,tref in ipairs(expr.method.returnTypes.typeReferences) do
					PushShellSpace(ces, tref.refType)
				end
			end

			if expr.delegateExpr then
				assert(expr.delegateExpr.accessModes[1] == "L")
				EmitValues(cs, expr.delegateExpr, ces, true)
			end

			EmitValues(cs, expr.parameters, ces)

			EmitMethodCall(ces, expr.method, expr.explicit, expr.delegateExpr)
			ces:Pop(#expr.method.actualParameterList.parameters)

			if expr.delegateExpr then
				ces:Pop(1)
			end
		end
	elseif expr.type == "CConstant" then
		if expr.signal == "NullRef" then
			ces:AddInstruction( { op = "null" } )
		elseif expr.signal == "Resource" then
			ces:AddInstruction( { op = "res", res1 = expr.value } )
		else
			local vType = expr.vTypes[1]
			ces:AddConstantInstruction(vType.longName, tostring(expr.value), expr.signal)
		end
		ces:Push()
	elseif expr.type == "CObjectProperty" then
		EmitValues(cs, expr.object, ces)
		ces:AddInstruction( { op = "property", int1 = expr.property.propertyIndex - 1 } )
	elseif expr.type == "CAdjustValueCount" then
		local vCount = #expr.expression.vTypes
		local targetVCount = #expr.vTypes
		EmitValues(cs, expr.expression, ces, allowLocals)

		while vCount > targetVCount do
			ces:AddInstruction( { op = "pop" } )
			ces:Pop()
			vCount = vCount - 1
		end
	elseif expr.type == "CArrayIndex" then
		local numDimensions = expr.operand.vTypes[1].dimensions
		assert(numDimensions)

		EmitValues(cs, expr.operand, ces)
		EmitValues(cs, expr.indexes, ces)
		ces:AddInstruction( { op = "arrayindex", int1 = numDimensions } )
		ces:Pop(numDimensions)
	elseif expr.type == "CLogicalNotNode" or expr.type == "CLogicalAndNode" or expr.type == "CLogicalOrNode" or expr.type == "CEqualityCompare" or expr.type == "CCheckCast" then
		EmitLogicalToBoolean(cs, expr, ces)
	elseif expr.type == "CNewInstance" then
		if expr.parameters then
			EmitValues(cs, expr.parameters, ces)
		end

		local numDimensions = 0

		if expr.vTypes[1].type == "CArrayOfType" then
			numDimensions = expr.vTypes[1].dimensions
		end

		ces:AddInstruction( { op = "newinstance", res1 = expr.vTypes[1].longName, int1 = numDimensions } )

		if expr.parameters then
			ces:Pop(#expr.parameters.vTypes)
		end
		ces:Push(1)	-- Instance
	elseif expr.type == "CInitializeAndRecover" then
		EmitValues(cs, expr.expression, ces)
	elseif expr.type == "CAllocateTemporary" then
		local temp = ces:AddTemporary(expr.vTypes[1])

		ces:AddInstruction( { op = "localref", int1 = temp.stackIndex } )
		ces:Push(1, temp)
	elseif expr.type == "CCloneExpression" then
		if expr.expression then
			EmitValues(cs, expr.expression, ces, allowLocals)
		end
		ces:AddInstruction( { op = "clone", int1 = 0, int2 = 1 } )
		ces:Clone(0, 1)
	elseif expr.type == "CStaticInstance" then
		ces:AddInstruction( { op = "res", res1 = expr.longName } )
		ces:Push(1)
	elseif expr.type == "CGenerateHashNode" then
		EmitValues(cs, expr.expression, ces)
		ces:AddInstruction( { op = "hash" } )

		-- Cycle the opstack to remove any temporary refs
		ces:Pop()
		ces:Push(1)
	elseif expr.type == "CInitializeArray" then
		local temp = ces:AddTemporary(expr.vTypes[1])

		for _,d in ipairs(expr.dimensions) do
			ces:AddConstantInstruction(arrayIndexType, tostring(d), "Value")
		end

		ces:AddInstruction( { op = "newinstance", res1 = expr.vTypes[1].longName, int1 = #expr.dimensions } )
		ces:AddInstruction( { op = "localref", int1 = temp.stackIndex } )
		ces:AddInstruction( { op = "move" } )

		expr.localVar.stackIndex = temp.stackIndex

		EmitArrayInitialize(cs, expr, ces)

		ces:AddInstruction( { op = "localref", int1 = temp.stackIndex } )
		ces:Push(1, temp)
	elseif expr.type == "CInitializeProperties" then
		local temp = ces:AddTemporary(expr.vTypes[1])
		local initIndexes = { }

		for _,i in ipairs(expr.initializers) do
			local property = i.dest.property
			initIndexes[#initIndexes+1] = property.propertyIndex - 1

			local reduced = AdjustValueCount(cs, i.src, i.incriminateNode, 1)
			local targetVType = property.typeOf.refType
			local targetAccessMode = "R"

			if VTypeIsRefStruct(targetVType) then
				targetAccessMode = "CP"
			end

			local converted = ConvertExpression(cs, i.incriminateNode, reduced, { targetVType }, { targetAccessMode }, false )
			EmitValues(cs, converted, ces, false)
		end
		
		ces:AddInstruction( { op = "newinstanceset", int1 = temp.stackIndex, intVar = initIndexes, res1 = expr.vTypes[1].longName } )
		ces:Pop(#initIndexes)

		ces:AddInstruction( { op = "localref", int1 = temp.stackIndex } )
		ces:Push(1, temp)
	elseif expr.type == "CMethodDelegation" then
		ces:AddInstruction( { op = "res", res1 = expr.method.longName } )
		ces:AddInstruction( { op = "cast", res1 = expr.vTypes[1].longName } )
		ces:Push(1)
	elseif expr.type == "CRecycledValues" then
		local recycleStart = expr.opstackOffset or (ces.opstackIndex - expr.opstackIndex - 1)
		local recycleCount = #expr.vTypes

		ces:AddInstruction( { op = "clone", int1 = recycleStart, int2 = recycleCount } )
		ces:Clone(recycleStart, recycleCount)
	elseif expr.type == "CTernary" then
		-- Push shells
		local numValues = #expr.vTypes
		for i=1,numValues do
			PushShellSpace(ces, expr.vTypes[i])
		end
		
		ces:AddInstruction( { op = "startbarrier", int1 = numValues } )

		local lbl = ces:CreateLabel()
		local trueLabel = lbl.."_true"
		local falseLabel = lbl.."_false"

		EmitLogical(cs, expr.cond, ces, trueLabel, falseLabel)

		ces:AddInstruction( { op = "label", str1 = trueLabel } )
		EmitValues(cs, expr.trueExprs, ces)
		ces:AddInstruction( { op = "return", int1 = numValues } )
		ces:Pop(numValues)
		ces:AddInstruction( { op = "label", str1 = falseLabel } )
		EmitValues(cs, expr.falseExprs, ces)
		ces:AddInstruction( { op = "return", int1 = numValues } )
		ces:Pop(numValues)
		ces:AddInstruction( { op = "endbarrier" } )
	elseif expr.type == "CBoundMethodDelegation" then
		local glue = MarshalForBoundDelegate(cs, expr.vTypes[1], expr.method)

		if expr.object then
			local temp = ces:AddTemporary(expr.vTypes[1])
			EmitValues(cs, expr.object, ces)
			ces:AddInstruction( { op = "newinstanceset", int1 = temp.stackIndex, intVar = { "0" }, res1 = glue.longName } )
			ces:Pop(1)
			ces:AddInstruction( { op = "localref", int1 = temp.stackIndex } )
			ces:Push(1, temp)
		else
			ces:AddInstruction( { op = "newinstance", int1 = 0, res1 = glue.longName } )
			ces:Push(1)
		end
	else
		error("Couldn't emit values for node type "..expr.type)
	end
end

EmitOperateAndAssign = function(cs, destNode, right, operator, scope, ces, node)
	-- Index: Cache operands[1] and [2]
	-- Indirect: Cache operand[1]
	-- Everything else: No cache

	if destNode.type == "Index" then
		local indexSource = CompileExpression(cs, destNode.operands[1], scope, true)
		local indexIndexes = CompileExpressionList(cs, destNode.operands[2], scope, true)

		indexSource = AdjustValueCount(cs, indexSource, node, 1)

		-- Make sure the recycled values are stored, locals can't be dismantled from multi-value expressions
		do
			local indexAccessModes = { }

			for idx,vt in ipairs(indexIndexes.vTypes) do
				if VTypeIsRefStruct(vt) then
					indexAccessModes[idx] = "CP"
				else
					indexAccessModes[idx] = "R"
				end
			end

			indexIndexes = ConvertExpression(cs, node, indexIndexes, indexIndexes.vTypes, indexAccessModes, false)
		end

		local sourceRecycle =
		{
			type = "CRecycledValues",
			vTypes = indexSource.vTypes,
			accessModes = indexSource.accessModes,
			opstackIndex = ces.opstackIndex,

			line = node.line,
			filename = node.filename,
		}

		EmitValues(cs, indexSource, ces, true)

		local indexRecycle =
		{
			type = "CRecycledValues",
			vTypes = indexIndexes.vTypes,
			accessModes = indexIndexes.accessModes,
			opstackIndex = ces.opstackIndex,

			line = node.line,
			filename = node.filename,
		}

		EmitValues(cs, indexIndexes, ces, true)

		local virtualNode =
		{
			type = "Index",
			operands = { sourceRecycle, SingleExpressionToList(indexRecycle) },

			line = node.line,
			filename = node.filename,
		}

		local loadExpression = CompileIndexExpression(cs, virtualNode, scope, true)

		local operationNode =
		{
			type = "BinaryOperatorNode",
			operator = node.operator,
			operands = { loadExpression, right },

			line = node.line,
			filename = node.filename,
		}
		assert(operationNode.line)

		local compiledResult = CompileExpression(cs, operationNode, scope, true)

		local saveExpression = CompileIndexExpression(cs, virtualNode, scope, false)

		EmitAssign(cs, SingleExpressionToList(saveExpression), SingleExpressionToList(compiledResult), scope, ces, node)

		local pops = #indexSource.vTypes + #indexIndexes.vTypes
		for i=1,pops do
			ces:AddInstruction( { op = "pop" } )
			ces:Pop(1)
		end
	elseif destNode.type == "Indirect" then
		local indirSource = CompileExpression(cs, destNode.operands[1], scope, true)

		-- TODO: If indirSource has "R" access mode, refuse this
		local sourceRecycle =
		{
			type = "CRecycledValues",
			vTypes = indirSource.vTypes,
			accessModes = indirSource.accessModes,
			opstackIndex = ces.opstackIndex,

			line = node.line,
			filename = node.filename,
		}

		EmitValues(cs, indirSource, ces, true)

		local virtualNode =
		{
			type = "Indirect",
			operands = { sourceRecycle, destNode.operands[2] },

			line = node.line,
			filename = node.filename,
		}

		local loadExpression = CompileExpression(cs, virtualNode, scope, true)

		local operationNode =
		{
			type = "BinaryOperatorNode",
			operator = node.operator,
			operands = { loadExpression, right },

			line = node.line,
			filename = node.filename,
		}
		assert(operationNode.line)

		local compiledResult = CompileExpression(cs, operationNode, scope, true)

		local saveExpression = CompileExpression(cs, virtualNode, scope, false)

		EmitAssign(cs, SingleExpressionToList(saveExpression), SingleExpressionToList(compiledResult), scope, ces, node)

		local pops = #indirSource.vTypes
		for i=1,pops do
			ces:AddInstruction( { op = "pop" } )
			ces:Pop(1)
		end
	else
		local loadTarget = CompileExpression(cs, destNode, scope, true)
		local saveTarget = CompileExpression(cs, destNode, scope, false)

		local operationNode =
		{
			type = "BinaryOperatorNode",
			operator = node.operator,
			operands = { loadTarget, right },

			line = destNode.line,
			filename = destNode.filename,
		}

		local compiledResult = CompileExpression(cs, operationNode, scope, true)

		EmitAssign(cs, SingleExpressionToList(saveTarget), SingleExpressionToList(compiledResult), scope, ces, node)
	end

	--assert(false)
end

EmitAssign = function(cs, dest, src, scope, ces, incriminate)
	assert(src.type == "CMultipleValues" or src.type == "CAdjustValueCount")
	assert(dest.type == "CMultipleValues")

	local leftExpr = CompileExpressionList(cs, dest, scope, false)

	if src.vTypes == nil then
		cerror(incriminate, "AssignRightSideNotValue")
	end
	if leftExpr.vTypes == nil or leftExpr.vTypes[1] == nil then
		cerror(incriminate, "AssignLeftSideNotVariable")
	end

	if #src.vTypes ~= #leftExpr.vTypes then
		cerror(incriminate, "AssignTooFewValues", tostring(#leftExpr.vTypes), tostring(#src.vTypes))
	end


	local targetVTypes = { }
	local targetAccessModes = { }
	local cachedIntercepts = { }

	for idx,leftExpr in ipairs(leftExpr.expressions) do
		if src.vTypes[idx] == nil then
			cerror(incriminate, "AssignRightSideTooFewValues")
		end

		local leftAccessMode = leftExpr.accessModes[1]
		local leftVType = leftExpr.vTypes[1]

		if leftAccessMode == "R" then
			if leftVType.longName == "Core.nullreference" then
				targetAccessModes[#targetAccessModes+1] = "*"
				targetVTypes[#targetVTypes+1] = leftVType
			else
				cerror(incriminate, "AssignLeftSideNotVariable")
			end
		elseif leftAccessMode == "L" or leftAccessMode == "P" then
			targetAccessModes[#targetAccessModes+1] = "*"
			targetVTypes[#targetVTypes+1] = leftVType
		elseif leftAccessMode == "I" then
			local placeholderParameters = SingleExpressionToList(PlaceholderValue(cs, src.vTypes[idx]))
			local intercept = MatchMethodCall(cs, leftExpr, placeholderParameters, incriminate)

			targetAccessModes[#targetAccessModes+1] = intercept.parameters.accessModes[1]
			targetVTypes[#targetVTypes+1] = intercept.parameters.vTypes[1]

			-- Convert target ref to the appropriate access mode
			intercept.convertedParameters = ConvertExpression(cs, incriminate, leftExpr.object, { intercept.parameters.vTypes[2] }, { intercept.parameters.accessModes[2] }, false)

			cachedIntercepts[idx] = intercept
		elseif leftAccessMode == "A" then
			local placeholderParameters = SingleExpressionToList(PlaceholderValue(cs, src.vTypes[idx]))

			for _,expr in ipairs(leftExpr.indexes.expressions) do
				AppendExpressionToMV(placeholderParameters, expr)
			end

			local intercept = MatchMethodCall(cs, leftExpr.methodCall, placeholderParameters, incriminate)

			-- Convert the indexes to the appropriate type
			do
				local indexTargetAccessModes = { }
				local indexTargetVTypes = { }

				for idx=3,#intercept.parameters.accessModes do
					indexTargetAccessModes[idx-2] = intercept.parameters.accessModes[idx]
					indexTargetVTypes[idx-2] = intercept.parameters.vTypes[idx]
				end
				intercept.convertedParameters = ConvertExpression(cs, incriminate, leftExpr.indexes, indexTargetVTypes, indexTargetAccessModes, false)
			end

			targetAccessModes[#targetAccessModes+1] = intercept.parameters.accessModes[1]
			targetVTypes[#targetVTypes+1] = intercept.parameters.vTypes[1]

			cachedIntercepts[idx] = intercept
		elseif leftAccessMode == "CP" then
			cerror(incriminate, "AssignToConstant")
		else
			assert(false, "Bad left side access mode from "..leftExpr.type..", discharged: "..tostring(leftExpr.discharged)..", access mode "..leftAccessMode)
		end
	end

	local cRight = ConvertExpression(cs, incriminate, src, targetVTypes, targetAccessModes, false, true)

	EmitValues(cs, cRight, ces, true)

	local numExpr = #leftExpr.expressions
	for iidx=1,numExpr do
		local idx = numExpr-iidx+1

		local leftExpr = leftExpr.expressions[idx]
		local leftVType = leftExpr.vTypes[1]
		local leftAccessMode = leftExpr.accessModes[1]
		local rightAccessMode = cRight.accessModes[idx]

		if leftAccessMode == "L" then
			EmitValues(cs, leftExpr, ces, true)

			ces:AddInstruction( { op = "move" } )
			ces:Pop(2)
		elseif leftAccessMode == "P" then
			EmitValues(cs, leftExpr, ces, true)

			ces:AddInstruction( { op = "move" } )

			ces:Pop(2)
		elseif leftAccessMode == "A" then
			EmitValues(cs, leftExpr.methodCall.object, ces, true)

			local intercept = cachedIntercepts[idx]

			intercept.parameters = intercept.convertedParameters  --leftExpr.indexes

			EmitValues(cs, intercept, ces)
		elseif leftAccessMode == "I" then
			-- FIXME: Using convertedParameters is a bit of a hack?

			local intercept = cachedIntercepts[idx]
			EmitValues(cs, intercept.convertedParameters, ces, true)

			intercept.parameters = EmptyExpressionList()

			EmitValues(cs, intercept, ces)
		elseif leftAccessMode == "R" and leftVType.longName == "Core.nullreference" then
			ces:AddInstruction( { op = "pop" } )
			ces:Pop(1)
		else
			assert(false)
		end
	end
end

local function UnhideLocals(localMV)
	for _,lv in ipairs(localMV.expressions) do
		lv.invisibleSymbol = nil
	end
end

local function EmitDeclsToLocals(cs, declsNode, block, ces)
	local declarations = { }
	local declVTypes = { }
	local declAccessModes = { }

	for _,decl in ipairs(declsNode.declarations) do
		local tr = TypeReference(cs, decl.type, block)
		CompileTypeReference(cs, tr)
		assert(tr.isCompiled)

		local t = tr.refType

		if t.longName == varyingType then
			cerror(decl.name, "VaryingLocal")
		end

		local l = LocalVariable(BlockMethod(block), t, false)

		ces:AddInstruction( { op = "alloclocal", res1 = l.vTypes[1].longName, str1 = decl.name.string } )

		block:CreateLocal(ces, l, decl.name, true)
		declarations[#declarations+1] = l
		declVTypes[#declVTypes+1] = l.vTypes[1]
		declAccessModes[#declAccessModes+1] = "L"
	end

	ces:Discharge()		-- Don't allow compiler temps to be created before these

	assert(declVTypes)
	return FlattenMV({
		type = "CMultipleValues",
		accessModes = declAccessModes,
		expressions = declarations,
		vTypes = declVTypes,
		})
end

local function EmitLocalDecl(cs, expr, block, ces)
	local mvNode = EmitDeclsToLocals(cs, expr.declarations, block, ces)

	if expr.initializers then
		local initializers = CompileExpressionList(cs, expr.initializers, block, true)
		initializers = AdjustValueCount(cs, initializers, expr.initializers, #mvNode.vTypes)
		EmitAssign(cs, mvNode, initializers, block, ces, expr)
	end

	UnhideLocals(mvNode)
end

-- Emits a "using" protection block for a single portion of an initializer list
-- If no initializers are available, emits the code block instead
local function EmitUsingDeclPortion(cs, baseExpr, localMV, initExprs, block, outerBlock, ces, localStart, exprStart)
	if localMV.vTypes[localStart] == nil then
		if initExprs[exprStart] ~= nil then
			cerror(baseExpr, "AssignTooFewValues", tostring(#localMV.vTypes), tostring(#initExprs))
		end

		-- Ran out of initializers, nothing left to do but emit the actual code
		UnhideLocals(localMV)

		local cBlock = CodeBlock(cs, nil, block, block.localIndex)
		CompileCodeBlock(cs, ces, block, baseExpr.block)
		return
	end

	local exprNode = CompileExpression(cs, initExprs[exprStart], outerBlock, true)

	if exprNode.accessModes == nil then
		cerror(node, "ExpectedExpression", exprNode.type)
	end

	local exprSubMV = FlattenMV({
		type = "CMultipleValues",
		accessModes = clonetable(exprNode.accessModes),
		vTypes = clonetable(exprNode.vTypes),
		expressions = { exprNode },
	})

	local localSubMV =
	{
		type = "CMultipleValues",
		accessModes = { },
		expressions = { },
		vTypes = { },
	}

	local numValues = #exprNode.accessModes
	for i=1,numValues do
		local subLocal = i + localStart - 1
		local l = localMV.expressions[subLocal]
		if l == nil then
			cerror(baseExpr, "AssignRightSideTooFewValues")
		end

		localSubMV.expressions[i] = l
		localSubMV.accessModes[i] = localMV.accessModes[subLocal]
		localSubMV.vTypes[i] = localMV.vTypes[subLocal]
	end

	-- Assign to these locals
	EmitAssign(cs, localSubMV, exprSubMV, block, ces, baseExpr)
	ces:Discharge()

	local guaranteeOuterBlock = GuaranteeOuterBlock(cs, ces, nil, block, block.localIndex)
	local guaranteeInnerBlock = StartGuaranteeInnerBlock(cs, ces, guaranteeOuterBlock)

	-- Emit more assignments or the code
	EmitUsingDeclPortion(cs, baseExpr, localMV, initExprs, guaranteeInnerBlock, outerBlock, ces, localStart + numValues, exprStart + 1)

	CloseGuaranteeInnerBlock(cs, ces, guaranteeInnerBlock)

	local disposeBlock = CodeBlock(cs, nil, guaranteeOuterBlock, guaranteeOuterBlock.localIndex)

	do
		local emptyParameterListNode = VirtualParseNode("ExpressionList", baseExpr)
		emptyParameterListNode.expressions = { }

		local disposeNode = VirtualParseNode("Dispose", baseExpr)
		disposeNode.string = "Dispose"

		local disposeIndirectNode = VirtualParseNode("Indirect", baseExpr)
		disposeIndirectNode.operands = { false, disposeNode }

		local disposeInvoke = VirtualParseNode("Invoke", baseExpr)
		disposeInvoke.operands = { disposeIndirectNode, emptyParameterListNode }

		for i=1,numValues do
			-- Dispose of values in reverse order
			disposeIndirectNode.operands[1] = localMV.expressions[localStart + numValues - i]
			local disposeCompiled = CompileExpression(cs, disposeInvoke, guaranteeOuterBlock, true)
			EmitValues(cs, AdjustValueCount(cs, disposeCompiled, baseExpr, 0), ces)
			ces:Discharge()
		end
	end

	CloseGuaranteeOuterBlock(cs, ces, guaranteeOuterBlock)
end

local function EmitUsingDecl(cs, expr, block, ces)
	local rootBlock = CodeBlock(cs, nil, block, block.localIndex)

	-- No we're not emitting any actual expression-based code (only local decls), push a code location
	ces:PushCodeLocation(expr.filename, expr.line)

	-- Initializers can potentially fail, but disposal should only occur if the local was successfully assigned to
	local mvNode = EmitDeclsToLocals(cs, expr.declarations, rootBlock, ces)

	EmitUsingDeclPortion(cs, expr, mvNode, expr.initializers.expressions, rootBlock, block, ces, 1, 1)

	-- Close the root block
	CloseCodeBlock(cs, ces, rootBlock)
end

BlockMethod = function(block)
	while not block.isRootLevel do
		block = block.owner
		if block == nil then
			return nil
		end
	end
	return block.method
end

local function EmitReturn(cs, stmt, block, ces)
	local m = BlockMethod(block)
	local numRequiredReturnValues = #m.returnTypes.typeReferences
	local returnVTypes = { }

	if stmt.returnValues then
		local returnExpr = CompileExpressionList(cs, stmt.returnValues, block, true)
		returnExpr = AdjustValueCount(cs, returnExpr, stmt, numRequiredReturnValues)

		local returnTypes = { }
		for _,ref in ipairs(m.returnTypes.typeReferences) do
			returnTypes[#returnTypes+1] = ref.refType
		end

		local convertedReturns = ConvertExpression(cs, stmt, returnExpr, returnTypes, nil, false)
		EmitValues(cs, convertedReturns, ces, true)
	else
		if numRequiredReturnValues ~= 0 then
			cerror(stmt, "ReturnValueCountMismatch", tostring(numRequiredReturnValues), "0")
		end
	end

	local guarantee = FindGuaranteeingBlock(cs, block)

	if guarantee == nil then
		-- Just return out of frame
		ces:AddInstruction( { op = "return", int1 = numRequiredReturnValues } )
		ces:Pop(numRequiredReturnValues)
	else
		-- Store and return to the guarantee
		for i=1,numRequiredReturnValues do
			local rvHolder = guarantee.returnValueHolders[numRequiredReturnValues-i+1]
			ces:AddInstruction( { op = "localref", int1 = rvHolder.stackIndex } )
			ces:AddInstruction( { op = "move" } )
			ces:Pop(1)
		end
		ces:AddInstruction( { op = "jump", str1 = guarantee.returnPath } )
	end
end

EmitAliasableJump = function(cs, ces, baseBlock, label)
	local gBlock = baseBlock.owner
	while gBlock ~= nil and (not gBlock.isGuaranteeInner) and (not gBlock.isRootLevel) do
		gBlock = gBlock.owner
	end

	if gBlock == nil or not (gBlock.isGuaranteeInner) then
		ces:AddInstruction( { op = "jump", str1 = label } )
	else
		-- Has a guarantee
		local newLabel = AddLabelToGuarantee(cs, gBlock.owner, label)
		ces:AddInstruction( { op = "jump", str1 = newLabel } )
	end
end

local function EmitBlockJump(cs, labelType, blockNameTarget, block, ces, incriminate)
	local baseBlock = block
	while true do
		if block[labelType] and (blockNameTarget == nil or blockNameTarget == block.flowControlLabel) then
			EmitAliasableJump(cs, ces, baseBlock, block[labelType])
			return
		end

		if block.isRootLevel then
			cerror(incriminate, "UnresolvedFlowControlTarget")
		end

		block = block.owner
	end
end

local function EmitForEachLoopArray(cs, stmt, enumeratorExpr, block, ces)
	local arrayIndexST = cs.gst[arrayIndexType]
	if arrayIndexST == nil then
		throw(SIGNAL_UnresolvedExpression)
	end

	local iteratorBlock = CodeBlock(cs, nil, block, block.localIndex)
	-- Since we're not compiling any statement blocks for this code block, need to push a code location
	ces:PushCodeLocation(stmt.filename, stmt.line)

	local indexLocal = LocalVariable(BlockMethod(block), arrayIndexST)

	do
		local opcode, int1, int2, res1, str1 = RDXC.Native.encodeConstant("Core.largeuint", "-1", "Value")
		ces:AddInstruction( { op = opcode, res1 = res1, int1 = int1, int2 = int2, str1 = str1 } )
		ces:Push(1)
	end
	ces:AddInstruction( { op = "createlocal", res1 = arrayIndexType, str1 = "foreach index" } )
	ces:Pop()
	iteratorBlock:CreateLocal(ces, indexLocal)
	ces:Discharge()

	local iteratorVariablesMV = EmitDeclsToLocals(cs, stmt.declarations, iteratorBlock, ces)
	ces:Discharge()

	local extractLocal
	local simpleExtract
	local mdIndexLocals = { }

	local arrayInteriorType = enumeratorExpr.vTypes[1].containedType

	if #iteratorVariablesMV.vTypes == 1 and TypeDirectlyCastable(arrayInteriorType, iteratorVariablesMV.vTypes[1]) then
		extractLocal = iteratorVariablesMV.expressions[1]
		simpleExtract = true
	else
		extractLocal = LocalVariable(BlockMethod(block), arrayInteriorType)

		ces:AddInstruction( { op = "alloclocal", res1 = arrayInteriorType.longName, str1 = "foreach value" } )
		iteratorBlock:CreateLocal(ces, extractLocal)

		-- Determine if this is a multidimensional array
		local numDimensions = enumeratorExpr.vTypes[1].dimensions
		if numDimensions > 1 then
			for i=1,numDimensions do
				local lcl = LocalVariable(BlockMethod(block), arrayIndexST)
				if i == numDimensions then
					do
						local opcode, int1, int2, res1, str1 = RDXC.Native.encodeConstant("Core.largeint", "-1", "Value")
						ces:AddInstruction( { op = opcode, res1 = res1, int1 = int1, int2 = int2, str1 = str1 } )
						ces:Push(1)
					end
					ces:AddInstruction( { op = "createlocal", res1 = arrayIndexType, str1 = "foreach index "..(i-1) } )
					ces:Pop()
				else
					ces:AddInstruction( { op = "alloclocal", res1 = arrayIndexType, str1 = "foreach index "..(i-1) } )
				end
				iteratorBlock:CreateLocal(ces, lcl)
				mdIndexLocals[i] = lcl
			end
		end
	end
	ces:Discharge()

	local arrayLocal = LocalVariable(BlockMethod(block), enumeratorExpr.vTypes[1])
	ces:AddInstruction( { op = "alloclocal", res1 = enumeratorExpr.vTypes[1].longName, str1 = "foreach array" } )
	iteratorBlock:CreateLocal(ces, arrayLocal)
	ces:Discharge()

	-- Assign the array
	EmitAssign(cs, SingleExpressionToList(arrayLocal), SingleExpressionToList(enumeratorExpr), iteratorBlock, ces, stmt)
	ces:Discharge()

	-- Emit the actual loop
	local lbl = ces:CreateLabel()
	local iterationLabel = lbl.."_foriter"
	local endLabel = lbl.."_forend"

	-- Make the iterator locals available
	UnhideLocals(iteratorVariablesMV)

	local forMainBlock = CodeBlock(cs, nil, iteratorBlock, iteratorBlock.localIndex)
	forMainBlock.breakLabel = endLabel
	forMainBlock.continueLabel = iterationLabel
	forMainBlock.flowControlLabel = (stmt.label and stmt.label.string)

	ces:AddInstruction( { op = "label", str1 = iterationLabel } )
	ces:Discharge()

	if #mdIndexLocals ~= 0 then
		-- The op only requires a reference to the first local
		ces:AddInstruction( { op = "localref", int1 = mdIndexLocals[1].stackIndex } )
	end

	ces:AddInstruction( { op = "localref", int1 = extractLocal.stackIndex } )
	ces:AddInstruction( { op = "localref", int1 = indexLocal.stackIndex } )
	ces:AddInstruction( { op = "localref", int1 = arrayLocal.stackIndex } )


	ces:AddInstruction( { op = "iteratearray", str1 = endLabel, int2 = #mdIndexLocals } )

	if not simpleExtract then
		local exprList = EmptyExpressionList()
		AppendExpressionToMV(exprList, extractLocal)

		if #mdIndexLocals == 0 then
			AppendExpressionToMV(exprList, indexLocal)
		else
			for _,subIndexLocal in ipairs(mdIndexLocals) do
				AppendExpressionToMV(exprList, subIndexLocal)
			end
		end

		EmitAssign(cs, iteratorVariablesMV, exprList, forMainBlock, ces, stmt)
		ces:Discharge()
	end

	CompileCodeBlock(cs, ces, forMainBlock, stmt.block)
	ces:AddInstruction( { op = "jump", str1 = iterationLabel } )

	ces:AddInstruction( { op = "label", str1 = endLabel } )
	ces:Discharge()

	CloseCodeBlock(cs, ces, iteratorBlock)
end

local function EmitForEachLoopValue(cs, stmt, enumeratorExpr, block, ces)
	local getEnumeratorNode = VirtualParseNode("Name", stmt)
	getEnumeratorNode.string = "GetEnumerator"

	local indirectNode = VirtualParseNode("Indirect", stmt)
	indirectNode.operands = { enumeratorExpr, getEnumeratorNode }

	local emptyParameterListNode = VirtualParseNode("ExpressionList", stmt)
	emptyParameterListNode.expressions = { }

	local invokeNode = VirtualParseNode("Invoke", stmt)
	invokeNode.operands = { indirectNode, emptyParameterListNode } 

	local enumeratorExpr = AdjustValueCount(cs, CompileExpression(cs, invokeNode, block, true), stmt, 1)

	if enumeratorExpr.vTypes[1].type == "CArrayOfType" then
		-- GetEnumerator returns an array
		EmitForEachLoopArray(cs, stmt, enumeratorExpr, block, ces)
		return
	end

	-- GetEnumerator returns an enumerable of some sort
	local forMainBlock = CodeBlock(cs, nil, block, block.localIndex)

	-- Since we're not compiling any statement blocks for this code block, need to push a code location
	ces:PushCodeLocation(stmt.filename, stmt.line)

	local enumeratorLocal = LocalVariable(BlockMethod(block), enumeratorExpr.vTypes[1])
	ces:AddInstruction( { op = "alloclocal", res1 = enumeratorExpr.vTypes[1].longName, str1 = "foreach enumerator" } )
	forMainBlock:CreateLocal(ces, enumeratorLocal)
	ces:Discharge()

	local iteratorVariablesMV = EmitDeclsToLocals(cs, stmt.declarations, forMainBlock, ces)
	ces:Discharge()

	EmitAssign(cs, SingleExpressionToList(enumeratorLocal), SingleExpressionToList(enumeratorExpr), forMainBlock, ces, stmt)
	ces:Discharge()

	
	local lbl = ces:CreateLabel()
	local condLabel = lbl.."_forcond"
	local bodyLabel = lbl.."_forbody"
	local iterationLabel = lbl.."_foriter"
	local endLabel = lbl.."_forend"

	forMainBlock.breakLabel = endLabel
	forMainBlock.continueLabel = condLabel
	forMainBlock.flowControlLabel = (stmt.label and stmt.label.string)

	-- Emit the HasNext check
	local hasNextNode = VirtualParseNode("Name", stmt)
	hasNextNode.string = "HasNext"

	local hasNextIndirectNode = VirtualParseNode("Indirect", stmt)
	hasNextIndirectNode.operands = { enumeratorLocal, hasNextNode }

	local hasNextInvoke = VirtualParseNode("Invoke", stmt)
	hasNextInvoke.operands = { hasNextIndirectNode, emptyParameterListNode } 

	ces:AddInstruction( { op = "label", str1 = condLabel } )
	ces:Discharge()
	local cond = CompiledExprToBoolean(cs, stmt, CompileExpression(cs, hasNextInvoke, forMainBlock, true))
	EmitLogical(cs, cond, ces, iterationLabel, endLabel)
	ces:Discharge()

	-- Emit the GetNext call and assign
	local getNextNode = VirtualParseNode("Name", stmt)
	getNextNode.string = "GetNext"

	local getNextIndirectNode = VirtualParseNode("Indirect", stmt)
	getNextIndirectNode.operands = { enumeratorLocal, getNextNode }

	local getNextInvoke = VirtualParseNode("Invoke", stmt)
	getNextInvoke.operands = { getNextIndirectNode, emptyParameterListNode } 

	local getNextExpr = CompileExpression(cs, getNextInvoke, forMainBlock, true)

	ces:AddInstruction( { op = "label", str1 = iterationLabel } )
	ces:Discharge()

	local truncatedGetNextExpr = AdjustValueCount(cs, SingleExpressionToList(getNextExpr), stmt, #iteratorVariablesMV.vTypes)
	EmitAssign(cs, iteratorVariablesMV, truncatedGetNextExpr, forMainBlock, ces, stmt)

	ces:AddInstruction( { op = "label", str1 = bodyLabel } )
	ces:Discharge()

	-- Emit the actual code
	UnhideLocals(iteratorVariablesMV)

	local forBodyBlock = CodeBlock(cs, nil, forMainBlock, forMainBlock.localIndex)
	CompileCodeBlock(cs, ces, forBodyBlock, stmt.block)

	ces:AddInstruction( { op = "jump", str1 = condLabel } )
	ces:AddInstruction( { op = "label", str1 = endLabel } )
	ces:Discharge()

	CloseCodeBlock(cs, ces, forMainBlock)
end

local function EmitForEachLoop(cs, stmt, block, ces)
	-- stmt.declarations, stmt.iterator, stmt.block...
	local enumeratorExpr = AdjustValueCount(cs, CompileExpression(cs, stmt.iterator, block, true), stmt, 1)

	if enumeratorExpr.vTypes[1].type == "CArrayOfType" then
		EmitForEachLoopArray(cs, stmt, enumeratorExpr, block, ces)
	else
		EmitForEachLoopValue(cs, stmt, enumeratorExpr, block, ces)
	end
end


local function TypeIsException(st)
	if st.type ~= "CStructuredType" then
		return false
	end
	while st ~= nil do
		if st.longName == "Core.Exception" then
			return true;
		end
		st = st.parentType
	end
	return false
end


local function CompileStatement(cs, stmt, block, ces)
	if stmt.type == "WhileLoop" then
		local cond = CompiledExprToBoolean(cs, stmt, CompileExpression(cs, stmt.condition, block, true))

		local lbl = ces:CreateLabel()
		local whileCondLabel = lbl.."_whilecond"
		local whileTrueLabel = lbl.."_whiletrue"
		local whileEndLabel = lbl.."_whileend"

		ces:AddInstruction( { op = "label", str1 = whileCondLabel } )
		ces:Discharge()

		EmitLogical(cs, cond, ces, whileTrueLabel, whileEndLabel)
		ces:Discharge()

		ces:AddInstruction( { op = "label", str1 = whileTrueLabel } )
		ces:Discharge()
		local whileBlock = CodeBlock(cs, nil, block, block.localIndex)
		whileBlock.breakLabel = whileEndLabel
		whileBlock.continueLabel = whileCondLabel
		whileBlock.flowControlLabel = (stmt.label and stmt.label.string)

		CompileCodeBlock(cs, ces, whileBlock, stmt.block)
		ces:AddInstruction( { op = "jump", str1 = whileCondLabel } )
		ces:AddInstruction( { op = "label", str1 = whileEndLabel } )
		ces:Discharge()
	elseif stmt.type == "DoLoop" then
		local lbl = ces:CreateLabel()
		local whileTrueLabel = lbl.."_whiletrue"
		local whileEndLabel = lbl.."_whileend"

		ces:AddInstruction( { op = "label", str1 = whileTrueLabel } )
		ces:Discharge()
		local whileBlock = CodeBlock(cs, nil, block, block.localIndex)
		whileBlock.breakLabel = whileEndLabel
		whileBlock.continueLabel = whileTrueLabel
		whileBlock.flowControlLabel = (stmt.label and stmt.label.string)

		CompileCodeBlock(cs, ces, whileBlock, stmt.block)

		if stmt.condition then
			local cond = CompiledExprToBoolean(cs, stmt, CompileExpression(cs, stmt.condition, block, true))
			EmitLogical(cs, cond, ces, whileTrueLabel, whileEndLabel)
			ces:Discharge()
		end

		ces:AddInstruction( { op = "label", str1 = whileEndLabel } )
		ces:Discharge()
	elseif stmt.type == "IfBlock" then
		local lbl = ces:CreateLabel()
		local ifTrueBlock = lbl.."_iftrue"
		local ifFalseBlock = lbl.."_iffalse"
		local ifEndBlock = lbl.."_ifend"

		local cond = CompiledExprToBoolean(cs, stmt, CompileExpression(cs, stmt.condition, block, true))

		-- Evaluate
		EmitLogical(cs, cond, ces, ifTrueBlock, ifFalseBlock)

		-- Discharge conditional temporaries
		ces:Discharge()

		-- Emit true statements
		ces:AddInstruction( { op = "label", str1 = ifTrueBlock } )
		ces:Discharge()
		local trueBlock = CodeBlock(cs, nil, block, block.localIndex)
		CompileCodeBlock(cs, ces, trueBlock, stmt.block)

		ces:AddInstruction( { op = "jump", str1 = ifEndBlock } )
		ces:AddInstruction( { op = "label", str1 = ifFalseBlock } )
		ces:Discharge()

		-- Emit false statements
		if stmt.elseBlock then
			local falseBlock = CodeBlock(cs, nil, block, block.localIndex)
			CompileCodeBlock(cs, ces, falseBlock, stmt.elseBlock)
		end
		ces:AddInstruction( { op = "label", str1 = ifEndBlock } )
		ces:Discharge()
	elseif stmt.type == "SwitchBlock" then
		if #stmt.cases == 0 then
			cerror(stmt, "SwitchWithNoCases")
		end

		-- Reduce to one value
		local valueExpr = AdjustValueCount(cs, CompileExpression(cs, stmt.value, block, true), stmt, 1)
		local valueVType = valueExpr.vTypes[1]

		-- Convert the value expression to a pointer or value
		if VTypeIsRefStruct(valueExpr.vTypes[1]) then
			valueExpr = ConvertExpression(cs, stmt, valueExpr, { valueVType }, { "CP" }, false)
		else
			valueExpr = ConvertExpression(cs, stmt, valueExpr, { valueVType }, { "R" }, false)
		end

		local defaultBlock
		local valueBlocks = { }

		local defaultInstanceDef = "{\n"

		local switchType = valueVType
		
		local objectST = cs.gst["Core.Object"]
		if objectST == nil then
			throw(SIGNAL_UnresolvedExpression)
		end

		if valueVType.longName == "Core.nullreference" then
			switchType = objectST
		end

		-- Try each value
		for blockIndex,caseNode in ipairs(stmt.cases) do
			for _,expr in ipairs(caseNode.expressions) do
				if expr.type == "DefaultCase" then
					if defaultBlock ~= nil then
						cerror(caseNode, "MultipleSwitchDefaults")
					end
					defaultBlock = blockIndex
				else
					valueBlocks[#valueBlocks+1] = blockIndex

					local valueExpr = CompileExpression(cs, expr, block, true)
					local exprAdjusted = AdjustValueCount(cs, valueExpr, caseNode, 1)
					local exprConverted

					local matchability = CastMatchability(exprAdjusted.vTypes[1], valueVType)
					if matchability == matchLevels.POLYMORPHIC then
						switchType = objectST
						exprConverted = exprAdjusted
					elseif matchability == matchLevels.DIRECT and exprAdjusted.accessModes[1] == "R" then
						exprConverted = exprAdjusted
					else
						exprConverted = ConvertExpression(cs, caseNode, exprAdjusted, { valueVType }, { "R" }, false)
					end

					local initValue
					if exprConverted.type ~= "CConstant" then
						if VTypeIsObjectReference(valueVType) and exprConverted.type == "CStaticInstance" then
							initValue = exprConverted.longName
						else
							cerror(caseNode, "NonConstantSwitchCase")
						end
					else
						-- Convert to an initialization value
						initValue = GetInitializationValue(cs, valueVType, exprConverted, false, 1, "!ERR_", { }, { }, false, caseNode)
					end


					defaultInstanceDef = defaultInstanceDef..initValue..",\n"
				end
			end
		end

		defaultInstanceDef = defaultInstanceDef.."}\n"

		if #valueBlocks == 0 then
			cerror(stmt, "SwitchCaseWithNoCases")
		end

		numCaseCollections = numCaseCollections + 1
		local caseInstanceName = BlockMethod(block).longName.."/cases_"..numCaseCollections

		-- Find or create an array of the values
		local arrayCode = ArrayOfTypeCode(1, true)
		local aot = switchType.arraysOf[arrayCode]

		if aot == nil then
			-- Create and compile it
			aot = ArrayOfType(cs, switchType, 1, true, stmt)
			switchType.arraysOf[arrayCode] = aot

			CompileTypeShells(cs)
			CompileArrayOfType(cs, aot)
		end

		-- Create the default instance
		DefaultInstance(cs, aot, defaultInstanceDef, caseInstanceName, { #valueBlocks }, false)

		local lbl = ces:CreateLabel()
		local caseLabel = lbl.."_case_"
		local endLabel = lbl.."_switchend"

		-- Emit the switch lookup
		EmitValues(cs, valueExpr, ces)
		ces:AddInstruction( { op = "switch", int1 = #valueBlocks, res1 = caseInstanceName } )
		ces:Pop()

		for idx,blockIdx in ipairs(valueBlocks) do
			ces:AddInstruction( { op = "case", str1 = caseLabel..blockIdx } )
		end

		-- Emit the default case jump
		if defaultBlock ~= nil then
			ces:AddInstruction( { op = "jump", str1 = caseLabel..defaultBlock } )
		else
			ces:AddInstruction( { op = "jump", str1 = endLabel } )
		end
		ces:Discharge()

		-- Emit each case
		for caseIndex,caseBlockNode in ipairs(stmt.cases) do
			ces:AddInstruction( { op = "label", str1 = caseLabel..caseIndex } )
			ces:Discharge()
			local caseBlock = CodeBlock(cs, nil, block, block.localIndex)
			caseBlock.breakLabel = endLabel
			caseBlock.flowControlLabel = (caseBlockNode.label and caseBlockNode.label.string)

			CompileCodeBlock(cs, ces, caseBlock, caseBlockNode.block)
			ces:AddInstruction( { op = "jump", str1 = endLabel } )
			ces:Discharge()
		end
		-- Finish
		ces:AddInstruction( { op = "label", str1 = endLabel } )
		ces:Discharge()
	elseif stmt.type == "ForLoop" then
		local lbl = ces:CreateLabel()
		local condLabel = lbl.."_forcond"
		local iterationLabel = lbl.."_foriter"
		local bodyLabel = lbl.."_forbody"
		local endLabel = lbl.."_forend"

		local forMainBlock = CodeBlock(cs, nil, block, block.localIndex)
		forMainBlock.breakLabel = endLabel
		forMainBlock.continueLabel = iterationLabel
		forMainBlock.flowControlLabel = (stmt.label and stmt.label.string)

		CompileCodeBlock(cs, ces, forMainBlock, stmt.initial, true)	-- Hold open


		-- Skip iteration initially
		ces:AddInstruction( { op = "jump", str1 = condLabel } )

		ces:AddInstruction( { op = "label", str1 = iterationLabel } )
		ces:Discharge()
		local forIterBlock = CodeBlock(cs, nil, forMainBlock, forMainBlock.localIndex)
		CompileCodeBlock(cs, ces, forIterBlock, stmt.iteration)

		ces:AddInstruction( { op = "label", str1 = condLabel } )
		ces:Discharge()

		if stmt.condition then
			local cond = CompiledExprToBoolean(cs, stmt, CompileExpression(cs, stmt.condition, forMainBlock, true))
			EmitLogical(cs, cond, ces, bodyLabel, endLabel)
		end
		ces:Discharge()

		ces:AddInstruction( { op = "label", str1 = bodyLabel } )
		ces:Discharge()
		local forBodyBlock = CodeBlock(cs, nil, forMainBlock, forMainBlock.localIndex)
		CompileCodeBlock(cs, ces, forBodyBlock, stmt.block)
		ces:AddInstruction( { op = "jump", str1 = iterationLabel } )

		ces:AddInstruction( { op = "label", str1 = endLabel } )
		ces:Discharge()

		-- Close and release anything from the initializer block
		CloseCodeBlock(cs, ces, forMainBlock)
	elseif stmt.type == "ForEachLoop" then
		EmitForEachLoop(cs, stmt, block, ces)
	elseif stmt.type == "LocalDecl" then
		EmitLocalDecl(cs, stmt, block, ces)
	elseif stmt.type == "UsingDecl" then
		EmitUsingDecl(cs, stmt, block, ces)
	elseif stmt.type == "Return" then
		EmitReturn(cs, stmt, block, ces)
	elseif stmt.type == "Continue" then
		EmitBlockJump(cs, "continueLabel", stmt.label and stmt.label.string, block, ces, stmt)
	elseif stmt.type == "Break" then
		EmitBlockJump(cs, "breakLabel", stmt.label and stmt.label.string, block, ces, stmt)
	elseif stmt.type == "CodeBlock" then
		local cb = CodeBlock(cs, nil, block, block.localIndex)
		CompileCodeBlock(cs, ces, cb, stmt)
	elseif stmt.type == "Assign" then
		local left = CompileExpressionList(cs, stmt.destinations, block, false)
		local right = CompileExpressionList(cs, stmt.sources, block, true)

		right = AdjustValueCount(cs, right, stmt, #left.vTypes)

		EmitAssign(cs, left, right, block, ces, stmt)
	elseif stmt.type == "IncrementalOperate" then
		local right = Constant(cs, "Core.byte", "1", "Value")
		EmitOperateAndAssign(cs, stmt.destination, right, stmt.operator, block, ces, stmt)
	elseif stmt.type == "OperateAndAssign" then
		local right = CompileExpression(cs, stmt.source, block, true)

		EmitOperateAndAssign(cs, stmt.destination, right, stmt.operator, block, ces, stmt)
	elseif stmt.type == "ExpressionList" then
		local expr = CompileExpressionList(cs, stmt, block, true)
		for _,am in ipairs(expr.accessModes) do
			if am ~= "R" and am ~= "CP" then
				cerror(stmt, "ExpressionStatementIsVariable")
			end
		end

		EmitValues(cs, AdjustValueCount(cs, expr, stmt, 0, true), ces)
	elseif stmt.type == "Throw" then
		local expr = CompileExpression(cs, stmt.expression, block, true)
		expr = AdjustValueCount(cs, expr, stmt, 1)

		local exceptionType = cs.gst["Core.Exception"]
		if not exceptionType then
			throw(SIGNAL_UnresolvedExpression)
		end
		expr = ConvertExpression(cs, stmt, expr, { exceptionType }, { "R" }, false )

		EmitValues(cs, expr, ces)
		ces:AddInstruction( { op = "throw" } )
		ces:Pop()
	elseif stmt.type == "TryBlock" then
		local lbl = ces:CreateLabel()
		local catchLabel = lbl.."_catch"
		local endLabel = lbl.."_tryend"
		local finishLabel = lbl.."_tryfinish"
		local guaranteeOuterBlock
		local guaranteeInnerBlock
		local tryCatchExternalBlock = block
		
		if not stmt.finallyBlock and not stmt.catchBlocks then
			cerror(stmt, "NoCatchOrFinally")
		end

		if stmt.finallyBlock then
			guaranteeOuterBlock = GuaranteeOuterBlock(cs, ces, nil, block, block.localIndex)
			guaranteeInnerBlock = StartGuaranteeInnerBlock(cs, ces, guaranteeOuterBlock)
			tryCatchExternalBlock = guaranteeInnerBlock
		end

		if stmt.catchBlocks then
			ces:AddInstruction( { op = "try", str1 = catchLabel, str2 = endLabel } )
			ces:Discharge()
		end

		local tryBlock = CodeBlock(cs, nil, tryCatchExternalBlock, tryCatchExternalBlock.localIndex)
		CompileCodeBlock(cs, ces, tryBlock, stmt.block)

		if stmt.catchBlocks then
			ces:AddInstruction( { op = "jump", str1 = finishLabel } )
			ces:AddInstruction( { op = "label", str1 = endLabel } )
			ces:Discharge()

			local catchTypes = { }
			local catchTypeLabels = { }
			for cidx,blk in ipairs(stmt.catchBlocks.catchBlocks) do
				local thisCatchLabel = catchLabel.."_"..cidx
				local catchBlock = CodeBlock(cs, nil, tryCatchExternalBlock, tryCatchExternalBlock.localIndex)
				local catchingType
				local catchParam = nil
				
				local guaranteeBlock

				if blk.exceptionType then
					local tr = TypeReference(cs, blk.exceptionType, tryCatchExternalBlock)
					CompileTypeReference(cs, tr)
					local exceptType = tr.refType
					catchingType = exceptType
					if not tr.isCompiled or not TypeIsException(exceptType) then
						cerror(blk, "ThrowNonException")
					end

					catchParam = LocalVariable(BlockMethod(tryCatchExternalBlock), exceptType)
					catchBlock:CreateLocal(ces, catchParam, blk.exceptionName)
					ces:AddInstruction( { op = "label", str1 = thisCatchLabel } )
					ces:AddInstruction( { op = "catch", res1 = catchingType.longName } )
					ces:AddInstruction( { op = "createlocal", res1 = catchingType.longName, str1 = blk.exceptionName.string } )
				else
					catchingType = cs.gst["Core.Exception"]
					assert(catchingType)
					ces:AddInstruction( { op = "label", str1 = thisCatchLabel } )
					ces:AddInstruction( { op = "catch", res1 = "Core.Exception" } )
					ces:AddInstruction( { op = "pop" } )
				end
				ces:Discharge()

				if catchTypeLabels[catchingType] then
					cerror(blk, "DuplicateCatch")
				end
				catchTypeLabels[catchingType] = thisCatchLabel
				catchTypes[cidx] = catchingType

				CompileCodeBlock(cs, ces, catchBlock, blk.block)
				ces:AddInstruction( { op = "jump", str1 = finishLabel } )

				ces:Discharge()
			end

			ces:AddInstruction( { op = "label", str1 = catchLabel } )
			ces:AddInstruction( { op = "catch", res1 = "Core.Exception" } )

			assert(#catchTypes == #stmt.catchBlocks.catchBlocks)
			for _,ct in ipairs(catchTypes) do
				ces:AddInstruction( { op = "trycatch", str1 = catchTypeLabels[ct], res1 = ct.longName } )
			end
			ces:AddInstruction( { op = "throw" } )
			ces:AddInstruction( { op = "label", str1 = finishLabel } )
			ces:Discharge()
		end
		
		if stmt.finallyBlock then
			CloseGuaranteeInnerBlock(cs, ces, guaranteeInnerBlock)

			local finallyBlock = CodeBlock(cs, nil, guaranteeOuterBlock, guaranteeOuterBlock.localIndex)
			CompileCodeBlock(cs, ces, finallyBlock, stmt.finallyBlock)
			CloseGuaranteeOuterBlock(cs, ces, guaranteeOuterBlock)
		end
	elseif stmt.type == "MemberDecl" and structuredTypeDeclTypes[stmt.declType.string] then
		local method = BlockMethod(block)
		method.numPrivateTypes = method.numPrivateTypes + 1
		local privateTypeName = method.longName.."/privatetype"..(method.numPrivateTypes)
		local typeNamespace = Namespace(nil, privateTypeName)
		typeNamespace.privateTypeName = privateTypeName

		local uncompiledType = StructuredType(cs, Namespace(typeNamespace, stmt.name.string), block, stmt.attribTags, stmt)
		block:InsertUnique(stmt.name, uncompiledType)
		uncompiledType.namespace.createdBy = uncompiledType

		cs.uncompiled[#cs.uncompiled+1] = uncompiledType
		CompileTypeShells(cs)
		uncompiledType.isPrivateType = true
	else
		error("Unemittable statement type "..stmt.type)
	end

	ces:Discharge()
end

CloseCodeBlock = function(cs, ces, block)
	assert(block.closed == nil, "Block was already closed")
	for i=1,#block.locals do
		ces:AddInstruction( { op = "removelocal" } )
		ces.localCount = ces.localCount - 1
	end
	block.closed = true
	ces:Discharge()
	ces:PopCodeLocation()
end

CompileCodeBlock = function(cs, ces, block, blockNode, holdOpen)
	assert(ces)

	ces:PushCodeLocation(blockNode.filename, blockNode.line)

	for _,stmt in ipairs(blockNode.statements) do
		ces:PushCodeLocation(stmt.filename, stmt.line)
		CompileStatement(cs, stmt, block, ces)
		ces:PopCodeLocation()
		assert(ces.opstackIndex == 0)
	end

	if not holdOpen then
		CloseCodeBlock(cs, ces, block)
	end
end

local function CodeEmissionState()
	local ces = {
		type = "CCodeEmissionState",
		statements = { },
		temporaries = { },
		instructions = { },
		pendingInstructions = { },
		opstackTempRefs = { },

		codeLocation = nil,
		localCount = 0,
		opstackIndex = 0,
		labelCount = 0,

		Push = function(self, count, tempRef)
			self.opstackIndex = self.opstackIndex + (count or 1)

			if tempRef ~= nil and tempRef.isTemporary then
				self.opstackTempRefs[self.opstackIndex] = tempRef
				tempRef.numTempReferences = tempRef.numTempReferences + 1
			end

			return self.opstackIndex
		end,

		Clone = function(self, start, count)
			if count == 0 then
				return
			end
			while count ~= nil and count > 1 do
				self:Clone(start, 1)
				count = count - 1
			end

			self.opstackIndex = self.opstackIndex + 1

			local migratedTemp = self.opstackTempRefs[self.opstackIndex - start - 1]
			if migratedTemp ~= nil and migratedTemp.numTempReferences ~= nil then
				self.opstackTempRefs[self.opstackIndex] = migratedTemp
				migratedTemp.numTempReferences = migratedTemp.numTempReferences + 1
			end
			return self.opstackIndex
		end,

		Pop = function(self, count)
			if count == 0 then
				return
			end

			while count ~= nil and count > 1 do
				self:Pop(1)
				count = count - 1
			end

			local tempRef = self.opstackTempRefs[self.opstackIndex]

			if tempRef ~= nil then
				self.opstackTempRefs[self.opstackIndex] = nil
				local numTempReferences = tempRef.numTempReferences - 1
				tempRef.numTempReferences = numTempReferences
				if numTempReferences == 0 and not tempRef.holdOpenTemp then
					tempRef.tempActive = false
				end
			end

			self.opstackIndex = self.opstackIndex - 1
			return self.opstackIndex
		end,

		AddTemporary = function(self, st)
			-- See if an existing temporary can be recycled
			for _,temp in ipairs(self.temporaries) do
				if temp.vTypes[1] == st and temp.tempActive == false then
					temp.tempActive = true
					return temp
				end
			end

			assert(st.longName ~= "Core.nullreference")

			local lv = LocalVariable(nil, st, false, false, "temp")
			lv.stackIndex = self.localCount
			self.localCount = self.localCount + 1
			self.temporaries[#self.temporaries+1] = lv

			lv.isTemporary = true
			lv.numTempReferences = 0
			lv.tempActive = true

			return lv
		end,

		Discharge = function(self)
			for _,temp in ipairs(self.temporaries) do
				self.instructions[#self.instructions+1] = { op = "alloclocal", res1 = temp.vTypes[1].longName, str1 = "compiler temp", filename = self.codeLocation.filename, line = self.codeLocation.line }
			end

			for _,instr in ipairs(self.pendingInstructions) do
				self.instructions[#self.instructions+1] = instr
			end
			self.pendingInstructions = { }

			for _,temp in ipairs(self.temporaries) do
				assert(temp.tempActive == false, "Internal error: Temporary was not properly disposed of")
				self.instructions[#self.instructions+1] = { op = "removelocal", filename = self.codeLocation.filename, line = self.codeLocation.line }
				self.localCount = self.localCount - 1
			end
			self.temporaries = { }
		end,

		AddInstruction = function(self, instruction)
			instruction.opstackIndex = self.opstackIndex
			if self.codeLocation then
				assert(self.codeLocation.filename)
				instruction.filename = self.codeLocation.filename
				instruction.line = self.codeLocation.line
			else
				assert(false, "No code location!")
			end
			self.pendingInstructions[#self.pendingInstructions+1] = instruction
		end,

		AddConstantInstruction = function(self, typeName, constant, signal)
			local opcode, int1, int2, res1, str1 = RDXC.Native.encodeConstant(typeName, constant, signal)

			return self:AddInstruction( { op = opcode, res1 = res1, int1 = int1, int2 = int2, str1 = str1 } )
		end,

		PopCodeLocation = function(self, filename, line)
			self.codeLocation = self.codeLocation.previous
		end,

		PushCodeLocation = function(self, filename, line)
			assert(filename)
			assert(line)
			self.codeLocation = { previous = self.codeLocation, filename = filename, line = line }
		end,

		AppendEmissionState = function(self, ces)
			local maxIdx = #self.instructions
			for idx,instr in ipairs(ces.instructions) do
				self.instructions[maxIdx+idx] = instr
			end
		end,

		CreateLabel = function(self)
			self.labelCount = self.labelCount + 1
			return "label"..self.labelCount
		end,
	}

	return ces
end


DumpCompiledInstructions = function(instrList)
	--if true then return end
	for iNum,instr in ipairs(instrList) do
		local opDump = iNum..": "..instr.op
		if instr.instructionNumber ~= nil then
			opDump = opDump.."  i "..(instr.instructionNumber)
		end
		if instr.reachable ~= nil then
			opDump = opDump.."  reachable: "..tostring(instr.reachable)
		end
		if instr.str1 then
			opDump = opDump.."  str1: "..instr.str1
		end
		if instr.str2 then
			opDump = opDump.."  str2: "..instr.str2
		end
		if instr.int1 then
			opDump = opDump.."  int1: "..instr.int1
		end
		if instr.int2 then
			opDump = opDump.."  int2: "..instr.int2
		end
		if instr.res1 then
			opDump = opDump.."  res1: "..instr.res1
		end
		print(opDump)
	end
end

-- Tags instructions with real instruction numbers and barrier depths
local function TagInstructions(instrs, labels)
	local instrCount = 0
	local barrierCount = 0
	for _,instr in ipairs(instrs) do
		if instr.op == "startbarrier" then
			barrierCount = barrierCount + 1
		elseif instr.op == "endbarrier" then
			barrierCount = barrierCount - 1
		end

		if instr.op == "label" then
			labels[instr.str1] = instrCount + 1
		end
		
		if not nonInstructionOpcodes[instr.op] then
			instrCount = instrCount + 1
			instr.instructionNumber = instrCount
			instr.barrierCount = barrierCount
		end
	end
end

-- This remaps any jumps that target other jumps so that they point to the same place.
local function RelinkJumps(instrList)
	local activeLabels = { }	-- List of labels targeting the current instruction
	local remappedLabels = { }
	local finalRemappedLabels = { }
	local labelNames = { }		-- Kept so that this is deterministic

	for idx,instr in ipairs(instrList) do
		if instr.op == "label" then
			labelNames[#labelNames+1] = instr.str1
			activeLabels[#activeLabels+1] = instr.str1
		else
			if instr.op == "jump" then
				for _,labelName in ipairs(activeLabels) do
					remappedLabels[labelName] = instr.str1
				end
			end
			if #activeLabels ~= 0 then
				activeLabels = { }
			end
		end
	end

	for _,labelName in ipairs(labelNames) do
		if remappedLabels[labelName] then
			for k,v in pairs(remappedLabels) do
				-- If another remap targets this label, reroute it
				if v == labelName then
					remappedLabels[k] = remappedLabels[labelName]
				end
			end
			finalRemappedLabels[labelName] = remappedLabels[labelName]
			remappedLabels[labelName] = nil
		end
	end

	-- Remap everything
	for _,instr in ipairs(instrList) do
		if instr.op == "jump" or optionalJumpOpcodes[instr.op] then
			if finalRemappedLabels[instr.str1] then
				instr.str1 = finalRemappedLabels[instr.str1]
			end
			if instr.str2 and finalRemappedLabels[instr.str2] then
				instr.str2 = finalRemappedLabels[instr.str2]
			end
		end
	end
end

local function PaintReachable(instrList, idx, labels)
	-- These labels are by raw instruction number, not the assembly instruction number, so they need to be rebuilt
	if labels == nil then
		labels = { }

		for idx,instr in ipairs(instrList) do
			if instr.op == "label" then
				labels[instr.str1] = idx
			elseif instr.op == "alloclocal" or instr.op == "removelocal" then
				instr.reachable = true	-- Local manipulation ops are always relevant
			end
		end
	end

	while instrList[idx] do
		local instr = instrList[idx]
		local wasTagged = instr.reachable

		instr.reachable = true

		if optionalJumpOpcodes[instr.op] then
			if wasTagged then
				return
			end
			PaintReachable(instrList, labels[instr.str1], labels)
			if instr.str2 and splittingJumpOpcodes[instr.op] then
				-- Instruction has a false label, so fall-through isn't reachable
				PaintReachable(instrList, labels[instr.str2], labels)
				return
			end
		end

		if instr.op == "return" then
			while instrList[idx] and instrList[idx].op ~= "endbarrier" do
				idx = idx + 1
			end
		elseif instr.op == "throw" or instr.op == "deadcode" then
			return
		elseif instr.op == "jump" then
			if wasTagged then
				return
			end
			idx = labels[instr.str1]
		else
			idx = idx + 1
		end
	end
end

local function CorrectBranches(instrList)
	local labels = { }

	local jumpNegations = {
		["jumpif"] = "jumpifnot",
		["jumpifnot"] = "jumpif",
		["jumpiftrue"] = "jumpiffalse",
		["jumpiffalse"] = "jumpiftrue",
		["jumpifnull"] = "jumpifnotnull",
		["jumpifnotnull"] = "jumpifnull",
		["jumpifequal"] = "jumpifnotequal",
		["jumpifnotequal"] = "jumpifequal",
		["jump"] = "jump",
		["iteratearray"] = "iteratearray",
	}

	-- Consolidate labels
	RelinkJumps(instrList)

	-- Remove dead code
	do
		local lastReachablePaths
		local firstPathReachPass = true
		while true do
			local enabledPaths = { }
			local numReachablePaths = 0
			for _,instr in ipairs(instrList) do
				if instr.op == "enablepath" and (firstPathReachPass or instr.reachable) then
					local pathName = instr.str1
					if not enabledPaths[pathName] then
						enabledPaths[pathName] = true
						numReachablePaths = numReachablePaths + 1
					end
				end
			end
			
			if firstPathReachPass == false then
				if numReachablePaths == lastReachablePaths then
					-- Didn't change the valid pass list, we're done
					break
				end
			end

			-- Convert path usage checks that didn't survive jumps, which will skip over code paths
			-- that got optimized out from dead code elimination elsewhere
			for _,instr in ipairs(instrList) do
				if instr.op == "checkpathusage" and not enabledPaths[instr.str2] then
					instr.op = "jump"
					instr.str2 = nil
				end
			end

			firstPathReachPass = false
			lastReachablePaths = numReachablePaths

			PaintReachable(instrList, 1)
			
			if numReachablePaths == 0 then
				break	-- All guaranteed code paths were eliminated
			end
		end
	end
	
	TagInstructions(instrList, labels)

	local newInstrList = { }
	for _,instr in ipairs(instrList) do
		local op = instr.op
		if instr.reachable then
			newInstrList[#newInstrList+1] = instr
		elseif instr.op == "label" then
			newInstrList[#newInstrList+1] = instr	-- Labels need to stay alive to prevent span labels (i.e. exception handlers) from ending at orphan labels
		elseif op == "deadcode" or op == "enablepath" then
			-- Pseudo-ops, not actually emitted
		elseif op == "createlocal" then
			instr.op = "alloclocal"		-- Local tracking needs to stay live.  Other local manipulation ops are automatically reachable.
			newInstrList[#newInstrList+1] = instr
		end
	end

	instrList = newInstrList

	-- Delete any useless jumps and locals
	while true do
		newInstrList = { }
		local deletedJumps = false
		local deletedCode = false

		TagInstructions(instrList, labels)

		local numInstr = #instrList
		for idx,instr in ipairs(instrList) do
			local op = instr.op
			if op == "jump" and (labels[instr.str1] == instr.instructionNumber + 1) then
				deletedJumps = true
			elseif op == "alloclocal" and idx ~= numInstr and instrList[idx+1].op == "removelocal" then
				deletedCode = true
			elseif op == "removelocal" and idx ~= 1 and instrList[idx-1].op == "alloclocal" then
				deletedCode = true
			else
				newInstrList[#newInstrList+1] = instr
			end
		end

		instrList = newInstrList

		if (not deletedJumps) and (not deletedCode) then
			break
		end
	end

	labels = { }
	TagInstructions(instrList, labels)

	-- Split jumps into single jump and fallthrough
	newInstrList = { }
	for _,instr in ipairs(instrList) do
		if jumpNegations[instr.op] and instr.str2 then
			assert(instr.str1 ~= instr.str2)

			local trueInstr = labels[instr.str1]
			local nextInstr = instr.instructionNumber + 1
			local falseLabel

			if trueInstr == nextInstr then
				instr.op = jumpNegations[instr.op]
				falseLabel = instr.str1
				instr.str1 = instr.str2
			else
				falseLabel = instr.str2
			end

			instr.str2 = nil

			newInstrList[#newInstrList+1] = instr

			if labels[falseLabel] ~= nextInstr then
				newInstrList[#newInstrList+1] = { op = "jump", str1 = falseLabel, filename = instr.filename, line = instr.line }
			end
		else
			newInstrList[#newInstrList+1] = instr
		end
	end

	instrList = newInstrList

	TagInstructions(instrList, labels)

	newInstrList = { }
	for _,instr in ipairs(instrList) do
		local op = instr.op
		-- Convert cases to jumps now that they can't block execution flow
		if op == "case" then
			instr.op = "jump"
			instr.doNotChangeOps = true
		end

		if nonInstructionOpcodes[op] then
			-- Unemitted instructions
		else
			if jumpNegations[instr.op] or instr.op == "trycatch" then
				instr.int1 = labels[instr.str1] - instr.instructionNumber
				instr.str1 = nil
			elseif instr.op == "try" then
				instr.int1 = labels[instr.str1] - instr.instructionNumber
				instr.int2 = labels[instr.str2] - instr.instructionNumber
				instr.str1 = nil
				instr.str2 = nil
			end
			newInstrList[#newInstrList+1] = instr
		end
	end


	-- Remap code locations
	for i=#newInstrList,2,-1 do
		local instr = newInstrList[i]
		local prevInstr = newInstrList[i-1]

		assert(instr.filename and instr.line)

		if instr.filename == prevInstr.filename then
			instr.filename = nil
		end
		if instr.line == prevInstr.line then
			instr.line = nil
		end
	end

	return newInstrList
end

-- Remaps jumps to return instructions to return, used for methods that return no values.  Assists with tail call optimization.
local function LinkReturnJumps(instrList)
	for idx,instr in ipairs(instrList) do
		if instr.op == "jump" and (not instr.doNotChangeOps) and instrList[idx + instr.int1].op == "return" then
			instr.op = "return"
			instr.int1 = 0
		end
	end
end

local function CompileMethodCode(cs, m, cacheState)
	local ces = CodeEmissionState()

	local codeBlockNode = cacheState:Decache(m.codeBlock)

	local memberLookupScope = m.definedByType.internalScope
	local thisLocal

	if not m.isStatic then
		memberLookupScope = MemberLookupScope(cs, memberLookupScope, m.definedByType)
	end

	local codeBlock = CodeBlock(cs, nil, memberLookupScope, 0)
	codeBlock.isRootLevel = true
	codeBlock.method = m

	-- Create locals
	for idx,param in ipairs(m.actualParameterList.parameters) do
		local paramType = param.type.refType
		local isPointer = false

		if paramType.type == "CStructuredType" and VTypeIsRefStruct(paramType) then
			isPointer = true
		end
		
		local isThis = (param.name.string == "this")
		local isConstant = ((isPointer and param.isConst) or (isThis and not isPointer))
		
		local l = LocalVariable(m, paramType, isPointer, isConstant)
		if isThis then
			thisLocal = l
		end
		codeBlock:CreateLocal(ces, l, param.name)
	end

	if not m.isStatic then
		assert(thisLocal)
		memberLookupScope.thisLocal = thisLocal
	end

	-- Create an interior code block so that the parameters and return values don't get discharged
	codeBlock = CodeBlock(cs, nil, codeBlock, codeBlock.localIndex)

	CompileCodeBlock(cs, ces, codeBlock, codeBlockNode)

	ces:PushCodeLocation(m.name.filename, m.name.line)
	ces:AddInstruction( { op = "terminate" } )
	ces:Discharge()
	ces:PopCodeLocation()

	m.compiledInstructions = ces.instructions
	--print("Method dump for "..m.longName)
	m.compiledInstructions = CorrectBranches(m.compiledInstructions)

	-- Strip local info if we're not emitting debug info
	if not emitDebugInfo then
		for _,instr in ipairs(m.compiledInstructions) do
			if instr.op == "createlocal" or instr.op == "alloclocal" then
				instr.str1 = nil
			end
		end
	end

	-- If this falls through to the end and there are no return args, return
	if m.compiledInstructions[#m.compiledInstructions].op == "terminate" then
		if #m.returnTypes.typeReferences == 0 then
			local lastInstr = m.compiledInstructions[#m.compiledInstructions]
			lastInstr.op = "return"
			lastInstr.int1 = 0
			LinkReturnJumps(m.compiledInstructions)
		else
			cerror(m.name, "OrphanControlPath")
		end
	end

	cacheState:DropCache(m.codeBlock)
end

local function CompileCode(cs, cacheState, includedNamespaces)
	local uncompiledCode = cs.uncompiledCode
	cs.uncompiledCode = { }

	for _,m in ipairs(uncompiledCode) do
		local canCompile

		if string.sub(m.longName, 1, 1) == "#" then
			canCompile = true
		else
			for _,ns in ipairs(includedNamespaces) do
				if string.sub(m.longName, 1, #ns) == ns then
					canCompile = true
					break
				end
			end
		end

		if canCompile then
			local compileCodeBM
			if benchmark then
				compileCodeBM = { name = "compile method "..m.longName, start = RDXC.Native.msecTime() }
				benchmarks[#benchmarks+1] = compileCodeBM
			end


			CompileMethodCode(cs, m, cacheState)

			if benchmark then
				compileCodeBM.endTime = RDXC.Native.msecTime()
			end
		end
	end
end

local function CompileAll(self, cacheState, iNamespaces)
	CompileTypeShells(self)
	while #self.uncompiledCode > 0 do
		CompileCode(self, cacheState, iNamespaces)
	end

	local gstAdditions = { }
	for k,v in pairs(self.gst) do
		if v.type == "CStructuredType" then
			v.defaultValue = GetStructuredTypeDefault(self, v)
			if v.defaultValue then
				gstAdditions[v.defaultValue.longName] = v.defaultValue
			end
		end
	end

	for k,v in pairs(gstAdditions) do
		self.gst[k] = v
	end
end

RDXC.CompilerState = function()
	return {
		MergeParsedFile = MergeParsedFile,
		CompileAll = CompileAll,
		CompileTypeShells = CompileTypeShells,
		globalNamespace = Namespace(nil),
		uncompiled = { },
		uncompiledCode = { },
		defaultInstances = { },
		gst = { },		-- Global symbol table
	}
end
