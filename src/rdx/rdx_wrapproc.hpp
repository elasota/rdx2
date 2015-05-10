#ifndef __RDX_WRAPPROC_HPP__
#define __RDX_WRAPPROC_HPP__

template<class T>
inline void *rdxWrapProcFunc(const T& input)
{
	void *ptrOut;
	memcpy(ptrOut, &input, sizeof(void*));
	return ptrOut;
}


#endif
