#pragma once
#include "McCallback.h"
#include "McSerialize.h"
#include "McComponents.h"
#include "McTagsAndLayers.h"

#include "TrashSystem.h"
#include "PickingSystem.h"
#include "InGameUiSystem.h"
#include "DropSoundSystem.h"
#include "InventorySystem.h"
#include "RoomEnterSystem.h"
#include "TimeLimitSystem.h"
#include "RandomGeneratorSystem.h"
#include "CharacterActionSystem.h"
#include "CharacterMovementSystem.h"
#include "FurnitureDiscardingSystem.h"
#include "IngameLightSystem.h"
#include "GameSettingSystem.h"
#include "BucketTillingSystem.h"
#include "CreditSceneSystem.h"
#include "CustomRenderSystem.h"
#include "BGMSystem.h"

#include <Animacore/PxUtils.h>
#include <Animacore/MetaCtxs.h>
#include <Animacore/MetaHelpers.h>
#include <Animacore/CoreSerialize.h>
#include <Animacore/SystemTemplates.h>
#include <Animacore/ComponentTemplates.h>




namespace mc
{
	inline void RegisterMcMetaData()
	{
		// 시스템 메타 데이터 등록
		{
			REGISTER_SYSTEM_META(CharacterMovementSystem);
			REGISTER_SYSTEM_META(PickingSystem);
			REGISTER_SYSTEM_META(InventorySystem);
			REGISTER_SYSTEM_META(CharacterActionSystem);
			REGISTER_SYSTEM_META(FurnitureDiscardingSystem);
			REGISTER_SYSTEM_META(TimeLimitSystem);
			REGISTER_SYSTEM_META(TrashSystem);
			REGISTER_SYSTEM_META(DropSoundSystem);
			REGISTER_SYSTEM_META(InGameUiSystem);
			REGISTER_SYSTEM_META(RoomEnterSystem);
			REGISTER_SYSTEM_META(IngameLightSystem);
			REGISTER_SYSTEM_META(RandomGeneratorSystem);
			REGISTER_SYSTEM_META(GameSettingSystem);
			REGISTER_SYSTEM_META(BucketTillingSystem);
			REGISTER_SYSTEM_META(CreditSceneSystem);
			REGISTER_SYSTEM_META(CustomRenderSystem);
			REGISTER_SYSTEM_META(BGMSystem);
		}

		// 컴포넌트 메타 데이터 등록
		{
			REGISTER_COMPONENT_META(CharacterMovement)
				SET_DEFAULT_FUNC(CharacterMovement)
				SET_MEMBER(CharacterMovement::speed, "Speed")
				SET_MEMBER(CharacterMovement::jump, "Jump")
				SET_MEMBER(CharacterMovement::jumpAcceleration, "Jump Acceleration")
				SET_DRAG_SPEED(0.01f)
				SET_MEMBER(CharacterMovement::dropAcceleration, "Drop Acceleration")
				SET_DRAG_SPEED(0.01f)
				SET_MEMBER(CharacterMovement::hDir, "Horizon Direction")
				SET_MEMBER(CharacterMovement::vDir, "Vertical Direction");

			REGISTER_COMPONENT_META(Picking)
				SET_DEFAULT_FUNC(Picking)
				SET_MEMBER(Picking::handSpot, "Hand Spot")
				SET_DESC("물건을 잡는 손 오브젝트")
				SET_MEMBER(Picking::pickingObject, "Picking Object")
				SET_DESC("잡고 있는 오브젝트");

			REGISTER_COMPONENT_META(RadialUI)
				SET_DEFAULT_FUNC(RadialUI)
				SET_MEMBER(RadialUI::type, "Type");

			REGISTER_COMPONENT_META(Tool)
				SET_DEFAULT_FUNC(Tool)
				SET_MEMBER(Tool::type, "Type");

			REGISTER_COMPONENT_META(Inventory)
				SET_DEFAULT_FUNC(Inventory)
				SET_MEMBER(Inventory::toolSlot, "Tool Slot");

			REGISTER_COMPONENT_META(RayCastingInfo)
				SET_DEFAULT_FUNC(RayCastingInfo)
				SET_MEMBER(RayCastingInfo::interactableRange, "Interactable Range");

			REGISTER_COMPONENT_META(PivotSlot)
				SET_DEFAULT_FUNC(PivotSlot)
				SET_MEMBER(PivotSlot::isUsing, "isUsing")
				SET_MEMBER(PivotSlot::pivotPosition, "Pivot Position")
				SET_MEMBER(PivotSlot::pivotRotation, "Pivot Rotation")
				SET_MEMBER(PivotSlot::pivotScale, "Pivot Scale");

			REGISTER_COMPONENT_META(WaterBucket)
				SET_DEFAULT_FUNC(WaterBucket)
				SET_MEMBER(WaterBucket::isFilled, "Is Filled");

			REGISTER_COMPONENT_META(Durability)
				SET_DEFAULT_FUNC(Durability)
				SET_MEMBER(Durability::maxDurability, "Max Durability")
				SET_MEMBER(Durability::currentDurability, "Current Durability");

			REGISTER_COMPONENT_META(Stain)
				SET_DEFAULT_FUNC(Stain)
				SET_MEMBER(Stain::isMopStain, "Is Mop Stain")
				SET_MEMBER(Stain::isSpongeStain, "Is Sponge Stain");

			REGISTER_COMPONENT_META(SnapAvailable)
				SET_DEFAULT_FUNC(SnapAvailable)
				SET_MEMBER(SnapAvailable::placedSpot, "Placed Spot")
				SET_MEMBER(SnapAvailable::length, "Available Length");

			REGISTER_COMPONENT_META(DestroyableTrash)
				SET_DEFAULT_FUNC(DestroyableTrash)
				SET_MEMBER(DestroyableTrash::inTrashBox, "In TrashBox")
				SET_MEMBER(DestroyableTrash::remainTime, "Remain Time");			

			REGISTER_COMPONENT_META(Room)
				SET_DEFAULT_FUNC(Room)
				SET_MEMBER(Room::type, "Type");

			REGISTER_COMPONENT_META(DissolveRenderer)
				SET_DEFAULT_FUNC(DissolveRenderer)
				SET_MEMBER(DissolveRenderer::dissolveFactor, "Dissolve Factor")
				SET_DRAG_SPEED(0.01f)
				SET_MEMBER(DissolveRenderer::dissolveSpeed, "Dissolve Speed")
				SET_MEMBER(DissolveRenderer::dissolveColor, "Dissolve Color")
				SET_MEMBER(DissolveRenderer::edgeWidth, "Edge Width")
				SET_MEMBER(DissolveRenderer::isPlaying, "Is Playing");

			REGISTER_COMPONENT_META(Door)
				SET_DEFAULT_FUNC(Door)
				SET_MEMBER(Door::state, "State");

			REGISTER_COMPONENT_META(Switch)
				SET_DEFAULT_FUNC(Switch)
				SET_MEMBER(Switch::isOn, "Is On")
				SET_MEMBER(Switch::lights, "Lights")
				SET_MEMBER(Switch::lever, "Lever");

			REGISTER_COMPONENT_META(Flashlight)
				SET_DEFAULT_FUNC(Flashlight)
				SET_MEMBER(Flashlight::isOn, "Is On");

			REGISTER_COMPONENT_META(TrashGenerator)
				SET_DEFAULT_FUNC(TrashGenerator)
				SET_MEMBER(TrashGenerator::trashCount, "Trash Count");

			REGISTER_COMPONENT_META(Mop)
				SET_DEFAULT_FUNC(Mop)
				SET_MEMBER(Mop::eraseCount, "Erase Count");

			REGISTER_COMPONENT_META(Sponge)
				SET_DEFAULT_FUNC(Sponge)
				SET_MEMBER(Sponge::spongePower, "Sponge Power");

			REGISTER_COMPONENT_META(TVTimer)
				SET_DEFAULT_FUNC(TVTimer)
				SET_MEMBER(TVTimer::minute, "Minute")
				SET_MEMBER(TVTimer::second, "Second");

			REGISTER_COMPONENT_META(GameManager)
				SET_DEFAULT_FUNC(GameManager)
				SET_MEMBER(GameManager::timeLimit, "Time Limit");

			REGISTER_COMPONENT_META(GameSettingController)
				SET_DEFAULT_FUNC(GameSettingController)
				SET_MEMBER(GameSettingController::screenResolution, "Screen Resolution")
				SET_MEMBER(GameSettingController::vSync, "VSync")
				SET_MEMBER(GameSettingController::masterVolume, "Master Volume")
				SET_MEMBER(GameSettingController::mouseSensitivity, "Mouse Sensitivity")
				SET_MEMBER(GameSettingController::fov, "FOV")
				SET_MEMBER(GameSettingController::invertYAxis, "Invert Y Axis");

			REGISTER_COMPONENT_META(MopStainGenerator)
				SET_DEFAULT_FUNC(MopStainGenerator)
				SET_MEMBER(MopStainGenerator::stainCount, "Stain Count");

			REGISTER_COMPONENT_META(SoundMaterial)
				SET_DEFAULT_FUNC(SoundMaterial)
				SET_MEMBER(SoundMaterial::type, "Type");

			REGISTER_COMPONENT_META(VideoRenderer)
				SET_DEFAULT_FUNC(VideoRenderer)
				SET_MEMBER(VideoRenderer::videoPath, "Video Path")
				SET_MEMBER(VideoRenderer::isPlaying, "Is Playing")
				SET_MEMBER(VideoRenderer::isLoop, "Is Loop");

			REGISTER_COMPONENT_META(StandardScore)
				SET_DEFAULT_FUNC(StandardScore)
				SET_MEMBER(StandardScore::timeBonusMaxLimitTime, "Time Bonus Max Limit Time")
				SET_MEMBER(StandardScore::timeBonusTimeMinLimitTime, "Time Bonus Min Limit Time")
				SET_MEMBER(StandardScore::bonusPerGhostEvent, "Bonus Per Ghost Event")
				SET_MEMBER(StandardScore::bonusPerTrash, "Bonus Per Trash")
				SET_MEMBER(StandardScore::bonusPerPercentage, "Bonus Per Percentage")
				SET_MEMBER(StandardScore::bonusPerTime, "Bonus Per Time")
				SET_MEMBER(StandardScore::minTimeBonus, "Minimum Time Bonus")
				SET_MEMBER(StandardScore::penaltyAbandoned, "Penalty Abandoned")
				SET_MEMBER(StandardScore::penaltyPerMess, "Penalty Per Mess")
				SET_MEMBER(StandardScore::fiveStarMinScore, "Five Star Minimum Score")
				SET_MEMBER(StandardScore::fourStarMinScore, "Four Star Minimum Score")
				SET_MEMBER(StandardScore::threeStarMinScore, "Three Star Minimum Score")
				SET_MEMBER(StandardScore::twoStarMinScore, "Two Star Minimum Score")
				SET_MEMBER(StandardScore::oneStarMinScore, "One Star Minimum Score");
		}

		// struct
		{
			REGISTER_CUSTOM_STRUCT_META(Inventory::ToolSlot)
				SET_MEMBER(Inventory::ToolSlot::tool1, "Tool 1")
				SET_MEMBER(Inventory::ToolSlot::tool2, "Tool 2")
				SET_MEMBER(Inventory::ToolSlot::tool3, "Tool 3")
				SET_MEMBER(Inventory::ToolSlot::tool4, "Tool 4");

			REGISTER_CUSTOM_STRUCT_META(Switch::Lights)
				SET_MEMBER(Switch::Lights::light0, "Light 1")
				SET_MEMBER(Switch::Lights::light1, "Light 2")
				SET_MEMBER(Switch::Lights::light2, "Light 3");
		}

		// enum
		{
			REGISTER_CUSTOM_ENUM_META(RadialUI::Type)
				SET_NAME(RadialUI::Type)
				SET_VALUE(RadialUI::Type::DEFAULT, "Default")
				SET_VALUE(RadialUI::Type::TEXT, "Text")
				SET_VALUE(RadialUI::Type::ARROW, "Arrow")
				SET_VALUE(RadialUI::Type::INVENTORY_LEFT, "Inventory left")
				SET_VALUE(RadialUI::Type::INVENTORY_RIGHT, "Inventory right")
				SET_VALUE(RadialUI::Type::INVENTORY_TOP, "Inventory top")
				SET_VALUE(RadialUI::Type::INVENTORY_BOTTOM, "Inventory bottom")
				SET_VALUE(RadialUI::Type::DIRECTION_LEFT, "Direction left")
				SET_VALUE(RadialUI::Type::DIRECTION_RIGHT, "Direction right")
				SET_VALUE(RadialUI::Type::DIRECTION_TOP, "Direction top")
				SET_VALUE(RadialUI::Type::DIRECTION_BOTTOM, "Direction bottom");

			REGISTER_CUSTOM_ENUM_META(Tool::Type)
				SET_NAME(Tool::Type)
				SET_VALUE(Tool::Type::DEFAULT, "Default")
				SET_VALUE(Tool::Type::AXE, "Axe")
				SET_VALUE(Tool::Type::MOP, "Mop")
				SET_VALUE(Tool::Type::SPONGE, "Sponge")
				SET_VALUE(Tool::Type::FLASHLIGHT, "Flashlight");

			REGISTER_CUSTOM_ENUM_META(Room::Type)
				SET_NAME(Room::Type)
				SET_VALUE(Room::Type::None, "None")
				SET_VALUE(Room::Type::Bathroom, "Bathroom")
				SET_VALUE(Room::Type::BreakRoom, "BreakRoom")
				SET_VALUE(Room::Type::Corridor, "Corridor")
				SET_VALUE(Room::Type::GuestRoom, "GuestRoom")
				SET_VALUE(Room::Type::Kitchen, "Kitchen")
				SET_VALUE(Room::Type::LaundryRoom, "LaundryRoom")
				SET_VALUE(Room::Type::LivingRoom, "LivingRoom")
				SET_VALUE(Room::Type::Outdoor, "Outdoor")
				SET_VALUE(Room::Type::Porch, "Porch")
				SET_VALUE(Room::Type::SmallRoom, "SmallRoom")
				SET_VALUE(Room::Type::Storage, "Storage");

			REGISTER_CUSTOM_ENUM_META(Door::State)
				SET_NAME(Door::State)
				SET_VALUE(Door::State::None, "None")
				SET_VALUE(Door::State::OpenPositiveX, "Open Positive X")
				SET_VALUE(Door::State::OpenPositiveZ, "Open Positive Z")
				SET_VALUE(Door::State::OpenNegativeX, "Open Negative X")
				SET_VALUE(Door::State::OpenNegativeZ, "Open Negative Z")
				SET_VALUE(Door::State::ClosePositiveX, "Closed Positive X")
				SET_VALUE(Door::State::ClosePositiveZ, "Closed Positive Z")
				SET_VALUE(Door::State::CloseNegativeX, "Closed Negative X")
				SET_VALUE(Door::State::CloseNegativeZ, "Closed Negative Z");

			REGISTER_CUSTOM_ENUM_META(SoundMaterial::Type)
				SET_NAME(SoundMaterial::Type)
				SET_VALUE(SoundMaterial::Type::DEFAULT, "Default")
				SET_VALUE(SoundMaterial::Type::GROUND, "Ground")
				SET_VALUE(SoundMaterial::Type::WOODEN, "Wooden")
				SET_VALUE(SoundMaterial::Type::TRUCK, "Truck")
				SET_VALUE(SoundMaterial::Type::TATAMI, "Tatami");
		}

		// 태그 및 레이어 등록
		{
			REGISTER_TAG_META(tag::WorkTerminal);
			REGISTER_TAG_META(tag::Door);
			REGISTER_TAG_META(tag::Switch);
			REGISTER_TAG_META(tag::WaterBucket);
			REGISTER_TAG_META(tag::WaterBucket);
			REGISTER_TAG_META(tag::Hologram);
			REGISTER_TAG_META(tag::Furniture);
			REGISTER_TAG_META(tag::Stain);
			REGISTER_TAG_META(tag::Wall);
			REGISTER_TAG_META(tag::Floor);
			REGISTER_TAG_META(tag::Trash);
			REGISTER_TAG_META(tag::Tool);
			REGISTER_TAG_META(tag::TrashBox);
			REGISTER_TAG_META(tag::Mess);
			REGISTER_TAG_META(tag::HoverUI);
			REGISTER_TAG_META(tag::QuestUI);
			REGISTER_TAG_META(tag::RoomEnter);
			REGISTER_TAG_META(tag::TrashGenerator);
			REGISTER_TAG_META(tag::TooltipUI);
			REGISTER_TAG_META(tag::AimUI);
			REGISTER_TAG_META(tag::ProgressUI);
			REGISTER_TAG_META(tag::SettingUI);
			REGISTER_TAG_META(tag::TooltipAlphaUI);
			REGISTER_TAG_META(tag::FurnitureHPUI);
			REGISTER_TAG_META(tag::TruckLight);
#pragma region Reward Stars
			REGISTER_TAG_META(tag::Star0);
			REGISTER_TAG_META(tag::Star1);
			REGISTER_TAG_META(tag::Star2);
			REGISTER_TAG_META(tag::Star3);
			REGISTER_TAG_META(tag::Star4);
#pragma endregion
#pragma region Reward Pay
			REGISTER_TAG_META(tag::WorkPay);
			REGISTER_TAG_META(tag::TotalPay);
			REGISTER_TAG_META(tag::TotalComment);
#pragma endregion
			REGISTER_TAG_META(tag::TruckDoorL);
			REGISTER_TAG_META(tag::TruckDoorR);
			REGISTER_TAG_META(tag::EndingCamera);
			REGISTER_TAG_META(tag::TruckPedestal);
			REGISTER_TAG_META(tag::Credit);
			REGISTER_TAG_META(tag::TriggerSound);
			REGISTER_TAG_META(tag::FinalPercentage);
			REGISTER_TAG_META(tag::CleanPlusWage);
			REGISTER_TAG_META(tag::MessBonus);
			REGISTER_TAG_META(tag::AbandonedBonus);
			REGISTER_TAG_META(tag::PercentageBonus);
			REGISTER_TAG_META(tag::GhostEventBonus);

			REGISTER_LAYER_META(layer::Stain);
			REGISTER_LAYER_META(layer::Furniture);
			REGISTER_LAYER_META(layer::Pickable);
			REGISTER_LAYER_META(layer::Hologram);
			REGISTER_LAYER_META(layer::Outline);
			REGISTER_LAYER_META(layer::RoomEnter);
			REGISTER_LAYER_META(layer::Door);
			REGISTER_LAYER_META(layer::Wall);
			REGISTER_LAYER_META(layer::Switch);
			REGISTER_LAYER_META(layer::WorkTerminal);
		}


		// 애니메이션 이벤트함수 등록
		{
			REGISTER_CALLBACK_EVENT_FUNC(OnPlaySound);
			REGISTER_CALLBACK_EVENT_FUNC(OnTest);
			REGISTER_CALLBACK_EVENT_FUNC(OnTestSceneChange);
			REGISTER_CALLBACK_EVENT_FUNC(OnQuitGame);
			REGISTER_CALLBACK_EVENT_FUNC(OnChangeResolution);
			REGISTER_CALLBACK_EVENT_FUNC(OnHitFurniture);
			REGISTER_CALLBACK_EVENT_FUNC(OnWipeTools);
			REGISTER_CALLBACK_EVENT_FUNC(OnPlayCurrentEntitySound);
			REGISTER_CALLBACK_EVENT_FUNC(OnChangeTrigger);
			REGISTER_CALLBACK_EVENT_FUNC(OnApplySetting);
			REGISTER_CALLBACK_EVENT_FUNC(OnDiscardSetting);
			REGISTER_CALLBACK_EVENT_FUNC(OnCloseSetting);
			REGISTER_CALLBACK_EVENT_FUNC(OnRespawn);
			REGISTER_CALLBACK_EVENT_FUNC(OnChangeBGMState);
		}


		// 충돌 처리
		{
			SET_LAYER_COLLISION(layer::Stain, layer::Pickable, false);
			SET_LAYER_COLLISION(layer::Stain, layer::Outline, false);
		}
	}
}

