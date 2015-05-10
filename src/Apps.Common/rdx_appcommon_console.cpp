#include <stdio.h>
#include "RDXInterface/Apps/Common/Console.hpp"

void RDXInterface::Apps::Common::Console::WriteLine(rdxSExportedCallEnvironment &callEnv, rdxWeakRTRef(rdxCString) str)
{
	if(str.IsNull())
		_putws(L"(null)");
	else
		_putws(str->AsCharsRTRef()->ArrayData());
}
