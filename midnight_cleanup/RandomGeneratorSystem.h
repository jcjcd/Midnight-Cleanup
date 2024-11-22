#pragma once
#include <Animacore/SystemInterface.h>
#include <Animacore/SystemTraits.h>

namespace core
{
	struct WorldTransform;
	struct OnPostStartSystem;
	struct OnFinishSystem;
}

namespace mc
{
	class RandomGeneratorSystem : public core::ISystem
	{
	public:
		explicit RandomGeneratorSystem(core::Scene& scene);
		~RandomGeneratorSystem() override;

	private:
		void postStartSystem(const core::OnPostStartSystem& event);
		void finishSystem(const core::OnFinishSystem& event);

		entt::dispatcher* _dispatcher = nullptr;
		std::vector<entt::entity> _trashGenerators;
	};
}
DEFINE_SYSTEM_TRAITS(mc::RandomGeneratorSystem)

