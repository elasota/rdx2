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
#ifndef __RDX_PLATFORM_HPP__
#define __RDX_PLATFORM_HPP__

#include <ctype.h>

#define RDX_DYNLIB_EXPORT		extern "C" __declspec(dllexport)
#define RDX_DYNLIB_IMPORT		extern "C" __declspec(dllimport)
#define RDX_DYNLIB_EXPORT_CPP	__declspec(dllexport)
#define RDX_DYNLIB_IMPORT_CPP	__declspec(dllimport)



// Macro that may execute the statement v, dumps some internal information
#define RDX_VERBOSE(v)

// Enable this to disable UTF8 decoding.  Characters will be decoded as single bytes from UTF8 strings.
// You should set RDX_CHAR_TYPE to char if you do this
//#define RDX_UNICODE_DISABLE_UTF8

// Enable this to use wchar_t for Unicode system-specific services, where applicable (i.e. Windows)
#define RDX_WCHAR_SYSTEM_SERVICES

// Character type used by RDX strings
#define RDX_CHAR_TYPE			wchar_t

// Macro that declares a string of RDX_CHAR_TYPE
#define RDX_STATIC_STRING(n)	L##n

// Define this to journal values that do not contain any references.  Increases memory requirements, but allows
// threads to be serialized and provides more debugging information.
#define RDX_JOURNAL_ALL_VALUES

// Define this to enable tail call optimization.  If RDX_JOURNAL_ALL_VALUES is defined, then "this" parameters
// and pointers can only be optimized if they pass through
// (DEPRECATED)
//#define RDX_TAIL_CALL_OPTIMIZATION

// Enable this to allow usage of the timeslice counter, which can cause threads to time out if they take too long.
#define RDX_ENABLE_TIMESLICE_COUNTER

// Enable this to enable symmetric multiprocessing operations.  This will allow RDX threads to run concurrently under
// the same object manager, but will degrade the performance of some operations.
//#define RDX_ENABLE_SMP

// File system path delimiter
#define RDX_FILESYSTEM_DELIMITER	'/'

// Maximum depth of recursive functions before failure is forced (i.e. ref type checks on arrays)
#define RDX_MAX_RECURSION_DEPTH		20

// Define this as RDX_BYTE_ORDER_LITTLE_ENDIAN or RDX_BYTE_ORDER_BIG_ENDIAN
#define RDX_BINARY_STORAGE_ORDER	RDX_BYTE_ORDER_LITTLE_ENDIAN


// "Core.int" data type.  Must be no larger than a pointer.
#define RDX_INT_TYPE			RDX_SINT32_TYPE

// "Core.uint" data type.  Must be no larger than a pointer.
#define RDX_UINT_TYPE			RDX_UINT32_TYPE

// "Core.float" type (can be set to double)
#define RDX_FLOAT_TYPE			RDX_FLOAT32_TYPE

// Type that enums are directly convertable to and from.
// This should match "enumType" in rdx_compile.lua and the "value" property of Enumerant in Internal.rdx
#define RDX_ENUM_TYPE			RDX_UINT_TYPE
#define RDX_ENUM_INTERNAL_TYPE	st_UInt

// This should be the largest possible integer type and it should be at least as large as any other type defined here
#define RDX_HUGEINT_TYPE		RDX_SINT64_TYPE
#define RDX_HUGEUINT_TYPE		RDX_UINT64_TYPE

#define RDX_SINT8_TYPE			char
#define RDX_UINT8_TYPE			unsigned char
#define RDX_SINT16_TYPE			short
#define RDX_UINT16_TYPE			unsigned short
#define RDX_SINT32_TYPE			int
#define RDX_UINT32_TYPE			unsigned int
#define RDX_SINT64_TYPE			long long
#define RDX_UINT64_TYPE			unsigned long long
#define RDX_FLOAT32_TYPE		float
#define RDX_FLOAT64_TYPE		double

#define RDX_BOOL_TYPE			unsigned char
#define RDX_TRUE_VALUE			1
#define RDX_FALSE_VALUE			0

// Call attribute for API callbacks
#define RDX_DECL_API			__stdcall

// Aligning attribute
#define RDX_ALIGN_ATTRIBUTE(n)		__declspec(align(n))

// Maximum alignment that an object could ever be allocated with
static const int RDX_MAX_ALIGNMENT		=	16;

// Signed integer the size of a pointer
// This must be at least as large as RDX_INT_TYPE
#define RDX_LARGEINT_TYPE			ptrdiff_t

// Unsigned integer the size of a pointer
// This must be at least as large as RDX_INT_TYPE
#define RDX_LARGEUINT_TYPE			size_t

#define RDX_LARGEINT_MAX			(static_cast<rdxLargeInt>( ((1 << (sizeof(rdxLargeInt) * 8 - 2)) - 1) * 2 + 1))
#define RDX_LARGEINT_MIN			(static_cast<rdxLargeInt>((-1) << (sizeof(rdxLargeInt) * 8 - 1)))
#define RDX_LARGEUINT_MAX			(static_cast<rdxLargeUInt>( ((static_cast<rdxLargeUInt>(1) << (sizeof(rdxLargeUInt) * 8 - 1)) - 1) * 2 + 1))

// Maximum number of dimensions an array can contain without causing a dynamic allocation
#define RDX_MAX_DIMENSIONS			10

// Value that Threading atomic operations can operate on.  Always declared volatile.
#define RDX_ATOMICINT_NV_TYPE		rdxLargeInt

// Value that is expected to appear at the start of RDXB files
#define RDX_MAGIC					"RDXBinaryPackage"

// Signed integer type used to store rdxLargeInt values in binary package files
// This allows a degree of portability in binary packages, but values that can not be converted successfully
// may cause an abort when saving or cause a integer overflow error when loading
#define RDX_BINARY_STORAGE_INT_TYPE		rdxInt64
#define RDX_BINARY_STORAGE_UINT_TYPE	rdxUInt64

// Maximum number of characters that an encoded number could contain.
// Remember that floats are encoded as a separate mantissa and exponent, plus a carat
#define RDX_MAX_ENCODED_NUMBER_SIZE	50

// Define this if it's OK to export duplicable methods as precompiled code.
// You should only do this if untrustworthy packages will not be able to load bad duplicated methods.
// This generally means one or more of:
// - All packages exported with ExportSource are permanently loaded, and loaded before anything else
// - No packages are loaded from untrusted sources
// - Packages from untrusted sources are loaded with CanContainMethods returning false
#define RDX_ALLOW_PRECOMPILED_DUPLICATES	1

// Define this to disable all internal protection checks, i.e. anything that uses RDX_PROTECT, RDX_TRY, and RDX_CATCH
//#define RDX_DISABLE_SANITY_CHECKS

// Define this to use C++ exceptions instead of protection macros
// Errors will result in references to the offending rdxSOperationContext being thrown
//#define RDX_USE_CPP_EXCEPTIONS			1

// Define this if wchar_t is treated as a separate type by overloads (as opposed to aliasing something like "unsigned short")
#define RDX_WCHAR_T_IS_NATIVE

// Define this to dump disassembly even if RDX_VERBOSE is set to do nothing
//#define RDX_DEBUG_NONVERBOSE_DISASSEMBLE

// Define this to enable sentinels around allocated ranges.  This will allow AuditSentinels to be used to debug overruns.
//#define RDX_ENABLE_SENTINELS

// Define this to validate that deserialized enumerant values are valid.  Disabling this will allow structures and arrays with
// enums to be bulk serialized, but may cause invalid enumerants to be loaded if the source data is untrusted.  Program behavior
// when enumerants are invalid is undefined and potentially unsafe.  Using this may also affect compatibility in binary serialized
// packages if bulk serialization is enabled, since it will prevent structs that contain enums from being bulk serialized.
#define RDX_VALIDATE_DESERIALIZED_ENUMS

// Define this to enable bulk serialization for structures that include padding.  Doing this will increase the size of files
// and cause platform-specific alignment dependencies, but may speed up deserialization.  Note that this interacts with
// RDX_VALIDATE_DESERIALIZED_ENUMS as well, since using that flag will prevent structures that include any enums from being
// bulk deserialized.
//#define RDX_ENABLE_PADDED_BULK_SERIALIZE

// Define this to enable bulk serialization for rdxLargeInt.  Doing this will cause RDX_BINARY_STORAGE_INT_TYPE to be ignored
// and handled as if it were defined as RDX_LARGEINT_TYPE, and will cause packages to be incompatible with packages from
// environments with different address sizes.
//#define RDX_ENABLE_LARGEINT_BULK_SERIALIZE

// Define this to enable bulk serialization of booleans.  Doing this will remove the guarantee that boolean values that test
// equivalently are also the same when compared if a package is deserialized containing an illegal boolean.
//#define RDX_ENABLE_BOOL_BULK_SERIALIZE

// Define this to use MurmurHash3 for fast hashes
#define RDX_HASH_MURMUR3

// Largest allowed compressed bytecode size.
#define RDX_COMPRESSED_BYTECODE_MAXSIZE		(4*1024*1024 - 1)

// Define this to make rdxRef use rdxHandle instead, speeds up compaction but worsens cache usage
//#define RDX_USE_HANDLES_INTERNALLY

// Define this to enable incremental garbage collection.  Makes garbage collection take more time
// overall and occasionally let garbage survive, but prevents allows collection to be amortized.
#define RDX_USE_INCREMENTAL_GC

#endif
