#include "rdx_opcodes.hpp"

const char *rdxILOpcodeNames[] =
{
	"invalid",

	"debuginfo",
	"move",
	"pushdefault",
	"clone",

	"tovarying",

	"pinl",
	"ptrprop",
	"changeproperty",


	"objproperty",
	"objproperty_notnull",
	"objproperty_notnull_persist",

	"immediate",
	"immediate_ptr",
	"immediate_rtp",

	"arrayindex",

	
	"call",

	"calldelegatebp",


	"calldelegateprv",


	"callvirtual",


	"callinterface",



	"verifynotnull",
	"zero_op",
	"zero_local",
	"newinstance",
	"newinstance_local",

	"exit",

	"jump",
	"jtrue",
	"jfalse",
	"jnullo",
	"jnotnullo",
	"jnulli",
	"jnotnulli",
	"jinherits",

	"tick",
	"assertenum",
	"assertinherits",
	"rcast_otoi_direct",
	"rcast_otoi",
	"rcast_itoi",
	"rcast_itoo",

	"jeq_f",
	"jeq_p",
	"jne_f",
	"jne_p",

	"xnullref",
	"catch",
	"fatal",
	"throw",

	"hardenstack",

	"switch",
	"switch_ptr",
	"iteratearray",
	"iteratearraysub",

	"iadd",
	"isub",
	"imul",
	"idiv",
	"imod",
	"ineg",
	"isx",
	"itof",

	"ilt",
	"igt",
	"ile",
	"ige",
	"ieq",
	"ine",
	
	"iudiv",
	"iumod",
	"izx",
	"iutof",

	"iult",
	"iugt",
	"iule",
	"iuge",

	"fadd",
	"fsub",
	"fmul",
	"fdiv",
	"fneg",
	"ftoi",
			
	"flt",
	"fgt",
	"fle",
	"fge",
	"feq",
	"fne",

	"jccp",
	"jccf",
};
