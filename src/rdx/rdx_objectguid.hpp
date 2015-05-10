#ifndef __RDX_OBJECTGUID_HPP__
#define __RDX_OBJECTGUID_HPP__

#include "rdx_coretypes.hpp"
#include "rdx_builtindomain.hpp"

struct rdxSDomainGUID
{
	static const rdxLargeUInt GUID_SIZE = 8;

	rdxUInt8 m_bytes[GUID_SIZE];
	char *m_debugStr;

	bool operator ==(const rdxSDomainGUID &other) const;
	bool operator !=(const rdxSDomainGUID &other) const;
	
	static rdxSDomainGUID FromName(const char *domain);

	static rdxSDomainGUID Builtin(rdxEBuiltinDomain builtinDomain);

	static rdxSDomainGUID Invalid();

	rdxHashValue Hash() const;

	inline const char *DebugStr() const
	{
		return m_debugStr;
	}
};

struct rdxSObjectGUID
{
	static const rdxLargeUInt GUID_SIZE = rdxSDomainGUID::GUID_SIZE;

	rdxSDomainGUID m_domain;
	rdxUInt8 m_bytes[GUID_SIZE];
	char *m_debugStr;

	bool operator ==(const rdxSObjectGUID &other) const;
	bool operator !=(const rdxSObjectGUID &other) const;

	rdxHashValue Hash() const;

	inline const char *DebugStr() const
	{
		return m_debugStr;
	}

	static rdxSObjectGUID FromObjectName(const char *domain, const char *symbol);

	static rdxSObjectGUID Invalid();
};

#include "rdx.h"
#include "rdx_utility.hpp"

inline bool rdxSDomainGUID::operator ==(const rdxSDomainGUID &other) const
{
	for(int i=0;i<sizeof(m_bytes);i++)
		if(m_bytes[i] != other.m_bytes[i])
			return false;
	return true;
}

inline bool rdxSDomainGUID::operator !=(const rdxSDomainGUID &other) const
{
	for(int i=0;i<sizeof(m_bytes);i++)
		if(m_bytes[i] != other.m_bytes[i])
			return true;
	return false;
}

inline rdxSDomainGUID rdxSDomainGUID::Invalid()
{
	rdxSDomainGUID guid;
	for(int i=0;i<sizeof(guid.m_bytes);i++)
		guid.m_bytes[i] = 0xff;
	return guid;
}

#endif

// TODO MUSTFIX: Remove
#include <string.h>
#include "rdx_guid.hpp"

#ifndef __RDX_OBJECTGUID_CODE_HPP__
#define __RDX_OBJECTGUID_CODE_HPP__

inline rdxSDomainGUID rdxSDomainGUID::FromName(const char *name)
{
	rdxSDomainGUID guid;
	RDX_ComputeGUID(name, guid.m_bytes);
	guid.m_debugStr = new char[strlen(name) + 1];
	memcpy(guid.m_debugStr, name, strlen(name) + 1);
	return guid;
}

inline rdxHashValue rdxSDomainGUID::Hash() const
{
	return rdxHashBytes(m_bytes, GUID_SIZE);
}

inline rdxSDomainGUID rdxSDomainGUID::Builtin(rdxEBuiltinDomain builtinDomain)
{
	rdxSDomainGUID guid;
	RDX_BuiltinDomainGUID(builtinDomain, &guid);
	return guid;
}

inline bool rdxSObjectGUID::operator ==(const rdxSObjectGUID &other) const
{
	for(int i=0;i<sizeof(m_bytes);i++)
		if(m_bytes[i] != other.m_bytes[i])
			return false;
	return true;
}

inline bool rdxSObjectGUID::operator !=(const rdxSObjectGUID &other) const
{
	for(int i=0;i<sizeof(m_bytes);i++)
		if(m_bytes[i] == other.m_bytes[i])
			return false;
	return true;
}

inline rdxSObjectGUID rdxSObjectGUID::Invalid()
{
	rdxSObjectGUID guid;
	for(int i=0;i<sizeof(guid.m_bytes);i++)
		guid.m_bytes[i] = 0xff;
	guid.m_domain = rdxSDomainGUID::Invalid();
	guid.m_debugStr = RDX_CNULL;
	return guid;
}

inline rdxSObjectGUID rdxSObjectGUID::FromObjectName(const char *domain, const char *symbol)
{
	rdxSObjectGUID guid;
	guid.m_domain = rdxSDomainGUID::FromName(domain);
	RDX_ComputeGUID(symbol, guid.m_bytes);
	guid.m_domain.m_debugStr = new char[strlen(domain) + 1];
	memcpy(guid.m_domain.m_debugStr, domain, strlen(domain) + 1);
	guid.m_debugStr = new char[strlen(symbol) + 1];
	memcpy(guid.m_debugStr, symbol, strlen(symbol) + 1);
	return guid;
}

inline rdxHashValue rdxSObjectGUID::Hash() const
{
	return rdxHashBytes(m_bytes, GUID_SIZE) + m_domain.Hash();
}


#endif
