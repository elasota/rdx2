#ifndef __RDX_CONDITIONALTYPE_HPP__
#define __RDX_CONDITIONALTYPE_HPP__

template<class T, bool TCondition>
struct rdxConditionalType;

template<class T>
struct rdxConditionalType<T, false>
{
	struct Type
	{
	};
};

template<class T>
struct rdxConditionalType<T, true>
{
	typedef T Type;
};

#endif
