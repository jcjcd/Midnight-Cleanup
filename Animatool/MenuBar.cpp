#include "pch.h"
#include "MenuBar.h"

#include "ToolProcess.h"
#include "MaterialPanel.h"
#include "AnimatorPanel.h"

#include <Animacore/Scene.h>
#include <Animacore/MetaCtxs.h>
#include <Animacore/Importer.h>
#include <Animacore/CoreComponents.h>

static constexpr const char* UNITY_SCENE_EXTENSION = ".unityscene";

tool::MenuBar::MenuBar(entt::dispatcher& dispatcher, Renderer* renderer, ToolProcess& toolProcess)
	: Panel(dispatcher)
	, _renderer(renderer)
	, _toolProcess(&toolProcess)
{
	_dispatcher->sink<OnToolModifyScene>().connect<&MenuBar::modifyScene>(this);
	_dispatcher->sink<OnToolLoadScene>().connect<&MenuBar::loadScene>(this);
	_dispatcher->sink<OnToolRemoveSystem>().connect<&MenuBar::removeSystem>(this);

	_fileDialog = ImGui::FileBrowser{
		ImGuiFileBrowserFlags_EnterNewFilename |
		ImGuiFileBrowserFlags_CloseOnEsc |
		ImGuiFileBrowserFlags_CreateNewDir |
		ImGuiFileBrowserFlags_ConfirmOnEnter |
		ImGuiFileBrowserFlags_HideRegularFiles |
		ImGuiFileBrowserFlags_NoStatusBar
	};

	_fileDialog.SetTypeFilters({
		core::Scene::SCENE_EXTENSION,
		UNITY_SCENE_EXTENSION
		});

	_fileDialog.SetPwd("./Resources/Scenes/");
	_currentScenePath = _fileDialog.GetPwd();
}

void tool::MenuBar::RenderPanel(float deltaTime)
{
	renderMenu();
	checkSelected();
	renderSavePopup();

	// pop-ups
	renderSystemManager();
	renderConfiguration();
}

void tool::MenuBar::renderMenu()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Scene", "Ctrl+N"))
			{
				if (_needSave)
				{
					_showSavePopup = true;
					_nextAction = MenuType::NewScene;
				}
				else
				{
					ToolProcess::scene->Clear();
					_currentScenePath.clear(); // 경로 초기화
				}
			}
			if (ImGui::MenuItem("Open Scene", "Ctrl+O"))
			{
				_currentMenu = MenuType::OpenScene;

				if (_needSave)
				{
					_showSavePopup = true;
					_nextAction = MenuType::OpenScene;
				}
				else
				{
					_fileDialog.Open();
				}
			}
			if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
			{
				saveScene();
			}
			if (ImGui::MenuItem("Save As ..."))
			{
				_currentMenu = MenuType::SaveAs;
				_fileDialog.Open();
			}
			if (ImGui::BeginMenu("Load"))  // Load 메뉴 시작
			{
				if (ImGui::MenuItem("Load Unity Scene (erase exists)"))
				{
					_currentMenu = MenuType::LoadNewFromFile;
					_fileDialog.Open();
				}
				if (ImGui::MenuItem("Load Unity Prefab (don't erase exists)"))
				{
					_currentMenu = MenuType::LoadNewFromFile;
					_fileDialog.Open();
				}

				ImGui::EndMenu();  // Load 메뉴 종료
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Window"))
		{
			if (ImGui::MenuItem("Animator"))
			{
				_dispatcher->enqueue<OnToolRequestAddPanel>(PanelType::Animator);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Settings"))
		{
			if (ImGui::MenuItem("Manage Systems"))
			{
				_showSystemManager = true;
			}
			if (ImGui::MenuItem("Configuration"))
			{
				_showConfig = true;
			}
			if (ImGui::MenuItem("Apply Current Cam"))
			{
				_dispatcher->trigger<OnToolApplyCamera>();
			}
			if (ImGui::MenuItem("Generate Snap Entities"))
			{
				_dispatcher->trigger<OnProcessSnapEntities>();
			}
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	// 파일 다이얼로그 출력
	_fileDialog.Display();
}

void tool::MenuBar::checkSelected()
{
	if (_fileDialog.HasSelected())
	{
		const auto selected = _fileDialog.GetSelected();

		if (_currentMenu == MenuType::OpenScene)
		{
			ToolProcess::scene->Clear();
			ToolProcess::scene->LoadScene(selected);
			_currentScenePath = selected; // 파일 경로 저장
			_needSave = false;
			_currentMenu = MenuType::None;
			_fileDialog.ClearSelected();
		}
		else if (_currentMenu == MenuType::SaveAs)
		{
			ToolProcess::scene->SaveScene(selected, false);
			_currentScenePath = selected; // 파일 경로 저장
			_needSave = false;
			_currentMenu = MenuType::None;
			_fileDialog.ClearSelected();
		}
		else if (_currentMenu == MenuType::LoadNewFromFile)
		{
			core::Importer::LoadUnityNew(selected, *ToolProcess::scene);
			_currentMenu = MenuType::None;
			_fileDialog.ClearSelected();
		}
		else if (_currentMenu == MenuType::LoadContinuousFromFile)
		{
			core::Importer::LoadUnityContinuous(selected, *ToolProcess::scene);
			_currentMenu = MenuType::None;
			_fileDialog.ClearSelected();
		}
	}
	else if (!_fileDialog.IsOpened() && _currentMenu == MenuType::SaveAs)
	{
		// 파일 다이얼로그가 닫혔고 선택이 취소된 경우
		_nextAction = MenuType::None;
		_currentMenu = MenuType::None;
	}
}

void tool::MenuBar::renderSavePopup()
{
	if (_showSavePopup)
	{
		ImGui::OpenPopup("Save Changes?");
		_showSavePopup = false;
	}

	if (ImGui::BeginPopupModal("Save Changes?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("You have unsaved changes. Would you like to save them?");
		ImGui::Separator();

		if (ImGui::Button("Yes", ImVec2(120, 0)))
		{
			saveScene();
			_needSave = false;

			performNextAction();
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("No", ImVec2(120, 0)))
		{
			ToolProcess::scene->Clear();
			_needSave = false;

			performNextAction();
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			_nextAction = MenuType::None;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void tool::MenuBar::renderSystemManager()
{
	static bool managerOpen = false;

	if (_showSystemManager)
	{
		ImGui::OpenPopup("System Manager");
		managerOpen = true;
		_showSystemManager = false;
	}

	if (ImGui::BeginPopupModal("System Manager", &managerOpen,
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
	{
		auto& scene = *ToolProcess::scene;
		auto& systemMap = scene.GetSystemMap();
		std::set<std::string> drawnSystem;

		ImGui::Text("Registered Systems");
		ImGui::Separator();

		// 슬라이드 바와 스크롤 영역을 사용하여 시스템 목록 표시
		ImGui::BeginChild("SystemList", ImVec2(400, 300), true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);
		int index = 0;
		for (const auto& [name, info] : systemMap)
		{
			if (drawnSystem.contains(name))
				continue;

			drawnSystem.emplace(name);

			ImGui::PushID(index);
			if (ImGui::Button(" - "))
			{
				auto type = info.systemId;
				if (auto removeFunc = entt::resolve(global::systemMetaCtx, type).func("RemoveSystem"_hs))
				{
					_dispatcher->enqueue<OnToolRemoveSystem>(removeFunc);
				}
			}
			ImGui::PopID();

			ImGui::SameLine();
			ImGui::TextWrapped("%s", name.c_str());

			++index;
		}
		ImGui::EndChild();

		if (ImGui::Button(" + "))
		{
			ImGui::OpenPopup("Add System");
		}

		if (ImGui::BeginPopup("Add System"))
		{
			for (auto&& [id, type] : entt::resolve(global::systemMetaCtx))
			{
				auto name = type.prop("name"_hs).value().cast<const char*>();
				if (ImGui::MenuItem(name))
				{
					auto registerFunc = type.func("RegisterSystem"_hs);
					if (registerFunc)
					{
						registerFunc.invoke({}, ToolProcess::scene);
					}
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}

		ImGui::EndPopup();
	}
}

void tool::MenuBar::renderConfiguration()
{
	static bool configOpen = false;

	if (_showConfig)
	{
		ImGui::OpenPopup("Configuration Settings");
		configOpen = true;
		_showConfig = false;
	}

	if (ImGui::BeginPopupModal("Configuration Settings", &configOpen,
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
	{
		auto& config = ToolProcess::scene->GetRegistry()->ctx().get<core::Configuration>();

		ImGui::DragScalar("Window X", ImGuiDataType_U32, &config.width);
		ImGui::DragScalar("Window Y", ImGuiDataType_U32, &config.height);

		ImGui::EndPopup();
	}
}

void tool::MenuBar::processShortcut()
{

}

void tool::MenuBar::saveScene()
{
	if (ToolProcess::scene->GetName() == core::Scene::DEFAULT_SCENE_NAME)
	{
		_currentMenu = MenuType::SaveAs;
		_fileDialog.Open();
	}
	else
	{
		ToolProcess::scene->SaveScene(_currentScenePath, false);
	}

	LOG_INFO(*ToolProcess::scene, "Scene Saved : {}", _currentScenePath.filename().string());
}

void tool::MenuBar::performNextAction()
{
	if (_nextAction == MenuType::OpenScene)
	{
		_fileDialog.Open();
	}
	_nextAction = MenuType::None;
}

void tool::MenuBar::loadScene(const OnToolLoadScene& event)
{
	_currentScenePath = event.path;
}

void tool::MenuBar::removeSystem(const OnToolRemoveSystem& event)
{
	event.func.invoke({}, ToolProcess::scene);
}
