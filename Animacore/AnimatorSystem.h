#pragma once
#include "SystemInterface.h"
#include "SystemTraits.h"

#include "AnimatorController.h"

namespace core
{
	struct OnCreateEntity;
	struct OnStartSystem;
	struct OnFinishSystem;

	class ControllerManager
	{
	public:
		ControllerManager() = default;
		~ControllerManager() = default;

		// 얘도 결국 드라이브에서 다 읽어야하나ㅏ..?
		void LoadControllersFromDrive(const std::string& path, Renderer* renderer);

		core::AnimatorController* LoadController(const std::string& path, Renderer* renderer);
		core::AnimatorController* GetController(const std::string& path);
		core::AnimatorController* GetController(const std::string& path, Renderer* renderer);

		std::unordered_map<std::string, AnimatorController> _controllers;
	};

	class AnimatorSystem : public ISystem, public IRenderSystem
	{
	public:
		AnimatorSystem(Scene& scene);
		bool processTransition(core::Animator& animator, core::AnimatorTransition& transition);
		~AnimatorSystem() override { _dispatcher->disconnect(this); }

		void operator()(Scene& scene, Renderer& renderer, float tick) override;

		inline static ControllerManager controllerManager;
	private:
		void createEntity(const OnCreateEntity& event);
		void startSystem(const OnStartSystem& event);
		void finishSystem(const OnFinishSystem& event);
		void initEntity(core::Entity entity, Scene& scene, Renderer& renderer);

		template <typename T>
		bool compare(const T& param, const T& threshold, core::AnimatorCondition::Mode comparison)
		{
			switch (comparison)
			{
			case core::AnimatorCondition::Mode::If:
			case core::AnimatorCondition::Mode::Equals:
				return param == threshold;
			case core::AnimatorCondition::Mode::IfNot:
			case core::AnimatorCondition::Mode::NotEqual:
				return param != threshold;
			case core::AnimatorCondition::Mode::Greater:
				return param > threshold;
			case core::AnimatorCondition::Mode::Less:
				return param < threshold;
			default:
				return false;
			}
		}

		entt::dispatcher* _dispatcher = nullptr;
	};
}
DEFINE_SYSTEM_TRAITS(core::AnimatorSystem)
