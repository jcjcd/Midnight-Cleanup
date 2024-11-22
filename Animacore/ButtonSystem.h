#pragma once
#include "SystemTraits.h"
#include "SystemInterface.h"

namespace core
{
	struct OnStartSystem;
	//struct OnGetTitleBarHeight;
}

namespace core
{
	class ButtonSystem : public ISystem, public IUpdateSystem
	{
	public:
		ButtonSystem(core::Scene& scene);
		~ButtonSystem();

		void operator()(Scene& scene, float tick) override;

	private:
		void startSystem(const core::OnStartSystem& event);
		//void getTitleBarHeight(const core::OnGetTitleBarHeight& event);

	private:
		entt::dispatcher* _dispatcher = nullptr;

		static constexpr float _defaultWidth = 1920.0f;
		static constexpr float _defaultHeight = 1080.0f;
		int _titleBarHeight = 0;

	};
}
DEFINE_SYSTEM_TRAITS(core::ButtonSystem)
