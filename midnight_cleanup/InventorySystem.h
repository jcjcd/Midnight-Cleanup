#pragma once
#include <Animacore/SystemTraits.h>
#include <Animacore/SystemInterface.h>

#include <Animacore/CoreSystemEvents.h>

namespace core
{
	class Animator;
}

namespace mc
{
	struct Inventory;

	class InventorySystem : public core::ISystem, public core::IRenderSystem
	{
	public:
		InventorySystem(core::Scene& scene);
		~InventorySystem();

		void operator()(core::Scene& scene, Renderer& renderer, float tick) override;

	private:
		void startSystem(const core::OnStartSystem& event);
		void finishSystem(const core::OnFinishSystem& event);

	private:
		Inventory* _mainInventory = nullptr;
		entt::entity _pivotEntity = entt::null;
		entt::dispatcher* _dispatcher = nullptr;

		core::Animator* _animator = nullptr;

		bool _isCurrentToolChanged = false;
	};
}

DEFINE_SYSTEM_TRAITS(mc::InventorySystem)