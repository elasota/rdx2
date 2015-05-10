#include <intrin.h>
#include "rdx_coretypes.hpp"


//#define RDX_64BIT

#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedCompareExchange64)
#pragma intrinsic(_mm_sfence)

#ifdef RDX_64BIT
	#pragma intrinsic(_InterlockedDecrement64)
	#pragma intrinsic(_InterlockedIncrement64)
	#pragma intrinsic(_InterlockedExchangeAdd64)
	#pragma intrinsic(_InterlockedSubtract64)
	#pragma intrinsic(_InterlockedExchange64)
#endif

class rdxWin32InterlockOverloadProxy
{
public:
	static RDX_FORCEINLINE rdxInt32 InterlockedDecrementProxy(volatile rdxInt32 *ptr)
	{
		return static_cast<rdxInt32>(InterlockedDecrement(reinterpret_cast<volatile long *>(ptr)));
	}

	static RDX_FORCEINLINE rdxUInt32 InterlockedDecrementProxy(volatile rdxUInt32 *ptr)
	{
		return static_cast<rdxUInt32>(_InterlockedDecrement(reinterpret_cast<volatile long *>(ptr)));
	}
	
#ifdef RDX_X64
	static RDX_FORCEINLINE rdxInt64 InterlockedDecrementProxy(volatile rdxInt64 *ptr)
	{
		return static_cast<rdxInt64>(_InterlockedDecrement64(reinterpret_cast<volatile __int64 *>(ptr)));
	}
#else
	static inline rdxInt64 InterlockedDecrementProxy(volatile rdxInt64 *ptr)
	{
		__int64 old;
		do
		{
			old = *ptr;
		} while(_InterlockedCompareExchange64(ptr, old - 1, old) != old);
		return old - 1;
	}
#endif

	static RDX_FORCEINLINE rdxUInt64 InterlockedDecrementProxy(volatile rdxUInt64 *ptr)
	{
		return static_cast<rdxUInt64>(InterlockedDecrementProxy(reinterpret_cast<volatile __int64 *>(ptr)));
	}

	static RDX_FORCEINLINE rdxInt32 InterlockedIncrementProxy(volatile rdxInt32 *ptr)
	{
		return static_cast<rdxInt32>(_InterlockedIncrement(reinterpret_cast<volatile long *>(ptr)));
	}

	static RDX_FORCEINLINE rdxUInt32 InterlockedIncrementProxy(volatile rdxUInt32 *ptr)
	{
		return static_cast<rdxUInt32>(_InterlockedIncrement(reinterpret_cast<volatile long *>(ptr)));
	}
	
#ifdef RDX_X64
	static RDX_FORCEINLINE rdxInt64 InterlockedIncrementProxy(volatile rdxInt64 *ptr)
	{
		return static_cast<rdxInt64>(_InterlockedIncrement64(reinterpret_cast<volatile __int64 *>(ptr)));
	}
#else
	static inline rdxInt64 InterlockedIncrementProxy(volatile rdxInt64 *ptr)
	{
		__int64 old;
		do
		{
			old = *ptr;
		} while(_InterlockedCompareExchange64(ptr, old + 1, old) != old);
		return old + 1;
	}
#endif

	static RDX_FORCEINLINE rdxUInt64 InterlockedIncrementProxy(volatile rdxUInt64 *ptr)
	{
		return static_cast<rdxUInt64>(InterlockedIncrementProxy(reinterpret_cast<volatile __int64 *>(ptr)));
	}
	
#ifdef RDX_X64
	static RDX_FORCEINLINE rdxInt64 InterlockedExchangeProxy(volatile rdxInt64 *dest, rdxInt64 repl)
	{
		return static_cast<rdxInt64>(_InterlockedExchange64(reinterpret_cast<volatile __int64 *>(dest), repl));
	}
#else
	static inline rdxInt64 InterlockedExchangeProxy(volatile rdxInt64 *dest, rdxInt64 repl)
	{
		__int64 old;
		do
		{
			old = *dest;
		} while(_InterlockedCompareExchange64(dest, old, old) != old);
		return old;
#endif
	}

	static RDX_FORCEINLINE rdxUInt64 InterlockedExchangeProxy(volatile rdxUInt64 *dest, rdxUInt64 repl)
	{
		return static_cast<rdxUInt64>(InterlockedExchangeProxy(reinterpret_cast<volatile __int64 *>(dest), static_cast<__int64>(repl)));
	}

	static RDX_FORCEINLINE rdxInt32 InterlockedExchangeProxy(volatile rdxInt32 *dest, rdxInt32 repl)
	{
		return static_cast<rdxInt32>(_InterlockedExchange(reinterpret_cast<volatile long *>(dest), repl));
	}

	static RDX_FORCEINLINE rdxUInt32 InterlockedExchangeProxy(volatile rdxUInt32 *dest, rdxUInt32 repl)
	{
		return static_cast<rdxUInt32>(_InterlockedExchange(reinterpret_cast<volatile long *>(dest), static_cast<long>(repl)));
	}

	static RDX_FORCEINLINE void *InterlockedExchangeProxy(void *volatile*dest, void *repl)
	{
		return static_cast<rdxUInt8*>(NULL) + InterlockedExchangeProxy(reinterpret_cast<volatile rdxLargeInt *>(dest), static_cast<rdxUInt8*>(repl) - static_cast<rdxUInt8*>(NULL));
	}

	static RDX_FORCEINLINE rdxInt64 InterlockedCompareExchangeProxy(volatile rdxInt64 *dest, rdxInt64 excg, rdxInt64 cmp)
	{
		return static_cast<rdxInt64>(_InterlockedCompareExchange64(reinterpret_cast<volatile __int64 *>(dest), excg, cmp));
	}

	static RDX_FORCEINLINE rdxUInt64 InterlockedCompareExchangeProxy(volatile rdxUInt64 *dest, rdxUInt64 excg, rdxUInt64 cmp)
	{
		return static_cast<rdxUInt64>(_InterlockedCompareExchange64(reinterpret_cast<volatile __int64 *>(dest), static_cast<__int64>(excg), static_cast<__int64>(cmp)));
	}

	static RDX_FORCEINLINE rdxInt32 InterlockedCompareExchangeProxy(volatile rdxInt32 *dest, rdxInt32 excg, rdxInt32 cmp)
	{
		return static_cast<rdxInt32>(_InterlockedCompareExchange(reinterpret_cast<volatile long *>(dest), static_cast<long>(excg), static_cast<long>(cmp)));
	}

	static RDX_FORCEINLINE rdxUInt32 InterlockedCompareExchangeProxy(volatile rdxUInt32 *dest, rdxUInt32 excg, rdxUInt32 cmp)
	{
		return static_cast<rdxUInt32>(_InterlockedCompareExchange(reinterpret_cast<volatile long *>(dest), static_cast<long>(excg), static_cast<long>(cmp)));
	}

	static RDX_FORCEINLINE void *InterlockedCompareExchangeProxy(void *volatile* dest, void *excg, void *cmp)
	{
		rdxLargeInt excgInt = static_cast<rdxUInt8*>(excg) - static_cast<rdxUInt8*>(NULL);
		rdxLargeInt cmpInt = static_cast<rdxUInt8*>(cmp) - static_cast<rdxUInt8*>(NULL);
		return static_cast<rdxUInt8*>(NULL) + InterlockedCompareExchangeProxy(reinterpret_cast<volatile rdxLargeInt *>(dest), excgInt, cmpInt);
	}
};

template<class T>
class rdxWin32AtomicOpsProxy
{
public:
	static RDX_FORCEINLINE T ReadAfterPrevious(const T *pValue)
	{
		_mm_sfence();
		return *static_cast<const volatile T*>(pValue);
	}

	static RDX_FORCEINLINE void WriteAfterPrevious(T *pValue, T value)
	{
		_mm_sfence();
		*static_cast<volatile T*>(pValue) = value;
	}

	static RDX_FORCEINLINE void WriteBeforeLater(T *pValue, T value)
	{
		*static_cast<volatile T*>(pValue) = value;
		_mm_sfence();
	}

	static RDX_FORCEINLINE void WriteFullFence(volatile T *pValue, T value)
	{
		_mm_sfence();
		*static_cast<volatile T*>(pValue) = value;
		_mm_sfence();
	}

	static RDX_FORCEINLINE T ExchangeFullFence(volatile T *pValue, T replacement)
	{
		return rdxWin32InterlockOverloadProxy::InterlockedExchangeProxy(pValue, replacement);
	}

	static RDX_FORCEINLINE T CompareExchangeFullFence(volatile T *pValue, T replacement, T compareTo)
	{
		return rdxWin32InterlockOverloadProxy::InterlockedCompareExchangeProxy(pValue, replacement, compareTo);
	}

	static RDX_FORCEINLINE T IncrementFullFence(volatile T *pValue)
	{
		return rdxWin32InterlockOverloadProxy::InterlockedIncrementProxy(pValue);
	}

	static RDX_FORCEINLINE T DecrementFullFence(volatile T *pValue)
	{
		return rdxWin32InterlockOverloadProxy::InterlockedDecrementProxy(pValue);
	}

	static RDX_FORCEINLINE void WriteUnfenced(T *pValue, T value)
	{
		*pValue = value;
	}

	static RDX_FORCEINLINE T ReadUnfenced(const T *pValue)
	{
		return *pValue;
	}

	static RDX_FORCEINLINE T CompareExchangeUnfenced(T *pValue, T replacement, T compareTo)
	{
		T original = *pValue;
		if(original == compareTo)
			*pValue = replacement;
		return original;
	}
};

template<class T>
RDX_FORCEINLINE rdxAtomic<T>::rdxAtomic()
{
}

template<class T>
RDX_FORCEINLINE rdxAtomic<T>::rdxAtomic(const rdxAtomic<T> &other)
	: m_value(other.m_value)
{
}

template<class T>
RDX_FORCEINLINE rdxAtomic<T> &rdxAtomic<T>::operator =(const rdxAtomic<T> &other)
{
	m_value = other.m_value;
	return *this;
}

template<class T>
RDX_FORCEINLINE T rdxAtomic<T>::IncrementFullFence()
{
	return rdxWin32AtomicOpsProxy<T>::IncrementFullFence(&m_value);
}


template<class T>
RDX_FORCEINLINE T rdxAtomic<T>::DecrementFullFence()
{
	return rdxWin32AtomicOpsProxy<T>::DecrementFullFence(&m_value);
}

template<class T>
RDX_FORCEINLINE T rdxAtomic<T>::ReadUnfenced() const
{
	return m_value;
}

template<class T>
RDX_FORCEINLINE void rdxAtomic<T>::WriteUnfenced(T value)
{
	m_value = value;
}

template<class T>
RDX_FORCEINLINE T rdxAtomic<T>::ExchangeFullFence(T replacement)
{
	return rdxWin32AtomicOpsProxy<T>::ExchangeFullFence(&m_value, replacement);
}

template<class T>
RDX_FORCEINLINE T rdxAtomic<T>::CompareExchangeFullFence(T replacement, T compareTo)
{
	return rdxWin32AtomicOpsProxy<T>::CompareExchangeFullFence(&m_value, replacement, compareTo);
}

