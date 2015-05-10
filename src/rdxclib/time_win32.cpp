#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "../rdx/rdx_basictypes.hpp"

rdxInt64 rdxMSecTime()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	rdxInt64 counts = li.QuadPart;
	QueryPerformanceFrequency(&li);
	rdxInt64 frequency = li.QuadPart;
	return counts * 1000 / frequency;
}
