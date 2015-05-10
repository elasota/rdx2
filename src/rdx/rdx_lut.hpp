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
#ifndef __RDX_LUT_HPP__
#define __RDX_LUT_HPP__

#include "rdx_basictypes.hpp"
#include "rdx_utility.hpp"
#include <stdio.h>

class rdxCString;

template<class TindexCharType, class TactualCharType>
class rdxSStaticLookupStringKey
{
private:
	const TactualCharType *m_value;
	rdxLargeUInt m_len;
	rdxHashValue m_hashValue;
	bool m_valid;

public:
	inline rdxSStaticLookupStringKey(const TactualCharType *str, rdxLargeUInt len)
	{
		rdxCIntermediateHash hv;
		m_valid = true;

		for(rdxLargeUInt i=0;i<len;i++)
		{
			TindexCharType lc = static_cast<TindexCharType>(str[i]);
			if(static_cast<TactualCharType>(lc) != str[i])
				m_valid = false;
			hv.FeedBytes(&lc, sizeof(TindexCharType));
		}

		m_value = str;
		m_len = len;
		m_hashValue = hv.Flush();
	}

	inline rdxSStaticLookupStringKey(const TactualCharType *str)
	{
		rdxCIntermediateHash hv;
		m_valid = true;
		rdxLargeUInt len;
		for(len=0;str[len];len++)
		{
			TindexCharType lc = static_cast<TindexCharType>(str[len]);
			if(static_cast<TactualCharType>(lc) != str[len])
				m_valid = false;
			hv.FeedBytes(&lc, sizeof(TindexCharType));
		}

		m_value = str;
		m_len = len;
		m_hashValue = hv.Flush();
	}

	inline rdxHashValue Hash() const
	{
		return m_hashValue;
	}

	inline bool IsValid() const
	{
		return m_valid;
	}

	inline rdxLargeUInt Length() const
	{
		return m_len;
	}

	inline const TactualCharType *Chars() const
	{
		return m_value;
	}

	template<class TrsActualCharType>
	inline bool operator ==(const rdxSStaticLookupStringKey<TindexCharType, TrsActualCharType> &rs) const
	{
		if(this->m_valid == false || rs.IsValid() == false || rs.Length() != this->m_len)
			return false;
		rdxLargeUInt len = m_len;
		const TactualCharType *c1 = m_value;
		const TrsActualCharType *c2 = rs.Chars();
		while(len--)
		{
			const TindexCharType lChar = static_cast<TindexCharType>(*c1++);
			const TindexCharType rChar = static_cast<TindexCharType>(*c2++);
			if(lChar != rChar)
				return false;
		}
		return true;
	}
};

template<class Tvalue>
class rdxCStaticLookupPODKey
{
private:
	Tvalue m_value;
	rdxHashValue m_hashValue;

public:
	inline rdxCStaticLookupPODKey(const Tvalue &v)
	{
		m_value = v;
		m_hashValue = rdxHashBytes(&v, sizeof(Tvalue));
	}

	inline rdxHashValue Hash() const
	{
		return m_hashValue;
	}

	inline const Tvalue &Value() const
	{
		return m_value;
	}

	inline bool operator ==(const rdxCStaticLookupPODKey<Tvalue> &rs) const
	{
		return rs.m_value == this->m_value;
	}
};

template<class Tkey, class TlookupType>
class rdxCStaticLookupTable
{
public:
	typedef Tkey LUTKey;
	typedef TlookupType LUTValue;

	struct Entry
	{
		LUTKey key;
		LUTValue value;

		struct HashState
		{
			bool containsData;
			rdxLargeUInt collisionDepth;	// Max entries to search when starting here
			Entry *itemTarget;				// Where the entry is located now
			Entry *itemSource;				// Where the entry was originally located
		} hashState;
	};

private:
	Entry *m_entries;
	rdxLargeUInt m_numEntries;
	bool m_useAddrLookup;

	inline rdxLargeUInt HashToLocation(rdxHashValue hv, rdxLargeUInt numEntries) const
	{
		return static_cast<rdxLargeUInt>(static_cast<rdxUInt32>(hv) % static_cast<rdxUInt32>(numEntries));
	}

public:

	template<class TlookupKey>
	inline const LUTValue *Lookup(const TlookupKey &key) const
	{
		rdxHashValue hv = key.Hash();

		rdxLargeUInt startingLocation = HashToLocation(hv, m_numEntries);
		rdxLargeUInt location = startingLocation;
		rdxLargeUInt collisionDepth = m_entries[startingLocation].hashState.collisionDepth;
		while(true)
		{
			const Entry *e = m_entries + location;
			if(e->key == key)
				return &e->value;

			location++;
			if(location == m_numEntries)
				location = 0;
			if(location == startingLocation)
				return NULL;
			collisionDepth--;
			if(collisionDepth == 0)
				return NULL;
		}
	}

	template<class TlookupKey>
	inline LUTValue *Lookup(const TlookupKey &key)
	{
		rdxHashValue hv = key.Hash();

		rdxLargeUInt startingLocation = HashToLocation(hv, m_numEntries);
		rdxLargeUInt location = startingLocation;
		rdxLargeUInt collisionDepth = m_entries[startingLocation].hashState.collisionDepth;
		while(true)
		{
			Entry *e = m_entries + location;
			if(e->key == key)
				return &e->value;

			location++;
			if(location == m_numEntries)
				location = 0;
			if(location == startingLocation)
				return NULL;
			collisionDepth--;
			if(collisionDepth == 0)
				return NULL;
		}
	}

	inline rdxCStaticLookupTable(Entry *entries, rdxLargeUInt numEntries)
	{
		m_entries = entries;
		m_numEntries = numEntries;

		for(rdxLargeUInt i=0;i<numEntries;i++)
		{
			entries[i].hashState.containsData = false;
			entries[i].hashState.collisionDepth = 0;
			entries[i].hashState.itemTarget = entries + i;
			entries[i].hashState.itemSource = entries + i;
		}

		for(rdxLargeUInt i=0;i<numEntries;i++)
		{
			Entry *entry = entries[i].hashState.itemTarget;
			// Find a place to insert this
			rdxLargeUInt location = HashToLocation(entry->key.Hash(), numEntries);
			rdxLargeUInt initialLocation = location;
			rdxLargeUInt depth = 1;
			while(entries[location].hashState.containsData)
			{
				depth++;
				location++;
				if(location == numEntries)
					location = 0;
			}

			if(entries[initialLocation].hashState.collisionDepth < depth)
				entries[initialLocation].hashState.collisionDepth = depth;

			Entry *swapTarget = entries + location;
			if(swapTarget != entry)
			{
				Entry tempEntry = *entry;
				*entry = *swapTarget;
				*swapTarget = tempEntry;

				swapTarget->hashState.itemSource->hashState.itemTarget = swapTarget;
				entry->hashState.itemSource->hashState.itemTarget = entry;
			}

			swapTarget->hashState.containsData = true;
		}
	}
};


#endif
