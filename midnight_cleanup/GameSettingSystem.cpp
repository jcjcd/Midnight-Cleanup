#include "pch.h"
#include "GameSettingSystem.h"

#include <Animacore/Scene.h>
#include <Animacore/CoreComponents.h>
#include <Animacore/RenderComponents.h>

#include "McEvents.h"
#include "McComponents.h"


mc::GameSettingSystem::GameSettingSystem(core::Scene& scene) :
	core::ISystem(scene)
{
	_dispatcher = scene.GetDispatcher();

	_dispatcher->sink<core::OnStartSystem>().connect<&GameSettingSystem::startSystem>(this);
	_dispatcher->sink<core::OnFinishSystem>().connect<&GameSettingSystem::finishSystem>(this);

	_dispatcher->sink<OnApplyChange>().connect<&GameSettingSystem::applyChange>(this);
	_dispatcher->sink<OnDiscardChange>().connect<&GameSettingSystem::discardChange>(this);

}

mc::GameSettingSystem::~GameSettingSystem()
{
	_dispatcher->disconnect(this);

}

void mc::GameSettingSystem::operator()(core::Scene& scene, float tick)
{
	if (!_gameSettingController)
		return;

	auto* registry = scene.GetRegistry();
	auto&& setting = registry->ctx().get<GameSetting>();

	setting.masterVolume = static_cast<int>(_masterVolumeSlider->curWeight);

	setting.mouseSensitivity = static_cast<int>(_mouseSensitivitySlider->curWeight);
	setting.fov = static_cast<int>(_fovSlider->curWeight);
	setting.invertYAxis = _invertYAxisCheckBox->isChecked;
}

void mc::GameSettingSystem::startSystem(const core::OnStartSystem& event)
{
	auto* registry = event.scene->GetRegistry();
	auto&& setting = registry->ctx().emplace<GameSetting>();
	auto& config = registry->ctx().get<core::Configuration>();

	_config = &config;

	loadConfigData();

	config.width = _screenWidth;
	config.height = _screenHeight;
	config.isFullScreen = _fullScreen;
	config.isWindowedFullScreen = _windowedFullScreen;
	config.enableVSync = _vSync;

	setting.masterVolume = _masterVolume;

	setting.mouseSensitivity = _mouseSensitivity;
	setting.fov = _fov;
	setting.invertYAxis = _invertYAxis;

	applyChange(OnApplyChange{});

	{
		auto&& view = event.scene->GetRegistry()->view<GameSettingController>();
		for (auto&& [entity, gsController] : view.each())
		{
			_gameSettingController = &gsController;
		}

		if (!_gameSettingController)
			return;

		_resolutionComboBox = registry->try_get<core::ComboBox>(_gameSettingController->screenResolution);
		_masterVolumeSlider = registry->try_get<core::Slider>(_gameSettingController->masterVolume);
		_mouseSensitivitySlider = registry->try_get<core::Slider>(_gameSettingController->mouseSensitivity);
		_fovSlider = registry->try_get<core::Slider>(_gameSettingController->fov);
		_invertYAxisCheckBox = registry->try_get<core::CheckBox>(_gameSettingController->invertYAxis);
		_vSyncCheckBox = registry->try_get<core::CheckBox>(_gameSettingController->vSync);

		if (_resolutionComboBox)
		{
			if (_fullScreen)
				_resolutionComboBox->curIndex = 2;
			else if (_windowedFullScreen)
				_resolutionComboBox->curIndex = 1;
			else
			{
				_resolutionComboBox->curIndex = 0;
			}
		}


		if (_masterVolumeSlider)
			_masterVolumeSlider->curWeight = static_cast<float>(_masterVolume);

		if (_mouseSensitivitySlider)
			_mouseSensitivitySlider->curWeight = static_cast<float>(_mouseSensitivity);

		if (_fovSlider)
			_fovSlider->curWeight = static_cast<float>(_fov);

		if (_invertYAxisCheckBox)
			_invertYAxisCheckBox->isChecked = _invertYAxis;

		if (_vSyncCheckBox)
			_vSyncCheckBox->isChecked = _vSync;

	}
}

void mc::GameSettingSystem::finishSystem(const core::OnFinishSystem& event)
{
	saveConfigData();

	_gameSettingController = nullptr;
	_resolutionComboBox = nullptr;
	_masterVolumeSlider = nullptr;
	_mouseSensitivitySlider = nullptr;
	_fovSlider = nullptr;
	_invertYAxisCheckBox = nullptr;
	_vSyncCheckBox = nullptr;

}


void mc::GameSettingSystem::applyChange(const OnApplyChange& event)
{
	// 설정 적용

	if (_resolutionComboBox)
	{
		int maxWidth = GetSystemMetrics(SM_CXSCREEN);
		int maxHeight = GetSystemMetrics(SM_CYSCREEN);	// 구현 필요
		switch (_resolutionComboBox->curIndex)
		{
		case 0:
			_screenWidth = 1920;
			_screenHeight = 1080;
			_windowedFullScreen = 0;
			_fullScreen = 0;
			break;
		case 1:
			// 전체창
			_screenWidth = maxWidth;
			_screenHeight = maxHeight;
			_windowedFullScreen = 1;
			_fullScreen = 0;
			break;
		case 2:
			// 전체화면
			_screenWidth = maxWidth;
			_screenHeight = maxHeight;
			_windowedFullScreen = 0;
			_fullScreen = 1;
			break;

		default:
			break;
		}
	}

	if (_masterVolumeSlider)
	{
		_masterVolume = static_cast<int>(_masterVolumeSlider->curWeight);
		_dispatcher->enqueue<core::OnSetVolume>(_masterVolumeSlider->curWeight / 100.f);
	}

	if (_mouseSensitivitySlider)
		_mouseSensitivity = static_cast<int>(_mouseSensitivitySlider->curWeight);

	if (_fovSlider)
		_fov = static_cast<int>(_fovSlider->curWeight);

	if (_invertYAxisCheckBox)
		_invertYAxis = _invertYAxisCheckBox->isChecked;

	if (_vSyncCheckBox)
		_vSync = _vSyncCheckBox->isChecked;


	// 그래픽스 세팅 적용
	{
		_config->width = _screenWidth;
		_config->height = _screenHeight;
		_config->isFullScreen = _fullScreen;
		_config->isWindowedFullScreen = _windowedFullScreen;
		_config->enableVSync = _vSync;

		core::OnChangeResolution changeResolution;
		changeResolution.width =  _screenWidth;
		changeResolution.height = _screenHeight;
		changeResolution.isFullScreen = _fullScreen;
		changeResolution.isWindowedFullScreen = _windowedFullScreen;

		_dispatcher->trigger<core::OnChangeResolution>(static_cast<core::OnChangeResolution>(changeResolution));
	}

	// 사운드 세팅 적용
	{
		// example
		//core::OnChangeMasterVolume changeMasterVolume;
		//changeMasterVolume.volume = _masterVolume;

		//_dispatcher->trigger<core::OnChangeMasterVolume>(static_cast<core::OnChangeMasterVolume>(changeMasterVolume));

	}
	// 게임 세팅 적용
	{
		// example
		//core::OnChangeMouseSensitivity changeMouseSensitivity;
		//changeMouseSensitivity.sensitivity = _mouseSensitivity;

		//_dispatcher->trigger<core::OnChangeMouseSensitivity>(static_cast<core::OnChangeMouseSensitivity>(changeMouseSensitivity));

		//core::OnChangeFOV changeFOV;
		//changeFOV.fov = _fov;

		//_dispatcher->trigger<core::OnChangeFOV>(static_cast<core::OnChangeFOV>(changeFOV));

		//core::OnChangeInvertYAxis changeInvertYAxis;
		//changeInvertYAxis.invertYAxis = _invertYAxis;

		//_dispatcher->trigger<core::OnChangeInvertYAxis>(static_cast<core::OnChangeInvertYAxis>(changeInvertYAxis));

	}

	saveConfigData();

}

void mc::GameSettingSystem::discardChange(const OnDiscardChange& event)
{

}

void mc::GameSettingSystem::loadConfigData()
{
	_screenWidth = GetPrivateProfileInt(GRAPHICS_SECTION, SCREEN_WIDTH, 1920, CONFIG_FILE);
	_screenHeight = GetPrivateProfileInt(GRAPHICS_SECTION, SCREEN_HEIGHT, 1080, CONFIG_FILE);
	_fullScreen = GetPrivateProfileInt(GRAPHICS_SECTION, FULLSCREEN, 0, CONFIG_FILE);
	_windowedFullScreen = GetPrivateProfileInt(GRAPHICS_SECTION, WINDOWED_FULLSCREEN, 0, CONFIG_FILE);
	_vSync = GetPrivateProfileInt(GRAPHICS_SECTION, VSYNC, 0, CONFIG_FILE);

	_masterVolume = GetPrivateProfileInt(SOUND_SECTION, MASTER_VOLUME, 100, CONFIG_FILE);

	_mouseSensitivity = GetPrivateProfileInt(GAME_SETTING_SECTION, MOUSE_SENSITIVITY, 100, CONFIG_FILE);
	_fov = GetPrivateProfileInt(GAME_SETTING_SECTION, FOV, 90, CONFIG_FILE);
	_invertYAxis = GetPrivateProfileInt(GAME_SETTING_SECTION, INVERT_Y_AXIS, 0, CONFIG_FILE);
}

void mc::GameSettingSystem::saveConfigData()
{
	WritePrivateProfileString(GRAPHICS_SECTION, SCREEN_WIDTH, std::to_wstring(_screenWidth).c_str(), CONFIG_FILE);
	WritePrivateProfileString(GRAPHICS_SECTION, SCREEN_HEIGHT, std::to_wstring(_screenHeight).c_str(), CONFIG_FILE);
	WritePrivateProfileString(GRAPHICS_SECTION, FULLSCREEN, std::to_wstring(_fullScreen).c_str(), CONFIG_FILE);
	WritePrivateProfileString(GRAPHICS_SECTION, WINDOWED_FULLSCREEN, std::to_wstring(_windowedFullScreen).c_str(), CONFIG_FILE);
	WritePrivateProfileString(GRAPHICS_SECTION, VSYNC, std::to_wstring(_vSync).c_str(), CONFIG_FILE);

	WritePrivateProfileString(SOUND_SECTION, MASTER_VOLUME, std::to_wstring(_masterVolume).c_str(), CONFIG_FILE);

	WritePrivateProfileString(GAME_SETTING_SECTION, MOUSE_SENSITIVITY, std::to_wstring(_mouseSensitivity).c_str(), CONFIG_FILE);
	WritePrivateProfileString(GAME_SETTING_SECTION, FOV, std::to_wstring(_fov).c_str(), CONFIG_FILE);
	WritePrivateProfileString(GAME_SETTING_SECTION, INVERT_Y_AXIS, std::to_wstring(_invertYAxis).c_str(), CONFIG_FILE);
}
