#ifndef __RDX_NAMEGENERATOR_HPP__
#define __RDX_NAMEGENERATOR_HPP__

class rdxCNameGenerator
{
	class SMethodName
	{
	};
	// Static delegate type: #DS- | ParameterList
	// Bound delegate type: #DB- | ParameterList
	// Bound delegate marshal: BoundDelegate | "/glue/" | Method
	// Bound delegate invoke: BoundDeletate | "/glueInvoke/" | Method
	// Template: "#Tmpl." | TemplateTypeName | ":<" | (Type [ | "," [ | ...] ] ) | ">"
	// Array: "#" | Type | "[" [ | "C" ] [ | "," [ | ...] ] | "]"
	// TypeTuple: "#TT-(" [ | Type [ | "," | ...] ] | ")"
	// ParameterList: "#PL-(" [ [| "notnull "] [| "const "] | Type [ | "," | ...] ] | ")"
	// Method: Type | "/methods/" | Name | removePrefix(ParameterList)
	// Coerce method: Type | "/coerce/" | Name | removePrefix(TypeTuple)
	// Static instance: Type | "." | Name

};

#endif
