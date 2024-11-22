#include "pch.h"
#include "McProcess.h"

#include "McMetaFuncs.h"


mc::McProcess::McProcess(const core::ProcessInfo& info)
	: CoreProcess(info)
{
	RegisterMcMetaData();
}

void mc::McProcess::Loop()
{
}
