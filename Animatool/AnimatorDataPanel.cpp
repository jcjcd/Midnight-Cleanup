#include "pch.h"
#include "AnimatorDataPanel.h"

#include "AnimatorPanel.h"
#include "ToolEvents.h"

#include "../Animavision/Renderer.h"
#include <../imgui_node_editor/imgui_node_editor.h>

#include "ToolProcess.h"

#include "../Animacore/MetaCtxs.h"
#include "../midnight_cleanup/McMetaFuncs.h"

tool::AnimatorDataPanel::AnimatorDataPanel(entt::dispatcher& dispatcher)
	: Panel(dispatcher)
{
	_dispatcher->sink<OnToolAddedPanel>().connect<&AnimatorDataPanel::addedPanel>(this);
	_dispatcher->sink<OnToolRemovePanel>().connect<&AnimatorDataPanel::removePanel>(this);
}

void tool::AnimatorDataPanel::RenderPanel(float deltaTime)
{
	if (!_animatorPanel)
		return;

	auto& io = ImGui::GetIO();
	ImGui::BeginChild("Parameter");

	auto meta = entt::resolve<global::CallBackFuncDummy>(global::callbackEventMetaCtx);

	auto funcs = meta.func();


	// 1. 노드를 클릭했을때
	std::vector<ax::NodeEditor::NodeId> selectedNodes;
	std::vector<ax::NodeEditor::LinkId> selectedLinks;
	selectedNodes.resize(ax::NodeEditor::GetSelectedObjectCount());
	selectedLinks.resize(ax::NodeEditor::GetSelectedObjectCount());

	int nodeCount = ax::NodeEditor::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
	int linkCount = ax::NodeEditor::GetSelectedLinks(selectedLinks.data(), static_cast<int>(selectedLinks.size()));

	selectedNodes.resize(nodeCount);
	selectedLinks.resize(linkCount);

	ImGui::Image(ToolProcess::icons["controller"], { 32, 32 });
	ImGui::SameLine();

	if (nodeCount == 0 && linkCount == 0)
	{
		ImGui::Text("Select a node or link to view its properties.");
	}
	else
	{
		if (nodeCount > 0 && linkCount == 0)
		{
			if (nodeCount == 1)
			{
				auto& nodeID = selectedNodes[0];
				auto node = _animatorPanel->FindNode(nodeID);
				if (node)
				{
					std::string nodeName = node->Name;
					if (ImGui::InputText("Name", &nodeName, ImGuiInputTextFlags_EnterReturnsTrue))
					{
						if (nodeName.empty())
						{
							nodeName = node->Name;
						}
						else if (nodeName != node->Name)
						{
							_animatorPanel->changeNodeName(node, nodeName);
						}
						// 여기에 isDirty가 들어있다.
					}
					ImGui::Separator();

					auto& motionName = _animatorPanel->_animatorController->stateMap[node->Name].motionName;
					if (ImGui::BeginCombo("Motion", motionName.c_str()))
					{
						for (const auto& key : *_animatorPanel->_renderer->GetAnimationClips() | std::views::keys)
						{
							bool isSelected = motionName == key;
							if (ImGui::Selectable(key.c_str(), isSelected))
							{
								motionName = key;
								_animatorPanel->_animatorController->stateMap[node->Name].motion = _animatorPanel->_renderer->GetAnimationClip(key);
								_animatorPanel->_isDirty = true;
							}
							if (isSelected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}

					ImGui::Separator();

					ImGui::Text("Transitions");

					for (int i = 0; i < _animatorPanel->_animatorController->stateMap[node->Name].transitions.size(); ++i)
					{
						auto& transition = _animatorPanel->_animatorController->stateMap[node->Name].transitions[i];
						std::string transitionName = node->Name + " -> " + transition.destinationState;
						if (ImGui::Selectable(transitionName.c_str()))
						{
							// 창: 여기 뭐 하자
						}
					}

					ImGui::Separator();

					if (ImGui::Checkbox("Is Loop", &_animatorPanel->_animatorController->stateMap[node->Name].isLoop))
					{
						_animatorPanel->_isDirty = true;
					}

					if (ImGui::InputFloat("Multiplier", &_animatorPanel->_animatorController->stateMap[node->Name].multiplier, 0.1f, 1.0f, "%.3f"))
					{
						_animatorPanel->_isDirty = true;
					}

					// 여기에 이벤트 작성
					ImGui::Separator();
					ImGui::Separator();
					ImGui::Text("Events");
					ImGui::Separator();

					auto& events = _animatorPanel->_animatorController->stateMap[node->Name].animationEvents;
					for (int i = 0; i < events.size(); ++i)
					{
						ImGui::PushID(i);
						auto& event = events[i];
						std::string functionName = event.functionName;
						if (ImGui::BeginCombo("Event", functionName.c_str()))
						{
							for (auto&& func : funcs)
							{
								bool isSelected = functionName == func.second.prop("name"_hs).value().cast<const char*>();
								if (ImGui::Selectable(func.second.prop("name"_hs).value().cast<const char*>(), isSelected))
								{
									event.functionName = func.second.prop("name"_hs).value().cast<const char*>();
									event.parameters.resize(func.second.arity());
									for (int j = 0; j < func.second.arity(); ++j)
									{
										auto param = func.second.arg(j);
										if (param.info() == entt::resolve<int>().info())
										{
											event.parameters[j] = 0;
										}
										else if (param.info() == entt::resolve<float>().info())
										{
											event.parameters[j] = 0.0f;
										}
										else if (param.info() == entt::resolve<bool>().info())
										{
											event.parameters[j] = false;
										}
										else if (param.info() == entt::resolve<std::string>().info())
										{
											event.parameters[j] = std::string("");
										}
										else if (param.info() == entt::resolve<entt::entity>().info())
										{
											event.parameters[j] = entt::entity(entt::null);
										}
										else
										{
											event.parameters[j] = std::string("");
										}
									}
									_animatorPanel->_isDirty = true;
								}
								if (isSelected)
								{
									ImGui::SetItemDefaultFocus();
								}
							}
							ImGui::EndCombo();
						}

						ImGui::SameLine();

						if (ImGui::Button("X"))
						{
							events.erase(events.begin() + i);
							_animatorPanel->_isDirty = true;
						}

						auto&& func = meta.func(entt::hashed_string{ event.functionName.c_str() });

						if (func)
						{
							auto size = func.arity();
							for (int i = 0; i < size; ++i)
							{
								ImGui::PushID(i);
								auto param = func.arg(i);
								// ImGui::Text(param.info().name().data());
								// 여기에 세부 수치를 조절 가능해야함
								if (param.info() == entt::resolve<int>().info())
								{
									ImGui::Text("int");
									ImGui::SameLine();
									int temp = event.parameters[i].cast<int>();
									if (ImGui::InputInt("##value", &temp))
									{
										event.parameters[i] = temp;
										_animatorPanel->_isDirty = true;
									}
								}
								else if (param.info() == entt::resolve<float>().info())
								{
									ImGui::Text("float");
									ImGui::SameLine();
									float temp = event.parameters[i].cast<float>();
									if (ImGui::InputFloat("##value", &temp))
									{
										event.parameters[i] = temp;
										_animatorPanel->_isDirty = true;
									}
								}
								else if (param.info() == entt::resolve<bool>().info())
								{
									ImGui::Text("bool");
									ImGui::SameLine();
									bool temp = event.parameters[i].cast<bool>();
									int intTemp = temp;
									if (ImGui::Combo("##value", &intTemp, "False\0True\0"))
									{
										event.parameters[i] = static_cast<bool>(intTemp);
										_animatorPanel->_isDirty = true;
									}
								}
								else if (param.info() == entt::resolve<std::string>().info())
								{
									ImGui::Text("string");
									ImGui::SameLine();
									std::string temp = event.parameters[i].cast<std::string>();

									bool isDirty = ImGui::InputText("##value", &temp);

									// 드래그 앤 드롭 동작 처리
									if (ImGui::BeginDragDropTarget())
									{
										if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PAYLOAD"))
										{
											auto path = std::string(static_cast<const char*>(payload->Data));
											temp = path;
											isDirty = true;
										}
										ImGui::EndDragDropTarget();
									}

									if (isDirty)
									{
										event.parameters[i] = temp;
										_animatorPanel->_isDirty = true;
									}
								}
								else if (param.info() == entt::resolve<entt::entity>().info())
								{
									ImGui::Text("entity");
									ImGui::SameLine();
									entt::entity temp = event.parameters[i].cast<entt::entity>();
									bool isDirty = ImGui::InputScalar("##value", ImGuiDataType_U32, &temp);

									// 드래그 앤 드롭 동작 처리
									if (ImGui::BeginDragDropTarget())
									{
										if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_PAYLOAD"))
										{
											auto entity = static_cast<entt::entity*>(payload->Data);
											temp = *entity;
											isDirty = true;
										}
										ImGui::EndDragDropTarget();
									}

									if (isDirty)
									{
										event.parameters[i] = temp;
										_animatorPanel->_isDirty = true;
									}
								}
								else
								{
									ImGui::Text(param.info().name().data());
								}

								ImGui::PopID();
							}

							if (ImGui::InputFloat("Time", &event.time))
							{
								_animatorPanel->_isDirty = true;
							}

							
						}

						ImGui::PopID();

						ImGui::Separator();
					}

					if (ImGui::Button("+"))
					{
						_animatorPanel->_animatorController->stateMap[node->Name].animationEvents.push_back({});
						_animatorPanel->_isDirty = true;
					}
					ImGui::SameLine();
					if (ImGui::Button("-"))
					{
						_animatorPanel->_animatorController->stateMap[node->Name].animationEvents.pop_back();
						_animatorPanel->_isDirty = true;
					}

					ImGui::Separator();


				}
			}
			else
			{
				for (auto& nodeID : selectedNodes)
				{
					auto node = _animatorPanel->FindNode(nodeID);
					if (node)
					{
						ImGui::Text("Name: %s", node->Name.c_str());
						ImGui::Text("ID: %p", node->ID.AsPointer());
						ImGui::Text("Type: %s", node->Type == NodeType::Blueprint ? "Blueprint" : (node->Type == NodeType::Tree ? "Tree" : "Comment"));
						ImGui::Text("Inputs: %d", (int)node->Inputs.size());
						ImGui::Text("Outputs: %d", (int)node->Outputs.size());
						ImGui::Separator();
					}
				}
			}
		}
		else if (linkCount > 0 && nodeCount == 0)
		{
			ImGui::Text("Link Properties");
			ImGui::Separator();

			if (linkCount == 1)
			{
				auto& linkID = selectedLinks[0];
				auto link = _animatorPanel->FindLink(linkID);
				if (link)
				{
					ImGui::Text("Transition: %s", link->Name.c_str());
					ImGui::Separator();
					ImGui::Text("Conditions");
					ImGui::Separator();

					auto& parameters = _animatorPanel->_animatorController->parameters;

					auto& transitions = _animatorPanel->_animatorController->stateMap[_animatorPanel->FindPin(link->StartPinID)->Node->Name].transitions;

					core::AnimatorTransition* transition = nullptr;
					for (auto& t : transitions)
					{
						if (t.destinationState == _animatorPanel->FindPin(link->EndPinID)->Node->Name)
						{
							transition = &t;
							break;
						}
					}

					int uid = 0;
					for (auto& condition : transition->conditions)
					{
						ImGui::PushID(uid++);

						if (parameters.size() > 0)
						{
							bool isExist = parameters.contains(condition.parameter);
							if (ImGui::BeginCombo("Parameter", condition.parameter.c_str()))
							{
								for (auto& param : parameters)
								{
									bool isSelected = condition.parameter == param.first;
									if (ImGui::Selectable(param.first.c_str(), isSelected))
									{
										condition.parameter = param.first;
										condition.threshold = param.second.value;
										_animatorPanel->_isDirty = true;
									}
									if (isSelected)
									{
										ImGui::SetItemDefaultFocus();
									}
								}
								ImGui::EndCombo();
							}


							if (!isExist)
							{
								ImGui::Text("Parameter not found");
							}
							else
							{
								int mode = (int)condition.mode;
								if (ImGui::Combo("##operator", &mode, "If\0If Not\0Greater\0Less\0Equal\0Not Equal\0Trigger\0"))
								{
									condition.mode = static_cast<core::AnimatorCondition::Mode>(mode);
									_animatorPanel->_isDirty = true;
								}

								if (condition.mode != core::AnimatorCondition::Mode::Trigger)
								{
									if (condition.threshold.type() == entt::resolve<int>())
									{
										int temp = condition.threshold.cast<int>();
										if (ImGui::InputInt("##value", &temp))
										{
											condition.threshold = temp;
											_animatorPanel->_isDirty = true;
										}
									}
									else if (condition.threshold.type() == entt::resolve<float>())
									{
										float temp = condition.threshold.cast<float>();
										if (ImGui::InputFloat("##value", &temp))
										{
											condition.threshold = temp;
											_animatorPanel->_isDirty = true;
										}
									}
									else if (condition.threshold.type() == entt::resolve<bool>())
									{
										bool temp = condition.threshold.cast<bool>();
										int intTemp = temp;
										if (ImGui::Combo("##value", &intTemp, "False\0True\0"))
										{
											condition.threshold = static_cast<bool>(intTemp);
											_animatorPanel->_isDirty = true;
										}
									}
									else if (condition.threshold.type() == entt::resolve<std::string>())
									{
										std::string temp = condition.threshold.cast<std::string>();
										if (ImGui::InputText("##value", &temp))
										{
											condition.threshold = temp;
											_animatorPanel->_isDirty = true;
										}
									}
								}
							}
						}
						else
						{
							ImGui::Text("No parameters in Current Controller");
						}

						ImGui::PopID();
						ImGui::Separator();

					}
					if (ImGui::Button("+"))
					{
						if (parameters.size() > 0)
						{
							if (parameters.begin()->second.value.type() == entt::resolve<int>())
							{
								transition->AddCondition(core::AnimatorCondition::Mode::Equals, parameters.begin()->first, 0);
								_animatorPanel->_isDirty = true;
							}
							else if (parameters.begin()->second.value.type() == entt::resolve<float>())
							{
								transition->AddCondition(core::AnimatorCondition::Mode::Equals, parameters.begin()->first, 0.0f);
								_animatorPanel->_isDirty = true;
							}
							else if (parameters.begin()->second.value.type() == entt::resolve<bool>())
							{
								transition->AddCondition(core::AnimatorCondition::Mode::Equals, parameters.begin()->first, false);
								_animatorPanel->_isDirty = true;
							}
							else if (parameters.begin()->second.value.type() == entt::resolve<std::string>())
							{
								transition->AddCondition(core::AnimatorCondition::Mode::Equals, parameters.begin()->first, std::string(""));
								_animatorPanel->_isDirty = true;
							}
						}
						else
						{
							transition->AddCondition(core::AnimatorCondition::Mode::Equals, "New Parameter", 0);
						}
					}
					ImGui::SameLine();
					if (ImGui::Button("-"))
					{
						transition->conditions.pop_back();
						_animatorPanel->_isDirty = true;
					}

					ImGui::Separator();
					// 여기에서 블렌드 시간 관리
					if (ImGui::InputFloat("Blend Time", &transition->blendTime))
					{
						_animatorPanel->_isDirty = true;
					}

					if (ImGui::InputFloat("Exit Time", &transition->exitTime))
					{
						_animatorPanel->_isDirty = true;
					}
				}
			}
			else
			{
				for (auto& linkID : selectedLinks)
				{
					auto link = _animatorPanel->FindLink(linkID);
					if (link)
					{
						ImGui::Text("ID: %p", link->ID.AsPointer());
						ImGui::Text("From: %p", link->StartPinID.AsPointer());
						ImGui::Text("To: %p", link->EndPinID.AsPointer());
						ImGui::Separator();
					}
				}
			}
		}
		else
		{
			// 이거 선택안되네.. 또르륵
			ImGui::Text("Multiple Selection");
			ImGui::Separator();
			ImGui::Text("Narrow the Selection");
			ImGui::Separator();
			ImGui::Text("%d Animator State(s)", nodeCount);
			ImGui::Text("%d Animator State Transition(s)", linkCount);
		}
	}


	ImGui::EndChild();
}

void tool::AnimatorDataPanel::addedPanel(const OnToolAddedPanel& event)
{
	if (event.panelType == PanelType::Animator)
		_animatorPanel = dynamic_cast<AnimatorPanel*>(event.panel);
}

void tool::AnimatorDataPanel::removePanel(const tool::OnToolRemovePanel& event)
{
	if (event.panelType == PanelType::Animator)
		_animatorPanel = nullptr;
}
