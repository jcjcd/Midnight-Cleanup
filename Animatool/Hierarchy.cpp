#include "pch.h"
#include "Hierarchy.h"

#include "ToolProcess.h"

#include <imgui_internal.h>
#include <Animacore/Scene.h>
#include <Animacore/CoreComponents.h>

tool::Hierarchy::Hierarchy(entt::dispatcher& dispatcher)
	: Panel(dispatcher)
{
	// 파일 브라우저 초기화
	_fileDialog = ImGui::FileBrowser(
		ImGuiFileBrowserFlags_EnterNewFilename |
		ImGuiFileBrowserFlags_CloseOnEsc |
		ImGuiFileBrowserFlags_CreateNewDir |
		ImGuiFileBrowserFlags_ConfirmOnEnter |
		ImGuiFileBrowserFlags_HideRegularFiles
	);

	_fileDialog.SetPwd("./Resources/Prefabs/");
	_fileDialog.SetTypeFilters({ ".prefab" });

	_dispatcher->sink<OnToolSelectEntity>().connect<&Hierarchy::selectEntity>(this);
	_dispatcher->sink<OnToolPostLoadScene>().connect<&Hierarchy::postLoadScene>(this);
}

void tool::Hierarchy::RenderPanel(float deltaTime)
{
	if (ImGui::Begin("Hierarchy"))
	{
		renderSearchAndContext(); // 검색과 컨텍스트 메뉴

		auto& registry = *ToolProcess::scene->GetRegistry();
		auto view = registry.view<entt::entity>();

		// 선택된 노드 색깔 설정
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4{ 0.33f, 0.33f, 0.37f, 1.0f });

		for (auto& entity : view)
		{
			auto* relationship = registry.try_get<core::Relationship>(entity);

			if (!relationship || relationship->parent == entt::null)
			{
				renderEntityNode(entity);
			}
		}

		ImGui::PopStyleColor();

		// Ctrl + C, Ctrl + V, Shift, Delete 기능 구현
		if (ImGui::IsWindowFocused())
		{
			if (ImGui::IsKeyPressed(ImGuiKey_C) && ImGui::GetIO().KeyCtrl
				&& !_selectedEntities.empty())
			{
				_entityToCopy = _selectedEntities.back();
			}
			if (ImGui::IsKeyPressed(ImGuiKey_V) && ImGui::GetIO().KeyCtrl
				&& _entityToCopy != entt::null)
			{
				auto newEntity = registry.create();
				_copyMapping.clear();
				copyEntity(registry, _entityToCopy, newEntity);
			}
			if (ImGui::IsKeyPressed(ImGuiKey_Delete))
			{
				throwDestroyEntities();
				_selectedEntities.clear();
			}
			if (ImGui::IsKeyPressed(ImGuiKey_T) && ImGui::GetIO().KeyCtrl
				&& !_selectedEntities.empty() && _selectedEntities.back() != entt::null)
			{
				// Move to Child 기능 구현
				if (_lastSelected != entt::null && _lastSelected != _selectedEntities.back())
				{
					auto& selectedEntity = _selectedEntities.back();
					auto coreEntity = core::Entity{ selectedEntity, registry };
					coreEntity.SetParent(_lastSelected);
					if (coreEntity.HasAllOf<core::WorldTransform>())
						registry.patch<core::WorldTransform>(coreEntity);
					throwSelectEntity();
				}
			}
			if (ImGui::IsKeyPressed(ImGuiKey_A) && ImGui::GetIO().KeyCtrl
				&& _entityToCopy != entt::null && !_selectedEntities.empty())
			{
				// Paste as Child 기능 구현
				auto newEntity = registry.create();
				_copyMapping.clear();
				copyEntity(registry, _entityToCopy, newEntity);
				auto& selectedEntity = _selectedEntities.back();
				auto coreEntity = core::Entity{ newEntity, registry };
				coreEntity.SetParent(selectedEntity);
				if (coreEntity.HasAllOf<core::WorldTransform>())
					registry.patch<core::WorldTransform>(coreEntity);
			}
		}

		// 창 전체에 드래그 드롭 기능 추가 (빈 공간에 드롭 시 최상위로 설정)
		if (beginDragDropTargetWindow("ENTITY_PAYLOAD"))
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_PAYLOAD"))
			{
				const entt::entity* draggedEntities = static_cast<const entt::entity*>(payload->Data);
				size_t entityCount = payload->DataSize / sizeof(entt::entity);

				for (size_t i = 0; i < entityCount; ++i)
				{
					auto entity = draggedEntities[i];

					// 부모 제거 후, WorldTransform을 LocalTransform으로 반영
					auto coreEntity = core::Entity{ entity, registry };
					if (coreEntity.HasAnyOf<core::Relationship>())
					{
						// 부모 관계를 제거
						coreEntity.SetParent(entt::null);

						// WorldTransform을 가져옴
						auto world = coreEntity.TryGet<core::WorldTransform>();
						auto local = coreEntity.TryGet<core::LocalTransform>();

						if (!world or !local)
							break;

						// WorldTransform 값을 LocalTransform에 반영
						local->position = world->position;
						local->rotation = world->rotation;
						local->scale = world->scale;

						// 변경 사항을 patch
						registry.patch<core::LocalTransform>(entity);
					}
				}
			}
			ImGui::EndDragDropTarget();
		}
	}
	ImGui::End();
}


void tool::Hierarchy::renderSearchAndContext()
{
	// 검색창과 콤보박스
	float comboWidth = ImGui::CalcTextSize("Name").x + 30;
	float searchWidth = ImGui::GetContentRegionAvail().x - comboWidth - ImGui::GetStyle().ItemSpacing.x;

	ImGui::PushItemWidth(searchWidth);
	ImGui::InputText("##Search", _searchBuffer, sizeof(_searchBuffer));
	ImGui::PopItemWidth();

	ImGui::SameLine();
	ImGui::PushItemWidth(comboWidth);
	const char* items[] = { "Name", "ID" };
	ImGui::Combo("##SearchBy", &_searchBy, items, IM_ARRAYSIZE(items));
	ImGui::PopItemWidth();

	// 우클릭 컨텍스트 메뉴
	if (ImGui::BeginPopupContextWindow("WindowPopup"))
	{
		if (ImGui::MenuItem("Create Entity")) {
			auto newEntity = ToolProcess::scene->CreateEntity();
			_selectedEntities.clear();
			_selectedEntities.push_back(newEntity);
		}
		if (ImGui::MenuItem("Paste Entity", "Ctrl + V", false, _entityToCopy != entt::null))
		{
			auto newEntity = ToolProcess::scene->GetRegistry()->create();
			_copyMapping.clear();
			copyEntity(*ToolProcess::scene->GetRegistry(), _entityToCopy, newEntity);
		}
		ImGui::EndPopup();
	}
}

void tool::Hierarchy::renderNodeContext(entt::entity entity, bool& nodeOpened, bool& dialogOpened)
{
	// 자식 엔티티 생성
	if (ImGui::MenuItem("Create Child Entity"))
	{
		auto newEntity = ToolProcess::scene->CreateEntity();
		newEntity.SetParent(entity);

		_selectedEntities.clear();
		_selectedEntities.push_back(entity);
		throwSelectEntity();

		BroadcastModifyScene();

		nodeOpened = true;
	}
	// 엔티티 삭제
	if (ImGui::MenuItem("Delete Entity", "Delete"))
	{
		throwDestroyEntities();
		BroadcastModifyScene();
	}
	// 엔티티 복사
	if (ImGui::MenuItem("Copy Entity", "Ctrl + C"))
	{
		_entityToCopy = entity;
		ImGui::CloseCurrentPopup();
	}
	// 프리팹 저장
	if (ImGui::MenuItem("Save as Prefab"))
	{
		dialogOpened = true;
	}
	// 이름 변경
	if (ImGui::MenuItem("Rename", "F2"))
	{
		_renamingEntity = entity;
	}
	// 자식으로 옮기기
	if (ImGui::MenuItem("Move to Child", "Ctrl + T", false, _lastSelected != entt::null && _lastSelected != entity))
	{
		auto coreEntity = core::Entity{ entity, *ToolProcess::scene->GetRegistry() };
		coreEntity.SetParent(_lastSelected);
		if (coreEntity.HasAllOf<core::WorldTransform>())
			ToolProcess::scene->GetRegistry()->patch<core::WorldTransform>(coreEntity);
		throwSelectEntity();
	}
	// 자식으로 복사하여 생성하기
	if (ImGui::MenuItem("Paste as Child", "Ctrl + A", false, _entityToCopy != entt::null))
	{
		auto newEntity = ToolProcess::scene->GetRegistry()->create();
		_copyMapping.clear();
		copyEntity(*ToolProcess::scene->GetRegistry(), _entityToCopy, newEntity);
		auto coreEntity = core::Entity{ newEntity, *ToolProcess::scene->GetRegistry() };
		coreEntity.SetParent(entity);
		if (coreEntity.HasAllOf<core::WorldTransform>())
			ToolProcess::scene->GetRegistry()->patch<core::WorldTransform>(coreEntity);
	}
}

bool tool::Hierarchy::searchMatches(entt::entity entity)
{
	auto& registry = *ToolProcess::scene->GetRegistry();
	if (strlen(_searchBuffer) == 0) return true;

	if (_searchBy == 0) {
		if (auto* nameComp = registry.try_get<core::Name>(entity)) {
			return nameComp->name.find(_searchBuffer) != std::string::npos;
		}
	}
	else if (_searchBy == 1) {
		std::string idStr = std::to_string(static_cast<uint32_t>(entity));
		return idStr.find(_searchBuffer) != std::string::npos;
	}

	return false;
}

bool tool::Hierarchy::searchRecursively(entt::entity entity)
{
	// 현재 엔티티가 검색 조건에 맞는지 확인
	if (searchMatches(entity))
		return true;

	auto& registry = *ToolProcess::scene->GetRegistry();

	// 자식들 중에서 하나라도 검색 조건에 맞으면 true
	if (auto* relationship = registry.try_get<core::Relationship>(entity))
	{
		for (auto child : relationship->children)
		{
			if (searchRecursively(child))
				return true;
		}
	}

	return false;
}

void tool::Hierarchy::renderEntityNode(entt::entity entity)
{
	if (!searchRecursively(entity))
		return;

	auto& registry = *ToolProcess::scene->GetRegistry();
	auto coreEntity = core::Entity{ entity, registry };
	bool hasChildren = (coreEntity.GetChildren().size() != 0);

	ImGui::PushID(static_cast<uint32_t>(entity));

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (std::ranges::find(_selectedEntities, entity) != _selectedEntities.end()) {
		flags |= ImGuiTreeNodeFlags_Selected;

		if (!_isFocused)
		{
			ImGui::SetScrollHereY();
			_isFocused = true;
		}
	}
	if (!hasChildren) {
		flags |= ImGuiTreeNodeFlags_Leaf;
	}

	// SetNextItemOpen 사용하여 선택된 엔티티와 부모가 열리도록 설정
	if (std::ranges::find(_expandedEntities, entity) != _expandedEntities.end())
	{
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	}

	// Name 컴포넌트가 있으면 이름으로 노드를 생성하고, 없으면 Entity %d로 노드 생성
	bool nodeOpened = false;
	auto* nameComp = registry.try_get<core::Name>(entity);
	if (nameComp) {
		nodeOpened = ImGui::TreeNodeEx(&entity, flags, "%s", nameComp->name.c_str());
	}
	else {
		nodeOpened = ImGui::TreeNodeEx(&entity, flags, "Entity %d", entity);
	}

	bool dragStarted = false;  // 드래그 상태를 확인하는 변수
	bool itemClicked = false;  // 클릭 상태를 확인하는 변수
	bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);  // 마우스가 눌려있는 상태 확인
	bool mouseReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);  // 마우스 버튼이 풀린 상태 확인

	// 드래그 시작
	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoDisableHover))
	{
		dragStarted = true;  // 드래그가 시작되었음을 표시

		// 선택된 여러 엔티티를 페이로드로 설정
		std::vector<entt::entity> dragEntities;

		if (std::ranges::find(_previousSelectedEntities, entity) == _previousSelectedEntities.end())
		{
			_selectedEntities.clear();
			_previousSelectedEntities.clear();
			_selectedEntities.push_back(entity);
		}
		else
		{
			_selectedEntities = _previousSelectedEntities;
		}
		dragEntities = _selectedEntities;

		ImGui::SetDragDropPayload("ENTITY_PAYLOAD",
			dragEntities.data(), sizeof(entt::entity) * dragEntities.size(), ImGuiCond_Once);

		ImGui::Text("Dragging %d entities", static_cast<int>(dragEntities.size()));
		ImGui::EndDragDropSource();
	}

	// 엔티티 위에 드래그 드롭 시 부모-자식 관계 설정
	if (!dragStarted && ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_PAYLOAD"))
		{
			const entt::entity* draggedEntities = static_cast<const entt::entity*>(payload->Data);
			size_t entityCount = payload->DataSize / sizeof(entt::entity);

			for (size_t i = 0; i < entityCount; ++i)
			{
				auto draggedCoreEntity = core::Entity{ draggedEntities[i], registry };

				// 부모 자식 관계 설정
				if (!draggedCoreEntity.IsAncestorOf(entity)) // 부모-자식 관계 체크
				{
					draggedCoreEntity.SetParent(entity); // 자식으로 설정
					if (draggedCoreEntity.HasAllOf<core::WorldTransform>())
						registry.patch<core::WorldTransform>(draggedCoreEntity);
				}
			}
		}
		ImGui::EndDragDropTarget();
	}

	// 마우스 감지
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("ID: %d", entity);

		// 클릭이 발생했는지 여부를 확인 (마우스가 눌려있지 않을 때만)
		if (!mouseDown && mouseReleased)
		{
			itemClicked = true;
		}

		// 더블 클릭을 처리
		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			_dispatcher->enqueue<OnToolDoubleClickEntity>(entity, ToolProcess::scene);
		}
	}

	// 클릭 이벤트 처리
	if (itemClicked && !dragStarted)  // 드래그가 시작되지 않았을 때만 클릭 처리
	{
		// 클릭 시점에 이전 선택 상태를 저장
		_previousSelectedEntities = _selectedEntities;

		// Shift 키가 눌렸을 때는 시작점(_lastSelected)을 변경하지 않음
		if (ImGui::GetIO().KeyShift && _lastSelected != entt::null)
		{
			selectEntitiesInRange(_lastSelected, entity);
		}
		else
		{
			// Shift가 눌리지 않은 경우에만 _lastSelected를 업데이트
			if (ImGui::GetIO().KeyCtrl)
			{
				if (std::ranges::find(_selectedEntities, entity) != _selectedEntities.end())
					std::erase(_selectedEntities, entity);
				else
					_selectedEntities.push_back(entity);
			}
			else
			{
				_selectedEntities.clear();
				_selectedEntities.push_back(entity);
			}

			_lastSelected = entity; // 여기서 _lastSelected를 업데이트
		}

		throwSelectEntity();
	}

	bool dialogOpened = false;
	if (ImGui::BeginPopupContextItem("ContextPopup"))
	{
		renderNodeContext(entity, nodeOpened, dialogOpened);
		ImGui::EndPopup();
	}

	// 프리팹 저장
	if (dialogOpened)
	{
		ImGui::OpenPopup("Prefab Path Input");
		_fileDialog.Open();
	}
	if (ImGui::BeginPopup("Prefab Path Input"))
	{
		_fileDialog.Display();
		ImGui::EndPopup();
	}
	if (_fileDialog.HasSelected())
	{
		ToolProcess::scene->SavePrefab(_fileDialog.GetSelected().string(), coreEntity);
		_fileDialog.ClearSelected();
	}

	// 엔티티 이름 변경 모드
	if (_renamingEntity == entity)
	{
		char nameBuffer[256] = {};

		if (nameComp)
			strncpy_s(nameBuffer, nameComp->name.c_str(), sizeof(nameBuffer));

		// 입력 필드로 이름 변경
		ImGui::SetKeyboardFocusHere();
		if (ImGui::InputText("##Rename", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
		{
			if (nameComp) {
				nameComp->name = nameBuffer;
			}
			_renamingEntity = entt::null;
		}

		if (ImGui::IsItemDeactivated() || ImGui::IsKeyPressed(ImGuiKey_Escape))
			_renamingEntity = entt::null; // Esc로 이름 변경 취소
	}

	if (nodeOpened)
	{
		auto children = coreEntity.GetChildren();
		for (auto& child : children)
		{
			renderEntityNode(child);
		}
		ImGui::TreePop();
	}

	// F2로 이름 변경 모드로 진입
	if (!_selectedEntities.empty())
		if (_selectedEntities.back() == entity && ImGui::IsKeyPressed(ImGuiKey_F2))
			_renamingEntity = entity;

	ImGui::PopID();
}



void tool::Hierarchy::selectEntitiesInRange(entt::entity start, entt::entity end)
{
	auto& registry = *ToolProcess::scene->GetRegistry();
	bool selecting = false;
	_selectedEntities.clear();

	auto view = registry.view<entt::entity>();
	bool startFound = false;
	bool endFound = false;

	for (auto entity : view) 
	{
		if (!searchRecursively(entity))
			continue;

		if (entity == start || entity == end) {
			selecting = !selecting;
			_selectedEntities.push_back(entity);

			// Mark start or end as found
			if (entity == start) {
				startFound = true;
			}
			else if (entity == end) {
				endFound = true;
			}
		}

		if (selecting)
			_selectedEntities.push_back(entity);

		if (startFound && endFound)
			break;
	}
}

void tool::Hierarchy::copyEntity(entt::registry& registry, entt::entity src, entt::entity dst)
{
	if (!registry.valid(src))
		return;

	_copyMapping[src] = dst;

	// 기본 컴포넌트 복사
	for (auto&& [id, storage] : registry.storage())
	{
		if (id == entt::type_id<core::Relationship>().hash())
			continue;

		if (storage.contains(src))
			storage.push(dst, storage.value(src));
	}

	// 자식 엔티티 복사
	if (auto* srcRs = registry.try_get<core::Relationship>(src))
	{
		auto& dstRs = registry.emplace<core::Relationship>(dst);
		dstRs.parent = entt::null;
		dstRs.children.clear();

		// 새로운 엔티티가 부모와 연결되도록 설정
		if (srcRs->parent != entt::null)
		{
			if (const auto it = _copyMapping.find(srcRs->parent); it != _copyMapping.end())
				dstRs.parent = it->second;
			else
				dstRs.parent = srcRs->parent;

			auto& parentRs = registry.get<core::Relationship>(dstRs.parent);
			parentRs.children.push_back(dst);
		}

		for (auto& srcChild : srcRs->children)
		{
			// 하위 객체 생성 및 복사
			auto dstChild = registry.create();
			copyEntity(registry, srcChild, dstChild);
		}
	}
}

bool tool::Hierarchy::beginDragDropTargetWindow(const char* payloadType)
{
	using namespace ImGui;
	ImRect inner_rect = GetCurrentWindow()->InnerRect; // 현재 창의 내부 영역 계산

	if (BeginDragDropTargetCustom(inner_rect, GetID("##WindowBgArea")))
	{
		if (const ImGuiPayload* payload = AcceptDragDropPayload(payloadType, ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
		{
			if (payload->IsPreview())
			{
				// 드래그가 완료되기 전에 하이라이팅
				ImDrawList* draw_list = GetForegroundDrawList();
				draw_list->AddRectFilled(inner_rect.Min, inner_rect.Max, GetColorU32(ImGuiCol_DragDropTarget, 0.05f));
				draw_list->AddRect(inner_rect.Min, inner_rect.Max, GetColorU32(ImGuiCol_DragDropTarget), 0.0f, 0, 2.0f);
			}
			if (payload->IsDelivery())
			{
				// 드롭 처리
				return true;
			}
		}
		EndDragDropTarget();
	}
	return false;
}

void tool::Hierarchy::selectEntity(const OnToolSelectEntity& event)
{
	_selectedEntities = event.entities;

	if (!_selectedEntities.empty())
	{
		auto& registry = *ToolProcess::scene->GetRegistry();

		// 선택된 엔티티와 부모 노드를 추적해서 열림 상태로 설정
		_expandedEntities.clear();  // 열려야 하는 엔티티들 저장

		std::function<void(entt::entity)> expandToSelected = [&](entt::entity entity)
			{
				_expandedEntities.emplace(entity);  // 열려야 하는 엔티티 추가
				if (auto* relationship = registry.try_get<core::Relationship>(entity))
				{
					if (relationship->parent != entt::null)
					{
						expandToSelected(relationship->parent);  // 재귀적으로 부모 추적
					}
				}
			};

		// 선택된 엔티티부터 부모까지 모두 열림 상태로 설정
		for (auto& entity : _selectedEntities)
			expandToSelected(entity);

		_isFocused = false;
	}
}

void tool::Hierarchy::postLoadScene(const OnToolPostLoadScene& event)
{
	_selectedEntities.clear();
}

void tool::Hierarchy::throwSelectEntity() const
{
	_dispatcher->enqueue<OnToolSelectEntity>(_selectedEntities, ToolProcess::scene);
}

void tool::Hierarchy::throwDestroyEntities()
{
	for (auto& entity : _selectedEntities)
	{
		_dispatcher->enqueue<OnToolDestroyEntity>(entity);
	}

	throwSelectEntity();
}
