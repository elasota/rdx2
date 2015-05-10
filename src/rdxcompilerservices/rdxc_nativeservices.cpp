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

#include "rdx_programmability.hpp"
#include "rdx_marshal.hpp"
#include "rdx_appservices.hpp"
#include "rdx_io.hpp"
#include "rdx_lut.hpp"
#include "rdx_constants.hpp"
#include "rdx_objectloader.hpp"
#include "rdx_murmur3.hpp"
#include "rdx_coretypeattribs.hpp"
#include "rdx_constants.hpp"
#include "rdx_builtins.hpp"

#include "rdx_atof.hpp"

#include "RDXInterface/RDX/Compiler/NativeServices.hpp"
#include "RDXInterface/RDX/Compiler/ErrorCode.hpp"
#include "RDXInterface/RDX/Compiler/WarningCode.hpp"
#include "RDXInterface/RDX/Compiler/CodeLocation.hpp"
#include "RDXInterface/RDX/Compiler/Token.hpp"
#include "RDXInterface/RDX/Compiler/EnumerantObject.hpp"
#include "RDXInterface/RDX/Compiler/Constant.hpp"
#include "RDXInterface/RDX/Compiler/Constant/Signal.hpp"
#include "RDXInterface/RDX/Compiler/CodeEmission/POp.hpp"


static void CRC32(rdxUInt32 *outCRC, const void *ptr, rdxLargeUInt n)
{
	static const rdxUInt32 crctable[] =
	{
		0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
		0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
		0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
		0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
		0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
		0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
		0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
		0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
		0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
		0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
		0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
		0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
		0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
		0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
		0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
		0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
		0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
		0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
		0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
		0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
		0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
		0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
		0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
		0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
		0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
		0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
		0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
		0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
		0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
		0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
		0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
		0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
	};

	const rdxUInt8 *bytes = static_cast<const rdxUInt8 *>(ptr);
	rdxUInt32 crc = 0xffffffff;	// ~previous crc

	while(n--)
		crc = crctable[(crc ^ (*bytes++)) & 0xff] ^ (crc >> 8);
	*outCRC = ~crc;
}

rdxWeakRTRef(rdxCString) RDXInterface::RDX::Compiler::NativeServices::HashString(rdxSExportedCallEnvironment &callEnv, rdxWeakRTRef(rdxCString) str)
{
	rdxUInt32 hash = 0;
	rdxCMurmur3Hash128 generator;
	generator.FeedBytes(str->AsCharsRTRef()->ArrayData(), str->Length() * sizeof(rdxChar));
	rdxUInt64 hashHigh, hashLow;
	generator.Flush(&hashHigh, &hashLow);

	const char *nibbles = "0123456789abcdef";
	rdxChar outHash[16];

	for(int i=0;i<16;i++)
		outHash[i] = static_cast<rdxChar>(nibbles[(hashLow >> (64 - 4 - 4*i)) & 0xf]);

	RDX_TRY(callEnv.ctx)
	{
		rdxCRef(rdxCString) str;
		RDX_PROTECT_ASSIGN(callEnv.ctx, str, callEnv.objm->CreateString(callEnv.ctx, outHash, true, sizeof(outHash) / sizeof(rdxChar)));
		return str.ToWeakRTRef();
	}
	RDX_CATCH(callEnv.ctx)
	{
		callEnv.Throw("Core", "RDX.AllocationFailureException.instance");
		return rdxWeakRTRef(rdxCString)::Null();
	}
	RDX_ENDTRY
}

static const rdxChar *ERROR_MESSAGES[] =
{
	RDX_STATIC_STRING("No error"),
	RDX_STATIC_STRING("Expected value expression but found %s"),
	RDX_STATIC_STRING("Could not convert %s to %s"),
	RDX_STATIC_STRING("Could not resolve operator method %s"),
	RDX_STATIC_STRING("Operator member %s was not a method"),
	RDX_STATIC_STRING("Can not strip constant qualifier"),
	RDX_STATIC_STRING("Attempted to call instance method without an instance"),
	RDX_STATIC_STRING("Ambiguous method call"),
	RDX_STATIC_STRING("Abstract method was invoked explicitly"),
	RDX_STATIC_STRING("Could not find method overload for %s for the specified parameter types"),
	RDX_STATIC_STRING("Expected value for index operand"),
	RDX_STATIC_STRING("Expected %s index operands, but only got %s"),
	RDX_STATIC_STRING("Could not resolve method __index"),
	RDX_STATIC_STRING("Member __index was not a method"),
	RDX_STATIC_STRING("Indexed object was a value"),
	RDX_STATIC_STRING("Could not resolve method __setindex"),
	RDX_STATIC_STRING("Member __setindex was not a method"),
	RDX_STATIC_STRING("Type does not have an Initialize member"),
	RDX_STATIC_STRING("Can not use initializers on non-referencable types"),
	RDX_STATIC_STRING("Initialize method returns values"),
	RDX_STATIC_STRING("Can only use parameterless initializers on one-dimensional arrays"),
	RDX_STATIC_STRING("Dimension in initializer was not a constant"),
	RDX_STATIC_STRING("Dimension in initializer was not of the correct type"),
	RDX_STATIC_STRING("Dimension was negative"),
	RDX_STATIC_STRING("Specified %s elements, but %s initializers"),
	RDX_STATIC_STRING("Can not use initializers on non-structured types"),
	RDX_STATIC_STRING("Property %s was already initialized"),
	RDX_STATIC_STRING("Member %s could not be resolved"),
	RDX_STATIC_STRING("Conditional expression doesn't evaluate to a value"),
	RDX_STATIC_STRING("Value pair %s in ternary expression reduces to multiple interfaces"),
	RDX_STATIC_STRING("Value pair %s in ternary conditional expressions are not directly convertible"),
	RDX_STATIC_STRING("Can not delegate an object-bound method to a static delegate"),
	RDX_STATIC_STRING("Could not find an appropriate overload to delegate"),
	RDX_STATIC_STRING("Member '%s' is inaccessible due to its 'private' accessibility"),
	RDX_STATIC_STRING("Member '%s' is inaccessible due to its 'protected' accessibility"),
	RDX_STATIC_STRING("Couldn't resolve symbol %s"),
	RDX_STATIC_STRING("Can't access local outside of the current method"),
	RDX_STATIC_STRING("Could not resolve member symbol '%s'"),
	RDX_STATIC_STRING("'%s' is not a static member, which was expected in this context"),
	RDX_STATIC_STRING("'%s' is not an instance member, which was expected in this context"),
	RDX_STATIC_STRING("Can not access member %s"),
	RDX_STATIC_STRING("Attempted to call a non-method"),
	RDX_STATIC_STRING("Attempted to call an intercept"),
	RDX_STATIC_STRING("Can only create templates from structured types"),
	RDX_STATIC_STRING("Template parameter count doesn't match the template definition"),
	RDX_STATIC_STRING("Expected a type as cast target"),
	RDX_STATIC_STRING("Delegations are only allowed to single types"),
	RDX_STATIC_STRING("Can not delegate a method group as %s"),
	RDX_STATIC_STRING("Expected expression but found %s"),
	RDX_STATIC_STRING("Expected type reference for 'new' operator"),
	RDX_STATIC_STRING("Can not instanciate interface"),
	RDX_STATIC_STRING("Can not instanciate abstract type: %s was not defined"),
	RDX_STATIC_STRING("Can not instanciate delegate type"),
	RDX_STATIC_STRING("Expected dimensions for a non-initialized array"),
	RDX_STATIC_STRING("Symbol '%s' already exists in this scope"),
	RDX_STATIC_STRING("Couldn't resolve namespace %s"),
	RDX_STATIC_STRING("Exceeded instanciated template limit"),
	RDX_STATIC_STRING("Decl type '%s' / '%s' isn't allowed in a namespace"),
	RDX_STATIC_STRING("Can not create arrays of varying"),
	RDX_STATIC_STRING("Array not allowed in this context"),
	RDX_STATIC_STRING("Expected type reference, but found %s"),
	RDX_STATIC_STRING("Can not instanciate instance of template type"),
	RDX_STATIC_STRING("Not expecting dimensions in this context"),
	RDX_STATIC_STRING("Expected dimensions"),
	RDX_STATIC_STRING("Can not use varying as a parameter of a non-native method"),
	RDX_STATIC_STRING("Can not use varying as return type"),
	RDX_STATIC_STRING("Can not define an interface as a resource"),
	RDX_STATIC_STRING("Expected constant as initializer"),
	RDX_STATIC_STRING("Can't define a resource as a string"),
	RDX_STATIC_STRING("Initializer is not compatible with the target"),
	RDX_STATIC_STRING("Can't define a resource as another resource"),
	RDX_STATIC_STRING("Can not use an expression of type %s as a property initializer"),
	RDX_STATIC_STRING("Expected enumerant as initializer but found %s"),
	RDX_STATIC_STRING("Can't use static instance as an initializer"),
	RDX_STATIC_STRING("Expected constructor but found %s"),
	RDX_STATIC_STRING("Can only initialize properties in this context"),
	RDX_STATIC_STRING("Could not find member %s to initialize"),
	RDX_STATIC_STRING("Invalid member type to initialize"),
	RDX_STATIC_STRING("Member %s already has a default declared"),
	RDX_STATIC_STRING("Can not use varying as a property type"),
	RDX_STATIC_STRING("Method declared multiple times"),
	RDX_STATIC_STRING("Can not override a virtual method with a static method"),
	RDX_STATIC_STRING("Overriding method has a different return signature than its base"),
	RDX_STATIC_STRING("Methods that override virtual methods must be declared 'final' or 'virtual'"),
	RDX_STATIC_STRING("Method was declared 'final', but doesn't override a virtual method"),
	RDX_STATIC_STRING("Could not match method %s to implement interface %s"),
	RDX_STATIC_STRING("Can not declare properties as static, use 'resource' instead"),
	RDX_STATIC_STRING("Unequal property and initializer counts"),
	RDX_STATIC_STRING("Method defined with a name already being used by a non-method symbol"),
	RDX_STATIC_STRING("Visibility specifier differs from other visible methods"),
	RDX_STATIC_STRING("Static specifier differs from other visible methods"),
	RDX_STATIC_STRING("Intercept specifier differs from other visible methods"),
	RDX_STATIC_STRING("Coerces must return exactly one type"),
	RDX_STATIC_STRING("Intercepts must either return one value and have no parameters, or accept one parameter and return no values"),
	RDX_STATIC_STRING("Abstract methods must be virtual"),
	RDX_STATIC_STRING("Static methods are not allowed in interfaces"),
	RDX_STATIC_STRING("Virtual specifiers are not allowed in interfaces, methods are automatically virtual"),
	RDX_STATIC_STRING("Abstract specifiers are not allowed in interfaces, methods are automatically abstract"),
	RDX_STATIC_STRING("Structures can not contain virtual methods"),
	RDX_STATIC_STRING("Abstract methods can not have code"),
	RDX_STATIC_STRING("Non-abstract non-native methods must have code"),
	RDX_STATIC_STRING("__setindex can not return a value"),
	RDX_STATIC_STRING("Can't extend templates"),
	RDX_STATIC_STRING("Only classes can extend classes"),
	RDX_STATIC_STRING("Only classes can be extended"),
	RDX_STATIC_STRING("Can not extend final classes"),
	RDX_STATIC_STRING("Declared multiple implementations of %s"),
	RDX_STATIC_STRING("Unsupported declaration type %s"),
	RDX_STATIC_STRING("Unsupported type member type %s"),
	RDX_STATIC_STRING("Expected integer constant for enumerator initializer"),
	RDX_STATIC_STRING("Enumerant value %s created by %s is already in use"),
	RDX_STATIC_STRING("Zero value for enumerant not defined"),
	RDX_STATIC_STRING("Couldn't resolve dependency, could be circular"),
	RDX_STATIC_STRING("Expected an expression returning a value"),
	RDX_STATIC_STRING("Expected an expression of at least %s values but only %s were available"),
	RDX_STATIC_STRING("Right-side expression did not evaluate to a value"),
	RDX_STATIC_STRING("Left-side expression did not evaluate to a variable"),
	RDX_STATIC_STRING("Left side of assignment contains %s values, but right contains %s"),
	RDX_STATIC_STRING("Right-side of assignment had fewer values than the left"),
	RDX_STATIC_STRING("Can not assign to a constant"),
	RDX_STATIC_STRING("Can not create locals of varying"),
	RDX_STATIC_STRING("Method requires %s return value(s), but %s were provided"),
	RDX_STATIC_STRING("Could not find a loop to exit with flow control statement"),
	RDX_STATIC_STRING("Switch block with no cases"),
	RDX_STATIC_STRING("Default node specified multiple times"),
	RDX_STATIC_STRING("Case value must be constant"),
	RDX_STATIC_STRING("No cases defined"),
	RDX_STATIC_STRING("Expression statement evaluates to a variable"),
	RDX_STATIC_STRING("Expected exception type"),
	RDX_STATIC_STRING("Exception catch for this type already exists"),
	RDX_STATIC_STRING("Not all control paths return a value"),
	RDX_STATIC_STRING("Type %s has a circular dependency"),
	RDX_STATIC_STRING("By-value struct contains a 'mustberef' struct"),
	RDX_STATIC_STRING("Soft dependencies must be alternating type names and specifiers"),
	RDX_STATIC_STRING("Malformed attribute tag"),
	RDX_STATIC_STRING("Symbols may not be contained within an object with cpp name attributes"),
	RDX_STATIC_STRING("Delegate must be specified as boundto or static"),
	RDX_STATIC_STRING("Delegate was specified as bound to the current class in a non-class declaration space"),
	RDX_STATIC_STRING("Can not delegate a static method to a bound delegate"),
	RDX_STATIC_STRING("Can't pass more values than would be required before 'this' value"),
	RDX_STATIC_STRING("Try block was not followed by any 'catch' or 'finally' blocks"),
	RDX_STATIC_STRING("Package attempted to reference external symbol '%s', which is anonymous and can only be accessed within its own package"),
	RDX_STATIC_STRING("Unexpected end-of-file"),
	RDX_STATIC_STRING("Newline in string constant"),
	RDX_STATIC_STRING("Unknown escape code"),
	RDX_STATIC_STRING("Out of memory"),
	RDX_STATIC_STRING("Unknown symbol"),
	RDX_STATIC_STRING("Unexpected token '%s'"),
	RDX_STATIC_STRING("Expected token of type %s, but found %s '%s'"),
	RDX_STATIC_STRING("Expected '%s', but found '%s'"),
	RDX_STATIC_STRING("Attribute '%s' was declared multiple times"),
	RDX_STATIC_STRING("Incomplete variable declaration"),
	RDX_STATIC_STRING("Expected an identifier or literal, but found '%s'"),
	RDX_STATIC_STRING("Unsupported operator type %s"),
	RDX_STATIC_STRING("Expected 1 expression for operate-and-assign operator"),
	RDX_STATIC_STRING("Attribute tags not followed by a declaration"),
	RDX_STATIC_STRING("Specified dimensions only allowed on the innermost dimension"),
	RDX_STATIC_STRING("Polymorphic conversion requires an explicit cast operator"),
	RDX_STATIC_STRING("Can't initialize an array type using a property constructor"),
	RDX_STATIC_STRING("Can't initialize a structured type using an expression list"),
	RDX_STATIC_STRING("Can't initialize a non-property using a property initializer"),
	RDX_STATIC_STRING("Implemented non-interface '%s'"),
	RDX_STATIC_STRING("Internal compiler error"),
	RDX_STATIC_STRING("Interface implementation expects different return types from '%s'"),
	RDX_STATIC_STRING("No package registered for '%s'"),
	RDX_STATIC_STRING("Resource instance must be the same type as the resource"),
};

static const rdxChar *WARNING_MESSAGES[] =
{
	RDX_STATIC_STRING("No warning"),
	RDX_STATIC_STRING("Couldn't fold constant expression, this probably means that it will cause an exception at runtime"),
	RDX_STATIC_STRING("Loss of precision from parameter conversion"),
	RDX_STATIC_STRING("null passed to notnull parameter, this will cause an exception at runtime"),
	RDX_STATIC_STRING("Local '%s' hides another local in the same function"),
	RDX_STATIC_STRING("Local '%s' is initialized with identifier '%s', which references something else because the local does not exist until the statement completes"),
	RDX_STATIC_STRING("Expression was truncated and the result will not be used"),
	RDX_STATIC_STRING("'is' expression will always return 'false' because the expression and type are incompatible"),
};

void RDXInterface::RDX::Compiler::NativeServices::Error(rdxSExportedCallEnvironment &callEnv, RDXInterface::RDX::Compiler::CodeLocation const & cl,
	RDXInterface::RDX::Compiler::ErrorCode errorCode, rdxWeakRTRef(rdxCString) param1, rdxWeakRTRef(rdxCString) param2, rdxWeakRTRef(rdxCString) param3)
{
	const rdxChar *p1s = NULL;
	const rdxChar *p2s = NULL;
	const rdxChar *p3s = NULL;
	if(param1.IsNotNull())
		p1s = param1->AsChars()->ArrayData();
	if(param2.IsNotNull())
		p2s = param2->AsChars()->ArrayData();
	if(param3.IsNotNull())
		p3s = param3->AsChars()->ArrayData();

	const rdxChar *errorClass = RDX_STATIC_STRING("ERROR: ");
	rdxChar errorCodeChars[5];
	errorCodeChars[4] = static_cast<rdxChar>(0);
	{
		rdxEnumValue codeReduce = errorCode;
		for(int i=0;i<4;i++)
		{
			errorCodeChars[3-i] = static_cast<rdxChar>('0' + errorCode % 10);
			codeReduce /= 10;
		}
	}

	wprintf(RDX_STATIC_STRING("%s%s[%i]: "), errorClass, cl.filename->AsChars(), static_cast<int>(cl.line));
	wprintf(ERROR_MESSAGES[errorCode], p1s, p2s, p3s);
	wprintf(RDX_STATIC_STRING("\n"));
	callEnv.Throw("Core.RDX.Compiler", "TracedErrorException.instance");
}

void RDXInterface::RDX::Compiler::NativeServices::Warning(rdxSExportedCallEnvironment &callEnv, RDXInterface::RDX::Compiler::CodeLocation const & cl, RDXInterface::RDX::Compiler::WarningCode warningCode,
	rdxWeakRTRef(rdxCString) param1, rdxWeakRTRef(rdxCString) param2, rdxWeakRTRef(rdxCString) param3)
{
	const rdxChar *p1s = NULL;
	const rdxChar *p2s = NULL;
	const rdxChar *p3s = NULL;
	if(param1.IsNotNull())
		p1s = param1->AsChars()->ArrayData();
	if(param2.IsNotNull())
		p2s = param2->AsChars()->ArrayData();
	if(param3.IsNotNull())
		p3s = param3->AsChars()->ArrayData();

	const rdxChar *warningClass = RDX_STATIC_STRING("WARNING: ");
	rdxChar warningCodeChars[5];
	warningCodeChars[4] = static_cast<rdxChar>(0);
	{
		rdxEnumValue codeReduce = warningCode;
		for(int i=0;i<4;i++)
		{
			warningCodeChars[3-i] = static_cast<rdxChar>('0' + warningCode % 10);
			codeReduce /= 10;
		}
	}

	wprintf(RDX_STATIC_STRING("%s%s[%i]: "), warningClass, cl.filename->AsChars(), static_cast<int>(cl.line));
	wprintf(WARNING_MESSAGES[warningCode], p1s, p2s, p3s);
	wprintf(RDX_STATIC_STRING("\n"));
}

void RDXInterface::RDX::Compiler::NativeServices::ExportObject(rdxSExportedCallEnvironment &callEnv, rdxWeakRTRef(rdxCString) path, rdxWeakRTRef(rdxCObject) obj)
{
	RDX_TRY(callEnv.ctx)
	{
		rdxIFileStream *fs = rdxGetFileSystem()->Open(path->AsChars()->ArrayData(), true);
		if(!fs)
			return;
		RDX_PROTECT(callEnv.ctx, callEnv.objm->SaveObject(callEnv.ctx, obj.ToWeakHdl(), fs, false));
		fs->Close();
	}
	RDX_CATCH(callEnv.ctx)
	{
	}
	RDX_ENDTRY
}

rdxWeakArrayRTRef(rdxByte) RDXInterface::RDX::Compiler::NativeServices::ReadFile(rdxSExportedCallEnvironment &callEnv, rdxWeakRTRef(rdxCString) path)
{
	rdxIFileStream *fs = rdxGetFileSystem()->Open(path->AsChars()->ArrayData(), false);
	if(!fs)
		return rdxWeakArrayRTRef(rdxByte)::Null();

	RDX_TRY(callEnv.ctx)
	{
		fs->SeekEnd(0);
		rdxLargeUInt fsize = fs->Tell();
		fs->SeekStart(0);

		rdxSObjectGUID byteTypeName = rdxSObjectGUID::FromObjectName("Core", "byte");
		rdxCRef(rdxCStructuredType) byteT;
		RDX_PROTECT_ASSIGN(callEnv.ctx, byteT, callEnv.objm->LookupSymbolSimple(callEnv.ctx, byteTypeName).ReinterpretCast<rdxCStructuredType>());
		if(byteT.IsNull())
		{
			fs->Close();
			return rdxWeakArrayRTRef(rdxByte)::Null();
		}
		rdxCRef(rdxCArrayOfType) byteAOT;
		RDX_PROTECT_ASSIGN(callEnv.ctx, byteAOT, rdxCExternalObjectFactory::AutoCreateArrayType<rdxUInt8>(callEnv.ctx, callEnv.objm, byteT.ToWeakHdl(), 1, true));
		rdxArrayCRef(rdxUInt8) bytes;
		RDX_PROTECT_ASSIGN(callEnv.ctx, bytes, rdxCExternalObjectFactory::Create1DArray<rdxUInt8>(callEnv.ctx, callEnv.objm, fsize, byteAOT.ToWeakHdl(), rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime)));
		if(fs->ReadBytes(bytes->ArrayModify(), fsize) != fsize)
		{
			fs->Close();
			return rdxWeakArrayRTRef(rdxByte)::Null();
		}
		fs->Close();
		return bytes.ToWeakRTRef();
	}
	RDX_CATCH(callEnv.ctx)
	{
		if(fs)
			fs->Close();
		return rdxWeakArrayRTRef(rdxByte)::Null();
	}
	RDX_ENDTRY
}

rdxEnumValue RDXInterface::RDX::Compiler::NativeServices::StrToEnumIntegral(rdxSExportedCallEnvironment &callEnv, rdxWeakRTRef(rdxCString) str)
{
	rdxEnumValue ev;
	rdxDecodeString(str->AsChars()->ArrayData(), ev);
	return ev;
}


void RDXInterface::RDX::Compiler::NativeServices::DebugBreak(rdxSExportedCallEnvironment &callEnv, rdxWeakRTRef(rdxCObject) obj)
{
	rdxDebugBreak(rdxBREAKCAUSE_UserBreak);
}

void RDXInterface::RDX::Compiler::NativeServices::DumpToken(rdxSExportedCallEnvironment &callEnv, rdxWeakRTRef(rdxCString) debugMessage, RDXInterface::RDX::Compiler::Token const & tk)
{
	wprintf(L"%s\n", debugMessage->AsChars()->ArrayData());
	wprintf(L"Type %i  Contents %s\n", static_cast<int>(tk.tokenType), tk.str->AsChars()->ArrayData());
}

template<class Tf>
static void RDXC_NS_EncodeFloat(rdxChar *outChars, rdxWeakRTRef(rdxCString) constantValue)
{
	union
	{
		Tf f;
		rdxInt64 i64;
		rdxInt32 i32;
	} u;
	u.i32 = 0;
	u.i64 = 0;
	rdxDecodeString(constantValue->AsChars()->ArrayData(), u.f);

	if(sizeof(Tf) == sizeof(rdxFloat32))
		rdxEncodeString(outChars, u.i32);
	else if(sizeof(Tf) == sizeof(rdxFloat64))
		rdxEncodeString(outChars, u.i64);
}

void RDXInterface::RDX::Compiler::NativeServices::EncodeConstant(rdxSExportedCallEnvironment &callEnv, RDXInterface::RDX::Compiler::CodeEmission::POp & r_op, rdxLong & r_int1, rdxLong & r_int2, rdxTracedRTRef(rdxCString) & r_res1, rdxTracedRTRef(rdxCString) & r_str1, rdxWeakRTRef(rdxCString) typeName, rdxWeakRTRef(rdxCString) constantValue, RDXInterface::RDX::Compiler::Constant::Signal signal)
{
	RDX_TRY(callEnv.ctx)
	{
		if(signal == RDXInterface::RDX::Compiler::Constant::Signal::Resource)
		{
			r_op = RDXInterface::RDX::Compiler::CodeEmission::POp::Res;
			r_int1 = 0;
			r_int2 = 0;
			r_res1 = typeName;
			r_str1 = rdxWeakRTRef(rdxCString)::Null();
			return;
		}
	
		if(signal == RDXInterface::RDX::Compiler::Constant::Signal::Enum_)
		{
			// Just reuse the int value
			r_op = RDXInterface::RDX::Compiler::CodeEmission::POp::Constant;
			rdxDecodeString(constantValue->AsChars()->ArrayData(), r_int1);
			r_int2 = 0;
			r_res1 = typeName;
			r_str1 = rdxWeakRTRef(rdxCString)::Null();
			return;
		}
	
		if(signal == RDXInterface::RDX::Compiler::Constant::Signal::NullRef)
		{
			r_op = RDXInterface::RDX::Compiler::CodeEmission::POp::Null;
			r_int1 = 0;
			r_int2 = 0;
			r_res1 = rdxWeakRTRef(rdxCString)::Null();
			r_str1 = rdxWeakRTRef(rdxCString)::Null();
			return;
		}
	
		if(signal == RDXInterface::RDX::Compiler::Constant::Signal::Value)
		{
			RDX_STHROW(callEnv.ctx, RDX_ERROR_INVALID_PARAMETER);
		}

		if(typeName->Equal("Value"))
		{
			callEnv.Throw("Core", "RDX.InvalidOperationException.instance");
			return;
		}

		if(typeName->Equal("Core.int") || typeName->Equal("Core.byte") || typeName->Equal("Core.largeint") ||
			typeName->Equal("Core.uint") || typeName->Equal("Core.largeuint"))
		{
			// Just reuse the int value
			r_op = RDXInterface::RDX::Compiler::CodeEmission::POp::Constant;
			rdxDecodeString(constantValue->AsChars()->ArrayData(), r_int1);
			r_int2 = 0;
			r_res1 = typeName;
			r_str1 = rdxWeakRTRef(rdxCString)::Null();
			return;
		}
		else if (typeName->Equal("Core.string"))
		{
			// Store as a string
			r_op = RDXInterface::RDX::Compiler::CodeEmission::POp::ConstantStr;
			r_int1 = 0;
			r_int2 = 0;
			r_res1 = typeName;
			r_str1 = constantValue;
			return;
		}
		else if (typeName->Equal("Core.float"))
		{
			rdxChar str[sizeof(rdxFloat)*3+2];
			RDXC_NS_EncodeFloat<rdxFloat>(str, constantValue);
			r_op = RDXInterface::RDX::Compiler::CodeEmission::POp::Constant;
			r_int1 = 0;
			r_int2 = 0;
			r_res1 = typeName;
			RDX_PROTECT_ASSIGN(callEnv.ctx, r_str1, callEnv.objm->CreateString(callEnv.ctx, str).ToWeakRTRef());
			return;
		}
		else if (typeName->Equal("Core.double"))
		{
			rdxChar str[sizeof(rdxDouble)*3+2];
			RDXC_NS_EncodeFloat<rdxDouble>(str, constantValue);
			r_op = RDXInterface::RDX::Compiler::CodeEmission::POp::Constant;
			r_int1 = 0;
			r_int2 = 0;
			r_res1 = typeName;
			RDX_PROTECT_ASSIGN(callEnv.ctx, r_str1, callEnv.objm->CreateString(callEnv.ctx, str).ToWeakRTRef());
			return;
		}
		else if (typeName->Equal("Core.bool"))
		{
			r_op = RDXInterface::RDX::Compiler::CodeEmission::POp::Constant;
			r_int1 = (constantValue->Equal("false") ? 0 : 1);
			r_int2 = 0;
			r_res1 = typeName;
			r_str1 = rdxWeakRTRef(rdxCString)::Null();
			return;
		}
	}
	RDX_CATCH(callEnv.ctx)
	{
	}
	RDX_ENDTRY

	callEnv.Throw("Core", "RDX.InvalidOperationException.instance");
}


void RDXInterface::RDX::Compiler::NativeServices::DebugMessage(rdxSExportedCallEnvironment &callEnv, rdxWeakRTRef(rdxCString) debugMessage)
{
	wprintf(L"%s\n", debugMessage->AsChars()->ArrayData());
}

namespace ConstFolding
{
	typedef bool (*ConstFoldFunc)(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakRTRef(rdxCString) *outResult, rdxWeakArrayRTRef(rdxTracedRTRef(rdxCString)) parameters);
	typedef rdxCStaticLookupTable<rdxSStaticLookupStringKey<rdxChar, char>, ConstFoldFunc> ConstFoldLUT;

	template<class _T> class AddOp { public: inline static _T Operate(const _T &ls, const _T &rs) { return ls + rs; } };
	template<class _T> class SubOp { public: inline static _T Operate(const _T &ls, const _T &rs) { return ls - rs; } };
	template<class _T> class DivOp { public: inline static _T Operate(const _T &ls, const _T &rs) { return ls / rs; } };
	template<class _T> class MulOp { public: inline static _T Operate(const _T &ls, const _T &rs) { return ls * rs; } };
	template<class _T> class ModOp { public: inline static _T Operate(const _T &ls, const _T &rs) { return ls % rs; } };
	template<class _T> class LTOp { public: inline static bool Operate(const _T &ls, const _T &rs) { return ls < rs; } };
	template<class _T> class LEOp { public: inline static bool Operate(const _T &ls, const _T &rs) { return ls <= rs; } };
	template<class _T> class GTOp { public: inline static bool Operate(const _T &ls, const _T &rs) { return ls > rs; } };
	template<class _T> class GEOp { public: inline static bool Operate(const _T &ls, const _T &rs) { return ls >= rs; } };
	template<class _T> class EqOp { public: inline static bool Operate(const _T &ls, const _T &rs) { return ls == rs; } };
	template<class _T> class NEOp { public: inline static bool Operate(const _T &ls, const _T &rs) { return ls != rs; } };
	template<class _T> class NegOp { public: inline static _T Operate(const _T &v) { return -v; } };
	template<class _T> class NotOp { public: inline static bool Operate(const _T &v) { return !v; } };

	template<class _Ttype>
	int UIntToStr(const _Ttype &v, rdxChar *out)
	{
		rdxChar backstack[(sizeof(_Ttype)*3+2)];
		int digits = 0;
		int chars = 0;

		do
		{
			backstack[digits++] = static_cast<rdxChar>((v % static_cast<_Ttype>(10)) + static_cast<_Ttype>('0'));
			v = v / static_cast<_Ttype>(10);
		} while(v);

		while(digits)
		{
			*out++ = backstack[--digits];
			chars++;
		}

		*out = static_cast<Char>('\0');

		return chars;
	}
	
	template<class _Ttype>
	int IntToStr(const _Ttype &v, rdxChar *out)
	{
		rdxChar backstack[(sizeof(_Ttype)*3+2)];
		int digits = 0;
		int chars = 0;
		bool negative = false;

		if(v < 0)
		{
			*out++ = '-';
			chars++;
			negative = true;
		}

		do
		{
			if(negative)
				backstack[digits++] = static_cast<rdxChar>(-(v % static_cast<_Ttype>(10)) + static_cast<_Ttype>('0'));
			else
				backstack[digits++] = static_cast<rdxChar>((v % static_cast<_Ttype>(10)) + static_cast<_Ttype>('0'));
			v = v / static_cast<_Ttype>(10);
		} while(v);

		while(digits)
		{
			*out++ = backstack[--digits];
			chars++;
		}

		*out = static_cast<rdxChar>('\0');

		return chars;
	}

	enum NumberType
	{
		NT_Float,
		NT_Double,
		NT_Int,
		NT_Long,
	};

	template<class LargeT, class SmallT>
	inline LargeT SVMin()
	{
		SmallT sv = (-1) << (sizeof(SmallT) * 8 - 1);
		return static_cast<LargeT>(sv);
	}

	template<class LargeT, class SmallT>
	inline LargeT SVMax()
	{
		SmallT sv = ~((-1) << (sizeof(SmallT) * 8 - 1));
		return static_cast<LargeT>(sv);
	}

	void SourceCodeStrToNumber(rdxWeakRTRef(rdxCString) str, NumberType *outNumberType, rdxInt64 *outFrac, rdxInt32 *outX)
	{
		const rdxChar *strChars = str->AsChars()->ArrayData();
		bool containsDecimal = false;
		bool endsWithF = false;
		bool containsCarat = false;
		rdxChar rawNumber[201];

		rdxLargeUInt nChars = str->Length();

		if(nChars > 200)
			nChars = 200;

		memcpy(rawNumber, strChars, sizeof(rdxChar)*nChars);
		rawNumber[nChars] = static_cast<rdxChar>(0);

		// See if this contains a decimal
		for(rdxLargeUInt i=0;i<nChars;i++)
		{
			if(rawNumber[i] == static_cast<rdxChar>('.'))
				containsDecimal = true;
			else if(rawNumber[i] == static_cast<rdxChar>('^'))
				containsCarat = true;
			else if(rawNumber[i] == static_cast<rdxChar>(0))
				nChars = i;
		}

		endsWithF = (nChars != 0 && rawNumber[nChars-1] == static_cast<rdxChar>('f'));

		if(containsDecimal || containsCarat || endsWithF)
		{
			if(endsWithF)
			{
				nChars--;
				rawNumber[nChars] = static_cast<rdxChar>(0);
			}

			if(containsCarat)
			{
				if(endsWithF)
				{
					rdxFloat32 fv;
					rdxInt32 ifrac;
					rdxInt32 ix;
					rdxDecodeString(rawNumber, fv);
					rdxDecomposeFloat(fv, ifrac, ix);
					*outX = ix;
					*outFrac = ifrac;
					*outNumberType = NT_Float;
					return;
				}
				else
				{
					rdxFloat64 fv;
					rdxInt64 ifrac;
					rdxInt32 ix;
					rdxDecodeString(rawNumber, fv);
					rdxDecomposeDouble(fv, ifrac, ix);
					*outX = ix;
					*outFrac = ifrac;
					*outNumberType = NT_Double;
					return;
				}
			}
			else if(endsWithF)
			{
				rdxInt32 ifrac, ix;
		
				rdxFloat64 d = rdxStringToDouble(rawNumber, nChars);
				rdxDecomposeFloat(static_cast<rdxFloat32>(d), ifrac, ix);
				*outX = ix;
				*outFrac = ifrac;
				*outNumberType = NT_Double;
				return;
			}
			else
			{
				rdxInt64 ifrac;
				rdxInt32 ix;
		
				rdxFloat64 d = rdxStringToDouble(rawNumber, nChars);
				rdxDecomposeDouble(d, ifrac, ix);
				*outX = ix;
				*outFrac = ifrac;
				*outNumberType = NT_Float;
				return;
			}
		}

		// Integer
		rdxInt64 i;
		rdxDecodeString(rawNumber, i);
		*outX = 0;
		*outFrac = i;

		if(i < SVMin<rdxLong, rdxInt>() || i > SVMax<rdxLong, rdxInt>())
			*outNumberType = NT_Long;
		else if(i < SVMin<rdxLong, rdxShort>() || i > SVMax<rdxLong, rdxShort>())
			*outNumberType = NT_Int;
		else
			*outNumberType = NT_Int;	// TODO: Maybe vary this
	}
	
	template<class _Tinputs, class _Tresult, bool _TzeroCheck, class _Top>
	inline bool BinOpConstFold(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakRTRef(rdxCString) *outResult, rdxWeakArrayRTRef(rdxTracedRTRef(rdxCString)) parameters)
	{
		*outResult = rdxWeakRTRef(rdxCString)::Null();
		RDX_TRY(ctx)
		{
			if(parameters->NumElements() < 2)
				return false;

			rdxChar chars[RDX_MAX_ENCODED_NUMBER_SIZE];
			// TODO: Fix ref constructor conversion :(
			rdxCRef(rdxCString) param0;
			param0 = parameters->Element(0);
			rdxCRef(rdxCString) param1;
			param1 = parameters->Element(1);
			if(param0.IsNull() || param1.IsNull())
				return false;
			_Tinputs ls, rs;
			rdxDecodeString(param1->AsChars()->ArrayData(), rs);
			if(_TzeroCheck && rs == static_cast<_Tinputs>(0))
				return false;

			rdxDecodeString(param1->AsChars()->ArrayData(), ls);
			rdxEncodeString(chars, _Top::Operate(ls, rs));
			rdxCRef(rdxCString) str;
			RDX_PROTECT_ASSIGN(ctx, str, objm->CreateString(ctx, chars));
			*outResult = str;
			return true;
		}
		RDX_CATCH(ctx)
		{
			return false;
		}
		RDX_ENDTRY
	}

	template<class _Tinputs, class _Tresult, class _Top>
	inline bool UnaryOpConstFold(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakRTRef(rdxCString) *outResult, rdxWeakArrayRTRef(rdxTracedRTRef(rdxCString)) parameters)
	{
		*outResult = rdxWeakRTRef(rdxCString)::Null();
		RDX_TRY(ctx)
		{
			if(parameters->NumElements() < 1)
				return false;
			rdxCRef(rdxCString) param0 = parameters->Element(0);
			rdxChar chars[RDX_MAX_ENCODED_NUMBER_SIZE];
			if(param0.IsNull())
				return false;
			_Tinputs v;
			rdxDecodeString(param0->AsChars()->ArrayData(), v);
			rdxEncodeString(chars, _Top::Operate(v));
			CRef<const rdxCString> str;
			RDX_PROTECT_ASSIGN(ctx, str, objm->CreateString(ctx, chars));
			*outResult = str.Object();
			return true;
		}
		RDX_CATCH(ctx)
		{
			return false;
		}
		RDX_ENDTRY
	}


	template<class _Tinputs, class _Tresult>
	inline bool ConvertConstFoldOp(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakRTRef(rdxCString) *outResult, rdxWeakArrayRTRef(rdxTracedRTRef(rdxCString)) parameters)
	{
		*outResult = rdxWeakRTRef(rdxCString)::Null();
		RDX_TRY(ctx)
		{
			if(parameters->NumElements() < 1)
				return false;
			rdxChar chars[RDX_MAX_ENCODED_NUMBER_SIZE];
			rdxCRef(rdxCString) param0;
			param0 = parameters->Element(0);
			if(param0.IsNull())
				return false;
			_Tinputs v;
			rdxDecodeString(param0->AsChars()->ArrayData(), v);
			rdxEncodeString(chars, static_cast<_Tresult>(v));
			rdxCRef(rdxCString) str;
			RDX_PROTECT_ASSIGN(ctx, str, objm->CreateString(ctx, chars));
			*outResult = str;
			return true;
		}
		RDX_CATCH(ctx)
		{
			return false;
		}
		RDX_ENDTRY
	}

	static ConstFoldLUT::Entry constFoldLUTEntries[] =
	{
		{ "Core.int/methods/__add(Core.int)", BinOpConstFold<rdxInt, rdxInt,  false, AddOp<rdxInt> > },
		{ "Core.int/methods/__sub(Core.int)", BinOpConstFold<rdxInt, rdxInt,  false, SubOp<rdxInt> > },
		{ "Core.int/methods/__div(Core.int)", BinOpConstFold<rdxInt, rdxInt,  true,  DivOp<rdxInt> > },
		{ "Core.int/methods/__mul(Core.int)", BinOpConstFold<rdxInt, rdxInt,  false, MulOp<rdxInt> > },
		{ "Core.int/methods/__mod(Core.int)", BinOpConstFold<rdxInt, rdxInt,  true,  ModOp<rdxInt> > },
		{ "Core.int/methods/__gt(Core.int)", BinOpConstFold<rdxInt, bool, false, GTOp<rdxInt> > },
		{ "Core.int/methods/__ge(Core.int)", BinOpConstFold<rdxInt, bool, false, GEOp<rdxInt> > },
		{ "Core.int/methods/__lt(Core.int)", BinOpConstFold<rdxInt, bool, false, LTOp<rdxInt> > },
		{ "Core.int/methods/__le(Core.int)", BinOpConstFold<rdxInt, bool, false, LEOp<rdxInt> > },
		{ "Core.int/methods/__eq(Core.int)", BinOpConstFold<rdxInt, bool, false, EqOp<rdxInt> > },
		{ "Core.int/methods/__ne(Core.int)", BinOpConstFold<rdxInt, bool, false, NEOp<rdxInt> > },

		{ "Core.double/methods/#coerce(Core.float)", ConvertConstFoldOp<rdxDouble, rdxFloat> },
		{ "Core.double/methods/#coerce(Core.int)", ConvertConstFoldOp<rdxDouble, rdxInt> },
		{ "Core.float/methods/#coerce(Core.double)", ConvertConstFoldOp<rdxFloat, rdxDouble> },
		{ "Core.float/methods/#coerce(Core.int)", ConvertConstFoldOp<rdxFloat, rdxInt> },

		{ "Core.string/methods/#coerce(Core.int)", ConvertConstFoldOp<rdxInt, rdxInt> },
		{ "Core.string/methods/#coerce(Core.double)", ConvertConstFoldOp<rdxDouble, rdxDouble> },
	};
	
	ConstFoldLUT constFoldLUT(constFoldLUTEntries, sizeof(constFoldLUTEntries)/sizeof(constFoldLUTEntries[0]));
}

rdxWeakRTRef(rdxCString) RDXInterface::RDX::Compiler::NativeServices::ConstFold(rdxSExportedCallEnvironment &callEnv, rdxWeakRTRef(rdxCString) coerceName, rdxWeakArrayRTRef(rdxTracedRTRef(rdxCString)) values)
{
	rdxSStaticLookupStringKey<rdxChar, rdxChar> strKey(coerceName->AsChars()->ArrayData(), coerceName->Length());
	ConstFolding::ConstFoldFunc cfunc = *ConstFolding::constFoldLUT.Lookup(strKey);
	rdxWeakRTRef(rdxCString) outStr;
	cfunc(callEnv.ctx, callEnv.objm, &outStr, values);
	return outStr;
}

rdxBool RDXInterface::RDX::Compiler::NativeServices::ConstFoldExists(rdxSExportedCallEnvironment &callEnv, rdxWeakRTRef(rdxCString) coerceName)
{
	rdxSStaticLookupStringKey<rdxChar, rdxChar> strKey(coerceName->AsChars()->ArrayData(), coerceName->Length());
	return (ConstFolding::constFoldLUT.Lookup(strKey) != NULL) ? rdxTrueValue : rdxFalseValue;
}

namespace StringEscapes
{
	// Returns length of escape sequence
	rdxLargeUInt EscapeForChar(rdxChar c, rdxChar *outSequence)
	{
		switch(c)
		{
		case '\n':
			if(outSequence)
			{
				outSequence[0] = '\\';
				outSequence[1] = 'n';
			}
			return 2;
		case '\t':
			if(outSequence)
			{
				outSequence[0] = '\\';
				outSequence[1] = 't';
			}
			return 2;
		case '\r':
			if(outSequence)
			{
				outSequence[0] = '\\';
				outSequence[1] = 'r';
			}
			return 2;
		case '\\':
			if(outSequence)
			{
				outSequence[0] = '\\';
				outSequence[1] = '\\';
			}
			return 2;
		case '\"':
			if(outSequence)
			{
				outSequence[0] = '\\';
				outSequence[1] = '\"';
			}
			return 2;
		default:
			if(outSequence)
				outSequence[0] = c;
			return 1;
		}
	}
};

rdxWeakRTRef(rdxCString) RDXInterface::RDX::Compiler::NativeServices::EscapeString(rdxSExportedCallEnvironment &callEnv, rdxWeakRTRef(rdxCString) str)
{
	rdxLargeUInt nChars = str->Length();
	const rdxChar *chars = str->AsChars()->ArrayData();
	rdxLargeUInt encodedLen = 0;
	for(rdxLargeUInt i=0;i<nChars;i++)
	{
		rdxLargeUInt cLen = StringEscapes::EscapeForChar(chars[i], NULL);
		if(!rdxCheckAddOverflowU(encodedLen, cLen))
		{
			callEnv.Throw("Core", "RDX.AllocationFailureException.instance");
			return rdxWeakRTRef(rdxCString)::Null();
		}
		encodedLen += cLen;
	}

	RDX_TRY(callEnv.ctx)
	{
		rdxArrayCRef(rdxChar) charArray;
		RDX_PROTECT_ASSIGN(callEnv.ctx, charArray, rdxCExternalObjectFactory::Create1DArray<rdxChar>(callEnv.ctx, callEnv.objm, encodedLen, callEnv.objm->GetBuiltIns()->aot_Char.ToWeakHdl(), rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime)));
		rdxChar *outChars = charArray->ArrayModify();
		for(rdxLargeUInt i=0;i<nChars;i++)
			outChars += StringEscapes::EscapeForChar(chars[i], outChars);
		rdxCRef(rdxCString) str;
		RDX_PROTECT_ASSIGN(callEnv.ctx, str, callEnv.objm->CreateString(callEnv.ctx, charArray->OffsetElementRTRef(0).ToHdl(), true, encodedLen));

		return str.ToWeakRTRef();
	}
	RDX_CATCH(callEnv.ctx)
	{
		callEnv.Throw("Core", "RDX.AllocationFailureException.instance");
		return rdxWeakRTRef(rdxCString)::Null();
	}
	RDX_ENDTRY
}

void RDXInterface::RDX::Compiler::NativeServices::ParseNumber(rdxSExportedCallEnvironment &callEnv, rdxTracedRTRef(rdxCString) & rValue, rdxTracedRTRef(rdxCString) & rType, rdxWeakRTRef(rdxCString) str)
{
	rValue = rdxWeakRTRef(rdxCString)::Null();
	rType = rdxWeakRTRef(rdxCString)::Null();

	RDX_TRY(callEnv.ctx)
	{
		ConstFolding::NumberType nt;
		rdxInt32 x;
		rdxInt64 frac;
		ConstFolding::SourceCodeStrToNumber(str, &nt, &frac, &x);

		rdxChar chars[RDX_MAX_ENCODED_NUMBER_SIZE];

		rdxLargeUInt nChars = 0;
		nChars += rdxEncodeString(chars, frac);
		if(nt == ConstFolding::NT_Double || nt == ConstFolding::NT_Float)
		{
			chars[nChars++] = '^';
			nChars += rdxEncodeString(chars + nChars, x);
		}

		rdxCRef(rdxCString) outValueStr;
		RDX_PROTECT_ASSIGN(callEnv.ctx, outValueStr, callEnv.objm->CreateString(callEnv.ctx, chars, true, nChars));

		const rdxChar *typeName = NULL;
		switch(nt)
		{
		case ConstFolding::NT_Float:
			typeName = RDX_STATIC_STRING("Core.float");
			break;
		case ConstFolding::NT_Double:
			typeName = RDX_STATIC_STRING("Core.double");
			break;
		case ConstFolding::NT_Int:
			typeName = RDX_STATIC_STRING("Core.int");
			break;
		case ConstFolding::NT_Long:
			typeName = RDX_STATIC_STRING("Core.long");
			break;
		};
		
		RDX_PROTECT_ASSIGN(callEnv.ctx, rType, callEnv.objm->CreateString(callEnv.ctx, typeName));
		rValue = outValueStr;
		return;
	}
	RDX_CATCH(callEnv.ctx)
	{
		callEnv.Throw("Core", "RDX.AllocationFailureException.instance");
		return;
	}
	RDX_ENDTRY
}

rdxLargeUInt RDXInterface::RDX::Compiler::NativeServices::StrToLargeUInt(rdxSExportedCallEnvironment &callEnv, rdxWeakRTRef(rdxCString) str)
{
	rdxLargeUInt i;
	rdxDecodeString(str->AsChars()->ArrayData(), i);
	return i;
}

static int enumSort(const void *a, const void *b)
{
	RDXInterface::RDX::Compiler::EnumerantObject const*const*pa = static_cast<RDXInterface::RDX::Compiler::EnumerantObject const*const*>(a);
	RDXInterface::RDX::Compiler::EnumerantObject const*const*pb = static_cast<RDXInterface::RDX::Compiler::EnumerantObject const*const*>(b);

	if((*pa)->value < (*pb)->value)
		return -1;
	if((*pa)->value > (*pb)->value)
		return 1;
	return 0;
}

void RDXInterface::RDX::Compiler::NativeServices::SortEnumerants(rdxSExportedCallEnvironment &callEnv, rdxWeakArrayRTRef(rdxTracedRTRef(RDXInterface::RDX::Compiler::EnumerantObject)) enumerants)
{
	// TODO: Fix this
	rdxLargeUInt nEnumerants = enumerants->NumElements();
	rdxTracedRTRef(RDXInterface::RDX::Compiler::EnumerantObject) *enumContents = enumerants->ArrayModify();

	rdxWeakRTRef(RDXInterface::RDX::Compiler::EnumerantObject) temp;
	
	for(rdxLargeUInt i=0;i<nEnumerants;i++)
	{
		rdxCRef(RDXInterface::RDX::Compiler::EnumerantObject) initialEnumerant;
		initialEnumerant = enumContents[i];

		if(initialEnumerant.IsNull())
		{
			callEnv.Throw("Core", "RDX.NullReferenceException.instance");
			return;
		}

		rdxLargeUInt lowestIndex = i;
		rdxEnumValue lowestValue = initialEnumerant->value;
		for(rdxLargeUInt j=i+1;j<nEnumerants;j++)
		{
			rdxCRef(RDXInterface::RDX::Compiler::EnumerantObject) compareE;
			compareE = enumContents[j].ToWeakRTRef();

			if(compareE.IsNull())
			{
				callEnv.Throw("Core", "RDX.NullReferenceException.instance");
				return;
			}

			rdxEnumValue compareValue = compareE->value;
			if(compareValue < lowestValue)
			{
				lowestValue = compareValue;
				lowestIndex = j;
			}
		}
		if(lowestIndex != i)
		{
			enumContents[i] = enumContents[lowestIndex];
			enumContents[lowestIndex] = initialEnumerant;
		}
	}
}
