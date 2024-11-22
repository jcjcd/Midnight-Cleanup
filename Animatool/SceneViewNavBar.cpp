#include "pch.h"
#include "SceneViewNavBar.h"

#include "Scene.h"
#include "ToolEvents.h"
#include "ToolProcess.h"

tool::SceneViewNavBar::SceneViewNavBar(entt::dispatcher& dispatcher)
	: _dispatcher(&dispatcher)
{
}

void tool::SceneViewNavBar::RenderToolTypePanel(Scene::GizmoOperation& gizmoOperation, tool::Scene::Mode& mode, bool& useSnap, Vector3& snap, bool isRightClicked, Scene* scene)
{
	if (_isPlaying)
		return;

	// 키 입력에 따라 GizmoOperation 변경
	if (!isRightClicked)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Q)) gizmoOperation = Scene::GizmoOperation::Hand;
		if (ImGui::IsKeyPressed(ImGuiKey_W)) gizmoOperation = Scene::GizmoOperation::Move;
		if (ImGui::IsKeyPressed(ImGuiKey_E)) gizmoOperation = Scene::GizmoOperation::Rotate;
		if (ImGui::IsKeyPressed(ImGuiKey_R)) gizmoOperation = Scene::GizmoOperation::Scale;
	}

	if (ImGui::BeginChild("Tool Type", {}, ImGuiChildFlags_AutoResizeX))
	{
		// GizmoOperation 라디오 버튼
		renderRadioButton(gizmoOperation, Scene::Hand, "Hand", "hand");
		renderRadioButton(gizmoOperation, Scene::Move, "Move", "move");
		renderRadioButton(gizmoOperation, Scene::Rotate, "Rotate", "rotate");
		renderRadioButton(gizmoOperation, Scene::Scale, "Scale", "scale");
		ImGui::NewLine();

		// Mode 라디오 버튼
		if (!(gizmoOperation & (Scene::GizmoOperation::Scale | Scene::GizmoOperation::Hand)))
		{
			ImGui::RadioButton("Local", reinterpret_cast<int*>(&mode), Scene::Mode::Local);
			ImGui::SameLine();
			ImGui::RadioButton("World", reinterpret_cast<int*>(&mode), Scene::Mode::World);
		}

		// Snap 여부 및 수치 조절
		ImGui::Checkbox("##useSnap", &useSnap);

		ImGui::SameLine();
		if (ImGui::DragFloat("Snap", &snap.x))
			snap.z = snap.y = snap.x;

		ImGui::Checkbox("manageUI", &scene->_manageUI);

		ImGui::Checkbox("useLightMap", &scene->_useLightMap);

		ImGui::Checkbox("showCollider", &scene->_showCollider);

		ImGui::EndChild();
	}
}


void tool::SceneViewNavBar::RenderPlayPauseButton()
{
	if (ImGui::BeginMenuBar())
	{
		// 가운데로 위치 설정
		float windowWidth = ImGui::GetWindowSize().x;
		float buttonWidth = 50 + ImGui::GetStyle().FramePadding.x * 2.0f;
		ImGui::SetCursorPosX((windowWidth - buttonWidth * 2) * 0.5f);

		if (_isPlaying)
		{
			if (ImGui::Button("Stop", ImVec2(buttonWidth, 0)))
			{
				// Scene 정지
				_dispatcher->enqueue<OnToolStopScene>();
				_isPlaying = false;
				_isPaused = false;
			}
		}
		else
		{
			if (ImGui::Button("Play", ImVec2(buttonWidth, 0)))
			{
				// Scene 재생
				_dispatcher->enqueue<OnToolPlayScene>();
				_isPlaying = true;
				_isPaused = false;
			}
		}

		ImGui::SameLine();

		ImGui::BeginDisabled(!_isPlaying);

		if (_isPaused)
		{
			if (ImGui::Button("Resume", ImVec2(buttonWidth, 0)))
			{
				_dispatcher->enqueue<OnToolResumeScene>();
				_isPaused = false;
			}
		}
		else
		{
			if (ImGui::Button("Pause", ImVec2(buttonWidth, 0)))
			{
				// Scene 일시정지
				_dispatcher->enqueue<OnToolPauseScene>();
				_isPaused = true;
			}
		}

		ImGui::EndDisabled();

	}

	ImGui::EndMenuBar();
}

void tool::SceneViewNavBar::renderRadioButton(tool::Scene::GizmoOperation& gizmoOperation,
	tool::Scene::GizmoOperation operation, const char* label, const char* icon)
{
	ImGui::PushID(label);

	// 선택 버튼 강조 표시
	if (const bool isSelected = (gizmoOperation == operation))
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.8f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.9f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.7f, 1.0f));
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
	}

	if (ImGui::ImageButton((void*)ToolProcess::icons[icon], ImVec2(24, 24)))
		gizmoOperation = operation;

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(label);

	ImGui::PopStyleColor(3);

	ImGui::PopID();
	ImGui::SameLine();
}
