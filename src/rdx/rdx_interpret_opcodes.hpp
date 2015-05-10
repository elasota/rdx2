#ifndef __RDX_INTERPRET_OPCODES_HPP__
#define __RDX_INTERPRET_OPCODES_HPP__

// Must match MOVE_OP_GROUP
const int RDX_NUM_OPTIMIZED_MOVES	= 36;

// Opcode encoding:
// High 2 bits: Operand size (1, 2, 4, or 8)
// Opcode limit: 64
enum rdxEInterpretOp
{
	rdxIOP_move_frame,			// <flags> <source> <dest> <type>                       Copy current frame to current frame.
	rdxIOP_move_res,			// <flags> <source> <dest>                              Copy global contents to current frame.

	rdxIOP_tovarying,			// <dest> <type> <src>                                  Converts a runtime pointer at the destination to a typed runtime pointer

	rdxIOP_pinl,				// <dest> <loc>                                         Creates a pointer to a frame location.
	rdxIOP_ptroffset,			// <loc> <offset>                                       Offsets a pointer.  Stack location does not change.

	rdxIOP_objproperty,			// <src> <dest> <offset>                                NULL check src and create a pointer to the start of src + offset
	rdxIOP_objproperty_notnull,	// <src> <dest> <offset>                                Creates a pointer to the start of src + offset
	rdxIOP_objproperty_notnull_persist,//												Same as ILOP_objinterior_notnull, but source isn't transient

	rdxIOP_immediate,			// <LA0:value> <LA1[0]:dest> <LA1[1]:type> <LA2[0]:sz>  Copy instruction value to current frame
	rdxIOP_immediate_ptr,		// <LA0:value> <LA1[0]:dest> <LA1[1]:type>              Copy instruction value to current frame (immediate is a pointer to an object)
	rdxIOP_immediate_rtp,		// <LA0:value> <LA1[0]:dest> <LA1[1]:type>              Copy instruction value to current frame (immediate is a runtime pointer to an object)

	rdxIOP_arrayindex,			// <indexsrc> <arraysrc> <dest> <numdimensions>         NULL check arraysrc from current frame, push internal reference to array head + offset

	rdxIOP_call,				// <frameoffs> <prvoffs> <method> <parambasesa> ...
								//     ... <numparams> <numreturns>                     Transfers control to the target method
	rdxIOP_calldelegatebp,		// <frameoffs> <prvoffs> <methodsrc> ...
								//     ... <parambasesa> <numparams> <numreturns> ...
								//     ... <methodsrcsa>                                Null checks and transfers control to the target method in current frame
	rdxIOP_calldelegateprv,		// <frameoffs> <prvoffs> <methodsrc> ...
								//     ... <parambasesa> <numparams> <numreturns> ...
								//     ... <methodsrcsa>                                Null checks and transfers control to the target method in parent frame
	rdxIOP_callvirtual,			// <frameoffs> <prvoffs> <objsrc> <(I)vstidx> ...
								//     ... <parambasesa> <numparams> <numreturns> ...
								//     ... <objsrcsa>                                   Loads the target method, null checks, and transfers control
	rdxIOP_callinterface,		// <frameoffs> <prvoffs> <objsrc> <(I)vstidx> ...
								//     ... <parambasesa> <numparams> <numreturns>
								//     ... <objsrcsa> <interface> <method>              Loads the target method, null checks, and transfers control.  Method is for source export only

	rdxIOP_verifynotnull,		// <loc> <instr offset>                                 Verify that a current-frame value is not null, if not, move forward a set number of instructions and throw an exception
	rdxIOP_zero_op,				// <loc> <type>                                         Zero fill a location in the current frame (opstack)
	rdxIOP_zero_local,			// <loc> <type>                                         Zero fill a location in the current frame (local)
	rdxIOP_newinstance,			// <loc> <type> <dimsrc> <ndim>                         Creates a new instance of the specified type in the current frame
	rdxIOP_newinstance_local,	//                                                      Same as newinstance, but target is a local instead of opstack

	rdxIOP_exit,				//                                                      Exits the current frame

	rdxIOP_jump,				// <instrnum>                                           Jumps to the specified instruction
	rdxIOP_jtrue,				// <instrnum> <src>                                     Jumps to the specified instruction if the target value is true
	rdxIOP_jfalse,				// <instrnum> <src>                                     Jumps to the specified instruction if the target value is not true
	rdxIOP_jinherits,			// <instrnum> <src> <type>                              Jumps to the specified instruction if the target value inherits from the specified type

	rdxIOP_tick,				//														Decrements the timeout counter, suspends the thread if timeout hits zero
	rdxIOP_assertenum,			// <src> <type>                                         Throws IncompatibleConversionException if value in current frame can not be found in a specified enum table.
	rdxIOP_assertinherits,		// <src> <type>                                         Throws IncompatibleConversionException if value in current frame can not be cast to type.  Value may be null.
	rdxIOP_assertimplements,	// <src> <type>                                         Throws IncompatibleConversionException if value in current frame can not be cast to type.  Value may be null.

	rdxIOP_jeq_f,				// <instrnum> <src1> <src2> <type>                      Jumps to the specified instruction if the two specified values are equal
	rdxIOP_jeq_p,				// <instrnum> <src1> <src2> <type>                      Jumps to the specified instruction if the two specified pointers are equal
	rdxIOP_jne_f,				// <instrnum> <src1> <src2> <type>                      Jumps to the specified instruction if the two specified values are not equal
	rdxIOP_jne_p,				// <instrnum> <src1> <src2> <type>                      Jumps to the specified instruction if the two specified pointers are not equal

	rdxIOP_xnullref,			//                                                      Throws a NullReferenceException
	rdxIOP_fatal,				//                                                      Throws an InvalidOperationException
	rdxIOP_throw,				// <src>                                                Throws the exception at src.  If src is null, throws UnspecifiedException

	rdxIOP_switch,				// <src> <array res> <count> <val type>                 Searches array for the value at src, if successful jumps the number of instructions, otherwise jumps <count>
	rdxIOP_switch_ptr,			// <src> <array res> <count> <val type>                 Searches array for the value referenced by a pointer at src, if successful jumps the number of instructions, otherwise jumps <count>
	rdxIOP_iteratearray,		// <array> <index> <dest> <exit> <subscript type>       If index is within the array bounds, copies the array element at that index to dest and increments it, otherwise jumps to exit
	rdxIOP_iteratearraysub,		// <array> <index> <dest> <exit> <subscript type>
								// ... <subidx loc> <num subidx>                        Same as ILOP_iteratearray, but also increments subindexes

	rdxIOP_subop_1,
};

enum rdxEInterpretIntrOp1
{
	rdxIOP_iadd,		// Adds two integers of size p2
	rdxIOP_isub,		// Subtracts two integers of size p2
	rdxIOP_imul,		// Multiplies two integers of size p2
	rdxIOP_idiv,		// Divides two integers of size p2
	rdxIOP_imod,		// Returns the modulo of two integers of size p2
	rdxIOP_ineg,		// Negates an integer of size p2
	rdxIOP_isx,		// Converts a signed integer of size p2 to a signed integer of size p3
	rdxIOP_itof,		// Converts a signed integer of size p2 to a float of size p3

	rdxIOP_ilt,		// Compares two integers of size p2, jumps to p3 if a < b
	rdxIOP_igt,		// Compares two integers of size p2, jumps to p3 if a > b
	rdxIOP_ile,		// Compares two integers of size p2, jumps to p3 if a <= b
	rdxIOP_ige,		// Compares two integers of size p2, jumps to p3 if a >= b
	rdxIOP_ieq,		// Compares two integers of size p2, jumps to p3 if a == b
	rdxIOP_ine,		// Compares two integers of size p2, jumps to p3 if a != b

	rdxIOP_iudiv,	// Divides two integers of size p2
	rdxIOP_iumod,	// Returns the modulo of two integers of size p2
	rdxIOP_izx,		// Converts an usigned integer of size p2 to a signed integer of size p3
	rdxIOP_iutof,	// Converts an unsigned integer of size p2 to a float of size p3

	rdxIOP_iult,	// Compares two unsigned integers of size p2, jumps to p3 if a < b
	rdxIOP_iugt,	// Compares two unsigned integers of size p2, jumps to p3 if a > b
	rdxIOP_iule,	// Compares two unsigned integers of size p2, jumps to p3 if a <= b
	rdxIOP_iuge,	// Compares two unsigned integers of size p2, jumps to p3 if a >= b

	rdxIOP_fadd,	// Adds two floats of size p2
	rdxIOP_fsub,	// Subtracts two floats of size p2
	rdxIOP_fmul,	// Multiplies two floats of size p2
	rdxIOP_fdiv,	// Divides two floats of size p2
	rdxIOP_fneg,	// Negates an floats of size p2
	rdxIOP_ftoi,	// Converts a float of size p2 to a signed integer of size p3
			
	rdxIOP_flt,		// Compares two integers of size p2, jumps to p3 if a < b
	rdxIOP_fgt,		// Compares two integers of size p2, jumps to p3 if a > b
	rdxIOP_fle,		// Compares two integers of size p2, jumps to p3 if a <= b
	rdxIOP_fge,		// Compares two integers of size p2, jumps to p3 if a >= b
	rdxIOP_feq,		// Compares two integers of size p2, jumps to p3 if a == b
	rdxIOP_fne,		// Compares two integers of size p2, jumps to p3 if a != b

	rdxIOP_jccp,	// Jumps if a conversion check from p1 to p2 passes
	rdxIOP_jccf,	// Jumps if a conversion check from p1 to p2 fails

	rdxILOP_iadd4,
	rdxILOP_iadd8,
	rdxILOP_isub4,
	rdxILOP_isub8,
	rdxILOP_imul4,
	rdxILOP_imul8,
	rdxILOP_idiv4,
	rdxILOP_idiv8,
	rdxILOP_imod4,
	rdxILOP_imod8,
	rdxILOP_ineg4,
	rdxILOP_ineg8,

	rdxILOP_isx18,
	rdxILOP_isx28,
	rdxILOP_isx48,
	rdxILOP_isx14,
	rdxILOP_isx24,
	rdxILOP_isx12,

	rdxILOP_ict84,
	rdxILOP_ict82,
	rdxILOP_ict81,
	rdxILOP_ict42,
	rdxILOP_ict41,
	rdxILOP_ict21,
			
	// Null conversions
	rdxILOP_nconv8,
	rdxILOP_nconv4,
	rdxILOP_nconv2,
	rdxILOP_nconv1,

	rdxILOP_itof44,
	rdxILOP_itof48,
	rdxILOP_itof84,
	rdxILOP_itof88,

	rdxILOP_ilt4,
	rdxILOP_ilt8,
	rdxILOP_igt4,
	rdxILOP_igt8,
	rdxILOP_ile4,
	rdxILOP_ile8,
	rdxILOP_ige4,
	rdxILOP_ige8,
	rdxILOP_ieq4,
	rdxILOP_ieq8,
	rdxILOP_ine4,
	rdxILOP_ine8,

	rdxILOP_fadd4,
	rdxILOP_fsub4,
	rdxILOP_fmul4,
	rdxILOP_fdiv4,
	rdxILOP_fneg4,

	rdxILOP_ftoi44,
	rdxILOP_ftoi48,
	rdxILOP_ftoi84,
	rdxILOP_ftoi88,
			
	rdxILOP_flt4,
	rdxILOP_fgt4,
	rdxILOP_fle4,
	rdxILOP_fge4,
	rdxILOP_feq4,
	rdxILOP_fne4,

	rdxILOP_iteratearray4,
	rdxILOP_iteratearray8,
	rdxILOP_iteratearraysub4,
	rdxILOP_iteratearraysub8,

	rdxILOP_zero_i8,
	rdxILOP_zero_i16,
	rdxILOP_zero_i32,
	rdxILOP_zero_i64,
	rdxILOP_zero_bool,
	rdxILOP_zero_flt,
	rdxILOP_zero_dbl,

	rdxILOP_immediate_i8,
	rdxILOP_immediate_i16,
	rdxILOP_immediate_i32,
	rdxILOP_immediate_i64,
	rdxILOP_immediate_flt,
	rdxILOP_immediate_dbl,
	rdxILOP_immediate_bool,

	// Make sure these match rdxCInterpreterCodeProvider::CreateExecutable
	rdxILOP_jeq_f_i1,
	rdxILOP_jeq_f_i2,
	rdxILOP_jeq_f_i4,
	rdxILOP_jeq_f_i8,
	rdxILOP_jeq_f_flt,
	rdxILOP_jeq_f_dbl,
	rdxILOP_jeq_f_ref,
	rdxILOP_jeq_f_bool,

	rdxILOP_jeq_p_i1,
	rdxILOP_jeq_p_i2,
	rdxILOP_jeq_p_i4,
	rdxILOP_jeq_p_i8,
	rdxILOP_jeq_p_flt,
	rdxILOP_jeq_p_dbl,
	rdxILOP_jeq_p_ref,
	rdxILOP_jeq_p_bool,
	
	rdxILOP_jne_f_i1,
	rdxILOP_jne_f_i2,
	rdxILOP_jne_f_i4,
	rdxILOP_jne_f_i8,
	rdxILOP_jne_f_flt,
	rdxILOP_jne_f_dbl,
	rdxILOP_jne_f_ref,
	rdxILOP_jne_f_bool,

	rdxILOP_jne_p_i1,
	rdxILOP_jne_p_i2,
	rdxILOP_jne_p_i4,
	rdxILOP_jne_p_i8,
	rdxILOP_jne_p_flt,
	rdxILOP_jne_p_dbl,
	rdxILOP_jne_p_ref,
	rdxILOP_jne_p_bool,

	rdxILOP_move1,
	rdxILOP_move2 = rdxILOP_move1 + RDX_NUM_OPTIMIZED_MOVES,
	rdxILOP_move4 = rdxILOP_move2 + RDX_NUM_OPTIMIZED_MOVES,
	rdxILOP_move8 = rdxILOP_move4 + RDX_NUM_OPTIMIZED_MOVES,
	rdxILOP_movertp = rdxILOP_move8 + RDX_NUM_OPTIMIZED_MOVES,
	rdxILOP_movetrtp = rdxILOP_movertp + RDX_NUM_OPTIMIZED_MOVES,
	rdxILOP_movesz = rdxILOP_movetrtp + RDX_NUM_OPTIMIZED_MOVES,
	rdxILOP_movep = rdxILOP_movesz + RDX_NUM_OPTIMIZED_MOVES,
	rdxILOP_moveflt = rdxILOP_movep + RDX_NUM_OPTIMIZED_MOVES,
	rdxILOP_movedbl = rdxILOP_moveflt + RDX_NUM_OPTIMIZED_MOVES,
	rdxILOP_movebool = rdxILOP_movedbl + RDX_NUM_OPTIMIZED_MOVES,

	rdxILOP_unused = rdxILOP_movebool + RDX_NUM_OPTIMIZED_MOVES,
};

