#include "../rdx/rdx_pccm.hpp"
#include "../rdx/rdx_objectmanagement.hpp"
#include "../rdx/rdx_ilcomp.hpp"
#include "../rdx/rdx_runtime.hpp"
#include "../rdx/rdx_lut.hpp"
using namespace RDX;
using namespace RDX::ObjectManagement;
using namespace RDX::Programmability;
using namespace RDX::Programmability::RuntimeUtilities;
#include "myapp_pccm.hpp"
static int ThrowException(RuntimeThread *t, Exception *e, const void *ip)
{
	t->ex = e;
	t->frame.ip = ip;
	return RuntimeState::Exception;
}
#define THROWEXCEPTION(e, instrNum) return ThrowException(thread, static_cast<Exception*>(e), instrTable + instrNum)
#define NULLCHECK(v, instrNum) if(v == NULL) NULLREFEXCEPTION(instrNum)
#define EXITFRAME do { const RuntimeStackFrame *rfp = reinterpret_cast<RuntimeStackFrame *>(bp); thread->frame = *rfp; return RuntimeState::Active; } while(0)
#define INVALIDOPERATIONEXCEPTION(instrNum) THROWEXCEPTION(providerDictionary[X_InvalidOperationException], instrNum)
#define INCOMPATIBLECONVERSION(instrNum) THROWEXCEPTION(providerDictionary[X_IncompatibleConversionException], instrNum)
#define NULLREFEXCEPTION(instrNum) THROWEXCEPTION(providerDictionary[X_NullReferenceException], instrNum)
#define DIVIDEBYZEROEXCEPTION(instrNum) THROWEXCEPTION(providerDictionary[X_DivideByZeroException], instrNum)
#define OUTOFBOUNDS(instrNum) THROWEXCEPTION(providerDictionary[X_IndexOutOfBoundsException], instrNum)
#define CALLMETHOD(invokedMethod, instrNum, frameOffs, prvOffs, forceNative, forceNotNative)	do {\
	thread->frame.ip = instrTable + (instrNum + 1);\
	const GCInfo *invokedMethodInfo = GCInfo::From(invokedMethod);\
	NativeCallback cb = invokedMethod->_native.nativeCall;\
	if(!forceNative)\
	{\
		NULLCHECK(cb, instrNum);\
	}\
	if(forceNative || (invokedMethod->bytecode != NULL && !forceNotNative))\
	{\
		RuntimeStackFrame newFrame;\
		if(!RuntimeUtilities::EnterMethod(objm, invokedMethod, &thread->frame, static_cast<UInt8*>(bp) + (frameOffs), thread->stackBytes,\
			reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8*>(bp) + (prvOffs)), &newFrame))\
			THROWEXCEPTION(objm->GetBuiltIns()->providerDictionary[X_StackOverflowException], instrNum);\
		thread->frame = newFrame;\
		return RuntimeState::Active;\
	}\
	else\
	{\
		RuntimeStackFrame currentFrame = thread->frame;\
		int status = cb(ctx, objm, invokedMethod, thread, reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8*>(bp) + (prvOffs)));\
		if(status <= 0)\
		{\
			thread->frame = currentFrame;\
			return status;\
		}\
	}\
} while(0)

#define TICK(instrNum)	do {\
		if((--thread->timeout) == 0)\
		{\
			thread->frame.ip = instrTable + (instrNum + 1);\
			return RuntimeState::TimedOut;\
		}\
	} while(0)


	// ***** Apps.MyApp.MyApp/methods/main(#Core.string[C])
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
// rdxsrc/Apps/test.rdx [line 14]
// instr 1
	RuntimeValue4 __bp_offs_4_sz4_1;
	memset(&__bp_offs_4_sz4_1, 0, 4);
// instr 2
	RuntimeValue4 __bp_offs_48_sz4_2;
	__bp_offs_48_sz4_2.addrs[0] = PackAddress(2,0,0,0);
// instr 3
	RuntimeValue4 __bp_offs_56_sz4_3;
	__bp_offs_56_sz4_3.addrs[0] = PackAddress(3,0,0,0);
// instr 4
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_2;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_1;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-56)))) = __bp_offs_56_sz4_3;
}
__func1_instr4:
{
	RuntimeValue4 __bp_offs_48_sz4_4;
	{
		const Type *t = static_cast<const Type *>(resArgs[0]);
		void *obj = NewObjectInstance(ctx, objm, t, reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8 *>(bp) + (-56)), 2);
		NULLCHECK(obj, 4);
		__bp_offs_48_sz4_4.p = obj;
	}
// instr 5
	RuntimeValue4 __bp_offs_4_sz4_5;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_48_sz4_4);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_4_sz4_5);
		*dest = *src;
	}
// instr 6
// rdxsrc/Apps/test.rdx [line 15]
// instr 7
	RuntimeValue4 __bp_offs_48_sz4_6;
	__bp_offs_48_sz4_6.cp = resArgs[2];
// instr 8
	RuntimeValue4 __bp_offs_56_sz4_7;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_5);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_56_sz4_7);
		*dest = *src;
	}
// instr 9
	RuntimeValue4 __bp_offs_64_sz4_8;
	__bp_offs_64_sz4_8.addrs[0] = PackAddress(0,0,0,0);
// instr 10
	RuntimeValue4 __bp_offs_72_sz4_9;
	__bp_offs_72_sz4_9.addrs[0] = PackAddress(0,0,0,0);
// instr 11
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_6;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-4)))) = __bp_offs_4_sz4_5;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-56)))) = __bp_offs_56_sz4_7;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-64)))) = __bp_offs_64_sz4_8;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-72)))) = __bp_offs_72_sz4_9;
	RuntimeValue8 __bp_offs_56_sz8_10;
	{
		void *dataLoc = RuntimeUtilities::ArrayIndex(__bp_offs_56_sz4_7.p, reinterpret_cast<const RuntimeStackValue *>((static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-72)))));
		if(dataLoc == NULL)
			OUTOFBOUNDS(11);
		__bp_offs_56_sz8_10.rtp.objectRef = __bp_offs_56_sz4_7.p;
		__bp_offs_56_sz8_10.rtp.valueRef = reinterpret_cast<UInt8 *>(dataLoc) + (0);
	}
// instr 12
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_48_sz4_6);
		const RuntimePointer<void> *destPtr = &__bp_offs_56_sz8_10.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 13
// rdxsrc/Apps/test.rdx [line 16]
// instr 14
	RuntimeValue4 __bp_offs_48_sz4_11;
	__bp_offs_48_sz4_11.cp = resArgs[4];
// instr 15
	RuntimeValue4 __bp_offs_56_sz4_12;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_5);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_56_sz4_12);
		*dest = *src;
	}
// instr 16
	__bp_offs_64_sz4_8.addrs[0] = PackAddress(0,0,0,0);
// instr 17
	__bp_offs_72_sz4_9.addrs[0] = PackAddress(1,0,0,0);
// instr 18
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_11;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-56)))) = __bp_offs_56_sz4_12;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-64)))) = __bp_offs_64_sz4_8;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-72)))) = __bp_offs_72_sz4_9;
	RuntimeValue8 __bp_offs_56_sz8_13;
	{
		void *dataLoc = RuntimeUtilities::ArrayIndex(__bp_offs_56_sz4_12.p, reinterpret_cast<const RuntimeStackValue *>((static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-72)))));
		if(dataLoc == NULL)
			OUTOFBOUNDS(18);
		__bp_offs_56_sz8_13.rtp.objectRef = __bp_offs_56_sz4_12.p;
		__bp_offs_56_sz8_13.rtp.valueRef = reinterpret_cast<UInt8 *>(dataLoc) + (0);
	}
// instr 19
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_48_sz4_11);
		const RuntimePointer<void> *destPtr = &__bp_offs_56_sz8_13.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 20
// rdxsrc/Apps/test.rdx [line 17]
// instr 21
	RuntimeValue4 __bp_offs_48_sz4_14;
	__bp_offs_48_sz4_14.cp = resArgs[5];
// instr 22
	RuntimeValue4 __bp_offs_56_sz4_15;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_5);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_56_sz4_15);
		*dest = *src;
	}
// instr 23
	__bp_offs_64_sz4_8.addrs[0] = PackAddress(0,0,0,0);
// instr 24
	__bp_offs_72_sz4_9.addrs[0] = PackAddress(2,0,0,0);
// instr 25
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_14;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-56)))) = __bp_offs_56_sz4_15;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-64)))) = __bp_offs_64_sz4_8;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-72)))) = __bp_offs_72_sz4_9;
	RuntimeValue8 __bp_offs_56_sz8_16;
	{
		void *dataLoc = RuntimeUtilities::ArrayIndex(__bp_offs_56_sz4_15.p, reinterpret_cast<const RuntimeStackValue *>((static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-72)))));
		if(dataLoc == NULL)
			OUTOFBOUNDS(25);
		__bp_offs_56_sz8_16.rtp.objectRef = __bp_offs_56_sz4_15.p;
		__bp_offs_56_sz8_16.rtp.valueRef = reinterpret_cast<UInt8 *>(dataLoc) + (0);
	}
// instr 26
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_48_sz4_14);
		const RuntimePointer<void> *destPtr = &__bp_offs_56_sz8_16.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 27
// rdxsrc/Apps/test.rdx [line 18]
// instr 28
	RuntimeValue4 __bp_offs_48_sz4_17;
	__bp_offs_48_sz4_17.cp = resArgs[6];
// instr 29
	RuntimeValue4 __bp_offs_56_sz4_18;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_5);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_56_sz4_18);
		*dest = *src;
	}
// instr 30
	__bp_offs_64_sz4_8.addrs[0] = PackAddress(1,0,0,0);
// instr 31
	__bp_offs_72_sz4_9.addrs[0] = PackAddress(0,0,0,0);
// instr 32
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_17;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-56)))) = __bp_offs_56_sz4_18;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-64)))) = __bp_offs_64_sz4_8;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-72)))) = __bp_offs_72_sz4_9;
	RuntimeValue8 __bp_offs_56_sz8_19;
	{
		void *dataLoc = RuntimeUtilities::ArrayIndex(__bp_offs_56_sz4_18.p, reinterpret_cast<const RuntimeStackValue *>((static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-72)))));
		if(dataLoc == NULL)
			OUTOFBOUNDS(32);
		__bp_offs_56_sz8_19.rtp.objectRef = __bp_offs_56_sz4_18.p;
		__bp_offs_56_sz8_19.rtp.valueRef = reinterpret_cast<UInt8 *>(dataLoc) + (0);
	}
// instr 33
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_48_sz4_17);
		const RuntimePointer<void> *destPtr = &__bp_offs_56_sz8_19.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 34
// rdxsrc/Apps/test.rdx [line 19]
// instr 35
	RuntimeValue4 __bp_offs_48_sz4_20;
	__bp_offs_48_sz4_20.cp = resArgs[7];
// instr 36
	RuntimeValue4 __bp_offs_56_sz4_21;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_5);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_56_sz4_21);
		*dest = *src;
	}
// instr 37
	__bp_offs_64_sz4_8.addrs[0] = PackAddress(1,0,0,0);
// instr 38
	__bp_offs_72_sz4_9.addrs[0] = PackAddress(1,0,0,0);
// instr 39
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_20;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-56)))) = __bp_offs_56_sz4_21;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-64)))) = __bp_offs_64_sz4_8;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-72)))) = __bp_offs_72_sz4_9;
	RuntimeValue8 __bp_offs_56_sz8_22;
	{
		void *dataLoc = RuntimeUtilities::ArrayIndex(__bp_offs_56_sz4_21.p, reinterpret_cast<const RuntimeStackValue *>((static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-72)))));
		if(dataLoc == NULL)
			OUTOFBOUNDS(39);
		__bp_offs_56_sz8_22.rtp.objectRef = __bp_offs_56_sz4_21.p;
		__bp_offs_56_sz8_22.rtp.valueRef = reinterpret_cast<UInt8 *>(dataLoc) + (0);
	}
// instr 40
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_48_sz4_20);
		const RuntimePointer<void> *destPtr = &__bp_offs_56_sz8_22.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 41
// rdxsrc/Apps/test.rdx [line 20]
// instr 42
	RuntimeValue4 __bp_offs_48_sz4_23;
	__bp_offs_48_sz4_23.cp = resArgs[8];
// instr 43
	RuntimeValue4 __bp_offs_56_sz4_24;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_5);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_56_sz4_24);
		*dest = *src;
	}
// instr 44
	__bp_offs_64_sz4_8.addrs[0] = PackAddress(1,0,0,0);
// instr 45
	__bp_offs_72_sz4_9.addrs[0] = PackAddress(2,0,0,0);
// instr 46
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_23;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-56)))) = __bp_offs_56_sz4_24;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-64)))) = __bp_offs_64_sz4_8;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-72)))) = __bp_offs_72_sz4_9;
	RuntimeValue8 __bp_offs_56_sz8_25;
	{
		void *dataLoc = RuntimeUtilities::ArrayIndex(__bp_offs_56_sz4_24.p, reinterpret_cast<const RuntimeStackValue *>((static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-72)))));
		if(dataLoc == NULL)
			OUTOFBOUNDS(46);
		__bp_offs_56_sz8_25.rtp.objectRef = __bp_offs_56_sz4_24.p;
		__bp_offs_56_sz8_25.rtp.valueRef = reinterpret_cast<UInt8 *>(dataLoc) + (0);
	}
// instr 47
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_48_sz4_23);
		const RuntimePointer<void> *destPtr = &__bp_offs_56_sz8_25.rtp;
		dest = static_cast<RuntimeValue4 *>(destPtr->valueRef);
		*dest = *src;
	}
// instr 48
// rdxsrc/Apps/test.rdx [line 21]
// instr 49
	RuntimeValue4 __bp_offs_48_sz4_26;
	__bp_offs_48_sz4_26.addrs[0] = PackAddress(255,255,255,255);
// instr 50
	RuntimeValue4 __bp_offs_8_sz4_27;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_48_sz4_26);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_8_sz4_27);
		*dest = *src;
	}
// instr 51
	RuntimeValue4 __bp_offs_12_sz4_28;
	memset(&__bp_offs_12_sz4_28, 0, 4);
// instr 52
	RuntimeValue4 __bp_offs_16_sz4_29;
	memset(&__bp_offs_16_sz4_29, 0, 4);
// instr 53
	RuntimeValue4 __bp_offs_20_sz4_30;
	memset(&__bp_offs_20_sz4_30, 0, 4);
// instr 54
	RuntimeValue4 __bp_offs_24_sz4_31;
	memset(&__bp_offs_24_sz4_31, 0, 4);
// instr 55
	RuntimeValue4 __bp_offs_28_sz4_32;
	memset(&__bp_offs_28_sz4_32, 0, 4);
// instr 56
	RuntimeValue4 __bp_offs_48_sz4_33;
	__bp_offs_48_sz4_33.addrs[0] = PackAddress(255,255,255,255);
// instr 57
	RuntimeValue4 __bp_offs_32_sz4_34;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_48_sz4_33);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_32_sz4_34);
		*dest = *src;
	}
// instr 58
	RuntimeValue4 __bp_offs_36_sz4_35;
	memset(&__bp_offs_36_sz4_35, 0, 4);
// instr 59
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_4_sz4_5);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_36_sz4_35);
		*dest = *src;
	}
// instr 60
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)))) = __bp_offs_12_sz4_28;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_29;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-20)))) = __bp_offs_20_sz4_30;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-24)))) = __bp_offs_24_sz4_31;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-28)))) = __bp_offs_28_sz4_32;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-32)))) = __bp_offs_32_sz4_34;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-36)))) = __bp_offs_36_sz4_35;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-8)))) = __bp_offs_8_sz4_27;
}
__func1_instr60:
{
	RuntimeValue4 __bp_offs_36_sz4_36;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-36)));
		__bp_offs_36_sz4_36 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_8_sz4_37;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-8)));
		__bp_offs_8_sz4_37 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_24_sz4_38;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-24)));
		__bp_offs_24_sz4_38 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_28_sz4_39;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-28)));
		__bp_offs_28_sz4_39 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_32_sz4_40;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-32)));
		__bp_offs_32_sz4_40 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	{
		const RuntimeValue4 *arrayRef = static_cast<const RuntimeValue4 *>(__bp_offs_36_sz4_36.p);
		NULLCHECK(__bp_offs_36_sz4_36.p, 60);
		LargeInt numElements = GCInfo::From(__bp_offs_36_sz4_36.p)->numElements;
		LargeInt index = __bp_offs_8_sz4_37.li + 1;
		if(index < 0 || index >= numElements)
			goto __func1_instr83;
		__bp_offs_24_sz4_38 = arrayRef[index];
		if(GCInfo::From(arrayRef)->numDimensions != 2)
			OUTOFBOUNDS(60);
		const LargeInt *dimensions = GCInfo::From(arrayRef)->dimensions;
		if(dimensions[0] == ++(__bp_offs_28_sz4_39).li)
		{
		__bp_offs_28_sz4_39.li = 0;
		++(__bp_offs_32_sz4_40).li;
		}
	}
// instr 61
	RuntimeValue4 __bp_offs_56_sz4_41;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_28_sz4_39);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_56_sz4_41);
		*dest = *src;
	}
// instr 62
	RuntimeValue4 __bp_offs_48_sz4_42;
	__bp_offs_48_sz4_42.i32 = static_cast<Int32>(		__bp_offs_56_sz4_41.i32);
// instr 63
	RuntimeValue4 __bp_offs_64_sz4_43;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_32_sz4_40);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_64_sz4_43);
		*dest = *src;
	}
// instr 64
	RuntimeValue4 __bp_offs_56_sz4_44;
	__bp_offs_56_sz4_44.i32 = static_cast<Int32>(		__bp_offs_64_sz4_43.i32);
// instr 65
	RuntimeValue4 __bp_offs_20_sz4_45;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_56_sz4_44);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_20_sz4_45);
		*dest = *src;
	}
// instr 66
	RuntimeValue4 __bp_offs_16_sz4_46;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_48_sz4_42);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_16_sz4_46);
		*dest = *src;
	}
// instr 67
	RuntimeValue4 __bp_offs_12_sz4_47;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_24_sz4_38);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_12_sz4_47);
		*dest = *src;
	}
// instr 68
// rdxsrc/Apps/test.rdx [line 23]
// instr 69
	RuntimeValue4 __bp_offs_48_sz4_48;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_16_sz4_46);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_48_sz4_48);
		*dest = *src;
	}
// instr 70
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-12)))) = __bp_offs_12_sz4_47;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-16)))) = __bp_offs_16_sz4_46;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-20)))) = __bp_offs_20_sz4_45;
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_48;
}
__func1_instr70:
{
	TICK(70);
// instr 71
}
__func1_instr71:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[11]);
		CALLMETHOD(invokedMethod, 71, -48, -40, true, false);
	}
// instr 72
}
__func1_instr72:
{
// rdxsrc/Apps/test.rdx [line 24]
// instr 73
	RuntimeValue4 __bp_offs_20_sz4_49;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-20)));
		__bp_offs_20_sz4_49 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_48_sz4_50;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_20_sz4_49);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_48_sz4_50);
		*dest = *src;
	}
// instr 74
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_50;
}
__func1_instr74:
{
	TICK(74);
// instr 75
}
__func1_instr75:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[11]);
		CALLMETHOD(invokedMethod, 75, -48, -40, true, false);
	}
// instr 76
}
__func1_instr76:
{
// rdxsrc/Apps/test.rdx [line 25]
// instr 77
	RuntimeValue4 __bp_offs_12_sz4_51;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-12)));
		__bp_offs_12_sz4_51 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_48_sz4_52;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_12_sz4_51);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_48_sz4_52);
		*dest = *src;
	}
// instr 78
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_52;
}
__func1_instr78:
{
	TICK(78);
// instr 79
}
__func1_instr79:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[12]);
		CALLMETHOD(invokedMethod, 79, -48, -40, true, false);
	}
// instr 80
}
__func1_instr80:
{
// rdxsrc/Apps/test.rdx [line 21]
// instr 81
}
__func1_instr81:
{
	TICK(81);
// instr 82
}
__func1_instr82:
{
	goto __func1_instr60;
// instr 83
}
__func1_instr83:
{
// rdxsrc/Apps/test.rdx [line 47]
// instr 84
	RuntimeValue4 __bp_offs_48_sz4_53;
	__bp_offs_48_sz4_53.cp = resArgs[13];
// instr 85
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-48)))) = __bp_offs_48_sz4_53;
}
__func1_instr85:
{
	TICK(85);
// instr 86
}
__func1_instr86:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[12]);
		CALLMETHOD(invokedMethod, 86, -48, -40, true, false);
	}
// instr 87
}
__func1_instr87:
{
// rdxsrc/Apps/test.rdx [line 48]
// instr 88
	RuntimeValue4 __prv_offs_8_sz4_54;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(prv) + (-8)));
		__prv_offs_8_sz4_54 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __bp_offs_56_sz4_55;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__prv_offs_8_sz4_54);
		dest = reinterpret_cast<RuntimeValue4 *>(&__bp_offs_56_sz4_55);
		*dest = *src;
	}
// instr 89
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(bp) + (-56)))) = __bp_offs_56_sz4_55;
}
__func1_instr89:
{
	TICK(89);
// instr 90
}
__func1_instr90:
{
	RuntimeValue4 __bp_offs_56_sz4_56;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-56)));
		__bp_offs_56_sz4_56 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	NULLCHECK(__bp_offs_56_sz4_56.p, 90);
// instr 91
}
__func1_instr91:
{
	{
		const Method *invokedMethod = static_cast<const Method *>(resArgs[14]);
		CALLMETHOD(invokedMethod, 91, -64, -48, true, false);
	}
// instr 92
}
__func1_instr92:
{
	RuntimeValue4 __bp_offs_48_sz4_57;
	{
		const void *src = (static_cast<const void *>(static_cast<const  UInt8 *>(bp) + (-48)));
		__bp_offs_48_sz4_57 = *reinterpret_cast<const RuntimeValue4 *>(src);
	}
	RuntimeValue4 __prv_offs0_sz4_58;
	{
		const RuntimeValue4 *src;
		RuntimeValue4 *dest;
		src = reinterpret_cast<const RuntimeValue4 *>(&__bp_offs_48_sz4_57);
		dest = reinterpret_cast<RuntimeValue4 *>(&__prv_offs0_sz4_58);
		*dest = *src;
	}
// instr 93
	*reinterpret_cast<RuntimeValue4 *>((static_cast<void *>(static_cast< UInt8 *>(prv) + (0)))) = __prv_offs0_sz4_58;
}
{
	EXITFRAME;
// instr 94
// rdxsrc/Apps/test.rdx [line 14]
// instr 95
}
{
	INVALIDOPERATIONEXCEPTION(95);
}
__func1_start:
switch(startInstruction)
{
case 0: goto __func1_instr0;
case 4: goto __func1_instr4;
case 60: goto __func1_instr60;
case 70: goto __func1_instr70;
case 71: goto __func1_instr71;
case 72: goto __func1_instr72;
case 74: goto __func1_instr74;
case 75: goto __func1_instr75;
case 76: goto __func1_instr76;
case 78: goto __func1_instr78;
case 79: goto __func1_instr79;
case 80: goto __func1_instr80;
case 81: goto __func1_instr81;
case 82: goto __func1_instr82;
case 83: goto __func1_instr83;
case 85: goto __func1_instr85;
case 86: goto __func1_instr86;
case 87: goto __func1_instr87;
case 89: goto __func1_instr89;
case 90: goto __func1_instr90;
case 91: goto __func1_instr91;
case 92: goto __func1_instr92;
};

return 0;

} // ************* END FUNC
// GIT format: Positive = -(value) to function encode, resume allowed
//             Negative = -(value) to function encode, resume forbidden
//             Zero     = Invalid, execution never stops or resumes at this point
static LargeInt globalInstructionTable[] = {
	0, 1, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 61, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 71, 72, 73, 0, 75, 76, 77, 0, 79, 80, 81, 82, 83, 84, 0, 86, 87, 88, 0, 90, 91, 92, 
	93, 0, 0, 0, 
};
static StaticLookupTable<char, PrecompiledFunctionInfo>::Entry functionTable[] =
{
	{ "Apps.MyApp.MyApp/methods/main(#Core.string[C])", { 1 } },
};
static RDX::StaticLookupTable<char, PrecompiledFunctionInfo> functionLookup(functionTable, sizeof(functionTable)/sizeof(functionTable[0]));
static void InitializeCompiledRDX()
{
	globalInstructionTable[0] = reinterpret_cast<const char *>(f1) - reinterpret_cast<const char *>(NULL);
	functionTable[0].value.compiledInstructions = globalInstructionTable + 1;
}
static AutoRunFunction compileInitializer(InitializeCompiledRDX);
namespace RDX
{
	namespace PCCM
	{
		PrecompiledCodeModule MyApp(globalInstructionTable, sizeof(globalInstructionTable), &functionLookup);
	}
}
