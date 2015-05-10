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
#include "regressiontest_pccm.hpp"
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


	// ***** Apps.RegressionTest.RegressionTest.TestUsing_Struct/methods/Initialize(Core.string)
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
// rdxsrc/Apps/RegressionTest.rdx [line 306]
// instr 1
	RuntimeValue8 __prv_offs_8_sz8_1;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-8)));
		__prv_offs_8_sz8_1 = *reinterpret_cast<const RuntimeValue8 *>(src);
	}
	RuntimeValue8 __bp_offs_8_sz8_2;
	{
		const RuntimeValue8 *src;
		RuntimeValue8 *dest;
		src = reinterpret_cast<const RuntimeValue8 *>(&__prv_offs_8_sz8_1);
		dest = reinterpret_cast<RuntimeValue8 *>(&__bp_offs_8_sz8_2);
		*dest = *src;
	}
// instr 2
	__bp_offs_8_sz8_2.rtp.valueRef = reinterpret_cast<UInt8 *>(__bp_offs_8_sz8_2.rtp.valueRef) + (0);
// instr 3
	RuntimeValue4 __prv_offs_16_sz4_3;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-16)));
		__prv_offs_16_sz4_3 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_16_sz4_3);
		const RuntimePointer<void> *destPtr = &__bp_offs_8_sz8_2.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 4
// rdxsrc/Apps/RegressionTest.rdx [line 304]
// instr 5
}
{
	EXITFRAME;
// instr 6
}
{
	INVALIDOPERATIONEXCEPTION(6);
}
__func1_start:
switch(startInstruction)
{
case 0: goto __func1_instr0;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestThreadDeserializationPD2()
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
// rdxsrc/Apps/RegressionTest.rdx [line 285]
// instr 1
	RuntimeValue4 __bp_offs_4_sz4_4;
	memset(&__bp_offs_4_sz4_4, 0, 4);
// instr 2
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_4;
}
__func2_instr2:
{
	RuntimeValue4 __bp_offs_16_sz4_5;
	{
		t->frame.ip = instrTable + ((2) + 1);
		const Type *t = static_cast<const Type *>(resArgs[0]);
		void *obj = RDX_RuntimeUtilities_NewObjectInstance(ctx, objm, t, reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8 *>(bp) + (0)), 0);
		NULLCHECK(obj, 2);
		__bp_offs_16_sz4_5.p = obj;
	}
// instr 3
	RuntimeValue4 __bp_offs_4_sz4_6;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_5);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_4_sz4_6);
		*dest = *src;
	}
// instr 4
// rdxsrc/Apps/RegressionTest.rdx [line 286]
// instr 5
	RuntimeValue4 __bp_offs_16_sz4_7;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_6);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_16_sz4_7);
		*dest = *src;
	}
// instr 6
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_7;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_6;
}
{
	TICK(6);
// instr 7
}
__func2_instr7:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[1]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 7, -16, -8, thread, instrTable, bp, providerDictionary);
	}
// instr 8
}
__func2_instr8:
{
// rdxsrc/Apps/RegressionTest.rdx [line 285]
// instr 9
}
__func2_instr9:
{
	EXITFRAME;
// instr 10
}
{
	INVALIDOPERATIONEXCEPTION(10);
}
__func2_start:
switch(startInstruction)
{
case 0: goto __func2_instr0;
case 2: goto __func2_instr2;
case 7: goto __func2_instr7;
case 8: goto __func2_instr8;
case 9: goto __func2_instr9;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestExceptions()
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
// rdxsrc/Apps/RegressionTest.rdx [line 143]
// instr 1
}
{
	TICK(1);
// instr 2
}
__func3_instr2:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[0]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 2, 0, -8, thread, instrTable, bp, providerDictionary);
	}
// instr 3
}
__func3_instr3:
{
// rdxsrc/Apps/RegressionTest.rdx [line 144]
// instr 4
}
__func3_instr4:
{
	TICK(4);
// instr 5
}
__func3_instr5:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[0]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 5, 0, -8, thread, instrTable, bp, providerDictionary);
	}
// instr 6
}
__func3_instr6:
{
// rdxsrc/Apps/RegressionTest.rdx [line 146]
// instr 7
}
__func3_instr7:
{
	TICK(7);
// instr 8
}
__func3_instr8:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[1]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 8, 0, -8, thread, instrTable, bp, providerDictionary);
	}
// instr 9
}
__func3_instr9:
{
// rdxsrc/Apps/RegressionTest.rdx [line 146]
// instr 10
}
__func3_instr10:
{
	goto __func3_instr32;
// instr 11
}
{
	INVALIDOPERATIONEXCEPTION(11);
// instr 12
}
__func3_instr12:
{
	RuntimeValue4 __bp_offs_16_sz4_8;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_8 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_4_sz4_9;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_8);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_4_sz4_9);
		*dest = *src;
	}
// instr 13
// rdxsrc/Apps/RegressionTest.rdx [line 152]
// instr 14
	RuntimeValue4 __bp_offs_16_sz4_10;
	__bp_offs_16_sz4_10.cp = resArgs[3];
// instr 15
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_10;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_9;
}
{
	TICK(15);
// instr 16
}
__func3_instr16:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[5]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 16, -16, -8, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 17
}
__func3_instr17:
{
// rdxsrc/Apps/RegressionTest.rdx [line 146]
// instr 18
}
__func3_instr18:
{
	goto __func3_instr32;
// instr 19
}
{
	INVALIDOPERATIONEXCEPTION(19);
// instr 20
}
__func3_instr20:
{
	RuntimeValue4 __bp_offs_16_sz4_11;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_11 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_4_sz4_12;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_11);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_4_sz4_12);
		*dest = *src;
	}
// instr 21
// rdxsrc/Apps/RegressionTest.rdx [line 156]
// instr 22
	RuntimeValue4 __bp_offs_16_sz4_13;
	__bp_offs_16_sz4_13.cp = resArgs[7];
// instr 23
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_13;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_12;
}
{
	TICK(23);
// instr 24
}
__func3_instr24:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[5]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 24, -16, -8, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 25
}
__func3_instr25:
{
// rdxsrc/Apps/RegressionTest.rdx [line 146]
// instr 26
}
__func3_instr26:
{
	goto __func3_instr32;
// instr 27
}
{
	INVALIDOPERATIONEXCEPTION(27);
// instr 28
}
__func3_instr28:
{
	RuntimeValue4 __bp_offs_16_sz4_14;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_14 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_16_sz4_14.p, 28);
	if(objm->TypesCompatible(GCInfo::From(__bp_offs_16_sz4_14.p)->containerType, static_cast<Type *>(resArgs[2])))
		goto __func3_instr12;
// instr 29
}
{
	goto __func3_instr20;
// instr 30
	RuntimeValue4 __bp_offs_16_sz4_15;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_15 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	THROWEXCEPTION(__bp_offs_16_sz4_15.p, 30);
// instr 31
// rdxsrc/Apps/RegressionTest.rdx [line 158]
// instr 32
}
__func3_instr32:
{
	RuntimeValue4 __bp_offs_16_sz4_16;
	__bp_offs_16_sz4_16.addrs[0] = PackAddress(0,0,0,0);
// instr 33
	RuntimeValue4 __prv_offs0_sz4_17;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_16);
		dest = reinterpret_cast<RuntimeValue4 *>(&__prv_offs0_sz4_17);
		*dest = *src;
	}
// instr 34
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(prv) + (0)))) = __prv_offs0_sz4_17;
}
{
	EXITFRAME;
// instr 35
}
{
	INVALIDOPERATIONEXCEPTION(35);
}
__func3_start:
switch(startInstruction)
{
case 0: goto __func3_instr0;
case 2: goto __func3_instr2;
case 3: goto __func3_instr3;
case 4: goto __func3_instr4;
case 5: goto __func3_instr5;
case 6: goto __func3_instr6;
case 7: goto __func3_instr7;
case 8: goto __func3_instr8;
case 9: goto __func3_instr9;
case 10: goto __func3_instr10;
case 12: goto __func3_instr12;
case 16: goto __func3_instr16;
case 17: goto __func3_instr17;
case 18: goto __func3_instr18;
case 20: goto __func3_instr20;
case 24: goto __func3_instr24;
case 25: goto __func3_instr25;
case 26: goto __func3_instr26;
case 28: goto __func3_instr28;
case 32: goto __func3_instr32;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest.TestUsing_Struct/methods/Dispose()
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
// rdxsrc/Apps/RegressionTest.rdx [line 311]
// instr 1
	RuntimeValue4 __bp_offs_16_sz4_18;
	__bp_offs_16_sz4_18.cp = resArgs[1];
// instr 2
	RuntimeValue8 __prv_offs_8_sz8_19;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-8)));
		__prv_offs_8_sz8_19 = *reinterpret_cast<const RuntimeValue8 *>(src);
	}
	RuntimeValue8 __bp_offs_24_sz8_20;
	{
		const RuntimeValue8 *src;
		RuntimeValue8 *dest;
		src = reinterpret_cast<const RuntimeValue8 *>(&__prv_offs_8_sz8_19);
		dest = reinterpret_cast<RuntimeValue8 *>(&__bp_offs_24_sz8_20);
		*dest = *src;
	}
// instr 3
	__bp_offs_24_sz8_20.rtp.valueRef = reinterpret_cast<UInt8 *>(__bp_offs_24_sz8_20.rtp.valueRef) + (0);
// instr 4
	RuntimeValue4 __bp_offs_24_sz4_21;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		const RuntimePointer<void> *srcPtr = &__bp_offs_24_sz8_20.rtp;
		src = static_cast<const RuntimeValue4 *>(srcPtr->valueRef);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_24_sz4_21);
		*dest = *src;
	}
// instr 5
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_18;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_21;
}
{
	TICK(5);
// instr 6
}
__func4_instr6:
{
	RuntimeValue4 __bp_offs_16_sz4_22;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_22 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_16_sz4_22.p, 6);
// instr 7
}
__func4_instr7:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[2]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 7, -32, -8, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 8
}
__func4_instr8:
{
	TICK(8);
// instr 9
}
__func4_instr9:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[3]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 9, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 10
}
__func4_instr10:
{
// rdxsrc/Apps/RegressionTest.rdx [line 309]
// instr 11
}
__func4_instr11:
{
	EXITFRAME;
// instr 12
}
{
	INVALIDOPERATIONEXCEPTION(12);
}
__func4_start:
switch(startInstruction)
{
case 0: goto __func4_instr0;
case 6: goto __func4_instr6;
case 7: goto __func4_instr7;
case 8: goto __func4_instr8;
case 9: goto __func4_instr9;
case 10: goto __func4_instr10;
case 11: goto __func4_instr11;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestLargeParametersCall(Core.int,Apps.RegressionTest.RegressionTest.FatStruct,Core.int)
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
// rdxsrc/Apps/RegressionTest.rdx [line 243]
// instr 1
	RuntimeValue4 __bp_offs_8_sz4_23;
	__bp_offs_8_sz4_23.cp = resArgs[0];
// instr 2
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_23;
}
{
	TICK(2);
// instr 3
}
__func5_instr3:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[2]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 3, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 4
}
__func5_instr4:
{
// rdxsrc/Apps/RegressionTest.rdx [line 244]
// instr 5
}
__func5_instr5:
{
	RuntimeValue4 __prv_offs_8_sz4_24;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-8)));
		__prv_offs_8_sz4_24 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_8_sz4_25;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_8_sz4_24);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_25);
		*dest = *src;
	}
// instr 6
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_25;
}
{
	TICK(6);
// instr 7
}
__func5_instr7:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[3]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 7, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 8
}
__func5_instr8:
{
// rdxsrc/Apps/RegressionTest.rdx [line 245]
// instr 9
}
__func5_instr9:
{
	RuntimeValue8 __bp_offs_8_sz8_26;
	__bp_offs_8_sz8_26.rtp.objectRef = &thread;
	__bp_offs_8_sz8_26.rtp.valueRef = (static_cast<void *>(static_cast< UInt8 *>(prv) + (-24)));
// instr 10
	RuntimeValue4 __bp_offs_8_sz4_27;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		const RuntimePointer<void> *srcPtr = &__bp_offs_8_sz8_26.rtp;
		src = static_cast<const RuntimeValue4 *>(srcPtr->valueRef);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_27);
		*dest = *src;
	}
// instr 11
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_27;
}
{
	TICK(11);
// instr 12
}
__func5_instr12:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[3]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 12, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 13
}
__func5_instr13:
{
// rdxsrc/Apps/RegressionTest.rdx [line 246]
// instr 14
}
__func5_instr14:
{
	RuntimeValue8 __bp_offs_8_sz8_28;
	__bp_offs_8_sz8_28.rtp.objectRef = &thread;
	__bp_offs_8_sz8_28.rtp.valueRef = (static_cast<void *>(static_cast< UInt8 *>(prv) + (-20)));
// instr 15
	RuntimeValue4 __bp_offs_8_sz4_29;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		const RuntimePointer<void> *srcPtr = &__bp_offs_8_sz8_28.rtp;
		src = static_cast<const RuntimeValue4 *>(srcPtr->valueRef);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_29);
		*dest = *src;
	}
// instr 16
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_29;
}
{
	TICK(16);
// instr 17
}
__func5_instr17:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[3]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 17, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 18
}
__func5_instr18:
{
// rdxsrc/Apps/RegressionTest.rdx [line 247]
// instr 19
}
__func5_instr19:
{
	RuntimeValue8 __bp_offs_8_sz8_30;
	__bp_offs_8_sz8_30.rtp.objectRef = &thread;
	__bp_offs_8_sz8_30.rtp.valueRef = (static_cast<void *>(static_cast< UInt8 *>(prv) + (-16)));
// instr 20
	RuntimeValue4 __bp_offs_8_sz4_31;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		const RuntimePointer<void> *srcPtr = &__bp_offs_8_sz8_30.rtp;
		src = static_cast<const RuntimeValue4 *>(srcPtr->valueRef);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_31);
		*dest = *src;
	}
// instr 21
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_31;
}
{
	TICK(21);
// instr 22
}
__func5_instr22:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[3]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 22, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 23
}
__func5_instr23:
{
// rdxsrc/Apps/RegressionTest.rdx [line 248]
// instr 24
}
__func5_instr24:
{
	RuntimeValue8 __bp_offs_8_sz8_32;
	__bp_offs_8_sz8_32.rtp.objectRef = &thread;
	__bp_offs_8_sz8_32.rtp.valueRef = (static_cast<void *>(static_cast< UInt8 *>(prv) + (-12)));
// instr 25
	RuntimeValue4 __bp_offs_8_sz4_33;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		const RuntimePointer<void> *srcPtr = &__bp_offs_8_sz8_32.rtp;
		src = static_cast<const RuntimeValue4 *>(srcPtr->valueRef);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_33);
		*dest = *src;
	}
// instr 26
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_33;
}
{
	TICK(26);
// instr 27
}
__func5_instr27:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[3]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 27, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 28
}
__func5_instr28:
{
// rdxsrc/Apps/RegressionTest.rdx [line 249]
// instr 29
}
__func5_instr29:
{
	RuntimeValue4 __prv_offs_32_sz4_34;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-32)));
		__prv_offs_32_sz4_34 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_8_sz4_35;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_32_sz4_34);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_35);
		*dest = *src;
	}
// instr 30
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_35;
}
{
	TICK(30);
// instr 31
}
__func5_instr31:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[3]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 31, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 32
}
__func5_instr32:
{
// rdxsrc/Apps/RegressionTest.rdx [line 241]
// instr 33
}
__func5_instr33:
{
	EXITFRAME;
// instr 34
}
{
	INVALIDOPERATIONEXCEPTION(34);
}
__func5_start:
switch(startInstruction)
{
case 0: goto __func5_instr0;
case 3: goto __func5_instr3;
case 4: goto __func5_instr4;
case 5: goto __func5_instr5;
case 7: goto __func5_instr7;
case 8: goto __func5_instr8;
case 9: goto __func5_instr9;
case 12: goto __func5_instr12;
case 13: goto __func5_instr13;
case 14: goto __func5_instr14;
case 17: goto __func5_instr17;
case 18: goto __func5_instr18;
case 19: goto __func5_instr19;
case 22: goto __func5_instr22;
case 23: goto __func5_instr23;
case 24: goto __func5_instr24;
case 27: goto __func5_instr27;
case 28: goto __func5_instr28;
case 29: goto __func5_instr29;
case 31: goto __func5_instr31;
case 32: goto __func5_instr32;
case 33: goto __func5_instr33;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest.InterfaceImplementingClass/methods/Func4()
static int f6(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func6_start;
__func6_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 77]
// instr 1
	RuntimeValue4 __bp_offs_8_sz4_36;
	__bp_offs_8_sz4_36.cp = resArgs[0];
// instr 2
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_36;
}
{
	TICK(2);
// instr 3
}
__func6_instr3:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[2]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 3, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 4
}
__func6_instr4:
{
	EXITFRAME;
// instr 5
}
{
	INVALIDOPERATIONEXCEPTION(5);
}
__func6_start:
switch(startInstruction)
{
case 0: goto __func6_instr0;
case 3: goto __func6_instr3;
case 4: goto __func6_instr4;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest.InterfaceImplementingClass/methods/Func3()
static int f7(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func7_start;
__func7_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 76]
// instr 1
}
{
	EXITFRAME;
// instr 2
}
{
	INVALIDOPERATIONEXCEPTION(2);
}
__func7_start:
switch(startInstruction)
{
case 0: goto __func7_instr0;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest.DelegateTestStruct/methods/PrintContents(Core.string)
static int f8(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func8_start;
__func8_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 24]
// instr 1
	RuntimeValue8 __prv_offs_8_sz8_37;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-8)));
		__prv_offs_8_sz8_37 = *reinterpret_cast<const RuntimeValue8 *>(src);
	}
	RuntimeValue8 __bp_offs_8_sz8_38;
	{
		const RuntimeValue8 *src;
		RuntimeValue8 *dest;
		src = reinterpret_cast<const RuntimeValue8 *>(&__prv_offs_8_sz8_37);
		dest = reinterpret_cast<RuntimeValue8 *>(&__bp_offs_8_sz8_38);
		*dest = *src;
	}
// instr 2
	__bp_offs_8_sz8_38.rtp.valueRef = reinterpret_cast<UInt8 *>(__bp_offs_8_sz8_38.rtp.valueRef) + (0);
// instr 3
	RuntimeValue4 __bp_offs_8_sz4_39;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		const RuntimePointer<void> *srcPtr = &__bp_offs_8_sz8_38.rtp;
		src = static_cast<const RuntimeValue4 *>(srcPtr->valueRef);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_39);
		*dest = *src;
	}
// instr 4
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_39;
}
{
	TICK(4);
// instr 5
}
__func8_instr5:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[0]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 5, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 6
}
__func8_instr6:
{
// rdxsrc/Apps/RegressionTest.rdx [line 22]
// instr 7
}
__func8_instr7:
{
	EXITFRAME;
// instr 8
}
{
	INVALIDOPERATIONEXCEPTION(8);
}
__func8_start:
switch(startInstruction)
{
case 0: goto __func8_instr0;
case 5: goto __func8_instr5;
case 6: goto __func8_instr6;
case 7: goto __func8_instr7;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestRValueDelegate(Apps.RegressionTest.RegressionTest.DelegateTestInterface)
static int f9(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func9_start;
__func9_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 206]
// instr 1
	RuntimeValue4 __bp_offs_4_sz4_40;
	memset(&__bp_offs_4_sz4_40, 0, 4);
// instr 2
	RuntimeValue4 __prv_offs_8_sz4_41;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-8)));
		__prv_offs_8_sz4_41 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_16_sz4_42;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_8_sz4_41);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_16_sz4_42);
		*dest = *src;
	}
// instr 3
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_42;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_40;
}
__func9_instr3:
{
	RuntimeValue4 __bp_offs_4_sz4_43;
	{
		t->frame.ip = instrTable + ((3) + 1);
		const Type *t = static_cast<const Type *>(resArgs[1]);
		void *obj = RDX_RuntimeUtilities_NewObjectInstance(ctx, objm, t, reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8 *>(bp) + (0)), 0);
		NULLCHECK(obj, 3);
		__bp_offs_4_sz4_43.p = obj;
	}
// instr 4
	RuntimeValue8 __bp_offs_24_sz8_44;
	__bp_offs_24_sz8_44.rtp.objectRef = __bp_offs_4_sz4_43.p;
	__bp_offs_24_sz8_44.rtp.valueRef = reinterpret_cast<UInt8 *>(__bp_offs_4_sz4_43.p) + (0);
// instr 5
	RuntimeValue4 __bp_offs_16_sz4_45;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_45 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_45);
		const RuntimePointer<void> *destPtr = &__bp_offs_24_sz8_44.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 6
	RuntimeValue4 __prv_offs0_sz4_46;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_43);
		dest = reinterpret_cast<RuntimeValue4 *>(&__prv_offs0_sz4_46);
		*dest = *src;
	}
// instr 7
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_43;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(prv) + (0)))) = __prv_offs0_sz4_46;
}
{
	EXITFRAME;
// instr 8
}
{
	INVALIDOPERATIONEXCEPTION(8);
}
__func9_start:
switch(startInstruction)
{
case 0: goto __func9_instr0;
case 3: goto __func9_instr3;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestFinally()
static int f10(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func10_start;
__func10_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 317]
// instr 1
	RuntimeValue4 __bp_offs_4_sz4_47;
	memset(&__bp_offs_4_sz4_47, 0, 4);
// instr 2
	RuntimeValue4 __bp_offs_24_sz4_48;
	__bp_offs_24_sz4_48.addrs[0] = PackAddress(0,0,0,0);
// instr 3
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_24_sz4_48);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_4_sz4_47);
		*dest = *src;
	}
// instr 4
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_47;
}
{
	goto __func10_instr9;
// instr 5
}
__func10_instr5:
{
	RuntimeValue4 __bp_offs_4_sz4_49;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-4)));
		__bp_offs_4_sz4_49 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_32_sz4_50;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_49);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_32_sz4_50);
		*dest = *src;
	}
// instr 6
	RuntimeValue4 __bp_offs_40_sz4_51;
	__bp_offs_40_sz4_51.addrs[0] = PackAddress(1,0,0,0);
// instr 7
	RuntimeValue4 __bp_offs_24_sz4_52;
	__bp_offs_24_sz4_52.i32 = __bp_offs_32_sz4_50.i32 + __bp_offs_40_sz4_51.i32;
// instr 8
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_24_sz4_52);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_4_sz4_49);
		*dest = *src;
	}
// instr 9
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_49;
}
__func10_instr9:
{
	RuntimeValue4 __bp_offs_4_sz4_53;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-4)));
		__bp_offs_4_sz4_53 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_24_sz4_54;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_53);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_24_sz4_54);
		*dest = *src;
	}
// instr 10
	RuntimeValue4 __bp_offs_32_sz4_55;
	__bp_offs_32_sz4_55.addrs[0] = PackAddress(10,0,0,0);
// instr 11
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_54;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_55;
	if(__bp_offs_24_sz4_54.i32 >= __bp_offs_32_sz4_55.i32)
		goto __func10_instr65;
// instr 12
// rdxsrc/Apps/RegressionTest.rdx [line 319]
// instr 13
	RuntimeValue4 __bp_offs_8_sz4_56;
	memset(&__bp_offs_8_sz4_56, 0, 4);
// instr 14
	RuntimeValue4 __bp_offs_12_sz4_57;
	memset(&__bp_offs_12_sz4_57, 0, 4);
// instr 15
	RuntimeValue4 __bp_offs_16_sz4_58;
	memset(&__bp_offs_16_sz4_58, 0, 4);
// instr 16
// rdxsrc/Apps/RegressionTest.rdx [line 321]
// instr 17
	RuntimeValue4 __bp_offs_24_sz4_59;
	__bp_offs_24_sz4_59.cp = resArgs[4];
// instr 18
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)))) = __bp_offs_12_sz4_57;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_58;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_59;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_56;
}
{
	TICK(18);
// instr 19
}
__func10_instr19:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[6]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 19, -32, -16, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 20
}
__func10_instr20:
{
// rdxsrc/Apps/RegressionTest.rdx [line 322]
// instr 21
}
__func10_instr21:
{
	RuntimeValue4 __bp_offs_4_sz4_60;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-4)));
		__bp_offs_4_sz4_60 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_24_sz4_61;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_60);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_24_sz4_61);
		*dest = *src;
	}
// instr 22
	RuntimeValue4 __bp_offs_32_sz4_62;
	__bp_offs_32_sz4_62.addrs[0] = PackAddress(4,0,0,0);
// instr 23
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_61;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_62;
	if(__bp_offs_24_sz4_61.i32 == __bp_offs_32_sz4_62.i32)
		goto __func10_instr33;
// instr 24
// rdxsrc/Apps/RegressionTest.rdx [line 319]
// instr 25
	RuntimeValue4 __bp_offs_24_sz4_63;
	__bp_offs_24_sz4_63.addrs[0] = PackAddress(0,0,0,0);
// instr 26
	RuntimeValue4 __bp_offs_12_sz4_64;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_24_sz4_63);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_12_sz4_64);
		*dest = *src;
	}
// instr 27
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)))) = __bp_offs_12_sz4_64;
}
{
	goto __func10_instr36;
// instr 28
}
{
	INVALIDOPERATIONEXCEPTION(28);
// instr 29
}
__func10_instr29:
{
	RuntimeValue4 __bp_offs_24_sz4_65;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-24)));
		__bp_offs_24_sz4_65 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_8_sz4_66;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_24_sz4_65);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_66);
		*dest = *src;
	}
// instr 30
	RuntimeValue4 __bp_offs_24_sz4_67;
	__bp_offs_24_sz4_67.addrs[0] = PackAddress(255,255,255,255);
// instr 31
	RuntimeValue4 __bp_offs_12_sz4_68;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_24_sz4_67);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_12_sz4_68);
		*dest = *src;
	}
// instr 32
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)))) = __bp_offs_12_sz4_68;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_66;
}
{
	goto __func10_instr36;
// instr 33
}
__func10_instr33:
{
	RuntimeValue4 __bp_offs_24_sz4_69;
	__bp_offs_24_sz4_69.addrs[0] = PackAddress(3,0,0,0);
// instr 34
	RuntimeValue4 __bp_offs_12_sz4_70;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_24_sz4_69);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_12_sz4_70);
		*dest = *src;
	}
// instr 35
// rdxsrc/Apps/RegressionTest.rdx [line 327]
// instr 36
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)))) = __bp_offs_12_sz4_70;
}
__func10_instr36:
{
	RuntimeValue4 __bp_offs_24_sz4_71;
	__bp_offs_24_sz4_71.cp = resArgs[8];
// instr 37
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_71;
}
{
	TICK(37);
// instr 38
}
__func10_instr38:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[6]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 38, -32, -16, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 39
}
__func10_instr39:
{
// rdxsrc/Apps/RegressionTest.rdx [line 319]
// instr 40
}
__func10_instr40:
{
	RuntimeValue4 __bp_offs_12_sz4_72;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-12)));
		__bp_offs_12_sz4_72 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_24_sz4_73;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_12_sz4_72);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_24_sz4_73);
		*dest = *src;
	}
// instr 41
	RuntimeValue4 __bp_offs_32_sz4_74;
	__bp_offs_32_sz4_74.addrs[0] = PackAddress(255,255,255,255);
// instr 42
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_73;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_74;
}
{
	if(memcmp(static_cast<const UInt8 *>(bp) + (-32), static_cast<const UInt8 *>(bp) + (-24), 4))
		goto __func10_instr45;
// instr 43
	RuntimeValue4 __bp_offs_8_sz4_75;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-8)));
		__bp_offs_8_sz4_75 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_24_sz4_76;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_75);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_24_sz4_76);
		*dest = *src;
	}
// instr 44
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_76;
	THROWEXCEPTION(__bp_offs_24_sz4_76.p, 44);
// instr 45
}
__func10_instr45:
{
	goto __func10_instr51;
// instr 46
	RuntimeValue4 __bp_offs_12_sz4_77;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-12)));
		__bp_offs_12_sz4_77 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_24_sz4_78;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_12_sz4_77);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_24_sz4_78);
		*dest = *src;
	}
// instr 47
	RuntimeValue4 __bp_offs_32_sz4_79;
	__bp_offs_32_sz4_79.addrs[0] = PackAddress(1,0,0,0);
// instr 48
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_78;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_79;
}
{
	if(memcmp(static_cast<const UInt8 *>(bp) + (-32), static_cast<const UInt8 *>(bp) + (-24), 4))
		goto __func10_instr51;
// instr 49
	RuntimeValue4 __bp_offs_16_sz4_80;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_80 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __prv_offs0_sz4_81;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_80);
		dest = reinterpret_cast<RuntimeValue4 *>(&__prv_offs0_sz4_81);
		*dest = *src;
	}
// instr 50
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(prv) + (0)))) = __prv_offs0_sz4_81;
}
{
	EXITFRAME;
// instr 51
}
__func10_instr51:
{
	RuntimeValue4 __bp_offs_12_sz4_82;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-12)));
		__bp_offs_12_sz4_82 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_24_sz4_83;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_12_sz4_82);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_24_sz4_83);
		*dest = *src;
	}
// instr 52
	RuntimeValue4 __bp_offs_32_sz4_84;
	__bp_offs_32_sz4_84.addrs[0] = PackAddress(2,0,0,0);
// instr 53
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_83;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_84;
}
{
	if(!memcmp(static_cast<const UInt8 *>(bp) + (-32), static_cast<const UInt8 *>(bp) + (-24), 4))
		goto __func10_instr58;
// instr 54
	RuntimeValue4 __bp_offs_12_sz4_85;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-12)));
		__bp_offs_12_sz4_85 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_24_sz4_86;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_12_sz4_85);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_24_sz4_86);
		*dest = *src;
	}
// instr 55
	RuntimeValue4 __bp_offs_32_sz4_87;
	__bp_offs_32_sz4_87.addrs[0] = PackAddress(3,0,0,0);
// instr 56
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_86;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_87;
}
{
	if(!memcmp(static_cast<const UInt8 *>(bp) + (-32), static_cast<const UInt8 *>(bp) + (-24), 4))
		goto __func10_instr65;
// instr 57
// rdxsrc/Apps/RegressionTest.rdx [line 329]
// instr 58
}
__func10_instr58:
{
	RuntimeValue4 __bp_offs_24_sz4_88;
	__bp_offs_24_sz4_88.cp = resArgs[9];
// instr 59
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_88;
}
{
	TICK(59);
// instr 60
}
__func10_instr60:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[6]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 60, -32, -16, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 61
}
__func10_instr61:
{
// rdxsrc/Apps/RegressionTest.rdx [line 317]
// instr 62
}
__func10_instr62:
{
	TICK(62);
// instr 63
}
__func10_instr63:
{
	goto __func10_instr5;
// instr 64
// rdxsrc/Apps/RegressionTest.rdx [line 332]
// instr 65
}
__func10_instr65:
{
	RuntimeValue4 __bp_offs_24_sz4_89;
	__bp_offs_24_sz4_89.addrs[0] = PackAddress(10,0,0,0);
// instr 66
	RuntimeValue4 __prv_offs0_sz4_90;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_24_sz4_89);
		dest = reinterpret_cast<RuntimeValue4 *>(&__prv_offs0_sz4_90);
		*dest = *src;
	}
// instr 67
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(prv) + (0)))) = __prv_offs0_sz4_90;
}
{
	EXITFRAME;
// instr 68
}
{
	INVALIDOPERATIONEXCEPTION(68);
}
__func10_start:
switch(startInstruction)
{
case 0: goto __func10_instr0;
case 5: goto __func10_instr5;
case 9: goto __func10_instr9;
case 19: goto __func10_instr19;
case 20: goto __func10_instr20;
case 21: goto __func10_instr21;
case 29: goto __func10_instr29;
case 33: goto __func10_instr33;
case 36: goto __func10_instr36;
case 38: goto __func10_instr38;
case 39: goto __func10_instr39;
case 40: goto __func10_instr40;
case 45: goto __func10_instr45;
case 51: goto __func10_instr51;
case 58: goto __func10_instr58;
case 60: goto __func10_instr60;
case 61: goto __func10_instr61;
case 62: goto __func10_instr62;
case 63: goto __func10_instr63;
case 65: goto __func10_instr65;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestThreadDeserializationDemote(Apps.RegressionTest.RegressionTest.DemotionTest)
static int f11(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func11_start;
__func11_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 276]
// instr 1
}
__func11_instr1:
{
	RuntimeValue4 __bp_offs_16_sz4_91;
	{
		t->frame.ip = instrTable + ((1) + 1);
		const Type *t = static_cast<const Type *>(resArgs[0]);
		void *obj = RDX_RuntimeUtilities_NewObjectInstance(ctx, objm, t, reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8 *>(bp) + (0)), 0);
		NULLCHECK(obj, 1);
		__bp_offs_16_sz4_91.p = obj;
	}
// instr 2
	RuntimeValue4 __prv_offs_8_sz4_92;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_91);
		dest = reinterpret_cast<RuntimeValue4 *>(&__prv_offs_8_sz4_92);
		*dest = *src;
	}
// instr 3
// rdxsrc/Apps/RegressionTest.rdx [line 277]
// instr 4
	RuntimeValue4 __bp_offs_4_sz4_93;
	memset(&__bp_offs_4_sz4_93, 0, 4);
// instr 5
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_93;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(prv) + (-8)))) = __prv_offs_8_sz4_92;
}
{
	TICK(5);
// instr 6
}
__func11_instr6:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[2]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 6, -16, -16, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 7
}
__func11_instr7:
{
	RuntimeValue4 __bp_offs_16_sz4_94;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_94 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_4_sz4_95;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_94);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_4_sz4_95);
		*dest = *src;
	}
// instr 8
// rdxsrc/Apps/RegressionTest.rdx [line 279]
// instr 9
	RuntimeValue4 __bp_offs_16_sz4_96;
	__bp_offs_16_sz4_96.cp = resArgs[3];
// instr 10
	RuntimeValue4 __bp_offs_24_sz4_97;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_95);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_24_sz4_97);
		*dest = *src;
	}
// instr 11
	RuntimeValue1 __bp_offs_32_sz1_98;
	__bp_offs_32_sz1_98.bytes[0] = 0;
// instr 12
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_96;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_97;
	*reinterpret_cast<RuntimeValue1 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz1_98;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_95;
}
{
	TICK(12);
// instr 13
}
__func11_instr13:
{
	RuntimeValue4 __bp_offs_16_sz4_99;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_99 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_16_sz4_99.p, 13);
// instr 14
	RuntimeValue4 __bp_offs_24_sz4_100;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-24)));
		__bp_offs_24_sz4_100 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_24_sz4_100.p, 14);
// instr 15
}
__func11_instr15:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[6]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 15, -32, -8, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 16
}
__func11_instr16:
{
// rdxsrc/Apps/RegressionTest.rdx [line 280]
// instr 17
}
__func11_instr17:
{
	RuntimeValue4 __bp_offs_24_sz4_101;
	__bp_offs_24_sz4_101.cp = resArgs[3];
// instr 18
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_101;
}
{
	TICK(18);
// instr 19
}
__func11_instr19:
{
	RuntimeValue4 __bp_offs_24_sz4_102;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-24)));
		__bp_offs_24_sz4_102 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_24_sz4_102.p, 19);
// instr 20
}
__func11_instr20:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[8]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 20, -32, -16, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 21
}
__func11_instr21:
{
	RuntimeValue4 __bp_offs_16_sz4_103;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_103 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_16_sz4_103.p, 21);
	if(!objm->ObjectCompatible(__bp_offs_16_sz4_103.p, static_cast<const Type *>(resArgs[1])))
		INCOMPATIBLECONVERSION(21);
// instr 22
	RuntimeValue4 __bp_offs_16_sz4_104;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_104 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_4_sz4_105;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_104);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_4_sz4_105);
		*dest = *src;
	}
// instr 23
// rdxsrc/Apps/RegressionTest.rdx [line 276]
// instr 24
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_105;
}
{
	EXITFRAME;
// instr 25
}
{
	INVALIDOPERATIONEXCEPTION(25);
}
__func11_start:
switch(startInstruction)
{
case 0: goto __func11_instr0;
case 1: goto __func11_instr1;
case 6: goto __func11_instr6;
case 7: goto __func11_instr7;
case 13: goto __func11_instr13;
case 15: goto __func11_instr15;
case 16: goto __func11_instr16;
case 17: goto __func11_instr17;
case 19: goto __func11_instr19;
case 20: goto __func11_instr20;
case 21: goto __func11_instr21;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestOrderOfOperations()
static int f12(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func12_start;
__func12_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 366]
// instr 1
	RuntimeValue4 __bp_offs_8_sz4_106;
	__bp_offs_8_sz4_106.cp = resArgs[0];
// instr 2
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_106;
}
{
	TICK(2);
// instr 3
}
__func12_instr3:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[2]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 3, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 4
}
__func12_instr4:
{
// rdxsrc/Apps/RegressionTest.rdx [line 367]
// instr 5
}
__func12_instr5:
{
	RuntimeValue4 __bp_offs_8_sz4_107;
	__bp_offs_8_sz4_107.cp = resArgs[3];
// instr 6
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_107;
}
{
	TICK(6);
// instr 7
}
__func12_instr7:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[2]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 7, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 8
}
__func12_instr8:
{
// rdxsrc/Apps/RegressionTest.rdx [line 364]
// instr 9
}
__func12_instr9:
{
	EXITFRAME;
// instr 10
}
{
	INVALIDOPERATIONEXCEPTION(10);
}
__func12_start:
switch(startInstruction)
{
case 0: goto __func12_instr0;
case 3: goto __func12_instr3;
case 4: goto __func12_instr4;
case 5: goto __func12_instr5;
case 7: goto __func12_instr7;
case 8: goto __func12_instr8;
case 9: goto __func12_instr9;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest.UnrelatedStruct/methods/Test()
static int f13(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func13_start;
__func13_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 32]
// instr 1
	RuntimeValue4 __bp_offs_8_sz4_108;
	__bp_offs_8_sz4_108.cp = resArgs[0];
// instr 2
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_108;
}
{
	TICK(2);
// instr 3
}
__func13_instr3:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[2]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 3, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 4
}
__func13_instr4:
{
// rdxsrc/Apps/RegressionTest.rdx [line 30]
// instr 5
}
__func13_instr5:
{
	EXITFRAME;
// instr 6
}
{
	INVALIDOPERATIONEXCEPTION(6);
}
__func13_start:
switch(startInstruction)
{
case 0: goto __func13_instr0;
case 3: goto __func13_instr3;
case 4: goto __func13_instr4;
case 5: goto __func13_instr5;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestExceptionInstrOffsetter()
static int f14(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func14_start;
__func14_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 131]
// instr 1
}
{
	EXITFRAME;
// instr 2
}
{
	INVALIDOPERATIONEXCEPTION(2);
}
__func14_start:
switch(startInstruction)
{
case 0: goto __func14_instr0;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestBD_Static(Core.string)
static int f15(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func15_start;
__func15_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 211]
// instr 1
	RuntimeValue4 __bp_offs_8_sz4_109;
	__bp_offs_8_sz4_109.cp = resArgs[0];
// instr 2
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_109;
}
{
	TICK(2);
// instr 3
}
__func15_instr3:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[2]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 3, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 4
}
__func15_instr4:
{
// rdxsrc/Apps/RegressionTest.rdx [line 209]
// instr 5
}
__func15_instr5:
{
	EXITFRAME;
// instr 6
}
{
	INVALIDOPERATIONEXCEPTION(6);
}
__func15_start:
switch(startInstruction)
{
case 0: goto __func15_instr0;
case 3: goto __func15_instr3;
case 4: goto __func15_instr4;
case 5: goto __func15_instr5;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestHashTable()
static int f16(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func16_start;
__func16_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 99]
// instr 1
	RuntimeValue4 __bp_offs_4_sz4_110;
	memset(&__bp_offs_4_sz4_110, 0, 4);
// instr 2
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_110;
}
__func16_instr2:
{
	RuntimeValue4 __bp_offs_48_sz4_111;
	{
		t->frame.ip = instrTable + ((2) + 1);
		const Type *t = static_cast<const Type *>(resArgs[0]);
		void *obj = RDX_RuntimeUtilities_NewObjectInstance(ctx, objm, t, reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8 *>(bp) + (0)), 0);
		NULLCHECK(obj, 2);
		__bp_offs_48_sz4_111.p = obj;
	}
// instr 3
	RuntimeValue4 __bp_offs_56_sz4_112;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_48_sz4_111);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_56_sz4_112);
		*dest = *src;
	}
// instr 4
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_111;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-56)))) = __bp_offs_56_sz4_112;
}
{
	TICK(4);
// instr 5
}
__func16_instr5:
{
	RuntimeValue4 __bp_offs_56_sz4_113;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-56)));
		__bp_offs_56_sz4_113 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_56_sz4_113.p, 5);
// instr 6
}
__func16_instr6:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[1]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 6, -64, -48, thread, instrTable, bp, providerDictionary);
	}
// instr 7
}
__func16_instr7:
{
	RuntimeValue4 __bp_offs_48_sz4_114;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-48)));
		__bp_offs_48_sz4_114 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_4_sz4_115;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_48_sz4_114);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_4_sz4_115);
		*dest = *src;
	}
// instr 8
// rdxsrc/Apps/RegressionTest.rdx [line 101]
// instr 9
	RuntimeValue4 __bp_offs_48_sz4_116;
	__bp_offs_48_sz4_116.cp = resArgs[2];
// instr 10
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_116;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_115;
}
{
	TICK(10);
// instr 11
}
__func16_instr11:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[4]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 11, -48, -40, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 12
}
__func16_instr12:
{
// rdxsrc/Apps/RegressionTest.rdx [line 102]
// instr 13
}
__func16_instr13:
{
	RuntimeValue4 __bp_offs_8_sz4_117;
	memset(&__bp_offs_8_sz4_117, 0, 4);
// instr 14
	RuntimeValue4 __bp_offs_48_sz4_118;
	__bp_offs_48_sz4_118.addrs[0] = PackAddress(0,0,0,0);
// instr 15
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_48_sz4_118);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_117);
		*dest = *src;
	}
// instr 16
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_117;
}
{
	goto __func16_instr21;
// instr 17
}
__func16_instr17:
{
	RuntimeValue4 __bp_offs_8_sz4_119;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-8)));
		__bp_offs_8_sz4_119 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_56_sz4_120;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_119);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_56_sz4_120);
		*dest = *src;
	}
// instr 18
	RuntimeValue4 __bp_offs_64_sz4_121;
	__bp_offs_64_sz4_121.addrs[0] = PackAddress(1,0,0,0);
// instr 19
	RuntimeValue4 __bp_offs_48_sz4_122;
	__bp_offs_48_sz4_122.i32 = __bp_offs_56_sz4_120.i32 + __bp_offs_64_sz4_121.i32;
// instr 20
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_48_sz4_122);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_119);
		*dest = *src;
	}
// instr 21
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_119;
}
__func16_instr21:
{
	RuntimeValue4 __bp_offs_8_sz4_123;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-8)));
		__bp_offs_8_sz4_123 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_48_sz4_124;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_123);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_48_sz4_124);
		*dest = *src;
	}
// instr 22
	RuntimeValue4 __bp_offs_56_sz4_125;
	__bp_offs_56_sz4_125.addrs[0] = PackAddress(20,0,0,0);
// instr 23
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_124;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-56)))) = __bp_offs_56_sz4_125;
	if(__bp_offs_48_sz4_124.i32 >= __bp_offs_56_sz4_125.i32)
		goto __func16_instr41;
// instr 24
// rdxsrc/Apps/RegressionTest.rdx [line 103]
// instr 25
	RuntimeValue4 __bp_offs_56_sz4_126;
	__bp_offs_56_sz4_126.cp = resArgs[8];
// instr 26
	RuntimeValue4 __bp_offs_72_sz4_127;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_123);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_72_sz4_127);
		*dest = *src;
	}
// instr 27
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-56)))) = __bp_offs_56_sz4_126;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-72)))) = __bp_offs_72_sz4_127;
}
{
	TICK(27);
// instr 28
}
__func16_instr28:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[9]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 28, -80, -64, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 29
}
__func16_instr29:
{
	TICK(29);
// instr 30
}
__func16_instr30:
{
	RuntimeValue4 __bp_offs_56_sz4_128;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-56)));
		__bp_offs_56_sz4_128 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_56_sz4_128.p, 30);
// instr 31
}
__func16_instr31:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[10]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 31, -64, -48, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 32
}
__func16_instr32:
{
	RuntimeValue4 __bp_offs_4_sz4_129;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-4)));
		__bp_offs_4_sz4_129 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_56_sz4_130;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_129);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_56_sz4_130);
		*dest = *src;
	}
// instr 33
	RuntimeValue4 __bp_offs_8_sz4_131;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-8)));
		__bp_offs_8_sz4_131 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_64_sz4_132;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_131);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_64_sz4_132);
		*dest = *src;
	}
// instr 34
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-56)))) = __bp_offs_56_sz4_130;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-64)))) = __bp_offs_64_sz4_132;
}
{
	TICK(34);
// instr 35
}
__func16_instr35:
{
	RuntimeValue4 __bp_offs_56_sz4_133;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-56)));
		__bp_offs_56_sz4_133 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_56_sz4_133.p, 35);
// instr 36
}
__func16_instr36:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[11]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 36, -64, -40, thread, instrTable, bp, providerDictionary);
	}
// instr 37
}
__func16_instr37:
{
// rdxsrc/Apps/RegressionTest.rdx [line 102]
// instr 38
}
__func16_instr38:
{
	TICK(38);
// instr 39
}
__func16_instr39:
{
	goto __func16_instr17;
// instr 40
// rdxsrc/Apps/RegressionTest.rdx [line 105]
// instr 41
}
__func16_instr41:
{
	RuntimeValue12 __bp_offs_16_sz12_134;
	memset(&__bp_offs_16_sz12_134, 0, 12);
// instr 42
	RuntimeValue4 __bp_offs_20_sz4_135;
	memset(&__bp_offs_20_sz4_135, 0, 4);
// instr 43
	RuntimeValue4 __bp_offs_24_sz4_136;
	memset(&__bp_offs_24_sz4_136, 0, 4);
// instr 44
	RuntimeValue12 __bp_offs_36_sz12_137;
	memset(&__bp_offs_36_sz12_137, 0, 12);
// instr 45
	*reinterpret_cast<RuntimeValue12 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz12_134;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-20)))) = __bp_offs_20_sz4_135;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_136;
	*reinterpret_cast<RuntimeValue12 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-36)))) = __bp_offs_36_sz12_137;
}
{
	RuntimeValue8 __bp_offs_48_sz8_138;
	__bp_offs_48_sz8_138.rtp.objectRef = &thread;
	__bp_offs_48_sz8_138.rtp.valueRef = (static_cast<void *>(static_cast< UInt8 *>(bp) + (-36)));
// instr 46
	RuntimeValue4 __bp_offs_4_sz4_139;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-4)));
		__bp_offs_4_sz4_139 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_56_sz4_140;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_139);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_56_sz4_140);
		*dest = *src;
	}
// instr 47
	*reinterpret_cast<RuntimeValue8 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz8_138;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-56)))) = __bp_offs_56_sz4_140;
}
{
	TICK(47);
// instr 48
}
__func16_instr48:
{
	RuntimeValue4 __bp_offs_56_sz4_141;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-56)));
		__bp_offs_56_sz4_141 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_56_sz4_141.p, 48);
// instr 49
}
__func16_instr49:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[13]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 49, -64, -48, thread, instrTable, bp, providerDictionary);
	}
// instr 50
}
__func16_instr50:
{
	RuntimeValue8 __bp_offs_48_sz8_142;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-48)));
		__bp_offs_48_sz8_142 = *reinterpret_cast<const RuntimeValue8 *>(src);
	}
	RuntimeValue12 __bp_offs_16_sz12_143;
	{
		const RuntimeValue12 *src;
		RuntimeValue12 *dest;
		const RuntimePointer<void> *srcPtr = &__bp_offs_48_sz8_142.rtp;
		src = static_cast<const RuntimeValue12 *>(srcPtr->valueRef);
		dest = reinterpret_cast<RuntimeValue12 *>(&__bp_offs_16_sz12_143);
		*dest = *src;
	}
// instr 51
	*reinterpret_cast<RuntimeValue12 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz12_143;
}
__func16_instr51:
{
	RuntimeValue8 __bp_offs_56_sz8_144;
	__bp_offs_56_sz8_144.rtp.objectRef = &thread;
	__bp_offs_56_sz8_144.rtp.valueRef = (static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)));
// instr 52
	*reinterpret_cast<RuntimeValue8 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-56)))) = __bp_offs_56_sz8_144;
}
{
	TICK(52);
// instr 53
}
__func16_instr53:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[15]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 53, -64, -48, thread, instrTable, bp, providerDictionary);
	}
// instr 54
}
__func16_instr54:
{
	RuntimeValue1 __bp_offs_48_sz1_145;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-48)));
		__bp_offs_48_sz1_145 = *reinterpret_cast<const RuntimeValue1 *>(src);
	}
	if(__bp_offs_48_sz1_145.bo == FalseValue)
		goto __func16_instr72;
// instr 55
}
{
	RuntimeValue8 __bp_offs_64_sz8_146;
	__bp_offs_64_sz8_146.rtp.objectRef = &thread;
	__bp_offs_64_sz8_146.rtp.valueRef = (static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)));
// instr 56
	*reinterpret_cast<RuntimeValue8 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-64)))) = __bp_offs_64_sz8_146;
}
{
	TICK(56);
// instr 57
}
__func16_instr57:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[16]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 57, -64, -56, thread, instrTable, bp, providerDictionary);
	}
// instr 58
}
__func16_instr58:
{
	RuntimeValue4 __bp_offs_56_sz4_147;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-56)));
		__bp_offs_56_sz4_147 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_24_sz4_148;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_56_sz4_147);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_24_sz4_148);
		*dest = *src;
	}
// instr 59
	RuntimeValue4 __bp_offs_48_sz4_149;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-48)));
		__bp_offs_48_sz4_149 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_20_sz4_150;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_48_sz4_149);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_20_sz4_150);
		*dest = *src;
	}
// instr 60
// rdxsrc/Apps/RegressionTest.rdx [line 107]
// instr 61
	RuntimeValue4 __bp_offs_48_sz4_151;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_20_sz4_150);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_48_sz4_151);
		*dest = *src;
	}
// instr 62
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-20)))) = __bp_offs_20_sz4_150;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_148;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_151;
}
{
	TICK(62);
// instr 63
}
__func16_instr63:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[17]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 63, -48, -40, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 64
}
__func16_instr64:
{
// rdxsrc/Apps/RegressionTest.rdx [line 108]
// instr 65
}
__func16_instr65:
{
	RuntimeValue4 __bp_offs_24_sz4_152;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-24)));
		__bp_offs_24_sz4_152 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_48_sz4_153;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_24_sz4_152);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_48_sz4_153);
		*dest = *src;
	}
// instr 66
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_153;
}
{
	TICK(66);
// instr 67
}
__func16_instr67:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[4]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 67, -48, -40, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 68
}
__func16_instr68:
{
// rdxsrc/Apps/RegressionTest.rdx [line 105]
// instr 69
}
__func16_instr69:
{
	TICK(69);
// instr 70
}
__func16_instr70:
{
	goto __func16_instr51;
// instr 71
// rdxsrc/Apps/RegressionTest.rdx [line 99]
// instr 72
}
__func16_instr72:
{
	EXITFRAME;
// instr 73
}
{
	INVALIDOPERATIONEXCEPTION(73);
}
__func16_start:
switch(startInstruction)
{
case 0: goto __func16_instr0;
case 2: goto __func16_instr2;
case 5: goto __func16_instr5;
case 6: goto __func16_instr6;
case 7: goto __func16_instr7;
case 11: goto __func16_instr11;
case 12: goto __func16_instr12;
case 13: goto __func16_instr13;
case 17: goto __func16_instr17;
case 21: goto __func16_instr21;
case 28: goto __func16_instr28;
case 29: goto __func16_instr29;
case 30: goto __func16_instr30;
case 31: goto __func16_instr31;
case 32: goto __func16_instr32;
case 35: goto __func16_instr35;
case 36: goto __func16_instr36;
case 37: goto __func16_instr37;
case 38: goto __func16_instr38;
case 39: goto __func16_instr39;
case 41: goto __func16_instr41;
case 48: goto __func16_instr48;
case 49: goto __func16_instr49;
case 50: goto __func16_instr50;
case 51: goto __func16_instr51;
case 53: goto __func16_instr53;
case 54: goto __func16_instr54;
case 57: goto __func16_instr57;
case 58: goto __func16_instr58;
case 63: goto __func16_instr63;
case 64: goto __func16_instr64;
case 65: goto __func16_instr65;
case 67: goto __func16_instr67;
case 68: goto __func16_instr68;
case 69: goto __func16_instr69;
case 70: goto __func16_instr70;
case 72: goto __func16_instr72;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestSwitch(Core.int)
static int f17(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func17_start;
__func17_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 114]
// instr 1
	RuntimeValue4 __prv_offs_8_sz4_154;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-8)));
		__prv_offs_8_sz4_154 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_8_sz4_155;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_8_sz4_154);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_155);
		*dest = *src;
	}
// instr 2
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_155;
	{
		const void *pv = &__bp_offs_8_sz4_155;
		const UInt8 *caseBytes = static_cast<const UInt8 *>(resArgs[0]);
		if(!memcmp(caseBytes + 0, pv, 4))
			goto __func17_instr3;
		if(!memcmp(caseBytes + 4, pv, 4))
			goto __func17_instr4;
		if(!memcmp(caseBytes + 8, pv, 4))
			goto __func17_instr5;
		goto __func17_instr6;
	}
// instr 3
}
__func17_instr3:
{
	goto __func17_instr8;
// instr 4
}
__func17_instr4:
{
	goto __func17_instr8;
// instr 5
}
__func17_instr5:
{
	goto __func17_instr14;
// instr 6
}
__func17_instr6:
{
	goto __func17_instr20;
// instr 7
// rdxsrc/Apps/RegressionTest.rdx [line 118]
// instr 8
}
__func17_instr8:
{
	RuntimeValue4 __bp_offs_8_sz4_156;
	__bp_offs_8_sz4_156.cp = resArgs[1];
// instr 9
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_156;
}
{
	TICK(9);
// instr 10
}
__func17_instr10:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[3]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 10, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 11
}
__func17_instr11:
{
// rdxsrc/Apps/RegressionTest.rdx [line 114]
// instr 12
}
__func17_instr12:
{
	EXITFRAME;
// instr 13
// rdxsrc/Apps/RegressionTest.rdx [line 122]
// instr 14
}
__func17_instr14:
{
	RuntimeValue4 __bp_offs_8_sz4_157;
	__bp_offs_8_sz4_157.cp = resArgs[4];
// instr 15
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_157;
}
{
	TICK(15);
// instr 16
}
__func17_instr16:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[3]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 16, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 17
}
__func17_instr17:
{
// rdxsrc/Apps/RegressionTest.rdx [line 114]
// instr 18
}
__func17_instr18:
{
	EXITFRAME;
// instr 19
// rdxsrc/Apps/RegressionTest.rdx [line 126]
// instr 20
}
__func17_instr20:
{
	RuntimeValue4 __bp_offs_8_sz4_158;
	__bp_offs_8_sz4_158.cp = resArgs[5];
// instr 21
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_158;
}
{
	TICK(21);
// instr 22
}
__func17_instr22:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[3]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 22, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 23
}
__func17_instr23:
{
// rdxsrc/Apps/RegressionTest.rdx [line 112]
// instr 24
}
__func17_instr24:
{
	EXITFRAME;
// instr 25
}
{
	INVALIDOPERATIONEXCEPTION(25);
}
__func17_start:
switch(startInstruction)
{
case 0: goto __func17_instr0;
case 3: goto __func17_instr3;
case 4: goto __func17_instr4;
case 5: goto __func17_instr5;
case 6: goto __func17_instr6;
case 8: goto __func17_instr8;
case 10: goto __func17_instr10;
case 11: goto __func17_instr11;
case 12: goto __func17_instr12;
case 14: goto __func17_instr14;
case 16: goto __func17_instr16;
case 17: goto __func17_instr17;
case 18: goto __func17_instr18;
case 20: goto __func17_instr20;
case 22: goto __func17_instr22;
case 23: goto __func17_instr23;
case 24: goto __func17_instr24;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestInterfaces()
static int f18(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func18_start;
__func18_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 163]
// instr 1
	RuntimeValue4 __bp_offs_4_sz4_159;
	memset(&__bp_offs_4_sz4_159, 0, 4);
// instr 2
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_159;
}
__func18_instr2:
{
	RuntimeValue4 __bp_offs_16_sz4_160;
	{
		t->frame.ip = instrTable + ((2) + 1);
		const Type *t = static_cast<const Type *>(resArgs[1]);
		void *obj = RDX_RuntimeUtilities_NewObjectInstance(ctx, objm, t, reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8 *>(bp) + (0)), 0);
		NULLCHECK(obj, 2);
		__bp_offs_16_sz4_160.p = obj;
	}
// instr 3
	RuntimeValue4 __bp_offs_4_sz4_161;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_160);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_4_sz4_161);
		*dest = *src;
	}
// instr 4
// rdxsrc/Apps/RegressionTest.rdx [line 164]
// instr 5
	RuntimeValue4 __bp_offs_16_sz4_162;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_161);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_16_sz4_162);
		*dest = *src;
	}
// instr 6
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_162;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_161;
}
{
	TICK(6);
// instr 7
}
__func18_instr7:
{
	RuntimeValue4 __bp_offs_16_sz4_163;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_163 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_16_sz4_163.p, 7);
// instr 8
}
__func18_instr8:
{
	RuntimeValue4 __bp_offs_16_sz4_164;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_164 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const Method *imethod = static_cast<const Method *>(resArgs[2]);
		const Type *interfaceType = imethod->parameters[imethod->thisParameterOffset - 1].type;
		const InterfaceImplementation *impl = static_cast<const StructuredType *>(GCInfo::From(__bp_offs_16_sz4_164.p)->containerType)->interfaces;
		while(impl->type != interfaceType)
			impl++;
		const Method *invokedMethod =
			static_cast<const StructuredType*>(GCInfo::From(__bp_offs_16_sz4_164.p)->containerType)->virtualMethods[(1) + impl->vftOffset];
		bool shouldContinue = false;
		int methodStatus = CallMethod(ctx, objm, invokedMethod, 8, -16, -8, thread, instrTable, bp, providerDictionary, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 9
}
__func18_instr9:
{
// rdxsrc/Apps/RegressionTest.rdx [line 163]
// instr 10
}
__func18_instr10:
{
	EXITFRAME;
// instr 11
}
{
	INVALIDOPERATIONEXCEPTION(11);
}
__func18_start:
switch(startInstruction)
{
case 0: goto __func18_instr0;
case 2: goto __func18_instr2;
case 7: goto __func18_instr7;
case 8: goto __func18_instr8;
case 9: goto __func18_instr9;
case 10: goto __func18_instr10;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestLargeParameters()
static int f19(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func19_start;
__func19_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 254]
// instr 1
	RuntimeValue16 __bp_offs_16_sz16_165;
	memset(&__bp_offs_16_sz16_165, 0, 16);
// instr 2
// rdxsrc/Apps/RegressionTest.rdx [line 255]
// instr 3
	RuntimeValue4 __bp_offs_24_sz4_166;
	__bp_offs_24_sz4_166.addrs[0] = PackAddress(2,0,0,0);
// instr 4
	*reinterpret_cast<RuntimeValue16 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz16_165;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_166;
}
{
	RuntimeValue8 __bp_offs_32_sz8_167;
	__bp_offs_32_sz8_167.rtp.objectRef = &thread;
	__bp_offs_32_sz8_167.rtp.valueRef = (static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)));
// instr 5
	RuntimeValue4 __bp_offs_24_sz4_168;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-24)));
		__bp_offs_24_sz4_168 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_24_sz4_168);
		const RuntimePointer<void> *destPtr = &__bp_offs_32_sz8_167.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 6
// rdxsrc/Apps/RegressionTest.rdx [line 256]
// instr 7
	RuntimeValue4 __bp_offs_24_sz4_169;
	__bp_offs_24_sz4_169.addrs[0] = PackAddress(3,0,0,0);
// instr 8
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_169;
}
{
	RuntimeValue8 __bp_offs_32_sz8_170;
	__bp_offs_32_sz8_170.rtp.objectRef = &thread;
	__bp_offs_32_sz8_170.rtp.valueRef = (static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)));
// instr 9
	RuntimeValue4 __bp_offs_24_sz4_171;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-24)));
		__bp_offs_24_sz4_171 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_24_sz4_171);
		const RuntimePointer<void> *destPtr = &__bp_offs_32_sz8_170.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 10
// rdxsrc/Apps/RegressionTest.rdx [line 257]
// instr 11
	RuntimeValue4 __bp_offs_24_sz4_172;
	__bp_offs_24_sz4_172.addrs[0] = PackAddress(4,0,0,0);
// instr 12
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_172;
}
{
	RuntimeValue8 __bp_offs_32_sz8_173;
	__bp_offs_32_sz8_173.rtp.objectRef = &thread;
	__bp_offs_32_sz8_173.rtp.valueRef = (static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)));
// instr 13
	RuntimeValue4 __bp_offs_24_sz4_174;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-24)));
		__bp_offs_24_sz4_174 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_24_sz4_174);
		const RuntimePointer<void> *destPtr = &__bp_offs_32_sz8_173.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 14
// rdxsrc/Apps/RegressionTest.rdx [line 258]
// instr 15
	RuntimeValue4 __bp_offs_24_sz4_175;
	__bp_offs_24_sz4_175.addrs[0] = PackAddress(5,0,0,0);
// instr 16
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_175;
}
{
	RuntimeValue8 __bp_offs_32_sz8_176;
	__bp_offs_32_sz8_176.rtp.objectRef = &thread;
	__bp_offs_32_sz8_176.rtp.valueRef = (static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)));
// instr 17
	RuntimeValue4 __bp_offs_24_sz4_177;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-24)));
		__bp_offs_24_sz4_177 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_24_sz4_177);
		const RuntimePointer<void> *destPtr = &__bp_offs_32_sz8_176.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 18
// rdxsrc/Apps/RegressionTest.rdx [line 259]
// instr 19
	RuntimeValue4 __bp_offs_24_sz4_178;
	__bp_offs_24_sz4_178.addrs[0] = PackAddress(1,0,0,0);
// instr 20
	RuntimeValue16 __bp_offs_16_sz16_179;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz16_179 = *reinterpret_cast<const RuntimeValue16 *>(src);
	}
	RuntimeValue16 __bp_offs_40_sz16_180;
	{
		const RuntimeValue16 *src;
		RuntimeValue16 *dest;
		src = reinterpret_cast<const RuntimeValue16 *>(&__bp_offs_16_sz16_179);
		dest = reinterpret_cast<RuntimeValue16 *>(&__bp_offs_40_sz16_180);
		*dest = *src;
	}
// instr 21
	RuntimeValue4 __bp_offs_48_sz4_181;
	__bp_offs_48_sz4_181.addrs[0] = PackAddress(6,0,0,0);
// instr 22
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_178;
	*reinterpret_cast<RuntimeValue16 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz16_180;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_181;
}
{
	TICK(22);
// instr 23
}
__func19_instr23:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[2]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 23, -48, -16, thread, instrTable, bp, providerDictionary);
	}
// instr 24
}
__func19_instr24:
{
// rdxsrc/Apps/RegressionTest.rdx [line 254]
// instr 25
}
__func19_instr25:
{
	EXITFRAME;
// instr 26
}
{
	INVALIDOPERATIONEXCEPTION(26);
}
__func19_start:
switch(startInstruction)
{
case 0: goto __func19_instr0;
case 23: goto __func19_instr23;
case 24: goto __func19_instr24;
case 25: goto __func19_instr25;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestThreadDeserializationPD1(#DS-#TT-()#PL-())
static int f20(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func20_start;
__func20_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 291]
// instr 1
}
{
	TICK(1);
// instr 2
}
__func20_instr2:
{
	RuntimeValue4 __prv_offs_8_sz4_182;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-8)));
		__prv_offs_8_sz4_182 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const Method *invokedMethod = static_cast<const Method *>(__prv_offs_8_sz4_182.p);
		NULLCHECK(invokedMethod, 2);
		bool shouldContinue = false;
		int methodStatus = CallMethod(ctx, objm, invokedMethod, 2, 0, 0, thread, instrTable, bp, providerDictionary, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 3
}
__func20_instr3:
{
// rdxsrc/Apps/RegressionTest.rdx [line 289]
// instr 4
}
__func20_instr4:
{
	EXITFRAME;
// instr 5
}
{
	INVALIDOPERATIONEXCEPTION(5);
}
__func20_start:
switch(startInstruction)
{
case 0: goto __func20_instr0;
case 2: goto __func20_instr2;
case 3: goto __func20_instr3;
case 4: goto __func20_instr4;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestDeadCodeElimination()
static int f21(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func21_start;
__func21_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 350]
// instr 1
}
{
	EXITFRAME;
// instr 2
}
{
	INVALIDOPERATIONEXCEPTION(2);
}
__func21_start:
switch(startInstruction)
{
case 0: goto __func21_instr0;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest.InterfaceImplementingClass/methods/Func1()
static int f22(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func22_start;
__func22_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 74]
// instr 1
}
{
	EXITFRAME;
// instr 2
}
{
	INVALIDOPERATIONEXCEPTION(2);
}
__func22_start:
switch(startInstruction)
{
case 0: goto __func22_instr0;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/main(#Core.string[C])
static int f23(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func23_start;
__func23_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 372]
// instr 1
}
{
	TICK(1);
// instr 2
}
__func23_instr2:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[1]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 2, -16, -16, thread, instrTable, bp, providerDictionary);
	}
// instr 3
}
__func23_instr3:
{
// rdxsrc/Apps/RegressionTest.rdx [line 373]
// instr 4
}
__func23_instr4:
{
	TICK(4);
// instr 5
}
__func23_instr5:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[2]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 5, 0, -8, thread, instrTable, bp, providerDictionary);
	}
// instr 6
}
__func23_instr6:
{
// rdxsrc/Apps/RegressionTest.rdx [line 374]
// instr 7
}
__func23_instr7:
{
	TICK(7);
// instr 8
}
__func23_instr8:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[3]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 8, 0, -8, thread, instrTable, bp, providerDictionary);
	}
// instr 9
}
__func23_instr9:
{
// rdxsrc/Apps/RegressionTest.rdx [line 375]
// instr 10
}
__func23_instr10:
{
	TICK(10);
// instr 11
}
__func23_instr11:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[4]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 11, 0, -8, thread, instrTable, bp, providerDictionary);
	}
// instr 12
}
__func23_instr12:
{
// rdxsrc/Apps/RegressionTest.rdx [line 376]
// instr 13
}
__func23_instr13:
{
	RuntimeValue4 __bp_offs_4_sz4_183;
	memset(&__bp_offs_4_sz4_183, 0, 4);
// instr 14
	RuntimeValue4 __bp_offs_16_sz4_184;
	__bp_offs_16_sz4_184.addrs[0] = PackAddress(0,0,0,0);
// instr 15
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_184);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_4_sz4_183);
		*dest = *src;
	}
// instr 16
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_183;
}
{
	goto __func23_instr21;
// instr 17
}
__func23_instr17:
{
	RuntimeValue4 __bp_offs_4_sz4_185;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-4)));
		__bp_offs_4_sz4_185 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_24_sz4_186;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_185);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_24_sz4_186);
		*dest = *src;
	}
// instr 18
	RuntimeValue4 __bp_offs_32_sz4_187;
	__bp_offs_32_sz4_187.addrs[0] = PackAddress(1,0,0,0);
// instr 19
	RuntimeValue4 __bp_offs_16_sz4_188;
	__bp_offs_16_sz4_188.i32 = __bp_offs_24_sz4_186.i32 + __bp_offs_32_sz4_187.i32;
// instr 20
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_188);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_4_sz4_185);
		*dest = *src;
	}
// instr 21
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_185;
}
__func23_instr21:
{
	RuntimeValue4 __bp_offs_4_sz4_189;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-4)));
		__bp_offs_4_sz4_189 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_16_sz4_190;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_189);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_16_sz4_190);
		*dest = *src;
	}
// instr 22
	RuntimeValue4 __bp_offs_24_sz4_191;
	__bp_offs_24_sz4_191.addrs[0] = PackAddress(5,0,0,0);
// instr 23
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_190;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_191;
	if(__bp_offs_16_sz4_190.i32 >= __bp_offs_24_sz4_191.i32)
		goto __func23_instr32;
// instr 24
// rdxsrc/Apps/RegressionTest.rdx [line 377]
// instr 25
	RuntimeValue4 __bp_offs_16_sz4_192;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_189);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_16_sz4_192);
		*dest = *src;
	}
// instr 26
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_192;
}
{
	TICK(26);
// instr 27
}
__func23_instr27:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[7]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 27, -16, -8, thread, instrTable, bp, providerDictionary);
	}
// instr 28
}
__func23_instr28:
{
// rdxsrc/Apps/RegressionTest.rdx [line 376]
// instr 29
}
__func23_instr29:
{
	TICK(29);
// instr 30
}
__func23_instr30:
{
	goto __func23_instr17;
// instr 31
// rdxsrc/Apps/RegressionTest.rdx [line 378]
// instr 32
}
__func23_instr32:
{
	TICK(32);
// instr 33
}
__func23_instr33:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[8]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 33, 0, -8, thread, instrTable, bp, providerDictionary);
	}
// instr 34
}
__func23_instr34:
{
// rdxsrc/Apps/RegressionTest.rdx [line 379]
// instr 35
}
__func23_instr35:
{
	TICK(35);
// instr 36
}
__func23_instr36:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[9]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 36, -16, -16, thread, instrTable, bp, providerDictionary);
	}
// instr 37
}
__func23_instr37:
{
// rdxsrc/Apps/RegressionTest.rdx [line 380]
// instr 38
}
__func23_instr38:
{
	TICK(38);
// instr 39
}
__func23_instr39:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[10]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 39, 0, -8, thread, instrTable, bp, providerDictionary);
	}
// instr 40
}
__func23_instr40:
{
// rdxsrc/Apps/RegressionTest.rdx [line 381]
// instr 41
}
__func23_instr41:
{
	TICK(41);
// instr 42
}
__func23_instr42:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[11]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 42, 0, -8, thread, instrTable, bp, providerDictionary);
	}
// instr 43
}
__func23_instr43:
{
// rdxsrc/Apps/RegressionTest.rdx [line 382]
// instr 44
}
__func23_instr44:
{
	TICK(44);
// instr 45
}
__func23_instr45:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[12]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 45, 0, -8, thread, instrTable, bp, providerDictionary);
	}
// instr 46
}
__func23_instr46:
{
// rdxsrc/Apps/RegressionTest.rdx [line 383]
// instr 47
}
__func23_instr47:
{
	TICK(47);
// instr 48
}
__func23_instr48:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[13]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 48, 0, -8, thread, instrTable, bp, providerDictionary);
	}
// instr 49
}
__func23_instr49:
{
// rdxsrc/Apps/RegressionTest.rdx [line 384]
// instr 50
}
__func23_instr50:
{
	TICK(50);
// instr 51
}
__func23_instr51:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[14]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 51, -16, -16, thread, instrTable, bp, providerDictionary);
	}
// instr 52
}
__func23_instr52:
{
// rdxsrc/Apps/RegressionTest.rdx [line 385]
// instr 53
}
__func23_instr53:
{
	TICK(53);
// instr 54
}
__func23_instr54:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[15]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 54, 0, -8, thread, instrTable, bp, providerDictionary);
	}
// instr 55
}
__func23_instr55:
{
// rdxsrc/Apps/RegressionTest.rdx [line 386]
// instr 56
}
__func23_instr56:
{
	RuntimeValue4 __bp_offs_16_sz4_193;
	__bp_offs_16_sz4_193.addrs[0] = PackAddress(0,0,0,0);
// instr 57
	RuntimeValue4 __prv_offs0_sz4_194;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_193);
		dest = reinterpret_cast<RuntimeValue4 *>(&__prv_offs0_sz4_194);
		*dest = *src;
	}
// instr 58
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(prv) + (0)))) = __prv_offs0_sz4_194;
}
{
	EXITFRAME;
// instr 59
}
{
	INVALIDOPERATIONEXCEPTION(59);
}
__func23_start:
switch(startInstruction)
{
case 0: goto __func23_instr0;
case 2: goto __func23_instr2;
case 3: goto __func23_instr3;
case 4: goto __func23_instr4;
case 5: goto __func23_instr5;
case 6: goto __func23_instr6;
case 7: goto __func23_instr7;
case 8: goto __func23_instr8;
case 9: goto __func23_instr9;
case 10: goto __func23_instr10;
case 11: goto __func23_instr11;
case 12: goto __func23_instr12;
case 13: goto __func23_instr13;
case 17: goto __func23_instr17;
case 21: goto __func23_instr21;
case 27: goto __func23_instr27;
case 28: goto __func23_instr28;
case 29: goto __func23_instr29;
case 30: goto __func23_instr30;
case 32: goto __func23_instr32;
case 33: goto __func23_instr33;
case 34: goto __func23_instr34;
case 35: goto __func23_instr35;
case 36: goto __func23_instr36;
case 37: goto __func23_instr37;
case 38: goto __func23_instr38;
case 39: goto __func23_instr39;
case 40: goto __func23_instr40;
case 41: goto __func23_instr41;
case 42: goto __func23_instr42;
case 43: goto __func23_instr43;
case 44: goto __func23_instr44;
case 45: goto __func23_instr45;
case 46: goto __func23_instr46;
case 47: goto __func23_instr47;
case 48: goto __func23_instr48;
case 49: goto __func23_instr49;
case 50: goto __func23_instr50;
case 51: goto __func23_instr51;
case 52: goto __func23_instr52;
case 53: goto __func23_instr53;
case 54: goto __func23_instr54;
case 55: goto __func23_instr55;
case 56: goto __func23_instr56;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestUsing()
static int f24(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func24_start;
__func24_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 337]
// instr 1
	RuntimeValue4 __bp_offs_4_sz4_195;
	memset(&__bp_offs_4_sz4_195, 0, 4);
// instr 2
	RuntimeValue4 __bp_offs_40_sz4_196;
	__bp_offs_40_sz4_196.addrs[0] = PackAddress(0,0,0,0);
// instr 3
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_196);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_4_sz4_195);
		*dest = *src;
	}
// instr 4
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_195;
}
{
	goto __func24_instr9;
// instr 5
}
__func24_instr5:
{
	RuntimeValue4 __bp_offs_4_sz4_197;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-4)));
		__bp_offs_4_sz4_197 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_48_sz4_198;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_197);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_48_sz4_198);
		*dest = *src;
	}
// instr 6
	RuntimeValue4 __bp_offs_56_sz4_199;
	__bp_offs_56_sz4_199.addrs[0] = PackAddress(1,0,0,0);
// instr 7
	RuntimeValue4 __bp_offs_40_sz4_200;
	__bp_offs_40_sz4_200.i32 = __bp_offs_48_sz4_198.i32 + __bp_offs_56_sz4_199.i32;
// instr 8
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_200);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_4_sz4_197);
		*dest = *src;
	}
// instr 9
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_197;
}
__func24_instr9:
{
	RuntimeValue4 __bp_offs_4_sz4_201;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-4)));
		__bp_offs_4_sz4_201 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_40_sz4_202;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_201);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_202);
		*dest = *src;
	}
// instr 10
	RuntimeValue4 __bp_offs_48_sz4_203;
	__bp_offs_48_sz4_203.addrs[0] = PackAddress(10,0,0,0);
// instr 11
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_202;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_203;
	if(__bp_offs_40_sz4_202.i32 >= __bp_offs_48_sz4_203.i32)
		goto __func24_instr103;
// instr 12
// rdxsrc/Apps/RegressionTest.rdx [line 339]
// instr 13
	RuntimeValue4 __bp_offs_8_sz4_204;
	memset(&__bp_offs_8_sz4_204, 0, 4);
// instr 14
	RuntimeValue4 __bp_offs_12_sz4_205;
	memset(&__bp_offs_12_sz4_205, 0, 4);
// instr 15
	RuntimeValue4 __bp_offs_16_sz4_206;
	memset(&__bp_offs_16_sz4_206, 0, 4);
// instr 16
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)))) = __bp_offs_12_sz4_205;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_206;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_204;
}
{
	RuntimeValue8 __bp_offs_40_sz8_207;
	__bp_offs_40_sz8_207.rtp.objectRef = &thread;
	__bp_offs_40_sz8_207.rtp.valueRef = (static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)));
// instr 17
	RuntimeValue4 __bp_offs_48_sz4_208;
	__bp_offs_48_sz4_208.cp = resArgs[4];
// instr 18
	*reinterpret_cast<RuntimeValue8 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz8_207;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_208;
}
{
	TICK(18);
// instr 19
}
__func24_instr19:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[6]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 19, -48, -32, thread, instrTable, bp, providerDictionary);
	}
// instr 20
}
__func24_instr20:
{
	RuntimeValue4 __bp_offs_16_sz4_209;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_209 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_8_sz4_210;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_209);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_210);
		*dest = *src;
	}
// instr 21
	memset(&__bp_offs_16_sz4_209, 0, 4);
// instr 22
	RuntimeValue4 __bp_offs_20_sz4_211;
	memset(&__bp_offs_20_sz4_211, 0, 4);
// instr 23
	RuntimeValue4 __bp_offs_24_sz4_212;
	memset(&__bp_offs_24_sz4_212, 0, 4);
// instr 24
	RuntimeValue4 __bp_offs_28_sz4_213;
	memset(&__bp_offs_28_sz4_213, 0, 4);
// instr 25
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_209;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-20)))) = __bp_offs_20_sz4_211;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_212;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-28)))) = __bp_offs_28_sz4_213;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_210;
}
{
	RuntimeValue8 __bp_offs_40_sz8_214;
	__bp_offs_40_sz8_214.rtp.objectRef = &thread;
	__bp_offs_40_sz8_214.rtp.valueRef = (static_cast<void *>(static_cast< UInt8 *>(bp) + (-28)));
// instr 26
	RuntimeValue4 __bp_offs_48_sz4_215;
	__bp_offs_48_sz4_215.cp = resArgs[8];
// instr 27
	*reinterpret_cast<RuntimeValue8 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz8_214;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_215;
}
{
	TICK(27);
// instr 28
}
__func24_instr28:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[6]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 28, -48, -32, thread, instrTable, bp, providerDictionary);
	}
// instr 29
}
__func24_instr29:
{
	RuntimeValue4 __bp_offs_28_sz4_216;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-28)));
		__bp_offs_28_sz4_216 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_12_sz4_217;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_28_sz4_216);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_12_sz4_217);
		*dest = *src;
	}
// instr 30
// rdxsrc/Apps/RegressionTest.rdx [line 341]
// instr 31
	RuntimeValue4 __bp_offs_4_sz4_218;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-4)));
		__bp_offs_4_sz4_218 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_40_sz4_219;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_218);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_219);
		*dest = *src;
	}
// instr 32
	RuntimeValue4 __bp_offs_48_sz4_220;
	__bp_offs_48_sz4_220.addrs[0] = PackAddress(4,0,0,0);
// instr 33
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)))) = __bp_offs_12_sz4_217;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_219;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_220;
	if(__bp_offs_40_sz4_219.i32 == __bp_offs_48_sz4_220.i32)
		goto __func24_instr43;
// instr 34
// rdxsrc/Apps/RegressionTest.rdx [line 339]
// instr 35
	RuntimeValue4 __bp_offs_40_sz4_221;
	__bp_offs_40_sz4_221.addrs[0] = PackAddress(0,0,0,0);
// instr 36
	RuntimeValue4 __bp_offs_20_sz4_222;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_221);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_20_sz4_222);
		*dest = *src;
	}
// instr 37
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-20)))) = __bp_offs_20_sz4_222;
}
{
	goto __func24_instr45;
// instr 38
}
{
	INVALIDOPERATIONEXCEPTION(38);
// instr 39
}
__func24_instr39:
{
	RuntimeValue4 __bp_offs_40_sz4_223;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_223 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_16_sz4_224;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_223);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_16_sz4_224);
		*dest = *src;
	}
// instr 40
	RuntimeValue4 __bp_offs_40_sz4_225;
	__bp_offs_40_sz4_225.addrs[0] = PackAddress(255,255,255,255);
// instr 41
	RuntimeValue4 __bp_offs_20_sz4_226;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_225);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_20_sz4_226);
		*dest = *src;
	}
// instr 42
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_224;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-20)))) = __bp_offs_20_sz4_226;
}
{
	goto __func24_instr45;
// instr 43
}
__func24_instr43:
{
	RuntimeValue4 __bp_offs_40_sz4_227;
	__bp_offs_40_sz4_227.addrs[0] = PackAddress(3,0,0,0);
// instr 44
	RuntimeValue4 __bp_offs_20_sz4_228;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_227);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_20_sz4_228);
		*dest = *src;
	}
// instr 45
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-20)))) = __bp_offs_20_sz4_228;
}
__func24_instr45:
{
	RuntimeValue8 __bp_offs_40_sz8_229;
	__bp_offs_40_sz8_229.rtp.objectRef = &thread;
	__bp_offs_40_sz8_229.rtp.valueRef = (static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)));
// instr 46
	*reinterpret_cast<RuntimeValue8 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz8_229;
}
{
	TICK(46);
// instr 47
}
__func24_instr47:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[10]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 47, -48, -32, thread, instrTable, bp, providerDictionary);
	}
// instr 48
}
__func24_instr48:
{
	RuntimeValue4 __bp_offs_20_sz4_230;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-20)));
		__bp_offs_20_sz4_230 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_40_sz4_231;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_20_sz4_230);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_231);
		*dest = *src;
	}
// instr 49
	RuntimeValue4 __bp_offs_48_sz4_232;
	__bp_offs_48_sz4_232.addrs[0] = PackAddress(255,255,255,255);
// instr 50
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_231;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_232;
}
{
	if(memcmp(static_cast<const UInt8 *>(bp) + (-48), static_cast<const UInt8 *>(bp) + (-40), 4))
		goto __func24_instr53;
// instr 51
	RuntimeValue4 __bp_offs_16_sz4_233;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_233 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_40_sz4_234;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_233);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_234);
		*dest = *src;
	}
// instr 52
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_234;
	THROWEXCEPTION(__bp_offs_40_sz4_234.p, 52);
// instr 53
}
__func24_instr53:
{
	goto __func24_instr57;
// instr 54
	RuntimeValue4 __bp_offs_20_sz4_235;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-20)));
		__bp_offs_20_sz4_235 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_40_sz4_236;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_20_sz4_235);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_236);
		*dest = *src;
	}
// instr 55
	RuntimeValue4 __bp_offs_48_sz4_237;
	__bp_offs_48_sz4_237.addrs[0] = PackAddress(1,0,0,0);
// instr 56
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_236;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_237;
}
{
	if(!memcmp(static_cast<const UInt8 *>(bp) + (-48), static_cast<const UInt8 *>(bp) + (-40), 4))
		goto __func24_instr71;
// instr 57
}
__func24_instr57:
{
	RuntimeValue4 __bp_offs_20_sz4_238;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-20)));
		__bp_offs_20_sz4_238 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_40_sz4_239;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_20_sz4_238);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_239);
		*dest = *src;
	}
// instr 58
	RuntimeValue4 __bp_offs_48_sz4_240;
	__bp_offs_48_sz4_240.addrs[0] = PackAddress(2,0,0,0);
// instr 59
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_239;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_240;
}
{
	if(!memcmp(static_cast<const UInt8 *>(bp) + (-48), static_cast<const UInt8 *>(bp) + (-40), 4))
		goto __func24_instr63;
// instr 60
	RuntimeValue4 __bp_offs_20_sz4_241;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-20)));
		__bp_offs_20_sz4_241 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_40_sz4_242;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_20_sz4_241);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_242);
		*dest = *src;
	}
// instr 61
	RuntimeValue4 __bp_offs_48_sz4_243;
	__bp_offs_48_sz4_243.addrs[0] = PackAddress(3,0,0,0);
// instr 62
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_242;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_243;
}
{
	if(!memcmp(static_cast<const UInt8 *>(bp) + (-48), static_cast<const UInt8 *>(bp) + (-40), 4))
		goto __func24_instr74;
// instr 63
}
__func24_instr63:
{
	RuntimeValue4 __bp_offs_40_sz4_244;
	__bp_offs_40_sz4_244.addrs[0] = PackAddress(0,0,0,0);
// instr 64
	RuntimeValue4 __bp_offs_20_sz4_245;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_244);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_20_sz4_245);
		*dest = *src;
	}
// instr 65
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-20)))) = __bp_offs_20_sz4_245;
}
{
	goto __func24_instr76;
// instr 66
}
{
	INVALIDOPERATIONEXCEPTION(66);
// instr 67
}
__func24_instr67:
{
	RuntimeValue4 __bp_offs_40_sz4_246;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_246 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_16_sz4_247;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_246);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_16_sz4_247);
		*dest = *src;
	}
// instr 68
	RuntimeValue4 __bp_offs_40_sz4_248;
	__bp_offs_40_sz4_248.addrs[0] = PackAddress(255,255,255,255);
// instr 69
	RuntimeValue4 __bp_offs_20_sz4_249;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_248);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_20_sz4_249);
		*dest = *src;
	}
// instr 70
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_247;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-20)))) = __bp_offs_20_sz4_249;
}
{
	goto __func24_instr76;
// instr 71
}
__func24_instr71:
{
	RuntimeValue4 __bp_offs_40_sz4_250;
	__bp_offs_40_sz4_250.addrs[0] = PackAddress(1,0,0,0);
// instr 72
	RuntimeValue4 __bp_offs_20_sz4_251;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_250);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_20_sz4_251);
		*dest = *src;
	}
// instr 73
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-20)))) = __bp_offs_20_sz4_251;
}
{
	goto __func24_instr76;
// instr 74
}
__func24_instr74:
{
	RuntimeValue4 __bp_offs_40_sz4_252;
	__bp_offs_40_sz4_252.addrs[0] = PackAddress(3,0,0,0);
// instr 75
	RuntimeValue4 __bp_offs_20_sz4_253;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_252);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_20_sz4_253);
		*dest = *src;
	}
// instr 76
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-20)))) = __bp_offs_20_sz4_253;
}
__func24_instr76:
{
	RuntimeValue8 __bp_offs_40_sz8_254;
	__bp_offs_40_sz8_254.rtp.objectRef = &thread;
	__bp_offs_40_sz8_254.rtp.valueRef = (static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)));
// instr 77
	*reinterpret_cast<RuntimeValue8 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz8_254;
}
{
	TICK(77);
// instr 78
}
__func24_instr78:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[10]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 78, -48, -32, thread, instrTable, bp, providerDictionary);
	}
// instr 79
}
__func24_instr79:
{
	RuntimeValue4 __bp_offs_20_sz4_255;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-20)));
		__bp_offs_20_sz4_255 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_40_sz4_256;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_20_sz4_255);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_256);
		*dest = *src;
	}
// instr 80
	RuntimeValue4 __bp_offs_48_sz4_257;
	__bp_offs_48_sz4_257.addrs[0] = PackAddress(255,255,255,255);
// instr 81
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_256;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_257;
}
{
	if(memcmp(static_cast<const UInt8 *>(bp) + (-48), static_cast<const UInt8 *>(bp) + (-40), 4))
		goto __func24_instr84;
// instr 82
	RuntimeValue4 __bp_offs_16_sz4_258;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_258 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_40_sz4_259;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_258);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_259);
		*dest = *src;
	}
// instr 83
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_259;
	THROWEXCEPTION(__bp_offs_40_sz4_259.p, 83);
// instr 84
}
__func24_instr84:
{
	RuntimeValue4 __bp_offs_20_sz4_260;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-20)));
		__bp_offs_20_sz4_260 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_40_sz4_261;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_20_sz4_260);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_261);
		*dest = *src;
	}
// instr 85
	RuntimeValue4 __bp_offs_48_sz4_262;
	__bp_offs_48_sz4_262.addrs[0] = PackAddress(1,0,0,0);
// instr 86
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_261;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_262;
}
{
	if(memcmp(static_cast<const UInt8 *>(bp) + (-48), static_cast<const UInt8 *>(bp) + (-40), 4))
		goto __func24_instr89;
// instr 87
	RuntimeValue4 __bp_offs_24_sz4_263;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-24)));
		__bp_offs_24_sz4_263 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __prv_offs0_sz4_264;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_24_sz4_263);
		dest = reinterpret_cast<RuntimeValue4 *>(&__prv_offs0_sz4_264);
		*dest = *src;
	}
// instr 88
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(prv) + (0)))) = __prv_offs0_sz4_264;
}
{
	EXITFRAME;
// instr 89
}
__func24_instr89:
{
	RuntimeValue4 __bp_offs_20_sz4_265;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-20)));
		__bp_offs_20_sz4_265 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_40_sz4_266;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_20_sz4_265);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_266);
		*dest = *src;
	}
// instr 90
	RuntimeValue4 __bp_offs_48_sz4_267;
	__bp_offs_48_sz4_267.addrs[0] = PackAddress(2,0,0,0);
// instr 91
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_266;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_267;
}
{
	if(!memcmp(static_cast<const UInt8 *>(bp) + (-48), static_cast<const UInt8 *>(bp) + (-40), 4))
		goto __func24_instr96;
// instr 92
	RuntimeValue4 __bp_offs_20_sz4_268;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-20)));
		__bp_offs_20_sz4_268 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_40_sz4_269;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_20_sz4_268);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_269);
		*dest = *src;
	}
// instr 93
	RuntimeValue4 __bp_offs_48_sz4_270;
	__bp_offs_48_sz4_270.addrs[0] = PackAddress(3,0,0,0);
// instr 94
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_269;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_270;
}
{
	if(!memcmp(static_cast<const UInt8 *>(bp) + (-48), static_cast<const UInt8 *>(bp) + (-40), 4))
		goto __func24_instr103;
// instr 95
// rdxsrc/Apps/RegressionTest.rdx [line 344]
// instr 96
}
__func24_instr96:
{
	RuntimeValue4 __bp_offs_40_sz4_271;
	__bp_offs_40_sz4_271.cp = resArgs[11];
// instr 97
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_271;
}
{
	TICK(97);
// instr 98
}
__func24_instr98:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[12]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 98, -48, -32, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 99
}
__func24_instr99:
{
// rdxsrc/Apps/RegressionTest.rdx [line 337]
// instr 100
}
__func24_instr100:
{
	TICK(100);
// instr 101
}
__func24_instr101:
{
	goto __func24_instr5;
// instr 102
// rdxsrc/Apps/RegressionTest.rdx [line 347]
// instr 103
}
__func24_instr103:
{
	RuntimeValue4 __bp_offs_40_sz4_272;
	__bp_offs_40_sz4_272.addrs[0] = PackAddress(10,0,0,0);
// instr 104
	RuntimeValue4 __prv_offs0_sz4_273;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_272);
		dest = reinterpret_cast<RuntimeValue4 *>(&__prv_offs0_sz4_273);
		*dest = *src;
	}
// instr 105
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(prv) + (0)))) = __prv_offs0_sz4_273;
}
{
	EXITFRAME;
// instr 106
}
{
	INVALIDOPERATIONEXCEPTION(106);
}
__func24_start:
switch(startInstruction)
{
case 0: goto __func24_instr0;
case 5: goto __func24_instr5;
case 9: goto __func24_instr9;
case 19: goto __func24_instr19;
case 20: goto __func24_instr20;
case 28: goto __func24_instr28;
case 29: goto __func24_instr29;
case 39: goto __func24_instr39;
case 43: goto __func24_instr43;
case 45: goto __func24_instr45;
case 47: goto __func24_instr47;
case 48: goto __func24_instr48;
case 53: goto __func24_instr53;
case 57: goto __func24_instr57;
case 63: goto __func24_instr63;
case 67: goto __func24_instr67;
case 71: goto __func24_instr71;
case 74: goto __func24_instr74;
case 76: goto __func24_instr76;
case 78: goto __func24_instr78;
case 79: goto __func24_instr79;
case 84: goto __func24_instr84;
case 89: goto __func24_instr89;
case 96: goto __func24_instr96;
case 98: goto __func24_instr98;
case 99: goto __func24_instr99;
case 100: goto __func24_instr100;
case 101: goto __func24_instr101;
case 103: goto __func24_instr103;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestUnboundDelegates()
static int f25(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func25_start;
__func25_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 185]
// instr 1
	RuntimeValue4 __bp_offs_4_sz4_274;
	memset(&__bp_offs_4_sz4_274, 0, 4);
// instr 2
	RuntimeValue4 __bp_offs_16_sz4_275;
	__bp_offs_16_sz4_275.cp = resArgs[1];
// instr 3
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_275;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_274;
	NULLCHECK(__bp_offs_16_sz4_275.p, 3);
	if(!objm->ObjectCompatible(__bp_offs_16_sz4_275.p, static_cast<const Type *>(resArgs[0])))
		INCOMPATIBLECONVERSION(3);
// instr 4
	RuntimeValue4 __bp_offs_16_sz4_276;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_276 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_276);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_4_sz4_274);
		*dest = *src;
	}
// instr 5
// rdxsrc/Apps/RegressionTest.rdx [line 186]
// instr 6
	RuntimeValue4 __bp_offs_8_sz4_277;
	memset(&__bp_offs_8_sz4_277, 0, 4);
// instr 7
// rdxsrc/Apps/RegressionTest.rdx [line 187]
// instr 8
	RuntimeValue4 __bp_offs_24_sz4_278;
	__bp_offs_24_sz4_278.cp = resArgs[3];
// instr 9
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_278;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_274;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_277;
}
{
	TICK(9);
// instr 10
}
__func25_instr10:
{
	RuntimeValue4 __bp_offs_4_sz4_279;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-4)));
		__bp_offs_4_sz4_279 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const Method *invokedMethod = static_cast<const Method *>(__bp_offs_4_sz4_279.p);
		NULLCHECK(invokedMethod, 10);
		bool shouldContinue = false;
		int methodStatus = CallMethod(ctx, objm, invokedMethod, 10, -32, -16, thread, instrTable, bp, providerDictionary, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 11
}
__func25_instr11:
{
	RuntimeValue4 __bp_offs_16_sz4_280;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_280 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_8_sz4_281;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_280);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_281);
		*dest = *src;
	}
// instr 12
// rdxsrc/Apps/RegressionTest.rdx [line 188]
// instr 13
	RuntimeValue4 __bp_offs_16_sz4_282;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_281);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_16_sz4_282);
		*dest = *src;
	}
// instr 14
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_282;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_281;
}
{
	TICK(14);
// instr 15
}
__func25_instr15:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[4]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 15, -16, -8, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 16
}
__func25_instr16:
{
// rdxsrc/Apps/RegressionTest.rdx [line 185]
// instr 17
}
__func25_instr17:
{
	EXITFRAME;
// instr 18
}
{
	INVALIDOPERATIONEXCEPTION(18);
}
__func25_start:
switch(startInstruction)
{
case 0: goto __func25_instr0;
case 10: goto __func25_instr10;
case 11: goto __func25_instr11;
case 15: goto __func25_instr15;
case 16: goto __func25_instr16;
case 17: goto __func25_instr17;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestArrayIterator()
static int f26(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func26_start;
__func26_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 86]
// instr 1
	RuntimeValue4 __bp_offs_4_sz4_283;
	memset(&__bp_offs_4_sz4_283, 0, 4);
// instr 2
	RuntimeValue4 __bp_offs_8_sz4_284;
	memset(&__bp_offs_8_sz4_284, 0, 4);
// instr 3
	RuntimeValue4 __bp_offs_32_sz4_285;
	__bp_offs_32_sz4_285.addrs[0] = PackAddress(3,0,0,0);
// instr 4
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_285;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_283;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_284;
}
__func26_instr4:
{
	RuntimeValue4 __bp_offs_32_sz4_286;
	{
		t->frame.ip = instrTable + ((4) + 1);
		const Type *t = static_cast<const Type *>(resArgs[0]);
		void *obj = RDX_RuntimeUtilities_NewObjectInstance(ctx, objm, t, reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8 *>(bp) + (-32)), 1);
		NULLCHECK(obj, 4);
		__bp_offs_32_sz4_286.p = obj;
	}
// instr 5
	RuntimeValue4 __bp_offs_8_sz4_287;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_32_sz4_286);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_287);
		*dest = *src;
	}
// instr 6
	RuntimeValue4 __bp_offs_32_sz4_288;
	__bp_offs_32_sz4_288.cp = resArgs[2];
// instr 7
	RuntimeValue4 __bp_offs_40_sz4_289;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_287);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_289);
		*dest = *src;
	}
// instr 8
	RuntimeValue4 __bp_offs_48_sz4_290;
	__bp_offs_48_sz4_290.addrs[0] = PackAddress(0,0,0,0);
// instr 9
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_288;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_289;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_290;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_287;
	RuntimeValue8 __bp_offs_40_sz8_291;
	{
		void *dataLoc = RDX_RuntimeUtilities_ArrayIndex(__bp_offs_40_sz4_289.p, reinterpret_cast<const RuntimeStackValue *>((static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-48)))));
		if(dataLoc == NULL)
			OUTOFBOUNDS(9);
		__bp_offs_40_sz8_291.rtp.objectRef = __bp_offs_40_sz4_289.p;
		__bp_offs_40_sz8_291.rtp.valueRef = reinterpret_cast<UInt8 *>(dataLoc) + (0);
	}
// instr 10
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_32_sz4_288);
		const RuntimePointer<void> *destPtr = &__bp_offs_40_sz8_291.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 11
	RuntimeValue4 __bp_offs_32_sz4_292;
	__bp_offs_32_sz4_292.cp = resArgs[4];
// instr 12
	RuntimeValue4 __bp_offs_40_sz4_293;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_287);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_293);
		*dest = *src;
	}
// instr 13
	__bp_offs_48_sz4_290.addrs[0] = PackAddress(1,0,0,0);
// instr 14
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_292;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_293;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_290;
	RuntimeValue8 __bp_offs_40_sz8_294;
	{
		void *dataLoc = RDX_RuntimeUtilities_ArrayIndex(__bp_offs_40_sz4_293.p, reinterpret_cast<const RuntimeStackValue *>((static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-48)))));
		if(dataLoc == NULL)
			OUTOFBOUNDS(14);
		__bp_offs_40_sz8_294.rtp.objectRef = __bp_offs_40_sz4_293.p;
		__bp_offs_40_sz8_294.rtp.valueRef = reinterpret_cast<UInt8 *>(dataLoc) + (0);
	}
// instr 15
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_32_sz4_292);
		const RuntimePointer<void> *destPtr = &__bp_offs_40_sz8_294.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 16
	RuntimeValue4 __bp_offs_32_sz4_295;
	__bp_offs_32_sz4_295.cp = resArgs[5];
// instr 17
	RuntimeValue4 __bp_offs_40_sz4_296;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_287);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_296);
		*dest = *src;
	}
// instr 18
	__bp_offs_48_sz4_290.addrs[0] = PackAddress(2,0,0,0);
// instr 19
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_295;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_296;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_290;
	RuntimeValue8 __bp_offs_40_sz8_297;
	{
		void *dataLoc = RDX_RuntimeUtilities_ArrayIndex(__bp_offs_40_sz4_296.p, reinterpret_cast<const RuntimeStackValue *>((static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-48)))));
		if(dataLoc == NULL)
			OUTOFBOUNDS(19);
		__bp_offs_40_sz8_297.rtp.objectRef = __bp_offs_40_sz4_296.p;
		__bp_offs_40_sz8_297.rtp.valueRef = reinterpret_cast<UInt8 *>(dataLoc) + (0);
	}
// instr 20
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_32_sz4_295);
		const RuntimePointer<void> *destPtr = &__bp_offs_40_sz8_297.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 21
	RuntimeValue4 __bp_offs_4_sz4_298;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_287);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_4_sz4_298);
		*dest = *src;
	}
// instr 22
// rdxsrc/Apps/RegressionTest.rdx [line 87]
// instr 23
	RuntimeValue4 __bp_offs_32_sz4_299;
	__bp_offs_32_sz4_299.cp = resArgs[6];
// instr 24
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_299;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_298;
}
{
	TICK(24);
// instr 25
}
__func26_instr25:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[7]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 25, -32, -24, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 26
}
__func26_instr26:
{
// rdxsrc/Apps/RegressionTest.rdx [line 88]
// instr 27
}
__func26_instr27:
{
	RuntimeValue4 __bp_offs_32_sz4_300;
	__bp_offs_32_sz4_300.addrs[0] = PackAddress(255,255,255,255);
// instr 28
	RuntimeValue4 __bp_offs_8_sz4_301;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_32_sz4_300);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_301);
		*dest = *src;
	}
// instr 29
	RuntimeValue4 __bp_offs_12_sz4_302;
	memset(&__bp_offs_12_sz4_302, 0, 4);
// instr 30
	RuntimeValue4 __bp_offs_16_sz4_303;
	memset(&__bp_offs_16_sz4_303, 0, 4);
// instr 31
	RuntimeValue4 __bp_offs_4_sz4_304;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-4)));
		__bp_offs_4_sz4_304 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_304);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_16_sz4_303);
		*dest = *src;
	}
// instr 32
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)))) = __bp_offs_12_sz4_302;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_303;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_301;
}
__func26_instr32:
{
	RuntimeValue4 __bp_offs_16_sz4_305;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_305 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_8_sz4_306;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-8)));
		__bp_offs_8_sz4_306 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_12_sz4_307;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-12)));
		__bp_offs_12_sz4_307 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *arrayRef = static_cast<const RuntimeValue4 *>(__bp_offs_16_sz4_305.p);
		NULLCHECK(__bp_offs_16_sz4_305.p, 32);
		LargeInt numElements = GCInfo::From(__bp_offs_16_sz4_305.p)->numElements;
		LargeInt nextIndex = (__bp_offs_8_sz4_306.li) + 1;
		if(nextIndex < 0 || nextIndex >= numElements)
			goto __func26_instr41;
		__bp_offs_8_sz4_306.li = nextIndex;
		__bp_offs_12_sz4_307 = arrayRef[nextIndex];
	}
// instr 33
// rdxsrc/Apps/RegressionTest.rdx [line 89]
// instr 34
	RuntimeValue4 __bp_offs_32_sz4_308;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_12_sz4_307);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_32_sz4_308);
		*dest = *src;
	}
// instr 35
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)))) = __bp_offs_12_sz4_307;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_308;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_306;
}
{
	TICK(35);
// instr 36
}
__func26_instr36:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[7]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 36, -32, -24, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 37
}
__func26_instr37:
{
// rdxsrc/Apps/RegressionTest.rdx [line 88]
// instr 38
}
__func26_instr38:
{
	TICK(38);
// instr 39
}
__func26_instr39:
{
	goto __func26_instr32;
// instr 40
// rdxsrc/Apps/RegressionTest.rdx [line 90]
// instr 41
}
__func26_instr41:
{
	RuntimeValue4 __bp_offs_32_sz4_309;
	__bp_offs_32_sz4_309.addrs[0] = PackAddress(255,255,255,255);
// instr 42
	RuntimeValue4 __bp_offs_8_sz4_310;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_32_sz4_309);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_310);
		*dest = *src;
	}
// instr 43
	RuntimeValue4 __bp_offs_12_sz4_311;
	memset(&__bp_offs_12_sz4_311, 0, 4);
// instr 44
	RuntimeValue4 __bp_offs_16_sz4_312;
	memset(&__bp_offs_16_sz4_312, 0, 4);
// instr 45
	RuntimeValue4 __bp_offs_20_sz4_313;
	memset(&__bp_offs_20_sz4_313, 0, 4);
// instr 46
	RuntimeValue4 __bp_offs_24_sz4_314;
	memset(&__bp_offs_24_sz4_314, 0, 4);
// instr 47
	RuntimeValue4 __bp_offs_4_sz4_315;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-4)));
		__bp_offs_4_sz4_315 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_315);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_24_sz4_314);
		*dest = *src;
	}
// instr 48
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)))) = __bp_offs_12_sz4_311;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_312;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-20)))) = __bp_offs_20_sz4_313;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_314;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_310;
}
__func26_instr48:
{
	RuntimeValue4 __bp_offs_24_sz4_316;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-24)));
		__bp_offs_24_sz4_316 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_8_sz4_317;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-8)));
		__bp_offs_8_sz4_317 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_20_sz4_318;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-20)));
		__bp_offs_20_sz4_318 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *arrayRef = static_cast<const RuntimeValue4 *>(__bp_offs_24_sz4_316.p);
		NULLCHECK(__bp_offs_24_sz4_316.p, 48);
		LargeInt numElements = GCInfo::From(__bp_offs_24_sz4_316.p)->numElements;
		LargeInt nextIndex = (__bp_offs_8_sz4_317.li) + 1;
		if(nextIndex < 0 || nextIndex >= numElements)
			goto __func26_instr65;
		__bp_offs_8_sz4_317.li = nextIndex;
		__bp_offs_20_sz4_318 = arrayRef[nextIndex];
	}
// instr 49
	RuntimeValue4 __bp_offs_40_sz4_319;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_317);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_319);
		*dest = *src;
	}
// instr 50
	RuntimeValue4 __bp_offs_32_sz4_320;
	__bp_offs_32_sz4_320.i32 = static_cast<Int32>(		__bp_offs_40_sz4_319.i32);
// instr 51
	RuntimeValue4 __bp_offs_16_sz4_321;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_32_sz4_320);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_16_sz4_321);
		*dest = *src;
	}
// instr 52
	RuntimeValue4 __bp_offs_12_sz4_322;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_20_sz4_318);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_12_sz4_322);
		*dest = *src;
	}
// instr 53
// rdxsrc/Apps/RegressionTest.rdx [line 92]
// instr 54
	RuntimeValue4 __bp_offs_32_sz4_323;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_321);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_32_sz4_323);
		*dest = *src;
	}
// instr 55
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)))) = __bp_offs_12_sz4_322;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_321;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-20)))) = __bp_offs_20_sz4_318;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_323;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_317;
}
{
	TICK(55);
// instr 56
}
__func26_instr56:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[10]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 56, -32, -24, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 57
}
__func26_instr57:
{
// rdxsrc/Apps/RegressionTest.rdx [line 93]
// instr 58
}
__func26_instr58:
{
	RuntimeValue4 __bp_offs_12_sz4_324;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-12)));
		__bp_offs_12_sz4_324 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_32_sz4_325;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_12_sz4_324);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_32_sz4_325);
		*dest = *src;
	}
// instr 59
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_325;
}
{
	TICK(59);
// instr 60
}
__func26_instr60:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[7]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 60, -32, -24, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 61
}
__func26_instr61:
{
// rdxsrc/Apps/RegressionTest.rdx [line 90]
// instr 62
}
__func26_instr62:
{
	TICK(62);
// instr 63
}
__func26_instr63:
{
	goto __func26_instr48;
// instr 64
// rdxsrc/Apps/RegressionTest.rdx [line 86]
// instr 65
}
__func26_instr65:
{
	EXITFRAME;
// instr 66
}
{
	INVALIDOPERATIONEXCEPTION(66);
}
__func26_start:
switch(startInstruction)
{
case 0: goto __func26_instr0;
case 4: goto __func26_instr4;
case 25: goto __func26_instr25;
case 26: goto __func26_instr26;
case 27: goto __func26_instr27;
case 32: goto __func26_instr32;
case 36: goto __func26_instr36;
case 37: goto __func26_instr37;
case 38: goto __func26_instr38;
case 39: goto __func26_instr39;
case 41: goto __func26_instr41;
case 48: goto __func26_instr48;
case 56: goto __func26_instr56;
case 57: goto __func26_instr57;
case 58: goto __func26_instr58;
case 60: goto __func26_instr60;
case 61: goto __func26_instr61;
case 62: goto __func26_instr62;
case 63: goto __func26_instr63;
case 65: goto __func26_instr65;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestUnboundDelegateFunc(Core.string)
static int f27(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func27_start;
__func27_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 179]
// instr 1
	RuntimeValue4 __prv_offs_8_sz4_326;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-8)));
		__prv_offs_8_sz4_326 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_8_sz4_327;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_8_sz4_326);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_327);
		*dest = *src;
	}
// instr 2
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_327;
}
{
	TICK(2);
// instr 3
}
__func27_instr3:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[0]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 3, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 4
}
__func27_instr4:
{
// rdxsrc/Apps/RegressionTest.rdx [line 180]
// instr 5
}
__func27_instr5:
{
	RuntimeValue4 __bp_offs_8_sz4_328;
	__bp_offs_8_sz4_328.cp = resArgs[1];
// instr 6
	RuntimeValue4 __prv_offs0_sz4_329;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_328);
		dest = reinterpret_cast<RuntimeValue4 *>(&__prv_offs0_sz4_329);
		*dest = *src;
	}
// instr 7
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(prv) + (0)))) = __prv_offs0_sz4_329;
}
{
	EXITFRAME;
// instr 8
}
{
	INVALIDOPERATIONEXCEPTION(8);
}
__func27_start:
switch(startInstruction)
{
case 0: goto __func27_instr0;
case 3: goto __func27_instr3;
case 4: goto __func27_instr4;
case 5: goto __func27_instr5;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestBoundDelegates()
static int f28(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func28_start;
__func28_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 216]
// instr 1
	RuntimeValue4 __bp_offs_40_sz4_330;
	__bp_offs_40_sz4_330.cp = resArgs[0];
// instr 2
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_330;
}
{
	TICK(2);
// instr 3
}
__func28_instr3:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[2]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 3, -48, -32, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 4
}
__func28_instr4:
{
// rdxsrc/Apps/RegressionTest.rdx [line 218]
// instr 5
}
__func28_instr5:
{
	RuntimeValue4 __bp_offs_4_sz4_331;
	memset(&__bp_offs_4_sz4_331, 0, 4);
// instr 6
// rdxsrc/Apps/RegressionTest.rdx [line 219]
// instr 7
	RuntimeValue4 __bp_offs_40_sz4_332;
	__bp_offs_40_sz4_332.cp = resArgs[4];
// instr 8
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_332;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_331;
}
{
	RuntimeValue8 __bp_offs_48_sz8_333;
	__bp_offs_48_sz8_333.rtp.objectRef = &thread;
	__bp_offs_48_sz8_333.rtp.valueRef = (static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)));
// instr 9
	RuntimeValue4 __bp_offs_40_sz4_334;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_334 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_334);
		const RuntimePointer<void> *destPtr = &__bp_offs_48_sz8_333.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 10
// rdxsrc/Apps/RegressionTest.rdx [line 220]
// instr 11
	RuntimeValue4 __bp_offs_8_sz4_335;
	memset(&__bp_offs_8_sz4_335, 0, 4);
// instr 12
	RuntimeValue4 __bp_offs_12_sz4_336;
	memset(&__bp_offs_12_sz4_336, 0, 4);
// instr 13
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)))) = __bp_offs_12_sz4_336;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_335;
}
{
	RuntimeValue8 __bp_offs_40_sz8_337;
	__bp_offs_40_sz8_337.rtp.objectRef = &thread;
	__bp_offs_40_sz8_337.rtp.valueRef = (static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)));
// instr 14
	*reinterpret_cast<RuntimeValue8 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz8_337;
}
__func28_instr14:
{
	RuntimeValue4 __bp_offs_12_sz4_338;
	{
		t->frame.ip = instrTable + ((14) + 1);
		const Type *t = static_cast<const Type *>(resArgs[6]);
		void *obj = RDX_RuntimeUtilities_NewObjectInstance(ctx, objm, t, reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8 *>(bp) + (0)), 0);
		NULLCHECK(obj, 14);
		__bp_offs_12_sz4_338.p = obj;
	}
// instr 15
	RuntimeValue8 __bp_offs_48_sz8_339;
	__bp_offs_48_sz8_339.rtp.objectRef = __bp_offs_12_sz4_338.p;
	__bp_offs_48_sz8_339.rtp.valueRef = reinterpret_cast<UInt8 *>(__bp_offs_12_sz4_338.p) + (0);
// instr 16
	RuntimeValue8 __bp_offs_40_sz8_340;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz8_340 = *reinterpret_cast<const RuntimeValue8 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		const RuntimePointer<void> *srcPtr = &__bp_offs_40_sz8_340.rtp;
		src = static_cast<const RuntimeValue4 *>(srcPtr->valueRef);
		const RuntimePointer<void> *destPtr = &__bp_offs_48_sz8_339.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 17
	RuntimeValue4 __bp_offs_8_sz4_341;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_12_sz4_338);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_341);
		*dest = *src;
	}
// instr 18
// rdxsrc/Apps/RegressionTest.rdx [line 221]
// instr 19
	RuntimeValue4 __bp_offs_40_sz4_342;
	__bp_offs_40_sz4_342.cp = resArgs[7];
// instr 20
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)))) = __bp_offs_12_sz4_338;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_342;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_341;
}
{
	RuntimeValue8 __bp_offs_48_sz8_343;
	__bp_offs_48_sz8_343.rtp.objectRef = &thread;
	__bp_offs_48_sz8_343.rtp.valueRef = (static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)));
// instr 21
	RuntimeValue4 __bp_offs_40_sz4_344;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_344 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_344);
		const RuntimePointer<void> *destPtr = &__bp_offs_48_sz8_343.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 22
// rdxsrc/Apps/RegressionTest.rdx [line 222]
// instr 23
	RuntimeValue4 __bp_offs_8_sz4_345;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-8)));
		__bp_offs_8_sz4_345 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_40_sz4_346;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_345);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_346);
		*dest = *src;
	}
// instr 24
	RuntimeValue4 __bp_offs_48_sz4_347;
	__bp_offs_48_sz4_347.cp = resArgs[8];
// instr 25
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_346;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_347;
}
{
	TICK(25);
// instr 26
}
__func28_instr26:
{
	RuntimeValue4 __bp_offs_40_sz4_348;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_348 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_40_sz4_348.p, 26);
// instr 27
}
__func28_instr27:
{
	RuntimeValue4 __bp_offs_40_sz4_349;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_349 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_40_sz4_349.p, 27);
	{
		const Method *invokedMethod = static_cast<const StructuredType *>(GCInfo::From(__bp_offs_40_sz4_349.p)->containerType)->virtualMethods[0];
		bool shouldContinue = false;
		int methodStatus = CallMethod(ctx, objm, invokedMethod, 27, -48, -32, thread, instrTable, bp, providerDictionary, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 28
}
__func28_instr28:
{
// rdxsrc/Apps/RegressionTest.rdx [line 223]
// instr 29
}
__func28_instr29:
{
	RuntimeValue4 __bp_offs_40_sz4_350;
	{
		t->frame.ip = instrTable + ((29) + 1);
		const Type *t = static_cast<const Type *>(resArgs[10]);
		void *obj = RDX_RuntimeUtilities_NewObjectInstance(ctx, objm, t, reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8 *>(bp) + (0)), 0);
		NULLCHECK(obj, 29);
		__bp_offs_40_sz4_350.p = obj;
	}
// instr 30
	RuntimeValue4 __bp_offs_8_sz4_351;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_350);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_351);
		*dest = *src;
	}
// instr 31
// rdxsrc/Apps/RegressionTest.rdx [line 224]
// instr 32
	RuntimeValue4 __bp_offs_40_sz4_352;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_351);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_352);
		*dest = *src;
	}
// instr 33
	RuntimeValue4 __bp_offs_48_sz4_353;
	__bp_offs_48_sz4_353.cp = resArgs[11];
// instr 34
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_352;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_353;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_351;
}
{
	TICK(34);
// instr 35
}
__func28_instr35:
{
	RuntimeValue4 __bp_offs_40_sz4_354;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_354 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_40_sz4_354.p, 35);
// instr 36
}
__func28_instr36:
{
	RuntimeValue4 __bp_offs_40_sz4_355;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_355 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_40_sz4_355.p, 36);
	{
		const Method *invokedMethod = static_cast<const StructuredType *>(GCInfo::From(__bp_offs_40_sz4_355.p)->containerType)->virtualMethods[0];
		bool shouldContinue = false;
		int methodStatus = CallMethod(ctx, objm, invokedMethod, 36, -48, -32, thread, instrTable, bp, providerDictionary, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 37
}
__func28_instr37:
{
// rdxsrc/Apps/RegressionTest.rdx [line 225]
// instr 38
}
__func28_instr38:
{
	RuntimeValue4 __bp_offs_12_sz4_356;
	memset(&__bp_offs_12_sz4_356, 0, 4);
// instr 39
	RuntimeValue4 __bp_offs_40_sz4_357;
	__bp_offs_40_sz4_357.cp = resArgs[12];
// instr 40
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)))) = __bp_offs_12_sz4_356;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_357;
	NULLCHECK(__bp_offs_40_sz4_357.p, 40);
	if(!objm->ObjectCompatible(__bp_offs_40_sz4_357.p, static_cast<const Type *>(resArgs[13])))
		INCOMPATIBLECONVERSION(40);
// instr 41
}
__func28_instr41:
{
	RuntimeValue4 __bp_offs_12_sz4_358;
	{
		t->frame.ip = instrTable + ((41) + 1);
		const Type *t = static_cast<const Type *>(resArgs[14]);
		void *obj = RDX_RuntimeUtilities_NewObjectInstance(ctx, objm, t, reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8 *>(bp) + (0)), 0);
		NULLCHECK(obj, 41);
		__bp_offs_12_sz4_358.p = obj;
	}
// instr 42
	RuntimeValue8 __bp_offs_48_sz8_359;
	__bp_offs_48_sz8_359.rtp.objectRef = __bp_offs_12_sz4_358.p;
	__bp_offs_48_sz8_359.rtp.valueRef = reinterpret_cast<UInt8 *>(__bp_offs_12_sz4_358.p) + (0);
// instr 43
	RuntimeValue4 __bp_offs_40_sz4_360;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_360 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_360);
		const RuntimePointer<void> *destPtr = &__bp_offs_48_sz8_359.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 44
	RuntimeValue4 __bp_offs_8_sz4_361;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_12_sz4_358);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_361);
		*dest = *src;
	}
// instr 45
// rdxsrc/Apps/RegressionTest.rdx [line 226]
// instr 46
	RuntimeValue4 __bp_offs_40_sz4_362;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_8_sz4_361);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_362);
		*dest = *src;
	}
// instr 47
	RuntimeValue4 __bp_offs_48_sz4_363;
	__bp_offs_48_sz4_363.cp = resArgs[15];
// instr 48
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)))) = __bp_offs_12_sz4_358;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_362;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_363;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_361;
}
{
	TICK(48);
// instr 49
}
__func28_instr49:
{
	RuntimeValue4 __bp_offs_40_sz4_364;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_364 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_40_sz4_364.p, 49);
// instr 50
}
__func28_instr50:
{
	RuntimeValue4 __bp_offs_40_sz4_365;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_365 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_40_sz4_365.p, 50);
	{
		const Method *invokedMethod = static_cast<const StructuredType *>(GCInfo::From(__bp_offs_40_sz4_365.p)->containerType)->virtualMethods[0];
		bool shouldContinue = false;
		int methodStatus = CallMethod(ctx, objm, invokedMethod, 50, -48, -32, thread, instrTable, bp, providerDictionary, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 51
}
__func28_instr51:
{
// rdxsrc/Apps/RegressionTest.rdx [line 228]
// instr 52
}
__func28_instr52:
{
	RuntimeValue4 __bp_offs_12_sz4_366;
	memset(&__bp_offs_12_sz4_366, 0, 4);
// instr 53
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)))) = __bp_offs_12_sz4_366;
}
__func28_instr53:
{
	RuntimeValue4 __bp_offs_40_sz4_367;
	{
		t->frame.ip = instrTable + ((53) + 1);
		const Type *t = static_cast<const Type *>(resArgs[16]);
		void *obj = RDX_RuntimeUtilities_NewObjectInstance(ctx, objm, t, reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8 *>(bp) + (0)), 0);
		NULLCHECK(obj, 53);
		__bp_offs_40_sz4_367.p = obj;
	}
// instr 54
	RuntimeValue4 __bp_offs_12_sz4_368;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_367);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_12_sz4_368);
		*dest = *src;
	}
// instr 55
// rdxsrc/Apps/RegressionTest.rdx [line 229]
// instr 56
	RuntimeValue4 __bp_offs_40_sz4_369;
	__bp_offs_40_sz4_369.cp = resArgs[17];
// instr 57
	RuntimeValue4 __bp_offs_48_sz4_370;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_12_sz4_368);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_48_sz4_370);
		*dest = *src;
	}
// instr 58
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)))) = __bp_offs_12_sz4_368;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_369;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_370;
}
{
	RuntimeValue4 __bp_offs_48_sz4_371;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-48)));
		__bp_offs_48_sz4_371 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue8 __bp_offs_48_sz8_372;
	NULLCHECK(__bp_offs_48_sz4_371.p, 58);
	__bp_offs_48_sz8_372.rtp.objectRef = __bp_offs_48_sz4_371.p;
	__bp_offs_48_sz8_372.rtp.valueRef = reinterpret_cast<UInt8 *>(__bp_offs_48_sz4_371.p) + (0);
// instr 59
	RuntimeValue4 __bp_offs_40_sz4_373;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_373 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_373);
		const RuntimePointer<void> *destPtr = &__bp_offs_48_sz8_372.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 60
// rdxsrc/Apps/RegressionTest.rdx [line 230]
// instr 61
	RuntimeValue4 __bp_offs_16_sz4_374;
	memset(&__bp_offs_16_sz4_374, 0, 4);
// instr 62
	RuntimeValue4 __bp_offs_20_sz4_375;
	memset(&__bp_offs_20_sz4_375, 0, 4);
// instr 63
	RuntimeValue4 __bp_offs_12_sz4_376;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-12)));
		__bp_offs_12_sz4_376 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_40_sz4_377;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_12_sz4_376);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_377);
		*dest = *src;
	}
// instr 64
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_374;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-20)))) = __bp_offs_20_sz4_375;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_377;
}
__func28_instr64:
{
	RuntimeValue4 __bp_offs_20_sz4_378;
	{
		t->frame.ip = instrTable + ((64) + 1);
		const Type *t = static_cast<const Type *>(resArgs[19]);
		void *obj = RDX_RuntimeUtilities_NewObjectInstance(ctx, objm, t, reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8 *>(bp) + (0)), 0);
		NULLCHECK(obj, 64);
		__bp_offs_20_sz4_378.p = obj;
	}
// instr 65
	RuntimeValue8 __bp_offs_48_sz8_379;
	__bp_offs_48_sz8_379.rtp.objectRef = __bp_offs_20_sz4_378.p;
	__bp_offs_48_sz8_379.rtp.valueRef = reinterpret_cast<UInt8 *>(__bp_offs_20_sz4_378.p) + (0);
// instr 66
	RuntimeValue4 __bp_offs_40_sz4_380;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_380 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_380);
		const RuntimePointer<void> *destPtr = &__bp_offs_48_sz8_379.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 67
	RuntimeValue4 __bp_offs_16_sz4_381;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_20_sz4_378);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_16_sz4_381);
		*dest = *src;
	}
// instr 68
// rdxsrc/Apps/RegressionTest.rdx [line 231]
// instr 69
	RuntimeValue4 __bp_offs_40_sz4_382;
	__bp_offs_40_sz4_382.cp = resArgs[20];
// instr 70
	RuntimeValue4 __bp_offs_12_sz4_383;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-12)));
		__bp_offs_12_sz4_383 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_48_sz4_384;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_12_sz4_383);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_48_sz4_384);
		*dest = *src;
	}
// instr 71
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_381;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-20)))) = __bp_offs_20_sz4_378;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_382;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_384;
}
{
	RuntimeValue4 __bp_offs_48_sz4_385;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-48)));
		__bp_offs_48_sz4_385 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue8 __bp_offs_48_sz8_386;
	NULLCHECK(__bp_offs_48_sz4_385.p, 71);
	__bp_offs_48_sz8_386.rtp.objectRef = __bp_offs_48_sz4_385.p;
	__bp_offs_48_sz8_386.rtp.valueRef = reinterpret_cast<UInt8 *>(__bp_offs_48_sz4_385.p) + (0);
// instr 72
	RuntimeValue4 __bp_offs_40_sz4_387;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_387 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_387);
		const RuntimePointer<void> *destPtr = &__bp_offs_48_sz8_386.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 73
// rdxsrc/Apps/RegressionTest.rdx [line 232]
// instr 74
	RuntimeValue4 __bp_offs_16_sz4_388;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_388 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_40_sz4_389;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_388);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_389);
		*dest = *src;
	}
// instr 75
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_389;
}
{
	TICK(75);
// instr 76
}
__func28_instr76:
{
	RuntimeValue4 __bp_offs_40_sz4_390;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_390 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_40_sz4_390.p, 76);
// instr 77
}
__func28_instr77:
{
	RuntimeValue4 __bp_offs_40_sz4_391;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_391 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_40_sz4_391.p, 77);
	{
		const Method *invokedMethod = static_cast<const StructuredType *>(GCInfo::From(__bp_offs_40_sz4_391.p)->containerType)->virtualMethods[0];
		bool shouldContinue = false;
		int methodStatus = CallMethod(ctx, objm, invokedMethod, 77, -48, -32, thread, instrTable, bp, providerDictionary, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 78
}
__func28_instr78:
{
// rdxsrc/Apps/RegressionTest.rdx [line 234]
// instr 79
}
__func28_instr79:
{
	RuntimeValue4 __bp_offs_20_sz4_392;
	memset(&__bp_offs_20_sz4_392, 0, 4);
// instr 80
	RuntimeValue4 __bp_offs_12_sz4_393;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-12)));
		__bp_offs_12_sz4_393 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_12_sz4_393);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_20_sz4_392);
		*dest = *src;
	}
// instr 81
// rdxsrc/Apps/RegressionTest.rdx [line 235]
// instr 82
	RuntimeValue4 __bp_offs_24_sz4_394;
	memset(&__bp_offs_24_sz4_394, 0, 4);
// instr 83
	RuntimeValue4 __bp_offs_48_sz4_395;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_20_sz4_392);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_48_sz4_395);
		*dest = *src;
	}
// instr 84
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-20)))) = __bp_offs_20_sz4_392;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_394;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_395;
}
{
	TICK(84);
// instr 85
}
__func28_instr85:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[23]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 85, -48, -40, thread, instrTable, bp, providerDictionary);
	}
// instr 86
}
__func28_instr86:
{
	RuntimeValue4 __bp_offs_40_sz4_396;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_396 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_24_sz4_397;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_396);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_24_sz4_397);
		*dest = *src;
	}
// instr 87
// rdxsrc/Apps/RegressionTest.rdx [line 236]
// instr 88
	RuntimeValue4 __bp_offs_40_sz4_398;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_24_sz4_397);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_398);
		*dest = *src;
	}
// instr 89
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_397;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_398;
}
{
	TICK(89);
// instr 90
}
__func28_instr90:
{
	RuntimeValue4 __bp_offs_40_sz4_399;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_399 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_40_sz4_399.p, 90);
// instr 91
}
__func28_instr91:
{
	RuntimeValue4 __bp_offs_40_sz4_400;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_400 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_40_sz4_400.p, 91);
	{
		const Method *invokedMethod = static_cast<const StructuredType *>(GCInfo::From(__bp_offs_40_sz4_400.p)->containerType)->virtualMethods[0];
		bool shouldContinue = false;
		int methodStatus = CallMethod(ctx, objm, invokedMethod, 91, -48, -32, thread, instrTable, bp, providerDictionary, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 92
}
__func28_instr92:
{
// rdxsrc/Apps/RegressionTest.rdx [line 237]
// instr 93
}
__func28_instr93:
{
	RuntimeValue4 __bp_offs_28_sz4_401;
	memset(&__bp_offs_28_sz4_401, 0, 4);
// instr 94
	RuntimeValue4 __bp_offs_12_sz4_402;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-12)));
		__bp_offs_12_sz4_402 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_40_sz4_403;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_12_sz4_402);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_403);
		*dest = *src;
	}
// instr 95
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-28)))) = __bp_offs_28_sz4_401;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_403;
}
__func28_instr95:
{
	RuntimeValue4 __bp_offs_28_sz4_404;
	{
		t->frame.ip = instrTable + ((95) + 1);
		const Type *t = static_cast<const Type *>(resArgs[19]);
		void *obj = RDX_RuntimeUtilities_NewObjectInstance(ctx, objm, t, reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8 *>(bp) + (0)), 0);
		NULLCHECK(obj, 95);
		__bp_offs_28_sz4_404.p = obj;
	}
// instr 96
	RuntimeValue8 __bp_offs_48_sz8_405;
	__bp_offs_48_sz8_405.rtp.objectRef = __bp_offs_28_sz4_404.p;
	__bp_offs_48_sz8_405.rtp.valueRef = reinterpret_cast<UInt8 *>(__bp_offs_28_sz4_404.p) + (0);
// instr 97
	RuntimeValue4 __bp_offs_40_sz4_406;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_406 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_40_sz4_406);
		const RuntimePointer<void> *destPtr = &__bp_offs_48_sz8_405.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 98
	RuntimeValue4 __bp_offs_24_sz4_407;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_28_sz4_404);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_24_sz4_407);
		*dest = *src;
	}
// instr 99
// rdxsrc/Apps/RegressionTest.rdx [line 238]
// instr 100
	RuntimeValue4 __bp_offs_40_sz4_408;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_24_sz4_407);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_40_sz4_408);
		*dest = *src;
	}
// instr 101
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_407;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-28)))) = __bp_offs_28_sz4_404;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-40)))) = __bp_offs_40_sz4_408;
}
{
	TICK(101);
// instr 102
}
__func28_instr102:
{
	RuntimeValue4 __bp_offs_40_sz4_409;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_409 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_40_sz4_409.p, 102);
// instr 103
}
__func28_instr103:
{
	RuntimeValue4 __bp_offs_40_sz4_410;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-40)));
		__bp_offs_40_sz4_410 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_40_sz4_410.p, 103);
	{
		const Method *invokedMethod = static_cast<const StructuredType *>(GCInfo::From(__bp_offs_40_sz4_410.p)->containerType)->virtualMethods[0];
		bool shouldContinue = false;
		int methodStatus = CallMethod(ctx, objm, invokedMethod, 103, -48, -32, thread, instrTable, bp, providerDictionary, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 104
}
__func28_instr104:
{
// rdxsrc/Apps/RegressionTest.rdx [line 216]
// instr 105
}
__func28_instr105:
{
	EXITFRAME;
// instr 106
}
{
	INVALIDOPERATIONEXCEPTION(106);
}
__func28_start:
switch(startInstruction)
{
case 0: goto __func28_instr0;
case 3: goto __func28_instr3;
case 4: goto __func28_instr4;
case 5: goto __func28_instr5;
case 14: goto __func28_instr14;
case 26: goto __func28_instr26;
case 27: goto __func28_instr27;
case 28: goto __func28_instr28;
case 29: goto __func28_instr29;
case 35: goto __func28_instr35;
case 36: goto __func28_instr36;
case 37: goto __func28_instr37;
case 38: goto __func28_instr38;
case 41: goto __func28_instr41;
case 49: goto __func28_instr49;
case 50: goto __func28_instr50;
case 51: goto __func28_instr51;
case 52: goto __func28_instr52;
case 53: goto __func28_instr53;
case 64: goto __func28_instr64;
case 76: goto __func28_instr76;
case 77: goto __func28_instr77;
case 78: goto __func28_instr78;
case 79: goto __func28_instr79;
case 85: goto __func28_instr85;
case 86: goto __func28_instr86;
case 90: goto __func28_instr90;
case 91: goto __func28_instr91;
case 92: goto __func28_instr92;
case 93: goto __func28_instr93;
case 95: goto __func28_instr95;
case 102: goto __func28_instr102;
case 103: goto __func28_instr103;
case 104: goto __func28_instr104;
case 105: goto __func28_instr105;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestReentryFunc(Core.string,Core.string)
static int f29(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func29_start;
__func29_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 169]
// instr 1
	RuntimeValue4 __prv_offs_8_sz4_411;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-8)));
		__prv_offs_8_sz4_411 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_8_sz4_412;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_8_sz4_411);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_412);
		*dest = *src;
	}
// instr 2
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_412;
}
{
	TICK(2);
// instr 3
}
__func29_instr3:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[0]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 3, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 4
}
__func29_instr4:
{
// rdxsrc/Apps/RegressionTest.rdx [line 170]
// instr 5
}
__func29_instr5:
{
	RuntimeValue4 __prv_offs_16_sz4_413;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-16)));
		__prv_offs_16_sz4_413 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_8_sz4_414;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_16_sz4_413);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_414);
		*dest = *src;
	}
// instr 6
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_414;
}
{
	TICK(6);
// instr 7
}
__func29_instr7:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[0]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 7, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 8
}
__func29_instr8:
{
// rdxsrc/Apps/RegressionTest.rdx [line 171]
// instr 9
}
__func29_instr9:
{
	RuntimeValue4 __prv_offs_16_sz4_415;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-16)));
		__prv_offs_16_sz4_415 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_8_sz4_416;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_16_sz4_415);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_416);
		*dest = *src;
	}
// instr 10
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_416;
}
{
	TICK(10);
// instr 11
}
__func29_instr11:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[1]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 11, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 12
}
__func29_instr12:
{
// rdxsrc/Apps/RegressionTest.rdx [line 172]
// instr 13
}
__func29_instr13:
{
	TICK(13);
// instr 14
}
__func29_instr14:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[2]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 14, 0, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 15
}
__func29_instr15:
{
// rdxsrc/Apps/RegressionTest.rdx [line 173]
// instr 16
}
__func29_instr16:
{
	RuntimeValue4 __bp_offs_8_sz4_417;
	{
		t->frame.ip = instrTable + ((16) + 1);
		const Type *t = static_cast<const Type *>(resArgs[3]);
		void *obj = RDX_RuntimeUtilities_NewObjectInstance(ctx, objm, t, reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8 *>(bp) + (0)), 0);
		NULLCHECK(obj, 16);
		__bp_offs_8_sz4_417.p = obj;
	}
// instr 17
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_417;
	THROWEXCEPTION(__bp_offs_8_sz4_417.p, 17);
// instr 18
}
{
	INVALIDOPERATIONEXCEPTION(18);
}
__func29_start:
switch(startInstruction)
{
case 0: goto __func29_instr0;
case 3: goto __func29_instr3;
case 4: goto __func29_instr4;
case 5: goto __func29_instr5;
case 7: goto __func29_instr7;
case 8: goto __func29_instr8;
case 9: goto __func29_instr9;
case 11: goto __func29_instr11;
case 12: goto __func29_instr12;
case 13: goto __func29_instr13;
case 14: goto __func29_instr14;
case 15: goto __func29_instr15;
case 16: goto __func29_instr16;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest.DelegateTestClass/methods/PrintContents()
static int f30(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func30_start;
__func30_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 43]
// instr 1
	RuntimeValue4 __bp_offs_8_sz4_418;
	__bp_offs_8_sz4_418.cp = resArgs[0];
// instr 2
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_418;
}
{
	TICK(2);
// instr 3
}
__func30_instr3:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[2]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 3, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 4
}
__func30_instr4:
{
// rdxsrc/Apps/RegressionTest.rdx [line 44]
// instr 5
}
__func30_instr5:
{
	RuntimeValue4 __prv_offs_8_sz4_419;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-8)));
		__prv_offs_8_sz4_419 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_8_sz4_420;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_8_sz4_419);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_420);
		*dest = *src;
	}
// instr 6
	RuntimeValue8 __bp_offs_8_sz8_421;
	__bp_offs_8_sz8_421.rtp.objectRef = __bp_offs_8_sz4_420.p;
	__bp_offs_8_sz8_421.rtp.valueRef = reinterpret_cast<UInt8 *>(__bp_offs_8_sz4_420.p) + (0);
// instr 7
	RuntimeValue4 __bp_offs_8_sz4_422;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		const RuntimePointer<void> *srcPtr = &__bp_offs_8_sz8_421.rtp;
		src = static_cast<const RuntimeValue4 *>(srcPtr->valueRef);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_422);
		*dest = *src;
	}
// instr 8
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_422;
}
{
	TICK(8);
// instr 9
}
__func30_instr9:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[2]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 9, -16, 0, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 10
}
__func30_instr10:
{
// rdxsrc/Apps/RegressionTest.rdx [line 41]
// instr 11
}
__func30_instr11:
{
	EXITFRAME;
// instr 12
}
{
	INVALIDOPERATIONEXCEPTION(12);
}
__func30_start:
switch(startInstruction)
{
case 0: goto __func30_instr0;
case 3: goto __func30_instr3;
case 4: goto __func30_instr4;
case 5: goto __func30_instr5;
case 9: goto __func30_instr9;
case 10: goto __func30_instr10;
case 11: goto __func30_instr11;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest.InterfaceImplementingClass/methods/Func2()
static int f31(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func31_start;
__func31_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 75]
// instr 1
}
{
	EXITFRAME;
// instr 2
}
{
	INVALIDOPERATIONEXCEPTION(2);
}
__func31_start:
switch(startInstruction)
{
case 0: goto __func31_instr0;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestThreadDeserialization()
static int f32(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func32_start;
__func32_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 296]
// instr 1
	RuntimeValue4 __bp_offs_4_sz4_423;
	memset(&__bp_offs_4_sz4_423, 0, 4);
// instr 2
	RuntimeValue4 __bp_offs_16_sz4_424;
	__bp_offs_16_sz4_424.cp = resArgs[1];
// instr 3
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_424;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_423;
	NULLCHECK(__bp_offs_16_sz4_424.p, 3);
	if(!objm->ObjectCompatible(__bp_offs_16_sz4_424.p, static_cast<const Type *>(resArgs[0])))
		INCOMPATIBLECONVERSION(3);
// instr 4
	RuntimeValue4 __bp_offs_16_sz4_425;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_425 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_425);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_4_sz4_423);
		*dest = *src;
	}
// instr 5
// rdxsrc/Apps/RegressionTest.rdx [line 297]
// instr 6
	RuntimeValue4 __bp_offs_16_sz4_426;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_423);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_16_sz4_426);
		*dest = *src;
	}
// instr 7
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_426;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_423;
}
{
	TICK(7);
// instr 8
}
__func32_instr8:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[2]);
		return CallMethodNotNative(ctx, objm, invokedMethod, 8, -16, -8, thread, instrTable, bp, providerDictionary);
	}
// instr 9
}
__func32_instr9:
{
// rdxsrc/Apps/RegressionTest.rdx [line 296]
// instr 10
}
__func32_instr10:
{
	EXITFRAME;
// instr 11
}
{
	INVALIDOPERATIONEXCEPTION(11);
}
__func32_start:
switch(startInstruction)
{
case 0: goto __func32_instr0;
case 8: goto __func32_instr8;
case 9: goto __func32_instr9;
case 10: goto __func32_instr10;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestExceptionThrow()
static int f33(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func33_start;
__func33_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 137]
// instr 1
}
__func33_instr1:
{
	RuntimeValue4 __bp_offs_8_sz4_427;
	{
		t->frame.ip = instrTable + ((1) + 1);
		const Type *t = static_cast<const Type *>(resArgs[0]);
		void *obj = RDX_RuntimeUtilities_NewObjectInstance(ctx, objm, t, reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8 *>(bp) + (0)), 0);
		NULLCHECK(obj, 1);
		__bp_offs_8_sz4_427.p = obj;
	}
// instr 2
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_427;
	THROWEXCEPTION(__bp_offs_8_sz4_427.p, 2);
// instr 3
}
{
	INVALIDOPERATIONEXCEPTION(3);
}
__func33_start:
switch(startInstruction)
{
case 0: goto __func33_instr0;
case 1: goto __func33_instr1;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestReentry()
static int f34(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func34_start;
__func34_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 193]
// instr 1
	RuntimeValue4 __bp_offs_16_sz4_428;
	__bp_offs_16_sz4_428.cp = resArgs[0];
// instr 2
	RuntimeValue4 __bp_offs_24_sz4_429;
	__bp_offs_24_sz4_429.cp = resArgs[2];
// instr 3
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_428;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_429;
	NULLCHECK(__bp_offs_24_sz4_429.p, 3);
	if(!objm->ObjectCompatible(__bp_offs_24_sz4_429.p, static_cast<const Type *>(resArgs[3])))
		INCOMPATIBLECONVERSION(3);
// instr 4
}
{
	TICK(4);
// instr 5
}
__func34_instr5:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[4]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 5, -32, -8, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 6
}
__func34_instr6:
{
// rdxsrc/Apps/RegressionTest.rdx [line 193]
// instr 7
}
__func34_instr7:
{
	goto __func34_instr20;
// instr 8
}
{
	INVALIDOPERATIONEXCEPTION(8);
// instr 9
}
__func34_instr9:
{
	RuntimeValue4 __bp_offs_16_sz4_430;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_430 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_4_sz4_431;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_430);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_4_sz4_431);
		*dest = *src;
	}
// instr 10
// rdxsrc/Apps/RegressionTest.rdx [line 199]
// instr 11
	RuntimeValue4 __bp_offs_16_sz4_432;
	__bp_offs_16_sz4_432.cp = resArgs[6];
// instr 12
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_432;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_431;
}
{
	TICK(12);
// instr 13
}
__func34_instr13:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[7]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 13, -16, -8, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 14
}
__func34_instr14:
{
// rdxsrc/Apps/RegressionTest.rdx [line 193]
// instr 15
}
__func34_instr15:
{
	goto __func34_instr20;
// instr 16
}
{
	INVALIDOPERATIONEXCEPTION(16);
// instr 17
}
__func34_instr17:
{
	RuntimeValue4 __bp_offs_16_sz4_433;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_433 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_16_sz4_433.p, 17);
	if(objm->TypesCompatible(GCInfo::From(__bp_offs_16_sz4_433.p)->containerType, static_cast<Type *>(resArgs[5])))
		goto __func34_instr9;
// instr 18
	RuntimeValue4 __bp_offs_16_sz4_434;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-16)));
		__bp_offs_16_sz4_434 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	THROWEXCEPTION(__bp_offs_16_sz4_434.p, 18);
// instr 19
// rdxsrc/Apps/RegressionTest.rdx [line 201]
// instr 20
}
__func34_instr20:
{
	RuntimeValue4 __bp_offs_16_sz4_435;
	__bp_offs_16_sz4_435.cp = resArgs[9];
// instr 21
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_435;
}
{
	TICK(21);
// instr 22
}
__func34_instr22:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[7]);
		bool shouldContinue = false;
		int methodStatus = CallMethodNative(ctx, objm, invokedMethod, 22, -16, -8, thread, instrTable, bp, shouldContinue);
		if(!shouldContinue)
			return methodStatus;
	}
// instr 23
}
__func34_instr23:
{
// rdxsrc/Apps/RegressionTest.rdx [line 191]
// instr 24
}
__func34_instr24:
{
	EXITFRAME;
// instr 25
}
{
	INVALIDOPERATIONEXCEPTION(25);
}
__func34_start:
switch(startInstruction)
{
case 0: goto __func34_instr0;
case 5: goto __func34_instr5;
case 6: goto __func34_instr6;
case 7: goto __func34_instr7;
case 9: goto __func34_instr9;
case 13: goto __func34_instr13;
case 14: goto __func34_instr14;
case 15: goto __func34_instr15;
case 17: goto __func34_instr17;
case 20: goto __func34_instr20;
case 22: goto __func34_instr22;
case 23: goto __func34_instr23;
case 24: goto __func34_instr24;
};

return 0;

} // ************* END FUNC


	// ***** Apps.RegressionTest.RegressionTest/methods/TestMaskWarning(Core.int)
static int f35(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary) {
void *bp = thread->frame.bp;
void *prv = thread->frame.prv;
const Method *m = thread->frame.method;
void **resArgs = m->resArgs;
LargeInt startInstruction = static_cast<const LargeInt *>(thread->frame.ip) - instrTable;
goto __func35_start;
__func35_instr0:
{
// instr 0
// rdxsrc/Apps/RegressionTest.rdx [line 359]
// instr 1
}
{
	EXITFRAME;
// instr 2
}
{
	INVALIDOPERATIONEXCEPTION(2);
}
__func35_start:
switch(startInstruction)
{
case 0: goto __func35_instr0;
};

return 0;

} // ************* END FUNC
// GIT format: Positive = -(value) to function encode, resume allowed
//             Negative = -(value) to function encode, resume forbidden
//             Zero     = Invalid, execution never stops or resumes at this point
static LargeInt globalInstructionTable[] = {
	// f1, Apps.RegressionTest.RegressionTest.TestUsing_Struct/methods/Initialize(Core.string)
	0, 1, 0, 0, 0, 0, 0, 0, 
	// f2, Apps.RegressionTest.RegressionTest/methods/TestThreadDeserializationPD2()
	0, 1, 0, 3, 0, 0, 0, 0, 8, 9, 10, 0, 
	// f3, Apps.RegressionTest.RegressionTest/methods/TestExceptions()
	0, 1, 0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 13, 0, 0, 0, 17, 18, 19, 0, 21, 0, 0, 0, 25, 26, 27, 0, 29, 0, 
	0, 0, 33, 0, 0, 0, 
	// f4, Apps.RegressionTest.RegressionTest.TestUsing_Struct/methods/Dispose()
	0, 1, 0, 0, 0, 0, 0, 7, 8, 9, 10, 11, 12, 0, 
	// f5, Apps.RegressionTest.RegressionTest/methods/TestLargeParametersCall(Core.int,Apps.RegressionTest.RegressionTest.FatStruct,Core.int)
	0, 1, 0, 0, 4, 5, 6, 0, 8, 9, 10, 0, 0, 13, 14, 15, 0, 0, 18, 19, 20, 0, 0, 23, 24, 25, 0, 0, 28, 29, 30, 
	0, 32, 33, 34, 0, 
	// f6, Apps.RegressionTest.RegressionTest.InterfaceImplementingClass/methods/Func4()
	0, 1, 0, 0, 4, 5, 0, 
	// f7, Apps.RegressionTest.RegressionTest.InterfaceImplementingClass/methods/Func3()
	0, 1, 0, 0, 
	// f8, Apps.RegressionTest.RegressionTest.DelegateTestStruct/methods/PrintContents(Core.string)
	0, 1, 0, 0, 0, 0, 6, 7, 8, 0, 
	// f9, Apps.RegressionTest.RegressionTest/methods/TestRValueDelegate(Apps.RegressionTest.RegressionTest.DelegateTestInterface)
	0, 1, 0, 0, 4, 0, 0, 0, 0, 0, 
	// f10, Apps.RegressionTest.RegressionTest/methods/TestFinally()
	0, 1, 0, 0, 0, 0, 6, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 21, 22, 0, 0, 0, 0, 0, 0, 0, 30, 
	0, 0, 0, 34, 0, 0, 37, 0, 39, 40, 41, 0, 0, 0, 0, 46, 0, 0, 0, 0, 0, 52, 0, 0, 0, 0, 0, 0, 59, 0, 61, 
	62, 63, 64, 0, 66, 0, 0, 0, 
	// f11, Apps.RegressionTest.RegressionTest/methods/TestThreadDeserializationDemote(Apps.RegressionTest.RegressionTest.DemotionTest)
	0, 1, 2, 0, 0, 0, 0, 7, 8, 0, 0, 0, 0, 0, 14, 0, 16, 17, 18, 0, 20, 21, 22, 0, 0, 0, 0, 
	// f12, Apps.RegressionTest.RegressionTest/methods/TestOrderOfOperations()
	0, 1, 0, 0, 4, 5, 6, 0, 8, 9, 10, 0, 
	// f13, Apps.RegressionTest.RegressionTest.UnrelatedStruct/methods/Test()
	0, 1, 0, 0, 4, 5, 6, 0, 
	// f14, Apps.RegressionTest.RegressionTest/methods/TestExceptionInstrOffsetter()
	0, 1, 0, 0, 
	// f15, Apps.RegressionTest.RegressionTest/methods/TestBD_Static(Core.string)
	0, 1, 0, 0, 4, 5, 6, 0, 
	// f16, Apps.RegressionTest.RegressionTest/methods/TestHashTable()
	0, 1, 0, 3, 0, 0, 6, 7, 8, 0, 0, 0, 12, 13, 14, 0, 0, 0, 18, 0, 0, 0, 22, 0, 0, 0, 0, 0, 0, 29, 30, 
	31, 32, 33, 0, 0, 36, 37, 38, 39, 40, 0, 42, 0, 0, 0, 0, 0, 0, 49, 50, 51, 52, 0, 54, 55, 0, 0, 58, 59, 0, 0, 
	0, 0, 64, 65, 66, 0, 68, 69, 70, 71, 0, 73, 0, 
	// f17, Apps.RegressionTest.RegressionTest/methods/TestSwitch(Core.int)
	0, 1, 0, 0, 4, 5, 6, 7, 0, 9, 0, 11, 12, 13, 0, 15, 0, 17, 18, 19, 0, 21, 0, 23, 24, 25, 0, 
	// f18, Apps.RegressionTest.RegressionTest/methods/TestInterfaces()
	0, 1, 0, 3, 0, 0, 0, 0, 8, 9, 10, 11, 0, 
	// f19, Apps.RegressionTest.RegressionTest/methods/TestLargeParameters()
	0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 25, 26, 0, 
	// f20, Apps.RegressionTest.RegressionTest/methods/TestThreadDeserializationPD1(#DS-#TT-()#PL-())
	0, 1, 0, 3, 4, 5, 0, 
	// f21, Apps.RegressionTest.RegressionTest/methods/TestDeadCodeElimination()
	0, 1, 0, 0, 
	// f22, Apps.RegressionTest.RegressionTest.InterfaceImplementingClass/methods/Func1()
	0, 1, 0, 0, 
	// f23, Apps.RegressionTest.RegressionTest/methods/main(#Core.string[C])
	0, 1, 0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 0, 0, 0, 18, 0, 0, 0, 22, 0, 0, 0, 0, 0, 28, 29, 30, 
	31, 0, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 0, 0, 
	// f24, Apps.RegressionTest.RegressionTest/methods/TestUsing()
	0, 1, 0, 0, 0, 0, 6, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 21, 0, 0, 0, 0, 0, 0, 0, 29, 30, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 40, 0, 0, 0, 44, 0, 46, 0, 48, 49, 0, 0, 0, 0, 54, 0, 0, 0, 58, 0, 0, 0, 
	0, 0, 64, 0, 0, 0, 68, 0, 0, 0, 72, 0, 0, 75, 0, 77, 0, 79, 80, 0, 0, 0, 0, 85, 0, 0, 0, 0, 90, 0, 0, 
	0, 0, 0, 0, 97, 0, 99, 100, 101, 102, 0, 104, 0, 0, 0, 
	// f25, Apps.RegressionTest.RegressionTest/methods/TestUnboundDelegates()
	0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11, 12, 0, 0, 0, 16, 17, 18, 0, 
	// f26, Apps.RegressionTest.RegressionTest/methods/TestArrayIterator()
	0, 1, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 26, 27, 28, 0, 0, 
	0, 0, 33, 0, 0, 0, 37, 38, 39, 40, 0, 42, 0, 0, 0, 0, 0, 0, 49, 0, 0, 0, 0, 0, 0, 0, 57, 58, 59, 0, 61, 
	62, 63, 64, 0, 66, 0, 
	// f27, Apps.RegressionTest.RegressionTest/methods/TestUnboundDelegateFunc(Core.string)
	0, 1, 0, 0, 4, 5, 6, 0, 0, 0, 
	// f28, Apps.RegressionTest.RegressionTest/methods/TestBoundDelegates()
	0, 1, 0, 0, 4, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 27, 28, 29, 30, 
	0, 0, 0, 0, 0, 36, 37, 38, 39, 0, 0, 42, 0, 0, 0, 0, 0, 0, 0, 50, 51, 52, 53, 54, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 65, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 77, 78, 79, 80, 0, 0, 0, 0, 0, 86, 87, 0, 0, 0, 91, 92, 
	93, 94, 0, 96, 0, 0, 0, 0, 0, 0, 103, 104, 105, 106, 0, 
	// f29, Apps.RegressionTest.RegressionTest/methods/TestReentryFunc(Core.string,Core.string)
	0, 1, 0, 0, 4, 5, 6, 0, 8, 9, 10, 0, 12, 13, 14, 15, 16, 17, 0, 0, 
	// f30, Apps.RegressionTest.RegressionTest.DelegateTestClass/methods/PrintContents()
	0, 1, 0, 0, 4, 5, 6, 0, 0, 0, 10, 11, 12, 0, 
	// f31, Apps.RegressionTest.RegressionTest.InterfaceImplementingClass/methods/Func2()
	0, 1, 0, 0, 
	// f32, Apps.RegressionTest.RegressionTest/methods/TestThreadDeserialization()
	0, 1, 0, 0, 0, 0, 0, 0, 0, 9, 10, 11, 0, 
	// f33, Apps.RegressionTest.RegressionTest/methods/TestExceptionThrow()
	0, 1, 2, 0, 0, 
	// f34, Apps.RegressionTest.RegressionTest/methods/TestReentry()
	0, 1, 0, 0, 0, 0, 6, 7, 8, 0, 10, 0, 0, 0, 14, 15, 16, 0, 18, 0, 0, 21, 0, 23, 24, 25, 0, 
	// f35, Apps.RegressionTest.RegressionTest/methods/TestMaskWarning(Core.int)
	0, 1, 0, 0, 
};
static StaticLookupTable<StaticLookupStringKey<char, char>, PrecompiledFunctionInfo>::Entry functionTable[] =
{
	{ "Apps.RegressionTest.RegressionTest.TestUsing_Struct/methods/Initialize(Core.string)", { 1 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestThreadDeserializationPD2()", { 2 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestExceptions()", { 3 } },
	{ "Apps.RegressionTest.RegressionTest.TestUsing_Struct/methods/Dispose()", { 4 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestLargeParametersCall(Core.int,Apps.RegressionTest.RegressionTest.FatStruct,Core.int)", { 5 } },
	{ "Apps.RegressionTest.RegressionTest.InterfaceImplementingClass/methods/Func4()", { 6 } },
	{ "Apps.RegressionTest.RegressionTest.InterfaceImplementingClass/methods/Func3()", { 7 } },
	{ "Apps.RegressionTest.RegressionTest.DelegateTestStruct/methods/PrintContents(Core.string)", { 8 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestRValueDelegate(Apps.RegressionTest.RegressionTest.DelegateTestInterface)", { 9 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestFinally()", { 10 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestThreadDeserializationDemote(Apps.RegressionTest.RegressionTest.DemotionTest)", { 11 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestOrderOfOperations()", { 12 } },
	{ "Apps.RegressionTest.RegressionTest.UnrelatedStruct/methods/Test()", { 13 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestExceptionInstrOffsetter()", { 14 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestBD_Static(Core.string)", { 15 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestHashTable()", { 16 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestSwitch(Core.int)", { 17 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestInterfaces()", { 18 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestLargeParameters()", { 19 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestThreadDeserializationPD1(#DS-#TT-()#PL-())", { 20 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestDeadCodeElimination()", { 21 } },
	{ "Apps.RegressionTest.RegressionTest.InterfaceImplementingClass/methods/Func1()", { 22 } },
	{ "Apps.RegressionTest.RegressionTest/methods/main(#Core.string[C])", { 23 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestUsing()", { 24 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestUnboundDelegates()", { 25 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestArrayIterator()", { 26 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestUnboundDelegateFunc(Core.string)", { 27 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestBoundDelegates()", { 28 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestReentryFunc(Core.string,Core.string)", { 29 } },
	{ "Apps.RegressionTest.RegressionTest.DelegateTestClass/methods/PrintContents()", { 30 } },
	{ "Apps.RegressionTest.RegressionTest.InterfaceImplementingClass/methods/Func2()", { 31 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestThreadDeserialization()", { 32 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestExceptionThrow()", { 33 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestReentry()", { 34 } },
	{ "Apps.RegressionTest.RegressionTest/methods/TestMaskWarning(Core.int)", { 35 } },
};
static void InitializeCompiledRDX()
{
	globalInstructionTable[0] = reinterpret_cast<const char *>(f1) - reinterpret_cast<const char *>(NULL);
	functionTable[0].value.compiledInstructions = globalInstructionTable + 1;
	globalInstructionTable[8] = reinterpret_cast<const char *>(f2) - reinterpret_cast<const char *>(NULL);
	functionTable[1].value.compiledInstructions = globalInstructionTable + 9;
	globalInstructionTable[20] = reinterpret_cast<const char *>(f3) - reinterpret_cast<const char *>(NULL);
	functionTable[2].value.compiledInstructions = globalInstructionTable + 21;
	globalInstructionTable[57] = reinterpret_cast<const char *>(f4) - reinterpret_cast<const char *>(NULL);
	functionTable[3].value.compiledInstructions = globalInstructionTable + 58;
	globalInstructionTable[71] = reinterpret_cast<const char *>(f5) - reinterpret_cast<const char *>(NULL);
	functionTable[4].value.compiledInstructions = globalInstructionTable + 72;
	globalInstructionTable[107] = reinterpret_cast<const char *>(f6) - reinterpret_cast<const char *>(NULL);
	functionTable[5].value.compiledInstructions = globalInstructionTable + 108;
	globalInstructionTable[114] = reinterpret_cast<const char *>(f7) - reinterpret_cast<const char *>(NULL);
	functionTable[6].value.compiledInstructions = globalInstructionTable + 115;
	globalInstructionTable[118] = reinterpret_cast<const char *>(f8) - reinterpret_cast<const char *>(NULL);
	functionTable[7].value.compiledInstructions = globalInstructionTable + 119;
	globalInstructionTable[128] = reinterpret_cast<const char *>(f9) - reinterpret_cast<const char *>(NULL);
	functionTable[8].value.compiledInstructions = globalInstructionTable + 129;
	globalInstructionTable[138] = reinterpret_cast<const char *>(f10) - reinterpret_cast<const char *>(NULL);
	functionTable[9].value.compiledInstructions = globalInstructionTable + 139;
	globalInstructionTable[208] = reinterpret_cast<const char *>(f11) - reinterpret_cast<const char *>(NULL);
	functionTable[10].value.compiledInstructions = globalInstructionTable + 209;
	globalInstructionTable[235] = reinterpret_cast<const char *>(f12) - reinterpret_cast<const char *>(NULL);
	functionTable[11].value.compiledInstructions = globalInstructionTable + 236;
	globalInstructionTable[247] = reinterpret_cast<const char *>(f13) - reinterpret_cast<const char *>(NULL);
	functionTable[12].value.compiledInstructions = globalInstructionTable + 248;
	globalInstructionTable[255] = reinterpret_cast<const char *>(f14) - reinterpret_cast<const char *>(NULL);
	functionTable[13].value.compiledInstructions = globalInstructionTable + 256;
	globalInstructionTable[259] = reinterpret_cast<const char *>(f15) - reinterpret_cast<const char *>(NULL);
	functionTable[14].value.compiledInstructions = globalInstructionTable + 260;
	globalInstructionTable[267] = reinterpret_cast<const char *>(f16) - reinterpret_cast<const char *>(NULL);
	functionTable[15].value.compiledInstructions = globalInstructionTable + 268;
	globalInstructionTable[342] = reinterpret_cast<const char *>(f17) - reinterpret_cast<const char *>(NULL);
	functionTable[16].value.compiledInstructions = globalInstructionTable + 343;
	globalInstructionTable[369] = reinterpret_cast<const char *>(f18) - reinterpret_cast<const char *>(NULL);
	functionTable[17].value.compiledInstructions = globalInstructionTable + 370;
	globalInstructionTable[382] = reinterpret_cast<const char *>(f19) - reinterpret_cast<const char *>(NULL);
	functionTable[18].value.compiledInstructions = globalInstructionTable + 383;
	globalInstructionTable[410] = reinterpret_cast<const char *>(f20) - reinterpret_cast<const char *>(NULL);
	functionTable[19].value.compiledInstructions = globalInstructionTable + 411;
	globalInstructionTable[417] = reinterpret_cast<const char *>(f21) - reinterpret_cast<const char *>(NULL);
	functionTable[20].value.compiledInstructions = globalInstructionTable + 418;
	globalInstructionTable[421] = reinterpret_cast<const char *>(f22) - reinterpret_cast<const char *>(NULL);
	functionTable[21].value.compiledInstructions = globalInstructionTable + 422;
	globalInstructionTable[425] = reinterpret_cast<const char *>(f23) - reinterpret_cast<const char *>(NULL);
	functionTable[22].value.compiledInstructions = globalInstructionTable + 426;
	globalInstructionTable[486] = reinterpret_cast<const char *>(f24) - reinterpret_cast<const char *>(NULL);
	functionTable[23].value.compiledInstructions = globalInstructionTable + 487;
	globalInstructionTable[594] = reinterpret_cast<const char *>(f25) - reinterpret_cast<const char *>(NULL);
	functionTable[24].value.compiledInstructions = globalInstructionTable + 595;
	globalInstructionTable[614] = reinterpret_cast<const char *>(f26) - reinterpret_cast<const char *>(NULL);
	functionTable[25].value.compiledInstructions = globalInstructionTable + 615;
	globalInstructionTable[682] = reinterpret_cast<const char *>(f27) - reinterpret_cast<const char *>(NULL);
	functionTable[26].value.compiledInstructions = globalInstructionTable + 683;
	globalInstructionTable[692] = reinterpret_cast<const char *>(f28) - reinterpret_cast<const char *>(NULL);
	functionTable[27].value.compiledInstructions = globalInstructionTable + 693;
	globalInstructionTable[800] = reinterpret_cast<const char *>(f29) - reinterpret_cast<const char *>(NULL);
	functionTable[28].value.compiledInstructions = globalInstructionTable + 801;
	globalInstructionTable[820] = reinterpret_cast<const char *>(f30) - reinterpret_cast<const char *>(NULL);
	functionTable[29].value.compiledInstructions = globalInstructionTable + 821;
	globalInstructionTable[834] = reinterpret_cast<const char *>(f31) - reinterpret_cast<const char *>(NULL);
	functionTable[30].value.compiledInstructions = globalInstructionTable + 835;
	globalInstructionTable[838] = reinterpret_cast<const char *>(f32) - reinterpret_cast<const char *>(NULL);
	functionTable[31].value.compiledInstructions = globalInstructionTable + 839;
	globalInstructionTable[851] = reinterpret_cast<const char *>(f33) - reinterpret_cast<const char *>(NULL);
	functionTable[32].value.compiledInstructions = globalInstructionTable + 852;
	globalInstructionTable[856] = reinterpret_cast<const char *>(f34) - reinterpret_cast<const char *>(NULL);
	functionTable[33].value.compiledInstructions = globalInstructionTable + 857;
	globalInstructionTable[883] = reinterpret_cast<const char *>(f35) - reinterpret_cast<const char *>(NULL);
	functionTable[34].value.compiledInstructions = globalInstructionTable + 884;
}
static AutoRunFunction compileInitializer(InitializeCompiledRDX);
static RDX::StaticLookupTable<RDX::StaticLookupStringKey<char, char>, PrecompiledFunctionInfo> functionLookup(functionTable, sizeof(functionTable)/sizeof(functionTable[0]));
namespace RDX
{
	namespace PCCM
	{
		PrecompiledCodeModule RegressionTest(globalInstructionTable, sizeof(globalInstructionTable), &functionLookup);
	}
}
