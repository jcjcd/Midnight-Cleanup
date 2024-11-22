#pragma once
#include <recastnavigation/Recast.h>
#include <recastnavigation/DetourNavMeshQuery.h>

class dtNavMesh;

namespace core
{
    class Scene;
	struct NavMeshData;
	struct NavMeshSettings;

    class NavMeshBuilder
    {
    public:
        NavMeshBuilder(Scene* scene);
        ~NavMeshBuilder();

        // 네비게이션 메쉬를 설정값과 데이터를 기반으로 생성하는 함수
        bool BuildNavMesh(const NavMeshSettings& settings, NavMeshData& data);

        // 네비게이션 메쉬를 파일에 저장하는 함수
        void SaveNavMesh(std::filesystem::path path);

        // 파일로부터 네비게이션 메쉬를 불러오는 함수
        void LoadNavMesh(std::filesystem::path path, NavMeshData& data);

        // 네비게이션 메쉬의 버텍스, 인덱스 정보를 직접 제공하는 함수
        void InputWorld(const std::vector<float>& vertices, const std::vector<int>& indices);

        // 파일에서 네비게이션 메쉬를 불러오는 함수
        void InputMesh(std::filesystem::path path);

    private:
        // 초기화 및 정리 함수
        void initialize();                // 데이터 구조 초기화
        void cleanup();                   // 리소스 정리
        void calcMinMax();                // min max 할당

        Scene* _scene = nullptr;

        // Recast/Detour와 관련된 멤버 변수
        rcContext* _context = nullptr;
        dtNavMesh* _navMesh = nullptr;                       // 네비게이션 메쉬
        dtNavMeshQuery* _navQuery = nullptr;                 // 네비게이션 쿼리 시스템
        rcConfig _config;                                    // Recast 설정
        rcHeightfield* _heightfield = nullptr;               // 높이 필드 (지형의 높이 데이터)
        rcCompactHeightfield* _compactHeightfield = nullptr; // 압축된 높이 필드
        rcContourSet* _contourSet = nullptr;                 // 윤곽선 데이터
        rcPolyMesh* _polyMesh = nullptr;                     // 폴리곤 메쉬
        rcPolyMeshDetail* _polyMeshDetail = nullptr;         // 세부 폴리곤 메쉬
        dtNavMeshQuery* _navMeshQuery = nullptr;             //

        std::vector<float> _vertices;
        std::vector<int> _indices;
        float _bMin[3];
        float _bMax[3];
    };
}
