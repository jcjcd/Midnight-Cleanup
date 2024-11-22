#pragma once

#include "Animacore/SystemInterface.h"
#include "Animacore/SystemTraits.h"

namespace core
{
	struct Configuration;

	struct OnStartSystem;
	struct OnFinishSystem;

	struct ComboBox;
	struct Slider;
	struct CheckBox;


}

namespace mc
{
	struct OnApplyChange;
	struct OnDiscardChange;
	struct GameSettingController;

	// 이벤트만 받고 던져주는 시스템
	class GameSettingSystem : public core::ISystem, public core::IUpdateSystem
	{
	public:
		GameSettingSystem(core::Scene& scene);
		~GameSettingSystem() override;

		void operator()(core::Scene& scene, float tick) override;

		void startSystem(const core::OnStartSystem& event);
		void finishSystem(const core::OnFinishSystem& event);

		void applyChange(const OnApplyChange& event);
		void discardChange(const OnDiscardChange& event);

		void loadConfigData();
		void saveConfigData();


	private:
		entt::dispatcher* _dispatcher = nullptr;

		core::Configuration* _config = nullptr;

		GameSettingController* _gameSettingController = nullptr;

		core::ComboBox* _resolutionComboBox = nullptr;
		core::Slider* _masterVolumeSlider = nullptr;
		core::Slider* _mouseSensitivitySlider = nullptr;
		core::Slider* _fovSlider = nullptr;
		core::CheckBox* _invertYAxisCheckBox = nullptr;
		core::CheckBox* _vSyncCheckBox = nullptr;

		int _screenWidth = 0;
		int _screenHeight = 0;
		int _fullScreen = 0;
		int _windowedFullScreen = 0;
		int _vSync = 0;

		int _masterVolume = 0;

		int _mouseSensitivity = 0;
		int _fov = 0;
		int _invertYAxis = 0;

		static constexpr wchar_t CONFIG_FILE[] = L"./config.ini";

		// 그래픽스 세팅 저장
		static constexpr wchar_t GRAPHICS_SECTION[] = L"Graphics";
		static constexpr wchar_t SCREEN_WIDTH[] = L"ScreenWidth";
		static constexpr wchar_t SCREEN_HEIGHT[] = L"ScreenHeight";
		static constexpr wchar_t FULLSCREEN[] = L"FullScreen";
		static constexpr wchar_t WINDOWED_FULLSCREEN[] = L"WindowedFullScreen";
		static constexpr wchar_t VSYNC[] = L"VSync";

		// 전체 창모드 구현 여부

		// 사운드 세팅 저장
		static constexpr wchar_t SOUND_SECTION[] = L"Sound";
		static constexpr wchar_t MASTER_VOLUME[] = L"MasterVolume";

		// 게임 세팅 저장
		static constexpr wchar_t GAME_SETTING_SECTION[] = L"GameSetting";
		static constexpr wchar_t MOUSE_SENSITIVITY[] = L"MouseSensitivity";
		static constexpr wchar_t FOV[] = L"FOV";
		static constexpr wchar_t INVERT_Y_AXIS[] = L"InvertYAxis";
	};
};
DEFINE_SYSTEM_TRAITS(mc::GameSettingSystem)