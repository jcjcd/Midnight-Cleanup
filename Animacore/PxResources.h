#pragma once
#include "CorePhysicsComponents.h"

namespace core
{
	class Scene;

	class PxResources
    {
        using p_mat = ColliderCommon::PhysicMaterial;

    public:
        // 물리 시스템 초기화 및 물리 씬 생성
		physx::PxScene* SceneInitialize(Scene* scene);

        // 물리 씬 마무리
        int SceneFinalize();

        // 물리 머터리얼 반환
        physx::PxMaterial* GetMaterial(const std::string& name);

        // 물리 머터리얼 저장
        static void SaveMaterial(const std::filesystem::path& path, const p_mat& material);

        // 물리 머터리얼 객체 생성
        static p_mat ReadMaterial(const std::filesystem::path& path);

        // 물리 머터리얼 로드 (시스템 내부적으로 사용)
        physx::PxMaterial* LoadMaterial(const std::filesystem::path& path);

        physx::PxPhysics* physics = nullptr;
        physx::PxPvd* pvd = nullptr;
        physx::PxFoundation* foundation = nullptr;
        physx::PxDefaultAllocator allocator;
        physx::PxDefaultErrorCallback errorCallback;

    private:
        int _sceneCounter = 0;

        // 물리 공유 자원
        std::vector<physx::PxShape*> _shapes;
        std::unordered_map<std::string, physx::PxMaterial*> _materials;
    };
}

