#pragma once
#include <Animacore/SystemTraits.h>
#include <Animacore/SystemInterface.h>

namespace mc
{
	class BGMSystem : public core::ISystem
	{
	public:
		BGMSystem(core::Scene& scene);
		~BGMSystem() override;

	private:
		void startSystem(const core::OnStartSystem& event);
		void finishSystem(const core::OnFinishSystem& event);

		entt::dispatcher* _dispatcher = nullptr;
	};
}
DEFINE_SYSTEM_TRAITS(mc::BGMSystem)
