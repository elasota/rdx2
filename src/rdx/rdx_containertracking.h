#ifndef __RDX_CONTAINERTRACKING_H__
#define __RDX_CONTAINERTRACKING_H__

#include "rdx_platform.hpp"

template<class _T> struct rdxGCInfo;

class rdxCContainerTracker
{
public:
	rdxCContainerTracker();

	void *AllocContainer() const;
};

#endif
