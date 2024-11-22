#pragma once
#include "Panel.h"

class Renderer;

namespace tool
{
    class Project : public Panel
	{
    public:
        Project(entt::dispatcher& dispatcher, Renderer* renderer);

        void RenderPanel(float deltaTime) override;
        PanelType GetType() override { return PanelType::Project; }

    private:
        // 파일 경로 출력
        void renderPathBreadcrumbs();

        // 디렉터리 트리 노드 출력
        void renderDirectoryTree(const std::filesystem::path& rootPath);

        // 컬럼에 맞게 이미지 버튼 정렬
        void renderDirectoryContents(const std::filesystem::path& path);

        // 파일 확장자별 이미지 버튼 출력
        void renderFile(const std::filesystem::path& path);

        void handleFileDrop();

        // 파일 검색
        void searchFiles(const std::string& query);

        // 우클릭 팝업 콘텍스트 처리
        void processPopup();

        // 머터리얼 생성
        void createMaterial(const std::string& name);

        // 애니메이션 생성
		void createAnimator(const std::string& name);

        // 물리 머터리얼 생성
        void createPhysicMaterial(const std::string& name);

        // 아이콘 매칭
        void matchIcon();

        // 아이콘 포인터 반환
        ImTextureID getIcon(const std::string& iconName);

    	std::filesystem::path _resourcesPath;
        std::filesystem::path _currentPath;
        std::filesystem::path _selectedFile;
        std::vector<std::filesystem::path> _searchResults;
        std::vector<std::string> _allowedExtensions;
        std::unordered_map<std::string, std::string> _extensionMatcher;

        std::string _searchQuery;
        Renderer* _renderer = nullptr;
        bool _showFavorites = false;
        ImVec2 _buttonSize = ImVec2(100.f, 20.f);  // 버튼 크기 조절
        ImVec2 _iconSize = ImVec2(64.f, 64.f);  // 버튼 크기 조절


        enum PopupTypes
        {
            None,
            NewMaterial,
            NewAnimator,
            NewPhysicMaterial,
        };

        PopupTypes _popupTypes = None;
    };

}
