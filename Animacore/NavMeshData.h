#pragma once
#include <recastnavigation/Recast.h>

namespace core
{
    class NavMeshData
    {
    public:
        // NavMesh 생성에 필요한 메쉬 데이터를 포함한 구조체
        std::vector<float> vertices; // 정점 데이터
        std::vector<int> indices;    // 인덱스 데이터
    };

}
