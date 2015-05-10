#include "rdx_coretypes.hpp"
#include "rdx_pragmas.hpp"

struct rdxX86AtomicBuiltins
{
	static void StoreFence();
	static void LoadFence();

	static rdxInt32 AtomicIncrement(volatile rdxInt32 *ptr);
	static rdxInt32 AtomicDecrement(volatile rdxInt32 *ptr);
	static rdxInt32 AtomicExchange(volatile rdxInt32 *dest, rdxInt32 repl);
	static rdxInt32 AtomicCompareExchange(volatile rdxInt32 *dest, rdxInt32 excg, rdxInt32 cmp);

	static rdxInt64 AtomicIncrement(volatile rdxInt64 *ptr);
	static rdxInt64 AtomicDecrement(volatile rdxInt64 *ptr);
	static rdxInt64 AtomicExchange(volatile rdxInt64 *dest, rdxInt64 repl);
	static rdxInt64 AtomicCompareExchange(volatile rdxInt64 *dest, rdxInt64 excg, rdxInt64 cmp);
};

template<int TAddressSize>
struct rdxX86ArchAtomic64BitBuiltins
{
};

#ifdef RDX_X64
template<>
struct rdxX86ArchAtomic64BitBuiltins<8>
{
	static rdxInt64 AtomicIncrement(volatile rdxInt64 *ptr);
	static rdxInt64 AtomicDecrement(volatile rdxInt64 *ptr);
	static rdxInt64 AtomicExchange(volatile rdxInt64 *dest, rdxInt64 repl);
};
#else
template<>
struct rdxX86ArchAtomic64BitBuiltins<4>
{
	static rdxInt64 AtomicIncrement(volatile rdxInt64 *ptr);
	static rdxInt64 AtomicDecrement(volatile rdxInt64 *ptr);
	static rdxInt64 AtomicExchange(volatile rdxInt64 *dest, rdxInt64 repl);
};
#endif

class rdxX86AtomicOverloadProxy
{
public:
	static RDX_FORCEINLINE rdxInt32 AtomicDecrementProxy(volatile rdxInt32 *ptr)
	{
		return static_cast<rdxInt32>(rdxX86AtomicBuiltins::AtomicDecrement(reinterpret_cast<volatile rdxInt32 *>(ptr)));
	}

	static RDX_FORCEINLINE rdxUInt32 AtomicDecrementProxy(volatile rdxUInt32 *ptr)
	{
		return static_cast<rdxUInt32>(rdxX86AtomicBuiltins::AtomicDecrement(reinterpret_cast<volatile rdxInt32 *>(ptr)));
	}

	static RDX_FORCEINLINE rdxInt64 AtomicDecrementProxy(volatile rdxInt64 *ptr)
	{
		return static_cast<rdxInt64>(rdxX86AtomicBuiltins::AtomicDecrement(reinterpret_cast<volatile rdxInt64 *>(ptr)));
	}

	static RDX_FORCEINLINE rdxUInt64 AtomicDecrementProxy(volatile rdxUInt64 *ptr)
	{
		return static_cast<rdxUInt64>(rdxX86AtomicBuiltins::AtomicDecrement(reinterpret_cast<volatile rdxInt64 *>(ptr)));
	}

	static RDX_FORCEINLINE rdxInt32 AtomicIncrementProxy(volatile rdxInt32 *ptr)
	{
		return static_cast<rdxInt32>(rdxX86AtomicBuiltins::AtomicIncrement(reinterpret_cast<volatile rdxInt32 *>(ptr)));
	}

	static RDX_FORCEINLINE rdxUInt32 AtomicIncrementProxy(volatile rdxUInt32 *ptr)
	{
		return static_cast<rdxUInt32>(rdxX86AtomicBuiltins::AtomicIncrement(reinterpret_cast<volatile rdxInt32 *>(ptr)));
	}

	static RDX_FORCEINLINE rdxInt64 AtomicIncrementProxy(volatile rdxInt64 *ptr)
	{
		return static_cast<rdxInt64>(rdxX86AtomicBuiltins::AtomicIncrement(reinterpret_cast<volatile rdxInt64 *>(ptr)));
	}

	static RDX_FORCEINLINE rdxUInt64 AtomicIncrementProxy(volatile rdxUInt64 *ptr)
	{
		return static_cast<rdxUInt64>(rdxX86AtomicBuiltins::AtomicIncrement(reinterpret_cast<volatile rdxInt64 *>(ptr)));
	}

	static RDX_FORCEINLINE rdxInt64 AtomicExchangeProxy(volatile rdxInt64 *dest, rdxInt64 repl)
	{
		return static_cast<rdxInt64>(rdxX86AtomicBuiltins::AtomicExchange(reinterpret_cast<volatile rdxInt64 *>(dest), repl));
	}

	static RDX_FORCEINLINE rdxUInt64 AtomicExchangeProxy(volatile rdxUInt64 *dest, rdxUInt64 repl)
	{
		return static_cast<rdxUInt64>(rdxX86AtomicBuiltins::AtomicExchange(reinterpret_cast<volatile rdxInt64 *>(dest), static_cast<rdxInt64>(repl)));
	}

	static RDX_FORCEINLINE rdxInt32 AtomicExchangeProxy(volatile rdxInt32 *dest, rdxInt32 repl)
	{
		return static_cast<rdxInt32>(rdxX86AtomicBuiltins::AtomicExchange(reinterpret_cast<volatile rdxInt32 *>(dest), repl));
	}

	static RDX_FORCEINLINE rdxUInt32 AtomicExchangeProxy(volatile rdxUInt32 *dest, rdxUInt32 repl)
	{
		return static_cast<rdxUInt32>(rdxX86AtomicBuiltins::AtomicExchange(reinterpret_cast<volatile rdxInt32 *>(dest), static_cast<rdxInt32>(repl)));
	}

	static RDX_FORCEINLINE void *AtomicExchangeProxy(void *volatile*dest, void *repl)
	{
		return static_cast<rdxUInt8*>(RDX_CNULL) + rdxX86AtomicBuiltins::AtomicExchange(reinterpret_cast<volatile rdxLargeInt *>(dest), static_cast<rdxUInt8*>(repl) - static_cast<rdxUInt8*>(RDX_CNULL));
	}

	static RDX_FORCEINLINE rdxInt64 AtomicCompareExchangeProxy(volatile rdxInt64 *dest, rdxInt64 excg, rdxInt64 cmp)
	{
		return static_cast<rdxInt64>(rdxX86AtomicBuiltins::AtomicCompareExchange(reinterpret_cast<volatile rdxInt64 *>(dest), excg, cmp));
	}

	static RDX_FORCEINLINE rdxUInt64 AtomicCompareExchangeProxy(volatile rdxUInt64 *dest, rdxUInt64 excg, rdxUInt64 cmp)
	{
		return static_cast<rdxUInt64>(rdxX86AtomicBuiltins::AtomicCompareExchange(reinterpret_cast<volatile rdxInt64 *>(dest), static_cast<rdxInt64>(excg), static_cast<rdxInt64>(cmp)));
	}

	static RDX_FORCEINLINE rdxInt32 AtomicCompareExchangeProxy(volatile rdxInt32 *dest, rdxInt32 excg, rdxInt32 cmp)
	{
		return static_cast<rdxInt32>(rdxX86AtomicBuiltins::AtomicCompareExchange(reinterpret_cast<volatile rdxInt32 *>(dest), static_cast<rdxInt32>(excg), static_cast<rdxInt32>(cmp)));
	}

	static RDX_FORCEINLINE rdxUInt32 AtomicCompareExchangeProxy(volatile rdxUInt32 *dest, rdxUInt32 excg, rdxUInt32 cmp)
	{
		return static_cast<rdxUInt32>(rdxX86AtomicBuiltins::AtomicCompareExchange(reinterpret_cast<volatile rdxInt32 *>(dest), static_cast<rdxInt32>(excg), static_cast<rdxInt32>(cmp)));
	}

	static RDX_FORCEINLINE void *AtomicCompareExchangeProxy(void *volatile* dest, void *excg, void *cmp)
	{
		rdxLargeInt excgInt = static_cast<rdxUInt8*>(excg) - static_cast<rdxUInt8*>(RDX_CNULL);
		rdxLargeInt cmpInt = static_cast<rdxUInt8*>(cmp) - static_cast<rdxUInt8*>(RDX_CNULL);
		return static_cast<rdxUInt8*>(RDX_CNULL) + rdxX86AtomicBuiltins::AtomicCompareExchange(reinterpret_cast<volatile rdxLargeInt *>(dest), excgInt, cmpInt);
	}
};

template<class T>
RDX_FORCEINLINE rdxAtomic<T>::rdxAtomic()
{
}


template<class T>
RDX_FORCEINLINE rdxAtomic<T>::rdxAtomic(const T &value)
{
	WriteUnfenced(value);
}

template<class T>
RDX_FORCEINLINE rdxAtomic<T>::rdxAtomic(const rdxAtomic<T> &other)
{
	WriteUnfenced(other.ReadUnfenced());
}

template<class T>
RDX_FORCEINLINE rdxAtomic<T>::~rdxAtomic()
{
}

template<class T>
RDX_FORCEINLINE rdxAtomic<T> &rdxAtomic<T>::operator =(const rdxAtomic<T> &other)
{
	WriteUnfenced(other.ReadUnfenced());
	return *this;
}


template<class T>
RDX_FORCEINLINE T rdxAtomic<T>::IncrementFullFence()
{
	return rdxX86AtomicOverloadProxy::AtomicIncrementProxy(&m_value);
}


template<class T>
RDX_FORCEINLINE T rdxAtomic<T>::DecrementFullFence()
{
	return rdxX86AtomicOverloadProxy::AtomicDecrementProxy(&m_value);
}

template<class T>
RDX_FORCEINLINE T rdxAtomic<T>::ReadBeforeLater() const
{
	T returnValue = *static_cast<const volatile T*>(&m_value);
	rdxX86AtomicBuiltins::LoadFence();
	return returnValue;
}

template<class T>
RDX_FORCEINLINE T rdxAtomic<T>::ReadUnfenced() const
{
	return m_value;
}

template<class T>
RDX_FORCEINLINE void rdxAtomic<T>::WriteAfterPrevious(T value)
{
	*static_cast<volatile T*>(&m_value) = value;
}

template<class T>
RDX_FORCEINLINE void rdxAtomic<T>::WriteBeforeLater(T value)
{
	*static_cast<volatile T*>(&m_value) = value;
}

template<class T>
RDX_FORCEINLINE void rdxAtomic<T>::WriteUnfenced(T value)
{
	m_value = value;
}

template<class T>
RDX_FORCEINLINE T rdxAtomic<T>::ExchangeFullFence(T replacement)
{
	return rdxX86AtomicOverloadProxy::AtomicExchangeProxy(&m_value, replacement);
}

template<class T>
RDX_FORCEINLINE T rdxAtomic<T>::CompareExchangeFullFence(T replacement, T compareTo)
{
	return rdxX86AtomicOverloadProxy::AtomicCompareExchangeProxy(&m_value, replacement, compareTo);
}

RDX_FORCEINLINE rdxInt64 rdxX86AtomicBuiltins::AtomicIncrement(volatile rdxInt64 *ptr)
{
	return rdxX86ArchAtomic64BitBuiltins<sizeof(void*)>::AtomicIncrement(ptr);
}

RDX_FORCEINLINE rdxInt64 rdxX86AtomicBuiltins::AtomicDecrement(volatile rdxInt64 *ptr)
{
	return rdxX86ArchAtomic64BitBuiltins<sizeof(void*)>::AtomicDecrement(ptr);
}

RDX_FORCEINLINE rdxInt64 rdxX86AtomicBuiltins::AtomicExchange(volatile rdxInt64 *dest, rdxInt64 repl)
{
	return rdxX86ArchAtomic64BitBuiltins<sizeof(void*)>::AtomicExchange(dest, repl);
}

#ifndef RDX_X64

inline rdxInt64 rdxX86ArchAtomic64BitBuiltins<4>::AtomicDecrement(volatile rdxInt64 *ptr)
{
	rdxInt64 old;
	do
	{
		old = *ptr;
	} while(rdxX86AtomicBuiltins::AtomicCompareExchange(ptr, old - 1, old) != old);
	return old - 1;
}

inline rdxInt64 rdxX86ArchAtomic64BitBuiltins<4>::AtomicIncrement(volatile rdxInt64 *ptr)
{
	rdxInt64 old;
	do
	{
		old = *ptr;
	} while(rdxX86AtomicBuiltins::AtomicCompareExchange(ptr, old + 1, old) != old);
	return old + 1;
}

inline rdxInt64 rdxX86ArchAtomic64BitBuiltins<4>::AtomicExchange(volatile rdxInt64 *dest, rdxInt64 repl)
{
	rdxInt64 old;
	do
	{
		old = *dest;
	} while(rdxX86AtomicBuiltins::AtomicCompareExchange(dest, old, old) != old);
	return old;
}

#endif

#ifdef __GNUC__
#include "rdx_atomic_x86_gcc.hpp"
#endif

#ifdef _MSC_VER
#include "rdx_atomic_x86_msvc.hpp"
#endif
