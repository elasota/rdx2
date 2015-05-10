#ifndef __RDX_CORETYPES_HPP__
#define __RDX_CORETYPES_HPP__

#include "rdx_platform.hpp"

#define RDX_CNULL			0

typedef RDX_CHAR_TYPE		rdxChar;
typedef RDX_INT_TYPE		rdxInt;							// Needs to be no larger than rdxLargeInt
typedef RDX_UINT_TYPE		rdxUInt;						// Needs to be no larger than rdxLargeInt
typedef RDX_FLOAT_TYPE		rdxFloat;
typedef RDX_UINT8_TYPE		rdxByte;
typedef RDX_SINT16_TYPE		rdxShort;
typedef RDX_UINT16_TYPE		rdxUShort;
typedef RDX_FLOAT64_TYPE	rdxDouble;
typedef RDX_UINT32_TYPE		rdxHashValue;
typedef RDX_SINT64_TYPE		rdxLong;
typedef RDX_UINT64_TYPE		rdxULong;
typedef RDX_BOOL_TYPE		rdxBool;

typedef RDX_LARGEINT_TYPE	rdxLargeInt;					// Needs to be the same size as a pointer
typedef RDX_LARGEUINT_TYPE	rdxLargeUInt;					// Needs to be the same size as a pointer
typedef RDX_SINT64_TYPE		rdxInt64;
typedef	RDX_UINT64_TYPE		rdxUInt64;
typedef RDX_SINT32_TYPE		rdxInt32;
typedef RDX_SINT16_TYPE		rdxInt16;
typedef RDX_SINT8_TYPE		rdxInt8;
typedef RDX_UINT32_TYPE		rdxUInt32;
typedef RDX_UINT16_TYPE		rdxUInt16;
typedef RDX_UINT8_TYPE		rdxUInt8;
typedef RDX_FLOAT64_TYPE	rdxFloat64;
typedef RDX_FLOAT32_TYPE	rdxFloat32;


#ifdef RDX_ENABLE_LARGEINT_BULK_SERIALIZE
	typedef RDX_LARGEINT_TYPE	rdxBinPackageLargeInt;
	typedef RDX_LARGEUINT_TYPE	rdxBinPackageLargeUInt;
#else
	typedef RDX_BINARY_STORAGE_INT_TYPE	rdxBinPackageLargeInt;
	typedef RDX_BINARY_STORAGE_UINT_TYPE rdxBinPackageLargeUInt;
#endif

typedef RDX_HUGEINT_TYPE	rdxHugeInt;
typedef RDX_HUGEUINT_TYPE	rdxHugeUInt;

static const rdxBool rdxTrueValue = RDX_TRUE_VALUE;
static const rdxBool rdxFalseValue = RDX_FALSE_VALUE;

static const rdxChar rdxCHAR_Invalid = static_cast<rdxChar>(~static_cast<rdxChar>(0));

typedef RDX_ENUM_TYPE rdxEnumValue;

#include "rdx_langutils.hpp"

static const rdxLargeUInt rdxALIGN_LargeInt		= rdxAlignOf(rdxLargeInt);
static const rdxLargeUInt rdxALIGN_Bool			= rdxAlignOf(rdxBool);
static const rdxLargeUInt rdxALIGN_HashValue	= rdxAlignOf(rdxHashValue);
static const rdxLargeUInt rdxALIGN_Float		= rdxAlignOf(rdxFloat);
static const rdxLargeUInt rdxALIGN_Int			= rdxAlignOf(rdxInt);
static const rdxLargeUInt rdxALIGN_Char			= rdxAlignOf(rdxChar);
static const rdxLargeUInt rdxALIGN_Short		= rdxAlignOf(rdxShort);
static const rdxLargeUInt rdxALIGN_Byte			= rdxAlignOf(rdxByte);
static const rdxLargeUInt rdxALIGN_EnumValue	= rdxAlignOf(rdxEnumValue);

typedef rdxUInt8 rdxScanID;

#endif
