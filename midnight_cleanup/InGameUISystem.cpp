#include "pch.h"
#include "InGameUiSystem.h"
#include "McComponents.h"
#include "McEvents.h"

#include "McTagsAndLayers.h"
#include "Animacore/CoreComponents.h"
#include "Animacore/CorePhysicsComponents.h"
#include "Animacore/CoreTagsAndLayers.h"
#include "Animacore/RenderComponents.h"
#include "Animacore/Scene.h"
#include "Animacore/CoreComponentInlines.h"

#include "McEvents.h"

mc::InGameUiSystem::InGameUiSystem(core::Scene& scene)
	: ISystem(scene)
	, _dispatcher(scene.GetDispatcher())
{
	_dispatcher->sink<core::OnPreStartSystem>()
		.connect<&InGameUiSystem::preStartSystem>(this);
	_dispatcher->sink<core::OnStartSystem>()
		.connect<&InGameUiSystem::startSystem>(this);
	_dispatcher->sink<core::OnPostStartSystem>()
		.connect<&InGameUiSystem::postStartSystem>(this);
	_dispatcher->sink<core::OnFinishSystem>()
		.connect<&InGameUiSystem::finishSystem>(this);

	_dispatcher->sink<OnDecreaseStain>()
		.connect<&InGameUiSystem::decreaseStain>(this);
	_dispatcher->sink<OnIncreaseStain>()
		.connect<&InGameUiSystem::increaseStain>(this);


	// 자막 ui 이벤트
	_dispatcher->sink<mc::OnShowCaption>()
		.connect<&InGameUiSystem::showCaption>(this);
}

mc::InGameUiSystem::~InGameUiSystem()
{
	_dispatcher->disconnect(this);

}

void mc::InGameUiSystem::operator()(core::Scene& scene, float tick)
{
	if (!scene.IsPlaying())
		return;

	auto registry = scene.GetRegistry();
	const auto& input = registry->ctx().get<core::Input>();

	// Camera Fixed UI
	if (_player != entt::null && _hoverUI != entt::null)
	{
		const auto& ray = registry->get<mc::RayCastingInfo>(_player);
		const auto& picking = registry->get<mc::Picking>(_player);
		const auto& inventory = registry->get<mc::Inventory>(_player);
		auto& hoverUI = registry->get<core::Text>(_hoverUI);
		auto& tooltipUI = registry->get<core::UICommon>(_tooltipUI);
		auto& aimUI = registry->get<core::UICommon>(_aimUI);
		auto& furnitureHPUI = registry->get<core::UICommon>(_furnitureHPUI);
		furnitureHPUI.isOn = false;

		auto& currentToolType = inventory.toolType[inventory.currentTool];

		bool isWorkTerminal = false;
		bool isDoor = false;
		bool isSwitch = false;
		bool isTapWater = false;
		bool isHologram = false;
		bool isPickable = false;
		bool isFurniture = false;
		bool isMopStain = false;
		bool isSpongeStain = false;
		bool isTrash = false;

		// 자막
		if (!_queuedCaptions.empty())
		{
			auto& [text, time, index] = _queuedCaptions.front();
			time -= tick;

			if (time < 0.f)
			{
				_queuedCaptions.pop();
				hoverUI.text = "";

				// 재등장 가능 자막
				if (index == CaptionLog::Caption_MopDirtyWater)
				{
					_captionLog->isShown[CaptionLog::Caption_MopDirtyWater] = false;
				}
			}
			else
			{
				hoverUI.text = text;
			}
		}
		blinking(*registry);

		std::vector <std::pair<entt::entity, core::Tag* >> tags;

		for (const auto& hit : ray.hits)
		{
			if (auto* tag = registry->try_get<core::Tag>(hit.entity))
				tags.push_back({ hit.entity,tag });
		}

		entt::entity firstHit = entt::null;

		for (const auto& hit : ray.hits)
		{
			firstHit = hit.entity;
			const auto tag = registry->try_get<core::Tag>(firstHit);

			if (!tag)
				break;

			if (tag->id == tag::WorkTerminal::id)
				isWorkTerminal = true;
			else if (tag->id == tag::Door::id)
				isDoor = true;
			else if (tag->id == tag::Switch::id)
				isSwitch = true;
			else if (tag->id == tag::Hologram::id)
			{
				if (picking.pickingObject != entt::null)
				{
					if (auto* snap = registry->try_get<mc::SnapAvailable>(picking.pickingObject))
					{
						if (snap->placedSpot == firstHit)
							isHologram = true;
					}
				}
			}
			else if (tag->id == tag::Furniture::id)
			{
				isFurniture = true;
				auto& durability = registry->get<mc::Durability>(firstHit);
				furnitureHPUI.percentage = static_cast<float>(durability.currentDurability) / static_cast<float>(durability.maxDurability);


				if (!_captionLog->isShown[CaptionLog::Caption_FurnitureFirst])
				{
					_queuedCaptions.emplace(CaptionLog::captions[CaptionLog::Caption_FurnitureFirst], 3.0f);
					_captionLog->isShown[CaptionLog::Caption_FurnitureFirst] = true;

					bool hasAxe = false;
					for (const auto tool : inventory.toolType)
					{
						if (tool == Tool::Type::AXE)
							hasAxe = true;
					}
					if (!hasAxe)
					{
						_queuedCaptions.emplace(CaptionLog::captions[CaptionLog::Caption_FurnitureNotTakeAxe], 3.0f);
						_captionLog->isShown[CaptionLog::Caption_FurnitureNotTakeAxe] = true;
					}
				}
			}
			else if (tag->id == tag::Stain::id)
			{
				registry->get<mc::Stain>(firstHit).isMopStain ? isMopStain = true : isSpongeStain = true;

				if (isMopStain && !_captionLog->isShown[CaptionLog::Caption_MopStainFirst])
				{
					_queuedCaptions.emplace(CaptionLog::captions[CaptionLog::Caption_MopStainFirst], 3.0f);
					_captionLog->isShown[CaptionLog::Caption_MopStainFirst] = true;

					bool hasMop = false;
					for (const auto tool : inventory.toolType)
					{
						if (tool == Tool::Type::MOP)
							hasMop = true;
					}
					if (!hasMop)
					{
						_queuedCaptions.emplace(CaptionLog::captions[CaptionLog::Caption_MopStainNotTakeMop], 3.0f);
						_captionLog->isShown[CaptionLog::Caption_MopStainNotTakeMop] = true;
					}
				}

				if (isSpongeStain && !_captionLog->isShown[CaptionLog::Caption_SpongeStainFirst])
				{
					_queuedCaptions.emplace(CaptionLog::captions[CaptionLog::Caption_SpongeStainFirst], 3.0f);
					_captionLog->isShown[CaptionLog::Caption_SpongeStainFirst] = true;

					bool hasSponge = false;
					for (const auto tool : inventory.toolType)
					{
						if (tool == Tool::Type::MOP)
							hasSponge = true;
					}
					if (!hasSponge)
					{
						_queuedCaptions.emplace(CaptionLog::captions[CaptionLog::Caption_SpongeStainNotTakeSponge], 3.0f);
						_captionLog->isShown[CaptionLog::Caption_SpongeStainNotTakeSponge] = true;
					}
				}
			}
			else if (tag->id == tag::Trash::id or tag->id == tag::Mess::id
				or tag->id == tag::Tool::id)
			{
				isPickable = true;

				if (tag->id == tag::Trash::id)
				{
					isTrash = true;
					if (!_captionLog->isShown[CaptionLog::Caption_TrashFirst])
					{
						_queuedCaptions.emplace(CaptionLog::captions[CaptionLog::Caption_TrashFirst], 3.0f);
						_captionLog->isShown[CaptionLog::Caption_TrashFirst] = true;
					}

					if (!_captionLog->isShown[CaptionLog::Caption_TrashWithNoHands])
					{
						bool noHands = true;
						for (const auto tool : inventory.toolType)
						{
							if (tool == Tool::Type::DEFAULT)
								noHands = false;
						}
						if (noHands)
						{
							_queuedCaptions.emplace(CaptionLog::captions[CaptionLog::Caption_TrashWithNoHands], 3.0f);
							_captionLog->isShown[CaptionLog::Caption_TrashWithNoHands] = true;
						}
					}
				}
				else if (!_captionLog->isShown[CaptionLog::Caption_MessFirst] && tag->id == tag::Mess::id)
				{
					_queuedCaptions.emplace(CaptionLog::captions[CaptionLog::Caption_MessFirst], 3.0f);
					_captionLog->isShown[CaptionLog::Caption_MessFirst] = true;
				}
			}
			else if (tag->id == tag::WaterBucket::id)
				isTapWater = isPickable = true;

			break;
		}

		if (!isHologram)
		{
			for (auto&& [entity, tag] : tags)
			{
				if (tag->id == tag::Hologram::id)
				{
					if (picking.pickingObject != entt::null)
					{
						if (auto* snap = registry->try_get<mc::SnapAvailable>(picking.pickingObject))
						{
							if (snap->placedSpot == entity)
								isHologram = true;
						}
					}
				}
			}
		}

		if (isWorkTerminal)		// todo 게임 시작 전후 처리 필요
		{
			furnitureHPUI.isOn = false;
			tooltipUI.textureString = tooltipFileStr[Interactable];
			aimUI.textureString = aimFileStr[Terminal];
		}
		else if (isDoor)
		{
			furnitureHPUI.isOn = false;
			tooltipUI.textureString = tooltipFileStr[Interactable];
			aimUI.textureString = aimFileStr[Hand];
		}
		else if (isSwitch)
		{
			furnitureHPUI.isOn = false;
			tooltipUI.textureString = tooltipFileStr[Interactable];
			aimUI.textureString = aimFileStr[Switch];
		}
		else if (picking.pickingObject != entt::null)
		{
			furnitureHPUI.isOn = false;

			// 피킹 오브젝트의 태그를 가져온다.
			auto& pickingObjectTag = registry->get<core::Tag>(picking.pickingObject);

			if (isHologram)
			{
				if (pickingObjectTag.id == tag::WaterBucket::id)
				{
					tooltipUI.textureString = tooltipFileStr[DropDown];
					aimUI.textureString = aimFileStr[FillWater];
				}
				else
				{
					tooltipUI.textureString = tooltipFileStr[DropDown];
					aimUI.textureString = aimFileStr[Organize];
				}
			}
			else
			{
				tooltipUI.textureString = tooltipFileStr[DropDown];
				aimUI.textureString = aimFileStr[Default];
			}
		}
		else if (currentToolType != Tool::Type::DEFAULT)
		{
			if (currentToolType == Tool::Type::AXE)
			{
				tooltipUI.textureString = tooltipFileStr[DropDown];
				aimUI.textureString = aimFileStr[Default];

				if (isFurniture)
				{
					aimUI.textureString = aimFileStr[Axe];
					furnitureHPUI.isOn = true;
				}
			}
			else if (currentToolType == Tool::Type::MOP)
			{
				furnitureHPUI.isOn = false;
				tooltipUI.textureString = tooltipFileStr[DropDown];
				aimUI.textureString = aimFileStr[Default];

				if (isMopStain or isTapWater)
					aimUI.textureString = aimFileStr[Mop];
			}
			else if (currentToolType == Tool::Type::SPONGE)
			{
				furnitureHPUI.isOn = false;
				tooltipUI.textureString = tooltipFileStr[RubSponge];
				aimUI.textureString = aimFileStr[Default];

				if (isSpongeStain)
					aimUI.textureString = aimFileStr[Sponge];
			}
			else
			{
				furnitureHPUI.isOn = false;
				tooltipUI.textureString = tooltipFileStr[FlashlightOnOff];
				aimUI.textureString = aimFileStr[Flashlight];
			}
		}
		else if (isPickable)
		{
			furnitureHPUI.isOn = false;
			tooltipUI.textureString = tooltipFileStr[Interactable];

			if (isTrash)
				aimUI.textureString = aimFileStr[Trash];
			else
				aimUI.textureString = aimFileStr[Hand];
		}
		else
		{
			furnitureHPUI.isOn = false;
			aimUI.textureString = aimFileStr[Default];
		}
	}

	if (input.keyStates[KEY('T')] == core::Input::State::Down)
	{
		_showProgress = !_showProgress;

		for (auto&& [entity, tag] : registry->view<core::Tag>().each())
		{
			if (tag.id == tag::ProgressUI::id)
			{
				auto* text = registry->try_get<core::Text>(entity);
				auto* uiCommon = registry->try_get<core::UICommon>(entity);

				if (text)
					text->isOn = _showProgress;
				else if (uiCommon)
					uiCommon->isOn = _showProgress;
			}
		}

		if (auto* uiCommon = registry->try_get<core::UICommon>(_progressUI))
			uiCommon->isOn = _showProgress;
	}

	if (_questUI && _showProgress)
	{
		const auto& room = registry->get<mc::Room>(_player).type;

		auto& placeUI = registry->get<core::Text>(_placeUI);
		placeUI.text = mc::Room::name[room];

		{
			auto& countUI = registry->get<core::Text>(_questUI->_furnitureCounter);
			countUI.text = std::to_string(_questUI->cleanedFurnitureInRoom[room]) + " / " + std::to_string(_questUI->totalFurnitureInRoom[room]);
			auto& uiCommon = registry->get<core::UICommon>(_questUI->_furnitureBar);

			if (_questUI->totalFurnitureInRoom[room] != 0)
				uiCommon.percentage = static_cast<float>(_questUI->cleanedFurnitureInRoom[room]) / static_cast<float>(_questUI->totalFurnitureInRoom[room]);
			else
				uiCommon.percentage = 1.0f;
		}

		{
			auto& countUI = registry->get<core::Text>(_questUI->_mopStainCounter);
			countUI.text = std::to_string(_questUI->cleanedMopStainsInRoom[room]) + " / " + std::to_string(_questUI->totalMopStainsInRoom[room]);
			auto& uiCommon = registry->get<core::UICommon>(_questUI->_mopStainBar);

			if (_questUI->totalMopStainsInRoom[room] != 0)
				uiCommon.percentage = static_cast<float>(_questUI->cleanedMopStainsInRoom[room]) / static_cast<float>(_questUI->totalMopStainsInRoom[room]);
			else
				uiCommon.percentage = 1.0f;
		}

		{
			auto& countUI = registry->get<core::Text>(_questUI->_spongeStainCounter);
			countUI.text = std::to_string(_questUI->cleanedSpongeStainInRoom[room]) + " / " + std::to_string(_questUI->totalSpongeStainInRoom[room]);
			auto& uiCommon = registry->get<core::UICommon>(_questUI->_spongeStainBar);

			if (_questUI->totalSpongeStainInRoom[room] != 0)
				uiCommon.percentage = static_cast<float>(_questUI->cleanedSpongeStainInRoom[room]) / static_cast<float>(_questUI->totalSpongeStainInRoom[room]);
			else
				uiCommon.percentage = 1.0f;
		}

		{
			auto& countUI = registry->get<core::Text>(_questUI->_trashCounter);
			countUI.text = std::to_string(_questUI->cleanedTrashes) + " / " + std::to_string(_questUI->totalTrashes);
			auto& uiCommon = registry->get<core::UICommon>(_questUI->_trashBar);

			if (_questUI->totalTrashes != 0)
				uiCommon.percentage = static_cast<float>(_questUI->cleanedTrashes) / static_cast<float>(_questUI->totalTrashes);
			else
				uiCommon.percentage = 1.0f;
		}

		{
			auto& countUI = registry->get<core::Text>(_questUI->_messCounter);
			countUI.text = std::to_string(_questUI->cleanedMessInRoom[room]) + " / " + std::to_string(_questUI->totalMessInRoom[room]);
			auto& uiCommon = registry->get<core::UICommon>(_questUI->_messBar);

			if (_questUI->totalMessInRoom[room] != 0)
				uiCommon.percentage = static_cast<float>(_questUI->cleanedMessInRoom[room]) / static_cast<float>(_questUI->totalMessInRoom[room]);
			else
				uiCommon.percentage = 1.0f;
		}

		{
			uint32_t currentSum = 0;
			uint32_t totalSum = 0;

			for (auto i = 1; i < mc::Room::Type::End; ++i)
			{
				currentSum += _questUI->cleanedFurnitureInRoom[i];
				currentSum += _questUI->cleanedMopStainsInRoom[i];
				currentSum += _questUI->cleanedSpongeStainInRoom[i];
				currentSum += _questUI->cleanedMessInRoom[i];

				totalSum += _questUI->totalFurnitureInRoom[i];
				totalSum += _questUI->totalMopStainsInRoom[i];
				totalSum += _questUI->totalSpongeStainInRoom[i];
				totalSum += _questUI->totalMessInRoom[i];
			}

			currentSum += _questUI->cleanedTrashes;
			totalSum += _questUI->totalTrashes;

			auto percentage = static_cast<float>(currentSum) / static_cast<float>(totalSum);
			_questUI->progressPercentage = percentage;
			auto& countUI = registry->get<core::Text>(_questUI->_progressCounter);

			std::ostringstream stream;
			stream << std::fixed << std::setprecision(2) << percentage * 100;

			countUI.text = stream.str() + "%";
			auto& uiCommon = registry->get<core::UICommon>(_questUI->_progressBar);
			uiCommon.percentage = percentage;
		}
	}
}

void mc::InGameUiSystem::preStartSystem(const core::OnPreStartSystem& event)
{
	entt::registry* registry = event.scene->GetRegistry();

	if (const auto it = registry->ctx().find<mc::Quest>())
		_questUI = it;
	else
		_questUI = &registry->ctx().emplace<mc::Quest>();

	if (const auto it = registry->ctx().find<mc::CaptionLog>())
		_captionLog = it;
	else
		_captionLog = &registry->ctx().emplace<mc::CaptionLog>();
}

void mc::InGameUiSystem::startSystem(const core::OnStartSystem& event)
{
	auto registry = event.scene->GetRegistry();

	// UI matching
	for (auto&& [entity, tag] : registry->view<core::Tag>().each())
	{
		if (tag.id == tag::HoverUI::id)
		{
			_hoverUI = entity;
			break;
		}
	}
	if (_hoverUI == entt::null)
		LOG_ERROR(*event.scene, "InGameUiSystem needs essential components(Hover UI)");

	for (auto&& [entity, tag] : registry->view<core::Tag>().each())
	{
		if (tag.id == tag::TooltipUI::id)
		{
			_tooltipUI = entity;
			break;
		}
	}
	if (_tooltipUI == entt::null)
		LOG_ERROR(*event.scene, "InGameUiSystem needs essential components(Tooltip UI)");

	for (auto&& [entity, tag] : registry->view<core::Tag>().each())
	{
		if (tag.id == tag::FurnitureHPUI::id)
		{
			_furnitureHPUI = entity;
			break;
		}
	}
	if (_furnitureHPUI == entt::null)
		LOG_ERROR(*event.scene, "InGameUiSystem needs essential components(Furniture UI)");

	for (auto&& [entity, tag] : registry->view<core::Tag>().each())
	{
		if (tag.id == tag::AimUI::id)
		{
			_aimUI = entity;
			break;
		}
	}
	if (_aimUI == entt::null)
		LOG_ERROR(*event.scene, "InGameUiSystem needs essential components(Aim UI)");

	// Player matching
	for (auto&& [entity, controller] : registry->view<core::CharacterController>().each())
	{
		if (registry->all_of<mc::Picking, mc::Inventory, mc::RayCastingInfo>(entity))
		{
			_player = entity;
			break;
		}
	}
	if (_player == entt::null)
		LOG_ERROR(*event.scene, "InGameUiSystem needs essential components(Player)");


	for (auto&& [entity, tag] : registry->view<core::Tag>().each())
	{
		if (tag.id == tag::QuestUI::id)
		{
			_progressUI = entity;
			const auto& UIs = registry->get<core::Relationship>(entity).children;
			auto& questUI = registry->ctx().get<Quest>();

			for (auto& ui : UIs)
			{
				const auto& sub = registry->get<core::Relationship>(ui);

				if (sub.children.empty())
				{
					_placeUI = ui;
				}

				for (auto& subChild : sub.children)
				{
					auto& name = registry->get<core::Name>(subChild).name;

					if (name == "Trash Current")
						questUI._trashCounter = subChild;
					else if (name == "Trash Bar")
						questUI._trashBar = subChild;
					else if (name == "Sponge Stain Current")
						questUI._spongeStainCounter = subChild;
					else if (name == "Sponge Stain Bar")
						questUI._spongeStainBar = subChild;
					else if (name == "Mop Stain Current")
						questUI._mopStainCounter = subChild;
					else if (name == "Mop Stain Bar")
						questUI._mopStainBar = subChild;
					else if (name == "Mess Current")
						questUI._messCounter = subChild;
					else if (name == "Mess Bar")
						questUI._messBar = subChild;
					else if (name == "Furniture Current")
						questUI._furnitureCounter = subChild;
					else if (name == "Furniture Bar")
						questUI._furnitureBar = subChild;
					else if (name == "Progress Current")
						questUI._progressCounter = subChild;
					else if (name == "Progress Bar")
						questUI._progressBar = subChild;
				}
			}
			break;
		}
	}
}

void mc::InGameUiSystem::postStartSystem(const core::OnPostStartSystem& event)
{
	entt::registry* registry = event.scene->GetRegistry();

	for (auto&& [entity, trash] : registry->view<mc::DestroyableTrash>().each())
	{
		_questUI->totalTrashes += 1;
	}

	for (auto&& [entity, snap, tag] : registry->view<mc::SnapAvailable, core::Tag>().each())
	{
		if (tag.id != tag::Mess::id)
			continue;

		auto room = registry->get<mc::Room>(entity).type;

		if (snap.isSnapped)
		{
			_questUI->cleanedMessInRoom[room] += 1;
		}

		_questUI->totalMessInRoom[room] += 1;
	}

	for (auto&& [entity, tag, room] : registry->view<core::Tag, mc::Room>().each())
	{
		if (room.type == Room::Type::None)
		{
			LOG_WARN(*event.scene, "{} : Need to initialize mc::Room::Type", entity);
			continue;
		}

		if (tag.id == tag::Stain::id)
		{
			auto [isSpongeStain, isMopStain] = registry->get<mc::Stain>(entity);
			if (isMopStain)
				_questUI->totalMopStainsInRoom[room.type] += 1;
			else if (isSpongeStain)
				_questUI->totalSpongeStainInRoom[room.type] += 1;
			else
				LOG_ERROR(*event.scene, "{} : Setup mc::Stain", entity);
		}
		else if (tag.id == tag::Furniture::id)
			_questUI->totalFurnitureInRoom[room.type] += 1;
	}

	_queuedCaptions.emplace(CaptionLog::captions[CaptionLog::Caption_GameIn], 2.0f);
	_queuedCaptions.emplace(CaptionLog::captions[CaptionLog::Caption_ClickTerminal], 3.0f);
	_queuedCaptions.emplace(CaptionLog::captions[CaptionLog::Caption_NotTakeAnyTool], 10.0f, CaptionLog::Caption_NotTakeAnyTool);

	_captionLog->isShown[CaptionLog::Caption_NotTakeAnyTool] = true;
	_captionLog->isShown[CaptionLog::Caption_GameIn] = true;
	_captionLog->isShown[CaptionLog::Caption_ClickTerminal] = true;


	for (auto&& [entity, tag] : registry->view<core::Tag>().each())
	{
		if (tag.id == tag::ProgressUI::id)
		{
			auto* text = registry->try_get<core::Text>(entity);
			auto* uiCommon = registry->try_get<core::UICommon>(entity);

			if (text)
				text->isOn = _showProgress;
			else if (uiCommon)
				uiCommon->isOn = _showProgress;
		}
	}

	if (auto* uiCommon = registry->try_get<core::UICommon>(_progressUI))
		uiCommon->isOn = _showProgress;
}

void mc::InGameUiSystem::finishSystem(const core::OnFinishSystem& event)
{
	event.scene->GetRegistry()->ctx().erase<Quest>();
	event.scene->GetRegistry()->ctx().erase<CaptionLog>();

	_questUI = nullptr;
	_player = entt::null;
	_aimUI = entt::null;
	_hoverUI = entt::null;
	_tooltipUI = entt::null;
}

void mc::InGameUiSystem::decreaseStain(const OnDecreaseStain& event)
{
	auto entity = event.entity;
	auto& registry = *event.scene->GetRegistry();
	auto& room = registry.get<mc::Room>(entity).type;
	auto& stain = registry.get<mc::Stain>(entity);

	if (stain.isMopStain)
		_questUI->cleanedMopStainsInRoom[room] += 1;
	else if (stain.isSpongeStain)
		_questUI->cleanedSpongeStainInRoom[room] += 1;
	else
		LOG_ERROR(registry.ctx().get<core::Scene>(), "{} : Setup mc::Stain", entity);
}

void mc::InGameUiSystem::increaseStain(const OnIncreaseStain& event)
{
	auto entity = event.entity;
	auto& registry = *event.scene->GetRegistry();
	auto& room = registry.get<mc::Room>(entity).type;
	auto& stain = registry.get<mc::Stain>(entity);

	if (stain.isMopStain)
		_questUI->totalMopStainsInRoom[room] += 1;
	else if (stain.isSpongeStain)
		_questUI->totalSpongeStainInRoom[room] += 1;
	else
		LOG_ERROR(registry.ctx().get<core::Scene>(), "{} : Setup mc::Stain", entity);

}
void mc::InGameUiSystem::showCaption(const mc::OnShowCaption& event)
{
	_queuedCaptions.emplace(CaptionLog::captions[event.index], event.time, event.index);
}

void mc::InGameUiSystem::blinking(entt::registry& registry)
{
	constexpr float blinkInterval = 0.5f;
	static int prevShare = 0;
	static bool isBlinking = false;

	if (!_queuedCaptions.empty())
	{
		const auto caption = _queuedCaptions.front();
		int currentShare = caption.time / blinkInterval;

		if (caption.index == CaptionLog::Text::Caption_NotTakeAnyTool)
		{
			for (auto&& [entity, tool, attr] : registry.view<mc::Tool, core::RenderAttributes>().each())
			{
				if (prevShare != currentShare) // 아웃라인 키기
				{
					attr.flags ^= core::RenderAttributes::Flag::OutLine;
				}
				if (currentShare == 0)
				{
					attr.flags &= ~core::RenderAttributes::Flag::OutLine;
				}
			}
		}

		prevShare = currentShare;
	}


}
