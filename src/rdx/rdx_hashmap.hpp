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
#ifndef __RDX_HASHMAP_HPP__
#define __RDX_HASHMAP_HPP__

#include "rdx_basictypes.hpp"
#include "rdx_objectmanagement.hpp"
#include "rdx_utility.hpp"
#include "rdx_longflow.hpp"
#include "rdx_blockcopy.hpp"

struct rdxSHashElement
{
	enum HashElementType
	{
		HMT_ElementTypeEmpty,			// Empty bucket, terminates probe
		HMT_ElementTypeDeleted,			// Empty bucket, won't terminate probe

		HMT_ContainsData,

		HMT_ElementTypeFilled,			// Bucket with a value
		HMT_ElementTypeLoose,			// Unsorted value
	};

	HashElementType elementType;
	rdxHashValue hash;
};

template<class Tk, class Tv>
struct rdxSHashEntry
{
	rdxSHashElement e;
	Tk k;
	Tv v;
};

template<class Tk, class Tv>
class rdxCHashMapSimple
{
protected:
	typedef Tk HTKey;
	typedef Tk CandidateKey;
	typedef Tv Value;
	typedef rdxSHashEntry<Tk,Tv> HTEntry;

public:
	struct NativeProperties
	{
		HTEntry *untracedEntries;
		rdxLargeUInt numEntries;
		rdxIObjectManager *objectManager;
	} m_native;

	inline void InitializeElement(rdxSHashEntry<Tk,Tv> *v)
	{
		memset(v, 0, sizeof(rdxSHashEntry<Tk,Tv>));
	}

	inline rdxLargeUInt UnitSize() const
	{
		return sizeof(rdxSHashEntry<Tk,Tv>);
	}

	inline rdxSHashEntry<Tk,Tv> *EntryElement(rdxLargeUInt idx)
	{
		return &m_native.untracedEntries[idx];
	}

	inline const rdxSHashEntry<Tk,Tv> *EntryElement(rdxLargeUInt idx) const
	{
		return &m_native.untracedEntries[idx];
	}

	inline bool CompareKey(const Tk *ckey, const rdxSHashEntry<Tk,Tv> *e) const
	{
		return ckey[0] == e->k;
	}

	inline rdxHashValue HashCKey(const Tk *ckey) const
	{
		return ckey->Hash();
	}

	inline void SwapEntryData(rdxLargeUInt entry1idx, rdxLargeUInt entry2idx)
	{
		if(entry1idx == entry2idx)
			return;

		rdxSHashEntry<Tk,Tv> *entry1 = EntryElement(entry1idx);
		rdxSHashEntry<Tk,Tv> *entry2 = EntryElement(entry2idx);
		rdxSHashEntry<Tk,Tv> swapTemp = *entry1;
		*entry1 = *entry2;
		*entry2 = swapTemp;
	}

	inline void ReadValue(rdxLargeUInt index, Tv *value) const
	{
		value[0] = EntryElement(index)->v;
	}

	inline void LoadValue(rdxLargeUInt index, const Tv *value)
	{
		EntryElement(index)->v = value[0];
	}

	inline Tk ConstructKey(rdxSOperationContext *ctx, const Tk *ckey, bool &recheck)
	{
		recheck = false;
		return *ckey;
	}

	inline rdxCType *ArrayType() const
	{
		return NULL;
	}

	inline void SetObjectManager(rdxIObjectManager *om)
	{
		m_native.objectManager = om;
	}
};

template<typename Tbase>
class rdxCHashMap : public Tbase
{
private:
	rdxLargeUInt load;
	rdxLargeUInt loadFactorPercentage;
	rdxITypeProcessor *typeProcessor;

public:
	inline void Clear()
	{
		if(this->m_native.objectManager)
		{
			this->m_native.objectManager->GetAllocator()->Realloc(this->m_native.untracedEntries, 0);
			this->m_native.untracedEntries = NULL;
			this->m_native.numEntries = 0;
		}
	}

	inline ~rdxCHashMap()
	{
		Clear();
	}

	inline void SetTypeProcessor(rdxITypeProcessor *tp)
	{
		typeProcessor = tp;
	}

	// Returns the first match of a key.  If not found, returns an index that it can be inserted at
	inline rdxLargeUInt FindElementIndex(const typename Tbase::CandidateKey *ckey, rdxHashValue hash, bool &found) const
	{
		const typename Tbase::HTEntry *entries = this->m_native.untracedEntries;
		if(!entries)
		{
			found = false;
			return 0;
		}

		rdxLargeUInt numEntries = this->m_native.numEntries;
		rdxLargeUInt idx = rdxHashValueIndex(hash, numEntries);

		rdxLargeUInt insertableIndex = 0;
		bool hitInsertable = false;

		for(rdxLargeUInt n=0;n<numEntries;n++,idx++)
		{
			if(idx == numEntries)
				idx = 0;
			
			const rdxSHashElement *entry = &this->EntryElement(idx)->e;
			if(entry->elementType != rdxSHashElement::HMT_ElementTypeFilled)
			{
				if(hitInsertable == false)
				{
					hitInsertable = true;
					insertableIndex = idx;
				}
						
				if(entry->elementType == rdxSHashElement::HMT_ElementTypeEmpty)
					break;	// Entry doesn't exist

				if(entry->elementType == rdxSHashElement::HMT_ElementTypeDeleted)
					continue;	// Keep going
			}

			if(hash == entry->hash && this->CompareKey(ckey, this->EntryElement(idx)))
			{
				found = true;
				return idx;
			}
		}

		found = false;
		return insertableIndex;
	}

	inline rdxLargeUInt FindElementIndex(const typename Tbase::CandidateKey *ckey, bool &found, rdxHashValue *hashValue) const
	{
		rdxHashValue hash = this->HashCKey(ckey);

		if(hashValue)
			*hashValue = hash;

		return FindElementIndex(ckey, hash, found);
	}

	inline void DestroyEntry(rdxLargeUInt idx)
	{
		this->InitializeElement(this->EntryElement(idx));

		this->EntryElement(idx)->e.elementType = rdxSHashElement::HMT_ElementTypeDeleted;
	}

	inline void RemoveEntry(rdxLargeUInt idx)
	{
		typename Tbase::HTEntry *entries = this->m_native.untracedEntries;
		rdxLargeUInt numEntries = this->m_native.numEntries;
		rdxSHashElement *entry = &this->EntryElement(idx)->e;

		if(entry->elementType == rdxSHashElement::HMT_ElementTypeFilled)
		{
			// If the next entry is empty, then this can be emptied, otherwise it needs to act as a gap filler for probing
			const rdxSHashElement *nextEntry = &this->EntryElement(idx+1)->e;
			if(idx + 1 == numEntries)
				nextEntry = &this->EntryElement(0)->e;

			if(nextEntry->elementType == rdxSHashElement::HMT_ElementTypeEmpty)
				entry->elementType = rdxSHashElement::HMT_ElementTypeEmpty;
			else
				entry->elementType = rdxSHashElement::HMT_ElementTypeDeleted;
			DestroyEntry(idx);
			load--;
		}
	}

	inline void Balance()
	{
		rdxLargeUInt shrinkThreshold = this->m_native.numEntries * loadFactorPercentage / 200;
		if(load > 8 && load < shrinkThreshold)
		{
			this->m_native.objectManager->GetAllocator()->auditHeapFunc();
			rdxLargeUInt newCount = this->m_native.numEntries / 2;
			Rehash(this->m_native.numEntries, newCount);
			this->m_native.objectManager->GetAllocator()->auditHeapFunc();

			typename Tbase::HTEntry *newEntries = this->m_native.objectManager->GetAllocator()->Realloc(this->m_native.untracedEntries, newCount);
			if(newEntries)
				this->m_native.untracedEntries = newEntries;	// Realloc failed, but it was a shrink op, so just reuse
			this->m_native.numEntries = newCount;
		}
	}

	inline rdxCHashMap()
	{
		load = 0;
		loadFactorPercentage = 80;
		this->m_native.untracedEntries = NULL;
		this->m_native.numEntries = 0;
		typeProcessor = NULL;
	}

	inline bool ContainsKey(const typename Tbase::CandidateKey *ckey) const
	{
		bool found;
		FindElementIndex(ckey, found, static_cast<rdxHashValue *>(NULL));
		return found;
	}

	inline bool SetElement(const typename Tbase::CandidateKey *ckey, const typename Tbase::Value *value, rdxHashValue hashValue)
	{
		bool found;
		rdxLargeUInt index = FindElementIndex(ckey, hashValue, found);

		if(!found)
			return false;
		LoadValue(index, value);
		return true;
	}

	inline bool SetElement(const typename Tbase::CandidateKey *ckey, const typename Tbase::Value *value)
	{
		bool found;
		rdxLargeUInt index = FindElementIndex(ckey, found);

		if(!found)
			return false;
		LoadValue(index, value);
		return true;
	}

	inline bool GetElement(const typename Tbase::CandidateKey *ckey, typename Tbase::Value *value, rdxHashValue hashValue) const
	{
		bool found;
		rdxLargeUInt index = FindElementIndex(ckey, hashValue, found);

		if(!found)
			return false;
		ReadValue(index, value);
		return true;
	}

	inline bool GetElement(const typename Tbase::CandidateKey *ckey, typename Tbase::Value *value, rdxHashValue *hashValue) const
	{
		bool found;
		rdxLargeUInt index = FindElementIndex(ckey, found, hashValue);

		if(!found)
			return false;
		this->ReadValue(index, value);
		return true;
	}
			
	inline bool GetElement(const typename Tbase::CandidateKey *ckey, typename Tbase::Value *value) const
	{
		return GetElement(ckey, value, static_cast<rdxHashValue *>(NULL));
	}

	inline void EnlargeEntryArray(rdxSOperationContext *ctx, rdxLargeUInt newCount)
	{
		typename Tbase::HTEntry *entries = this->m_native.untracedEntries;
		rdxLargeUInt oldCount;
		rdxLargeUInt unitSize = this->UnitSize();

		if(!entries)
			oldCount = 0;
		else
			oldCount = this->m_native.numEntries;

		void *newEntries;

		this->m_native.objectManager->GetAllocator()->auditHeapFunc();
		newEntries = this->m_native.objectManager->GetAllocator()->Realloc(this->m_native.untracedEntries, newCount);
		this->m_native.objectManager->GetAllocator()->auditHeapFunc();
		if(!newEntries)
		{
			RDX_LTHROW(ctx, RDX_ERROR_ALLOCATION_FAILED);
		}

		this->m_native.untracedEntries = static_cast<typename Tbase::HTEntry *>(newEntries);
		this->m_native.numEntries = newCount;
		
		this->m_native.objectManager->GetAllocator()->auditHeapFunc();
		for(rdxLargeUInt i=oldCount ; i < newCount ; i++)
			this->InitializeElement(this->EntryElement(i));
		this->m_native.objectManager->GetAllocator()->auditHeapFunc();
	}

	inline void Insert(rdxSOperationContext *ctx, const typename Tbase::CandidateKey *ckey, const typename Tbase::Value *value, rdxHashValue hash)
	{
		RDX_TRY(ctx)
		{
			typename Tbase::HTEntry *entries = this->m_native.untracedEntries;

			rdxLargeUInt numEntries = 0;

			if(entries)
				numEntries = this->m_native.numEntries;

			if(load*100 >= loadFactorPercentage*numEntries)
			{
				rdxLargeUInt oldCount = 0;
				if(this->m_native.untracedEntries)
					oldCount = this->m_native.numEntries;

				rdxLargeUInt newCount;
				if(!oldCount)
					newCount = 1;
				else
					newCount = oldCount * 2;

				RDX_PROTECT(ctx, EnlargeEntryArray(ctx, newCount));

				entries = this->m_native.untracedEntries;

				Rehash(oldCount, newCount);
			}

			bool found;
			rdxLargeUInt index = FindElementIndex(ckey, hash, found);

			if(found)
			{
				this->LoadValue(index, value);
			}
			else
			{
				bool recheck;
				typename Tbase::HTKey htkey;
				RDX_PROTECT_ASSIGN(ctx, htkey, this->ConstructKey(ctx, ckey, recheck));

				// If a recheck is required, the hash list may have changed from a rebalance during the GC stage
				if(recheck)
					index = FindElementIndex(ckey, hash, found);

				this->EntryElement(index)->k = htkey;

				this->LoadValue(index, value);
					
				this->EntryElement(index)->e.hash = hash;
				this->EntryElement(index)->e.elementType = rdxSHashElement::HMT_ElementTypeFilled;

				load++;
			}
		}
		RDX_CATCH(ctx)
		{
			RDX_RETHROW(ctx);
		}
		RDX_ENDTRY
	}
			
	inline void Insert(rdxSOperationContext *ctx, const typename Tbase::CandidateKey *ckey, const typename Tbase::Value *value)
	{
		rdxHashValue hash = this->HashCKey(ckey);

		Insert(ctx, ckey, value, hash);
	}

	inline void Rehash(rdxLargeUInt oldCount, rdxLargeUInt newCount)
	{
		typename Tbase::HTEntry *entries = this->m_native.untracedEntries;

		// Convert all deleted to empty and all filled to loose
		for(rdxLargeUInt i=0;i<oldCount;i++)
		{
			rdxSHashElement *entry = &this->EntryElement(i)->e;

			if(entry->elementType == rdxSHashElement::HMT_ElementTypeDeleted)
				entry->elementType = rdxSHashElement::HMT_ElementTypeEmpty;
			else if(entry->elementType == rdxSHashElement::HMT_ElementTypeFilled)
				entry->elementType = rdxSHashElement::HMT_ElementTypeLoose;
		}

		for(rdxLargeUInt i=0;i<oldCount;i++)
		{
			rdxSHashElement *element = &this->EntryElement(i)->e;

			// As long as this bucket contains a filled element, keep moving it to the proper location
			while(element->elementType == rdxSHashElement::HMT_ElementTypeLoose)
			{
				if(element->elementType == rdxSHashElement::HMT_ElementTypeEmpty)
					break;

				// Reinsert this element
				rdxLargeUInt destination = rdxHashValueIndex(element->hash, newCount);
				while(this->EntryElement(destination)->e.elementType == rdxSHashElement::HMT_ElementTypeFilled)
				{
					destination++;
					if(destination == newCount)
						destination = 0;
				}

				element->elementType = rdxSHashElement::HMT_ElementTypeFilled;
				this->SwapEntryData(i, destination);
			}
		}
	}
};


#endif
