
RDX_FORCEINLINE rdxInt32 rdxX86AtomicBuiltins::AtomicIncrement(volatile rdxInt32 *ptr)
{
	return __atomic_add_fetch(ptr, 1, __ATOMIC_SEQ_CST);
}

RDX_FORCEINLINE rdxInt32 rdxX86AtomicBuiltins::AtomicDecrement(volatile rdxInt32 *ptr)
{
	return __atomic_sub_fetch(ptr, 1, __ATOMIC_SEQ_CST);
}

RDX_FORCEINLINE rdxInt32 rdxX86AtomicBuiltins::AtomicExchange(volatile rdxInt32 *dest, rdxInt32 repl)
{
	rdxInt32 rv;
	__atomic_exchange(dest, &repl, &rv, __ATOMIC_SEQ_CST);
	return rv;
}


RDX_FORCEINLINE rdxInt32 rdxX86AtomicBuiltins::AtomicCompareExchange(volatile rdxInt32 *dest, rdxInt32 excg, rdxInt32 cmp)
{
	__atomic_compare_exchange_n(dest, &cmp, &excg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	return excg;
}

RDX_FORCEINLINE rdxInt64 rdxX86AtomicBuiltins::AtomicCompareExchange(volatile rdxInt64 *dest, rdxInt64 excg, rdxInt64 cmp)
{
	__atomic_compare_exchange_n(dest, &cmp, &excg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	return excg;
}

