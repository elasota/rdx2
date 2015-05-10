#include "../rdx/rdx_pragmas.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <direct.h>
#include <time.h>
#include "../rdx/rdx_guid.hpp"
#include "../rdx/rdx_objectguid.hpp"

#include "../lua/src/lua.h"
#include "../rdx/rdx_basictypes.hpp"
#include "../lz4/lz4hc.h"

#define BACKSTACK_SIZE	(sizeof(rdxHugeInt)*3+2)

int closeCacheFile(lua_State *L);
int writeCacheObject(lua_State *L);
int readCacheObject(lua_State *L);
int createCacheFile(lua_State *L);
rdxInt64 rdxMSecTime();

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

template<class LargeT, class SmallT>
inline LargeT UVMax()
{
	SmallT sv = ~static_cast<SmallT>(0);
	return static_cast<LargeT>(sv);
}

int myatoll(const char *str, rdxHugeUInt &out, bool &outIsNegative)
{
	const char *baseStr = str;

	bool sign = false;
	if(str[0] == '-')
	{
		sign = true;
		str++;
	}
	rdxHugeUInt rv = 0;
	while(str[0] >= '0' && str[0] <= '9')
	{
		rv = rv * rdxHugeUInt(10) + static_cast<rdxHugeUInt>(str[0] - '0');
		str++;
	}
	outIsNegative = sign;
	out = rv;
	return str - baseStr;
}

int myatoll(const char *str, rdxHugeInt &out)
{
	rdxHugeUInt hugeU;
	bool isNegative;
	int result = myatoll(str, hugeU, isNegative);
	rdxHugeInt hugeS = static_cast<rdxHugeInt>(hugeU);
	if(isNegative)
		hugeS = -hugeS;
	out = hugeS;
	return result;
}

int IntToString(char *out, rdxHugeInt v)
{
	char backstack[BACKSTACK_SIZE];
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
			backstack[digits++] = static_cast<char>(-(v % rdxHugeInt(10)) + '0');
		else
			backstack[digits++] = static_cast<char>((v % rdxHugeInt(10)) + '0');
		v = v / rdxHugeInt(10);
	} while(v);

	while(digits)
	{
		*out++ = backstack[--digits];
		chars++;
	}

	*out = '\0';

	return chars;
}

int UIntToString(char *out, rdxHugeUInt v)
{
	char backstack[BACKSTACK_SIZE];
	int digits = 0;
	int chars = 0;

	do
	{
		backstack[digits++] = static_cast<char>((v % rdxHugeUInt(10)) + '0');
		v = v / rdxHugeUInt(10);
	} while(v);

	while(digits)
	{
		*out++ = backstack[--digits];
		chars++;
	}

	*out = '\0';

	return chars;
}

void DecomposedToString(char *out, rdxInt64 frac, rdxInt32 x)
{
	out += IntToString(out, frac);
	if(x)
	{
		*out++ = '^';
		out += IntToString(out, x);
	}
	*out = '\0';
}


void DecomposeString(const char *str, rdxInt64 &frac, rdxInt32 &x)
{
	str += myatoll(str, frac);

	if(*str == '^')
	{
		str++;
		rdxHugeInt v;
		myatoll(str, v);
		x = static_cast<rdxInt32>(v);
	}
	else
		x = 0;
}

void DecomposeFloat(float f, rdxInt32 &frac, rdxInt32 &x)
{
	union
	{
		float f;
		rdxInt32 i;
	} RDX_MAY_ALIAS u;

	u.f = f;
	if(u.i == 0)
	{
		frac = 0;
		x = 0;
		return;
	}

	rdxInt32 baseFrac = u.i & 0x7FFFFF;
	baseFrac |= 0x800000;
	rdxInt32 baseX = (u.i & 0x7F800000) >> 23;

	while(!(baseFrac & 1))
	{
		baseX++;
		baseFrac >>= 1;
	}

	x = baseX - 150;

	if((u.i & 0x80000000) != 0)
		baseFrac = -baseFrac;
	frac = baseFrac;
}

float RecomposeFloat(rdxInt32 frac, rdxInt32 x)
{
	union
	{
		float f;
		rdxInt32 i;
	} RDX_MAY_ALIAS u;

	if(frac == 0)
		return 0;

	bool sign = false;
	if(frac < 0)
	{
		sign = true;
		frac = -frac;
	}

	x += 150;

	while(!(frac & 0x800000))
	{
		x--;
		frac <<= 1;
	}

	frac &= 0x7FFFFF;
	u.i = frac | (x << 23);
	if(sign)
		u.i |= 0x80000000;

	return u.f;
}

void DecomposeDouble(double f, rdxInt64 &frac, rdxInt32 &x)
{
	union
	{
		double f;
		rdxInt64 i;
	} RDX_MAY_ALIAS u;

	u.f = f;
	if(u.i == 0)
	{
		frac = 0;
		x = 0;
		return;
	}

	rdxInt64 baseFrac = u.i & 0xFFFFFFFFFFFFFLL;
	baseFrac |= 0x10000000000000LL;
	long baseX = static_cast<rdxInt32>( (u.i & 0x7FF0000000000000LL) >> 52LL);

	while(!(baseFrac & 1))
	{
		baseX++;
		baseFrac >>= 1LL;
	}

	x = baseX - 1075;

	if((u.i & 0x8000000000000000LL) != 0)
		baseFrac = -baseFrac;
	frac = baseFrac;
}


double RecomposeDouble(rdxInt64 frac, rdxInt32 x)
{
	union
	{
		double f;
		rdxInt64 i;
	} RDX_MAY_ALIAS u;

	x += 1075;
	
	bool sign = false;
	if(frac < 0LL)
	{
		sign = true;
		frac = -frac;
	}

	while(!(frac & 0x10000000000000LL))
	{
		x--;
		frac <<= 1LL;
	}

	frac &= 0xFFFFFFFFFFFFFLL;
	u.i = frac | (static_cast<rdxInt64>(x) << 52LL);
	if(sign)
		u.i |= 0x8000000000000000LL;

	return u.f;
}



void RecomposeVariant(rdxInt64 frac, rdxInt32 x, float &out)
{
	out = RecomposeFloat(static_cast<rdxInt32>(frac), x);
}

void RecomposeVariant(rdxInt64 frac, rdxInt32 x, double &out)
{
	out = RecomposeDouble(frac, x);
}

// StrToNumber template overloads
void StrToNumber(const char *str, double &out)
{
	rdxInt64 frac;
	rdxInt32 x;
	DecomposeString(str, frac, x);
	out = RecomposeDouble(frac, x);
}

void StrToNumber(const char *str, float &out)
{
	rdxInt64 frac;
	rdxInt32 x;
	DecomposeString(str, frac, x);
	out = RecomposeFloat(static_cast<rdxInt32>(frac), x);
}

void StrToNumber(const char *str, rdxHugeInt &out)
{
	myatoll(str, out);
}

void StrToNumber(const char *str, rdxInt32 &out)
{
	rdxHugeInt temp;
	myatoll(str, temp);
	out = static_cast<rdxInt32>(temp);
}

void StrToNumber(const char *str, rdxInt16 &out)
{
	rdxHugeInt temp;
	myatoll(str, temp);
	out = static_cast<rdxInt16>(temp);
}

void StrToNumber(const char *str, rdxInt8 &out)
{
	rdxHugeInt temp;
	myatoll(str, temp);
	out = static_cast<rdxInt8>(temp);
}

void StrToNumber(const char *str, rdxUInt64 &out)
{
	rdxHugeInt temp;
	myatoll(str, temp);
	out = static_cast<rdxUInt64>(temp);
}

void StrToNumber(const char *str, rdxUInt32 &out)
{
	rdxHugeInt temp;
	myatoll(str, temp);
	out = static_cast<rdxUInt32>(temp);
}

void StrToNumber(const char *str, rdxUInt16 &out)
{
	rdxHugeInt temp;
	myatoll(str, temp);
	out = static_cast<rdxUInt16>(temp);
}

void StrToNumber(const char *str, rdxUInt8 &out)
{
	rdxHugeInt temp;
	myatoll(str, temp);
	out = static_cast<rdxUInt8>(temp);
}

void StrToNumber(const char *str, bool &out)
{
	out = (strcmp(str, "false") != 0);
}

// NumberToStr overloads
void NumberToStr(double v, char *out)
{
	rdxInt64 frac;
	rdxInt32 x;
	DecomposeDouble(v, frac, x);
	DecomposedToString(out, frac, x);
}

void NumberToStr(float v, char *out)
{
	rdxInt32 frac;
	rdxInt32 x;
	DecomposeFloat(v, frac, x);
	DecomposedToString(out, frac, x);
}

void NumberToStr(rdxInt64 v, char *out)
{
	IntToString(out, v);
}

void NumberToStr(rdxInt32 v, char *out)
{
	IntToString(out, v);
}

void NumberToStr(rdxInt16 v, char *out)
{
	IntToString(out, v);
}

void NumberToStr(rdxInt8 v, char *out)
{
	IntToString(out, v);
}

void NumberToStr(rdxUInt64 v, char *out)
{
	UIntToString(out, v);
}

void NumberToStr(rdxUInt32 v, char *out)
{
	UIntToString(out, v);
}

void NumberToStr(rdxUInt16 v, char *out)
{
	UIntToString(out, v);
}

void NumberToStr(rdxUInt8 v, char *out)
{
	UIntToString(out, v);
}

void NumberToStr(bool v, char *out)
{
	strcpy(out, v ? "true" : "false");
}

void HexStringifyLong(rdxUInt32 v, char *cout)
{
	static const char *nibbles = "0123456789abcdef";
	int i;
	for(i=0;i<8;i++)
		cout[i] = nibbles[ (v >> (i*4)) & 0xf ];
}


void HexStringifyByte(rdxUInt8 v, char *cout)
{
	static const char *nibbles = "0123456789abcdef";
	int i;
	for(i=0;i<2;i++)
		cout[i] = nibbles[ (v >> ((2-1-i)*4)) & 0xf ];
}

int sha256(lua_State *L)
{
	size_t len;
	const char *str = lua_tolstring(L, 1, &len);
	rdxUInt8 data[32];

	rdxCSHA256Generator generator;
	generator.FeedBytes(str, len);
	generator.Flush(data);

	char output[64];

	for(int i=0;i<32;i++)
		HexStringifyByte(data[i], output + i * 2);

	lua_pushlstring(L, output, 64);
	return 1;
}

int computeguid(lua_State *L)
{
	size_t len;
	const char *str = lua_tolstring(L, 1, &len);
	rdxUInt8 data[rdxSDomainGUID::GUID_SIZE];
	char output[rdxSDomainGUID::GUID_SIZE * 2];

	RDX_ComputeGUID(str, data);
		
	for(int i=0;i<rdxSDomainGUID::GUID_SIZE;i++)
	{
		rdxUInt8 b = data[i];
		HexStringifyByte(b, output + i*2);
	}

	lua_pushlstring(L, output, rdxSDomainGUID::GUID_SIZE * 2);
	return 1;
}

/*
int crc32(lua_State *L)
{
	size_t len;
	const char *str = lua_tolstring(L, 1, &len);
	char output[8];
	
	rdxCCRC32Generator generator;
	generator.FeedBytes(str, len);
	rdxUInt32 crc = generator.Flush();
		
	for(int i=0;i<4;i++)
		HexStringifyByte(static_cast<rdxUInt8>((crc >> (3 - i)) * 0xff), output + i*2);

	lua_pushlstring(L, output, 8);
	return 1;
}
*/

int parseNumber(lua_State *L)
{
	const char *str = lua_tostring(L, 1);
	const char *subStr = str;
	bool containsDecimal = false;
	bool endsWithF = false;
	bool endsWithS = false;
	bool containsCarat = false;
	char numericOutput[50];
	char rawNumber[201];

	if(!str || strlen(str) >= 200)
		return 0;

	rdxLargeUInt nChars = strlen(str);
	strcpy(rawNumber, str);

	// See if this contains a decimal
	while(*subStr)
	{
		if(subStr[0] == '.')
			containsDecimal = true;
		else if(subStr[0] == '^')
			containsCarat = true;
		subStr++;
	}

	endsWithF = (str[0] != '\0' && subStr[-1] == 'f');
	endsWithS = (str[0] != '\0' && subStr[-1] == 'S');

	if(containsDecimal || containsCarat || endsWithF)
	{
		if(endsWithF)
		{
			nChars--;
			rawNumber[nChars] = '\0';
		}

		if(containsCarat)
		{
			// Just recycle it
			lua_pushstring(L, rawNumber);
			if(endsWithF)
				lua_pushstring(L, "Core.float");
			else
				lua_pushstring(L, "Core.double");
			return 2;
		}
		else if(endsWithF)
		{
			rdxInt32 frac, x;
		
			double d = atof(rawNumber);
			DecomposeFloat(static_cast<float>(d), frac, x);

			DecomposedToString(numericOutput, frac, x);
			lua_pushstring(L, numericOutput);
			lua_pushstring(L, "Core.float");
			return 2;
		}
		else
		{
			rdxInt64 frac;
			rdxInt32 x;
		
			double d = atof(rawNumber);
			DecomposeDouble(d, frac, x);

			DecomposedToString(numericOutput, frac, x);
			lua_pushstring(L, numericOutput);
			lua_pushstring(L, "Core.double");
			return 2;
		}
	}

	// Integer
	if(endsWithS)
	{
		nChars--;
		rawNumber[nChars] = '\0';
	}

	bool isNegative;
	rdxHugeUInt i;
	myatoll(str, i, isNegative);

	// TODO: Parse failures, including negative U
	if(isNegative)
	{
		if(i != 0)
			i = ~static_cast<rdxHugeUInt>(i - static_cast<rdxHugeUInt>(1));
	}

	bool isUnsigned = true;
	//if(i < UVMax<rdxHugeUInt, rdxByte>())
	//	isUnsigned = true;
	if(endsWithS || isNegative)
		isUnsigned = false;

	// TODO: This should be replaced when better integral constant support is added
	if(isUnsigned)
	{
		UIntToString(numericOutput, i);
		lua_pushstring(L, numericOutput);

		if(i > UVMax<rdxHugeUInt, rdxUInt>())
			lua_pushstring(L, "Core.ulong");
		else if(i > UVMax<rdxHugeUInt, rdxUShort>())
			lua_pushstring(L, "Core.uint");
		else if(i > UVMax<rdxHugeUInt, rdxByte>())
			lua_pushstring(L, "Core.uint");
		else
			lua_pushstring(L, "Core.uint");
	}
	else
	{
		rdxHugeInt si = static_cast<rdxHugeInt>(i);
		IntToString(numericOutput, si);
		lua_pushstring(L, numericOutput);

		if(si < SVMin<rdxLong, rdxInt>() || si > SVMax<rdxLong, rdxInt>())
			lua_pushstring(L, "Core.long");
		else if(si < SVMin<rdxLong, rdxShort>() || si > SVMax<rdxLong, rdxShort>())
			lua_pushstring(L, "Core.int");
		else 
			lua_pushstring(L, "Core.int");	// TODO: Maybe vary this
	}

	return 2;
}

int cf_coerce_noop(lua_State *L)
{
	lua_pushvalue(L, 1);
	return 1;
}

#define BIN_OP(opName, op)	\
	template<class _T, class _Tout>\
	struct BinOp##opName\
	{\
		typedef _T InType;\
		typedef _Tout OutType;\
		inline static bool Operate(_T left, _T right, bool zeroCheck, _Tout &out)\
		{\
			if(zeroCheck && right == static_cast<_T>(0))\
				return false;\
			out = static_cast<_Tout>(left op right);\
			return true;\
		}\
	}

#define UN_OP(opName, op)	\
	template<class _T>\
	struct UnOp##opName\
	{\
		typedef _T OpType;\
		inline static bool Operate(_T v, _T &out)\
		{\
			out = op v;\
			return true;\
		}\
	}

BIN_OP(Add, +);
BIN_OP(Subtract, -);
BIN_OP(Multiply, *);
BIN_OP(Divide, /);
BIN_OP(Modulo, %);
BIN_OP(GE, >=);
BIN_OP(GT, >);
BIN_OP(LE, <=);
BIN_OP(LT, <);
BIN_OP(EQ, ==);
BIN_OP(NE, !=);
UN_OP(Negate, -);


template<class TbinOp, bool zeroCheck>
int ConstFoldBinaryOp(lua_State *L)
{
	TbinOp::InType left;
	TbinOp::InType right;
	TbinOp::OutType out;
	StrToNumber(lua_tostring(L, 1), left);
	StrToNumber(lua_tostring(L, 2), right);
	bool succeeded = TbinOp::Operate(left, right, zeroCheck, out);
	if(!succeeded)
		lua_pushnil(L);
	else
	{
		char number[100];
		NumberToStr(out, number);
		lua_pushstring(L, number);
	}
	return 1;
}

template<class TsourceType, class TdestType>
int ConstFoldConvertOp(lua_State *L)
{
	TsourceType v;
	StrToNumber(lua_tostring(L, 1), v);

	char number[100];
	NumberToStr(static_cast<TdestType>(v), number);
	lua_pushstring(L, number);
	return 1;
}

template<class TsourceType>
int IntToStringOp(lua_State *L)
{
	TsourceType v;
	StrToNumber(lua_tostring(L, 1), v);

	char number[RDX_MAX_ENCODED_NUMBER_SIZE];
	NumberToStr(v, number);
	lua_pushstring(L, number);
	return 1;
}

template<class TunOp>
int ConstFoldUnaryOp(lua_State *L)
{
	TunOp::OpType v;
	TunOp::OpType out;
	StrToNumber(lua_tostring(L, 1), v);
	bool succeeded = TunOp::Operate(v, out);
	if(!succeeded)
		lua_pushnil(L);
	else
	{
		char number[100];
		NumberToStr(out, number);
		lua_pushstring(L, number);
	}
	return 1;
}

template<class Tf>
int encodeFloat(const char *coreType, lua_State *L)
{
	rdxInt64 frac;
	rdxInt32 x;

	DecomposeString(lua_tostring(L, 2), frac, x);

	Tf fval;
	RecomposeVariant(frac, x, fval);
	
	union { Tf f; rdxInt64 i64; rdxInt32 i32; } RDX_MAY_ALIAS u;
	u.i32 = 0;
	u.i64 = 0;
	u.f = fval;

	char str[BACKSTACK_SIZE];

	if(sizeof(Tf) == sizeof(rdxFloat32))
		IntToString(str, u.i32);
	else if(sizeof(Tf) == sizeof(rdxFloat64))
		IntToString(str, u.i64);
	else
	{
		lua_pushstring(L, "constant");
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushstring(L, coreType);
		lua_pushvalue(L, 2);
		return 5;
	}

	// Push the converted value
	lua_pushstring(L, "constant");
	lua_pushstring(L, str);
	lua_pushnil(L);
	lua_pushstring(L, coreType);
	lua_pushnil(L);
	return 5;
}

void vliEncode(rdxInt64 value, unsigned char *bytesOut, size_t *outByteCount)
{
	rdxInt64 stopValue;

	if(value < 0)
		stopValue = -1;
	else
		stopValue = 0;

	size_t numPieces = 0;
	rdxInt64 stopTest = value;
	do
	{
		stopTest = stopTest >> 7;
		numPieces++;
	} while(stopTest != stopValue);

	// The final value is sign extended, so make sure the last bit will extend to the correct value
	if(value < 0 && (value & (static_cast<rdxInt64>(1) << (numPieces * 7 - 1))) == 0)
		numPieces++;
	else if(value >= 0 && (value & (static_cast<rdxInt64>(1) << (numPieces * 7 - 1))) != 0)
		numPieces++;

	for(size_t i=0;i<numPieces;i++)
	{
		rdxUInt8 encoded = static_cast<rdxUInt8>((value >> (numPieces - i - 1)*7) & 0x7f);
		if(i != numPieces - 1)
			encoded |= 0x80;
		*bytesOut++ = encoded;
	}
	*outByteCount = numPieces;
}

struct dataPacker
{
	size_t dataSize;
	size_t capacity;
	void *bytes;

	dataPacker()
	{
		capacity = 64;
		dataSize = 0;
		bytes = realloc(NULL, capacity);
	}

	~dataPacker()
	{
		bytes = realloc(bytes, 0);
	}

	void AddBytes(const void *inBytes, size_t sz)
	{
		if(dataSize + sz > capacity)
		{
			while(dataSize + sz > capacity)
				capacity *= 2;
			bytes = realloc(bytes, capacity);
		}
		memcpy(static_cast<rdxUInt8*>(bytes) + dataSize, inBytes, sz);
		dataSize += sz;
	}

	void Compress()
	{
		char *compressedBuffer = static_cast<char*>(malloc(dataSize));
		size_t compressedSize = static_cast<size_t>(LZ4_compressHC2_limitedOutput(static_cast<const char*>(bytes), compressedBuffer, static_cast<int>(dataSize), static_cast<int>(dataSize), 16));
		size_t ucSizeSz;
		rdxUInt8 ucSize[sizeof(rdxHugeInt)*2];
		vliEncode(static_cast<rdxInt64>(dataSize), ucSize, &ucSizeSz);

		if(compressedSize <= 0 || ucSizeSz + static_cast<size_t>(compressedSize) >= 1 + dataSize)
		{
			// Failed to compress, or expanded
			char *outBytes = static_cast<char*>(malloc(dataSize + 1));
			memcpy(outBytes + 1, bytes, dataSize);
			outBytes[0] = '\0';
			free(bytes);
			bytes = outBytes;
			dataSize = dataSize + 1;
		}
		else
		{
			// Successfully compressed
			char *outBytes = static_cast<char*>(malloc(compressedSize + ucSizeSz));
			memcpy(outBytes, ucSize, ucSizeSz);
			memcpy(outBytes + ucSizeSz, compressedBuffer, compressedSize);
			dataSize = ucSizeSz + compressedSize;
			free(bytes);
			bytes = outBytes;
		}
		free(compressedBuffer);
	}
};

int createBytecodePacker(lua_State *L)
{
	dataPacker *bpacker = new dataPacker();
	lua_pushlightuserdata(L, bpacker);
	return 1;
}

int bytecodePackerAddInteger(lua_State *L)
{
	dataPacker *bpacker = static_cast<dataPacker*>(lua_touserdata(L, 1));
	rdxInt64 parsed;

	if(lua_type(L, 2) == LUA_TNUMBER)
		parsed = lua_tointeger(L, 2);
	else
		myatoll(lua_tostring(L, 2), parsed);

	size_t vliSize;
	rdxUInt8 vliInt[sizeof(rdxHugeInt)*2];
	vliEncode(parsed, vliInt, &vliSize);

	bpacker->AddBytes(vliInt, vliSize);

	return 0;
}

int finishBytecodePacker(lua_State *L)
{
	dataPacker *bpacker = static_cast<dataPacker*>(lua_touserdata(L, 1));
	bpacker->Compress();
	lua_pushlstring(L, static_cast<const char *>(bpacker->bytes), bpacker->dataSize);
	delete bpacker;
	return 1;
}

int makeBytecodeASCII(lua_State *L)
{
	size_t len;
	const rdxUInt8 *srcBytecode = reinterpret_cast<const rdxUInt8*>(lua_tolstring(L, 1, &len));

	dataPacker packer;
	for(size_t i=0;i<len;i++)
	{
		char nBuf[6];
		sprintf(nBuf, "%3d, ", srcBytecode[i]);
		packer.AddBytes(nBuf, strlen(nBuf));
		if((i & 15) == 15)
			packer.AddBytes("\n", 1);
	}
	if((len & 15) != 15)
		packer.AddBytes("\n", 1);
	lua_pushlstring(L, static_cast<const char *>(packer.bytes), packer.dataSize);
	return 1;
}

// (type, value, signal)
// returns opcode, int1, int2, res1, str1
int encodeConstant(lua_State *L)
{
	const char *signal = lua_tostring(L, 3);
	if(!strcmp(signal, "Resource"))
	{
		lua_pushstring(L, "res");
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushvalue(L, 2);
		lua_pushnil(L);
		return 5;
	}

	if(!strcmp(signal, "Enum"))
	{
		// Just reuse the int value
		lua_pushstring(L, "constant");
		lua_pushvalue(L, 2);
		lua_pushnil(L);
		lua_pushvalue(L, 1);
		lua_pushnil(L);
		return 5;
	}

	if(!strcmp(signal, "NullRef"))
	{
		lua_pushstring(L, "null");
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushnil(L);
		return 5;
	}

	if(strcmp(signal, "Value"))
	{
		lua_pushstring(L, "Unknown signal for constant encoding");
		lua_error(L);
	}

	const char *coreType = lua_tostring(L, 1);

	if(!strcmp(coreType, "Core.int") || !strcmp(coreType, "Core.byte") || !strcmp(coreType, "Core.largeint") ||
		!strcmp(coreType, "Core.uint") || !strcmp(coreType, "Core.largeuint"))
	{
		// Just reuse the int value
		lua_pushstring(L, "constant");
		lua_pushvalue(L, 2);
		lua_pushnil(L);
		lua_pushstring(L, coreType);
		lua_pushnil(L);
		return 5;
	}
	else if (!strcmp(coreType, "Core.string"))
	{
		// Store as a string
		lua_pushstring(L, "constant_str");
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushstring(L, coreType);
		lua_pushvalue(L, 2);
		return 5;
	}
	else if (!strcmp(coreType, "Core.float"))
	{
		return encodeFloat<rdxFloat>(coreType, L);
	}
	else if (!strcmp(coreType, "Core.double"))
	{
		return encodeFloat<rdxDouble>(coreType, L);
	}
	else if (!strcmp(coreType, "Core.bool"))
	{
		const char *str = lua_tostring(L, 2);
		lua_pushstring(L, "constant");
		if(!strcmp(str, "false"))
			lua_pushstring(L, "0");
		else
			lua_pushstring(L, "1");
		lua_pushnil(L);
		lua_pushstring(L, coreType);
		lua_pushnil(L);
		return 5;
	}

	lua_pushstring(L, "Invalid constant for encoding");
	lua_error(L);

	return 0;
}


int mkdirFunc(lua_State *L)
{
	_mkdir(lua_tostring(L, 1));
	return 0;
}

class lexState
{
	int				m_eofFlag;
	lua_Integer		m_lineNumber;
	lua_Integer		m_offset;
	lua_Integer		m_max;
	const rdxUInt8	*m_str;
	const char		*m_errorMessage;
	const char		*m_returnStr;
	size_t			m_returnStrSize;
	char			*m_returnStrBuf;
	size_t			m_returnStrCapacity;
	const char		*m_tokenType;

public:
	lexState(int eofFlag, lua_Integer lineNumber, lua_Integer offset, lua_Integer max, const char *str);
	~lexState();
	
	void NextChar();
	rdxUInt8 Peek1();
	bool Peek2(rdxUInt8 &out);
	bool Check2(rdxUInt8 b1, rdxUInt8 b2);
	void LexSingle();
	bool AppendChar(rdxUInt8 c);

	static bool IsDigit(rdxUInt8 c);
	static bool IsNameStartChar(rdxUInt8 c);
	static bool IsNameChar(rdxUInt8 c);

	bool MatchPunctuation();
	int ExportState(lua_State *L);
};

lexState::lexState(int eofFlag, lua_Integer lineNumber, lua_Integer offset, lua_Integer max, const char *str)
{
	m_eofFlag = eofFlag;
	m_lineNumber = lineNumber;
	m_offset = offset;
	m_max = max;
	m_str = reinterpret_cast<const rdxUInt8 *>(str);
	m_errorMessage = NULL;
	m_returnStr = NULL;
	m_returnStrBuf = NULL;
	m_returnStrSize = 0;
	m_returnStrCapacity = 0;
	m_tokenType = NULL;
}

lexState::~lexState()
{
	if(m_returnStrBuf)
		realloc(m_returnStrBuf, 0);
}

void lexState::NextChar()
{
	bool wasCR = false;
	rdxUInt8 thisC = m_str[m_offset - 1];
	if(thisC == '\n')
		m_lineNumber++;
	if(thisC == '\r')
	{
		m_lineNumber++;
		wasCR = true;
	}
	m_offset++;
	if(m_offset > m_max)
		m_eofFlag = 1;

	if(wasCR && !m_eofFlag && m_str[m_offset - 1] == '\n')
		m_offset++;	// Windows CR/LF line break
}

rdxUInt8 lexState::Peek1()
{
	return m_str[m_offset - 1];
}

bool lexState::Peek2(rdxUInt8 &out)
{
	if(m_offset >= m_max)
		return false;
	out = m_str[m_offset];
	return true;
}

bool lexState::Check2(rdxUInt8 b1, rdxUInt8 b2)
{
	if(m_offset == m_max)
		return false;
	return (m_str[m_offset - 1] == b1) && (m_str[m_offset] == b2);
}

bool lexState::AppendChar(rdxUInt8 c)
{
	if(m_returnStrSize == m_returnStrCapacity)
	{
		m_returnStrCapacity *= 2;
		if(m_returnStrCapacity == 0)
			m_returnStrCapacity = 16;
		if(m_returnStrCapacity > RDX_LARGEINT_MAX)
			return false;
		void *newBuf = realloc(m_returnStrBuf, m_returnStrCapacity);
		if(!newBuf)
			return false;
		m_returnStrBuf = static_cast<char *>(newBuf);
		m_returnStr = m_returnStrBuf;
	}
	m_returnStrBuf[m_returnStrSize++] = static_cast<char>(c);
	return true;
}

bool lexState::IsDigit(rdxUInt8 c)
{
	return (c >= '0' && c <= '9');
}

bool lexState::IsNameStartChar(rdxUInt8 c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool lexState::IsNameChar(rdxUInt8 c)
{
	return IsNameStartChar(c) || IsDigit(c);
}

bool lexState::MatchPunctuation()
{
	const char *matchStr = NULL;
	rdxUInt8 b1, b2;
	bool has2;
	b1 = Peek1();
	has2 = Peek2(b2);
	switch(b1)
	{
	case ':':
		if(has2 && b2 == '<')
		{
			matchStr = ":<";
			break;
		}
		matchStr = ":";
		break;
	case '<':
		if(has2 && b2 == '=')
		{
			matchStr = "<=";
			break;
		}
		matchStr = "<";
		break;
	case '>':
		if(has2 && b2 == '=')
		{
			matchStr = ">=";
			break;
		}
		matchStr = ">";
		break;
	case '=':
		if(has2 && b2 == '=')
		{
			matchStr = "==";
			break;
		}
		matchStr = "=";
		break;
	case '!':
		if(has2 && b2 == '=')
		{
			matchStr = "!=";
			break;
		}
		matchStr = "!";
		break;
	case '+':
		if(has2)
		{
			if(b2 == '=')
			{
				matchStr = "+=";
				break;
			}
			if(b2 == '+')
			{
				matchStr = "++";
				break;
			}
		}
		matchStr = "+";
		break;
	case '-':
		if(has2)
		{
			if(b2 == '=')
			{
				matchStr = "-=";
				break;
			}
			if(b2 == '-')
			{
				matchStr = "--";
				break;
			}
		}
		matchStr = "-";
		break;
	case '*':
		if(has2 && b2 == '=')
		{
			matchStr = "*=";
			break;
		}
		matchStr = "*";
		break;
	case '/':
		if(has2 && b2 == '=')
		{
			matchStr = "/=";
			break;
		}
		matchStr = "/";
		break;
	case '%':
		if(has2 && b2 == '=')
		{
			matchStr = "%=";
			break;
		}
		matchStr = "%";
		break;
	case '&':
		if(has2 && b2 == '&')
		{
			matchStr = "&&";
			break;
		}
		matchStr = "&";
		break;
	case '|':
		if(has2 && b2 == '|')
		{
			matchStr = "||";
			break;
		}
		matchStr = "|";
		break;
	case ';': matchStr = ";"; break;
	case ',': matchStr = ","; break;
	case '{': matchStr = "{"; break;
	case '}': matchStr = "}"; break;
	case '[': matchStr = "["; break;
	case ']': matchStr = "]"; break;
	case '(': matchStr = "("; break;
	case ')': matchStr = ")"; break;
	case '.': matchStr = "."; break;
	case '?': matchStr = "?"; break;
	}

	if(matchStr)
	{
		m_returnStr = matchStr;
		m_returnStrSize = 0;
		while(*matchStr)
		{
			NextChar();
			m_returnStrSize++;
			matchStr++;
		}
		m_tokenType = "Punctuation";
		return true;
	}

	return false;
}

void lexState::LexSingle()
{
	bool moreWS = true;
	while(moreWS)
	{
		// Skip over whitespace
		rdxUInt8 wsByte = ' ';

		while(Peek1() <= wsByte)
		{
			NextChar();
			if(m_eofFlag)
				return;
		}

		moreWS = false;

		if(Check2('/', '/'))
		{
			rdxUInt8 b = Peek1();
			while(b != '\n' && b != '\r')
			{
				NextChar();
				if(m_eofFlag)
					return;
				b = Peek1();
			}
			moreWS = true;
		}

		if(Check2('/', '*'))
		{
			NextChar();
			NextChar();
			while(!Check2('*', '/'))
			{
				NextChar();
				if(m_eofFlag)
				{
					m_errorMessage = "UnexpectedEOF";
					return;
				}
			}
			NextChar();
			NextChar();
			moreWS = true;
		}

		if(!moreWS)
			break;
	}

	// Long strings
	if(Peek1() == '\"')
	{
		while(true)
		{
			NextChar();
			if(m_eofFlag)
			{
				m_errorMessage = "UnexpectedEOF";
				return;
			}
			rdxUInt8 c = Peek1();
			if(c == '\n' || c == '\r')
			{
				m_errorMessage = "NewlineInStringConstant";
				return;
			}
			if(c == '\"')
				break;
			if(c == '\\')
			{
				NextChar();
				if(m_eofFlag)
				{
					m_errorMessage = "UnexpectedEOF";
					return;
				}

				rdxUInt8 escapeC = Peek1();
				switch(escapeC)
				{
				case 'n':
					c = '\n';
					break;
				case 'r':
					c = '\r';
					break;
				case 't':
					c = '\t';
					break;
				case '\"':
					c = '\"';
					break;
				case '\\':
					c = '\\';
					break;
				default:
					{
						m_errorMessage = "UnknownEscape";
						return;
					}
				}
			}
			if(!AppendChar(c))
			{
				m_errorMessage = "MemoryAllocationFailure";
				return;
			}
		}
		NextChar();
		m_tokenType = "String";
		return;
	}

	if(IsDigit(Peek1()))
	{
		bool printNum = false;
		lua_Integer strStart = m_offset;
		lua_Integer strEnd = m_offset;
		while(true)
		{
			NextChar();
			if(m_eofFlag)
				break;
			rdxUInt8 c = Peek1();
			if(!IsDigit(c) && c != '.' && c != 'f' && c != '^')
				break;
			if(c == '^')
			{
				if(Check2('^', '-'))
				{
					strEnd = strEnd + 1;
					NextChar();	// Skip one
					c = Peek1();
					printNum = true;
				}
			}
			strEnd++;
		}
		m_tokenType = "Number";
		m_returnStr = reinterpret_cast<const char *>(m_str + strStart - 1);
		m_returnStrSize = static_cast<size_t>(strEnd - strStart + 1);
		return;
	}

	if(IsNameStartChar(Peek1()))
	{
		lua_Integer strStart = m_offset;
		lua_Integer strEnd = m_offset;

		while(true)
		{
			NextChar();
			if(m_eofFlag)
				break;
			rdxUInt8 c = Peek1();
			if(!IsNameChar(c))
				break;
			strEnd++;
		}
		
		m_returnStr = reinterpret_cast<const char *>(m_str + strStart - 1);
		m_returnStrSize = static_cast<size_t>(strEnd - strStart + 1);
		m_tokenType = "Name";
		return;
	}

	if(MatchPunctuation())
		return;

	m_errorMessage = "UnknownSymbol";
}

// In state: eof, lineNumber, offset, max, str
// Out state: eof, lineNumber, offset, error message, token type, token text
int lexState::ExportState(lua_State *L)
{
	lua_pushboolean(L, m_eofFlag);
	lua_pushinteger(L, m_lineNumber);
	lua_pushinteger(L, m_offset);
	if(m_errorMessage) lua_pushstring(L, m_errorMessage); else lua_pushnil(L);
	if(m_tokenType) lua_pushstring(L, m_tokenType); else lua_pushnil(L);
	if(m_returnStr) lua_pushlstring(L, m_returnStr, m_returnStrSize); else lua_pushnil(L);
	return 6;
}


int lexSingleToken(lua_State *L)
{
	int eofFlag = lua_toboolean(L, 1);
	lua_Integer lineNumber = lua_tointeger(L, 2);
	lua_Integer offset = lua_tointeger(L, 3);
	lua_Integer max = lua_tointeger(L, 4);
	const char *str = NULL;
	if(!lua_isnoneornil(L, 5)) str = lua_tostring(L, 5);

	lexState ls(eofFlag, lineNumber, offset, max, str);
	ls.LexSingle();
	return ls.ExportState(L);
}


int msecTime(lua_State *L)
{
	lua_pushinteger(L, static_cast<lua_Integer>(rdxMSecTime()));
	return 1;
}

extern "C" __declspec(dllexport) int RegisterRDXC(lua_State *L)
{
	lua_createtable(L, 0, 0);
	lua_createtable(L, 0, 0);
	lua_pushcfunction(L, parseNumber);
	lua_setfield(L, -2, "parseNumber");

	lua_pushcfunction(L, encodeConstant);
	lua_setfield(L, -2, "encodeConstant");

	//lua_pushcfunction(L, encodeBytecodeInteger);
	//lua_setfield(L, -2, "encodeBytecodeInteger");

	lua_pushcfunction(L, createBytecodePacker);
	lua_setfield(L, -2, "createBytecodePacker");
	
	lua_pushcfunction(L, bytecodePackerAddInteger);
	lua_setfield(L, -2, "bytecodePackerAddInteger");

	lua_pushcfunction(L, finishBytecodePacker);
	lua_setfield(L, -2, "finishBytecodePacker");

	lua_pushcfunction(L, makeBytecodeASCII);
	lua_setfield(L, -2, "makeBytecodeASCII");

	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpAdd<rdxInt, rdxInt>, false >));
	lua_setfield(L, -2, "cf_Core.int/methods/__add(Core.int)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpSubtract<rdxInt, rdxInt>, false >));
	lua_setfield(L, -2, "cf_Core.int/methods/__sub(Core.int)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpMultiply<rdxInt, rdxInt>, false >));
	lua_setfield(L, -2, "cf_Core.int/methods/__mul(Core.int)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpDivide<rdxInt, rdxInt>, true >));
	lua_setfield(L, -2, "cf_Core.int/methods/__div(Core.int)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpModulo<rdxInt, rdxInt>, true >));
	lua_setfield(L, -2, "cf_Core.int/methods/__mod(Core.int)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpGT<rdxInt, bool>, false >));
	lua_setfield(L, -2, "cf_Core.int/methods/__gt(Core.int)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpLT<rdxInt, bool>, false >));
	lua_setfield(L, -2, "cf_Core.int/methods/__lt(Core.int)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpGE<rdxInt, bool>, false >));
	lua_setfield(L, -2, "cf_Core.int/methods/__ge(Core.int)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpLE<rdxInt, bool>, false >));
	lua_setfield(L, -2, "cf_Core.int/methods/__le(Core.int)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpEQ<rdxInt, bool>, false >));
	lua_setfield(L, -2, "cf_Core.int/methods/__eq(Core.int)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpNE<rdxInt, bool>, false >));
	lua_setfield(L, -2, "cf_Core.int/methods/__ne(Core.int)");
	
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpAdd<rdxDouble, rdxDouble>, false >));
	lua_setfield(L, -2, "cf_Core.double/methods/__add(Core.double)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpSubtract<rdxDouble, rdxDouble>, false >));
	lua_setfield(L, -2, "cf_Core.double/methods/__sub(Core.double)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpMultiply<rdxDouble, rdxDouble>, false >));
	lua_setfield(L, -2, "cf_Core.double/methods/__mul(Core.double)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpDivide<rdxDouble, rdxDouble>, false >));
	lua_setfield(L, -2, "cf_Core.double/methods/__div(Core.double)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpGT<rdxDouble, bool>, false >));
	lua_setfield(L, -2, "cf_Core.double/methods/__gt(Core.double)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpLT<rdxDouble, bool>, false >));
	lua_setfield(L, -2, "cf_Core.double/methods/__lt(Core.double)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpGE<rdxDouble, bool>, false >));
	lua_setfield(L, -2, "cf_Core.double/methods/__ge(Core.double)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpLE<rdxDouble, bool>, false >));
	lua_setfield(L, -2, "cf_Core.double/methods/__le(Core.double)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpEQ<rdxDouble, bool>, false >));
	lua_setfield(L, -2, "cf_Core.double/methods/__eq(Core.double)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpNE<rdxDouble, bool>, false >));
	lua_setfield(L, -2, "cf_Core.double/methods/__ne(Core.double)");

	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpAdd<rdxFloat, rdxFloat>, false >));
	lua_setfield(L, -2, "cf_Core.float/methods/__add(Core.float)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpSubtract<rdxFloat, rdxFloat>, false >));
	lua_setfield(L, -2, "cf_Core.float/methods/__sub(Core.float)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpMultiply<rdxFloat, rdxFloat>, false >));
	lua_setfield(L, -2, "cf_Core.float/methods/__mul(Core.float)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpDivide<rdxFloat, rdxFloat>, false >));
	lua_setfield(L, -2, "cf_Core.float/methods/__div(Core.float)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpGT<rdxFloat, bool>, false >));
	lua_setfield(L, -2, "cf_Core.float/methods/__gt(Core.float)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpLT<rdxFloat, bool>, false >));
	lua_setfield(L, -2, "cf_Core.float/methods/__lt(Core.float)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpGE<rdxFloat, bool>, false >));
	lua_setfield(L, -2, "cf_Core.float/methods/__ge(Core.float)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpLE<rdxFloat, bool>, false >));
	lua_setfield(L, -2, "cf_Core.float/methods/__le(Core.float)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpEQ<rdxFloat, bool>, false >));
	lua_setfield(L, -2, "cf_Core.float/methods/__eq(Core.float)");
	lua_pushcfunction(L, (ConstFoldBinaryOp<BinOpNE<rdxFloat, bool>, false >));
	lua_setfield(L, -2, "cf_Core.float/methods/__ne(Core.float)");

	lua_pushcfunction(L, ConstFoldUnaryOp<UnOpNegate<rdxInt> >);
	lua_setfield(L, -2, "cf_Core.int/methods/__neg()");
	lua_pushcfunction(L, ConstFoldUnaryOp<UnOpNegate<rdxFloat> >);
	lua_setfield(L, -2, "cf_Core.float/methods/__neg()");
	lua_pushcfunction(L, ConstFoldUnaryOp<UnOpNegate<rdxDouble> >);
	lua_setfield(L, -2, "cf_Core.double/methods/__neg()");

	lua_pushcfunction(L, (ConstFoldConvertOp<rdxInt, rdxFloat>));
	lua_setfield(L, -2, "cf_Core.int/methods/#coerce(Core.float)");
	
	lua_pushcfunction(L, (ConstFoldConvertOp<rdxDouble, rdxFloat>));
	lua_setfield(L, -2, "cf_Core.double/methods/#coerce(Core.float)");
	lua_pushcfunction(L, (ConstFoldConvertOp<rdxDouble, rdxInt>));
	lua_setfield(L, -2, "cf_Core.double/methods/#coerce(Core.int)");

	lua_pushcfunction(L, (ConstFoldConvertOp<rdxFloat, rdxDouble>));
	lua_setfield(L, -2, "cf_Core.float/methods/#coerce(Core.double)");
	lua_pushcfunction(L, (ConstFoldConvertOp<rdxFloat, rdxInt>));
	lua_setfield(L, -2, "cf_Core.float/methods/#coerce(Core.int)");

	lua_pushcfunction(L, (ConstFoldConvertOp<rdxInt, rdxLargeInt>));
	lua_setfield(L, -2, "cf_Core.int/methods/#coerce(Core.largeint)");
	lua_pushcfunction(L, (ConstFoldConvertOp<rdxInt, rdxUShort>));
	lua_setfield(L, -2, "cf_Core.int/methods/#coerce(Core.ushort)");
	lua_pushcfunction(L, (ConstFoldConvertOp<rdxInt, rdxByte>));
	lua_setfield(L, -2, "cf_Core.int/methods/#coerce(Core.byte)");
	lua_pushcfunction(L, (IntToStringOp<rdxInt>));
	lua_setfield(L, -2, "cf_Core.int/methods/#coerce(Core.string)");
	
	lua_pushcfunction(L, (ConstFoldConvertOp<rdxUInt, rdxLargeUInt>));
	lua_setfield(L, -2, "cf_Core.uint/methods/#coerce(Core.largeuint)");

	lua_pushcfunction(L, (ConstFoldConvertOp<rdxShort, rdxInt>));
	lua_setfield(L, -2, "cf_Core.short/methods/#coerce(Core.int)");

	// String-to-number ops should just be handled as const fold converts to the same type
	lua_pushcfunction(L, (ConstFoldConvertOp<rdxInt, rdxInt>));
	lua_setfield(L, -2, "cf_Core.string/methods/#coerce(Core.int)");

	lua_pushcfunction(L, sha256);
	lua_setfield(L, -2, "sha256");
	//lua_pushcfunction(L, crc32);
	//lua_setfield(L, -2, "crc32");
	lua_pushcfunction(L, computeguid);
	lua_setfield(L, -2, "computeguid");

	
	lua_pushcfunction(L, closeCacheFile);
	lua_setfield(L, -2, "closeCacheFile");
	lua_pushcfunction(L, writeCacheObject);
	lua_setfield(L, -2, "writeCacheObject");
	lua_pushcfunction(L, createCacheFile);
	lua_setfield(L, -2, "createCacheFile");
	lua_pushcfunction(L, readCacheObject);
	lua_setfield(L, -2, "readCacheObject");
	lua_pushcfunction(L, mkdirFunc);
	lua_setfield(L, -2, "mkdir");
	lua_pushcfunction(L, lexSingleToken);
	lua_setfield(L, -2, "lexSingleToken");
	
	lua_pushcfunction(L, msecTime);
	lua_setfield(L, -2, "msecTime");

	
	lua_setfield(L, -2, "Native");
	lua_setglobal(L, "RDXC");

	return 0;
}
