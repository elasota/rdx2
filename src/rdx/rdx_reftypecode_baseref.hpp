#ifndef __RDX_REFTYPECODE_BASEREF_HPP__
#define __RDX_REFTYPECODE_BASEREF_HPP__

// base ref
template<class TObjectRefType>
RDX_FORCEINLINE TObjectRefType *rdxBaseRef<TObjectRefType>::UnextractPOD(void *ptr)
{
	return static_cast<TObjectRefType*>(ptr);
}

template<class TObjectRefType>
RDX_FORCEINLINE TObjectRefType *rdxBaseRef<TObjectRefType>::GetPOD() const
{
	return static_cast<TObjectRefType*>(m_ref.ReadUnfenced());
}


template<class TObjectRefType>
rdxBaseRef<TObjectRefType>::rdxBaseRef(rdxBaseRef_ConSignal_Null_Type nullSig)
	: m_ref(RDX_CNULL)
{
}

template<class TObjectRefType>
RDX_FORCEINLINE rdxBaseRef<TObjectRefType>::rdxBaseRef(rdxBaseRef_ConSignal_POD_Type signalValue, TObjectRefType *ref)
	: m_ref(ref)
{
}

template<class TObjectRefType>
inline rdxBaseRef<TObjectRefType>::rdxBaseRef(rdxBaseRef_ConSignal_Marking_Type signalValue, TObjectRefType *ref)
	: m_ref(ref)
{
	rdxGCInfo *newGCInfo = rdxRefConverter<rdxGCInfo>::Convert(newRawRef);
	if(newGCInfo && !(newGCInfo->objectFlags & rdxGCInfo::GCOF_GCMarkOnAssign))
		newGCInfo->ownerObjectManager->ExternalGCMarkObject(newGCInfo);
}

template<class TObjectRefType>
inline rdxBaseRef<TObjectRefType>::rdxBaseRef(rdxBaseRef_ConSignal_Counting_Type signalValue, TObjectRefType *ref)
	: m_ref(ref)
{
	if(ref)
	{
		rdxGCInfo *newGCInfo = rdxRefConverter<rdxGCInfo>::Convert(ref);
		newGCInfo->numExternalReferences.IncrementFullFence();
		if(!(newGCInfo->objectFlags & rdxGCInfo::GCOF_GCMarkOnAssign))
			newGCInfo->ownerObjectManager->ExternalGCMarkObject(newGCInfo);
	}
}

template<class TObjectRefType>
RDX_FORCEINLINE void rdxBaseRef<TObjectRefType>::AssignRef(rdxBaseRef_ConSignal_POD_Type signalValue, TObjectRefType *ref)
{
	m_ref.WriteUnfenced(ref);
}

template<class TObjectRefType>
inline void rdxBaseRef<TObjectRefType>::AssignRef(rdxBaseRef_ConSignal_Marking_Type signalValue, TObjectRefType *ref)
{
	m_ref.WriteUnfenced(ref);

	if(ref)
	{
		rdxGCInfo *newGCInfo = rdxRefConverter<rdxGCInfo>::Convert(ref);
		if(!(newGCInfo->objectFlags & rdxGCInfo::GCOF_GCMarkOnAssign))
			newGCInfo->ownerObjectManager->ExternalGCMarkObject(newGCInfo);
	}
}

template<class TObjectRefType>
inline void rdxBaseRef<TObjectRefType>::AssignRef(rdxBaseRef_ConSignal_Counting_Type signalValue, TObjectRefType *newRawRef)
{
	void *replacement = newRawRef;
	void *oldRawRef = m_ref.ExchangeFullFence(replacement);

	if(oldRawRef)
	{
		rdxGCInfo *oldGCInfo = rdxRefConverter<rdxGCInfo>::Convert(static_cast<TObjectRefType*>(oldRawRef));
		oldGCInfo->numExternalReferences.DecrementFullFence();
	}
	if(newRawRef)
	{
		rdxGCInfo *newGCInfo = rdxRefConverter<rdxGCInfo>::Convert(newRawRef);
		newGCInfo->numExternalReferences.IncrementFullFence();
		if(!(newGCInfo->objectFlags & rdxGCInfo::GCOF_GCMarkOnAssign))
			newGCInfo->ownerObjectManager->ExternalGCMarkObject(newGCInfo);
	}
}

template<class TObjectRefType>
RDX_FORCEINLINE void rdxBaseRef<TObjectRefType>::DestroyRef(rdxBaseRef_ConSignal_POD_Type signalValue)
{
}

template<class TObjectRefType>
RDX_FORCEINLINE void rdxBaseRef<TObjectRefType>::DestroyRef(rdxBaseRef_ConSignal_Marking_Type signalValue)
{
}

template<class TObjectRefType>
RDX_FORCEINLINE void rdxBaseRef<TObjectRefType>::DestroyRef(rdxBaseRef_ConSignal_Counting_Type signalValue)
{
	void *oldThisPtr = m_ref.ReadUnfenced();
	if(oldThisPtr)
		rdxRefConverter<rdxGCInfo>::Convert(static_cast<TObjectRefType*>(oldThisPtr))->numExternalReferences.DecrementFullFence();
}

#endif
