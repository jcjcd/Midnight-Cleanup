#include "pch.h"
#include "CharacterActionSystem.h"

#include "McComponents.h"

#include <Animacore/Scene.h>
#include <Animacore/CoreComponents.h>
#include <Animacore/RenderComponents.h>
#include <Animacore/RaycastHit.h>
#include <Animacore/PhysicsScene.h>
#include <Animacore/CoreSystemEvents.h>

#include "McTagsAndLayers.h"
#include <Animacore/InputSystem.h>

#include "McEvents.h"
#include <Animacore/CoreComponentInlines.h>
#include <Animacore/ParticleSystemPass.h>


namespace
{
	constexpr uint32_t targetLayerMask = layer::Stain::mask
		| layer::Furniture::mask
		| layer::Pickable::mask
		| layer::Hologram::mask
		| layer::Outline::mask
		| layer::Tool::mask
		| layer::Door::mask
		| layer::Wall::mask
		| layer::Switch::mask
		| layer::Default::mask;


	Vector3 calculateEulerForTool(mc::Tool::Type type)
	{
		switch (type)
		{
		case mc::Tool::Type::AXE:
			return { 39.5f, 100.1f, -80.00f };
		case mc::Tool::Type::MOP:
			return { -4.1f, -10.4f, 174.504f };
		case mc::Tool::Type::SPONGE:
			return { 0.0f, 102.800f, -98.000f };
		case mc::Tool::Type::FLASHLIGHT:
			return { -81.90f, 0.0f, 175.2f };
		default:
			return Vector3::Zero;
		}
	}

	Vector3 calculatePositionForTool(mc::Tool::Type type)
	{
		switch (type)
		{
		case mc::Tool::Type::AXE:
			return { -0.01f, -0.01f, -0.07f };
		case mc::Tool::Type::MOP:
			return { 0.02f, -0.04f, -0.23f };
		case mc::Tool::Type::SPONGE:
			return { -0.05f, -0.04f, 0.04f };
		case mc::Tool::Type::FLASHLIGHT:
			return { -0.01f, -0.035f, 0.03f };
		default:
			return Vector3::Zero;
		}
	}
}

mc::CharacterActionSystem::CharacterActionSystem(core::Scene& scene) :
	ISystem(scene),
	_dispatcher(scene.GetDispatcher())
{
	_dispatcher->sink<core::OnStartSystem>()
		.connect<&CharacterActionSystem::startSystem>(this);
	_dispatcher->sink<core::OnFinishSystem>()
		.connect<&CharacterActionSystem::finishSystem>(this);

	_dispatcher->sink<mc::OnProcessWipe>()
		.connect<&CharacterActionSystem::handleRaycastInteraction>(this);
	_dispatcher->sink<mc::OnDiscardTool>()
		.connect<&CharacterActionSystem::discardTool>(this);
}

mc::CharacterActionSystem::~CharacterActionSystem()
{
	_dispatcher->disconnect(this);
}

void mc::CharacterActionSystem::PreUpdate(core::Scene& scene, float tick)
{
	if (!scene.IsPlaying())
		return;

	auto registry = scene.GetRegistry();
	auto view = registry->view<core::WorldTransform, mc::RayCastingInfo>();

	for (auto&& [entity, world, rayCastingInfo] : view.each())
	{
		const auto& picking = registry->get<mc::Picking>(entity);

		// ���� ���� ���� ��ƼƼ �ʱ�ȭ
		for (const auto& hit : rayCastingInfo.hits)
		{
			if (!registry->valid(hit.entity))
				continue;

			auto* layer = registry->try_get<core::Layer>(hit.entity);

			// �ƿ����� ��Ȱ��ȭ
			if (layer && layer->mask & layer::Outline::mask)
			{
				auto& attributes = registry->get<core::RenderAttributes>(hit.entity);
				attributes.flags &= ~core::RenderAttributes::Flag::OutLine;

				// ��� �ִ� ��ü�� ��� ����
				if (layer->mask & layer::Pickable::mask)
				{
					if (picking.pickingObject == hit.entity)
						attributes.flags |= core::RenderAttributes::Flag::OutLine;
				}
			}
		}

		Vector3 rayOrigin = _rayCastingTransform->position;
		Vector3 rayDirection = _rayCastingTransform->matrix.Backward();
		rayDirection.Normalize();

		auto& hits = rayCastingInfo.hits;
		scene.GetPhysicsScene()->Raycast(rayOrigin, rayDirection,
			hits, 10,
			rayCastingInfo.interactableRange, targetLayerMask);

		// �Ÿ��� ����
		std::sort(hits.begin(), hits.end());
		//int size = rayCastingInfo.hits.size();


		//// Ÿ�� �ε���
		//int targetIndex = -1;

		//// Ÿ�� ���� ����
		//// 1. ù��° ����
		//// 2. ������ ��� ���� ��� �� ��° ���ǵ� ����
		//// 3. ������ ������ ����ĳ���� ���� ��� Ȧ�α׷��� ����
		//if (size > 0)
		//	targetIndex = 0;

		//for (auto i = 0; i < size; ++i)
		//{
		//	if (auto* snap = registry->try_get<mc::SnapAvailable>(hits[i].entity))
		//	{
		//		if (i <= 1)
		//			targetIndex = i;
		//		break;
		//	}
		//}

		//bool isPicking = false;
		//if (picking.pickingObject != entt::null && size > 1)
		//{
		//	isPicking = true;

		//	if (picking.pickingObject == hits[0].entity)
		//		targetIndex = 1;
		//	else
		//		targetIndex = 0;
		//}

		//if (targetIndex != -1)
		//	std::swap(hits[0], hits[targetIndex]);

		////if (size > 1)
		////	rayCastingInfo.hits.erase(hits.begin() + 1, hits.end());

		//if (!hits.empty() && !isPicking)
		//{
		//	if (auto* layer = registry->try_get<core::Layer>(hits[0].entity))
		//	{
		//		if (layer->mask & layer::Outline::mask)
		//		{
		//			auto& attributes = registry->get<core::RenderAttributes>(hits[0].entity);
		//			attributes.flags |= core::RenderAttributes::Flag::OutLine;
		//		}
		//	}
		//}
	}
}

void mc::CharacterActionSystem::operator()(core::Scene& scene, float tick)
{
	if (!scene.IsPlaying())
		return;

	auto& input = scene.GetRegistry()->ctx().get<core::Input>();
	auto view = scene.GetRegistry()->view<mc::Inventory>();

	if (!_animator)
		return;

	for (auto&& [entity, inventory] : view.each())
	{
		auto* currentTool = scene.GetRegistry()->try_get<Tool>(inventory.toolSlot.tools[inventory.currentTool]);
		_animator->parameters["ToolType"].value = currentTool ? static_cast<int>(currentTool->type) : static_cast<int>(Tool::Type::DEFAULT);

		if (inventory.toolSlot.tools[inventory.currentTool] == entt::null || currentTool->type == Tool::Type::DEFAULT)
		{
			pickUpTool(scene, input, inventory);
		}
		else
		{
			handleToolInteractions(scene, input, entity, inventory, currentTool->type, tick);
		}
	}
}

void mc::CharacterActionSystem::pickUpTool(core::Scene& scene, const core::Input& input, mc::Inventory& inventory)
{
	if (!_rayCastingInfo || input.keyStates[KEY(VK_LBUTTON)] != core::Input::State::Down)
		return;

	auto reigstry = scene.GetRegistry();


	auto& hits = _rayCastingInfo->hits;
	for (int i = 0; i < hits.size(); ++i)
	{
		if (i != 0)
			return;

		auto& hit = hits[i];
		if (auto* tool = reigstry->try_get<Tool>(hit.entity))
		{
			inventory.toolSlot.tools[inventory.currentTool] = hit.entity;
			if (auto* toolTransform = reigstry->try_get<core::WorldTransform>(hit.entity))
			{
				if (auto rigidbody = reigstry->try_get<core::Rigidbody>(hit.entity))
				{
					rigidbody->useGravity = false;
					rigidbody->isKinematic = false;
					rigidbody->isDisabled = true;
					reigstry->patch<core::Rigidbody>(hit.entity);
				}

				core::Entity(inventory.toolSlot.tools[inventory.currentTool], *reigstry).SetParent(_pivotEntity);
				auto pivotWorld = reigstry->get<core::WorldTransform>(_pivotEntity);
				auto position = calculatePositionForTool(tool->type);
				auto rotation = Quaternion::CreateFromYawPitchRoll(calculateEulerForTool(tool->type) * (DirectX::XM_PI / 180.f));
				toolTransform->position = Vector3::Transform(position, pivotWorld.matrix); // pivotWorld.position + pivotWorld.rotation * position;
				toolTransform->rotation = rotation * pivotWorld.rotation;
				reigstry->patch<core::WorldTransform>(hit.entity);



				_isPickedUp = true;
			}
			return;
		}
	}
}

void mc::CharacterActionSystem::handleToolInteractions(core::Scene& scene, const core::Input& input, entt::entity entity, mc::Inventory& inventory, Tool::Type curToolType, float tick)
{
	auto& registry = *scene.GetRegistry();

	if (input.keyStates[KEY(VK_LBUTTON)] == core::Input::State::Down &&
		global::mouseMode == global::MouseMode::Lock)
	{
		if (_isPickedUp)
			return;

		if (curToolType == Tool::Type::MOP)
		{
			_animator->parameters["ActionTrigger"].value = true;

		}
		else if (curToolType == Tool::Type::AXE)
		{
			_animator->parameters["ActionTrigger"].value = true;
		}
		else if (curToolType == Tool::Type::SPONGE)
		{
			_animator->parameters["ActionTrigger"].value = true;
		}
	}
	else if (input.keyStates[KEY(VK_LBUTTON)] == core::Input::State::Hold &&
		global::mouseMode == global::MouseMode::Lock)
	{
		if (_isPickedUp)
			return;

		if (curToolType == Tool::Type::MOP)
		{
			_animator->parameters["ActionTrigger"].value = true;
		}
		else if (curToolType == Tool::Type::AXE)
		{
			_animator->parameters["ActionTrigger"].value = true;
		}
	}
	else if (input.keyStates[KEY(VK_LBUTTON)] == core::Input::State::Up)
	{
		if (_isPickedUp)
			_isPickedUp = false;

		if (curToolType == Tool::Type::SPONGE)
		{
			endInteraction(scene);
		}
	}

	if (input.keyStates[KEY('F')] == core::Input::State::Down)
	{
		if (curToolType == Tool::Type::FLASHLIGHT)
			_dispatcher->enqueue<mc::OnHandleFlashLight>(scene, _flashlight, false);

		auto& sound = registry.get<core::Sound>(inventory.toolSlot.tools[inventory.currentTool]);
		sound.isPlaying = true;
		registry.patch<core::Sound>(inventory.toolSlot.tools[inventory.currentTool]);
	}

	if (_isInteracting)
	{
		continueInteraction(scene, input, inventory, tick);
	}
}

void mc::CharacterActionSystem::handleRaycastInteraction(const OnProcessWipe& event)
{
	if (!_rayCastingInfo)
		return;

	auto& scene = *event.scene;
	auto& registry = *scene.GetRegistry();

	entt::entity currentToolEntity = _inventory->toolSlot.tools[_inventory->currentTool];

	auto* durability = registry.try_get<Durability>(currentToolEntity);
	auto* currentTool = registry.try_get<Tool>(currentToolEntity);
	if (!durability || !currentTool)
		return;

	const auto& hits = _rayCastingInfo->hits;



	bool findStain = false;
	std::vector<entt::entity> findEntities;
	std::vector<entt::entity> newEntities;
	auto* mop = registry.try_get<Mop>(currentToolEntity);
	auto* sponge = registry.try_get<Sponge>(currentToolEntity);

	if (currentTool->type == Tool::Type::MOP && mop)
	{
		auto& sound = registry.get<core::Sound>(currentToolEntity);
		sound.isPlaying = true;
		sound.path = "Resources/Sound/tool/Mop_Cleaning.wav";
		registry.patch<core::Sound>(currentToolEntity);
	}

	float prevPercentage = durability->currentDurability / static_cast<float>(durability->maxDurability);

	for (const auto& hit : hits)
	{
		auto* targetTag = registry.try_get<core::Tag>(hit.entity);
		if (!targetTag)
			continue;

		if (targetTag->id == tag::Wall::id || targetTag->id == tag::Floor::id)
		{
			break;
		}
		else if (targetTag->id == tag::WaterBucket::id)
		{
			repairTool(scene, *_inventory, hit.entity);
			break;
		}

		if (durability->currentDurability <= 0)
			continue;

		auto* decal = registry.try_get<core::Decal>(hit.entity);
		auto* stain = registry.try_get<Stain>(hit.entity);
		if (!decal || !stain)
			continue;

		// ���⼭ ����ĳ��Ʈ �����̶� �븻 �����̶� dot �ؼ� �޸��̸� ����
		auto& decalTransform = registry.get<core::WorldTransform>(hit.entity);
		Vector3 decalNormal = Vector3::Transform(Vector3::Up, decalTransform.rotation);
		Vector3 rayDirection = _rayCastingTransform->matrix.Backward();

		if (rayDirection.Dot(decalNormal) > 0)
			continue;

		if (currentTool->type == Tool::Type::MOP && stain->isMopStain)
		{
			findStain = true;
			if (mop)
			{
				// �� ����� ���� ���� ��ȣ�ۿ��ϰ��ִ� ������� Ȯ��
				auto find = std::find(mop->interactingEntities.begin(), mop->interactingEntities.end(), hit.entity);
				if (find != mop->interactingEntities.end())
					findEntities.push_back(hit.entity);
				else
					newEntities.push_back(hit.entity);
			}
		}
		else if (currentTool->type == Tool::Type::SPONGE && stain->isSpongeStain)
		{
			_dispatcher->trigger<mc::OnInputLock>(OnInputLock{ true, true });
			_isInteracting = true;
			// ���� �������� interactEntity�� ����
			_interactingEntity = currentToolEntity;
		}
	}

	if (findStain && mop)
	{
		// �� ��� ��� ��ȣ�ۿ� ����� �߰� �Ǿ��� ���
		if (mop->interactingEntities.size() == findEntities.size())
		{
			// ��ȣ�ۿ� ����� ���� �� ���� �ʾ����� �� ��ƼƼ������ �߰�
			auto iter = newEntities.begin();
			for (int i = mop->interactingEntities.size(); i < mop->eraseCount; ++i)
			{
				// �� ��ƼƼ�� �Ѱ����� ������� ������ �극��ũ
				if (iter == newEntities.end())
					break;

				mop->interactingEntities.push_back(*iter);
				++iter;
			}
		}
		else
		{
			// �ϳ��� ��ȣ�ۿ� ����� �ٸ���
			// ��ȣ�ۿ� ��� �����
			mop->interactingEntities.clear();

			// findEntities�� �ִ� ������ ��� �߰�
			// ������ �����ͺ��ٴ� �����Ƿ�
			for (auto& entity : findEntities)
			{
				mop->interactingEntities.push_back(entity);
			}
			// ���� ������ ī��Ʈ �������� �߰�
			auto iter = newEntities.begin();
			for (int i = mop->interactingEntities.size(); i < mop->eraseCount; ++i)
			{
				// �� ��ƼƼ�� �Ѱ����� ������� ������ �극��ũ
				if (iter == newEntities.end())
					break;

				mop->interactingEntities.push_back(*iter);
				++iter;
			}
		}

		// ���⼭ ��� ����°� ó��
		for (auto& entity : mop->interactingEntities)
		{
			cleanStain(scene, *_inventory, entity);
		}
	}
	else if (mop)
	{
		mop->interactingEntities.clear();

		if (durability->currentDurability <= 0)
		{
			createNewStain(scene);
			auto& sound = registry.get<core::Sound>(currentToolEntity);
			sound.isPlaying = true;
			sound.path = "Resources/Sound/tool/Mop_Staining.wav";
			registry.patch<core::Sound>(currentToolEntity);
		}
	}

	if (mop)
	{
		auto& particle = registry.get<core::ParticleSystem>(_mopParticle);
		particle.isOn = true;
		particle.instanceData.isReset = true;
		registry.patch<core::ParticleSystem>(_mopParticle);
		scene.GetDispatcher()->enqueue<core::OnParticleTransformUpdate>(_mopParticle);

		updateMopMaterial(scene, *_inventory, durability, prevPercentage);
	}

	if (sponge)
	{
		updateSpongeMaterial(scene, *_inventory, durability, prevPercentage);
	}

}

void mc::CharacterActionSystem::discardTool(const mc::OnDiscardTool& event)
{
	auto registry = event.scene->GetRegistry();
	auto core = core::Entity(_inventory->toolSlot.tools[_inventory->currentTool], *registry);

	core.SetParent(entt::null);

	auto& rigid = core.Get<core::Rigidbody>();
	rigid.useGravity = true;
	rigid.isDisabled = false;

	_animator->parameters["ActionTrigger"].value = false;
	_animator->parameters["IsAction"].value = false;

	registry->patch<core::WorldTransform>(core);
	registry->patch<core::Rigidbody>(core);

	auto* currentTool = registry->try_get<Tool>(_inventory->toolSlot.tools[_inventory->currentTool]);
	if (currentTool->type == Tool::Type::FLASHLIGHT)
		_dispatcher->enqueue<mc::OnHandleFlashLight>(*event.scene, _flashlight, true);

	_inventory->toolSlot.tools[_inventory->currentTool] = entt::null;
}

void mc::CharacterActionSystem::repairTool(core::Scene& scene, mc::Inventory& inventory, entt::entity hitEntity)
{
	auto* durability = scene.GetRegistry()->try_get<Durability>(inventory.toolSlot.tools[inventory.currentTool]);
	auto* waterBucket = scene.GetRegistry()->try_get<WaterBucket>(hitEntity);
	if (!durability || !waterBucket || !waterBucket->isFilled)
		return;

	auto* bucketDurability = scene.GetRegistry()->try_get<Durability>(hitEntity);
	if (!bucketDurability)
		return;

	auto prevDurability = bucketDurability->currentDurability / static_cast<float>(bucketDurability->maxDurability);

	// mop�ϰ��
	auto* mop = scene.GetRegistry()->try_get<Mop>(inventory.toolSlot.tools[inventory.currentTool]);
	if (mop)
	{
		int repairDurability = durability->maxDurability - durability->currentDurability;
		if (bucketDurability->currentDurability >= repairDurability)
		{
			durability->currentDurability = durability->maxDurability;
			bucketDurability->currentDurability -= repairDurability;
		}
		else
		{
			if (bucketDurability->currentDurability == 0 && !scene.GetRegistry()->ctx().get<CaptionLog>().isShown[CaptionLog::Caption_MopDirtyWater])
			{
				_dispatcher->enqueue<OnShowCaption>(scene, CaptionLog::Caption_MopDirtyWater, 2.5f);
				scene.GetRegistry()->ctx().get<CaptionLog>().isShown[CaptionLog::Caption_MopDirtyWater] = true;
			}

			durability->currentDurability += bucketDurability->currentDurability;
			bucketDurability->currentDurability = 0;
		}
	}
	else
	{
		int repairDurability = durability->maxDurability - durability->currentDurability;
		repairDurability /= 10;
		if (bucketDurability->currentDurability >= repairDurability)
		{
			durability->currentDurability += repairDurability * 10;
			bucketDurability->currentDurability -= repairDurability;
		}
		else
		{
			if (bucketDurability->currentDurability == 0 && !scene.GetRegistry()->ctx().get<CaptionLog>().isShown[CaptionLog::Caption_MopDirtyWater])
			{
				_dispatcher->enqueue<OnShowCaption>(scene, CaptionLog::Caption_MopDirtyWater, 2.5f);
				scene.GetRegistry()->ctx().get<CaptionLog>().isShown[CaptionLog::Caption_MopDirtyWater] = true;
			}

			durability->currentDurability += bucketDurability->currentDurability * 10;
			bucketDurability->currentDurability = 0;
		}
	}

	auto currentDurability = bucketDurability->currentDurability / static_cast<float>(bucketDurability->maxDurability);

	if (auto* relationship = scene.GetRegistry()->try_get<core::Relationship>(hitEntity))
	{
		for (auto& child : relationship->children)
		{
			auto* meshRenderer = scene.GetRegistry()->try_get<core::MeshRenderer>(child);
			if (!meshRenderer)
				continue;

			if (prevDurability > 0.5f and currentDurability <= 0.5f)
			{
				auto iter = meshRenderer->materialStrings.begin();
				*iter = "./Resources/Materials/Water_v2.material";
				meshRenderer->materials.clear();
			}
			else if (prevDurability > 0.2f and currentDurability <= 0.2f)
			{
				auto iter = meshRenderer->materialStrings.begin();
				*iter = "./Resources/Materials/Water_v3.material";
				meshRenderer->materials.clear();
			}
		}
	}


}

void mc::CharacterActionSystem::interactWithStain(core::Scene& scene, mc::Inventory& inventory, entt::entity hitEntity)
{
	// �����丵 ����

}

void mc::CharacterActionSystem::cleanStain(core::Scene& scene, mc::Inventory& inventory, entt::entity hitEntity)
{
	auto* durability = scene.GetRegistry()->try_get<Durability>(inventory.toolSlot.tools[inventory.currentTool]);
	auto* stainDurability = scene.GetRegistry()->try_get<Durability>(hitEntity);
	auto* decal = scene.GetRegistry()->try_get<core::Decal>(hitEntity);

	if (!durability || !stainDurability || !decal)
		return;

	if (durability->currentDurability <= 0)
		return;

	durability->currentDurability -= 1;
	stainDurability->currentDurability -= 1;
	//decal->fadeFactor = stainDurability->currentDurability / static_cast<float>(stainDurability->maxDurability);
	// fade factor�� �ƹ��� 0.5 ~ 1.0�� �� �հ�
	decal->fadeFactor = 0.5f + (stainDurability->currentDurability / static_cast<float>(stainDurability->maxDurability)) * 0.5f;

	if (stainDurability->currentDurability <= 0)
	{
		_dispatcher->trigger<mc::OnDecreaseStain>({ scene, hitEntity });
		scene.DestroyEntity(hitEntity);
	}
}

void mc::CharacterActionSystem::updateMopMaterial(core::Scene& scene, mc::Inventory& inventory, Durability* durability, float prevPercentage)
{
	float percentage = durability->currentDurability / static_cast<float>(durability->maxDurability);
	if ((percentage < 0.2f && prevPercentage >= 0.2f) || (percentage < 0.5f && prevPercentage >= 0.5f))
	{
		auto* meshRenderer = scene.GetRegistry()->try_get<core::MeshRenderer>(inventory.toolSlot.tools[inventory.currentTool]);
		if (!meshRenderer)
			return;

		auto iter = meshRenderer->materialStrings.begin();
		*iter = percentage < 0.2f ? "./Resources/Materials/Broon_v3.material" : "./Resources/Materials/Broon_v2.material";
		++iter;
		*iter = percentage < 0.2f ? "./Resources/Materials/Fetlock_v3.material" : "./Resources/Materials/Fetlock_v2.material";

		meshRenderer->materials.clear();
	}

	// ����������
	if ((percentage >= 0.5f && prevPercentage < 0.5f) || (percentage >= 0.2f && prevPercentage < 0.2f))
	{
		auto* meshRenderer = scene.GetRegistry()->try_get<core::MeshRenderer>(inventory.toolSlot.tools[inventory.currentTool]);
		if (!meshRenderer)
			return;

		auto iter = meshRenderer->materialStrings.begin();
		*iter = percentage >= 0.5f ? "./Resources/Materials/Broon_v1.material" : "./Resources/Materials/Broon_v2.material";
		++iter;
		*iter = percentage >= 0.5f ? "./Resources/Materials/Fetlock_v1.material" : "./Resources/Materials/Fetlock_v2.material";

		meshRenderer->materials.clear();
	}
}

void mc::CharacterActionSystem::updateSpongeMaterial(core::Scene& scene, mc::Inventory& inventory,
	Durability* durability, float prevPercentage)
{
	float percentage = durability->currentDurability / static_cast<float>(durability->maxDurability);
	if ((percentage < 0.2f && prevPercentage >= 0.2f) || (percentage < 0.5f && prevPercentage >= 0.5f))
	{
		auto* meshRenderer = scene.GetRegistry()->try_get<core::MeshRenderer>(inventory.toolSlot.tools[inventory.currentTool]);
		if (!meshRenderer)
			return;

		auto iter = meshRenderer->materialStrings.begin();
		*iter = percentage < 0.2f ? "./Resources/Materials/Sponge_v3.material" : "./Resources/Materials/Sponge_v2.material";

		meshRenderer->materials.clear();
	}

	// ����������
	if ((percentage >= 0.5f && prevPercentage < 0.5f) || (percentage >= 0.2f && prevPercentage < 0.2f))
	{
		auto* meshRenderer = scene.GetRegistry()->try_get<core::MeshRenderer>(inventory.toolSlot.tools[inventory.currentTool]);
		if (!meshRenderer)
			return;

		auto iter = meshRenderer->materialStrings.begin();
		*iter = percentage >= 0.5f ? "./Resources/Materials/Sponge_v1.material" : "./Resources/Materials/Sponge_v2.material";

		meshRenderer->materials.clear();
	}
}

void mc::CharacterActionSystem::endInteraction(core::Scene& scene)
{
	_dispatcher->trigger<mc::OnInputLock>(OnInputLock{ false, false });
	_isInteracting = false;

	// �������� ���� Ʈ�������� ������� �����ش�.
	auto* transform = scene.GetRegistry()->try_get<core::LocalTransform>(_interactingEntity);
	if (transform)
	{
		transform->position = calculatePositionForTool(Tool::Type::SPONGE);
		transform->rotation = Quaternion::CreateFromYawPitchRoll(calculateEulerForTool(Tool::Type::SPONGE) * (DirectX::XM_PI / 180.f));
		scene.GetRegistry()->patch<core::LocalTransform>(_interactingEntity);
	}


	_interactingEntity = entt::null;

	_interactingOffsetX = 0.0f;
	_interactingOffsetY = 0.0f;
}

void mc::CharacterActionSystem::continueInteraction(core::Scene& scene, const core::Input& input, mc::Inventory& inventory, float tick)
{
	if (!_rayCastingTransform || !_rayCastingInfo)
		return;

	std::vector<physics::RaycastHit> hits;

	_interactingOffsetX += input.mouseDeltaRotation.x;
	_interactingOffsetY += input.mouseDeltaRotation.y;

	Vector3 rayOrigin = _rayCastingTransform->position;
	Quaternion rayRotation = _rayCastingTransform->rotation;

	float rotationSpeed = 0.3f; // ȸ�� �ӵ� ����
	float yaw = _interactingOffsetX * rotationSpeed; // �¿� ȸ�� (Y�� ȸ��)
	float pitch = _interactingOffsetY * rotationSpeed; // ���� ȸ�� (X�� ȸ��)

	// Quaternion�� ����Ͽ� ȸ�� ���� (Y�� ȸ�� + X�� ȸ��)
	Quaternion yawRotation = Quaternion::CreateFromAxisAngle(Vector3::Up, yaw);
	Quaternion pitchRotation = Quaternion::CreateFromAxisAngle(Vector3::Right, pitch);

	Quaternion temp = pitchRotation * rayRotation;
	Quaternion temp2 = temp * yawRotation;

	// ȸ�� ����
	Vector3 rayDirection = Vector3::Transform(Vector3::UnitZ, temp2);

	rayDirection.Normalize();


	// �ϴ� �Ÿ��� �ִ� �浹�� 10�����Ѵ�.?
	scene.GetPhysicsScene()->Raycast(rayOrigin, rayDirection,
		hits, 10,
		10, targetLayerMask);

	// �Ÿ��� ����
	std::sort(hits.begin(), hits.end(), [](const physics::RaycastHit& a, const physics::RaycastHit& b)
		{
			return a.distance < b.distance;
		});


	auto* transform = scene.GetRegistry()->try_get<core::WorldTransform>(_interactingEntity);
	bool isHit = false;
	for (auto& hit : hits)
	{
		// ���� ������ ������ ��ġ�� �ű�� �Űܺ���
		auto* targetLayer = scene.GetRegistry()->try_get<core::Layer>(hit.entity);
		if (targetLayer && (targetLayer->mask & layer::Wall::mask))
		{
			if (!isHit)
			{
				transform->position = hit.point + hit.normal * 0.1f;
				Vector3 convertNormal = -Vector3{ hit.normal.x, hit.normal.y, hit.normal.z };
				transform->rotation = Quaternion::LookRotation(convertNormal, Vector3::Forward);
				transform->rotation *= Quaternion::CreateFromYawPitchRoll(DirectX::XM_PIDIV2, 0.0f, 0.0f);
				scene.GetRegistry()->patch<core::WorldTransform>(_interactingEntity);
				isHit = true;
			}
		}
		else if (targetLayer && (targetLayer->mask & layer::Stain::mask))
		{
			auto* durability = scene.GetRegistry()->try_get<Durability>(_interactingEntity);
			auto* stainDurability = scene.GetRegistry()->try_get<Durability>(hit.entity);
			auto* decal = scene.GetRegistry()->try_get<core::Decal>(hit.entity);

			if (!durability || !stainDurability || !decal)
				continue;

			// ��Ÿ�� ������ ��Ƽ��
			if (input.mouseDeltaPosition.x == 0.0f && input.mouseDeltaPosition.y == 0.0f)
				continue;

			float prevPercentage = durability->currentDurability / static_cast<float>(durability->maxDurability);
			// �ϵ��ڵ��� ����� �ӵ�
			_accumulator += tick * 50;
			while (_accumulator >= 1)
			{
				if (durability->currentDurability > 0)
				{
					durability->currentDurability -= 1;
					stainDurability->currentDurability -= 1;
					decal->fadeFactor = stainDurability->currentDurability / static_cast<float>(stainDurability->maxDurability);

					if (stainDurability->currentDurability <= 10)
					{
						_dispatcher->trigger<mc::OnDecreaseStain>({ scene, hit.entity });
						scene.DestroyEntity(hit.entity);
						_accumulator = 0;
					}
				}
				else
				{
					_accumulator = 0;
				}
				_accumulator -= 1;
			}

			auto& sound = scene.GetRegistry()->get<core::Sound>(inventory.toolSlot.tools[inventory.currentTool]);
			if (!sound.isPlaying)
			{
				sound.isPlaying = true;
				scene.GetRegistry()->patch<core::Sound>(inventory.toolSlot.tools[inventory.currentTool]);
			}

			updateSpongeMaterial(scene, inventory, durability, prevPercentage);
		}

	}

	if (!isHit)
	{
		_interactingOffsetX = 0;
		_interactingOffsetY = 0;
	}
}

void mc::CharacterActionSystem::createNewStain(core::Scene& scene)
{
	const auto& hits = _rayCastingInfo->hits;

	entt::entity roomTrigger = entt::null;
	// �켱 �ѹ����� ���Ƽ� ���� �ָ��ִ� roomtrigger �� ã�´�.
	std::vector<physics::RaycastHit> roomHits;
	Vector3 rayOrigin = _rayCastingTransform->position;
	Vector3 rayDirection = _rayCastingTransform->matrix.Backward();
	rayDirection.Normalize();

	scene.GetPhysicsScene()->Raycast(rayOrigin, rayDirection,
		roomHits, 10,
		5.f, layer::RoomEnter::mask);

	// �Ÿ��� ����
	std::sort(roomHits.begin(), roomHits.end());

	for (const auto& hit : roomHits)
	{
		roomTrigger = hit.entity;
	}



	for (int i = 0; i < hits.size(); ++i)
	{
		auto hitEntity = hits[i].entity;
		auto* targetLayer = scene.GetRegistry()->try_get<core::Layer>(hitEntity);
		if (targetLayer && (targetLayer->mask & layer::Wall::mask))
		{
			// ���� Room�� Ÿ�ٿ� ���� ������ ����
			mc::Room* hitRoom = nullptr;
			if (auto* room = scene.GetRegistry()->try_get<mc::Room>(roomTrigger))
			{
				hitRoom = room;
			}
			else
			{
				hitRoom = scene.GetRegistry()->try_get<mc::Room>(_player);
			}

			Vector3 hitPoint = hits[i].point;
			Vector3 hitNormal = hits[i].normal;

			auto newEntity = scene.LoadPrefab("./Resources/Prefabs/Stain/Decal_Mop_Big_Beginning_06.prefab");
			auto& worldTransform = scene.GetRegistry()->get<core::WorldTransform>(newEntity);

			Vector3 up(0.0f, 1.0f, 0.0f);
			Quaternion rotation;

			if (hitNormal.Dot(up) < 0.999f)
			{
				Vector3 axis = up.Cross(hitNormal);
				axis.Normalize();
				hitNormal.Normalize();
				float angle = acos(up.Dot(hitNormal));
				rotation = Quaternion::CreateFromAxisAngle(axis, angle);
			}
			else
			{
				rotation = Quaternion::Identity;
			}

			worldTransform.position = hitPoint;
			worldTransform.rotation = rotation;
			//worldTransform.scale;
			scene.GetRegistry()->patch<core::WorldTransform>(newEntity);

			auto& room = scene.GetRegistry()->get<mc::Room>(newEntity);
			room.type = hitRoom->type;

			_dispatcher->trigger<mc::OnIncreaseStain>({ scene, newEntity });

			auto& log = scene.GetRegistry()->ctx().get<CaptionLog>();
			if (!log.isShown[CaptionLog::Caption_MopDirty])
			{
				log.isShown[CaptionLog::Caption_MopDirty] = true;
				log.isShown[CaptionLog::Caption_MopGotoOutdoor] = true;

				_dispatcher->enqueue<OnShowCaption>(scene, CaptionLog::Caption_MopDirty, 2.5f);
				_dispatcher->enqueue<OnShowCaption>(scene, CaptionLog::Caption_MopGotoOutdoor, 2.5f);
			}

			return;
		}
	}
}

void mc::CharacterActionSystem::startSystem(const core::OnStartSystem& event)
{
	while (ShowCursor(false) >= 0);
	global::mouseMode = global::MouseMode::Lock;

	auto registry = event.scene->GetRegistry();

	{
		auto view = registry->view<mc::CharacterMovement>();
		for (auto&& [entity, movement] : view.each())
		{
			_player = entity;
		}
	}


	{
		auto view = registry->view<mc::RayCastingInfo>();
		for (auto&& [entity, rayCastingInfo] : view.each())
		{
			_rayCastingInfo = &rayCastingInfo;
		}
	}

	{
		auto view = registry->view<mc::PivotSlot>();
		for (auto&& [entity, pivotSlot] : view.each())
		{
			if (pivotSlot.isUsing)
			{
				_pivotEntity = entity;
				break;
			}
		}
	}

	{
		auto view = registry->view<core::Camera, core::WorldTransform>();
		for (auto&& [entity, camera, world] : view.each())
		{
			if (camera.isActive)
			{
				_rayCastingTransform = &world;
				break;
			}
		}
	}

	{
		auto view = registry->view<core::Animator>();
		for (auto&& [entity, animator] : view.each())
		{
			_animator = &animator;
			break;
		}
	}

	// inventory
	{
		auto view = registry->view<mc::Inventory>();
		for (auto&& [entity, inventory] : view.each())
		{
			_inventory = &inventory;
			break;
		}
	}

	// flashlight
	{
		auto view = registry->view<mc::Flashlight>();
		for (auto&& [entity, flashlight] : view.each())
		{
			_flashlight = entity;
			break;
		}
	}

	{
		if (_flashlight != entt::null)
		{
			if (auto* relationship = registry->try_get<core::Relationship>(_flashlight))
			{
				auto& parentRelationship = registry->get<core::Relationship>(relationship->parent);

				for (auto& child : parentRelationship.children)
				{
					if (auto* particle = registry->try_get<core::ParticleSystem>(child))
					{
						_mopParticle = child;
						break;
					}
				}
			}
		}
	}
}

void mc::CharacterActionSystem::finishSystem(const core::OnFinishSystem& event)
{
	while (::ShowCursor(true) < 0);

	_flashlight = entt::null;
	_mopParticle = entt::null;
}