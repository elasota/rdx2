/*
 * Copyright (C) 2011-2013 Eric Lasota
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __RDX_ILOPCODES_HPP__
#define __RDX_ILOPCODES_HPP__

#include "rdx_basictypes.hpp"

static const rdxLargeInt RDX_ILOP_MOVE_DEST_PARENT_FRAME	= 1;		// Destination is relative to PRV instead of BP
static const rdxLargeInt RDX_ILOP_MOVE_DEST_DEREF			= 2;		// Destination is a pointer that needs to be dereferenced
static const rdxLargeInt RDX_ILOP_MOVE_DEST_OBJECT			= 4;		// Destination is inside of an object and source export should not optimize memory
static const rdxLargeInt RDX_ILOP_MOVE_DEST_DESTROY			= 8;		// Destination contains a live value that must be destroyed
static const rdxLargeInt RDX_ILOP_MOVE_DEST_OPSTACK			= 16;		// Destination is on the opstack.
static const rdxLargeInt RDX_ILOP_MOVE_DEST_TRANSIENT		= 32;		// (Used with pointer destinations) Destination is destroyed after this op.
static const rdxLargeInt RDX_ILOP_MOVE_DEST_INITIALIZE		= 64;		// Destination is created by the move op
static const rdxLargeInt RDX_ILOP_MOVE_SRC_PARENT_FRAME		= 128;		// Source is relative to PRV instead of BP
static const rdxLargeInt RDX_ILOP_MOVE_SRC_DEREF			= 256;		// Source is a pointer that needs to be dereferenced
static const rdxLargeInt RDX_ILOP_MOVE_SRC_TRANSIENT		= 512;		// Source dies after this op, so it may be optimized out if the previous op stored it
static const rdxLargeInt RDX_ILOP_MOVE_SRC_TYPE_DEFAULT		= 1024;		// Source is a handle instead of offset, sixth parameter indicating the type it defaulted from
static const rdxLargeInt RDX_ILOP_MOVE_SRC_OBJECT			= 2048;		// Source is inside of an object and source export should not optimize memory
static const rdxLargeInt RDX_ILOP_MOVE_SRC_IS_CLASS_REF		= 4096;		// Source is a reference, gray target if source is white
static const rdxLargeInt RDX_ILOP_MOVE_SRC_CONTAINS_REFS	= 8192;		// Source is a structure that contains references, always gray target
static const rdxLargeInt RDX_ILOP_MOVE_IS_RTP				= 16384;	// Both operands are runtime pointers
static const rdxLargeInt RDX_ILOP_MOVE_SRC_IS_INTERFACE_REF	= 32768;	// Both operands are runtime pointers

// NOTE:
// OP_property can potentially come after instructions emitting any of:
// - ILOP_incptr (OP_property)
// - ILOP_pinp (OP_pinlocal)
// - ILOP_arrayindex (OP_arrayindex)
// - ILOP_objproperty (OP_property)
// All of these must support incrprop MOP
enum rdxEILOpcode
{
	rdxILOP_invalid,

	rdxILOP_debuginfo,			// <filename> <linenum>                                 Does nothing, used for debug information
	rdxILOP_move,				// <flags> <source> <dest> <type>                       Copy current frame to current frame.  Size may be zero.
	rdxILOP_pushdefault,		// TODO - Same as move
	rdxILOP_clone,				// <source> <dest>                                      Clones a stack slot

	rdxILOP_tovarying,			// <dest> <type> <src>                                  Converts a runtime pointer at the destination to a typed runtime pointer

	rdxILOP_pinl,				// <dest> <loc>                                         Creates a pointer to a stack slot
	rdxILOP_ptrprop,			// <loc> <type> <property index> <old SA>               Descends a pointer to a property.  Stack location does not change.
	rdxILOP_changeproperty,		// <type> <oldproperty> <newproperty>                   Changes a pointer to reference a different property (TODO)


	rdxILOP_objproperty,		// <src> <dest> <type> <property>                       NULL check src and create a pointer to the start of src + offset
	rdxILOP_objproperty_notnull,// <src> <dest> <type> <property>                       Creates a pointer to the start of src + offset
	rdxILOP_objproperty_notnull_persist,//												Same as ILOP_objinterior_notnull, but source isn't transient

	rdxILOP_immediate,			// <LA0:value> <LA1[0]:dest> <LA1[1]:type> <LA2[0]:sz>  Copy instruction value to current frame
	rdxILOP_immediate_ptr,		// <LA0:value> <LA1[0]:dest> <LA1[1]:type>              Copy instruction value to current frame (immediate is a pointer to an object)
	rdxILOP_immediate_rtp,		// <LA0:value> <LA1[0]:dest> <LA1[1]:type>              Copy instruction value to current frame (immediate is a runtime pointer to an object)

	rdxILOP_arrayindex,			// <indexsrc> <arraysrc> <dest> <numdimensions>         NULL check arraysrc from current frame, push internal reference to array head + offset

	// NOTE: Frameoffs is always aligned to RDX_MAX_ALIGNMENT
	rdxILOP_call,				// <frameoffs> <prvoffs> <method> <parambasesa> ...
								//     ... <numparams> <numreturns>                     Transfers control to the target method
	rdxILOP_calldelegatebp,		// <frameoffs> <prvoffs> <methodsrc> ...
								//     ... <parambasesa> <numparams> <numreturns> ...
								//     ... <methodsrcsa>                                Null checks and transfers control to the target method in current frame
	rdxILOP_calldelegateprv,	// <frameoffs> <prvoffs> <methodsrc> ...
								//     ... <parambasesa> <numparams> <numreturns> ...
								//     ... <methodsrcsa>                                Null checks and transfers control to the target method in parent frame
	rdxILOP_callvirtual,		// <frameoffs> <prvoffs> <objsrc> <(I)vstidx> ...
								//     ... <parambasesa> <numparams> <numreturns> ...
								//     ... <objsrcsa>                                   Loads the target method, null checks, and transfers control
	rdxILOP_callinterface,		// <frameoffs> <prvoffs> <objsrc> <(I)vstidx> ...
								//     ... <parambasesa> <numparams> <numreturns>
								//     ... <objsrcsa> <interface> <method>              Loads the target method, null checks, and transfers control.  Method is for source export only

	rdxILOP_verifynotnull,		// <loc> <instr offset>                                 Verify that a current-frame value is not null, if not, move forward a set number of instructions and throw an exception
	rdxILOP_zero_op,			// <loc> <type>                                         Zero fill a location in the current frame (opstack)
	rdxILOP_zero_local,			// <loc> <type>                                         Zero fill a location in the current frame (local)
	rdxILOP_newinstance,		// <loc> <type> <dimsrc> <ndim>                         Creates a new instance of the specified type in the current frame
	rdxILOP_newinstance_local,	//                                                      Same as newinstance, but target is a local instead of opstack

	rdxILOP_exit,				//                                                      Exits the current frame

	rdxILOP_jump,				// <instrnum>                                           Jumps to the specified instruction
	rdxILOP_jtrue,				// <instrnum> <src>                                     Jumps to the specified instruction if the target value is true
	rdxILOP_jfalse,				// <instrnum> <src>                                     Jumps to the specified instruction if the target value is not true
	rdxILOP_jnullo,				// <instrnum> <src>                                     Jumps to the specified instruction if the target object ref is null
	rdxILOP_jnotnullo,			// <instrnum> <src>                                     Jumps to the specified instruction if the target object ref is not null
	rdxILOP_jnulli,				// <instrnum> <src>                                     Jumps to the specified instruction if the target interface ref is null
	rdxILOP_jnotnulli,			// <instrnum> <src>                                     Jumps to the specified instruction if the target interface ref is not null
	rdxILOP_jinherits,			// <instrnum> <src> <type>                              Jumps to the specified instruction if the target value inherits from the specified type

	rdxILOP_tick,				//														Decrements the timeout counter, suspends the thread if timeout hits zero
	rdxILOP_assertenum,			// <src> <type>                                         Throws IncompatibleConversionException if value in current frame can not be found in a specified enum table.
	rdxILOP_assertinherits,		// <src> <type>                                         Throws IncompatibleConversionException if value in current frame can not be cast to type.  Value may be null.
	rdxILOP_rcast_otoi_direct,	// <src> <type> <dest> <offset>                         Converts object reference to a specific interface reference.  Value may be null.
	rdxILOP_rcast_otoi,			// <src> <type> <dest>                                  Throws IncompatibleConversionException if value in current frame can not be cast to type.  Value may be null.  Converts object reference to interface reference.
	rdxILOP_rcast_itoi,			// <src> <type> <dest>                                  Throws IncompatibleConversionException if value in current frame can not be cast to type.  Value may be null.  Converts interface reference to object reference.
	rdxILOP_rcast_itoo,			// <src> <type> <dest>                                  Throws IncompatibleConversionException if value in current frame can not be cast to type.  Value may be null.  Converts interface reference to a different interface reference.

#ifdef RDX_ALLOW_HASH_OPS
	rdxILOP_hash_f,				// <src> <dest> <size> <align>                          Generates a hashcode from the current frame and stores it at dest
	rdxILOP_hash_p,				// <src> <dest> <size> <align>                          Generates a hashcode from a pointer on current frame and stores it at dest
#endif

	rdxILOP_jeq_f,				// <instrnum> <src1> <src2> <type>                      Jumps to the specified instruction if the two specified values are equal
	rdxILOP_jeq_p,				// <instrnum> <src1> <src2> <type>                      Jumps to the specified instruction if the two specified pointers are equal
	rdxILOP_jne_f,				// <instrnum> <src1> <src2> <type>                      Jumps to the specified instruction if the two specified values are not equal
	rdxILOP_jne_p,				// <instrnum> <src1> <src2> <type>                      Jumps to the specified instruction if the two specified pointers are not equal

	rdxILOP_xnullref,			//                                                      Throws a NullReferenceException
	rdxILOP_catch,				//                                                      Throws an InvalidOperationException, but internally, marks the next instruction as patchable
	rdxILOP_fatal,				//                                                      Throws an InvalidOperationException
	rdxILOP_throw,				// <src>                                                Throws the exception at src.  If src is null, throws UnspecifiedException

	rdxILOP_hardenstack,		//                                                      Do not allow the next instruction to optimize transient storage

	rdxILOP_switch,				// <src> <array res> <count> <val type>                 Searches array for the value at src, if successful jumps the number of instructions, otherwise jumps <count>
	rdxILOP_switch_ptr,			// <src> <array res> <count> <val type>                 Searches array for the value referenced by a pointer at src, if successful jumps the number of instructions, otherwise jumps <count>
	rdxILOP_iteratearray,		// <array> <index> <dest> <exit> <subscript type>       If index is within the array bounds, copies the array element at that index to dest and increments it, otherwise jumps to exit
	rdxILOP_iteratearraysub,	// <array> <index> <dest> <exit> <subscript type>
								// ... <subidx loc> <num subidx>                        Same as ILOP_iteratearray, but also increments subindexes

	// *** INTRINSICS ***
	// Function intrinsics are always <parameters> <return values>
	// Branch intrinsics are always <target> <parameters>
	// Unlike calls, parameters are not guaranteed to be aligned to RDX_MAX_ALIGNMENT
	rdxILOP_iadd,		// Adds two integers of size p2
	rdxILOP_isub,		// Subtracts two integers of size p2
	rdxILOP_imul,		// Multiplies two integers of size p2
	rdxILOP_idiv,		// Divides two integers of size p2
	rdxILOP_imod,		// Returns the modulo of two integers of size p2
	rdxILOP_ineg,		// Negates an integer of size p2
	rdxILOP_isx,		// Converts a signed integer of size p2 to a signed integer of size p3
	rdxILOP_itof,		// Converts a signed integer of size p2 to a float of size p3

	rdxILOP_ilt,		// Compares two integers of size p2, jumps to p3 if a < b
	rdxILOP_igt,		// Compares two integers of size p2, jumps to p3 if a > b
	rdxILOP_ile,		// Compares two integers of size p2, jumps to p3 if a <= b
	rdxILOP_ige,		// Compares two integers of size p2, jumps to p3 if a >= b
	rdxILOP_ieq,		// Compares two integers of size p2, jumps to p3 if a == b
	rdxILOP_ine,		// Compares two integers of size p2, jumps to p3 if a != b
	
	rdxILOP_iudiv,		// Divides two integers of size p2
	rdxILOP_iumod,		// Returns the modulo of two integers of size p2
	rdxILOP_izx,		// Converts an usigned integer of size p2 to a signed integer of size p3
	rdxILOP_iutof,		// Converts an unsigned integer of size p2 to a float of size p3

	rdxILOP_iult,		// Compares two unsigned integers of size p2, jumps to p3 if a < b
	rdxILOP_iugt,		// Compares two unsigned integers of size p2, jumps to p3 if a > b
	rdxILOP_iule,		// Compares two unsigned integers of size p2, jumps to p3 if a <= b
	rdxILOP_iuge,		// Compares two unsigned integers of size p2, jumps to p3 if a >= b

	rdxILOP_fadd,		// Adds two floats of size p2
	rdxILOP_fsub,		// Subtracts two floats of size p2
	rdxILOP_fmul,		// Multiplies two floats of size p2
	rdxILOP_fdiv,		// Divides two floats of size p2
	rdxILOP_fneg,		// Negates an floats of size p2
	rdxILOP_ftoi,		// Converts a float of size p2 to a signed integer of size p3
			
	rdxILOP_flt,		// Compares two integers of size p2, jumps to p3 if a < b
	rdxILOP_fgt,		// Compares two integers of size p2, jumps to p3 if a > b
	rdxILOP_fle,		// Compares two integers of size p2, jumps to p3 if a <= b
	rdxILOP_fge,		// Compares two integers of size p2, jumps to p3 if a >= b
	rdxILOP_feq,		// Compares two integers of size p2, jumps to p3 if a == b
	rdxILOP_fne,		// Compares two integers of size p2, jumps to p3 if a != b

	rdxILOP_jccp,		// Jumps if a conversion check from p1 to p2 passes
	rdxILOP_jccf,		// Jumps if a conversion check from p1 to p2 fails
};

enum rdxEMarkupOpcode
{
	rdxMOP_incrprop,
	rdxMOP_moveprop,
	rdxMOP_addshell,
	//rdxMOP_debuginfo,
	rdxMOP_pop,
	rdxMOP_unshell,
	rdxMOP_movesa,
};

enum rdxEMidILOpcode
{
	rdxMIL_debuginfo,
	rdxMIL_move,
	rdxMIL_tovarying,
	rdxMIL_pinbp,
	rdxMIL_pinprv,
	rdxMIL_changeprop,
	rdxMIL_objproperty,
	rdxMIL_objproperty_notnull,
	rdxMIL_objproperty_notnull_persist,
	rdxMIL_immediate,
	rdxMIL_immediate_ptr,
	rdxMIL_immediate_rtp,
	rdxMIL_arrayindex,
	rdxMIL_call,
	rdxMIL_calldelegatebp,
	rdxMIL_calldelegateprv,
	rdxMIL_callvirtual,
	rdxMIL_callinterface,
	rdxMIL_verifynotnull,
	rdxMIL_zerofill,
	rdxMIL_newinstance,		// ???
	rdxMIL_exit,
	rdxMIL_jump,
	rdxMIL_jtrue,
	rdxMIL_jfalse,
	rdxMIL_jinherits,
	rdxMIL_tick,
	rdxMIL_verifyenum,
	rdxMIL_verifyinherits,
	rdxMIL_jeq_bp,
	rdxMIL_jeq_prv,
	rdxMIL_jne_bp,
	rdxMIL_jne_prv,
	rdxMIL_xnullref,
	rdxMIL_catch,
	rdxMIL_fatal,
	rdxMIL_throw,
	rdxMIL_flushstack,
	rdxMIL_switch,
	rdxMIL_switch_rtp,
	rdxMIL_iteratearray,
	rdxMIL_iteratearray_sub,
	rdxMIL_addslotinit,
	rdxMIL_addslotempty,
	rdxMIL_intrinsic,
};

#endif
