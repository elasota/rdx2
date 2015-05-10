#include <intrin.h>


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

