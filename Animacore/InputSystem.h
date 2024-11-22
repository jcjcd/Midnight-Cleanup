#pragma once
#include "SystemTraits.h"
#include "SystemInterface.h"


namespace global
{
	enum class MouseMode
	{
		Lock,
		Clamp,
		None
	};

	inline MouseMode mouseMode = MouseMode::None;
}


namespace core
{
	struct Input;
	struct OnFinishSystem;
	struct OnStartSystem;

	class InputSystem : public ISystem, public IPreUpdateSystem
	{
	public:
		InputSystem(Scene& scene);
		~InputSystem() override { _dispatcher->disconnect(this); }

		// 인풋은 업데이트 맨 처음 도는게 필요하다.
		void PreUpdate(Scene& scene, float tick) override;

	private:
		bool isKeyPressed(int keyCode);

		// events
		void startSystem(const OnStartSystem& event);
		void finishSystem(const OnFinishSystem& event);
		void changeResolution(const OnChangeResolution& event);

		Input* _input = nullptr;

		entt::dispatcher* _dispatcher = nullptr;

		uint32_t _windowWidth = 0;
		uint32_t _windowHeight = 0;
	};
}
DEFINE_SYSTEM_TRAITS(core::InputSystem)

