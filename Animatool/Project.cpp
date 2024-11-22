#include "pch.h"
#include "Project.h"

#include "ToolProcess.h"

#include <Animacore/Scene.h>
#include <Animavision/Material.h>
#include <Animacore/PxResources.h>
#include <Animacore/AnimatorController.h>

#include <Animacore/AnimatorSystem.h>


namespace fs = std::filesystem;

tool::Project::Project(entt::dispatcher& dispatcher, Renderer* renderer)
	: Panel(dispatcher)
	, _renderer(renderer)
{
	_resourcesPath = fs::current_path() / "Resources";
	_currentPath = _resourcesPath;

	_allowedExtensions = { ".material", ".fbx", ".prefab", ".png", ".jpeg", ".jpg", ".dds", ".scene", ".hdr", ".controller", ".pmaterial" };

	matchIcon();
}

void tool::Project::RenderPanel(float deltaTime)
{
	if (ImGui::Begin("Project", nullptr, ImGuiWindowFlags_NoCollapse))
	{
		ImGui::SetNextItemWidth(200);  // 검색창 크기 조절
		if (ImGui::InputText("Search", &_searchQuery))
		{
			searchFiles(_searchQuery);  // 검색어가 입력될 때마다 검색 수행
		}

		ImGui::SameLine(ImGui::GetContentRegionAvail().x - 110);  // 슬라이더를 오른쪽으로 치우침
		ImGui::SetNextItemWidth(100);  // 슬라이더 크기 조절
		if (ImGui::SliderFloat("##IconSize", &_iconSize.x, 16.f, 128.f, "%.f"))
		{
			_iconSize.y = _iconSize.x;
		}

		ImGui::Columns(2);

		if (ImGui::BeginChild("DirectoryTree", ImVec2(0, 0), true))
		{
			renderDirectoryTree(_resourcesPath);
		}
		ImGui::EndChild();

		ImGui::NextColumn();

		if (ImGui::BeginChild("DirectoryContents", ImVec2(0, 0), true))
		{
			renderPathBreadcrumbs();

			ImGui::Separator();

			if (!_searchQuery.empty())
			{
				// 검색 결과 표시
				for (const auto& file : _searchResults)
				{
					renderFile(file);
				}
			}
			else
			{
				// 디렉토리 콘텐츠 표시
				renderDirectoryContents(_currentPath);
			}

			processPopup();
		}
		ImGui::EndChild();

		ImGui::Columns(1);
	}

	ImGui::End();
}


void tool::Project::renderPathBreadcrumbs()
{
	std::vector<std::filesystem::path> pathParts;
	for (auto part = _currentPath.begin(); part != _currentPath.end(); ++part)
	{
		pathParts.push_back(*part);
	}

	std::filesystem::path currentPath;
	for (size_t i = 0; i < pathParts.size(); ++i)
	{
		currentPath /= pathParts[i];
		if (ImGui::Button(reinterpret_cast<const char*>(pathParts[i].u8string().c_str())))
		{
			_currentPath = currentPath;
			_showFavorites = false;
			_searchQuery.clear();
			_searchResults.clear();
		}

		if (i < pathParts.size() - 1)
		{
			ImGui::SameLine();
			ImGui::Text(">");
			ImGui::SameLine();
		}
	}
}

void tool::Project::renderDirectoryTree(const std::filesystem::path& rootPath)
{
	for (const auto& entry : fs::directory_iterator(rootPath))
	{
		if (entry.is_directory())
		{
			ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
			bool isOpen = ImGui::TreeNodeEx(entry.path().filename().string().c_str(), nodeFlags);

			if (ImGui::IsItemClicked())
			{
				_currentPath = entry.path();
				_showFavorites = false;
				_searchQuery.clear();
				_searchResults.clear();
			}

			if (isOpen)
			{
				renderDirectoryTree(entry.path());
				ImGui::TreePop();
			}
		}
	}
}

void tool::Project::renderDirectoryContents(const std::filesystem::path& path)
{
	float panelWidth = ImGui::GetContentRegionAvail().x;
	float buttonWidth = _iconSize.x;
	int itemsPerRow = static_cast<int>(panelWidth / (buttonWidth + 16.f));
	if (itemsPerRow == 0) itemsPerRow = 1;
	int itemCount = 0;

	for (const auto& entry : fs::directory_iterator(path))
	{
		auto extension = entry.path().extension().string();
		if (entry.is_directory()
			|| std::ranges::find(_allowedExtensions, extension) != _allowedExtensions.end())
		{
			renderFile(entry.path());
			itemCount++;

			if (itemCount > 0 && itemCount % itemsPerRow == 0)
				ImGui::NewLine();
		}
	}
}

void tool::Project::renderFile(const std::filesystem::path& path)
{
	const std::string fileName = path.filename().string();
	const bool isDirectory = fs::is_directory(path);
	const bool isSelected = _selectedFile == path;
	auto extension = path.extension().string();
	auto icon = getIcon(extension);

	// 확장자가 대문자인 경우 소문자로 변경
	std::ranges::transform(extension, extension.begin(), ::tolower);

	ImGui::BeginGroup();

	// 선택 파일 강조 표시
	if (isSelected)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.9f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.7f, 1.0f));
	}

	ImGui::PushID(fileName.c_str());

	if (getIcon(extension) == getIcon(".png"))
	{
		auto relative = std::filesystem::relative(path);
		std::string pathName = relative.string();
		std::ranges::replace(pathName, '\\', '/');
		relative = "./" + pathName;
		auto texture = _renderer->CreateTexture(relative.string().c_str());
		if (texture->m_Type == Texture::Type::TextureCube && texture->m_Type == Texture::Type::Texture2DArray)
		{
			icon = getIcon(extension);
		}
		else
		{
			icon = texture->GetShaderResourceView();
		}
	}

	if (ImGui::ImageButton((void*)icon, _iconSize))
	{
		_dispatcher->enqueue<OnToolSelectFile>(path.string()); // 파일 선택 이벤트
		_selectedFile = path;
	}

	if (isSelected)
		ImGui::PopStyleColor(3);

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%s", path.filename().string().c_str());

		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			// 더블 클릭 시 파일 실행(열기)
			if (isDirectory)
			{
				_currentPath = path;
				_showFavorites = false;
				_searchQuery.clear();
				_searchResults.clear();
			}
			else if (path.extension() == core::Scene::SCENE_EXTENSION)
			{
				_dispatcher->enqueue<OnToolLoadScene>(path.string());
			}
			else if (path.extension() == core::Scene::PREFAB_EXTENSION)
			{
				_dispatcher->enqueue<OnToolLoadPrefab>(path.string());
			}
			else if (path.extension() == core::Scene::CONTROLLER_EXTENSION)
			{
				_dispatcher->enqueue<OnToolRequestAddPanel>(PanelType::Animator, path.string());
			}
			else if (path.extension() == core::Scene::FBX_EXTENSION)
			{
				_dispatcher->enqueue<OnToolLoadFbx>(path.string());
			}
		}
	}

	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
	{
		std::string filePath = path.string();
		ImGui::SetDragDropPayload("FILE_PAYLOAD", filePath.c_str(), filePath.size() + 1);
		ImGui::Image((void*)icon, _iconSize);  // 잔상을 이미지로 설정
		ImGui::EndDragDropSource();
	}


	// 파일 이름을 이미지 버튼 아래에 표시
	ImVec2 textSize = ImGui::CalcTextSize(fileName.c_str());
	if (textSize.x > _iconSize.x)
	{
		std::string shortenedName = fileName.substr(0, (size_t)(_iconSize.x / textSize.x * fileName.length()) - 1) + "...";
		ImGui::TextWrapped("%s", shortenedName.c_str());
	}
	else
	{
		ImGui::TextWrapped("%s", fileName.c_str());
	}

	ImGui::EndGroup();
	ImGui::SameLine();
	ImGui::PopID();
}

void tool::Project::handleFileDrop()
{
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PAYLOAD"))
		{
			std::string filePath = (const char*)payload->Data;
			// 드래그 앤 드롭 동작 처리
		}
		ImGui::EndDragDropTarget();
	}
}

void tool::Project::searchFiles(const std::string& query)
{
	_searchResults.clear();  // 이전 검색 결과를 초기화

	if (query.empty())
	{
		return;  // 검색어가 비어 있을 경우 검색하지 않음
	}

	std::string lowerQuery = query;
	std::ranges::transform(lowerQuery, lowerQuery.begin(), ::tolower);  // 쿼리를 소문자로 변환

	for (const auto& entry : fs::recursive_directory_iterator(_resourcesPath))
	{
		// 파일명 및 경로의 확장자 확인
		const std::string fileName = entry.path().filename().string();
		const std::string extension = entry.path().extension().string();

		// 확장자가 허용된 확장자 목록에 있는지 확인
		if (!entry.is_directory() &&
			(std::ranges::find(_allowedExtensions, extension) != _allowedExtensions.end()))
		{
			std::string lowerFileName = fileName;
			std::ranges::transform(lowerFileName, lowerFileName.begin(), ::tolower);  // 파일명을 소문자로 변환

			// 검색 쿼리가 파일명에 포함되는지 확인
			if (lowerFileName.find(lowerQuery) != std::string::npos)
			{
				_searchResults.push_back(entry.path());
			}
		}
	}
}

void tool::Project::processPopup()
{
	// 마우스 우클릭
	if (ImGui::BeginPopupContextWindow("ContextPopup"))
	{
		if (ImGui::MenuItem("New Material"))
		{
			_popupTypes = NewMaterial;
		}
		if (ImGui::MenuItem("New Animator"))
		{
			_popupTypes = NewAnimator;
		}
		if (ImGui::MenuItem("New Physic Material"))
		{
			_popupTypes = NewPhysicMaterial;
		}

		ImGui::EndPopup();
	}

	// 팝업 호출
	switch (_popupTypes)
	{
	case NewMaterial:
		ImGui::OpenPopup("Create Material Popup");
		_popupTypes = None;
		break;
	case NewAnimator:
		ImGui::OpenPopup("Create Animator Popup");
		_popupTypes = None;
		break;
	case NewPhysicMaterial:
		ImGui::OpenPopup("Create Physic Material Popup");
		_popupTypes = None;
		break;
	default:
		_popupTypes = None;
		break;
	}

	// Create Material Popup 처리
	if (ImGui::BeginPopupModal("Create Material Popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static char materialName[128] = "";
		ImGui::InputText("Material Name", materialName, IM_ARRAYSIZE(materialName));

		if (ImGui::Button("Create"))
		{
			createMaterial(materialName);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	// Create Animator Popup 처리
	if (ImGui::BeginPopupModal("Create Animator Popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static char animatorName[128] = "";
		ImGui::InputText("Animator Name", animatorName, IM_ARRAYSIZE(animatorName));

		if (ImGui::Button("Create"))
		{
			createAnimator(animatorName);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	// Create Animator Popup 처리
	if (ImGui::BeginPopupModal("Create Physic Material Popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static char materialName[128] = "";
		ImGui::InputText("Material Name", materialName, IM_ARRAYSIZE(materialName));

		if (ImGui::Button("Create"))
		{
			createPhysicMaterial(materialName);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void tool::Project::createMaterial(const std::string& name)
{
	const auto material = Material::Create(_renderer->GetShaders()->begin()->second);
	const fs::path path = _currentPath / (name + core::Scene::MATERIAL_EXTENSION);

	std::string pathName = fs::relative(path).string();
	std::ranges::replace(pathName, '\\', '/');
	fs::path finalPath = "./" + pathName;

	material->SetName(finalPath.string());
	_renderer->AddMaterial(material);
	_renderer->SaveMaterial(finalPath.string());
}


void tool::Project::createAnimator(const std::string& name)
{
	auto animator = core::AnimatorController();
	const fs::path path = _currentPath / (name + core::Scene::CONTROLLER_EXTENSION);

	animator.animatorName = name;
	animator.entryStateName = "Entry";
	animator.AddState("Entry");

	//_renderer->AddAnimator(animator);
	core::AnimatorController::Save(path.string(), animator);

	// 절대경로를 상대경로로 바꾼다.
	std::string pathName = fs::relative(path).string();
	std::ranges::replace(pathName, '\\', '/');
	fs::path finalPath = "./" + pathName;

	core::AnimatorSystem::controllerManager.LoadController(finalPath.string(), _renderer);

}

void tool::Project::createPhysicMaterial(const std::string& name)
{
	const fs::path path = _currentPath / (name + core::Scene::PHYSIC_MATERIAL_EXTENSION);

	// 임시 객체 생성
	core::PxResources::SaveMaterial(path, {});
}

void tool::Project::matchIcon()
{
	_extensionMatcher[""] = "directory";
	_extensionMatcher[".material"] = "material";
	_extensionMatcher[".fbx"] = "model";
	_extensionMatcher[".prefab"] = "prefab";
	_extensionMatcher[".cpp"] = "script";
	_extensionMatcher[".png"] = "texture";
	_extensionMatcher[".jpeg"] = "texture";
	_extensionMatcher[".jpg"] = "texture";
	_extensionMatcher[".dds"] = "texture";
	_extensionMatcher[".scene"] = "scene";
	_extensionMatcher[".controller"] = "controller";
	_extensionMatcher[".pmaterial"] = "pmaterial";
}

ImTextureID tool::Project::getIcon(const std::string& extension)
{
	auto it = _extensionMatcher.find(extension);
	if (it != _extensionMatcher.end())
	{
		return ToolProcess::icons[it->second];
	}
	return ToolProcess::icons["default"];
}
