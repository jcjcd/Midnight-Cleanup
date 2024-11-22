#pragma once
#include <Animacore/CoreProcess.h>

namespace mc
{
	class McProcess : public core::CoreProcess
	{
	public:
		McProcess(const core::ProcessInfo& info);
		~McProcess() override = default;

		void Loop() override;
	};
}
