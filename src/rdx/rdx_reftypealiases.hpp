#ifndef __RDX_REFTYPEALIASES_HPP__
#define __RDX_REFTYPEALIASES_HPP__

template<class T, class TRefPOD> struct rdxRTRef;
template<class T, bool TCounting, class TRefPOD> struct rdxRef;
template<bool TCounting, class TBaseRef> struct rdxTypelessOffsetObjRef;
template<class TRefPOD> struct rdxTypelessRTRef;
template<class TObjectRefType> struct rdxBaseRef;

template<class T, bool TCounting, class TRefPOD> struct rdxOffsetObjRef;
template<class TContents> class rdxCArray;

class rdxCObject;
struct rdxGCInfo;
struct rdxSObjectInterfaceImplementation;

typedef rdxBaseRef<rdxCObject> rdxBaseRTRef;
typedef rdxBaseRef<rdxGCInfo> rdxBaseHdl;
typedef rdxBaseRef<rdxSObjectInterfaceImplementation> rdxBaseIfcRef;

enum rdxBaseRef_ConSignal_Null_Type
{
	rdxBaseRef_ConSignal_Null
};

enum rdxBaseRef_ConSignal_POD_Type
{
	rdxBaseRef_ConSignal_POD
};

enum rdxBaseRef_ConSignal_Counting_Type
{
	rdxBaseRef_ConSignal_Counting
};

enum rdxBaseRef_ConSignal_Marking_Type
{
	rdxBaseRef_ConSignal_Marking
};

template<bool TCounting, bool TMarking>
struct rdxBaseRef_ConSignal
{
};

template<>
struct rdxBaseRef_ConSignal<false, false>
{
	static const rdxBaseRef_ConSignal_POD_Type Value = rdxBaseRef_ConSignal_POD;
};


template<>
struct rdxBaseRef_ConSignal<false, true>
{
	// If incremental GC is disabled, traced refs are weak references and all references are resolved
	// via sweeps instead
#ifdef RDX_USE_INCREMENTAL_GC
	static const rdxBaseRef_ConSignal_Marking_Type Value = rdxBaseRef_ConSignal_Marking;
#else
	static const rdxBaseRef_ConSignal_POD_Type Value = rdxBaseRef_ConSignal_POD;
#endif
};

template<>
struct rdxBaseRef_ConSignal<true, true>
{
	static const rdxBaseRef_ConSignal_Counting_Type Value = rdxBaseRef_ConSignal_Counting;
};

template<class T>
struct rdxRefType
{
	//typedef rdxBaseRef<rdxSObjectInterfaceImplementation*> BaseIfcRef;

	typedef rdxRTRef<T, rdxBaseRTRef> TracedRTRef;
	typedef rdxRef<T, true, rdxBaseHdl> CRef;

	typedef rdxRef<T, false, rdxBaseRTRef> WeakRTRef;
	typedef rdxRef<T, false, rdxBaseHdl> WeakHdl;

	typedef rdxOffsetObjRef<T, false, rdxBaseRTRef> WeakOffsetRTRef;
	typedef rdxOffsetObjRef<T, false, rdxBaseHdl> WeakOffsetHdl;
	typedef rdxOffsetObjRef<T, true, rdxBaseHdl> WeakOffsetCRef;

	typedef rdxRTRef<T, rdxBaseIfcRef> TracedIRef;
	typedef rdxRef<T, false, rdxBaseIfcRef> WeakIRef;

private:
	rdxRefType();
	rdxRefType(const rdxRefType& rs);
	~rdxRefType();
	rdxRefType& operator=(const rdxRefType& rs);
};

typedef rdxTypelessOffsetObjRef<false, rdxBaseRTRef> rdxWeakTypelessOffsetRTRef;
typedef rdxTypelessOffsetObjRef<false, rdxBaseHdl> rdxWeakTypelessOffsetHdl;
typedef rdxTypelessOffsetObjRef<true, rdxBaseHdl> rdxWeakTypelessOffsetCRef;

typedef rdxTypelessRTRef<rdxBaseRTRef> rdxTracedTypelessRTRef;
typedef rdxTypelessRTRef<rdxBaseIfcRef> rdxTracedTypelessIfcRef;

#define rdxTracedRTRef(T) rdxRefType<T>::TracedRTRef
#define rdxTracedIRef(T) rdxRefType<T>::TracedIRef
#define rdxTracedHdl(T) rdxRefType<T>::TracedHdl
#define rdxWeakHdl(T) rdxRefType<T>::WeakHdl
#define rdxWeakRTRef(T) rdxRefType<T>::WeakRTRef
#define rdxWeakIRef(T) rdxRefType<T>::WeakIRef
#define rdxCRef(T) rdxRefType<T>::CRef

typedef rdxRefType<rdxSObjectInterfaceImplementation>::TracedIRef rdxTracedTypelessIRef;
typedef rdxRefType<rdxSObjectInterfaceImplementation>::WeakIRef rdxWeakTypelessIRef;

#define rdxTracedArrayRTRef(T) rdxRefType<rdxCArray<T> >::TracedRTRef
#define rdxTracedArrayHdl(T) rdxRefType<rdxCArray<T> >::TracedHdl
#define rdxWeakArrayHdl(T) rdxRefType<rdxCArray<T> >::WeakHdl
#define rdxWeakArrayRTRef(T) rdxRefType<rdxCArray<T> >::WeakRTRef
#define rdxArrayCRef(T) rdxRefType<rdxCArray<T> >::CRef

#define rdxWeakOffsetRTRef(T) rdxRefType<T>::WeakOffsetRTRef
#define rdxWeakOffsetHdl(T) rdxRefType<T>::WeakOffsetHdl
//#define rdxWeakTypelessOffsetHdl(T) rdxRefType<T>::WeakTypelessOffsetHdl
//#define rdxWeakTypelessOffsetRTRef(T) rdxRefType<T>::WeakTypelessOffsetRTRef



#endif
