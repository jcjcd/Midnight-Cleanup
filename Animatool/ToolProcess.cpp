#include "pch.h"
#include "ToolProcess.h"

#include <d3d12.h>
#include <imgui_impl_dx11.h>
#include "imgui_impl_dx12.h"
#include <imgui_impl_win32.h>

#include <Animacore/Timer.h>
#include <Animacore/Scene.h>
#include <Animacore/AnimatorSystem.h>
#include <Animacore/TransformSystem.h>
#include <Animacore/CoreSystemEvents.h>

#include "Scene.h"
#include "Project.h"
#include "MenuBar.h"
#include "Console.h"
#include "Hierarchy.h"
#include "Inspector.h"
#include "AnimatorPanel.h"
#include "Animacore/CorePhysicsComponents.h"
#include "Animacore/CoreTagsAndLayers.h"
#include "Animavision/IModelParser.h"
#include "Animavision/Mesh.h"
#include "Animavision/ModelLoader.h"
#include "midnight_cleanup/McComponents.h"
#include "midnight_cleanup/McTagsAndLayers.h"


extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

tool::ToolProcess::ToolProcess(const core::ProcessInfo& info)
	: mc::McProcess(info)
{
	// ImGui 초기화
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;


	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(_hWnd);

	if (_processInfo.api == Renderer::API::DirectX11)
	{
		auto device = static_cast<ID3D11Device*>(_renderer->GetDevice());
		auto context = static_cast<ID3D11DeviceContext*>(_renderer->GetDeviceContext());

		ImGui_ImplDX11_Init(device, context);
	}
	if (_processInfo.api == Renderer::API::DirectX12)
	{
		auto device = static_cast<ID3D12Device*>(_renderer->GetDevice());

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};

		auto heap = _renderer->AllocateShaderVisibleDescriptor(&cpuHandle, &gpuHandle);

		_descriptorHeap = static_cast<ID3D12DescriptorHeap*>(heap);

		ImGui_ImplDX12_Init(device, 2, DXGI_FORMAT_R8G8B8A8_UNORM, _descriptorHeap,
			cpuHandle,
			gpuHandle);
	}
	scene = new core::Scene();

	// 이벤트 연결
	_dispatcher.sink<OnToolLoadScene>().connect<&ToolProcess::loadScene>(this);
	_dispatcher.sink<OnToolLoadPrefab>().connect<&ToolProcess::loadPrefab>(this);
	_dispatcher.sink<OnToolDestroyEntity>().connect<&ToolProcess::destroyEntity>(this);
	_dispatcher.sink<OnToolPlayScene>().connect<&ToolProcess::playScene>(this);
	_dispatcher.sink<OnToolStopScene>().connect<&ToolProcess::stopScene>(this);
	_dispatcher.sink<OnToolPauseScene>().connect<&ToolProcess::pauseScene>(this);
	_dispatcher.sink<OnToolResumeScene>().connect<&ToolProcess::resumeScene>(this);
	_dispatcher.sink<OnToolRequestAddPanel>().connect<&ToolProcess::requestAddPanel>(this);
	_dispatcher.sink<OnToolRemovePanel>().connect<&ToolProcess::removePanel>(this);
	_dispatcher.sink<OnToolLoadFbx>().connect<&ToolProcess::loadFbx>(this);
	_dispatcher.sink<OnToolShowFPS>().connect<&ToolProcess::showFps>(this);

	scene->GetDispatcher()->sink<core::OnChangeScene>().connect<&ToolProcess::changeScene>(this);
	scene->GetDispatcher()->sink<core::OnChangeResolution>().connect<&ToolProcess::changeResolution>(this);

	// 헬퍼 이벤트 연결
	_dispatcher.sink<OnProcessSnapEntities>().connect<&ToolProcess::processSnapEntities>(this);

	// 씬 이벤트 연결 (todo: 씬이 여러개가 된다면 분리 설정 필요)
	scene->GetDispatcher()->sink<core::OnThrow>().connect<&ToolProcess::throwException>(this);

	_renderer->LoadMeshesFromDrive("./Resources/Models");
	if (_renderer->IsRayTracing())
	{
		_renderer->InitializeTopLevelAS();
	}
	_renderer->LoadMaterialsFromDrive("./Resources");
	_renderer->LoadShadersFromDrive("./Shaders");
	_renderer->LoadAnimationClipsFromDrive("./Resources/Animations");
	_renderer->LoadUITexturesFromDrive("./Resources/UI");
	_renderer->LoadParticleTexturesFromDrive("./Resources/Particles");
	_renderer->LoadFontsFromDrive("./Resources/Fonts");

	core::AnimatorSystem::controllerManager.LoadControllersFromDrive("./Resources", _renderer.get());

	// 패널 생성
	_panels.emplace_back(new Project(_dispatcher, _renderer.get()));
	_panels.emplace_back(new MenuBar(_dispatcher, _renderer.get(), *this));
	_panels.emplace_back(new Hierarchy(_dispatcher));
	_panels.emplace_back(new Inspector(_dispatcher, *_renderer.get()));
	_panels.emplace_back(new Scene(_dispatcher, _renderer.get()));
	_panels.emplace_back(new Console(_dispatcher));

	setStyle();
	loadIcons();
}

tool::ToolProcess::~ToolProcess()
{
	if (_processInfo.api == Renderer::API::DirectX11)
	{
		ImGui_ImplDX11_Shutdown();
	}
	if (_processInfo.api == Renderer::API::DirectX12)
	{
		ImGui_ImplDX12_Shutdown();
	}

	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	for (auto& panel : _panels)
	{
		delete panel;
		panel = nullptr;
	}

	delete scene;
	scene = nullptr;
}

void tool::ToolProcess::Loop()
{
	static core::Timer timer;


	MSG msg;

	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (tool::ToolProcess::isResized)
			{
				RECT rect;
				GetClientRect(_hWnd, &rect);
				_renderer->Resize(rect.right, rect.bottom);

				Scene* toolScene = nullptr;
				for (auto* panel : _panels)
				{
					if (panel->GetType() == PanelType::Scene)
					{
						toolScene = static_cast<Scene*>(panel);
						break;
					}
				}
				toolScene->Resize(rect.right, rect.bottom);

				tool::ToolProcess::isResized = false;
				_processInfo.width = rect.right;
				_processInfo.height = rect.bottom;
			}
		}
		else
		{
			timer.Update();
			const auto tick = timer.GetTick();

			// 씬 재생
			if (_isPlaying && !_isPaused)
				scene->Update(tick);

			// 렌더링
			if (!tool::ToolProcess::isMinimized)
			{
				_renderer->BeginRender();

				if (_isPlaying)
					scene->Render(tick, _renderer.get());

				beginGui();
				renderGui(tick);
				endGui();

				_renderer->EndRender();
			}

			if (_isPlaying && !_isPaused)
			{
				scene->ProcessEvent();
				if (_showFps)
				{
					_fpsElapsedTime += tick;

					if (_fpsElapsedTime > 1.f)
					{
						auto fps = timer.GetFPS();
						LOG_INFO(*scene, "FPS : {}", fps);
						_fpsElapsedTime -= 1.f;
					}
				}

				mouseWheel = 0;
			}

			// 트랜스폼 업데이트
			core::TransformSystem tfSystem{ *scene };
			if (!_isPlaying or _isPaused)
				tfSystem.Update(*scene, tick);

			// 씬과 인풋시스템 동기화
			if (_scenePanel != nullptr)
			{
				auto&& input = scene->GetRegistry()->ctx().get<core::Input>();
				auto viewportPos = _scenePanel->viewportPos;
				viewportPos.y += 22.f;
				input.viewportPosition = viewportPos;
				input.viewportSize = _scenePanel->viewportSize;
			}

			// 툴 이벤트 처리
			_dispatcher.update();

			// 자동 저장
			if (!_isPlaying && !_isPaused)
			{
				_autoSaveElapsedTime += tick;

				if (_autoSaveElapsedTime > AUTO_SAVE_LIMIT)
				{
					scene->SaveScene(TEMP_SCENE_PATH);
					_autoSaveElapsedTime -= AUTO_SAVE_LIMIT;
				}
			}

			changeExecution();
		}
	}
}

bool tool::ToolProcess::checkIsExist(PanelType type)
{
	for (auto* panel : _panels)
	{
		if (panel->GetType() == type)
			return true;
	}

	return false;
}

void tool::ToolProcess::loadScene(const OnToolLoadScene& event)
{
	scene->Clear();
	scene->LoadScene(event.path);

	// 씬 로드 이후 처리
	_dispatcher.trigger<OnToolPostLoadScene>({ scene });
}

void tool::ToolProcess::playScene(const OnToolPlayScene& event)
{
	_isPlaying = true;
	_isPaused = false;

	scene->SaveScene(TEMP_SCENE_PATH);
	scene->Start(_renderer.get());

	for (auto& panel : _panels)
	{
		if (panel->GetType() == PanelType::Scene)
		{
			_scenePanel = static_cast<Scene*>(panel);
			break;
		}
	}

	_sceneName = scene->GetName();
}

void tool::ToolProcess::stopScene(const OnToolStopScene& event)
{
	_isPlaying = false;
	_isPaused = false;

	_scenePanel = nullptr;

	// BGM을 종료
	if (scene->GetRegistry()->ctx().contains<core::BGM>())
	{
		auto& bgm = scene->GetRegistry()->ctx().get<core::BGM>().sound;
		bgm.isPlaying = false;
		bgm.isLoop = false;
		scene->GetDispatcher()->trigger<core::OnUpdateBGMState>({ *scene });
	}

	scene->Finish(_renderer.get());
	scene->LoadScene(TEMP_SCENE_PATH);

	scene->SetName(_sceneName);
	scene->GetDispatcher()->trigger<core::OnSetTimeScale>(core::OnSetTimeScale{ 1.0f });


}

void tool::ToolProcess::pauseScene(const OnToolPauseScene& event)
{
	_isPaused = true;
}

void tool::ToolProcess::saveTempScene(const OnToolSaveTempScene& event)
{
	scene->SaveScene(TEMP_SCENE_PATH);
}

void tool::ToolProcess::resumeScene(const OnToolResumeScene& event)
{
	_isPaused = false;
}

void tool::ToolProcess::loadPrefab(const OnToolLoadPrefab& event)
{
	std::filesystem::path path(event.path);

	if (path.extension() == core::Scene::PREFAB_EXTENSION)
	{
		scene->LoadPrefab(path);
		//_dispatcher.enqueue<OnToolPostLoadPrefab>(event.path, scene);
	}
}

void tool::ToolProcess::loadFbx(const OnToolLoadFbx& event)
{
	auto newEntity = scene->CreateEntity();

	ModelParserFlags flags = ModelParserFlags::NONE;
	flags |= ModelParserFlags::TRIANGULATE;
	//flags |= ModelParserFlags::GEN_NORMALS;
	flags |= ModelParserFlags::GEN_SMOOTH_NORMALS;
	flags |= ModelParserFlags::GEN_UV_COORDS;
	flags |= ModelParserFlags::CALC_TANGENT_SPACE;
	flags |= ModelParserFlags::GEN_BOUNDING_BOXES;
	flags |= ModelParserFlags::MAKE_LEFT_HANDED;
	flags |= ModelParserFlags::FLIP_UVS;
	flags |= ModelParserFlags::FLIP_WINDING_ORDER;
	flags |= ModelParserFlags::LIMIT_BONE_WEIGHTS;
	flags |= ModelParserFlags::JOIN_IDENTICAL_VERTICES;
	flags |= ModelParserFlags::GLOBAL_SCALE;

	auto model = ModelLoader::CreateForEntity(event.path, flags);

	auto registry = scene->GetRegistry();

	std::function<void(core::Entity entity, EntityNode* node)> pushChildAndComponents = [&](core::Entity entity, EntityNode* node)
		{
			// Name 컴포넌트 가져오기
			entity.Get<core::Name>().name = node->name;
			auto& local = entity.Get<core::LocalTransform>();
			Vector3 position, scale;
			Quaternion rotation;
			node->NodeTransform.Decompose(scale, rotation, position);
			local.position = position;
			local.rotation = rotation;
			local.scale = scale;

			if (node->meshCount > 0)
			{
				auto& meshRenderer = entity.Emplace<core::MeshRenderer>();
				meshRenderer.meshString = node->name;

				if (node->isSkinned)
					meshRenderer.isSkinned = true;

				for (uint32_t i = 0; i < node->meshCount; i++)
				{
					if (meshRenderer.isSkinned)
						meshRenderer.materialStrings.push_back("./Resources/Materials/Skin.material");
					else
						meshRenderer.materialStrings.push_back({});
				}
			}

			for (auto& child : node->children)
			{
				auto childEntity = ToolProcess::scene->CreateEntity();
				childEntity.SetParent(entity);
				pushChildAndComponents(childEntity, child);
			}
		};

	pushChildAndComponents(newEntity, model->m_EntityRootNode);

	// 가장 부모는 fbx이름과 같게
	// 근데 파일 이름만 잘라서 넣기
	std::filesystem::path path = event.path;
	std::string name = path.filename().string();
	name = name.substr(0, name.find_last_of('.'));
	newEntity.Get<core::Name>().name = name;
}

void tool::ToolProcess::destroyEntity(const OnToolDestroyEntity& event)
{
	scene->DestroyEntityImmediately(event.entity);
}

void tool::ToolProcess::requestAddPanel(const tool::OnToolRequestAddPanel& event)
{
	if (checkIsExist(event.panelType))
		return;

	switch (event.panelType)
	{
	case PanelType::Animator:
		_panels.emplace_back(new AnimatorPanel(_dispatcher, _renderer.get()));
		break;
	default:
		break;
	}

	_dispatcher.enqueue<OnToolAddedPanel>(event.panelType, _panels.back());
}

void tool::ToolProcess::removePanel(const tool::OnToolRemovePanel& event)
{
	bool isExist = false;
	auto iter = _panels.begin();

	for (; iter != _panels.end(); ++iter)
	{
		if ((*iter)->GetType() == event.panelType)
		{
			isExist = true;
			break;
		}
	}

	if (isExist)
	{
		delete (*iter);
		_panels.erase(iter);
	}
}

void tool::ToolProcess::changeScene(const core::OnChangeScene& event)
{
	_receiveChangeEvent = true;
	_scenePath = event.path;
}

void tool::ToolProcess::changeExecution()
{
	if (_receiveChangeEvent)
	{
		scene->Finish(_renderer.get());
		scene->LoadScene(_scenePath);
		scene->Start(_renderer.get());

		_receiveChangeEvent = false;
		_scenePath.clear();
	}
}

void tool::ToolProcess::showFps(const OnToolShowFPS& event)
{
	_showFps = event.show;
}


void tool::ToolProcess::throwException(const core::OnThrow& event)
{
	switch (event.exceptionType)
	{
	case core::OnThrow::Type::Info:
		spdlog::info(event.message);
		break;
	case core::OnThrow::Type::Warn:
		spdlog::warn(event.message);
		break;
	case core::OnThrow::Type::Error:
		spdlog::error(event.message);
		break;
	case core::OnThrow::Type::Debug:
		spdlog::debug(event.message);
		break;
	}
}

void tool::ToolProcess::changeResolution(const core::OnChangeResolution& event)
{
	//if (event.isFullScreen)
	//{
	//	_renderer->SetFullScreen(true);
	//}
	//else
	//{
	//	if (_renderer->IsFullScreen())
	//		_renderer->SetFullScreen(false);

	//	if (event.isWindowedFullScreen)
	//	{
	//		int width = GetSystemMetrics(SM_CXSCREEN);
	//		int height = GetSystemMetrics(SM_CYSCREEN);

	//		// 윈도우 크기를 해당 해상도로 변경
	//		SetWindowPos(_hWnd, HWND_TOP, 0, 0, width, height, SWP_SHOWWINDOW);
	//		_renderer->Resize(width, height);

	//		// 윈도우 스타일을 팝업으로 변경
	//		SetWindowLong(_hWnd, GWL_STYLE, WS_POPUP);
	//		ShowWindow(_hWnd, SW_MAXIMIZE);

	//	}
	//	else
	//	{
	//		if (event.width > 0 && event.height > 0)
	//		{
	//			// 윈도우 크기를 해당 해상도로 변경
	//			SetWindowPos(_hWnd, HWND_TOP, 0, 0, event.width, event.height, SWP_SHOWWINDOW);
	//			_renderer->Resize(event.width, event.height);

	//			// 윈도우 스타일을 팝업으로 변경
	//			SetWindowLong(_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
	//			ShowWindow(_hWnd, SW_SHOWNORMAL);
	//		}
	//	}
	//}

	scene->GetDispatcher()->trigger<core::OnDestroyRenderResources>({ *scene, _renderer.get() });
	scene->GetDispatcher()->trigger<core::OnCreateRenderResources>({ *scene, _renderer.get() });
	scene->GetDispatcher()->trigger<core::OnLateCreateRenderResources>({ *scene, _renderer.get() });
}

void tool::ToolProcess::processSnapEntities(const OnProcessSnapEntities& event)
{
	auto registry = scene->GetRegistry();
	auto&& view = registry->view<core::WorldTransform, core::MeshRenderer, mc::SnapAvailable>();

	for (auto [entity, world, meshRenderer, snapAvailable] : view.each())
	{
		// 이미 relationship이 있으면 넘어감
		if (registry->any_of<core::Relationship>(entity))
			continue;

		// 이름 가져옴
		auto& name = registry->get_or_emplace<core::Name>(entity).name;
		{
			meshRenderer.canReceivingDecal = false;

			// 해당 엔티티에 ColliderCommon, RigidBody, meshCollider추가
			auto& rigidbody = registry->get_or_emplace<core::Rigidbody>(entity);
			rigidbody.mass = 10.f;
			rigidbody.drag = 2.f;
			rigidbody.angularDrag = 1.f;
			rigidbody.useGravity = true;
			rigidbody.constraints = core::Rigidbody::Constraints::FreezeAll;

			auto& collider = registry->get_or_emplace<core::ColliderCommon>(entity);

			auto& meshCollider = registry->get_or_emplace<core::MeshCollider>(entity);
			meshCollider.convex = true;

			// tag, layer 추가
			auto& tag = registry->get_or_emplace<core::Tag>(entity);
			tag.id = tag::Mess::id;

			auto& layer = registry->get_or_emplace<core::Layer>(entity);
			layer.mask = layer::Pickable::mask | layer::Outline::mask;

			// renderAttribute, Room 추가
			auto& renderAttributes = registry->get_or_emplace<core::RenderAttributes>(entity);

			auto& room = registry->get_or_emplace<mc::Room>(entity);
		}

		/// 씬에 새로운 엔티티 추가
		auto newEntity = scene->CreateEntity();
		// 새로운 엔티티 이름을 기존 엔티티 이름 + _H로 만듬
		newEntity.Get<core::Name>().name = name + "_H";

		{
			// 새로운 엔티티에 ColliderCommon, meshCollider추가
			auto& collider = registry->get_or_emplace<core::ColliderCommon>(newEntity);
			collider.isTrigger = true;

			auto& meshCollider = registry->get_or_emplace<core::MeshCollider>(newEntity);
			meshCollider.convex = true;

			// tag, layer 추가
			auto& tag = registry->get_or_emplace<core::Tag>(newEntity);
			tag.id = tag::Hologram::id;

			auto& layer = registry->get_or_emplace<core::Layer>(newEntity);
			layer.mask = layer::Hologram::mask;

			// 메쉬 렌더러 추가
			auto& newMeshRenderer = registry->get_or_emplace<core::MeshRenderer>(newEntity);
			newMeshRenderer.isOn = false;
			newMeshRenderer.meshString = meshRenderer.meshString;
			newMeshRenderer.materialStrings.resize(1);
			newMeshRenderer.materialStrings.front() = "./Resources/Materials/MI_Hologram.material";
			newMeshRenderer.canReceivingDecal = false;
			newMeshRenderer.receiveShadow = false;

			// renderAttributes 추가
			auto& renderAttributes = registry->get_or_emplace<core::RenderAttributes>(newEntity);
			renderAttributes.flags = core::RenderAttributes::IsTransparent;

			// 새로운 엔티티의 트랜스폼을 가지고 옴
			auto& newWorld = registry->get_or_emplace<core::WorldTransform>(newEntity);
			newWorld = world;
			registry->patch<core::WorldTransform>(newEntity);
		}

		// 새로운 엔티티의 자식으로 원래 엔티티를 집어넣음
		core::Entity(entity, *registry).SetParent(newEntity);

		// snapAvailable에 부모를 집어넣음
		snapAvailable.placedSpot = newEntity;

		// 기존 엔티티의 로컬을 초기화시킴
		auto& local = registry->get_or_emplace<core::LocalTransform>(entity);
		local.position = Vector3::Zero;
		local.rotation = Quaternion::Identity;
		local.scale = Vector3::One;
		registry->patch<core::LocalTransform>(entity);
	}

}

void tool::ToolProcess::renderGui(float deltaTime)
{
	ImGui::DockSpaceOverViewport();

	for (auto panel : _panels)
		panel->RenderPanel(deltaTime);
}

void tool::ToolProcess::beginGui()
{
	if (_processInfo.api == Renderer::API::DirectX11)
	{
		ImGui_ImplDX11_NewFrame();
	}
	else if (_processInfo.api == Renderer::API::DirectX12)
	{
		ImGui_ImplDX12_NewFrame();
	}

	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();
}

void tool::ToolProcess::endGui()
{
	ImGui::Render();

	if (_processInfo.api == Renderer::API::DirectX11)
	{
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
	else if (_processInfo.api == Renderer::API::DirectX12)
	{
		auto context = static_cast<ID3D12GraphicsCommandList*>(_renderer->GetRenderCommandList());
		auto descriptorHeap = _descriptorHeap;
		context->SetDescriptorHeaps(1, &descriptorHeap);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), context);

	}
}

void tool::ToolProcess::loadIcons()
{
	auto toolDirectory = std::filesystem::recursive_directory_iterator("./Resources/Icons/");

	for (auto&& file : toolDirectory)
	{
		if (!file.exists())
			continue;

		auto texture = _renderer->CreateTexture(file.path().string().c_str());

		if (!texture)
			continue;

		auto fileName = file.path().filename().replace_extension("").string();
		icons.insert({ fileName, texture->GetShaderResourceView() });
	}
}

void tool::ToolProcess::setStyle()
{
	auto&& io = ImGui::GetIO();

	// 폰트 추가
	io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\Arial.ttf)", 16.0f);

	ImFontConfig config;
	config.MergeMode = true;
	io.Fonts->AddFontFromFileTTF(
		R"(C:\Windows\Fonts\malgun.ttf)",
		16.0f, &config, io.Fonts->GetGlyphRangesKorean());

	auto& colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.1f, 0.13f, 1.0f };
	colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

	// Border
	colors[ImGuiCol_Border] = ImVec4{ 0.44f, 0.37f, 0.61f, 0.29f };
	colors[ImGuiCol_BorderShadow] = ImVec4{ 0.0f, 0.0f, 0.0f, 0.24f };

	// Text
	colors[ImGuiCol_Text] = ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f };
	colors[ImGuiCol_TextDisabled] = ImVec4{ 0.5f, 0.5f, 0.5f, 1.0f };

	// Headers
	colors[ImGuiCol_Header] = ImVec4{ 0.13f, 0.13f, 0.17f, 1.0f };
	colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, 1.0f };
	colors[ImGuiCol_HeaderActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

	// Buttons
	colors[ImGuiCol_Button] = ImVec4{ 0.13f, 0.13f, 0.17f, 1.0f };
	colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, 1.0f };
	colors[ImGuiCol_ButtonActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
	colors[ImGuiCol_CheckMark] = ImVec4{ 0.74f, 0.58f, 0.98f, 1.0f };

	// Popups
	colors[ImGuiCol_PopupBg] = ImVec4{ 0.1f, 0.1f, 0.13f, 0.92f };

	// Slider
	colors[ImGuiCol_SliderGrab] = ImVec4{ 0.44f, 0.37f, 0.61f, 0.54f };
	colors[ImGuiCol_SliderGrabActive] = ImVec4{ 0.74f, 0.58f, 0.98f, 0.54f };

	// Frame BG
	colors[ImGuiCol_FrameBg] = ImVec4{ 0.13f, 0.13f, 0.17f, 1.0f };
	colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, 1.0f };
	colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.16f, 0.16f, 0.99f, 1.0f };

	// Tabs
	colors[ImGuiCol_Tab] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
	colors[ImGuiCol_TabHovered] = ImVec4{ 0.24f, 0.24f, 0.32f, 1.0f };
	colors[ImGuiCol_TabActive] = ImVec4{ 0.2f, 0.22f, 0.27f, 1.0f };
	colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

	// Title
	colors[ImGuiCol_TitleBg] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
	colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

	// Scrollbar
	colors[ImGuiCol_ScrollbarBg] = ImVec4{ 0.1f, 0.1f, 0.13f, 1.0f };
	colors[ImGuiCol_ScrollbarGrab] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, 1.0f };
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4{ 0.24f, 0.24f, 0.32f, 1.0f };

	// Seperator
	colors[ImGuiCol_Separator] = ImVec4{ 0.44f, 0.37f, 0.61f, 1.0f };
	colors[ImGuiCol_SeparatorHovered] = ImVec4{ 0.74f, 0.58f, 0.98f, 1.0f };
	colors[ImGuiCol_SeparatorActive] = ImVec4{ 0.84f, 0.58f, 1.0f, 1.0f };

	// Resize Grip
	colors[ImGuiCol_ResizeGrip] = ImVec4{ 0.44f, 0.37f, 0.61f, 0.29f };
	colors[ImGuiCol_ResizeGripHovered] = ImVec4{ 0.74f, 0.58f, 0.98f, 0.29f };
	colors[ImGuiCol_ResizeGripActive] = ImVec4{ 0.84f, 0.58f, 1.0f, 0.29f };

	// Docking
	colors[ImGuiCol_DockingPreview] = ImVec4{ 0.44f, 0.37f, 0.61f, 1.0f };

	auto& style = ImGui::GetStyle();
	style.TabRounding = 4;
	style.ScrollbarRounding = 9;
	style.WindowRounding = 7;
	style.GrabRounding = 3;
	style.FrameRounding = 3;
	style.PopupRounding = 4;
	style.ChildRounding = 4;
}
