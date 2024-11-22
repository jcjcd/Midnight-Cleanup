#pragma once
#include <Animacore/TagAndLayerHelpers.h>


// 충돌 반응용
namespace tag
{
	DEFINE_TAG(WorkTerminal);

	DEFINE_TAG(Door);
	DEFINE_TAG(Switch);

	DEFINE_TAG(WaterBucket);
	DEFINE_TAG(Hologram);

	DEFINE_TAG(Furniture);
	DEFINE_TAG(Stain);
	DEFINE_TAG(Wall);
	DEFINE_TAG(Floor);

	DEFINE_TAG(Trash);
	DEFINE_TAG(Tool);
	DEFINE_TAG(Mess);
	DEFINE_TAG(TrashBox);
	DEFINE_TAG(HoverUI);

	DEFINE_TAG(QuestUI);
	DEFINE_TAG(RoomEnter);
	DEFINE_TAG(TrashGenerator);

	DEFINE_TAG(TooltipUI);
	DEFINE_TAG(AimUI);
	DEFINE_TAG(ProgressUI);
	DEFINE_TAG(SettingUI);
	DEFINE_TAG(TooltipAlphaUI);
	DEFINE_TAG(FurnitureHPUI);

	DEFINE_TAG(TruckLight);
	DEFINE_TAG(TruckDoorL);
	DEFINE_TAG(TruckDoorR);
	DEFINE_TAG(TruckPedestal);
	DEFINE_TAG(EndingCamera);

#pragma region Reward Stars
	DEFINE_TAG(Star0);
	DEFINE_TAG(Star1);
	DEFINE_TAG(Star2);
	DEFINE_TAG(Star3);
	DEFINE_TAG(Star4);
#pragma endregion

#pragma region Reward Pay
	DEFINE_TAG(WorkPay);
	DEFINE_TAG(TotalPay);
	DEFINE_TAG(TotalComment);
#pragma endregion

	DEFINE_TAG(Credit);
	DEFINE_TAG(TriggerSound);
	DEFINE_TAG(FinalPercentage);

	DEFINE_TAG(CleanPlusWage);
	DEFINE_TAG(MessBonus);
	DEFINE_TAG(AbandonedBonus);
	DEFINE_TAG(GhostEventBonus);
	DEFINE_TAG(PercentageBonus);
}

// 레이캐스트, 충돌 검출 
namespace layer
{
	DEFINE_LAYER(Stain, 5);
	DEFINE_LAYER(Furniture, 6);
	DEFINE_LAYER(Pickable, 7);
	DEFINE_LAYER(Hologram, 8);
	DEFINE_LAYER(Outline, 9);
	DEFINE_LAYER(Tool, 10);
	DEFINE_LAYER(RoomEnter, 11);
	DEFINE_LAYER(Door, 12);
	DEFINE_LAYER(Wall, 13);
	DEFINE_LAYER(Switch, 14);
	DEFINE_LAYER(WorkTerminal, 15);
}


// layer : tag 들의 모임
// tag : 오브젝트의 특성을 사용자가 정의할 수 있도록 만든 객체