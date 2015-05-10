/////////////////////////////////////////////////////////////////////////
// Typeless RTRef
template<class TBaseRef>
RDX_FORCEINLINE rdxTypelessRTRef<TBaseRef>::rdxTypelessRTRef()
	: m_baseRef(rdxBaseRef_ConSignal_Null)
{
}

template<class TBaseRef>
RDX_FORCEINLINE rdxTypelessRTRef<TBaseRef>::rdxTypelessRTRef(const rdxTypelessRTRef<TBaseRef> &other)
	: m_baseRef(rdxBaseRef_ConSignal<false, true>::Value, rdxRefConverter<typename TRefPOD::DerefPODType>::Convert(other.GetPOD()))
{
}

template<class TBaseRef>
template<class TOtherBaseRef>
RDX_FORCEINLINE rdxTypelessRTRef<TBaseRef>::rdxTypelessRTRef(rdxObjRef_CSignal_BaseRef_Type signal, const TOtherBaseRef &otherRef)
	: m_baseRef(rdxBaseRef_ConSignal<false, true>::Value, otherRef.GetPOD())
{
}

template<class TBaseRef>
RDX_FORCEINLINE rdxTypelessRTRef<TBaseRef>::rdxTypelessRTRef(rdxObjRef_CSignal_DataPointer_Type signal, rdxCObject *ptr)
	: m_baseRef(rdxBaseRef_ConSignal<false, true>::Value, rdxRefConverter<typename TBaseRef::DerefPODType>::Convert(ptr))
{
}

template<class TBaseRef>
RDX_FORCEINLINE rdxTypelessRTRef<TBaseRef>::rdxTypelessRTRef(rdxObjRef_CSignal_RawRef_Type signal, void *ptr)
	: m_baseRef(rdxBaseRef_ConSignal<false, true>::Value, static_cast<typename TBaseRef::PODType>(ptr))
{
}

template<class TBaseRef>
RDX_FORCEINLINE rdxTypelessRTRef<TBaseRef>::rdxTypelessRTRef(rdxObjRef_CSignal_GCInfo_Type signal, const rdxGCInfo *ptr)
	: m_baseRef(rdxBaseRef_ConSignal<false, true>::Value, rdxRefConverter<TBaseRef::DerefPODType>::Convert(const_cast<rdxGCInfo*>(ptr)))
{
}

template<class TBaseRef>
RDX_FORCEINLINE void rdxTypelessRTRef<TBaseRef>::InitRef(typename TBaseRef::PODType data)
{
	m_baseRef.AssignRef(rdxBaseRef_ConSignal<false, true>::Value, data);
}

template<class TBaseRef>
RDX_FORCEINLINE void rdxTypelessRTRef<TBaseRef>::AssignRawRef(typename TBaseRef::PODType data)
{
	m_baseRef.AssignRef(rdxBaseRef_ConSignal<false, true>::Value, data);
}

template<class TBaseRef>
RDX_FORCEINLINE rdxTypelessRTRef<TBaseRef>::~rdxTypelessRTRef()
{
	m_baseRef.DestroyRef(rdxBaseRef_ConSignal<false, true>::Value);
}

template<class TBaseRef>
RDX_FORCEINLINE rdxTypelessOffsetObjRef<false, TBaseRef> rdxTypelessRTRef<TBaseRef>::NoOffset() const
{
	return rdxTypelessOffsetObjRef<false, TBaseRef>(this->ToWeakRTRef().ReinterpretCast<rdxCObject>(), 0);
}

template<class TBaseRef>
RDX_FORCEINLINE bool rdxTypelessRTRef<TBaseRef>::IsNull() const
{
	return m_baseRef.GetPOD() == NULL;
}
	
template<class TBaseRef>
RDX_FORCEINLINE bool rdxTypelessRTRef<TBaseRef>::IsNotNull() const
{
	return m_baseRef.GetPOD() != NULL;
}

template<class TBaseRef>
RDX_FORCEINLINE typename TBaseRef::PODType rdxTypelessRTRef<TBaseRef>::GetPOD() const
{
	return m_baseRef.GetPOD();
}

template<class TBaseRef>
RDX_FORCEINLINE typename rdxWeakRTRef(rdxCObject) rdxTypelessRTRef<TBaseRef>::ToWeakRTRef() const
{
	return rdxWeakRTRef(rdxCObject)(rdxObjRef_CSignal_BaseRef, m_baseRef);
}

template<class TBaseRef>
template<class TNew>
RDX_FORCEINLINE rdxRTRef<TNew, TBaseRef> rdxTypelessRTRef<TBaseRef>::ReinterpretCast() const
{
	return rdxRTRef<TNew, TBaseRef>(rdxObjRef_CSignal_RefPOD, GetPOD());
}

/////////////////////////////////////////////////////////////
// Typed RTRef
template<class T, class TBaseRef>
RDX_FORCEINLINE rdxRTRef<T, TBaseRef>::rdxRTRef()
	: Super()
{
}

template<class T, class TBaseRef>
RDX_FORCEINLINE rdxRTRef<T, TBaseRef>::rdxRTRef(const rdxRTRef<T, TBaseRef> &other)
{
	InitRef(other.GetPOD());
}

template<class T, class TBaseRef>
template<class TOtherBaseRef>
RDX_FORCEINLINE rdxRTRef<T, TBaseRef>::rdxRTRef(rdxObjRef_CSignal_BaseRef_Type signal, const TOtherBaseRef &otherBaseRef)
{
	InitRef(rdxRefConverter<typename TRefPOD::DerefPODType>::Convert(otherBaseRef.GetPOD()));
}

template<class T, class TBaseRef>
RDX_FORCEINLINE rdxRTRef<T, TBaseRef>::rdxRTRef(rdxObjRef_CSignal_RawRef_Type signal, void *ptr)
{
	InitRef(static_cast<typename TBaseRef::PODType>(ptr));
}

template<class T, class TBaseRef>
RDX_FORCEINLINE rdxRTRef<T, TBaseRef>::rdxRTRef(rdxObjRef_CSignal_DataPointer_Type signal, T *ptr)
{
	InitRef(rdxRefConverter<typename TRefPOD::DerefPODType>::Convert(ptr));
}

template<class T, class TBaseRef>
RDX_FORCEINLINE rdxRTRef<T, TBaseRef>::rdxRTRef(rdxObjRef_CSignal_GCInfo_Type signal, const rdxGCInfo *gcinfo)
{
	InitRef(rdxRefConverter<typename TRefPOD::DerefPODType>::Convert(const_cast<rdxGCInfo*>(gcinfo)));
}


template<class T, class TBaseRef>
RDX_FORCEINLINE rdxRTRef<T, TBaseRef>::rdxRTRef(rdxObjRef_CSignal_Null_Type signal)
{
	InitRef(static_cast<typename TBaseRef::PODType>(RDX_CNULL));
}

template<class T, class TBaseRef>
template<class TOtherBaseRef>
RDX_FORCEINLINE rdxRTRef<T, TBaseRef> &rdxRTRef<T, TBaseRef>::operator =(const rdxRTRef<T, TOtherBaseRef> &other)
{
	AssignRawRef(rdxRefConverter<typename TBaseRef::DerefPODType>::Convert(other.GetPOD()));
	return *this;
}

template<class T, class TBaseRef>
template<class TOther, bool TOtherCounting, class TOtherBaseRef>
RDX_FORCEINLINE rdxRTRef<T, TBaseRef> &rdxRTRef<T, TBaseRef>::operator =(const rdxRef<TOther, TOtherCounting, TOtherBaseRef> &other)
{
	AssignRawRef(rdxRefConverter<typename TBaseRef::DerefPODType>::Convert(other.GetPOD()));
	return *this;
}

template<class T, class TBaseRef>
RDX_FORCEINLINE rdxRTRef<T, TBaseRef> &rdxRTRef<T, TBaseRef>::operator =(const rdxRTRef<T, TBaseRef> &other)
{
	AssignRawRef(rdxRefConverter<typename TBaseRef::DerefPODType>::Convert(other.GetPOD()));
	return *this;
}

template<class T, class TBaseRef>
RDX_FORCEINLINE T *rdxRTRef<T, TBaseRef>::operator ->() const
{
	return this->Modify();
}

template<class T, class TBaseRef>
template<class TOther, class TOtherBaseRef>
RDX_FORCEINLINE bool rdxRTRef<T, TBaseRef>::operator ==(const rdxRTRef<TOther, TOtherBaseRef> &other) const
{
	return rdxRefConverter<TBaseRef::DerefPODType>::Convert(other.GetPOD()) == this->GetPOD();
}

template<class T, class TBaseRef>
template<class TOther, bool TOtherCounting, class TOtherRefPOD>
RDX_FORCEINLINE bool rdxRTRef<T, TBaseRef>::operator ==(const rdxRef<TOther, TOtherCounting, TOtherRefPOD> &other) const
{
	// Enforce that this is a sensical comparison
	bool validator = (static_cast<const T *>(RDX_CNULL) == static_cast<const TOther *>(RDX_CNULL));
	return rdxRefConverter<TBaseRef::DerefPODType>::Convert(other.GetPOD()) == this->GetPOD();
}

template<class T, class TBaseRef>
template<class TOther, class TOtherBaseRef>
RDX_FORCEINLINE bool rdxRTRef<T, TBaseRef>::operator !=(const rdxRTRef<TOther, TOtherBaseRef> &other) const
{
	return rdxRefConverter<TBaseRef::DerefPODType>::Convert(other.GetPOD()) != this->GetPOD();
}

template<class T, class TBaseRef>
template<class TOther, bool TOtherCounting, class TOtherRefPOD>
RDX_FORCEINLINE bool rdxRTRef<T, TBaseRef>::operator !=(const rdxRef<TOther, TOtherCounting, TOtherRefPOD> &other) const
{
	// Enforce that this is a sensical comparison
	bool validator = (static_cast<const T *>(RDX_CNULL) == static_cast<const TOther *>(RDX_CNULL));
	return rdxRefConverter<TBaseRef::DerefPODType>::Convert(other.GetPOD()) != this->GetPOD();
}

template<class T, class TBaseRef>
RDX_FORCEINLINE typename rdxWeakRTRef(T) rdxRTRef<T, TBaseRef>::ToWeakRTRef() const
{
	return typename rdxWeakRTRef(T)(rdxObjRef_CSignal_BaseRef, this->m_baseRef);
}

template<class T, class TBaseRef>
RDX_FORCEINLINE typename rdxWeakHdl(T) rdxRTRef<T, TBaseRef>::ToWeakHdl() const
{
	return typename rdxWeakHdl(T)(rdxObjRef_CSignal_BaseRef, this->m_baseRef);
}

template<class T, class TBaseRef>
RDX_FORCEINLINE typename rdxCRef(T) rdxRTRef<T, TBaseRef>::ToCRef() const
{
	return typename rdxCRef(T)(rdxObjRef_CSignal_BaseRef, this->m_baseRef);
}

template<class T, class TBaseRef>
RDX_FORCEINLINE const T *rdxRTRef<T, TBaseRef>::Data() const
{
	return static_cast<const T*>(rdxRefConverter<TBaseRef::DerefPODType>::Convert(this->m_baseRef.GetPOD()));
}

template<class T, class TBaseRef>
RDX_FORCEINLINE T *rdxRTRef<T, TBaseRef>::Modify() const
{
	return static_cast<T*>(rdxRefConverter<TBaseRef::DerefPODType>::Convert(this->m_baseRef.GetPOD()));
}

template<class T, class TBaseRef>
RDX_FORCEINLINE rdxRTRef<T, TBaseRef> rdxRTRef<T, TBaseRef>::Null()
{
	return rdxRTRef<T, TBaseRef>(rdxObjRef_CSignal_RawRef, NULL);
}

template<class T, class TBaseRef>
RDX_FORCEINLINE rdxRTRef<T, TBaseRef> rdxRTRef<T, TBaseRef>::FromPtr(T *ptr)
{
	return rdxObjRef<T, TCounting, false, TRefPOD>(rdxObjRef_CSignal_DataPointer, ptr);
}

template<class T, class TBaseRef>
RDX_FORCEINLINE bool rdxRTRef<T, TBaseRef>::IsNull() const
{
	return this->m_baseRef.GetPOD() == RDX_CNULL;
}
	
template<class T, class TBaseRef>
RDX_FORCEINLINE bool rdxRTRef<T, TBaseRef>::IsNotNull() const
{
	return this->m_baseRef.GetPOD() != RDX_CNULL;
}

template<class T, class TBaseRef>
template<class TNew>
RDX_FORCEINLINE rdxRef<TNew, false, TBaseRef> rdxRTRef<T, TBaseRef>::StaticCast() const
{
	const TNew *validator = static_cast<const TNew *>(static_cast<const T *>(NULL));
	return rdxRef<TNew, false, TBaseRef>(rdxObjRef_CSignal_RawRef, m_baseRef.GetPOD());
}

template<class T, class TBaseRef>
template<class SelfT, class TMemberType>
RDX_FORCEINLINE rdxOffsetObjRef<TMemberType, false, TBaseRef> rdxRTRef<T, TBaseRef>::OffsetMember(TMemberType SelfT::* member) const
{
	const rdxUInt8 *offsetLoc = reinterpret_cast<const rdxUInt8 *>(&(static_cast<const T *>(NULL)->*member));
	rdxLargeUInt offset = static_cast<rdxLargeUInt>(offsetLoc - reinterpret_cast<const rdxUInt8 *>(NULL));
	return rdxOffsetObjRef<TMemberType, TCounting, TRefPOD>(this->ToWeakRTRef(), offset);
}
