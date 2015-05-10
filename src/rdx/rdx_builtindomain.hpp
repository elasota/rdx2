#ifndef __RDX_BUILTINDOMAIN_HPP__
#define __RDX_BUILTINDOMAIN_HPP__

enum rdxEBuiltinDomain
{
	rdxDOMAIN_Indeterminate,	// Passed to DomainForSymbolName if loading a package

	rdxDOMAIN_Duplicable,
	rdxDOMAIN_Core,
	rdxDOMAIN_Runtime,
	rdxDOMAIN_ArrayDef,

	rdxDOMAIN_Invalid,
};

#endif
