#pragma once
#include "McComponents.h"

#include <Animacore/SerializeHelper.h>

namespace cereal
{
	template <class Archive>
	void serialize(Archive& archive, mc::CharacterMovement& data)
	{
		archive(
			MAKE_NVP(speed),
			MAKE_NVP(jump),
			MAKE_NVP(jumpAcceleration),
			MAKE_NVP(dropAcceleration),
			MAKE_NVP(hDir),
			MAKE_NVP(vDir)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::Picking& data)
	{
		archive(
			MAKE_NVP(handSpot),
			MAKE_NVP(pickingObject)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, mc::RadialUI& data)
	{
		archive(
			MAKE_NVP(type)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, mc::Tool& data)
	{
		archive(
			MAKE_NVP(type)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::Inventory::ToolSlot& data)
	{
		archive(
			MAKE_NVP(tool1),
			MAKE_NVP(tool2),
			MAKE_NVP(tool3),
			MAKE_NVP(tool4)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, mc::Inventory& data)
	{
		archive(
			MAKE_NVP(toolSlot)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::RayCastingInfo& data)
	{
		archive(
			MAKE_NVP(interactableRange)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::PivotSlot& data)
	{
		archive(
			MAKE_NVP(isUsing),
			MAKE_NVP(pivotPosition),
			MAKE_NVP(pivotRotation),
			MAKE_NVP(pivotScale)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::WaterBucket& data)
	{
		archive(
			MAKE_NVP(isFilled)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::Durability& data)
	{
		archive(
			MAKE_NVP(maxDurability),
			MAKE_NVP(currentDurability)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::Stain& data)
	{
		archive(
			MAKE_NVP(isMopStain),
			MAKE_NVP(isSpongeStain)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::SnapAvailable& data)
	{
		archive(
			MAKE_NVP(placedSpot),
			MAKE_NVP(length)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::DestroyableTrash& data)
	{
		archive(
			MAKE_NVP(inTrashBox),
			MAKE_NVP(remainTime)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::Room& data)
	{
		archive(
			MAKE_NVP(type)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::DissolveRenderer& data)
	{
		archive(
			MAKE_NVP(dissolveFactor),
			MAKE_NVP(dissolveSpeed),
			MAKE_NVP(dissolveColor),
			MAKE_NVP(edgeWidth),
			MAKE_NVP(isPlaying)
			);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::Door& data)
	{
		archive(
			MAKE_NVP(state)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::Switch& data)
	{
		archive(
			MAKE_NVP(isOn),
			MAKE_NVP(lights),
			MAKE_NVP(lever)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::Switch::Lights& data)
	{
		archive(
			MAKE_NVP(light0),
			MAKE_NVP(light1),
			MAKE_NVP(light2)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::Flashlight& data)
	{
		archive(
			MAKE_NVP(isOn)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::TrashGenerator& data)
	{
		archive(
			MAKE_NVP(trashCount)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::Mop& data)
	{
		archive(
			MAKE_NVP(eraseCount)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::Sponge& data)
	{
		archive(
			MAKE_NVP(spongePower)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::TVTimer& data)
	{
		archive(
			MAKE_NVP(minute),
			MAKE_NVP(second)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::GameManager& data)
	{
		archive(
			MAKE_NVP(timeLimit)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::GameSettingController& data)
	{
		archive(
			MAKE_NVP(screenResolution),
			MAKE_NVP(vSync),
			MAKE_NVP(masterVolume),
			MAKE_NVP(mouseSensitivity),
			MAKE_NVP(fov),
			MAKE_NVP(invertYAxis)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::MopStainGenerator& data)
	{
		archive(
			MAKE_NVP(stainCount)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::SoundMaterial& data)
	{
		archive(
			MAKE_NVP(type)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::VideoRenderer& data)
	{
		archive(
			MAKE_NVP(videoPath),
			MAKE_NVP(isPlaying),
			MAKE_NVP(isLoop)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, mc::StandardScore& data)
	{
		archive(
			MAKE_NVP(timeBonusMaxLimitTime),
			MAKE_NVP(timeBonusTimeMinLimitTime),
			MAKE_NVP(bonusPerGhostEvent),
			MAKE_NVP(bonusPerTrash),
			MAKE_NVP(bonusPerPercentage),
			MAKE_NVP(bonusPerTime),
			MAKE_NVP(minTimeBonus),
			MAKE_NVP(penaltyAbandoned),
			MAKE_NVP(penaltyPerMess),
			MAKE_NVP(fiveStarMinScore),
			MAKE_NVP(fourStarMinScore),
			MAKE_NVP(threeStarMinScore),
			MAKE_NVP(twoStarMinScore),
			MAKE_NVP(oneStarMinScore)
		);
	}
}
