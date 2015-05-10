#ifndef __RDX_SUPERRESOLVER_H__
#define __RDX_SUPERRESOLVER_H__

template<class T, bool TUseSuper> struct rdxSuperResolverSub;

template<class T> struct rdxExplicitSuperResolver;

template<class T>
struct rdxSuperResolverSub<T, true>
{
	typedef typename T::super Type;
};

template<class T>
struct rdxSuperResolverSub<T, false>
{
	typedef typename rdxExplicitSuperResolver<T>::Type Type;
};

template<class T>
struct rdxSuperResolver
{
private:
	typedef char HasSuperHelper[2];
	typedef char HasNoSuperHelper[1];

	template<typename THelper>
    static HasSuperHelper& CheckSuper(typename THelper::super *);

	template<typename THelper>
    static HasNoSuperHelper& CheckSuper(...);

public:
    typedef typename rdxSuperResolverSub<T, sizeof(CheckSuper<T>(0)) == sizeof(HasSuperHelper)>::Type Type;
};

#define rdxSuper(T) rdxExplicitSuperResolver<T>::Type

#define RDX_DECLARE_EXPLICIT_SUPER(typeConstruct, type, superType)	\
	typeConstruct type;\
	template<>\
	struct rdxExplicitSuperResolver<type>\
	{\
		typedef superType Type;\
	}

#define RDX_DECLARE_SUPER(type, superType)	\
	template<>\
	struct rdxExplicitSuperResolver<type>\
	{\
		typedef superType Type;\
	}

#endif
