#ifndef __RDX_STEAL_HPP__
#define __RDX_STEAL_HPP__

template<class _T>
struct rdxSStealProxy
{
private:
	_T *m_source;

public:
	explicit rdxSStealProxy(_T &source);
	rdxSStealProxy(const rdxSStealProxy<_T> &other);
	operator _T *() const;
	_T *operator ->() const;
};


template<class _T>
inline rdxSStealProxy<_T>::rdxSStealProxy(_T &source)
{
	m_source = &source;
}

template<class _T>
inline rdxSStealProxy<_T>::rdxSStealProxy(const rdxSStealProxy<_T> &other)
{
	m_source = other.m_source;
}


template<class _T>
inline rdxSStealProxy<_T>::operator _T *() const
{
	return m_source;
}

template<class _T>
inline _T *rdxSStealProxy<_T>::operator ->() const
{
	return m_source;
}

template<class _T>
inline void rdxStealSwap(_T *a, _T *b)
{
	_T atemp(rdxSStealProxy<_T>(*a));
	new (a) _T(rdxSStealProxy<_T>(*b));
	new (b) _T(rdxSStealProxy<_T>(atemp));
}

#endif
