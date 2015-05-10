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

setglobal("perror", function(line, filename, reason, param1, param2, param3)
	terror(line, filename, true, "C", reason, param1, param2, param3)
end )

local function Node(nodeType, lexState)
	return { type = nodeType, line = lexState.GetToken().line, filename = lexState.GetToken().filename }
end

local function Operator(opSymbol, opType, opCategory, validInTypeName)
	return { symbol = opSymbol, type = opType, category = opCategory, validInTypeName = validInTypeName }
end

local function TokenToNode(ls)
	local t = ls.GetToken()
	return { type = t.type, string = t.string, line = t.line, filename = t.filename }
end


local function UnknownToken(lexState)
	local t = lexState.GetToken()
	perror(t.line, t.filename, "UnknownToken", t.string)
end

local function ExpectType(lexState, type)
	local t = lexState.GetToken()
	if t.type ~= type  then
		perror(t.line, t.filename, "UnexpectedTokenType", type, t.type, t.string)
	end
end

local function Expect(lexState, value, type)
	if type == nil then
		type = "Punctuation"
	end

	local t = lexState.GetToken()
	if t.string ~= value then
		perror(t.line, t.filename, "ExpectedDifferentToken", value, t.string)
	end

	ExpectType(lexState, type)
end

local function CheckToken(lexState, value, type)
	if type == nil then
		type = "Punctuation"
	end
	local t = lexState.GetToken()

	return t.type == type and t.string == value
end

-- Interdicting types require special consideration
local interdictingTypes = set { "Cast", "Invoke", "Indirect", "Binary", "Index", "Template", "Ternary", "CheckCast" }

local assignmentOperators = set { "*=", "+=", "-=", "/=", "%=" }

local incrementalOperators = set { "++", "--" }

local operators =
{
	{
		Operator("(", "Punctuation", "Isolate"),
	},

	{
		Operator("generatehash", "generatehash", "GenerateHash"),
		Operator("typeof", "typeof", "TypeOf" ),
		Operator("new", "new", "New"),
		Operator("tuple", "tuple", "CreateTuple"),
	},
	
	{
		Operator(":<", "Punctuation", "Template", true),
		Operator(".", "Punctuation", "Indirect", true),
		Operator("[", "Punctuation", "Index"),
		Operator("(", "Punctuation", "Invoke"),
	},

	{
		Operator("-", "Punctuation", "Unary"),
		Operator("!", "Punctuation", "Unary"),
	},

	{
		Operator("*", "Punctuation", "Binary"),
		Operator("/", "Punctuation", "Binary"),
		Operator("%", "Punctuation", "Binary"),
	},

	{
		Operator("+", "Punctuation", "Binary"),
		Operator("-", "Punctuation", "Binary"),
	},
	
	{
		Operator("as", "as", "Cast"),
		Operator("is", "is", "CheckCast"),
	},
	
	{
		Operator("==", "Punctuation", "Binary"),
		Operator("!=", "Punctuation", "Binary"),
		Operator("<=", "Punctuation", "Binary"),
		Operator(">=", "Punctuation", "Binary"),
		Operator("<", "Punctuation", "Binary"),
		Operator(">", "Punctuation", "Binary"),
	},

	{
		Operator("&&", "Punctuation", "Binary"),
		Operator("||", "Punctuation", "Binary"),
	},

	{
		Operator("?", "Punctuation", "Ternary"),
	},
}


local ParseDeclSpaceNode
local ParseNamespaceNode
local ParseUsingNode
local ParseMemberDeclNode
local ParseDefaultDeclNode
local ParseDefaultDeclListNode
local ParseTemplateParameterDeclNode
local ParseAccessDescriptorNode
local ParseTypeMembersNode
local ParseTypeTupleNode
local ParseFunctionDeclParameterListNode
local ParseFunctionDeclParameterNode
local ParseTypeNode
local ParseCodeBlockNode
local ParseStatementNode
local ParseExpressionTerminusNode
local ParseExpressionNode
local ParseExpressionListNode
local ParseTypeMembersNode
local ParseLocalDeclNode
local ParseDeclarationListNode
local ParseEnumerationListNode
local ParseEnumerationNode

local ParseAttributeTags = function(lexState)
	if not CheckToken(lexState, "[") then
		return nil
	end
	
	local attribTags = { }
	
	while CheckToken(lexState, "[") do
		lexState.NextToken()
		ExpectType(lexState, "Name")

		local tagType = lexState.GetToken().string
		lexState.NextToken()
		
		local attribSet = attribTags[tagType]
		
		if attribTags[tagType] == nil then
			attribSet = { }
			attribTags[tagType] = attribSet
		end
		
		while not CheckToken(lexState, "]") do
			ExpectType(lexState, "Name")
			local attribNameNode = TokenToNode(lexState)
			local attribName = lexState.GetToken().string
			lexState.NextToken()
			
			if attribSet[attribName] ~= nil then
				local t = lexState.GetToken()
				perror(t.line, t.filename, "RepeatedAttribute", attribName)
			end

			local attribData = true
			
			if CheckToken(lexState, "(") then
				lexState.NextToken()

				attribData = { sourceNode = attribNameNode }
				while not CheckToken(lexState, ")") do
					local t = lexState.GetToken()
					if t.type ~= "Number" and t.type ~= "String" then
						ExpectType(lexState, "Name")
					end

					attribData[#attribData+1] = TokenToNode(lexState)
					lexState.NextToken()
				end
				lexState.NextToken()
			end
			
			attribSet[attribName] = attribData
		end
		lexState.NextToken()
	end

	return attribTags
end

ParseEnumerationNode = function(lexState)
	local node = Node("Enumeration", lexState)

	ExpectType(lexState, "Name")

	node.name = TokenToNode(lexState)
	lexState.NextToken()

	if CheckToken(lexState, "=") then
		lexState.NextToken()
		ExpectType(lexState, "Number")
		node.initializer = TokenToNode(lexState)

		lexState.NextToken()
	end

	return node
end

ParseEnumerationListNode = function(lexState)
	local node = Node("EnumerationList", lexState)

	ExpectType(lexState, "Name")

	node.enumerants = { }
	while true do
		if lexState.GetToken().type ~= "Name" then
			return node
		end

		node.enumerants[#node.enumerants+1] = ParseEnumerationNode(lexState)

		if not CheckToken(lexState, ",") then
			return node
		end
		lexState.NextToken()
	end
end


ParseDeclarationListNode = function(lexState)
	local node = Node("DeclarationList", lexState)

	local currentType = nil

	node.declarations = { }
	while true do
		ExpectType(lexState, "Name")

		local declName = TokenToNode(lexState)		-- Parse as a name
		local declType = ParseTypeNode(lexState)	-- Parse as a type

		if lexState.GetToken().type == "Name" then
			-- Type/name combo
			currentType = declType
			declName = TokenToNode(lexState)	-- The real name
			lexState.NextToken()
		else
			-- Just a name
			declType = currentType
		end

		if declType == nil then
			perror(declName.line, declName.filename, "IncompleteVariableDeclaration")
		end

		node.declarations[#node.declarations+1] = { type = declType, name = declName }

		if not CheckToken(lexState, ",") then
			return node
		end
		lexState.NextToken()
	end
end

ParseExpressionTerminusNode = function(lexState)
	local t = lexState.GetToken()
	if t.type ~= "Name" and
		t.type ~= "Number" and
		t.type ~= "String" and
		t.type ~= "this" and
		t.type ~= "true" and
		t.type ~= "false" and
		t.type ~= "null" then
		perror(t.line, t.filename, "ExpectedTerminus", t.string)
	end

	local node = TokenToNode(lexState)
	lexState.NextToken()
	return node
end


local function ParsePropertyInitializerList(lexState)
	local node = Node("PropertyInitializerListNode", lexState)
	node.initializers = { }

	if lexState.GetToken().type ~= "Name" then
		return ParseExpressionListNode(lexState, true)
	end

	do
		local nameCheckToken = lexState.GetToken()
		lexState.NextToken()
		local isExpressionList = not CheckToken(lexState, "=")
		lexState.ReinsertToken(nameCheckToken)

		if isExpressionList then
			return ParseExpressionListNode(lexState, true)
		end
	end

	local isFirst = true

	while true do
		if CheckToken(lexState, "}") then
			return node
		else
			if isFirst then
				isFirst = false
			else
				Expect(lexState, ",")
				lexState.NextToken()

				if CheckToken(lexState, "}") then
					return node
				end
			end
		end

		local initNode = Node("PropertyInitializerNode", lexState)
		ExpectType(lexState, "Name")

		initNode.name = TokenToNode(lexState)
		lexState.NextToken()

		Expect(lexState, "=")
		lexState.NextToken()


		initNode.expression = ParseExpressionNode(lexState)

		node.initializers[#node.initializers+1] = initNode
	end
end

ParseExpressionNode = function(lexState, isTypeName, priorityLevel)
	if priorityLevel == nil then
		priorityLevel = #operators
	end

	if priorityLevel == 0 then
		return ParseExpressionTerminusNode(lexState)
	end

	-- Single result node
	for _,operator in ipairs(operators[priorityLevel]) do
		local t = lexState.GetToken()
		if (operator.validInTypeName or not isTypeName) and CheckToken(lexState, operator.symbol, operator.type) then
			if operator.category == "Isolate" then
				lexState.NextToken()
				local node = Node("SingleResultNode", lexState)
				node.operands = { ParseExpressionNode(lexState, isTypeName) }	-- Top priority
				Expect(lexState, ")")
				lexState.NextToken()
				return node
			elseif operator.category == "Unary" then
				local node = Node("UnaryOperatorNode", lexState)
				node.operator = TokenToNode(lexState)
				lexState.NextToken()
				node.operands = { ParseExpressionNode(lexState, isTypeName, priorityLevel) }
				return node
			elseif operator.category == "New" then
				local node = Node("NewInstanceNode", lexState)
				lexState.NextToken()

				node.typeSpec = ParseTypeNode(lexState, true)

				if node.typeSpec.type ~= "ArrayOfType" and CheckToken(lexState, "(") then
					lexState.NextToken()

					if not CheckToken(lexState, ")") then
						node.initParameters = ParseExpressionListNode(lexState)
						Expect(lexState, ")")
					end
					lexState.NextToken()
				elseif CheckToken(lexState, "{") then
					lexState.NextToken()

					node.initializers = ParsePropertyInitializerList(lexState)

					Expect(lexState, "}")
					lexState.NextToken()
				end

				return node
			elseif operator.category == "GenerateHash" then
				local node = Node("GenerateHashNode", lexState)
				lexState.NextToken()
				Expect(lexState, "(")
				lexState.NextToken()
				node.expression = ParseExpressionNode(lexState)
				Expect(lexState, ")")
				lexState.NextToken()
				return node
			elseif operator.category == "TypeOf" then
				local node = Node("TypeOfNode", lexState)
				lexState.NextToken()
				Expect(lexState, "(")
				lexState.NextToken()
				node.expression = ParseExpressionNode(lexState)
				Expect(lexState, ")")
				lexState.NextToken()
				return node
			elseif operator.category == "CreateTuple" then
				local node = Node("CreateTupleNode", lexState)
				lexState.NextToken()
				Expect(lexState, "(")
				lexState.NextToken()
				node.expression = ParseExpressionListNode(lexState)
				Expect(lexState, ")")
				lexState.NextToken()
				return node
			else
				-- Binary operator that isn't valid here
			end
		end
	end

	local leftNode = ParseExpressionNode(lexState, isTypeName, priorityLevel - 1)

	while true do
		local t = lexState.GetToken()
		local matchedOperator = false
		for _,operator in ipairs(operators[priorityLevel]) do
			if interdictingTypes[operator.category] and CheckToken(lexState, operator.symbol, operator.type) and (operator.validInTypeName or not isTypeName) then
				matchedOperator = true
				local operatorNode

				if operator.category == "Binary" then
					operatorNode = Node("BinaryOperatorNode", lexState)
					operatorNode.operator = TokenToNode(lexState)
					lexState.NextToken()
					operatorNode.operands = { leftNode, ParseExpressionNode(lexState, isTypeName, priorityLevel-1) }
				elseif operator.category == "Invoke" then
					operatorNode = Node("Invoke", lexState)
					lexState.NextToken()
					operatorNode.operands = { leftNode }
					if not CheckToken(lexState, ")") then
						operatorNode.operands[2] = ParseExpressionListNode(lexState)
					end
					Expect(lexState, ")")
					lexState.NextToken()
				elseif operator.category == "Index" then
					operatorNode = Node("Index", lexState)
					lexState.NextToken()
					operatorNode.operands = { leftNode, ParseExpressionListNode(lexState) }
					Expect(lexState, "]")
					lexState.NextToken()
				elseif operator.category == "Template" then
					operatorNode = Node("Template", lexState)
					lexState.NextToken()
					operatorNode.operands = { leftNode, ParseTypeTupleNode(lexState) }
					Expect(lexState, ">")
					lexState.NextToken()
				elseif operator.category == "Indirect" then
					operatorNode = Node("Indirect", lexState)
					lexState.NextToken()

					if lexState.GetToken().type == "explicit" then
						operatorNode.explicit = true
						lexState.NextToken()
						Expect(lexState, ":")
						lexState.NextToken()
					end

					ExpectType(lexState, "Name")

					operatorNode.operands = { leftNode, TokenToNode(lexState) }
					lexState.NextToken()
				elseif operator.category == "Ternary" then
					operatorNode = Node("Ternary", lexState)
					lexState.NextToken()
					operatorNode.condition = leftNode
					local trueExpr = ParseExpressionNode(lexState)
					Expect(lexState, ":")
					lexState.NextToken()
					local falseExpr = ParseExpressionNode(lexState)
					
					operatorNode.operands = { trueExpr, falseExpr }
				elseif operator.category == "Cast" then
					operatorNode = Node("Cast", lexState)
					lexState.NextToken()
					
					if CheckToken(lexState, "(") then
						lexState.NextToken()
						operatorNode.operands = { leftNode, ParseTypeTupleNode(lexState) }
						Expect(lexState, ")")
						lexState.NextToken()
						operatorNode.multipleTypes = true
					else
						operatorNode.operands = { leftNode, ParseTypeNode(lexState) }
						operatorNode.multipleTypes = false
					end
				elseif operator.category == "CheckCast" then
					operatorNode = Node("CheckCast", lexState)
					lexState.NextToken()

					operatorNode.operands = { leftNode, ParseTypeNode(lexState) }
				else
					local token = lexState.GetToken()
					perror(token.line, token.filename, "UnsupportedOperator", operator.category)
				end

				leftNode = operatorNode

				break
			end
		end

		if not matchedOperator then
			break
		end
	end

	return leftNode
end

ParseExpressionListNode = function(lexState, allowUnfollowedComma)
	local node = Node("ExpressionList", lexState)
	node.expressions = { }

	while true do
		if allowUnfollowedComma and CheckToken(lexState, "}") then
			return node
		end

		node.expressions[#node.expressions+1] = ParseExpressionNode(lexState)
		if not CheckToken(lexState, ",") then
			return node
		end

		lexState.NextToken()
	end
end

local ParseCatchAndFinallyBlocks = function(lexState, node)	
	local catchNodes = Node("CatchBlocks", lexState)
	catchNodes.catchBlocks = { }

	while true do
		if CheckToken(lexState, "catch", "catch") then
			lexState.NextToken()

			local catchNode = Node("CatchBlock", lexState)
			if CheckToken(lexState, "(") then
				lexState.NextToken()
				catchNode.exceptionType = ParseTypeNode(lexState)
				ExpectType(lexState, "Name")
				catchNode.exceptionName = TokenToNode(lexState)
				lexState.NextToken()
				Expect(lexState, ")")
				lexState.NextToken()
			end
			catchNode.block = ParseCodeBlockNode(lexState)

			catchNodes.catchBlocks[#catchNodes.catchBlocks+1] = catchNode
			node.catchBlocks = catchNodes
		elseif CheckToken(lexState, "finally", "finally") then
			lexState.NextToken()
			node.finallyBlock = ParseCodeBlockNode(lexState)
		else
			break
		end
	end

	return node
end

local ParseCaseNode = function(lexState)
	local node = Node("SwitchCase", lexState)

	Expect(lexState, "case", "case")
	lexState.NextToken()

	if CheckToken(lexState, ":") then
		lexState.NextToken()
		ExpectType(lexState, "Name")
		node.label = TokenToNode(lexState)
		lexState.NextToken()
	end

	Expect(lexState, "(")
	lexState.NextToken()

	node.expressions = { }

	while true do
		if CheckToken(lexState, "default", "default") then
			node.expressions[#node.expressions+1] = Node("DefaultCase", lexState)
			lexState.NextToken()
		else
			node.expressions[#node.expressions+1] = ParseExpressionNode(lexState)
		end

		if CheckToken(lexState, ")") then
			break
		end
		Expect(lexState, ",")
		lexState.NextToken()
	end

	lexState.NextToken()
	Expect(lexState, ":")
	lexState.NextToken();

	node.block = ParseCodeBlockNode(lexState)

	return node
end

ParseStatementNode = function(lexState, suppressExpectSemi)
	if lexState.GetToken().type == "while" then
		local node = Node("WhileLoop", lexState)
		lexState.NextToken()

		if CheckToken(lexState, ":") then
			lexState.NextToken()
			ExpectType(lexState, "Name")
			node.label = TokenToNode(lexState)
			lexState.NextToken()
		end

		Expect(lexState, "(")
		lexState.NextToken()
		node.condition = ParseExpressionNode(lexState)

		Expect(lexState, ")")
		lexState.NextToken()

		node.block = ParseCodeBlockNode(lexState)
		return node
	end

	if lexState.GetToken().type == "do" then
		local node = Node("DoLoop", lexState)
		lexState.NextToken()

		if CheckToken(lexState, ":") then
			lexState.NextToken()
			ExpectType(lexState, "Name")
			node.label = TokenToNode(lexState)
			lexState.NextToken()
		end

		node.block = ParseCodeBlockNode(lexState)

		if CheckToken(lexState, "while", "while") then
			lexState.NextToken()
			Expect(lexState, "(")
			lexState.NextToken()
			node.condition = ParseExpressionNode(lexState)

			Expect(lexState, ")")
			lexState.NextToken()
		end

		if not suppressExpectSemi then
			Expect(lexState, ";")
			lexState.NextToken()
		end

		return node
	end

	if lexState.GetToken().type == "if" then
		local node = Node("IfBlock", lexState)
		lexState.NextToken()
		Expect(lexState, "(")
		lexState.NextToken()
		node.condition = ParseExpressionNode(lexState)
		Expect(lexState, ")")
		lexState.NextToken()
		node.block = ParseCodeBlockNode(lexState)
		if lexState.GetToken().type == "else" then
			lexState.NextToken()
			node.elseBlock = ParseCodeBlockNode(lexState)
		end

		return node
	end

	if lexState.GetToken().type == "switch" then
		local node = Node("SwitchBlock", lexState)
		lexState.NextToken()
		Expect(lexState, "(")
		lexState.NextToken()

		node.value = ParseExpressionNode(lexState)
		Expect(lexState, ")")
		lexState.NextToken()

		Expect(lexState, "{")
		lexState.NextToken()

		node.cases = { }

		while not CheckToken(lexState, "}") do
			node.cases[#node.cases+1] = ParseCaseNode(lexState)
		end
		lexState.NextToken()

		return node
	end

	if lexState.GetToken().type == "try" then
		local node = Node("TryBlock", lexState)
		lexState.NextToken()
		node.block = ParseCodeBlockNode(lexState)
		ParseCatchAndFinallyBlocks(lexState, node)
		return node
	end

	if lexState.GetToken().type == "throw" then
		local node = Node("Throw", lexState)
		lexState.NextToken()
		node.expression = ParseExpressionNode(lexState)

		if not suppressExpectSemi then
			Expect(lexState, ";")
			lexState.NextToken()
		end

		return node
	end

	if lexState.GetToken().type == "for" then
		local node = Node("ForLoop", lexState)
		lexState.NextToken()
		if CheckToken(lexState, ":") then
			lexState.NextToken()
			ExpectType(lexState, "Name")
			node.label = TokenToNode(lexState)
			lexState.NextToken()
		end

		Expect(lexState, "(")
		lexState.NextToken()

		if CheckToken(lexState, ";") then
			lexState.NextToken()
		else
			node.initial = ParseCodeBlockNode(lexState)
		end

		if not CheckToken(lexState, ";") then
			node.condition = ParseExpressionNode(lexState)
		end
		Expect(lexState, ";")
		lexState.NextToken()

		if not CheckToken(lexState, ")") then
			node.iteration = ParseCodeBlockNode(lexState, true)
		end
		Expect(lexState, ")")
		lexState.NextToken()
		node.block = ParseCodeBlockNode(lexState)

		return node
	end

	if lexState.GetToken().type == "foreach" then
		local node = Node("ForEachLoop", lexState)
		lexState.NextToken()

		if CheckToken(lexState, ":") then
			lexState.NextToken()
			ExpectType(lexState, "Name")
			node.label = TokenToNode(lexState)
			lexState.NextToken()
		end

		Expect(lexState, "(")
		lexState.NextToken()

		node.declarations = ParseDeclarationListNode(lexState)

		Expect(lexState, "in", "in")
		lexState.NextToken()

		node.iterator = ParseExpressionNode(lexState)

		Expect(lexState, ")")
		lexState.NextToken()

		node.block = ParseCodeBlockNode(lexState)

		return node
	end

	if lexState.GetToken().type == "local" then
		lexState.NextToken()

		local t = lexState.GetToken()
		local node
		
		if t.type == "class" or t.type == "struct" or t.type == "interface" or t.type == "enum" then
			node = ParseMemberDeclNode(lexState)
		else
			node = Node("LocalDecl", lexState)
			node.declarations = ParseDeclarationListNode(lexState)

			if CheckToken(lexState, "=") then
				lexState.NextToken()
				node.initializers = ParseExpressionListNode(lexState)
			end
		end

		if not suppressExpectSemi then
			Expect(lexState, ";")
			lexState.NextToken()
		end

		return node
	end

	if lexState.GetToken().type == "using" then
		local node = Node("UsingDecl", lexState)
		lexState.NextToken()
		Expect(lexState, "(")
		lexState.NextToken()

		node.declarations = ParseDeclarationListNode(lexState)

		Expect(lexState, "=")
		lexState.NextToken()
		node.initializers = ParseExpressionListNode(lexState)

		Expect(lexState, ")")
		lexState.NextToken()

		node.block = ParseCodeBlockNode(lexState)

		return node
	end

	if lexState.GetToken().type == "return" then
		local node = Node("Return", lexState)

		lexState.NextToken()
		if CheckToken(lexState, ";") then
			lexState.NextToken()
			return node
		end

		node.returnValues = ParseExpressionListNode(lexState)
		Expect(lexState, ";")
		lexState.NextToken()

		return node
	end

	if lexState.GetToken().type == "continue" then
		local node = Node("Continue", lexState)
		lexState.NextToken()

		if lexState.GetToken().type == "Name" then
			node.label = TokenToNode(lexState)
			lexState.NextToken()
		end

		if not suppressExpectSemi then
			Expect(lexState, ";")
			lexState.NextToken()
		end

		return node
	end

	if lexState.GetToken().type == "break" then
		local node = Node("Break", lexState)
		lexState.NextToken()

		if lexState.GetToken().type == "Name" then
			node.label = TokenToNode(lexState)
			lexState.NextToken()
		end

		if not suppressExpectSemi then
			Expect(lexState, ";")
			lexState.NextToken()
		end

		return node
	end

	if CheckToken(lexState, "{") then
		return ParseCodeBlockNode(lexState)
	end

	local node = ParseExpressionListNode(lexState)

	if CheckToken(lexState, "=") then
		lexState.NextToken()

		local assignNode = Node("Assign", lexState)
		assignNode.destinations = node
		assignNode.sources = ParseExpressionListNode(lexState)

		node = assignNode
	else
		local t = lexState.GetToken()

		if t.type == "Punctuation" then
			local ao = assignmentOperators[t.string]
			local inco = incrementalOperators[t.string]
			if ao then
				local operatorNode = TokenToNode(lexState)
				lexState.NextToken()
				local oaaNode = Node("OperateAndAssign", lexState)

				if #node.expressions ~= 1 then
					perror(t.line, t.filename, "MultipleValueOOA")
				end

				oaaNode.destination = node.expressions[1]
				oaaNode.source = ParseExpressionNode(lexState)
				oaaNode.operator = operatorNode

				node = oaaNode
			elseif inco then
				local operatorNode = TokenToNode(lexState)
				lexState.NextToken()
				local incoNode = Node("IncrementalOperate", lexState)

				if #node.expressions ~= 1 then
					perror(t.line, t.filename, "MultipleValueOOA")
				end

				incoNode.destination = node.expressions[1]
				incoNode.operator = operatorNode

				node = incoNode
			end
		end
	end

	if not suppressExpectSemi then
		Expect(lexState, ";")
		lexState.NextToken()
	end

	return node
end


ParseCodeBlockNode = function(lexState, suppressExpectSemi)
	local isMultiStatement = false
	if CheckToken(lexState, "{") then
		isMultiStatement = true
		lexState.NextToken()
	end

	local node = Node("CodeBlock", lexState)

	node.statements = { }

	while true do
		if isMultiStatement then
			if CheckToken(lexState, "}") then
				lexState.NextToken()
				return node
			end
		end
		node.statements[#node.statements+1] = ParseStatementNode(lexState, (not isMultiStatement) and suppressExpectSemi)
		if not isMultiStatement then
			return node
		end
	end
end

ParseTypeNode = function(lexState, requireDimensions)
	local node = Node("Type", lexState)
	local arrayChain = nil

	node.baseType = ParseExpressionNode(lexState, true)

	while true do
		local nextIsConst = nil
		if CheckToken(lexState, "const", "const") then
			nextIsConst = true
			lexState.NextToken()
			Expect(lexState, "[")
		end

		if CheckToken(lexState, "[") then
			lexState.NextToken()

			if arrayChain == nil then
				arrayChain = { }
			end

			local newNode = Node("ArrayOfType", lexState)
			newNode.dimensions = 1
			newNode.isConst = nextIsConst

			arrayChain[#arrayChain+1] = newNode

			if not CheckToken(lexState, ",") and not CheckToken(lexState, "]") then
				if #arrayChain > 1 then
					perror(node.line, node.filename, "SpecifiedOuterDimensions")
				end
			
				newNode.dimensions = nil
				newNode.specifiedDimensions = ParseExpressionListNode(lexState)
				Expect(lexState, "]")
			else
				while not CheckToken(lexState, "]") do
					Expect(lexState, ",")
					lexState.NextToken()
					newNode.dimensions = newNode.dimensions + 1
				end
			end
			lexState.NextToken()
		else
			if arrayChain ~= nil then
				-- Relink
				local numLinks = #arrayChain
				for i=1,numLinks-1 do
					arrayChain[i].subType = arrayChain[i+1]
				end
				arrayChain[numLinks].subType = node
				return arrayChain[1]
			end

			assert(node)
			return node
		end
	end
	assert(false)
end


ParseTypeTupleNode = function(lexState)
	local node = Node("TypeTuple", lexState)

	node.types = { }

	if CheckToken(lexState, "void", "void") then
		lexState.NextToken()
		return node
	end

	while true do
		node.types[#node.types+1] = ParseTypeNode(lexState)
		if not CheckToken(lexState, ",") then
			return node
		end
		lexState.NextToken()
	end
end

ParseFunctionDeclParameterNode = function(lexState)
	local node = Node("FunctionDeclParameter", lexState)

	if CheckToken(lexState, "const", "const") then
		node.const = TokenToNode(lexState)
		lexState.NextToken()
	end

	if CheckToken(lexState, "notnull", "notnull") then
		node.notnull = TokenToNode(lexState)
		lexState.NextToken()
	end

	node.type = ParseTypeNode(lexState)

	if node.type ~= "this" then
		ExpectType(lexState, "Name")
	end
	node.name = TokenToNode(lexState)

	lexState.NextToken()

	return node
end


ParseFunctionDeclParameterListNode = function(lexState)
	local node = Node("FunctionDeclParameterList", lexState)

	node.parameters = { }

	if CheckToken(lexState, ")") then
		lexState.NextToken()
		return node
	end

	while true do
		node.parameters[#node.parameters+1] = ParseFunctionDeclParameterNode(lexState)
		if CheckToken(lexState, ")") then
			lexState.NextToken()
			return node
		end
		Expect(lexState, ",")
		lexState.NextToken()
	end

	return node
end

ParseTypeMembersNode = function(lexState)
	local node = Node("TypeMembers", lexState)

	node.members = { }

	while true do
		if CheckToken(lexState, "}") then
			return node
		end
		
		local attribTags = ParseAttributeTags(lexState)

		if CheckToken(lexState, "default", "default") then
			local defaultDeclListNode = ParseDefaultDeclListNode(lexState)
			defaultDeclListNode.attribTags = attribTags
			node.members[#node.members+1] = defaultDeclListNode
		else
			local memberDeclNode = ParseMemberDeclNode(lexState)
			memberDeclNode.attribTags = attribTags
			node.members[#node.members+1] = memberDeclNode
		end
	end
end

ParseAccessDescriptorNode = function(lexState)
	local node = Node("AccessDescriptor", lexState)

	local t = lexState.GetToken()
	if t.type == "private" or t.type == "public" or t.type == "protected" then
		node.visibility = TokenToNode(lexState)
		lexState.NextToken()
		t = lexState.GetToken()
	end

	if t.type == "anonymous" then
		node.anonymous = TokenToNode(lexState)
		lexState.NextToken()
		t = lexState.GetToken()
	end

	if t.type == "branching" then
		node.branching = TokenToNode(lexState)
		lexState.NextToken()
		t = lexState.GetToken()
		ExpectType(lexState, "native")
	end

	if t.type == "native" then
		node.native = TokenToNode(lexState)
		lexState.NextToken()
		t = lexState.GetToken()
	end

	if t.type == "byval" then
		node.byVal = true
		lexState.NextToken()
		t = lexState.GetToken()
	elseif t.type == "mustberef" then
		node.mustBeRef = true
		lexState.NextToken()
		t = lexState.GetToken()
	end

	if t.type == "static" then
		node.static = TokenToNode(lexState)
		lexState.NextToken()
		t = lexState.GetToken()
	else
		if t.type == "final" then
			node.final = TokenToNode(lexState)
			lexState.NextToken()
			t = lexState.GetToken()
		else
			if t.type == "localized" then
				node.localized = TokenToNode(lexState)
				lexState.NextToken()
				t = lexState.GetToken()
			end
			if t.type == "abstract" then
				node.abstract = TokenToNode(lexState)
				lexState.NextToken()
				t = lexState.GetToken()
			end
			if t.type == "virtual" then
				node.virtual = TokenToNode(lexState)
				lexState.NextToken()
				t = lexState.GetToken()
			end
		end

		if t.type == "intercept" then
			node.intercept = TokenToNode(lexState)
			lexState.NextToken()
			t = lexState.GetToken()
		end
	end

	if t.type == "const" then
		node.const = TokenToNode(lexState)
		lexState.NextToken()
		t = lexState.GetToken()
	end

	if t.type == "mustbeconst" then
		node.mustbeconst = TokenToNode(lexState)
		lexState.NextToken()
		t = lexState.GetToken()
	end

	return node
end

ParseTemplateParameterDeclNode = function(lexState)
	local node = Node("TemplateParameterDecl", lexState)

	ExpectType(lexState, "Name")
	node.parameters = { }
	while true do
		node.parameters[#node.parameters+1] = TokenToNode(lexState)
		lexState.NextToken()
		if CheckToken(lexState, ">") then
			return node
		end
		Expect(lexState, ",")
		lexState.NextToken()
	end
end

ParseDefaultDeclNode = function(lexState)
	local node = Node("DefaultDeclList", lexState)

	ExpectType(lexState, "Name")

	node.fieldName = TokenToNode(lexState)
	lexState.NextToken()
	Expect(lexState, "=")
	lexState.NextToken()
	node.expression = ParseExpressionNode(lexState)

	return node
end

ParseDefaultDeclListNode = function(lexState)
	local node = Node("DefaultDeclList", lexState)

	Expect(lexState, "default", "default")
	lexState.NextToken()

	node.defaultDecls = { }
	while true do
		ExpectType(lexState, "Name")
		node.defaultDecls[#node.defaultDecls+1] = ParseDefaultDeclNode(lexState)
		if CheckToken(lexState, ";") then
			lexState.NextToken()
			return node
		end
		Expect(lexState, ",")
		lexState.NextToken()
	end
end


ParseMemberDeclNode = function(lexState)
	local node = Node("MemberDecl", lexState)

	node.accessDescriptor = ParseAccessDescriptorNode(lexState)

	local t = lexState.GetToken()

	if t.type == "class" or t.type == "struct" or t.type == "interface" then
		node.declType = TokenToNode(lexState)
		lexState.NextToken()

		ExpectType(lexState, "Name")
		node.name = TokenToNode(lexState)
		lexState.NextToken()

		if CheckToken(lexState, ":<") then
			lexState.NextToken()
			node.templateParameters = ParseTemplateParameterDeclNode(lexState)
			Expect(lexState, ">")
			lexState.NextToken()
		end

		if lexState.GetToken().type == "extends" and t.type == "class" then
			lexState.NextToken()
			node.parent = ParseTypeNode(lexState)
		end

		if lexState.GetToken().type == "implements" and t.type == "class" then
			lexState.NextToken()
			node.interfaces = ParseExpressionListNode(lexState)
		end

		if CheckToken(lexState, "{") then
			lexState.NextToken()
			node.typeMembers = ParseTypeMembersNode(lexState)
			Expect(lexState, "}")
		else
			Expect(lexState, ";")
		end
		lexState.NextToken()
	elseif t.type == "typedef" then
		node.declType = TokenToNode(lexState)
		lexState.NextToken()
		ExpectType(lexState, "Name")
		node.name = TokenToNode(lexState)
		lexState.NextToken()
		Expect(lexState, "=")
		lexState.NextToken()
		node.specifiedType = ParseTypeNode(lexState)
		Expect(lexState, ";")
		lexState.NextToken()
	elseif t.type == "property" or t.type == "resource" then
		node.declType = TokenToNode(lexState)
		lexState.NextToken()
		node.declList = ParseDeclarationListNode(lexState)
		if CheckToken(lexState, "=") then
			lexState.NextToken()
			node.initializers = ParseExpressionListNode(lexState)
		end

		Expect(lexState, ";")
		lexState.NextToken()
	elseif t.type == "coerce" or t.type == "promote" then
		node.declType = TokenToNode(lexState)
		lexState.NextToken()
		node.returnType = ParseTypeTupleNode(lexState)
		if CheckToken(lexState, "{") then
			local codeBlock = ParseCodeBlockNode(lexState)
			node.codeBlockCacheID = lexState.cacheState:Cache(codeBlock)
		else
			Expect(lexState, ";")
			lexState.NextToken()
		end
	elseif t.type == "function" then
		node.declType = TokenToNode(lexState)
		lexState.NextToken()

		node.returnType = ParseTypeTupleNode(lexState)

		ExpectType(lexState, "Name")
		node.name = TokenToNode(lexState)
		lexState.NextToken()


		Expect(lexState, "(")
		lexState.NextToken()
		node.parameters = ParseFunctionDeclParameterListNode(lexState)

		if CheckToken(lexState, "{") then
			local codeBlock = ParseCodeBlockNode(lexState)
			node.codeBlockCacheID = lexState.cacheState:Cache(codeBlock)
		else
			Expect(lexState, ";")
			lexState.NextToken()
		end
	elseif t.type == "delegate" then
		node.declType = TokenToNode(lexState)
		lexState.NextToken()

		node.returnType = ParseTypeTupleNode(lexState)

		ExpectType(lexState, "Name")
		node.name = TokenToNode(lexState)
		lexState.NextToken()
		Expect(lexState, "(")
		lexState.NextToken()

		node.parameters = ParseFunctionDeclParameterListNode(lexState)
		Expect(lexState, ";")
		lexState.NextToken()
	elseif t.type == "enum" then
		node.declType = TokenToNode(lexState)
		lexState.NextToken()

		ExpectType(lexState, "Name")
		node.name = TokenToNode(lexState)
		lexState.NextToken()

		Expect(lexState, "{")
		lexState.NextToken()

		node.enumerants = ParseEnumerationListNode(lexState)
		Expect(lexState, "}")
		lexState.NextToken()
		
	else
		UnknownToken(lexState)
	end
	return node
end


ParseUsingNode = function(lexState)
	local node = Node("Using", lexState)

	lexState.NextToken()

	node.namespacePath = { }

	while true do
		ExpectType(lexState, "Name")
		node.namespacePath[#node.namespacePath+1] = TokenToNode(lexState)
		lexState.NextToken()
		if CheckToken(lexState, ";") then
			break
		end
		Expect(lexState, ".")
		lexState.NextToken()
	end
	lexState.NextToken()
	return node
end

ParseNamespaceNode = function(lexState)
	local node = Node("Namespace", lexState)

	lexState.NextToken()

	ExpectType(lexState, "Name")
	node.name = lexState.GetToken()
	
	lexState.NextToken()
	
	Expect(lexState, "{")
	lexState.NextToken()

	node.members = ParseDeclSpaceNode(lexState)

	Expect(lexState, "}")
	lexState.NextToken()

	return node
end

ParseDeclSpaceNode = function(lexState)
	local t = lexState.GetToken()
	local node = Node("DeclSpace", lexState)

	node.declarations = { }

	while t.type ~= "EOF" do
		local attribTags = ParseAttributeTags(lexState)
		t = lexState.GetToken()

		if t.type == "namespace" then
			local nsNode = ParseNamespaceNode(lexState)
			nsNode.attribTags = attribTags
			node.declarations[#node.declarations+1] = nsNode
		elseif t.type == "using" then
			local usingNode = ParseUsingNode(lexState)
			usingNode.attribTags = attribTags
			node.declarations[#node.declarations+1] = usingNode
		elseif CheckToken(lexState, "}") then
			if attribTags ~= nil then
				local t = lexState.GetToken()
				perror(t.line, t.filename, "OrphanAttributes")
			end
			return node
		else
			local memberDeclNode = ParseMemberDeclNode(lexState)
			memberDeclNode.attribTags = attribTags
			node.declarations[#node.declarations+1] = memberDeclNode
		end

		t = lexState.GetToken()
	end

	return node
end

RDXC.CacheState = function()
	return {
		Decache = function(self, cacheID)
			return unpackCachedTable(RDXC.Native.readCacheObject(RDXC.compilerCachePath.."/parse_"..cacheID.library..".cache", cacheID.index))
		end,

		Cache = function(self, data)
			local cacheID = { library = self.library, index = self.numCachedObjects }
			self.numCachedObjects = self.numCachedObjects + 1

			self.objectOffsets[self.numCachedObjects] = RDXC.Native.writeCacheObject(self.cacheFile, cacheTable(data) )

			return cacheID
		end,

		DropCache = function(self, cacheID)
		end,
	}
end

RDXC.Parse = function(lexState, cacheState)
	lexState.cacheState = cacheState
	lexState.NextToken()	-- Get the first token

	local node = ParseDeclSpaceNode(lexState)
	if lexState.GetToken().type ~= "EOF" then
		UnknownToken(lexState)
	end

	return node
end
