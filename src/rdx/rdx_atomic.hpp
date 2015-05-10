#ifndef __RDX_ATOMIC_HPP__
#define __RDX_ATOMIC_HPP__

template<class T>
struct rdxAtomic
{
private:
	T m_value;

public:
	typedef T ContainedType;

	rdxAtomic();
	rdxAtomic(const T &value);
	rdxAtomic(const rdxAtomic<T> &other);
	~rdxAtomic();
	rdxAtomic<T> &operator =(const rdxAtomic<T> &other);

	T ReadAfterPrevious() const;
	T ReadBeforeLater() const;
	T ReadUnfenced() const;
	T ReadFullFence() const;

	void WriteAfterPrevious(T value);
	void WriteBeforeLater(T value);
	void WriteUnfenced(T value);
	void WriteFullFence(T value);

	T IncrementFullFence();
	T IncrementUnfenced();

	T DecrementFullFence();
	T DecrementUnfenced();

	// Compares *exchangeAddr to compareTo, and if equal, sets it to replacement.  Returns the original value of *exchangeAddr
	T CompareExchangeFullFence(T replacement, T compareTo);
	T CompareExchangeUnfenced(T replacement, T compareTo);

	// Writes replacement, returns original value
	T ExchangeFullFence(T replacement);
	T ExchangeUnfenced(T replacement);
};

typedef rdxAtomic<rdxLargeUInt> rdxAtomicUInt;

#include "rdx_atomic_x86.hpp"

#endif
