#include "../rdx/rdx.h"
#include "../rdx/rdx_pccm.hpp"
#include "../rdx/rdx_objectmanagement.hpp"
#include "../rdx/rdx_ilcomp.hpp"
#include "../rdx/rdx_runtime.hpp"
#include "../rdx/rdx_lut.hpp"
using namespace RDX;
using namespace RDX::ObjectManagement;
using namespace RDX::Programmability;
using namespace RDX::Programmability::RuntimeUtilities;
#include "rdx_corelib_pccm.hpp"
static int ThrowException(RuntimeThread *t, Exception *e, const void *ip)
{
	t->ex = e;
	t->frame.ip = ip;
	return RuntimeState::Exception;
}
#define THROWEXCEPTION(e, instrNum) return ThrowException(thread, static_cast<Exception*>(e), instrTable + instrNum + 1)
#define NULLCHECK(v, instrNum) if(v == NULL) NULLREFEXCEPTION(instrNum)
#define EXITFRAME do { thread->frame.bp = reinterpret_cast<RuntimeStackFrame *>(bp); return RuntimeState::AbandonFrame; } while(0)
#define INVALIDOPERATIONEXCEPTION(instrNum) THROWEXCEPTION(providerDictionary[X_InvalidOperationException], instrNum)
#define INCOMPATIBLECONVERSION(instrNum) THROWEXCEPTION(providerDictionary[X_IncompatibleConversionException], instrNum)
#define NULLREFEXCEPTION(instrNum) THROWEXCEPTION(providerDictionary[X_NullReferenceException], instrNum)
#define DIVIDEBYZEROEXCEPTION(instrNum) THROWEXCEPTION(providerDictionary[X_DivideByZeroException], instrNum)
#define OUTOFBOUNDS(instrNum) THROWEXCEPTION(providerDictionary[X_IndexOutOfBoundsException], instrNum)
#include "../rdx/rdx_pccm_private.hpp"
#define TICK(instrNum)	do {\
		if((--thread->timeout) == 0)\
		{\
			thread->frame.ip = instrTable + (instrNum + 1);\
			return RuntimeState::TimedOut;\
		}\
	} while(0)


	// ***** Core.GenericException/methods/description()
static int f1(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func1_start;
__func1_instr0:
{
// instr 0
// rdxsrc/Internal.rdx [line 174]
// instr 1
	RuntimeValue4 __prv_offs_8_sz4_1;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-8)));
		__prv_offs_8_sz4_1 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_8_sz4_2;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_8_sz4_1);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_2);
		*dest = *src;
	}
// instr 2
	RuntimeValue8 __bp_offs_8_sz8_3;
	__bp_offs_8_sz8_3.rtp.objectRef = __bp_offs_8_sz4_2.p;
	__bp_offs_8_sz8_3.rtp.valueRef = reinterpret_cast<UInt8 *>(__bp_offs_8_sz4_2.p) + (4);
// instr 3
	RuntimeValue4 __prv_offs0_sz4_4;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		const RuntimePointer<void> *srcPtr = &__bp_offs_8_sz8_3.rtp;
		src = static_cast<const RuntimeValue4 *>(srcPtr->valueRef);
		dest = reinterpret_cast<RuntimeValue4 *>(&__prv_offs0_sz4_4);
		*dest = *src;
	}
// instr 4
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(prv) + (0)))) = __prv_offs0_sz4_4;
}
{
	EXITFRAME;
// instr 5
}
{
	INVALIDOPERATIONEXCEPTION(5);
}
__func1_start:
switch(startInstruction)
{
case 0: goto __func1_instr0;
};

return 0;

} // ************* END FUNC


	// ***** Core.Collections.ListBase/methods/IncreaseSize(Core.int)
static int f2(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func2_start;
__func2_instr0:
{
// instr 0
// rdxsrc/List.rdx [line 36]
// instr 1
	RuntimeValue4 __bp_offs_4_sz4_5;
	memset(&__bp_offs_4_sz4_5, 0, 4);
// instr 2
	RuntimeValue4 __prv_offs_8_sz4_6;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-8)));
		__prv_offs_8_sz4_6 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_32_sz4_7;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_8_sz4_6);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_32_sz4_7);
		*dest = *src;
	}
// instr 3
	RuntimeValue8 __bp_offs_32_sz8_8;
	__bp_offs_32_sz8_8.rtp.objectRef = __bp_offs_32_sz4_7.p;
	__bp_offs_32_sz8_8.rtp.valueRef = reinterpret_cast<UInt8 *>(__bp_offs_32_sz4_7.p) + (4);
// instr 4
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		const RuntimePointer<void> *srcPtr = &__bp_offs_32_sz8_8.rtp;
		src = static_cast<const RuntimeValue4 *>(srcPtr->valueRef);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_4_sz4_5);
		*dest = *src;
	}
// instr 5
// rdxsrc/List.rdx [line 37]
// instr 6
	RuntimeValue4 __bp_offs_8_sz4_9;
	memset(&__bp_offs_8_sz4_9, 0, 4);
// instr 7
	RuntimeValue4 __bp_offs_40_sz4_10;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_8_sz4_6);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_10);
		*dest = *src;
	}
// instr 8
	RuntimeValue8 __bp_offs_40_sz8_11;
	__bp_offs_40_sz8_11.rtp.objectRef = __bp_offs_40_sz4_10.p;
	__bp_offs_40_sz8_11.rtp.valueRef = reinterpret_cast<UInt8 *>(__bp_offs_40_sz4_10.p) + (4);
// instr 9
	RuntimeValue4 __bp_offs_40_sz4_12;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		const RuntimePointer<void> *srcPtr = &__bp_offs_40_sz8_11.rtp;
		src = static_cast<const RuntimeValue4 *>(srcPtr->valueRef);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_12);
		*dest = *src;
	}
// instr 10
	RuntimeValue4 __prv_offs_16_sz4_13;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-16)));
		__prv_offs_16_sz4_13 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_48_sz4_14;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_16_sz4_13);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_48_sz4_14);
		*dest = *src;
	}
// instr 11
	RuntimeValue4 __bp_offs_32_sz4_15;
	__bp_offs_32_sz4_15.i32 = __bp_offs_40_sz4_12.i32 + __bp_offs_48_sz4_14.i32;
// instr 12
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_32_sz4_15);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_9);
		*dest = *src;
	}
// instr 13
// rdxsrc/List.rdx [line 38]
// instr 14
	RuntimeValue4 __bp_offs_32_sz4_16;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_9);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_32_sz4_16);
		*dest = *src;
	}
// instr 15
	RuntimeValue4 __bp_offs_40_sz4_17;
	__bp_offs_40_sz4_17.addrs[0] = PackAddress(0,0,0,0);
// instr 16
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_16;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_17;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_5;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_9;
	if(__bp_offs_32_sz4_16.i32 < __bp_offs_40_sz4_17.i32)
		goto __func2_instr23;
// instr 17
	RuntimeValue4 __bp_offs_32_sz4_18;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_9);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_32_sz4_18);
		*dest = *src;
	}
// instr 18
	RuntimeValue4 __bp_offs_40_sz4_19;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_8_sz4_6);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_19);
		*dest = *src;
	}
// instr 19
	RuntimeValue8 __bp_offs_40_sz8_20;
	__bp_offs_40_sz8_20.rtp.objectRef = __bp_offs_40_sz4_19.p;
	__bp_offs_40_sz8_20.rtp.valueRef = reinterpret_cast<UInt8 *>(__bp_offs_40_sz4_19.p) + (4);
// instr 20
	RuntimeValue4 __bp_offs_40_sz4_21;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		const RuntimePointer<void> *srcPtr = &__bp_offs_40_sz8_20.rtp;
		src = static_cast<const RuntimeValue4 *>(srcPtr->valueRef);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_21);
		*dest = *src;
	}
// instr 21
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_18;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_21;
	if(__bp_offs_32_sz4_18.i32 >= __bp_offs_40_sz4_21.i32)
		goto __func2_instr26;
// instr 22
// rdxsrc/List.rdx [line 39]
// instr 23
}
__func2_instr23:
{
	RuntimeValue4 __bp_offs_32_sz4_22;
	__bp_offs_32_sz4_22.cp = resArgs[3];
// instr 24
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_22;
	THROWEXCEPTION(__bp_offs_32_sz4_22.p, 24);
// instr 25
// rdxsrc/List.rdx [line 41]
// instr 26
}
__func2_instr26:
{
	RuntimeValue4 __bp_offs_12_sz4_23;
	memset(&__bp_offs_12_sz4_23, 0, 4);
// instr 27
	RuntimeValue4 __prv_offs_8_sz4_24;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-8)));
		__prv_offs_8_sz4_24 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_32_sz4_25;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_8_sz4_24);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_32_sz4_25);
		*dest = *src;
	}
// instr 28
	RuntimeValue8 __bp_offs_32_sz8_26;
	__bp_offs_32_sz8_26.rtp.objectRef = __bp_offs_32_sz4_25.p;
	__bp_offs_32_sz8_26.rtp.valueRef = reinterpret_cast<UInt8 *>(__bp_offs_32_sz4_25.p) + (0);
// instr 29
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		const RuntimePointer<void> *srcPtr = &__bp_offs_32_sz8_26.rtp;
		src = static_cast<const RuntimeValue4 *>(srcPtr->valueRef);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_12_sz4_23);
		*dest = *src;
	}
// instr 30
// rdxsrc/List.rdx [line 42]
// instr 31
	RuntimeValue4 __bp_offs_8_sz4_27;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-8)));
		__bp_offs_8_sz4_27 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_32_sz4_28;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_27);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_32_sz4_28);
		*dest = *src;
	}
// instr 32
	RuntimeValue4 __bp_offs_40_sz4_29;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_12_sz4_23);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_29);
		*dest = *src;
	}
// instr 33
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)))) = __bp_offs_12_sz4_23;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_28;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_29;
	if(__bp_offs_32_sz4_28.i32 > __bp_offs_40_sz4_29.i32)
		goto __func2_instr41;
// instr 34
// rdxsrc/List.rdx [line 43]
// instr 35
	RuntimeValue1 __bp_offs_32_sz1_30;
	__bp_offs_32_sz1_30.bytes[0] = 0;
// instr 36
	RuntimeValue4 __prv_offs0_sz4_31;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_12_sz4_23);
		dest = reinterpret_cast<RuntimeValue4 *>(&__prv_offs0_sz4_31);
		*dest = *src;
	}
// instr 37
	RuntimeValue4 __prv_offs8_sz4_32;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_27);
		dest = reinterpret_cast<RuntimeValue4 *>(&__prv_offs8_sz4_32);
		*dest = *src;
	}
// instr 38
	RuntimeValue1 __prv_offs16_sz1_33;
	{
		const RuntimeValue1 *src;
		RuntimeValue1 *dest;
		src = reinterpret_cast<const RuntimeValue1 *>(&__bp_offs_32_sz1_30);
		dest = reinterpret_cast<RuntimeValue1 *>(&__prv_offs16_sz1_33);
		*dest = *src;
	}
// instr 39
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(prv) + (0)))) = __prv_offs0_sz4_31;
	*reinterpret_cast<RuntimeValue1 *>((static_cast<void *>(static_cast< UInt8 *>(prv) + (16)))) = __prv_offs16_sz1_33;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(prv) + (8)))) = __prv_offs8_sz4_32;
}
{
	EXITFRAME;
// instr 40
// rdxsrc/List.rdx [line 45]
// instr 41
}
__func2_instr41:
{
	RuntimeValue4 __bp_offs_16_sz4_34;
	memset(&__bp_offs_16_sz4_34, 0, 4);
// instr 42
	RuntimeValue4 __bp_offs_12_sz4_35;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-12)));
		__bp_offs_12_sz4_35 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_12_sz4_35);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_16_sz4_34);
		*dest = *src;
	}
// instr 43
// rdxsrc/List.rdx [line 46]
// instr 44
	RuntimeValue4 __bp_offs_32_sz4_36;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_34);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_32_sz4_36);
		*dest = *src;
	}
// instr 45
	RuntimeValue4 __bp_offs_40_sz4_37;
	__bp_offs_40_sz4_37.addrs[0] = PackAddress(0,0,0,0);
// instr 46
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_34;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_36;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_37;
	if(__bp_offs_32_sz4_36.i32 != __bp_offs_40_sz4_37.i32)
		goto __func2_instr51;
// instr 47
// rdxsrc/List.rdx [line 47]
// instr 48
	RuntimeValue4 __bp_offs_32_sz4_38;
	__bp_offs_32_sz4_38.addrs[0] = PackAddress(8,0,0,0);
// instr 49
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_32_sz4_38);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_16_sz4_34);
		*dest = *src;
	}
// instr 50
// rdxsrc/List.rdx [line 49]
// instr 51
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_34;
}
__func2_instr51:
{
	RuntimeValue4 __bp_offs_20_sz4_39;
	memset(&__bp_offs_20_sz4_39, 0, 4);
// instr 52
	RuntimeValue4 __bp_offs_16_sz4_40;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_40 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_40);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_20_sz4_39);
		*dest = *src;
	}
// instr 53
// rdxsrc/List.rdx [line 50]
// instr 54
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-20)))) = __bp_offs_20_sz4_39;
}
__func2_instr54:
{
	RuntimeValue4 __bp_offs_16_sz4_41;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_41 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_32_sz4_42;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_41);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_32_sz4_42);
		*dest = *src;
	}
// instr 55
	RuntimeValue4 __bp_offs_8_sz4_43;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-8)));
		__bp_offs_8_sz4_43 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_40_sz4_44;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_43);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_44);
		*dest = *src;
	}
// instr 56
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_42;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_44;
	if(__bp_offs_32_sz4_42.i32 >= __bp_offs_40_sz4_44.i32)
		goto __func2_instr75;
// instr 57
// rdxsrc/List.rdx [line 52]
// instr 58
	RuntimeValue4 __bp_offs_40_sz4_45;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_41);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_45);
		*dest = *src;
	}
// instr 59
	RuntimeValue4 __bp_offs_48_sz4_46;
	__bp_offs_48_sz4_46.addrs[0] = PackAddress(2,0,0,0);
// instr 60
	RuntimeValue4 __bp_offs_32_sz4_47;
	__bp_offs_32_sz4_47.i32 = __bp_offs_40_sz4_45.i32 * __bp_offs_48_sz4_46.i32;
// instr 61
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_32_sz4_47);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_16_sz4_41);
		*dest = *src;
	}
// instr 62
// rdxsrc/List.rdx [line 53]
// instr 63
	RuntimeValue4 __bp_offs_32_sz4_48;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_41);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_32_sz4_48);
		*dest = *src;
	}
// instr 64
	RuntimeValue4 __bp_offs_20_sz4_49;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-20)));
		__bp_offs_20_sz4_49 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_40_sz4_50;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_20_sz4_49);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_50);
		*dest = *src;
	}
// instr 65
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_41;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_48;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_50;
	if(__bp_offs_32_sz4_48.i32 >= __bp_offs_40_sz4_50.i32)
		goto __func2_instr70;
// instr 66
// rdxsrc/List.rdx [line 54]
// instr 67
	RuntimeValue4 __bp_offs_32_sz4_51;
	__bp_offs_32_sz4_51.cp = resArgs[3];
// instr 68
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_51;
	THROWEXCEPTION(__bp_offs_32_sz4_51.p, 68);
// instr 69
// rdxsrc/List.rdx [line 55]
// instr 70
}
__func2_instr70:
{
	RuntimeValue4 __bp_offs_16_sz4_52;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_52 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_20_sz4_53;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_52);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_20_sz4_53);
		*dest = *src;
	}
// instr 71
// rdxsrc/List.rdx [line 50]
// instr 72
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-20)))) = __bp_offs_20_sz4_53;
}
{
	TICK(72);
// instr 73
}
__func2_instr73:
{
	goto __func2_instr54;
// instr 74
// rdxsrc/List.rdx [line 58]
// instr 75
}
__func2_instr75:
{
	RuntimeValue1 __bp_offs_32_sz1_54;
	__bp_offs_32_sz1_54.bytes[0] = 1;
// instr 76
	RuntimeValue4 __bp_offs_16_sz4_55;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_55 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __prv_offs0_sz4_56;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_55);
		dest = reinterpret_cast<RuntimeValue4 *>(&__prv_offs0_sz4_56);
		*dest = *src;
	}
// instr 77
	RuntimeValue4 __bp_offs_8_sz4_57;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-8)));
		__bp_offs_8_sz4_57 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __prv_offs8_sz4_58;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_57);
		dest = reinterpret_cast<RuntimeValue4 *>(&__prv_offs8_sz4_58);
		*dest = *src;
	}
// instr 78
	RuntimeValue1 __prv_offs16_sz1_59;
	{
		const RuntimeValue1 *src;
		RuntimeValue1 *dest;
		src = reinterpret_cast<const RuntimeValue1 *>(&__bp_offs_32_sz1_54);
		dest = reinterpret_cast<RuntimeValue1 *>(&__prv_offs16_sz1_59);
		*dest = *src;
	}
// instr 79
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(prv) + (0)))) = __prv_offs0_sz4_56;
	*reinterpret_cast<RuntimeValue1 *>((static_cast<void *>(static_cast< UInt8 *>(prv) + (16)))) = __prv_offs16_sz1_59;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(prv) + (8)))) = __prv_offs8_sz4_58;
}
{
	EXITFRAME;
// instr 80
// rdxsrc/List.rdx [line 36]
// instr 81
}
{
	INVALIDOPERATIONEXCEPTION(81);
}
__func2_start:
switch(startInstruction)
{
case 0: goto __func2_instr0;
case 23: goto __func2_instr23;
case 26: goto __func2_instr26;
case 41: goto __func2_instr41;
case 51: goto __func2_instr51;
case 54: goto __func2_instr54;
case 70: goto __func2_instr70;
case 73: goto __func2_instr73;
case 75: goto __func2_instr75;
};

return 0;

} // ************* END FUNC


	// ***** Core.string/methods/GetEnumerator()
static int f3(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func3_start;
__func3_instr0:
{
// instr 0
// rdxsrc/Internal.rdx [line 138]
// instr 1
	RuntimeValue4 __prv_offs_8_sz4_60;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-8)));
		__prv_offs_8_sz4_60 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_16_sz4_61;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_8_sz4_60);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_16_sz4_61);
		*dest = *src;
	}
// instr 2
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_61;
}
{
	TICK(2);
// instr 3
}
__func3_instr3:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[1]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 3, -16, -8, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 4
}
__func3_instr4:
{
	RuntimeValue4 __bp_offs_8_sz4_62;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-8)));
		__bp_offs_8_sz4_62 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __prv_offs0_sz4_63;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_62);
		dest = reinterpret_cast<RuntimeValue4 *>(&__prv_offs0_sz4_63);
		*dest = *src;
	}
// instr 5
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(prv) + (0)))) = __prv_offs0_sz4_63;
}
{
	EXITFRAME;
// instr 6
}
{
	INVALIDOPERATIONEXCEPTION(6);
}
__func3_start:
switch(startInstruction)
{
case 0: goto __func3_instr0;
case 3: goto __func3_instr3;
case 4: goto __func3_instr4;
};

return 0;

} // ************* END FUNC


	// ***** Core.Collections.HashSetBase/methods/Initialize(Core.Array)
static int f4(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func4_start;
__func4_instr0:
{
// instr 0
// rdxsrc/HashTable.rdx [line 70]
// instr 1
	RuntimeValue4 __prv_offs_8_sz4_64;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-8)));
		__prv_offs_8_sz4_64 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_8_sz4_65;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_8_sz4_64);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_65);
		*dest = *src;
	}
// instr 2
	RuntimeValue8 __bp_offs_8_sz8_66;
	__bp_offs_8_sz8_66.rtp.objectRef = __bp_offs_8_sz4_65.p;
	__bp_offs_8_sz8_66.rtp.valueRef = reinterpret_cast<UInt8 *>(__bp_offs_8_sz4_65.p) + (12);
// instr 3
	RuntimeValue4 __prv_offs_16_sz4_67;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-16)));
		__prv_offs_16_sz4_67 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_16_sz4_67);
		const RuntimePointer<void> *destPtr = &__bp_offs_8_sz8_66.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 4
// rdxsrc/HashTable.rdx [line 71]
// instr 5
	RuntimeValue4 __bp_offs_24_sz4_68;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_16_sz4_67);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_24_sz4_68);
		*dest = *src;
	}
// instr 6
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_68;
}
{
	TICK(6);
// instr 7
}
__func4_instr7:
{
	RuntimeValue4 __bp_offs_24_sz4_69;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-24)));
		__bp_offs_24_sz4_69 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_24_sz4_69.p, 7);
// instr 8
}
__func4_instr8:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[2]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 8, -32, -16, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 9
}
__func4_instr9:
{
	RuntimeValue4 __bp_offs_16_sz4_70;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_70 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_8_sz4_71;
	__bp_offs_8_sz4_71.i32 = static_cast<Int32>(		__bp_offs_16_sz4_70.i32);
// instr 10
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_71;
}
__func4_instr10:
{
	RuntimeValue4 __bp_offs_8_sz4_72;
	{
		t->frame.ip = instrTable + ((10) + 1);
		const Type *t = static_cast<const Type *>(resArgs[4]);
		void *obj = RDX_RuntimeUtilities_NewObjectInstance(ctx, objm, t, reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8 *>(bp) + (-8)), 1);
		NULLCHECK(obj, 10);
		__bp_offs_8_sz4_72.p = obj;
	}
// instr 11
	RuntimeValue4 __prv_offs_8_sz4_73;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-8)));
		__prv_offs_8_sz4_73 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_16_sz4_74;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_8_sz4_73);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_16_sz4_74);
		*dest = *src;
	}
// instr 12
	RuntimeValue8 __bp_offs_16_sz8_75;
	__bp_offs_16_sz8_75.rtp.objectRef = __bp_offs_16_sz4_74.p;
	__bp_offs_16_sz8_75.rtp.valueRef = reinterpret_cast<UInt8 *>(__bp_offs_16_sz4_74.p) + (8);
// instr 13
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_72);
		const RuntimePointer<void> *destPtr = &__bp_offs_16_sz8_75.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 14
// rdxsrc/HashTable.rdx [line 68]
// instr 15
}
{
	EXITFRAME;
// instr 16
}
{
	INVALIDOPERATIONEXCEPTION(16);
}
__func4_start:
switch(startInstruction)
{
case 0: goto __func4_instr0;
case 7: goto __func4_instr7;
case 8: goto __func4_instr8;
case 9: goto __func4_instr9;
case 10: goto __func4_instr10;
};

return 0;

} // ************* END FUNC


	// ***** Core.Exception/methods/description()
static int f5(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func5_start;
__func5_instr0:
{
// instr 0
// rdxsrc/Internal.rdx [line 164]
// instr 1
	RuntimeValue4 __bp_offs_8_sz4_76;
	__bp_offs_8_sz4_76.cp = resArgs[0];
// instr 2
	RuntimeValue4 __prv_offs0_sz4_77;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_76);
		dest = reinterpret_cast<RuntimeValue4 *>(&__prv_offs0_sz4_77);
		*dest = *src;
	}
// instr 3
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(prv) + (0)))) = __prv_offs0_sz4_77;
}
{
	EXITFRAME;
// instr 4
}
{
	INVALIDOPERATIONEXCEPTION(4);
}
__func5_start:
switch(startInstruction)
{
case 0: goto __func5_instr0;
};

return 0;

} // ************* END FUNC
// GIT format: Positive = -(value) to function encode, resume allowed
//             Negative = -(value) to function encode, resume forbidden
//             Zero     = Invalid, execution never stops or resumes at this point
static LargeInt globalInstructionTable[] = {
	// f1, Core.GenericException/methods/description()
	0, 1, 0, 0, 0, 0, 0, 
	// f2, Core.Collections.ListBase/methods/IncreaseSize(Core.int)
	0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 0, 0, 27, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 42, 0, 0, 0, 0, 0, 0, 0, 0, 0, 52, 0, 0, 55, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 71, 0, 0, 74, 0, 76, 0, 0, 0, 0, 0, 0, 
	// f3, Core.string/methods/GetEnumerator()
	0, 1, 0, 0, 4, 5, 0, 0, 
	// f4, Core.Collections.HashSetBase/methods/Initialize(Core.Array)
	0, 1, 0, 0, 0, 0, 0, 0, 8, 9, 10, 11, 0, 0, 0, 0, 0, 0, 
	// f5, Core.Exception/methods/description()
	0, 1, 0, 0, 0, 0, 
};
static StaticLookupTable<StaticLookupStringKey<char, char>, PrecompiledFunctionInfo>::Entry functionTable[] =
{
	{ "Core.GenericException/methods/description()", { 1 } },
	{ "Core.Collections.ListBase/methods/IncreaseSize(Core.int)", { 2 } },
	{ "Core.string/methods/GetEnumerator()", { 3 } },
	{ "Core.Collections.HashSetBase/methods/Initialize(Core.Array)", { 4 } },
	{ "Core.Exception/methods/description()", { 5 } },
};
static void InitializeCompiledRDX()
{
	globalInstructionTable[0] = reinterpret_cast<const char *>(f1) - reinterpret_cast<const char *>(NULL);
	functionTable[0].value.compiledInstructions = globalInstructionTable + 1;
	globalInstructionTable[7] = reinterpret_cast<const char *>(f2) - reinterpret_cast<const char *>(NULL);
	functionTable[1].value.compiledInstructions = globalInstructionTable + 8;
	globalInstructionTable[90] = reinterpret_cast<const char *>(f3) - reinterpret_cast<const char *>(NULL);
	functionTable[2].value.compiledInstructions = globalInstructionTable + 91;
	globalInstructionTable[98] = reinterpret_cast<const char *>(f4) - reinterpret_cast<const char *>(NULL);
	functionTable[3].value.compiledInstructions = globalInstructionTable + 99;
	globalInstructionTable[116] = reinterpret_cast<const char *>(f5) - reinterpret_cast<const char *>(NULL);
	functionTable[4].value.compiledInstructions = globalInstructionTable + 117;
}
static AutoRunFunction compileInitializer(InitializeCompiledRDX);
static RDX::StaticLookupTable<RDX::StaticLookupStringKey<char, char>, PrecompiledFunctionInfo> functionLookup(functionTable, sizeof(functionTable)/sizeof(functionTable[0]));
namespace RDX
{
	namespace PCCM
	{
		PrecompiledCodeModule Core(globalInstructionTable, sizeof(globalInstructionTable), &functionLookup);
	}
}
