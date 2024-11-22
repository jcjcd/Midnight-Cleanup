#include "pch.h"
#include "PxResources.h"

#include "Scene.h"
#include "PxUtils.h"
#include "CollisionCallback.h"

#include <fstream>

#include "PhysicsScene.h"

physx::PxScene* core::PxResources::SceneInitialize(Scene* scene)
{
	using namespace physx;

	// PxFoundation 생성
	if (!foundation)
		foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);

	// PxPvd 생성
	if (!pvd)
		pvd = PxCreatePvd(*foundation);

	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);

	// PxPhysics 생성
	if (!physics)
	{
		if (transport && pvd->connect(*transport, PxPvdInstrumentationFlag::eALL))
			physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale(), false, pvd);
		else
			physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale());
	}

	// PxScene 생성
	PxSceneDesc sceneDesc(physics->getTolerancesScale());
	PxDefaultCpuDispatcher* pxDispatcher = PxDefaultCpuDispatcherCreate(4);
	sceneDesc.gravity = PxVec3(0.0f, -9.81f * 2.f, 0.0f);
	sceneDesc.cpuDispatcher = pxDispatcher;
	sceneDesc.filterShader = core::CustomFilterShader;

	auto pxScene = physics->createScene(sceneDesc);
	pxScene->setSimulationEventCallback(new core::CollisionCallback(*scene));

	if (PxPvdSceneClient* pvdClient = pxScene->getScenePvdClient())
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	if (pxScene)
		++_sceneCounter;

	return pxScene;
}

int core::PxResources::SceneFinalize()
{
	--_sceneCounter;
	 
	if (_sceneCounter == 0) {
		if (physics) {
			physics->release();
			physics = nullptr;
		}
		if (pvd) {
			pvd->release();
			pvd = nullptr;
		}
		if (foundation) {
			foundation->release();
			foundation = nullptr;
		}
	}

	return _sceneCounter;
}

physx::PxMaterial* core::PxResources::GetMaterial(const std::string& name)
{
	if (_materials.contains(name))
		return _materials[name];
	else
		return LoadMaterial(name);
}

void core::PxResources::SaveMaterial(const std::filesystem::path& path, const ColliderCommon::PhysicMaterial& material)
{
	std::ofstream os(path, std::ios::binary);
	cereal::BinaryOutputArchive archive(os);

	float dynamicFriction = material.dynamicFriction;
	float staticFriction = material.staticFriction;
	float restitution = material.bounciness;

	archive(dynamicFriction, staticFriction, restitution);
}

core::PxResources::p_mat core::PxResources::ReadMaterial(const std::filesystem::path& path)
{
	std::ifstream is(path, std::ios::binary);
	cereal::BinaryInputArchive archive(is);

	float dynamicFriction, staticFriction, restitution;
	archive(dynamicFriction, staticFriction, restitution);

	return { dynamicFriction, staticFriction, restitution };
}

physx::PxMaterial* core::PxResources::LoadMaterial(const std::filesystem::path& path)
{
	if (!std::filesystem::exists(path) || path.extension() != Scene::PHYSIC_MATERIAL_EXTENSION)
		return nullptr;

	if (_materials.contains(path.string()))
	{
		auto mat = ReadMaterial(path);
		auto pxMat = _materials[path.string()];

		if (mat.staticFriction == pxMat->getStaticFriction()
			&& mat.dynamicFriction == pxMat->getDynamicFriction()
			&& mat.bounciness == pxMat->getRestitution())
			return _materials[path.string()];
	}

	std::ifstream is(path, std::ios::binary);
	cereal::BinaryInputArchive archive(is);

	float dynamicFriction, staticFriction, restitution;
	archive(dynamicFriction, staticFriction, restitution);

	physx::PxMaterial* material = physics->createMaterial(
		staticFriction, dynamicFriction, restitution
	);

	if (material)
		_materials[path.string()] = material;

	return material;
}
