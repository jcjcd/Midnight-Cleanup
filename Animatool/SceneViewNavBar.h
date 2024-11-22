#pragma once
#include "Scene.h"

namespace tool
{
    enum class ToolType
    {
        Hand,
        Move,
        Rotate,
        Scale
    };
    
    class SceneViewNavBar
    {
    public:
        explicit SceneViewNavBar(entt::dispatcher& dispatcher);

        void RenderToolTypePanel(Scene::GizmoOperation& gizmoOperation, tool::Scene::Mode& mode, bool& useSnap, Vector3& snap, bool isRightClicked, Scene* scene);
        void RenderPlayPauseButton();

    private:
        void renderRadioButton(tool::Scene::GizmoOperation& gizmoOperation, tool::Scene::GizmoOperation operation, const char* label, const char* icon);

        entt::dispatcher* _dispatcher = nullptr;

        ToolType _currentTool = ToolType::Hand;
        bool _isPlaying = false;
        bool _isPaused = false;
    };
}
