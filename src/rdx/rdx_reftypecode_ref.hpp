
/////////////////////////////////////////////////////////////////////////
template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE rdxRef<T, TCounting, TBaseRef>::rdxRef()
	: m_baseRef(rdxBaseRef_ConSignal_Null)
{
}

template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE rdxRef<T, TCounting, TBaseRef>::rdxRef(const rdxRef<T, TCounting, TBaseRef> &other)
	: m_baseRef(rdxBaseRef_ConSignal<TCounting, TCounting>::Value, rdxRefConverter<typename TBaseRef::DerefPODType>::Convert(other.GetPOD()))
{
}

template<class T, bool TCounting, class TBaseRef>
template<class TOther, bool TOtherCounting, class TOtherBaseRef>
RDX_FORCEINLINE rdxRef<T, TCounting, TBaseRef>::rdxRef(const rdxRef<TOther, TOtherCounting, TOtherBaseRef> &other)
	: m_baseRef(rdxBaseRef_ConSignal<TCounting, TCounting>::Value, rdxRefConverter<typename TBaseRef::DerefPODType>::Convert(other.GetPOD()))
{
	const T *validator = static_cast<const TOther *>(RDX_CNULL);
}

template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE rdxRef<T, TCounting, TBaseRef>::~rdxRef()
{
	m_baseRef.DestroyRef(rdxBaseRef_ConSignal<TCounting, TCounting>::Value);
}

template<class T, bool TCounting, class TBaseRef>
template<class TOtherBaseRef>
RDX_FORCEINLINE rdxRef<T, TCounting, TBaseRef>::rdxRef(rdxObjRef_CSignal_BaseRef_Type signal, const TOtherBaseRef &otherRef)
	: m_baseRef(rdxBaseRef_ConSignal<TCounting, TCounting>::Value, rdxRefConverter<typename TBaseRef::DerefPODType>::Convert(otherRef.GetPOD()))
{
}


template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE rdxRef<T, TCounting, TBaseRef>::rdxRef(rdxObjRef_CSignal_RawRef_Type signal, void *ptr)
	: m_baseRef(rdxBaseRef_ConSignal<TCounting, TCounting>::Value, static_cast<typename TBaseRef::PODType>(ptr))
{
}

template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE rdxRef<T, TCounting, TBaseRef>::rdxRef(rdxObjRef_CSignal_DataPointer_Type signal, T *ptr)
	: m_baseRef(rdxBaseRef_ConSignal<TCounting, TCounting>::Value, rdxRefConverter<typename TBaseRef::DerefPODType>::Convert(ptr))
{
}

template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE rdxRef<T, TCounting, TBaseRef>::rdxRef(rdxObjRef_CSignal_GCInfo_Type signal, const rdxGCInfo *ptr)
	: m_baseRef(rdxBaseRef_ConSignal<TCounting, TCounting>::Value, rdxRefConverter<TBaseRef::DerefPODType>::Convert(const_cast<rdxGCInfo*>(ptr)))
{
}


template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE rdxRef<T, TCounting, TBaseRef>::rdxRef(rdxObjRef_CSignal_Null_Type signal)
	: m_baseRef(rdxBaseRef_ConSignal_Null)
{
}

template<class T, bool TCounting, class TBaseRef>
template<bool TOtherCounting, class TOtherBaseRef>
RDX_FORCEINLINE rdxRef<T, TCounting, TBaseRef> &rdxRef<T, TCounting, TBaseRef>::operator =(const rdxRef<T, TOtherCounting, TOtherBaseRef> &other)
{
	m_baseRef.AssignRef(rdxBaseRef_ConSignal<TCounting, TCounting>::Value, rdxRefConverter<TBaseRef::DerefPODType>::Convert(other.GetPOD()));
	return *this;
}

template<class T, bool TCounting, class TBaseRef>
template<class TOther, class TOtherBaseRef>
RDX_FORCEINLINE rdxRef<T, TCounting, TBaseRef> &rdxRef<T, TCounting, TBaseRef>::operator =(const rdxRTRef<TOther, TOtherBaseRef> &other)
{
	m_baseRef.AssignRef(rdxBaseRef_ConSignal<TCounting, TCounting>::Value, rdxRefConverter<TBaseRef::DerefPODType>::Convert(other.GetPOD()));
	return *this;
}

template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE rdxRef<T, TCounting, TBaseRef> &rdxRef<T, TCounting, TBaseRef>::operator =(const rdxRef<T, TCounting, TBaseRef> &other)
{
	m_baseRef.AssignRef(rdxBaseRef_ConSignal<TCounting, TCounting>::Value, other.GetPOD());
	return *this;
}

template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE T *rdxRef<T, TCounting, TBaseRef>::operator ->() const
{
	return this->Modify();
}

template<class T, bool TCounting, class TBaseRef>
template<class TOther, bool TOtherCounting, class TOtherRefPOD>
RDX_FORCEINLINE bool rdxRef<T, TCounting, TBaseRef>::operator ==(const rdxRef<TOther, TOtherCounting, TOtherRefPOD> &other) const
{
	return rdxRefConverter<TBaseRef::DerefPODType>::Convert(other.GetPOD()) == this->GetPOD();
}

template<class T, bool TCounting, class TBaseRef>
template<class TOther, class TOtherBaseRef>
RDX_FORCEINLINE bool rdxRef<T, TCounting, TBaseRef>::operator ==(const rdxRTRef<TOther, TOtherBaseRef> &other) const
{
	return rdxRefConverter<TBaseRef::DerefPODType>::Convert(other.GetPOD()) == this->GetPOD();
}

template<class T, bool TCounting, class TBaseRef>
template<class TOther, bool TOtherCounting, class TOtherRefPOD>
RDX_FORCEINLINE bool rdxRef<T, TCounting, TBaseRef>::operator !=(const rdxRef<TOther, TOtherCounting, TOtherRefPOD> &other) const
{
	return rdxRefConverter<TBaseRef::DerefPODType>::Convert(other.GetPOD()) != this->GetPOD();
}

template<class T, bool TCounting, class TBaseRef>
template<class TOther, class TOtherBaseRef>
RDX_FORCEINLINE bool rdxRef<T, TCounting, TBaseRef>::operator !=(const rdxRTRef<TOther, TOtherBaseRef> &other) const
{
	return rdxRefConverter<TBaseRef::DerefPODType>::Convert(other.GetPOD()) != this->GetPOD();
}

template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE typename rdxWeakRTRef(T) rdxRef<T, TCounting, TBaseRef>::ToWeakRTRef() const
{
	return rdxWeakRTRef(T)(rdxObjRef_CSignal_BaseRef, m_baseRef);
}

template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE typename rdxWeakHdl(T) rdxRef<T, TCounting, TBaseRef>::ToWeakHdl() const
{
	return rdxWeakHdl(T)(rdxObjRef_CSignal_BaseRef, m_baseRef);
}

template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE typename rdxCRef(T) rdxRef<T, TCounting, TBaseRef>::ToCRef() const
{
	return rdxCRef(T)(rdxObjRef_CSignal_BaseRef, m_baseRef);
}

template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE const T *rdxRef<T, TCounting, TBaseRef>::Data() const
{
	return static_cast<const T*>(rdxRefConverter<rdxCObject>::Convert(this->m_baseRef.GetPOD()));
}

template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE T *rdxRef<T, TCounting, TBaseRef>::Modify() const
{
	return static_cast<T*>(rdxRefConverter<rdxCObject>::Convert(this->m_baseRef.GetPOD()));
}

template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE rdxRef<T, TCounting, TBaseRef> rdxRef<T, TCounting, TBaseRef>::Null()
{
	return rdxRef<T, TCounting, TBaseRef>(rdxObjRef_CSignal_Null);
}

template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE rdxRef<T, TCounting, TBaseRef> rdxRef<T, TCounting, TBaseRef>::FromPtr(T *ptr)
{
	return rdxRef<T, TCounting, TBaseRef>(rdxObjRef_CSignal_DataPointer, ptr);
}

template<class T, bool TCounting, class TBaseRef>
template<class TNew>
RDX_FORCEINLINE rdxRef<TNew, TCounting, TBaseRef> rdxRef<T, TCounting, TBaseRef>::StaticCast() const
{
	const TNew *validator = static_cast<const TNew *>(static_cast<const T *>(NULL));
	return rdxRef<TNew, TCounting, TBaseRef>(rdxObjRef_CSignal_RawRef, m_baseRef.GetPOD());
}

template<class T, bool TCounting, class TBaseRef>
template<class TNew>
RDX_FORCEINLINE rdxRef<TNew, TCounting, TBaseRef> rdxRef<T, TCounting, TBaseRef>::ReinterpretCast() const
{
	return rdxRef<TNew, TCounting, TBaseRef>(rdxObjRef_CSignal_RawRef, m_baseRef.GetPOD());
}

template<class T, bool TCounting, class TBaseRef>
template<class TNew>
RDX_FORCEINLINE rdxRef<TNew, TCounting, rdxBaseIfcRef> rdxRef<T, TCounting, TBaseRef>::StaticCastToInterface() const
{
	const TNew *validator = static_cast<const TNew *>(static_cast<const T *>(NULL));
	return rdxRef<TNew, TCounting, rdxBaseIfcRef>(rdxObjRef_CSignal_DataPointer,
		static_cast<T*>(rdxRefConverter<rdxCObject>::Convert(static_cast<T*>(m_baseRef.GetPOD()))));
}

template<class T, bool TCounting, class TBaseRef>
template<class SelfT, class TMemberType>
RDX_FORCEINLINE rdxOffsetObjRef<TMemberType, TCounting, TBaseRef> rdxRef<T, TCounting, TBaseRef>::OffsetMember(TMemberType SelfT::* member) const
{
	const rdxUInt8 *offsetLoc = reinterpret_cast<const rdxUInt8 *>(&(static_cast<const T *>(NULL)->*member));
	rdxLargeUInt offset = static_cast<rdxLargeUInt>(offsetLoc - reinterpret_cast<const rdxUInt8 *>(NULL));
	return rdxOffsetObjRef<TMemberType, TCounting, TBaseRef>(this->ToWeakRTRef(), offset);
}

template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE bool rdxRef<T, TCounting, TBaseRef>::IsNull() const
{
	return m_baseRef.GetPOD() == RDX_CNULL;
}

template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE bool rdxRef<T, TCounting, TBaseRef>::IsNotNull() const
{
	return m_baseRef.GetPOD() != RDX_CNULL;
}

template<class T, bool TCounting, class TBaseRef>
rdxTypelessOffsetObjRef<TCounting, TBaseRef> rdxRef<T, TCounting, TBaseRef>::NoOffset() const
{
	return rdxTypelessOffsetObjRef<false, TBaseRef>(this->ToWeakRTRef().ReinterpretCast<rdxCObject>(), 0);
}

template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE typename TBaseRef::PODType rdxRef<T, TCounting, TBaseRef>::GetPOD() const
{
	return m_baseRef.GetPOD();
}

template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE TBaseRef &rdxRef<T, TCounting, TBaseRef>::GetBaseRef()
{
	return m_baseRef;
}

template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE const TBaseRef &rdxRef<T, TCounting, TBaseRef>::GetBaseRef() const
{
	return m_baseRef;
}
	
template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE void rdxRef<T, TCounting, TBaseRef>::InitRef(typename TBaseRef::PODType data)
{
	m_baseRef.AssignRef(rdxBaseRef_ConSignal<TCounting, TCounting>::Value, data);
}

template<class T, bool TCounting, class TBaseRef>
RDX_FORCEINLINE void rdxRef<T, TCounting, TBaseRef>::AssignRawRef(typename TBaseRef::PODType data)
{
	m_baseRef.AssignRef(rdxBaseRef_ConSignal<TCounting, TCounting>::Value, data);
}