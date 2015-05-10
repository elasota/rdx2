#include <string.h>

#include "rdx_basictypes.hpp"
#include "rdx_guid.hpp"
#include "rdx_assert.hpp"

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
	rdxUInt32 crc = ~(*outCRC);	// ~previous crc

	while(n--)
		crc = crctable[(crc ^ (*bytes++)) & 0xff] ^ (crc >> 8);
	*outCRC = ~crc;
}

class rdxXAPI_CRC32_Shim
{
private:
	static void Init(rdxCCRC32Generator *self);
	static void FeedBytes(rdxCCRC32Generator *self, const void *ptr, rdxLargeUInt sz);
	static rdxUInt32 Flush(rdxCCRC32Generator *self);
};

void rdxXAPI_CRC32_Shim::Init(rdxCCRC32Generator *self)
{
	self->m_crc = 0;
}

void rdxXAPI_CRC32_Shim::FeedBytes(rdxCCRC32Generator *self, const void *ptr, rdxLargeUInt sz)
{
	CRC32(&self->m_crc, ptr, sz);
}

rdxUInt32 rdxXAPI_CRC32_Shim::Flush(rdxCCRC32Generator *self)
{
	return self->m_crc;
}

static RDX_FORCEINLINE rdxUInt32 ShiftRightRotate(rdxUInt32 v, int count)
{
	return ((v >> count) | (v << (32 - count))) & 0xffffffff;
}

static void SHA256Cycle(rdxUInt32 data[8], rdxUInt32 seed, int round)
{
	static const rdxUInt32 k[] = {
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
		0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
		0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
		0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
		0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
		0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
		0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
	};

	rdxUInt32 ch = (data[4] & data[5]) ^ ((~data[4]) & data[6]);
	rdxUInt32 s1 = ShiftRightRotate(data[4], 6) ^ ShiftRightRotate(data[4], 11) ^ ShiftRightRotate(data[4], 25);
	rdxUInt32 combine1 = k[round] + seed + ch + data[7] + s1;
	rdxUInt32 ma = (data[0] & data[1]) ^ (data[0] & data[2]) ^ (data[1] & data[2]);
	rdxUInt32 s0 = ShiftRightRotate(data[0], 2) ^ ShiftRightRotate(data[0], 13) ^ ShiftRightRotate(data[0], 22);
	data[7] = data[6];
	data[6] = data[5];
	data[5] = data[4];
	data[4] = data[3] + combine1;
	data[3] = data[2];
	data[2] = data[1];
	data[1] = data[0];
	data[0] = combine1 + ma + s0;
}

class rdxXAPI_SHA256_Shim
{
public:
	static void Init(rdxCSHA256Generator *self);
	static void FeedBytes(rdxCSHA256Generator *self, const void *ptr, rdxLargeUInt sz);
	static void Flush(rdxCSHA256Generator *self, rdxUInt8 *output);
};

RDX_UTIL_DYNLIB_API void rdxXAPI_SHA256_Init(rdxCSHA256Generator *self)
{
	rdxXAPI_SHA256_Shim::Init(self);
}

void rdxXAPI_SHA256_Shim::Init(rdxCSHA256Generator *self)
{
	self->m_state[0] = 0x6a09e667;
	self->m_state[1] = 0xbb67ae85;
	self->m_state[2] = 0x3c6ef372;
	self->m_state[3] = 0xa54ff53a;
	self->m_state[4] = 0x510e527f;
	self->m_state[5] = 0x9b05688c;
	self->m_state[6] = 0x1f83d9ab;
	self->m_state[7] = 0x5be0cd19;
	self->m_messageLength = 0;
	self->m_seedOffset = 0;
	for(int i=0;i<16;i++)
		self->m_seeds[i] = 0;
}


RDX_UTIL_DYNLIB_API void rdxXAPI_SHA256_FeedBytes(rdxCSHA256Generator *self, const void *ptr, rdxLargeUInt n)
{
	rdxXAPI_SHA256_Shim::FeedBytes(self, ptr, n);
}

void rdxXAPI_SHA256_Shim::FeedBytes(rdxCSHA256Generator *self, const void *ptr, rdxLargeUInt n)
{
	rdxUInt32 *seeds = self->m_seeds;
	rdxLargeUInt seedOffset = self->m_seedOffset;
	rdxLargeUInt messageLength = self->m_messageLength;
	const rdxUInt8 *input = static_cast<const rdxUInt8 *>(ptr);

	while(n--)
	{
		self->m_seeds[seedOffset/4] |= (*input++) << ((seedOffset % 4) * 8);
		seedOffset++;
		messageLength++;
		if(seedOffset == 64)
		{
			for(int i=16;i<64;i++)
			{
				rdxUInt32 s0 = (ShiftRightRotate(seeds[i-15], 7) ^ ShiftRightRotate(seeds[i-15], 18) ^ ShiftRightRotate(seeds[i-15], 3));
				rdxUInt32 s1 = (ShiftRightRotate(seeds[i-2], 17) ^ ShiftRightRotate(seeds[i-2], 19) ^ ShiftRightRotate(seeds[i-2], 10));
				seeds[i] = seeds[i-16] + seeds[i-7] + s0 + s1;
			}

			for(int i=0;i<64;i++)
				SHA256Cycle(self->m_state, seeds[i], i);
			seedOffset = 0;
			for(int i=0;i<16;i++)
				seeds[i] = 0;
		}
	}
	self->m_messageLength = messageLength;
	self->m_seedOffset = seedOffset;
}

RDX_UTIL_DYNLIB_API void rdxXAPI_SHA256_Flush(rdxCSHA256Generator *self, rdxUInt8 *output)
{
	rdxXAPI_SHA256_Shim::Flush(self, output);
}

void rdxXAPI_SHA256_Shim::Flush(rdxCSHA256Generator *self, rdxUInt8 *output)
{
	rdxUInt8 terminator[63];
	rdxUInt8 oneBit = 0x80;
	memset(terminator, 0, sizeof(terminator));

	rdxLargeUInt messageLength = self->m_messageLength;
	rdxLargeUInt paddingNeeded = 63 - ((messageLength + 8) & 63);

	FeedBytes(self, &oneBit, 1);
	FeedBytes(self, terminator, paddingNeeded);

	for(int bytePos=0;bytePos<8;bytePos++)
	{
		if(bytePos >= sizeof(messageLength))
			terminator[bytePos] = 0;
		else
			terminator[bytePos] = (messageLength >> (bytePos * 8)) & 0xff;
	}
	FeedBytes(self, terminator, 8);

	
	for(int bytePos=0;bytePos<8;bytePos++)
	{
		if(bytePos >= sizeof(messageLength))
			terminator[bytePos] = 0;
		else
			terminator[bytePos] = (messageLength >> (bytePos * 8)) & 0xff;
	}

	for(int bytePos=0;bytePos<32;bytePos++)
		output[bytePos] = (self->m_state[bytePos/4] >> ((bytePos % 4) * 8)) & 0xff;
}

static rdxSDomainGUID rdxBuiltinGUID_Duplicable = rdxSDomainGUID::FromName("#");
static rdxSDomainGUID rdxBuiltinGUID_Core = rdxSDomainGUID::FromName("Core");
static rdxSDomainGUID rdxBuiltinGUID_Runtime = rdxSDomainGUID::FromName("%Runtime");
static rdxSDomainGUID rdxBuiltinGUID_ArrayDef = rdxSDomainGUID::FromName("%ArrayDef");

RDX_UTIL_DYNLIB_API void RDX_BuiltinDomainGUID(rdxEBuiltinDomain builtinDomain, rdxSDomainGUID *outGUID)
{
	switch(builtinDomain)
	{
	case rdxDOMAIN_Duplicable:
		*outGUID = rdxBuiltinGUID_Duplicable;
		return;
	case rdxDOMAIN_Core:
		*outGUID = rdxBuiltinGUID_Core;
		return;
	case rdxDOMAIN_Runtime:
		*outGUID = rdxBuiltinGUID_Runtime;
		return;
	case rdxDOMAIN_ArrayDef:
		*outGUID = rdxBuiltinGUID_ArrayDef;
		return;
	};
	*outGUID = rdxSDomainGUID::Invalid();
}

RDX_UTIL_DYNLIB_API void RDX_ComputeGUID(const char *name, rdxUInt8 *bytes)
{
	// This must match rdxCObjectManager::ComputeGUID
	rdxCSHA256Generator hash;
	hash.FeedBytes(name, strlen(name));

	rdxUInt8 flushed[hash.OUTPUT_SIZE_BYTES];
	hash.Flush(flushed);
	memcpy(bytes, flushed, rdxSDomainGUID::GUID_SIZE);

	rdxStaticAssert(sizeof(flushed) >= rdxSDomainGUID::GUID_SIZE);
}
