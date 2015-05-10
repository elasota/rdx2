#ifndef __RDX_LANGUTILS_HPP__
#define __RDX_LANGUTILS_HPP__

class rdxCObject;

#ifdef RDX_ENABLE_SMP
#	define RDX_POSSIBLY_VOLATILE	volatile
#else
#	define RDX_POSSIBLY_VOLATILE
#endif

inline rdxLargeUInt rdxBasedPropertyOffsetOf(const rdxCObject *objectBase, const void *prop)
{
	return static_cast<rdxLargeUInt>(reinterpret_cast<const rdxUInt8 *>(prop) - reinterpret_cast<const rdxUInt8 *>(objectBase));
}

inline rdxLargeUInt rdxBasedPropertyOffsetOf(const void *structBase, const void *prop)
{
	return static_cast<rdxLargeUInt>(reinterpret_cast<const rdxUInt8 *>(prop) - reinterpret_cast<const rdxUInt8 *>(structBase));
}

template<class TType, class TMember>
inline rdxLargeUInt rdxRuntimePropertyOffsetOf(TMember TType::* member)
{
	const TType *base = reinterpret_cast<const TType *>(static_cast<const rdxUInt8 *>(0) + 1);
	const void *prop = &(base->*member);
	return rdxBasedPropertyOffsetOf(base, prop);
}

template<class TType, class TMember>
inline rdxLargeUInt rdxOffsetOf(TMember TType::* member)
{
	const TType *base = reinterpret_cast<const TType *>(static_cast<const rdxUInt8 *>(0) + 1);
	return static_cast<rdxLargeUInt>(reinterpret_cast<const rdxUInt8 *>(&(base->*member)) - reinterpret_cast<const rdxUInt8 *>(base));
}

template<class TClass, class TBaseClass>
inline rdxLargeUInt rdxOffsetOfBaseClass()
{
	const TClass *pClass = reinterpret_cast<const TClass *>(static_cast<const rdxUInt8*>(0) + 1);
	const TBaseClass *pBase = pClass;
	return static_cast<rdxLargeUInt>(reinterpret_cast<const rdxUInt8 *>(pBase) - reinterpret_cast<const rdxUInt8 *>(pClass));
}


template<class T>
struct rdxAlignmentCheckHelper
{
	rdxUInt8 pad;
	T v;
};

template<class T>
struct rdxAlignmentDeduction
{
	static const rdxLargeUInt ALIGNMENT = sizeof(rdxAlignmentCheckHelper<T>) - sizeof(T);
};

#define rdxAlignOf(T) (::rdxAlignmentDeduction<T>::ALIGNMENT)


#endif
