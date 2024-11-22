#pragma once
#include "Animacore/SystemInterface.h"
#include "Animacore/SystemTraits.h"

namespace mc
{
	struct OnDecreaseStain;
	struct OnIncreaseStain;
	struct OnShowCaption;
	struct Quest;
	struct CaptionLog;

	class InGameUiSystem : public core::ISystem, public core::IUpdateSystem
	{
	public:
		InGameUiSystem(core::Scene& scene);
		~InGameUiSystem() override;

		void operator()(core::Scene& scene, float tick) override;

	private:
		void preStartSystem(const core::OnPreStartSystem& event);
		void startSystem(const core::OnStartSystem& event);
		void postStartSystem(const core::OnPostStartSystem& event);
		void finishSystem(const core::OnFinishSystem& event);
		void decreaseStain(const OnDecreaseStain& event);
		void increaseStain(const OnIncreaseStain& event);

		void showCaption(const mc::OnShowCaption& event);
		void blinking(entt::registry& registry);

	private:
		entt::dispatcher* _dispatcher = nullptr;
		mc::Quest* _questUI = nullptr;
		mc::CaptionLog* _captionLog = nullptr;

		entt::entity _player = entt::null;
		entt::entity _aimUI = entt::null;
		entt::entity _hoverUI = entt::null;
		entt::entity _tooltipUI = entt::null;
		entt::entity _progressUI = entt::null;
		entt::entity _placeUI = entt::null;
		entt::entity _furnitureHPUI = entt::null;

		bool _showProgress = true;

		struct QueuedCaption
		{
			const char* text;
			float time;
			uint32_t index;
		};
		std::queue<QueuedCaption> _queuedCaptions;


		enum
		{
			Interactable,		// 상호작용
			DropDown,			// 내려 놓기
			RubSponge,			// 스펀지로 문지르기
			FlashlightOnOff,	// 손전등 켜고 끄기

			Number
		};

		static constexpr const char* tooltipFileStr[Number]
		{
			"T_Aim_Hand_ToolTip.dds",
			"T_Aim_grab_ToolTip.dds",
			"T_Aim_Sponge_ToolTip.dds",
			"T_Aim_Flashlight_ToolTip.dds",
		};

		enum
		{
			Axe,
			Mop,
			Sponge,
			Flashlight,
			Hand,
			Trash,
			Default,
			Switch,
			FillWater,
			Organize,
			Terminal,

			Count
		};

		static constexpr const char* aimFileStr[Count]
		{
			"T_Aim_Axe.dds",
			"T_Aim_Mop.dds",
			"T_Aim_Sponge.dds",
			"T_Aim_FlashLight.dds",
			"T_Aim_Hand.dds",
			"T_Aim_Trash.dds",
			"T_Aim.dds",
			"T_Aim_Switch.dds",
			"T_Aim_Bucket.dds",
			"T_Aim_Organize.dds",
			"T_Aim_Hand_b.dds",
		};

	};
}
DEFINE_SYSTEM_TRAITS(mc::InGameUiSystem)

