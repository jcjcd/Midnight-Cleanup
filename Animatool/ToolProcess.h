#pragma once
#include <midnight_cleanup/McProcess.h>

#include "ToolEvents.h"

#include <../cppwinrt/winrt/base.h>

namespace core
{
	struct OnThrow;
}

struct ID3D12DescriptorHeap;
struct ID3D12Resource;

namespace tool
{
	class Panel;
	class Scene;

	class ToolProcess : public mc::McProcess
	{
	public:
		ToolProcess(const core::ProcessInfo& info);
		~ToolProcess() override;

		void Loop() override;

		inline static core::Scene* scene;
		inline static bool isResized = false;
		inline static bool isMinimized = false;
		inline static std::unordered_map<std::string, ImTextureID> icons;

	private:
		// tool internal func
		bool checkIsExist(PanelType type);

		// event func
		void loadScene(const OnToolLoadScene& event);
		void playScene(const OnToolPlayScene& event);
		void stopScene(const OnToolStopScene& event);
		void pauseScene(const OnToolPauseScene& event);
		void saveTempScene(const OnToolSaveTempScene& event);
		void resumeScene(const OnToolResumeScene& event);
		void loadPrefab(const OnToolLoadPrefab& event);
		void loadFbx(const OnToolLoadFbx& event);
		void destroyEntity(const OnToolDestroyEntity& event);
		void requestAddPanel(const OnToolRequestAddPanel& event);
		void removePanel(const OnToolRemovePanel& event);
		void changeScene(const core::OnChangeScene& event);
		void changeExecution();
		void showFps(const OnToolShowFPS& event);

		// scene exception event
		void throwException(const core::OnThrow& event);

		// scene event
		void changeResolution(const core::OnChangeResolution& event);

		// mc Helper func
		void processSnapEntities(const OnProcessSnapEntities& event);

		// gui loop
		void renderGui(float deltaTime);
		void beginGui();
		void endGui();

		// tool settings
		void loadIcons();
		void setStyle();

		entt::dispatcher _dispatcher;
		std::vector<Panel*> _panels;
		Scene* _scenePanel = nullptr;
		std::string _sceneName;

		ID3D12DescriptorHeap* _descriptorHeap = nullptr;
		winrt::com_ptr<ID3D12Resource> _rtvTexture = nullptr;

		bool _isPlaying = false;
		bool _isPaused = false;

		bool _showFps = false;
		float _fpsElapsedTime = 0.f;

		double _autoSaveElapsedTime = 0.f;

		bool _receiveChangeEvent;
		std::filesystem::path _scenePath;

		inline static constinit double AUTO_SAVE_LIMIT = 60.f;
		inline static constinit const char* TEMP_SCENE_PATH = "./temp.scene";
	};
}
