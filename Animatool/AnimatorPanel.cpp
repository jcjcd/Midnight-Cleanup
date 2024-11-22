#include "pch.h"
#include "AnimatorPanel.h"

#include "ToolProcess.h"
#include "ToolEvents.h"

#include <Animacore/Scene.h>
#include <Animacore/AnimatorState.h>
#include <Animacore/RenderComponents.h>
#include <Animacore/AnimatorTransition.h>

#include "imgui_internal.h"
#include "..\Animacore\AnimatorSystem.h"
#include "../Animavision/Mesh.h"


tool::AnimatorPanel::AnimatorPanel(entt::dispatcher& dispatcher, Renderer* renderer)
	: Panel(dispatcher), _renderer(renderer)
{
	_dispatcher->sink<OnToolSelectEntity>().connect<&AnimatorPanel::selectEntity>(this);
	_dispatcher->sink<OnToolAddedPanel>().connect<&AnimatorPanel::createContext>(this);
	_dispatcher->sink<OnToolRemovePanel>().connect<&AnimatorPanel::destroyContext>(this);
	_dispatcher->sink<OnToolSelectFile>().connect<&AnimatorPanel::changeAnimator>(this);

	// 엔티티에서 애니메이터를 클릭했을때..

	// 실행하면 여기도 받아야한다.
	_dispatcher->sink<OnToolPlayScene>().connect<&AnimatorPanel::playScene>(this);
	_dispatcher->sink<OnToolStopScene>().connect<&AnimatorPanel::stopScene>(this);
}


void tool::AnimatorPanel::RenderPanel(float deltaTime)
{
	auto& registry = *ToolProcess::scene->GetRegistry();

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoScrollbar;

	if (ImGui::Begin("Animator", &_isOpen, flags))
	{
		// 이 창의 크기를 받아옴
		ImVec2 size = ImGui::GetWindowSize();

		static float leftPaneWidth = 400.0f;
		float rightPaneWidth = size.x - leftPaneWidth - 4.0f; // Splitter 두께 반영

		if (rightPaneWidth < 50.0f) rightPaneWidth = 50.0f; // 최소 크기

		// 왼쪽 패널
		ImGui::BeginChild("LeftPane", ImVec2(leftPaneWidth, 0));
		showLeftPane(leftPaneWidth);
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::Button("LeftSplitter", ImVec2(4.f, -1));
		if (ImGui::IsItemActive())
		{
			leftPaneWidth += ImGui::GetIO().MouseDelta.x;
			rightPaneWidth -= ImGui::GetIO().MouseDelta.x;
		}

		ImGui::SameLine();

		// 중간 패널
		ImGui::BeginChild("MidPane", ImVec2(rightPaneWidth, 0));

		ax::NodeEditor::SetCurrentEditor(_context);
		ax::NodeEditor::Begin("Animator");
		{
			if (_selectControllerPath != "")
			{
				auto cursorTopLeft = ImGui::GetCursorScreenPos();

				for (auto& node : m_Nodes)
				{
					printNode(&node);
				}

				for (auto& link : m_Links)
				{
					ax::NodeEditor::Link(link.ID, link.StartPinID, link.EndPinID, link.Color, 2.0f);
				}

				showContextMenu();

				ImGui::SetCursorScreenPos(cursorTopLeft);

				if (_isPlaying)
				{
					_isDirty = false;
				}

				if (_isDirty)
				{
					core::AnimatorController::Save(_selectControllerPath.string(), *_animatorController);
					_isDirty = false;
				}
			}
		}
		ax::NodeEditor::End();

		ImGui::EndChild();

		refreshInspector();
	}
	ImGui::End();

	if (!_isOpen)
	{
		_dispatcher->enqueue<OnToolRemovePanel>(PanelType::Animator);
	}
}

void tool::AnimatorPanel::selectEntity(const OnToolSelectEntity& event)
{
	_selectedEntities.clear();
	_selectedEntities = event.entities;

	if (_selectedEntities.empty())
	{
		_selectControllerPath = "";
		_selectControllerMetaPath = "";
		_animatorController = nullptr;
	}
	else
	{
		auto& registry = *ToolProcess::scene->GetRegistry();
		auto entity = _selectedEntities.back();

		auto* animator = registry.try_get<core::Animator>(entity);

		if (animator)
		{
			_animator = animator;

			if (animator->animatorFileName != "")
			{
				_selectControllerPath = animator->animatorFileName;
				std::filesystem::path metaPath = _selectControllerPath;
				metaPath.replace_extension(".meta");

				_selectControllerMetaPath = metaPath.string();

				_animatorController = core::AnimatorSystem::controllerManager.GetController(_selectControllerPath.string());

				createContext({ PanelType::Animator, nullptr });

				generateNodes(*_animatorController);
			}
			else
			{
				_selectControllerPath = "";
				_selectControllerMetaPath = "";
				_animatorController = nullptr;
			}
		}
		else
		{
			_animator = nullptr;

			_selectControllerPath = "";
			_selectControllerMetaPath = "";
			_animatorController = nullptr;
		}
	}
}

void tool::AnimatorPanel::createContext(const tool::OnToolAddedPanel& event)
{
	if (event.panelType != PanelType::Animator)
		return;

	destroyContext({ PanelType::Animator });

	std::filesystem::path settingPath = _selectControllerPath;
	settingPath.replace_extension(".meta");

	_selectControllerMetaPath = settingPath.string();

	ax::NodeEditor::Config config;
	if (_selectControllerPath.empty())
	{
		config.SettingsFile = nullptr;
	}
	else
	{
		config.SettingsFile = _selectControllerMetaPath.c_str();
	}

	config.UserPointer = this;


	config.LoadNodeSettings = [](ax::NodeEditor::NodeId nodeId, char* data, void* userPointer) -> size_t
		{
			auto self = static_cast<AnimatorPanel*>(userPointer);

			auto node = self->FindNode(nodeId);
			if (!node)
				return 0;

			if (data != nullptr)
				memcpy(data, node->State.data(), node->State.size());
			return node->State.size();
		};

	config.SaveNodeSettings = [](ax::NodeEditor::NodeId nodeId, const char* data, size_t size, ax::NodeEditor::SaveReasonFlags reason, void* userPointer) -> bool
		{
			auto self = static_cast<AnimatorPanel*>(userPointer);

			auto node = self->FindNode(nodeId);
			if (!node)
				return false;

			node->State.assign(data, size);

			return true;
		};

	_context = ax::NodeEditor::CreateEditor(&config);
	ax::NodeEditor::SetCurrentEditor(_context);

	ax::NodeEditor::NavigateToContent();
}

void tool::AnimatorPanel::showContextMenu()
{
	static ax::NodeEditor::NodeId contextNodeId = 0;
	static ax::NodeEditor::LinkId contextLinkId = 0;
	static ax::NodeEditor::PinId  contextPinId = 0;
	static bool createNewNode = false;
	static Pin* newNodeLinkPin = nullptr;

# if 1
	auto openPopupPosition = ImGui::GetMousePos();
	ax::NodeEditor::Suspend();
	if (ax::NodeEditor::ShowNodeContextMenu(&contextNodeId))
		ImGui::OpenPopup("Node Context Menu");
	else if (ax::NodeEditor::ShowPinContextMenu(&contextPinId))
		ImGui::OpenPopup("Pin Context Menu");
	else if (ax::NodeEditor::ShowLinkContextMenu(&contextLinkId))
		ImGui::OpenPopup("Link Context Menu");
	else if (ax::NodeEditor::ShowBackgroundContextMenu())
	{
		ImGui::OpenPopup("Create New Node");
		newNodeLinkPin = nullptr;
	}
	ax::NodeEditor::Resume();

	ax::NodeEditor::Suspend();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	if (ImGui::BeginPopup("Node Context Menu"))
	{
		auto node = FindNode(contextNodeId);

		ImGui::TextUnformatted("Node Context Menu");
		ImGui::Separator();
		if (node)
		{
			ImGui::Text("ID: %p", node->ID.AsPointer());
			ImGui::Text("Type: %s", node->Type == NodeType::Blueprint ? "Blueprint" : (node->Type == NodeType::Tree ? "Tree" : "Comment"));
			ImGui::Text("Inputs: %d", (int)node->Inputs.size());
			ImGui::Text("Outputs: %d", (int)node->Outputs.size());
		}
		else
			ImGui::Text("Unknown node: %p", contextNodeId.AsPointer());
		ImGui::Separator();

		if (ImGui::MenuItem("Delete"))
		{
			ax::NodeEditor::DeleteNode(contextNodeId);
		}
		ImGui::EndPopup();
	}

	if (!createNewNode)
	{
		if (ax::NodeEditor::BeginCreate(ImColor(255, 255, 255), 2.0f))
		{
			auto showLabel = [](const char* label, ImColor color)
				{
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
					auto size = ImGui::CalcTextSize(label);

					auto padding = ImGui::GetStyle().FramePadding;
					auto spacing = ImGui::GetStyle().ItemSpacing;


					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x + spacing.x, ImGui::GetCursorPos().y - spacing.y));

					ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
					auto rectMin = ImVec2(cursorScreenPos.x - padding.x, cursorScreenPos.y - padding.y);
					auto rectMax = ImVec2(cursorScreenPos.x + size.x + padding.x, cursorScreenPos.y + size.y + padding.y);

					auto drawList = ImGui::GetWindowDrawList();
					drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
					ImGui::TextUnformatted(label);
				};

			ax::NodeEditor::PinId startPinId = 0, endPinId = 0;
			if (ax::NodeEditor::QueryNewLink(&startPinId, &endPinId))
			{
				auto startPin = FindPin(startPinId);
				auto endPin = FindPin(endPinId);

				_newLinkPin = startPin ? startPin : endPin;

				if (startPin->Kind == PinKind::Input)
				{
					std::swap(startPin, endPin);
					std::swap(startPinId, endPinId);
				}

				if (startPin && endPin)
				{
					if (endPin == startPin)
					{
						ax::NodeEditor::RejectNewItem(ImColor(255, 0, 0), 2.0f);
					}
					else if (endPin->Kind == startPin->Kind)
					{
						showLabel("x Incompatible Pin Kind", ImColor(45, 32, 32, 180));
						ax::NodeEditor::RejectNewItem(ImColor(255, 0, 0), 2.0f);
					}
					else if (endPin->Type != startPin->Type)
					{
						showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
						ax::NodeEditor::RejectNewItem(ImColor(255, 128, 128), 1.0f);
					}
					else if (endPin->Node->ID == startPin->Node->ID)
					{
						showLabel("x Cannot connect to self", ImColor(45, 32, 32, 180));
						ax::NodeEditor::RejectNewItem(ImColor(255, 128, 128), 1.0f);
					}
					else
					{
						showLabel("+ Create Link", ImColor(32, 45, 32, 180));
						if (ax::NodeEditor::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
						{
							std::string name = startPin->Node->Name;
							name += " -> ";
							name += endPin->Node->Name;
							uint32_t id = entt::hashed_string(name.c_str()).value();

							m_Links.emplace_back(Link(id, startPin->ID, endPin->ID));
							m_Links.back().Color = GetIconColor(startPin->Type);
							m_Links.back().Name = name;
							_animatorController->stateMap[startPin->Node->Name].transitions.push_back({});
							_animatorController->stateMap[startPin->Node->Name].transitions.back().destinationState = endPin->Node->Name;

							_isDirty = true;
						}
					}
				}
			}
		}
		else
			_newLinkPin = nullptr;
		ax::NodeEditor::EndCreate();

		if (ax::NodeEditor::BeginDelete())
		{
			ax::NodeEditor::NodeId nodeId = 0;
			while (ax::NodeEditor::QueryDeletedNode(&nodeId))
			{
				if (ax::NodeEditor::AcceptDeletedItem())
				{
					auto id = std::find_if(m_Nodes.begin(), m_Nodes.end(), [nodeId](auto& node) { return node.ID == nodeId; });

					_animatorController->RemoveState(id->Name);
					_isDirty = true;

					if (id != m_Nodes.end())
						m_Nodes.erase(id);
				}
			}

			ax::NodeEditor::LinkId linkId = 0;
			while (ax::NodeEditor::QueryDeletedLink(&linkId))
			{
				if (ax::NodeEditor::AcceptDeletedItem())
				{
					auto id = std::find_if(m_Links.begin(), m_Links.end(), [linkId](auto& link) { return link.ID == linkId; });

					auto link = FindLink(linkId);
					auto startPin = FindPin(link->StartPinID);
					if (startPin)
					{
						auto state = _animatorController->stateMap.find(startPin->Node->Name);
						if (state != _animatorController->stateMap.end())
						{
							auto transition = std::find_if(state->second.transitions.begin(), state->second.transitions.end(),
								[&](auto& transition) {
									bool determine = false;
									auto findPin = FindPin(link->EndPinID);
									if (findPin)
										determine = transition.destinationState == findPin->Node->Name;
									return !FindNode(transition.destinationState.c_str()) || determine;

								}
							);
							if (transition != state->second.transitions.end())
								state->second.transitions.erase(transition);
						}
					}

					_isDirty = true;

					if (id != m_Links.end())
						m_Links.erase(id);
				}
			}
		}
		ax::NodeEditor::EndDelete();

		// 	if (ImGui::BeginPopup("Pin Context Menu"))
		// 	{
		// 		auto pin = FindPin(contextPinId);
		// 
		// 		ImGui::TextUnformatted("Pin Context Menu");
		// 		ImGui::Separator();
		// 		if (pin)
		// 		{
		// 			ImGui::Text("ID: %p", pin->ID.AsPointer());
		// 			if (pin->Node)
		// 				ImGui::Text("Node: %p", pin->Node->ID.AsPointer());
		// 			else
		// 				ImGui::Text("Node: %s", "<none>");
		// 		}
		// 		else
		// 			ImGui::Text("Unknown pin: %p", contextPinId.AsPointer());
		// 
		// 		ImGui::EndPopup();
		// 	}

// 		if (ImGui::BeginPopup("Link Context Menu"))
// 		{
// 			auto link = FindLink(contextLinkId);
// 
// 			ImGui::TextUnformatted("Link Context Menu");
// 			ImGui::Separator();
// 			if (link)
// 			{
// 				ImGui::Text("ID: %p", link->ID.AsPointer());
// 				ImGui::Text("From: %p", link->StartPinID.AsPointer());
// 				ImGui::Text("To: %p", link->EndPinID.AsPointer());
// 			}
// 			else
// 				ImGui::Text("Unknown link: %p", contextLinkId.AsPointer());
// 			ImGui::Separator();
// 			if (ImGui::MenuItem("Delete"))
// 				ax::NodeEditor::DeleteLink(contextLinkId);
// 			ImGui::EndPopup();
// 		}

		if (ImGui::BeginPopup("Create New Node"))
		{
			auto newNodePostion = openPopupPosition;
			//ImGui::SetCursorScreenPos(ImGui::GetMousePosOnOpeningCurrentPopup());

			//auto drawList = ImGui::GetWindowDrawList();
			//drawList->AddCircleFilled(ImGui::GetMousePosOnOpeningCurrentPopup(), 10.0f, 0xFFFF00FF);

			// 노드를 하나 만든다.
			Node* node = nullptr;
			if (ImGui::MenuItem("Create Empty"))
			{
				std::string nodeName = "Empty Node";

				// 이름이 이미 있는지 검사
				auto iter = _animatorController->stateMap.find(nodeName);
				if (iter != _animatorController->stateMap.end())
				{
					nodeName += " ";
					int index = 0;
					while (true)
					{
						std::string tempName = nodeName + std::to_string(index);
						iter = _animatorController->stateMap.find(tempName);
						if (iter == _animatorController->stateMap.end())
						{
							nodeName = tempName;
							break;
						}
						++index;
					}
				}

				node = SpawnStateNode(nodeName);
				_animatorController->AddState(nodeName);

				_isDirty = true;
			}

			if (node)
			{
				createNewNode = false;

				ax::NodeEditor::SetNodePosition(node->ID, newNodePostion);
			}

			ImGui::EndPopup();
		}
		else
			createNewNode = false;
		ImGui::PopStyleVar();
		ax::NodeEditor::Resume();
# endif
	}
}

void tool::AnimatorPanel::destroyContext(const OnToolRemovePanel& event)
{
	if (event.panelType != PanelType::Animator)
		return;

	m_Nodes.clear();
	m_Links.clear();

	if (_context)
	{
		ax::NodeEditor::DestroyEditor(_context);
		_context = nullptr;
	}
}

void tool::AnimatorPanel::changeAnimator(const OnToolSelectFile& event)
{
	std::filesystem::path path = event.path;

	if (!std::filesystem::is_regular_file(path) || path.extension() != ".controller")
	{
		return;
	}

	_selectControllerPath = event.path;

	// 그냥 파일이 터져있으면 고장나니까 뭐 집어넣어서 초기화해준다.
	if (std::filesystem::file_size(path) == 0)
	{
		core::AnimatorController controller;
		core::AnimatorController::Save(_selectControllerPath.string(), controller);
	}

	// 로드 하고 노드를 만든다.
	_animatorController = core::AnimatorSystem::controllerManager.GetController(_selectControllerPath.string(), _renderer);

	createContext({ PanelType::Animator, nullptr });

	generateNodes(*_animatorController);
}

void tool::AnimatorPanel::generateNodes(const core::AnimatorController& controller)
{
	for (auto& state : controller.stateMap)
	{
		auto node = SpawnStateNode(state.first);
		node->Name = state.first;
		ax::NodeEditor::RestoreNodeState(node->ID);

		for (int i = 0; i < state.second.transitions.size(); ++i)
		{
			std::string name = state.first;
			name += " -> ";
			name += state.second.transitions[i].destinationState;

			std::string startPinName = state.first + " + Out";
			std::string endPinName = state.second.transitions[i].destinationState + " + In";

			uint32_t startPinID = entt::hashed_string(startPinName.c_str()).value();
			uint32_t endPinID = entt::hashed_string(endPinName.c_str()).value();

			uint32_t id = entt::hashed_string(name.c_str()).value();

			m_Links.push_back(Link(id, startPinID, endPinID));
			m_Links.back().Name = name;
		}
	}
}


void tool::AnimatorPanel::printNode(Node* node)
{
	const float rounding = 5.0f;
	const float padding = 12.0f;

	const auto pinBackground = ax::NodeEditor::GetStyle().Colors[ax::NodeEditor::StyleColor_NodeBg];

	ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeBg, ImColor(128, 128, 128, 200));
	ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeBorder, ImColor(32, 32, 32, 200));
	ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_PinRect, ImColor(60, 180, 255, 150));
	ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_PinRectBorder, ImColor(60, 180, 255, 150));

	ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_NodePadding, ImVec4(0, 0, 0, 0));
	ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_NodeRounding, rounding);
	ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_SourceDirection, ImVec2(0.0f, 1.0f));
	ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
	ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_LinkStrength, 0.0f);
	ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinBorderWidth, 1.0f);
	ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinRadius, 0.0f);
	ax::NodeEditor::BeginNode(node->ID);

	ImRect inputsRect;
	int inputAlpha = 200;
	if (!node->Inputs.empty())
	{
		auto& pin = node->Inputs[0];
		ImGui::Dummy(ImVec2(150.f, 12.f));

		inputsRect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

		ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinArrowSize, 10.0f);
		ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinArrowWidth, 10.0f);
		ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinCorners, ImDrawFlags_RoundCornersBottom);
		ax::NodeEditor::BeginPin(pin.ID, ax::NodeEditor::PinKind::Input);
		ax::NodeEditor::PinPivotRect(inputsRect.GetTL(), inputsRect.GetBR());
		ax::NodeEditor::PinRect(inputsRect.GetTL(), inputsRect.GetBR());
		ax::NodeEditor::EndPin();
		ax::NodeEditor::PopStyleVar(3);

		if (_newLinkPin && !CanCreateLink(_newLinkPin, &pin) && &pin != _newLinkPin)
			inputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
	}
	else
		ImGui::Dummy(ImVec2(padding, padding));



	ImGui::Text(node->Name.c_str());
	auto contentRect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());


	ImRect outputsRect;
	int outputAlpha = 200;
	if (!node->Outputs.empty())
	{
		auto& pin = node->Outputs[0];
		ImGui::Dummy(ImVec2(150.f, 12.f));
		outputsRect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

		ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinCorners, ImDrawFlags_RoundCornersTop);
		ax::NodeEditor::BeginPin(pin.ID, ax::NodeEditor::PinKind::Output);
		ax::NodeEditor::PinPivotRect(outputsRect.GetTL(), outputsRect.GetBR());
		ax::NodeEditor::PinRect(outputsRect.GetTL(), outputsRect.GetBR());
		ax::NodeEditor::EndPin();
		ax::NodeEditor::PopStyleVar();

		if (_newLinkPin && !CanCreateLink(_newLinkPin, &pin) && &pin != _newLinkPin)
			outputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
	}
	else
		ImGui::Dummy(ImVec2(padding, padding));

	// 밑에 프로그레스 바를 추가한다.
	if (_isPlaying)
	{
		if (_animator)
		{
			if (_animator->_currentState->name == node->Name)
			{
				if (_animator->_currentState->motion)
				{
					float fraction = _animator->_currentTimePos / _animator->_currentState->motion->duration;
					ImGui::ProgressBar(fraction, { 150.f, 5.f });
				}
			}
			if (_animator->_nextState)
			{
				if (_animator->_nextState->name == node->Name)
				{
					if (_animator->_nextState->motion)
					{
						float fraction = _animator->_nextTimePos / _animator->_nextState->motion->duration;
						ImGui::ProgressBar(fraction, { 150.f, 5.f });
					}
				}
			}
		}
	}


	ax::NodeEditor::EndNode();
	ax::NodeEditor::PopStyleVar(7);
	ax::NodeEditor::PopStyleColor(4);

	auto drawList = ax::NodeEditor::GetNodeBackgroundDrawList(node->ID);

	const auto    topRoundCornersFlags = ImDrawFlags_RoundCornersTop;
	const auto bottomRoundCornersFlags = ImDrawFlags_RoundCornersBottom;

	drawList->AddRectFilled(inputsRect.GetTL(), inputsRect.GetBR(),
		IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, bottomRoundCornersFlags);
	drawList->AddRect(inputsRect.GetTL(), inputsRect.GetBR(),
		IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, bottomRoundCornersFlags);
	drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR(),
		IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, topRoundCornersFlags);
	drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR(),
		IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, topRoundCornersFlags);
	// 	drawList->AddRectFilled(contentRect.GetTL(), contentRect.GetBR(), 
	// 		IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 0.0f);
	// 	drawList->AddRect(
	// 		contentRect.GetTL(),
	// 		contentRect.GetBR(),
	// 		IM_COL32(48, 128, 255, 100), 0.0f);
}

void tool::AnimatorPanel::showLeftPane(float paneWidth)
{
	auto& io = ImGui::GetIO();

	ImGui::BeginChild("Parameter", ImVec2(paneWidth, 0));

	static std::string searchQuery;

	// 검색창 추가
	ImGui::InputText("Search", &searchQuery);

	_searchQueryLower = searchQuery;
	std::transform(_searchQueryLower.begin(), _searchQueryLower.end(), _searchQueryLower.begin(), ::tolower);

	// '+' 버튼 추가
	if (_isPlaying)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}

	ImGui::SameLine();
	if (ImGui::Button("+")) {
		ImGui::OpenPopup("AddParameterPopup");
	}

	if (_isPlaying)
	{
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();
	}

	// 드롭다운 메뉴
	if (ImGui::BeginPopup("AddParameterPopup")) {
		if (ImGui::MenuItem("int")) {
			_animatorController->AddParameter("New Int", entt::meta_any(0));
			_isDirty = true;
		}
		if (ImGui::MenuItem("float")) {
			_animatorController->AddParameter("New Float", entt::meta_any(0.0f));
			_isDirty = true;
		}
		if (ImGui::MenuItem("bool")) {
			_animatorController->AddParameter("New Bool", entt::meta_any(false));
			_isDirty = true;
		}
		if (ImGui::MenuItem("string")) {
			_animatorController->AddParameter("New String", entt::meta_any(std::string("")));
			_isDirty = true;
		}
		ImGui::EndPopup();
	}

	ImGui::Separator();

	showParameters();



	ImGui::EndChild();
}


void tool::AnimatorPanel::showParameters()
{
	std::unordered_map<std::string, core::AnimatorParameter>* parameters = nullptr;

	if (_selectedEntities.empty())
	{
		if (_animatorController)
		{
			parameters = &_animatorController->parameters;
		}
		else
			return;
	}
	else
	{
		if (_animator)
		{
			if (_isPlaying)
			{
				parameters = &_animator->parameters;
			}
			else
			{
				parameters = &_animatorController->parameters;
			}
		}
		else
			return;
	}

	if (parameters)
	{
		// 파라미터 표시 함수 호출
		for (auto& param : *parameters)
		{
			std::string paramNameLower = param.first;
			std::transform(paramNameLower.begin(), paramNameLower.end(), paramNameLower.begin(), ::tolower);

			// 검색어와 일치하는 파라미터만 표시
			if (_searchQueryLower.empty() || paramNameLower.find(_searchQueryLower) != std::string::npos)
			{
				ImGui::PushID(param.first.c_str());

				if (_isPlaying)
				{
					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				}

				if (ImGui::Button("-"))
				{
					_animatorController->parameters.erase(param.first);
					_isDirty = true;
					ImGui::PopID();
					break;
				}

				if (_isPlaying)
				{
					ImGui::PopItemFlag();
					ImGui::PopStyleVar();
				}

				ImGui::SameLine();

				auto&& paramType = param.second.value.type();

				if (paramType == entt::resolve<int>())
				{
					int temp = param.second.value.cast<int>();
					if (ImGui::InputInt("##value", &temp))
					{
						param.second.value = temp;
						_isDirty = true;
					}
				}
				else if (paramType == entt::resolve<float>())
				{
					float temp = param.second.value.cast<float>();
					if (ImGui::InputFloat("##value", &temp))
					{
						param.second.value = temp;
						_isDirty = true;
					}
				}
				else if (paramType == entt::resolve<bool>())
				{
					bool temp = param.second.value.cast<bool>();
					if (ImGui::Checkbox("##value", &temp))
					{
						param.second.value = temp;
						_isDirty = true;
					}
				}
				else if (paramType == entt::resolve<std::string>())
				{
					std::string temp = param.second.value.cast<std::string>();
					if (ImGui::InputText("##value", &temp))
					{
						param.second.value = temp;
						_isDirty = true;
					}
				}

				ImGui::SameLine();

				static std::string newName;
				static bool isRenaming = false;
				static std::string renamingParam;

				if (isRenaming && renamingParam == param.first)
				{
					char buffer[256];
					strncpy_s(buffer, newName.c_str(), sizeof(buffer));
					if (ImGui::InputText("##rename", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue))
					{
						newName = buffer;
						if (newName.empty())
						{
							newName = param.first;
						}
						if (newName != param.first)
						{
							_animatorController->parameters[newName] = std::move(_animatorController->parameters[param.first]);
							_animatorController->parameters.erase(param.first);
						}
						_isDirty = true;
						isRenaming = false;

						ImGui::PopID();
						break;
					}

					if (ImGui::IsItemDeactivated())
					{
						isRenaming = false;
					}
				}
				else
				{
					if (ImGui::Selectable(param.first.c_str()))
					{
						newName = param.first;
						isRenaming = true;
						renamingParam = param.first;
					}
				}

				//ImGui::Text(param.first.c_str());

				ImGui::PopID();

			}
		}
	}

}

void tool::AnimatorPanel::changeNodeName(Node* node, const std::string& newName)
{
	auto nodePos = ax::NodeEditor::GetNodePosition(node->ID);
	auto originalName = node->Name;

	int index = 0;
	std::string availableName = newName;
	while (_animatorController->stateMap.find(availableName) != _animatorController->stateMap.end())
	{
		availableName = newName + " " + std::to_string(index++);
	}

	auto& state = _animatorController->stateMap[node->Name];
	state.name = availableName;
	_animatorController->stateMap[availableName] = std::move(state);
	_animatorController->stateMap.erase(node->Name);

	node->Name = availableName;

	node->ID = entt::hashed_string(node->Name.c_str()).value();

	auto newInputPinID = entt::hashed_string((node->Name + " + In").c_str()).value();
	auto newOutputPinID = entt::hashed_string((node->Name + " + Out").c_str()).value();

	for (auto& link : m_Links)
	{
		if (link.StartPinID == node->Outputs[0].ID)
		{
			auto endPinOwner = FindPin(link.EndPinID)->Node;
			std::string newLinkName = node->Name + " -> " + endPinOwner->Name;
			link.ID = entt::hashed_string(newLinkName.c_str()).value();
			link.StartPinID = newOutputPinID;
		}

		else if (link.EndPinID == node->Inputs[0].ID)
		{
			auto startPinOwner = FindPin(link.StartPinID)->Node;
			std::string newLinkName = startPinOwner->Name + " -> " + node->Name;
			link.ID = entt::hashed_string(newLinkName.c_str()).value();
			link.EndPinID = newInputPinID;

			for (auto& transition : _animatorController->stateMap[startPinOwner->Name].transitions)
			{
				if (transition.destinationState == originalName)
				{
					transition.destinationState = node->Name;
				}
			}

		}
	}

	node->Inputs[0].ID = newInputPinID;
	node->Inputs[0].Name = node->Name + " + In";
	node->Outputs[0].ID = newOutputPinID;
	node->Outputs[0].Name = node->Name + " + Out";

	_isDirty = true;

	ax::NodeEditor::SetNodePosition(node->ID, nodePos);

}

void tool::AnimatorPanel::refreshInspector()
{
	static bool isSentEvent = false;

	auto isSelected = ax::NodeEditor::GetSelectedObjectCount();
	auto isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	if (!isSentEvent && isSelected && isFocused)
	{
		_dispatcher->enqueue<OnToolSelectFile>(_selectControllerPath.string());
		isSentEvent = true;
	}
	else if (isSentEvent && (!isSelected || !isFocused))
	{
		isSentEvent = false;
	}
}

void tool::AnimatorPanel::playScene(const OnToolPlayScene& event)
{
	_isPlaying = true;
}

void tool::AnimatorPanel::stopScene(const OnToolStopScene& event)
{
	_isPlaying = false;
}
