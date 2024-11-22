#pragma once
#include "SystemInterface.h"
#include "SystemTraits.h"

namespace core
{
	class PlayerTestSystem : public ISystem, public IUpdateSystem
	{
	public:
		PlayerTestSystem(Scene& scene) : ISystem(scene) {}
		~PlayerTestSystem() override = default;

		void operator()(Scene& scene, float tick) override;
	};
}
DEFINE_SYSTEM_TRAITS(core::PlayerTestSystem)
