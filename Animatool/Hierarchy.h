#pragma once
#include "Panel.h"

namespace core
{
	class Scene;
}

namespace tool
{
    class Hierarchy : public Panel
	{
    public:
        Hierarchy(entt::dispatcher& dispatcher);

        void RenderPanel(float deltaTime) override;
        PanelType GetType() override { return PanelType::Hierarchy; }

        void renderEntityNode(entt::entity entity);
    private:
        std::vector<entt::entity> _selectedEntities;
        std::vector<entt::entity> _previousSelectedEntities;
        std::unordered_set<entt::entity> _expandedEntities;
        std::map<entt::entity, entt::entity> _copyMapping;
        entt::entity _lastSelected = entt::null;
        entt::entity _entityToCopy = entt::null;
        entt::entity _renamingEntity = entt::null;
        ImGui::FileBrowser _fileDialog;

        char _searchBuffer[256] = "";
        int _searchBy = 0;
        bool _isFocused = true;

        // 엔티티 렌더링 및 검색 기능
        void renderSearchAndContext();
        void renderNodeContext(entt::entity entity, bool& nodeOpened, bool& dialogOpened);
        bool searchMatches(entt::entity entity);
        bool searchRecursively(entt::entity entity);

        // 엔티티 선택 범위 함수
        void selectEntitiesInRange(entt::entity start, entt::entity end);

        // 이벤트 핸들러
        void selectEntity(const OnToolSelectEntity& event);
        void postLoadScene(const OnToolPostLoadScene& event);

        // 이벤트 송신
        void throwSelectEntity() const;
        void throwDestroyEntities();

        // 복사 기능
        void copyEntity(entt::registry& registry, entt::entity src, entt::entity dst);

        // 드래그 앤 드롭 타겟 설정
        bool beginDragDropTargetWindow(const char* payloadType);
	};
}

