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
setglobal("Errors", {
	ExpectedValueExpression = { 1, "Expected value expression but found {1}" },
	CouldNotConvert = { 2, "Could not convert {1} to {2}" },
	CouldNotResolveOperator = { 3, "Could not resolve operator method {1}" },
	OperatorNotMethod = { 4, "Operator member {1} was not a method" },
	CanNotStripConst = { 5, "Can not strip constant qualifier" },
	UnboundInstanceCall = { 6, "Attempted to call instance method without an instance" },
	AmbiguousMethodCall = { 7, "Ambiguous method call" },
	AbstractMethodInvokedExplicitly = { 8, "Abstract method was invoked explicitly" },
	CouldNotMatchOverload = { 9, "Could not find method overload for {1} for the specified parameter types" },
	ExpectedValueForIndex = { 10, "Expected value for index operand" },
	ArrayIndexCountMismatch = { 11, "Expected {1} index operands, but only got {2}" },
	CouldNotFindIndexMethod = { 12, "Could not resolve method __index" },
	IndexMemberNotMethod = { 13, "Member __index was not a method" },
	IndexedObjectWasValue = { 14, "Indexed object was a value" },
	CouldNotFindSetIndexMethod = { 15, "Could not resolve method __setindex" },
	SetIndexMemberNotMethod = { 16, "Member __setindex was not a method" },
	TypeDoesNotHaveInitialize = { 17, "Type does not have an Initialize member" },
	InitializeOnNonReferenceType = { 18, "Can not use initializers on non-referencable types" },
	InitializeMethodReturnsValues = { 19, "Initialize method returns values" },
	ParameterlessInitializerOnMultidimensionalArray = { 20, "Can only use parameterless initializers on one-dimensional arrays" },
	InitializerDimensionNotConstant = { 21, "Dimension in initializer was not a constant" },
	InitializerDimensionWrongType = { 22, "Dimension in initializer was not of the correct type" },
	NegativeDimension = { 23, "Dimension was negative" },
	DimensionInitializerMismatch = { 24, "Specified {1} elements, but {2} initializers" },
	InitializerOnNonStructuredType = { 25, "Can not use initializers on non-structured types" },
	PropertyAlreadyInitialized = { 26, "Property {1} was already initialized" },
	InitializerPropertyNotFound = { 27, "Member {1} could not be resolved" },
	ConditionalExpressionNotValue = { 28, "Conditional expression doesn't evaluate to a value" },
	TernaryPairResolvesToMultipleInterfaces = { 29, "Value pair {1} in ternary expression reduces to multiple interfaces" },
	TernaryPairNotConvertable = { 30, "Value pair {1} in ternary conditional expressions are not directly convertible" },
	DelegatedBoundMethod = { 31, "Can not delegate an object-bound method to a static delegate" },
	CouldNotMatchDelegate = { 32, "Could not find an appropriate overload to delegate" },
	InaccessiblePrivateMember = { 33, "Member '{1}' is inaccessible due to its 'private' accessibility" },
	InaccessibleProtectedMember = { 34, "Member '{1}' is inaccessible due to its 'protected' accessibility" },
	UnresolvedName = { 35, "Couldn't resolve symbol {1}" },
	AccessedExternalLocal = { 36, "Can't access local outside of the current method" },
	UnresolvedMember = { 37, "Could not resolve member symbol '{1}' of type '{2}'" },
	ExpectedStaticMember = { 38, "'{1}' is a bound member, but was used like a static member" },
	ExpectedInstanceMember = { 39, "'{1}' is a static member, but was used like a bound member" },
	CanNotAccessArrayIntercept = { 40, "Can not access member {1}" },
	CalledNonMethod = { 41, "Attempted to call a non-method" },
	CalledIntercept = { 42, "Attempted to call an intercept" },
	TemplateFromNonStructuredType = { 43, "Can only create templates from structured types" },
	TemplateParameterMismatch = { 44, "Template parameter count doesn't match the template definition" },
	CastToNonType = { 45, "Expected a type as cast target" },
	DelegatedToMultipleTypes = { 46, "Delegations are only allowed to single types" },
	BadDelegation = { 47, "Can not delegate a method group as {1}" },
	ExpectedExpression = { 48, "Expected expression but found {1}" },
	NewCreatedNonType = { 49, "Expected type reference for 'new' operator" },
	NewInterface = { 50, "Can not instanciate interface" },
	NewAbstractType = { 51, "Can not instanciate abstract type: {1} was not defined" },
	NewDelegate = { 52, "Can not instanciate delegate type" },
	ExpectedDimensionsForArrayCreation = { 53, "Expected dimensions for a non-initialized array" },
	DuplicateSymbol = { 54, "Symbol '{1}' already exists in this scope" },
	CouldNotResolveNamespace = { 55, "Couldn't resolve namespace {1}" },
	TooManyTemplates = { 56, "Exceeded instanciated template limit" },
	BadDeclInNamespace = { 57, "Decl type '{1}' / '{2}' isn't allowed in a namespace" },
	CreatedArrayOfVarying = { 58, "Can not create arrays of varying" },
	ArraysNotAllowed = { 59, "Array not allowed in this context" },
	ExpectedTypeReference = { 60, "Expected type reference, but found {1}" },
	NewTemplate = { 61, "Can not instanciate instance of template type" },
	UnexpectedDimensions = { 62, "Not expecting dimensions in this context" },
	ExpectedDimensions = { 63, "Expected dimensions" },
	VaryingParameterInNonNative = { 64, "Can not use varying as a parameter of a non-native method" },
	VaryingReturnType = { 65, "Can not use varying as return type" },
	InterfaceResource = { 66, "Can not define an interface as a resource" },
	NonConstantInitializer = { 67, "Expected constant as initializer" },
	StringResource = { 68, "Can't define a resource as a string" },
	InitializerNotCompatible = { 69, "Initializer is not compatible with the target" },
	ResourceReferencesResource = { 70, "Can't define a resource as another resource" },
	BadPropertyInitializerType = { 71, "Can not use an expression of type {1} as a property initializer" },
	BadEnumeratorValueType = { 72, "Expected enumerant as initializer but found {1}" },
	StaticInstanceInitializer = { 73, "Can't use static instance as an initializer" },
	ExpectedConstructorInitializer = { 74, "Expected constructor but found {1}" },
	OnlyPropertyInitializersAllowed = { 75, "Can only initialize properties in this context" },
	UnresolvedMemberInitializer = { 76, "Could not find member {1} to initialize" },
	UninitializableMember = { 77, "Invalid member type to initialize" },
	MemberAlreadyHasDefault = { 78, "Member {1} already has a default declared" },
	VaryingProperty = { 79, "Can not use varying as a property type" },
	DuplicatedMethod = { 80, "Method declared multiple times" },
	OverridedVirtualWithStatic = { 81, "Can not override a virtual method with a static method" },
	OverrideHasDifferentReturn = { 82, "Overriding method has a different return signature than its base" },
	InvalidOverrideFlags = { 83, "Methods that override virtual methods must be declared 'final' or 'virtual'" },
	FinalMethodDoesNotOverride = { 84, "Method was declared 'final', but doesn't override a virtual method" },
	InterfaceMethodMissing = { 85, "Could not match method {1} to implement interface {2}" },
	StaticProperty = { 86, "Can not declare properties as static, use 'resource' instead" },
	PropertyInitializerCountMismatch = { 87, "Unequal property and initializer counts" },
	MethodCollidesWithNonMethod = { 88, "Method defined with a name already being used by a non-method symbol" },
	MethodVisibilityMismatch = { 89, "Visibility specifier differs from other visible methods" },
	MethodStaticMismatch = { 90, "Static specifier differs from other visible methods" },
	MethodInterceptMismatch = { 91, "Intercept specifier differs from other visible methods" },
	CoerceDoesNotReturnOneType = { 92, "Coerces must return exactly one type" },
	InvalidInterceptFormat = { 93, "Intercepts must either return one value and have no parameters, or accept one parameter and return no values" },
	NonVirtualAbstract = { 94, "Abstract methods must be virtual" },
	StaticMethodInInterface = { 95, "Static methods are not allowed in interfaces" },
	VirtualMethodInInterface = { 96, "Virtual specifiers are not allowed in interfaces, methods are automatically virtual" },
	AbstractMethodInInterface = { 97, "Abstract specifiers are not allowed in interfaces, methods are automatically abstract" },
	VirtualMethodInStructure = { 98, "Structures can not contain virtual methods" },
	AbstractMethodHasCode = { 99, "Abstract methods can not have code" },
	MethodMissingCode = { 100, "Non-abstract non-native methods must have code" },
	SetIndexWithReturnValue = { 101, "__setindex can not return a value" },
	ExtendedTemplate = { 102, "Can't extend templates" },
	NonClassExtended = { 103, "Only classes can extend classes" },
	ExtendedNonClass = { 104, "Only classes can be extended" },
	ExtendedFinalClass = { 105, "Can not extend final classes" },
	DuplicateImplementations = { 106, "Declared multiple implementations of {1}" },
	UnsupportedDeclType = { 107, "Unsupported declaration type {1}" },
	UnsupportedTypeMemberType = { 108, "Unsupported type member type {1}" },
	EnumInitializerNotInteger = { 109, "Expected integer constant for enumerator initializer" },
	DuplicateEnumValue = { 110, "Enumerant value {1} created by {2} is already in use" },
	MissingZeroEnumerant = { 111, "Zero value for enumerant not defined" },
	CircularDependency = { 112, "Couldn't resolve dependency, could be circular" },
	ExpectedValue = { 113, "Expected an expression returning a value" },
	TooFewValues = { 114, "Expected an expression of at least {1} values but only {2} were available" },
	AssignRightSideNotValue = { 115, "Right-side expression did not evaluate to a value" },
	AssignLeftSideNotVariable = { 116, "Left-side expression did not evaluate to a variable" },
	AssignTooFewValues = { 117, "Left side of assignment contains {1} values, but right contains {2}" },
	AssignRightSideTooFewValues = { 118, "Right-side of assignment had fewer values than the left" },
	AssignToConstant = { 119, "Can not assign to a constant" },
	VaryingLocal = { 120, "Can not create locals of varying" },
	ReturnValueCountMismatch = { 121, "Method requires {1} return value(s), but {2} were provided" },
	UnresolvedFlowControlTarget = { 122, "Could not find a loop to exit with flow control statement" },
	SwitchWithNoCases = { 123, "Switch block with no cases" },
	MultipleSwitchDefaults = { 124, "Default node specified multiple times" },
	NonConstantSwitchCase = { 125, "Case value must be constant" },
	SwitchCaseWithNoCases = { 126, "No cases defined" },
	ExpressionStatementIsVariable = { 127, "Expression statement evaluates to a variable" },
	ThrowNonException = { 128, "Expected exception type" },
	DuplicateCatch = { 129, "Exception catch for this type already exists" },
	OrphanControlPath = { 130, "Not all control paths return a value" },
	VerifiedCircularDependency = { 131, "Type {1} has a circular dependency" },
	UnalignableByVal = { 132, "By-value struct contains a 'mustberef' struct" },
	MalformedSoftDependencies = { 133, "Soft dependencies must be alternating type names and specifiers" },
	MalformedAttributeTag = { 134, "Malformed attribute tag" },
	SymbolInsideNamedCppObject = { 135, "Symbols may not be contained within an object with cpp name attributes" },
	UnspecifiedDelegateBinding = { 136, "Delegate must be specified as boundto or static" },
	SelfBoundDelegateOutsideOfType = { 137, "Delegate was specified as bound to the current class in a non-class declaration space" },
	DelegatedStaticMethod = { 138, "Can not delegate a static method to a bound delegate" },
	OverranThisParameter = { 139, "Can't pass more values than would be required before 'this' value" },
	NoCatchOrFinally = { 140, "Try block was not followed by any 'catch' or 'finally' blocks" },
	ReferencedAnonymous = { 141, "Package attempted to reference external symbol '{1}', which is anonymous and can only be accessed within its own package" },
	UnexpectedEOF = { 142, "Unexpected end-of-file" },
	NewlineInStringConstant = { 143, "Newline in string constant" },
	UnknownEscape = { 144, "Unknown escape code" },
	MemoryAllocationFailure = { 145, "Out of memory" },
	UnknownSymbol = { 146, "Unknown symbol" },
	UnknownToken = { 147, "Unexpected token '{1}'" },
	UnexpectedTokenType = { 148, "Expected token of type {1}, but found {2} '{3}'" },
	ExpectedDifferentToken = { 149, "Expected '{1}', but found '{2}'" },
	RepeatedAttribute = { 150, "Attribute '{1}' was declared multiple times" },
	IncompleteVariableDeclaration = { 151, "Incomplete variable declaration" },
	ExpectedTerminus = { 152, "Expected an identifier or literal, but found '{1}'" },
	UnsupportedOperator = { 153, "Unsupported operator type {1}" },
	MultipleValueOOA = { 154, "Expected 1 expression for operate-and-assign operator" },
	OrphanAttributes = { 155, "Attribute tags not followed by a declaration" },
	SpecifiedOuterDimensions = { 156, "Specified dimensions only allowed on the innermost dimension" },
	PolyNotAllowed = { 157, "Polymorphic conversion from {1} to {2} requires an explicit cast operator" },
	InitializedArrayWithProperties = { 158, "Can't initialize an array type using a property constructor" },
	InitializedObjectWithExpressions = { 159, "Can't initialize a structured type using an expression list" },
	InitializedNonProperty = { 160, "Can't initialize a non-property using a property initializer" },
	ImplementedNonInterface = { 161, "Implemented non-interface {1}" },
	InternalError = { 162, "Internal compiler error" },
	InterfaceReturnTypeMismatch = { 163, "Interface implementation expects different return types from {1}" },
	PackageNotDefined = { 164, "No package registered for {1}" },
	ResourceInstanceIncompatible = { 165, "Resource instance must be the same type as the resource" },
} )

setglobal("PrettyInternalClasses", {
	CMultipleValues = "list of values",
	CConstant = "constant value",
	CConvertExpression = "converted expression",
	CDispose = "value disposal target",
	CBoundMethodDelegation = "bound method delegation",
	CMethodDelegation = "static method delegation",
	CPlaceholderValue = "placeholder value",
	CObjectProperty = "bound property reference",
	CObjectMethod = "bound method",
	CMemberLookupScope = "member lookup scope",
	CProperty = "property",
	CMethodGroup = "method group",
	CLogicalAndNode = "logical 'and'",
	CLogicalOrNode = "logical 'or'",
	CLogicalNotNode = "logical 'not'",
	CEqualityCompare = "equality comparison",
	CArrayOfType = "array type",
	CStructuredType = "structured type",
	CStaticDelegateType = "static delegate type",
	CBoundDelegateType = "bound delegate type",
	CDelegateMethodGroup = "method group delegation",
	CMethodCall = "method call",
	CArrayIndex = "index expression",
	CSetIndexCall = "'set index' method call",
	CAllocateTemporary = "temporary allocation",
	CCloneExpression = "carried-through expression",
	CInitializeAndRecover = "reusable initialization expression",
	CInitializeArray = "array initializer",
	CInitializeProperties = "property initializer",
	CTernary = "ternary expression",
	CInstanciatedNamespace = "instanciated namespace",
	CLocalVariable = "local variable",
	CNamespace = "namespace",
	CStaticInstance = "static instance",
	CNewInstance = "instance creation expression",
	CGenerateHashNode = "hash computation expression",
	CTypeReference = "type reference",
	CScope = "scope",
	CImportedNamespace = "imported namespace",
	CCompilableNamespace = "compilable namespace",
	CUnresolvedExpression = "unresolved expression",
	CTypeTuple = "type list",
	CParameterList = "parameter list",
	CInitializer = "initializer",
	CTypeFinalizer = "type finalizer",
	CInterfaceImplementation = "interface implementation",
	CEnumerant = "enumerant",
	CMethod = "method",
	CCodeBlock = "code block",
	CAdjustValueCount = "value count reduction",
	CCodeEmissionState = "code emission state",
	CShellSpace = "shell space",
} )

setglobal("Warnings", {
	CouldNotFoldConstant = { 1, "Couldn't fold constant expression, this probably means that it will cause an exception at runtime" },
	LossyConversion = { 2, "Loss of precision from parameter conversion" },
	NullPassedToNotNull = { 3, "null passed to notnull parameter, this will cause an exception at runtime" },
	MaskedLocal = { 4, "Local '{1}' hides another local in the same function" },
	InvisibleSymbol = { 5, "Local '{1}' is initialized with identifier '{2}', which references something else because the local does not exist until the statement completes" },
	TruncatedValue = { 6, "Expression was truncated and the result will not be used" },
	ImpossibleConversionCheck = { 7, "'is' expression will always return 'false' because the expression and type are incompatible" },
} )

setglobal("ExpandCode", function(code, digits)
	code = tostring(code)
	while #code < digits do
		code = "0"..code
	end
	return code
end )

setglobal("terror", function(line, filename, fatal, subClass, errType, param1, param2, param3)
	local errorTable = Errors
	local fullMsg = "ERROR: "
	if not fatal then
		fullMsg = "WARNING: "
		errorTable = Warnings
	end

	local errorCode = ExpandCode(errorTable[errType][1], 4)
	local errorExplanation = errorTable[errType][2]

	if line ~= nil and filename ~= nil then
		fullMsg = fullMsg..filename.."["..line.."]: "
	end

	fullMsg = fullMsg..subClass..errorCode

	if fatal and traceErrors then
		error(fullMsg.."/"..errType)
	else
		if param1 ~= nil then
			errorExplanation = string.gsub(errorExplanation, "{1}", param1)
		end
		if param2 ~= nil then
			errorExplanation = string.gsub(errorExplanation, "{2}", param2)
		end
		if param3 ~= nil then
			errorExplanation = string.gsub(errorExplanation, "{3}", param3)
		end
		io.stderr:write(fullMsg..": "..errorExplanation.."\n")
		if fatal then
			os.exit(-1)
		end
	end
end )

setglobal("ncerror", function(errType, param1, param2, param3)
	terror(nil, nil, true, "C", errType, param1, param2, param3)
end )