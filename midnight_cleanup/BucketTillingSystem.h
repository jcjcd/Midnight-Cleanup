#pragma once
#include <Animacore/SystemInterface.h>
#include <Animacore/SystemTraits.h>

namespace core
{
	struct OnStartSystem;
	struct OnFinishSystem;
}


class BucketTillingSystem : public core::ISystem, public core::IUpdateSystem
{
public:
	BucketTillingSystem(core::Scene& scene);
	~BucketTillingSystem() override;

	void operator()(core::Scene& scene, float tick) override;

private:
	void startSystem(const core::OnStartSystem& event);
	void finishSystem(const core::OnFinishSystem& event);

private:
	entt::dispatcher* _dispatcher = nullptr;

	entt::entity _waterPlane= entt::null;
	entt::entity _waterParticle = entt::null;
};
DEFINE_SYSTEM_TRAITS(BucketTillingSystem)

