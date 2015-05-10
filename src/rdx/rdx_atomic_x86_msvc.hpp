#include <intrin.h>

#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedCompareExchange64)

#ifdef RDX_64BIT
	#pragma intrinsic(_InterlockedDecrement64)
	#pragma intrinsic(_InterlockedIncrement64)
	#pragma intrinsic(_InterlockedExchangeAdd64)
	#pragma intrinsic(_InterlockedSubtract64)
	#pragma intrinsic(_InterlockedExchange64)
#endif

RDX_FORCEINLINE rdxInt32 rdxX86AtomicBuiltins::AtomicIncrement(volatile rdxInt32 *ptr)
{
	return _InterlockedIncrement(reinterpret_cast<volatile long *>(ptr));
}

RDX_FORCEINLINE rdxInt32 rdxX86AtomicBuiltins::AtomicDecrement(volatile rdxInt32 *ptr)
{
	return _InterlockedDecrement(reinterpret_cast<volatile long *>(ptr));
}

RDX_FORCEINLINE rdxInt32 rdxX86AtomicBuiltins::AtomicExchange(volatile rdxInt32 *dest, rdxInt32 repl)
{
	return _InterlockedExchange(reinterpret_cast<volatile long *>(dest), repl);
}

RDX_FORCEINLINE rdxInt32 rdxX86AtomicBuiltins::AtomicCompareExchange(volatile rdxInt32 *dest, rdxInt32 excg, rdxInt32 cmp)
{
	return _InterlockedCompareExchange(reinterpret_cast<volatile long *>(dest), excg, cmp);
}

RDX_FORCEINLINE rdxInt64 rdxX86AtomicBuiltins::AtomicCompareExchange(volatile rdxInt64 *dest, rdxInt64 excg, rdxInt64 cmp)
{
	return _InterlockedCompareExchange64(dest, excg, cmp);
}

