#include "../exported/RDXInterface/Core/Collections/HashSetBase.hpp"
#include "../exported/RDXInterface/Core/Collections/HashSetBase/HashElementInfo.hpp"
#include "../exported/RDXInterface/Core/Array.hpp"
#include "../rdx/rdx_objectmanagement.hpp"
#include "../rdx/rdx_marshal.hpp"
#include "../rdx/rdx_utility.hpp"
#include "../rdx/rdx_blockcopy.hpp"

using namespace RDX;
using namespace RDX::ObjectManagement;
using namespace RDX::Programmability;
using namespace RDX::Utility;
using namespace RDXInterface;

namespace RDXCoreLibExt
{
	class HashSetBase_Private : public RDXInterface::Core::Collections::HashSetBase
	{
	public:
		int Rehash(RDX::Marshaling::ExportedCallEnvironment &callEnv, LargeUInt numElements, CRef<void> values, bool lock,
			CRef<void> *outKeys, CRef<void> *outValues, CRef<Core::Collections::HashSetBase::HashElementInfo> *outElementInfo);
		static void LIUnitMove(const HashElementInfo *elements, rdxLargeInt numElements, void *destDataV, const void *srcDataV, LargeUInt numLIs);
		static void BlockUnitMove(const HashElementInfo *elements, rdxLargeInt numElements, void *destDataV, const void *srcDataV, LargeUInt dataUnitSize);
		static void RehashArrays(void *keyDest, const void *keySource, LargeUInt keySize,
			void *valueDest, const void *valueSource, LargeUInt valueSize,
			HashElementInfo *elementsDest, const HashElementInfo *elementsSrc);
		static void RemakeArray(rdxSOperationContext *ctx, CRef<void> &outARef, rdxIObjectManager *objm, LargeUInt newSize, const void *originalArray);

		template<class _BT>
		inline static void UnitMove(const HashElementInfo *elements, rdxLargeInt numElements, void *destDataV, const void *srcDataV)
		{
			_BT *destData = static_cast<_BT *>(destDataV);
			const _BT *srcData = static_cast<const _BT *>(srcDataV);

			while(numElements--)
			{
				if(elements->elementType == ElementType::Filled)
					*destData = srcData[elements->sourceIndex];
				destData++;
				elements++;
			}
		}
	};
}


// Block-moves largeint-aligned units
// This should ensure that pointer-containing struct moves move pointers atomically
inline void RDXCoreLibExt::HashSetBase_Private::LIUnitMove(const HashElementInfo *elements, rdxLargeInt numElements, void *destDataV, const void *srcDataV, LargeUInt numLIs)
{
	rdxLargeInt *destData = static_cast<rdxLargeInt *>(destDataV);
	const rdxLargeInt *srcData = static_cast<const rdxLargeInt *>(srcDataV);

	while(numElements--)
	{
		if(elements->elementType == ElementType::Filled)
		{
			const rdxLargeInt *srcDataStart = srcData + elements->sourceIndex * numLIs;
			for(LargeUInt i=0;i<numLIs;i++)
				destData[i] = srcDataStart[i];
		}
		destData += numLIs;
		elements++;
	}
}

void RDXCoreLibExt::HashSetBase_Private::BlockUnitMove(const HashElementInfo *elements, rdxLargeInt numElements, void *destDataV, const void *srcDataV, LargeUInt dataUnitSize)
{
	// Try to do some fast versions
	if(dataUnitSize == sizeof(UInt8))
		UnitMove<UInt8>(elements, numElements, destDataV, srcDataV);
	else if(dataUnitSize == sizeof(UInt16))
		UnitMove<UInt16>(elements, numElements, destDataV, srcDataV);
	else if(dataUnitSize == sizeof(UInt32))
		UnitMove<UInt32>(elements, numElements, destDataV, srcDataV);
	else if(dataUnitSize == sizeof(rdxLargeInt))
		UnitMove<rdxLargeInt>(elements, numElements, destDataV, srcDataV);
	else if(dataUnitSize % sizeof(rdxLargeInt) == 0)
		LIUnitMove(elements, numElements, destDataV, srcDataV, dataUnitSize / sizeof(rdxLargeInt));
	else
	{
		UInt8 *destData = static_cast<UInt8 *>(destDataV);
		const UInt8 *srcData = static_cast<const UInt8 *>(srcDataV);

		while(numElements--)
		{
			if(elements->elementType == ElementType::Filled)
				BlockCopy(destData, srcData + elements->sourceIndex * dataUnitSize, dataUnitSize);
			destData += dataUnitSize;
			elements++;
		}
	}
}

void RDXCoreLibExt::HashSetBase_Private::RehashArrays(void *keyDest, const void *keySource, LargeUInt keySize,
			void *valueDest, const void *valueSource, LargeUInt valueSize,
			HashElementInfo *elementsDest, const HashElementInfo *elementsSrc)
{
	rdxLargeInt numSrcElements = GCInfo::From(keySource)->numElements;
	rdxLargeInt numDestElements = GCInfo::From(keyDest)->numElements;

	if(valueSource && GCInfo::From(valueSource)->numElements != numSrcElements)
		return;
	if(valueDest && GCInfo::From(valueDest)->numElements != numDestElements)
		return;
	if(GCInfo::From(elementsSrc)->numElements != numSrcElements)
		return;
	if(GCInfo::From(elementsDest)->numElements != numDestElements)
		return;

	for(rdxLargeInt i=0;i<numSrcElements;i++)
	{
		const HashElementInfo &element = elementsSrc[i];
		if(element.elementType != ElementType::Filled)
			continue;

		bool foundNewSpot = false;

		// Find a new spot for this
		LargeUInt insertLoc = element.hashCode % static_cast<LargeUInt>(numDestElements);
		for(rdxLargeInt j=0;j<numDestElements;j++)
		{
			HashElementInfo &destElement = elementsDest[insertLoc];

			if(destElement.elementType == ElementType::Empty)
			{
				destElement.hashCode = element.hashCode;
				destElement.elementType = ElementType::Filled;
				destElement.sourceIndex = i;
				foundNewSpot = true;
				break;
			}

			insertLoc++;
			if(insertLoc == static_cast<LargeUInt>(numDestElements))
				insertLoc = 0;
		}

		if(!foundNewSpot)
			return;		// Sabotaged
	}
				
	// Re-copy
	if(valueSource)
		BlockUnitMove(elementsDest, numDestElements, valueDest, valueSource, valueSize);
	BlockUnitMove(elementsDest, numDestElements, keyDest, keySource, keySize);
}

void RDXCoreLibExt::HashSetBase_Private::RemakeArray(rdxSOperationContext *ctx, CRef<void> &outARef, rdxIObjectManager *objm, LargeUInt newSize, const void *originalArray)
{
	RDX_TRY(ctx)
	{
		if(originalArray == NULL)
			return;
				
		const GCInfo *arrayGCI = GCInfo::From(originalArray);
		if(arrayGCI->numDimensions != 1)
			return;

		RDX_PROTECT_ASSIGN(ctx, outARef, objm->CreateContainer(ctx, arrayGCI->elementSize, static_cast<rdxLargeInt>(newSize), 1, DOMAIN_Runtime, arrayGCI->containerType, arrayGCI->typeProcessor, 0));
		GCInfo::From(outARef)->dimensions[0] = static_cast<rdxLargeInt>(newSize);
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

inline int RDXCoreLibExt::HashSetBase_Private::Rehash(RDX::Marshaling::ExportedCallEnvironment &callEnv, LargeUInt numElements, CRef<void> values, bool lock,
	CRef<void> *outKeys, CRef<void> *outValues, CRef<Core::Collections::HashSetBase::HashElementInfo> *outElementInfo)
{
	RDX_TRY(callEnv.ctx)
	{
		if(numElements <= 0)
			return callEnv.Throw(static_cast<Exception*>(callEnv.objm->GetBuiltIns()->providerDictionary[X_InvalidOperationException]));

		CRef<void> newValues;
		CRef<void> newKeys;
		CRef<void> newElements;

		// Use CRefs in case this is changed
		CRef<void> oldKeys = _keys;
		CRef<HashElementInfo> oldElementInfo = _elementInfo;

		RDX_PROTECT(callEnv.ctx, RemakeArray(callEnv.ctx, newValues, callEnv.objm, numElements, values));
		RDX_PROTECT(callEnv.ctx, RemakeArray(callEnv.ctx, newKeys, callEnv.objm, numElements, oldKeys));
		RDX_PROTECT(callEnv.ctx, RemakeArray(callEnv.ctx, newElements, callEnv.objm, numElements, oldElementInfo));

		if(newKeys == NULL || newElements == NULL)
			return callEnv.Throw(static_cast<Exception*>(callEnv.objm->GetBuiltIns()->providerDictionary[X_InvalidOperationException]));

		LargeUInt valueSize = 0;
		if(newValues != NULL)
			valueSize = static_cast<LargeUInt>(GCInfo::From(newValues)->elementSize);
		
#ifdef RDX_ENABLE_SMP
		if(lock)
			_native._deserializeRehashMutex.Acquire();
#endif

		RehashArrays(newKeys, oldKeys, static_cast<LargeUInt>(GCInfo::From(newKeys)->elementSize),
			newValues, values, valueSize,
			newElements.ReinterpretCast<HashElementInfo>(), oldElementInfo);
		
#ifdef RDX_ENABLE_SMP
		if(lock)
			_native._deserializeRehashMutex.Release();
#endif

		// Store out results
		*outKeys = newKeys;
		*outElementInfo = newElements.ReinterpretCast<HashElementInfo>();
		*outValues = newValues;

		return RuntimeState::Active;
	}
	RDX_CATCH(callEnv.ctx)
	{
		return callEnv.Throw(static_cast<Exception*>(callEnv.objm->GetBuiltIns()->providerDictionary[X_AllocationFailureException]));
	}
	RDX_ENDTRY
}

void Core::Collections::HashSetBase::FindKey(Marshaling::ExportedCallEnvironment &callEnv, Bool &outSucceeded, rdxLargeInt &outIndex, TypedRuntimePointer vkey, Core::Array *values, Bool inserting)
{
	outSucceeded = FalseValue;
	outIndex = 0;

	RDX_TRY(callEnv.ctx)
	{
		CRef<void> keys;
		CRef<HashElementInfo> elementInfos;
		CRef<void> values;

		keys = this->_keys;
		elementInfos = this->_elementInfo;

		if(keys == NULL || elementInfos == NULL)
		{
			callEnv.thread->ex = static_cast<Exception*>(callEnv.objm->GetBuiltIns()->providerDictionary[X_InvalidOperationException]);
			callEnv.status = RuntimeState::Exception;
			return;
		}

		rdxLargeInt numElements = GCInfo::From(keys)->numElements;
		if(numElements != GCInfo::From(elementInfos)->numElements)
		{
			callEnv.thread->ex = static_cast<Exception*>(callEnv.objm->GetBuiltIns()->providerDictionary[X_InvalidOperationException]);
			callEnv.status = RuntimeState::Exception;
			return;
		}
					
		const Type *keyType = vkey.type;
		if(GCInfo::From(keys)->contentsType != keyType)
		{
			callEnv.thread->ex = static_cast<Exception*>(callEnv.objm->GetBuiltIns()->providerDictionary[X_InvalidOperationException]);
			callEnv.status = RuntimeState::Exception;
			return;
		}

		const void *keyRef = vkey.rtp.valueRef;

		rdxLargeInt keySize, align;
		RDX_PROTECT(callEnv.ctx, callEnv.objm->TypeValueSize(callEnv.ctx, keyType, keySize, align));

		// Detect serialization sentinel.  If this is present, a rebuild of all hash codes and a rehash is required
		if(this->_serializationDetector)
		{
			// Rebuild all key hashes
			const UInt8 *keyBytes = reinterpret_cast<const UInt8 *>(keys.Object());
			for(rdxLargeInt i=0;i<numElements;i++,keyBytes+=keySize)
			{
				HashElementInfo &ei = elementInfos[i];
				ei.hashCode = HashBytes(keyBytes, static_cast<size_t>(keySize));
			}

			RDX_PROTECT(callEnv.ctx, static_cast<RDXCoreLibExt::HashSetBase_Private *>(this)->Rehash(callEnv, static_cast<LargeUInt>(numElements), values, true, &keys, &values, &elementInfos));

#ifdef RDX_ENABLE_SMP
			// It's possible that pre-lock volatility caused bad data to be reinserted, so integrity check that we're still looking at the same array sizes
			if(keys == NULL || elementInfos == NULL || GCInfo::From(keys)->numElements != numElements || GCInfo::From(values)->numElements != numElements ||
				(values != NULL && GCInfo::From(values)->numElements != numElements))
			{
				callEnv.thread->ex = static_cast<Exception*>(callEnv.objm->GetBuiltIns()->providerDictionary[X_InvalidOperationException]);
				callEnv.status = RuntimeState::Exception;
				return;
			}
#endif

			_serializationDetector = FalseValue;
		}

		HashValue keyHashCode = HashBytes(keyRef, static_cast<LargeUInt>(keySize));

		LargeUInt searchLoc = keyHashCode % static_cast<LargeUInt>(numElements);
		bool foundKey = false;
		bool foundEmpty = false;

		const UInt8 *searchKeyBytes = reinterpret_cast<const UInt8 *>(keys.Object()) + searchLoc * keySize;
		HashElementInfo *searchEI = NULL;
		for(rdxLargeInt i=0;i<numElements;i++,searchLoc++,searchKeyBytes+=keySize)
		{
			if(searchLoc == static_cast<LargeUInt>(numElements))
			{
				searchLoc = 0;
				searchKeyBytes = reinterpret_cast<const UInt8 *>(keys.Object());
			}

			searchEI = elementInfos + searchLoc;

			if(searchEI->elementType == ElementType::Empty)
			{
				foundEmpty = true;
				break;
			}

			if(searchEI->hashCode != keyHashCode)
				continue;

			if(!memcmp(searchKeyBytes, keyRef, static_cast<size_t>(keySize)))
			{
				foundKey = true;
				break;
			}
		}

		outIndex = static_cast<Int>(searchLoc);
		if(inserting != FalseValue)
		{
			if(foundEmpty || foundKey)
			{
				outSucceeded = TrueValue;
				// Insert the key
				BlockCopy(const_cast<UInt8 *>(searchKeyBytes), keyRef, static_cast<LargeUInt>(keySize));
				searchEI->elementType = ElementType::Filled;
				searchEI->hashCode = keyHashCode;
				this->_load++;
			}
			else
				outSucceeded = FalseValue;
		}
		else
			outSucceeded = foundKey ? TrueValue : FalseValue;
	}
	RDX_CATCH(callEnv.ctx)
	{
		callEnv.thread->ex = static_cast<Exception*>(callEnv.objm->GetBuiltIns()->providerDictionary[X_InvalidOperationException]);
		callEnv.status = RuntimeState::Exception;
		return;
	}
	RDX_ENDTRY
}

Core::Array *RDXInterface::Core::Collections::HashSetBase::Rehash(Marshaling::ExportedCallEnvironment &callEnv, rdxLargeInt numElements, Core::Array *values)
{
	CRef<void> outValues;
	CRef<void> outKeys;
	CRef<HashElementInfo> outElementInfo;
	int status = static_cast<RDXCoreLibExt::HashSetBase_Private *>(this)->Rehash(callEnv, static_cast<LargeUInt>(numElements), values, false, &outKeys, &outValues, &outElementInfo);

	_keys = reinterpret_cast<Core::Array *>(outKeys.Object());
	_elementInfo = outElementInfo;

	if(status == RuntimeState::Active)
		return reinterpret_cast<Core::Array *>(outValues.Object());
	return NULL;
}

void RDXInterface::Core::Collections::HashSetBase::Next(Marshaling::ExportedCallEnvironment &callEnv, Bool &outHasNext, rdxLargeInt &outNextIndex, rdxLargeInt oldIndex)
{
	RDX_TRY(callEnv.ctx)
	{
		CRef<HashElementInfo> elementInfos = _elementInfo;
		if(elementInfos == NULL)
		{
			callEnv.Throw(static_cast<Exception*>(callEnv.objm->GetBuiltIns()->providerDictionary[X_InvalidOperationException]));
			return;
		}

		rdxLargeInt numElements = GCInfo::From(elementInfos)->numElements;

		outNextIndex = 0;
		outHasNext = FalseValue;

		rdxLargeInt nextElement = oldIndex + 1;

		if(nextElement < 0)
			return;

		while(nextElement < numElements)
		{
			if(elementInfos[nextElement].elementType == ElementType::Filled)
			{
				outHasNext = TrueValue;
				outNextIndex = nextElement;
				return;
			}
			// Search for the next active element
			nextElement++;
		}

		return;
	}
	RDX_CATCH(callEnv.ctx)
	{
		callEnv.Throw(static_cast<Exception*>(callEnv.objm->GetBuiltIns()->providerDictionary[X_InvalidOperationException]));
		return;
	}
	RDX_ENDTRY
}

// Disables an element
Bool RDXInterface::Core::Collections::HashSetBase::RemoveElement(Marshaling::ExportedCallEnvironment &callEnv, rdxLargeInt idx)
{
	RDX_TRY(callEnv.ctx)
	{
		CRef<HashElementInfo> elementInfos = _elementInfo;

		if(elementInfos == NULL)
		{
			callEnv.Throw(static_cast<Exception*>(callEnv.objm->GetBuiltIns()->providerDictionary[X_InvalidOperationException]));
			return FalseValue;
		}

		rdxLargeInt numElements = GCInfo::From(elementInfos)->numElements;
					
		if(idx < 0 || idx >= numElements)
		{
			callEnv.Throw(static_cast<Exception*>(callEnv.objm->GetBuiltIns()->providerDictionary[X_IndexOutOfBoundsException]));
			return FalseValue;
		}

		elementInfos[idx].elementType = ElementType::Deleted;
		rdxLargeInt nextIndex = idx + 1;
		if(nextIndex == numElements)
			nextIndex = 0;

		// If the next one is empty, then this deleted the last element of a probe chain, so go back and clean up
		if(elementInfos[nextIndex].elementType == ElementType::Empty)
		{
			while(elementInfos[idx].elementType == ElementType::Deleted)
			{
				elementInfos[idx].elementType = ElementType::Empty;
				if(idx == 0)
					idx = numElements - 1;
				else
					idx--;
			}
		}

		rdxLargeInt newLoad = (--_load);

		return (numElements > 8 && newLoad < numElements / 2) ? TrueValue : FalseValue;
	}
	RDX_CATCH(callEnv.ctx)
	{
		callEnv.Throw(static_cast<Exception*>(callEnv.objm->GetBuiltIns()->providerDictionary[X_InvalidOperationException]));
		return FalseValue;
	}
	RDX_ENDTRY
}

