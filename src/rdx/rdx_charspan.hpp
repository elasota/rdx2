#ifndef __RDX_CHARSPAN_HPP__
#define __RDX_CHARSPAN_HPP__

#include "rdx_coretypes.hpp"

struct rdxSCharSpan
{
	rdxSCharSpan();
	rdxSCharSpan(const rdxSCharSpan &other);
	rdxSCharSpan(const rdxChar *chars, rdxLargeUInt len);
	const rdxChar *Chars() const;
	rdxLargeUInt Length() const;
	bool Equal(const char *str) const;

private:
	const rdxChar *m_chars;
	rdxLargeUInt m_len;
};


inline rdxSCharSpan::rdxSCharSpan()
	: m_chars(RDX_CNULL)
	, m_len(0)
{
}

inline rdxSCharSpan::rdxSCharSpan(const rdxSCharSpan &other)
	: m_chars(other.m_chars)
	, m_len(other.m_len)
{
}

inline rdxSCharSpan::rdxSCharSpan(const rdxChar *chars, rdxLargeUInt len)
	: m_chars(chars)
	, m_len(len)
{
}

inline const rdxChar *rdxSCharSpan::Chars() const
{
	return m_chars;
}

inline rdxLargeUInt rdxSCharSpan::Length() const
{
	return m_len;
}


inline bool rdxSCharSpan::Equal(const char *str) const
{
	rdxLargeUInt len = m_len;
	const rdxChar *chars = m_chars;
	for(rdxLargeUInt i=0;i<len;i++)
		if(static_cast<rdxChar>(str[i]) != chars[i])
			return false;
	return str[len] == '\0';
}

#endif
