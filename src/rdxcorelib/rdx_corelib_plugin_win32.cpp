#include "../rdx/rdx_objectmanagement.hpp"

namespace RDXInterface
{
	namespace Core
	{
		const rdxINativeTypeHost *CoreLibPlugin();
	}
}

extern "C" __declspec(dllexport) const void *RDXPlugin()
{
	return ::RDXInterface::Core::CoreLibPlugin();
}
