#pragma once
#include <midnight_cleanup/McProcess.h>

namespace core
{
	class Scene;
	struct OnChangeScene;
}

namespace launcher
{
	class LauncherProcess : public mc::McProcess
	{
	public:
		LauncherProcess(const core::ProcessInfo& info);

		void Loop() override;
		inline static bool isMinimized = false;

	private:
		void changeScene(const core::OnChangeScene& event);

		void changeExecution();
		void changeResolution(const core::OnChangeResolution& event);

		std::shared_ptr<core::Scene> _currentScene;

		bool _receiveChangeEvent = false;
		std::filesystem::path _scenePath;

		//int _titleBarHeight = 0;
	};
}

