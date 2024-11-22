#include "pch.h"
#include "BucketTillingSystem.h"

#include <Animacore/Scene.h>

#include "McComponents.h"
#include "Animacore/CoreComponents.h"
#include <Animacore/RenderComponents.h>



BucketTillingSystem::BucketTillingSystem(core::Scene& scene) :
	ISystem(scene)
{
	_dispatcher = scene.GetDispatcher();
	_dispatcher->sink<core::OnStartSystem>().connect<&BucketTillingSystem::startSystem>(this);
	_dispatcher->sink<core::OnFinishSystem>().connect<&BucketTillingSystem::finishSystem>(this);
}

BucketTillingSystem::~BucketTillingSystem()
{
	_dispatcher->disconnect(this);
}

void BucketTillingSystem::operator()(core::Scene& scene, float tick)
{
	auto& registry = *scene.GetRegistry();
	auto& planeRenderer = registry.get<core::MeshRenderer>(_waterPlane);
	auto& waterRenderer = registry.get<core::ParticleSystem>(_waterParticle);

	auto view = registry.view<core::LocalTransform, mc::WaterBucket, core::Relationship, core::Sound>();

	for (auto&& [entity, local, waterBucket, relationShip, sound] : view.each())
	{
		Vector3 localEuler = local.rotation.ToEuler() / (DirectX::XM_PI / 180.f);
		if (abs(localEuler.x) > 50.f || abs(localEuler.y) > 50.f)
		{
			if (waterBucket.isFilled)
			{
				sound.isPlaying = true;
				sound.path = "Resources/Sound/tool/Bucket_Spoiled.wav";
				registry.patch<core::Sound>(entity);

				waterRenderer.isOn = true;
				//waterRenderer.instanceData.isReset = true;
				registry.patch<core::ParticleSystem>(_waterParticle);
				_dispatcher->enqueue<core::OnParticleTransformUpdate>(_waterParticle);
			}

			// 쏟아진다
			waterBucket.isFilled = false;
			planeRenderer.isOn = false;
			// 이벤트 보내기
			registry.patch<core::MeshRenderer>(_waterPlane);
		}
	}

}

void BucketTillingSystem::startSystem(const core::OnStartSystem& event)
{
	auto view = event.scene->GetRegistry()->view<core::LocalTransform, mc::WaterBucket, core::Relationship>();

	for (auto&& [entity, local, waterBucket, relationShip] : view.each())
	{
		for (auto& child : relationShip.children)
		{
			if (auto* renderer = event.scene->GetRegistry()->try_get<core::MeshRenderer>(child))
				_waterPlane = child;
			if (auto* renderer = event.scene->GetRegistry()->try_get<core::ParticleSystem>(child))
				_waterParticle = child;
		}

		break;
	}
}

void BucketTillingSystem::finishSystem(const core::OnFinishSystem& event)
{
	_waterPlane = entt::null;
	_waterParticle = entt::null;
}
