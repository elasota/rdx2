#ifndef __RDX_ASSERT_HPP__
#define __RDX_ASSERT_HPP__

#include "rdx_pragmas.hpp"

template<bool T>
struct rdxStaticAssertCheck
{
};

template<>
struct rdxStaticAssertCheck<true>
{
	RDX_FORCEINLINE static void STATIC_ASSERT_FAILED() { }
};

#define rdxStaticAssert(cond) do { rdxStaticAssertCheck<cond>::STATIC_ASSERT_FAILED(); } while(false)

#endif
