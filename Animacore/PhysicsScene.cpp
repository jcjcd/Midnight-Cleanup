#include "pch.h"
#include "PhysicsScene.h"

#include "Scene.h"
#include "Entity.h"
#include "EnumHelper.h"
#include "CoreComponents.h"
#include "RenderComponents.h"
#include "CoreSystemEvents.h"
#include "CoreTagsAndLayers.h"
#include "CorePhysicsComponents.h"

#include <../Animavision/Mesh.h>

#include "CollisionCallback.h"


core::PhysicsScene::PhysicsScene(Scene& scene)
	: _scene(&scene)
{
	using namespace physx;

	_pxScene = _resources.SceneInitialize(_scene);
	_ctManager = PxCreateControllerManager(*_pxScene);
}

core::PhysicsScene::~PhysicsScene()
{
	using namespace physx;

	Clear();

	delete _pxScene->getSimulationEventCallback();

	_ctManager->release();
	_pxScene->release();

	_resources.SceneFinalize();
}

void core::PhysicsScene::Update(float tick)
{
	using namespace physx;

	// 시뮬레이션 업데이트
	_pxScene->simulate(tick);
	_pxScene->fetchResults(true);

	// 실제 씬 적용
	sceneFetch();
}

void core::PhysicsScene::CreatePhysicsActor(entt::entity handle, entt::registry& registry)
{
	using namespace physx;
	auto entity = Entity(handle, registry);
	auto& physics = _resources.physics;

	// 필수 컴포넌트 검사
	if (!entity.HasAllOf<WorldTransform, ColliderCommon>())
		return;

	// 중복 검사
	if (_entityToDynamic.contains(entity) or _entityToStatic.contains(entity))
		return;

	const auto& collider = entity.Get<ColliderCommon>();
	const auto& world = entity.Get<WorldTransform>();

	// 물리 Material 생성
	PxMaterial* material = _resources.GetMaterial(collider.materialName);

	if (!material)
	{
		LOG_ERROR(*_scene, "{} : \"{}\" Invalid Physic Material", handle, collider.materialName);
		return;
	}

	// 쉐이프 오프셋 설정
	PxTransform centerPos = PxTransform(
		Convert<PxVec3>(collider.center));

	// 캐릭터 컨트롤러 생성
	if (entity.HasAllOf<CharacterController>())
	{
		auto& controllerComponent = entity.Get<CharacterController>();
		auto pxWorldPos = PxVec3T<double>(
			world.position.x,
			world.position.y,
			world.position.z);

		if (auto* controller = createController(material, controllerComponent, pxWorldPos))
		{
			_entityToCharacterIndex.emplace(entity, _controllerCount++);
			controller->setUserData(new entt::entity(entity.GetHandle()));

			// 오프셋 설정, todo: 오프셋이 제대로 작동하지 않음
			PxShape* shape = nullptr;
			controller->getActor()->userData = new entt::entity(entity.GetHandle());
			controller->getActor()->getShapes(&shape, 1);
			//shape->setLocalPose(centerPos);

			controllerComponent.mass = controller->getActor()->getMass();
		}
		else
		{
			if (controllerComponent.radius <= 0.f or controllerComponent.height <= 0.f)
				LOG_ERROR(*_scene, "{} : Failed to create Character Controller (radius, height > 0)", entity);
			else if (controllerComponent.height < controllerComponent.stepOffset)
				LOG_ERROR(*_scene, "{} : Failed to create Character Controller (stepOffset < height)", entity);
			else if (controllerComponent.skinWidth <= 0.f)
				LOG_ERROR(*_scene, "{} : Failed to create Character Controller (skinWidth > 0.f)", entity);
			else
				LOG_ERROR(*_scene, "{} : Failed to create Character Controller", entity);
		}

		return;
	}

	// 쉐이프 생성
	PxShape* shape = createShape(entity, material);
	if (!shape)
		return;

	shape->setContactOffset(collider.contactOffset);

	shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, !collider.isTrigger);
	shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, collider.isTrigger);

	auto pose = shape->getLocalPose() * centerPos;

	shape->setLocalPose(pose);

	// 액터 생성
	PxActor* actor = createActor(entity, shape);

	if (!actor)
		return;

	// 레이어(필터) 설정
	if (!setFilter(entity, shape, actor))
		return;

	// userData 할당
	actor->userData = new entt::entity(entity.GetHandle());

	// 매핑 테이블에 액터 추가 및 물리 씬에 추가
	_pxScene->addActor(*actor);
}

void core::PhysicsScene::DestroyPhysicsActor(entt::entity handle, entt::registry& registry)
{
	using namespace physx;

	// 엔티티와 매핑된 액터 찾기
	if (const auto dIt = _entityToDynamic.find(handle); dIt != _entityToDynamic.end())
	{
		// 액터를 물리 씬에서 제거
		PxRigidDynamic* actor = dIt->second;
		_pxScene->removeActor(*actor);

		// 액터 메모리 해제
		if (actor->userData)
			delete static_cast<entt::entity*>(actor->userData);

		actor->release();

		// 매핑에서 엔티티 제거
		_entityToDynamic.erase(dIt);
	}
	// 엔티티와 매핑된 액터 찾기
	else if (const auto sIt = _entityToStatic.find(handle); sIt != _entityToStatic.end())
	{
		// 액터를 물리 씬에서 제거
		PxRigidStatic* actor = sIt->second;
		_pxScene->removeActor(*actor);

		// 액터 메모리 해제
		if (actor->userData)
			delete static_cast<entt::entity*>(actor->userData);

		actor->release();

		// 매핑에서 엔티티 제거
		_entityToStatic.erase(sIt);
	}
	// 캐릭터 컨트롤러
	else if (const auto cIt = _entityToCharacterIndex.find(handle);
		cIt != _entityToCharacterIndex.end())
	{
		auto* controller = _ctManager->getController(_entityToCharacterIndex[handle]);
		// 액터 메모리 해제
		if (auto* data = static_cast<entt::entity*>(controller->getUserData()))
		{
			_entityToCharacterIndex.erase(*data);
			delete data;
		}

		if (auto* data = static_cast<entt::entity*>(controller->getActor()->userData))
			delete data;

		controller->release();
	}

}

void core::PhysicsScene::SetGravity(Vector3 gravity) const
{
	using namespace physx;

	_pxScene->setGravity(Convert<PxVec3>(gravity));
}

Vector3 core::PhysicsScene::GetGravity() const
{
	return Convert<Vector3>(_pxScene->getGravity());
}

void core::PhysicsScene::UseGravity(entt::entity entity, bool useGravity)
{
	using namespace physx;

	if (!_entityToDynamic.contains(entity))
		return;

	_scene->GetRegistry()->get<Rigidbody>(entity).useGravity = useGravity;
	_entityToDynamic[entity]->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !useGravity);
}

void core::PhysicsScene::AddForce(entt::entity entity, Vector3 force, physics::ForceMode mode)
{
	using namespace physx;

	if (!_entityToDynamic.contains(entity))
		return;

	_entityToDynamic[entity]->addForce(Convert<PxVec3>(force), Convert<PxForceMode::Enum>(mode));
}

void core::PhysicsScene::AddTorque(entt::entity entity, Vector3 torque, physics::ForceMode mode)
{
	using namespace physx;

	if (!_entityToDynamic.contains(entity))
		return;

	_entityToDynamic[entity]->addTorque(Convert<PxVec3>(torque), Convert<PxForceMode::Enum>(mode));
}

void core::PhysicsScene::MoveCharacter(entt::entity entity, CharacterController& controller, Vector3 disp, uint32_t targetLayer, float elapsedTime, float minDist)
{
	using namespace physx;

	// 엔티티가 유효한지 확인
	if (!_entityToCharacterIndex.contains(entity))
		return;

	// 캐릭터 컨트롤러 가져오기
	PxController* pxController = _ctManager->getController(_entityToCharacterIndex[entity]);
	if (!pxController)
		return;

	// 변위를 PxVec3로 변환
	PxVec3 displacement(disp.x, disp.y, disp.z);

	PxFilterData characterFilterData;
	characterFilterData.word0 = targetLayer;

	// 필터 생성 todo : 특정 레이어 충돌 처리
	PxControllerFilters filters; // 필터 데이터로 초기화
	filters.mFilterData = &characterFilterData;

	// 캐릭터 이동
	PxControllerCollisionFlags collisionFlags = pxController->move(displacement, minDist, elapsedTime, filters);

	PxControllerState state;
	pxController->getState(state);

	if (collisionFlags & PxControllerCollisionFlag::eCOLLISION_DOWN)
		controller.isGrounded = true;
	else
		controller.isGrounded = false;

	// 충돌 오브젝트 할당
	if (state.touchedActor)
	{
		if (collisionFlags & PxControllerCollisionFlag::eCOLLISION_SIDES)
			controller.collisionFlags = CharacterController::CollisionFlags::Sides;
		else if (collisionFlags & PxControllerCollisionFlag::eCOLLISION_UP)
			controller.collisionFlags = CharacterController::CollisionFlags::Above;
		else
			controller.collisionFlags = CharacterController::CollisionFlags::Below;
	}
	else
	{
		controller.collisionFlags = CharacterController::CollisionFlags::None;
	}
}

bool core::PhysicsScene::LoadMaterial(const std::filesystem::path& path)
{
	if (_resources.LoadMaterial(path))
		return true;

	return false;
}

bool core::PhysicsScene::Raycast(Vector3 origin, Vector3 direction, std::vector<physics::RaycastHit>& hits, uint32_t maxHit, float maxDistance, uint32_t layerMask, physics::QueryTriggerInteraction queryTriggerInteraction)
{
	using namespace physx;

	PxVec3 pxOrigin = Convert<PxVec3>(origin);
	PxVec3 pxDirection = Convert<PxVec3>(direction);

	// 레이캐스트 버퍼 및 충돌 레이어 지정
	PxQueryFilterData filterData;
	filterData.data.word0 = layerMask;

	constexpr PxU32 bufferSize = 256;
	PxRaycastHit hitBuffer[bufferSize];
	PxRaycastBuffer buf(hitBuffer, bufferSize);

	// QueryTriggerInteraction 설정 반영
	switch (queryTriggerInteraction)
	{
	case physics::QueryTriggerInteraction::Ignore:
		filterData.flags = PxQueryFlag::eDYNAMIC;
		break;
	case physics::QueryTriggerInteraction::Collide:
		filterData.flags = PxQueryFlag::eDYNAMIC | PxQueryFlag::eSTATIC;
		break;
	}

	PxHitFlags hitFlag = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;

	if (maxHit == 1)
		hitFlag |= PxHitFlag::eANY_HIT;

	// Raycast 실행
	bool status = _pxScene->raycast(pxOrigin, pxDirection, maxDistance,
		buf, hitFlag, filterData);

	uint32_t nbAnyHits = buf.getNbAnyHits();
	if (status && nbAnyHits)
	{
		if (maxHit < nbAnyHits)
			nbAnyHits = maxHit;

		hits.resize(nbAnyHits);

		for (uint32_t i = 0; i < nbAnyHits; ++i)
		{
			auto&& anyHit = buf.getAnyHit(i);

			hits[i].entity = *static_cast<entt::entity*>(anyHit.actor->userData);
			hits[i].point = Convert<Vector3>(anyHit.position);
			hits[i].normal = Convert<Vector3>(anyHit.normal);
			hits[i].distance = anyHit.distance;
		}
		return true;
	}

	hits.clear();

	return false;
}

bool core::PhysicsScene::Raycast(const physics::Ray& ray, std::vector<physics::RaycastHit>& hitInfo, uint32_t maxHit, float maxDistance, uint32_t layerMask, physics::QueryTriggerInteraction queryTriggerInteraction)
{
	return Raycast(ray.origin, ray.direction, hitInfo, maxHit, maxDistance, layerMask, queryTriggerInteraction);
}

bool core::PhysicsScene::Boxcast(Vector3 center, Vector3 halfExtents, Vector3 direction, std::vector<physics::RaycastHit>& hits, Quaternion orientation, float maxDistance, uint32_t layerMask, physics::QueryTriggerInteraction queryTriggerInteraction)
{
	using namespace physx;

	// 상자의 형태와 초기 위치 및 회전을 설정
	PxBoxGeometry box(halfExtents.x, halfExtents.y, halfExtents.z);
	PxTransform pose(Convert<PxVec3>(center), Convert<PxQuat>(orientation));
	PxVec3 pxDirection(Convert<PxVec3>(direction));

	// 스윕 버퍼 및 필터 데이터 설정 (기본: layer::IgnoreRaycast 제외 모든 레이어)
	PxSweepBuffer hit;
	PxQueryFilterData filterData;
	filterData.data.word0 = layerMask;

	// QueryTriggerInteraction 설정을 반영
	switch (queryTriggerInteraction)
	{
	case physics::QueryTriggerInteraction::Ignore:
		filterData.flags |= PxQueryFlag::eDYNAMIC;
		break;
	case physics::QueryTriggerInteraction::Collide:
		filterData.flags |= PxQueryFlag::eDYNAMIC | PxQueryFlag::eSTATIC;
		break;
	}

	// Sweep 실행
	bool status = _pxScene->sweep(box, pose, pxDirection, maxDistance, hit, PxHitFlag::eDEFAULT, filterData);

	if (status && hit.hasBlock) // 적어도 하나의 충돌이 발견된 경우
	{
		uint32_t nbTouches = hit.getNbTouches();
		hits.resize(nbTouches);

		for (uint32_t i = 0; i < nbTouches; ++i)
		{
			auto&& touch = hit.getTouch(i);
			hits[i].entity = *static_cast<entt::entity*>(hit.block.actor->userData);	// 충돌 객체
			hits[i].point = Convert<Vector3>(touch.position);		// 충돌 위치
			hits[i].normal = Convert<Vector3>(touch.normal);		// 충돌 위치의 법선
			hits[i].distance = touch.distance;					// 충돌까지의 거리
		}
		return true;
	}

	return false;
}

void core::PhysicsScene::GetStaticPoly(std::vector<float>& vertices, std::vector<int>& indices, uint32_t layerMask)
{
	using namespace physx;

	for (auto& staticActor : _entityToStatic | std::views::values) {
		// PxShape 가져오기
		PxRigidActor* actor = staticActor;
		uint32_t shapeCount = actor->getNbShapes();
		std::vector<PxShape*> shapes(shapeCount);
		actor->getShapes(shapes.data(), shapeCount);

		// 각 쉐이프의 버텍스/인덱스 추출
		for (PxShape* shape : shapes) {

			// 레이어 필터링을 위해 필터 데이터 가져오기
			PxFilterData filterData = shape->getSimulationFilterData();
			uint32_t shapeLayer = filterData.word0;

			// 지정된 레이어와 일치하지 않으면 스킵
			if ((shapeLayer & layerMask) == 0)
				continue;

			// PxGeometryHolder를 사용하여 지오메트리 유형을 처리
			PxGeometryHolder geomHolder = shape->getGeometry();

			// eBOX 처리
			if (geomHolder.getType() == PxGeometryType::eBOX) {
				const PxBoxGeometry& boxGeom = geomHolder.box();
				const PxVec3& halfExtents = boxGeom.halfExtents;

				// 박스의 8개의 꼭짓점을 추가
				const PxVec3 boxVertices[8] = {
					{-halfExtents.x, -halfExtents.y, -halfExtents.z},
					{ halfExtents.x, -halfExtents.y, -halfExtents.z},
					{ halfExtents.x,  halfExtents.y, -halfExtents.z},
					{-halfExtents.x,  halfExtents.y, -halfExtents.z},
					{-halfExtents.x, -halfExtents.y,  halfExtents.z},
					{ halfExtents.x, -halfExtents.y,  halfExtents.z},
					{ halfExtents.x,  halfExtents.y,  halfExtents.z},
					{-halfExtents.x,  halfExtents.y,  halfExtents.z}
				};

				// 박스의 인덱스 (삼각형 목록)
				const int boxIndices[36] = {
					0, 1, 2, 0, 2, 3, // Bottom face
					4, 6, 5, 4, 7, 6, // Top face
					0, 5, 1, 0, 4, 5, // Front face
					1, 6, 2, 1, 5, 6, // Right face
					2, 7, 3, 2, 6, 7, // Back face
					3, 4, 0, 3, 7, 4  // Left face
				};

				// 박스의 버텍스 추가
				for (int i = 0; i < 8; ++i) {
					vertices.push_back(boxVertices[i].x);
					vertices.push_back(boxVertices[i].y);
					vertices.push_back(boxVertices[i].z);
				}

				// 박스의 인덱스 추가
				for (int i = 0; i < 36; ++i) {
					indices.push_back(boxIndices[i]);
				}
			}
			if (geomHolder.getType() == PxGeometryType::eCONVEXMESH) {
				// Convex Mesh 처리
				const PxConvexMeshGeometry& convexGeom = geomHolder.convexMesh();
				const PxConvexMesh* convexMesh = convexGeom.convexMesh;
				const PxU32 nbVerts = convexMesh->getNbVertices();
				const PxVec3* verts = convexMesh->getVertices();
				const PxU32 nbPolygons = convexMesh->getNbPolygons();

				// Convex 메쉬의 버텍스를 벡터에 추가
				for (PxU32 i = 0; i < nbVerts; ++i) {
					vertices.push_back(verts[i].x);
					vertices.push_back(verts[i].y);
					vertices.push_back(verts[i].z);
				}

				for (PxU32 i = 0; i < nbPolygons; ++i) {
					PxHullPolygon polygon;
					convexMesh->getPolygonData(i, polygon);

					auto indexBuffer = convexMesh->getIndexBuffer() + polygon.mIndexBase;
					for (PxU32 j = 0; j < polygon.mNbVerts; ++j) {
						indices.push_back(indexBuffer[j]);
					}
				}

				// Convex Mesh는 일반적으로 인덱스 정보가 없으므로 별도로 처리할 필요 없음
			}
			else if (geomHolder.getType() == PxGeometryType::eTRIANGLEMESH) {
				// Triangle Mesh 처리
				const PxTriangleMeshGeometry& triMeshGeom = geomHolder.triangleMesh();
				const PxTriangleMesh* triMesh = triMeshGeom.triangleMesh;
				const PxU32 nbVerts = triMesh->getNbVertices();
				const PxVec3* verts = triMesh->getVertices();
				const PxU32 nbTris = triMesh->getNbTriangles();
				const void* tris = triMesh->getTriangles();

				// Triangle 메쉬의 버텍스를 벡터에 추가
				for (PxU32 i = 0; i < nbVerts; ++i) {
					vertices.push_back(verts[i].x);
					vertices.push_back(verts[i].y);
					vertices.push_back(verts[i].z);
				}

				// Triangle 메쉬의 인덱스를 벡터에 추가 (16-bit 또는 32-bit 인덱스일 수 있음)
				if (triMesh->getTriangleMeshFlags() & PxTriangleMeshFlag::e16_BIT_INDICES) {
					const PxU16* indices16 = static_cast<const PxU16*>(tris);
					for (PxU32 i = 0; i < nbTris * 3; ++i) {
						indices.push_back(indices16[i]);
					}
				}
				else {
					const PxU32* indices32 = static_cast<const PxU32*>(tris);
					for (PxU32 i = 0; i < nbTris * 3; ++i) {
						indices.push_back(indices32[i]);
					}
				}
			}
		}
	}
}

void core::PhysicsScene::SetLinearVelocity(entt::entity entity, const Vector3& velocity)
{
	using namespace physx;

	if (_entityToDynamic.contains(entity))
	{
		_entityToDynamic[entity]->setLinearVelocity(Convert<PxVec3>(velocity));
	}
	else
	{
		LOG_ERROR(*_scene, "{} : Entity does not contain Rigidbody", entity);
	}
}

void core::PhysicsScene::sceneFetch()
{
	using namespace physx;

	auto registry = _scene->GetRegistry();
	auto dispatcher = _scene->GetDispatcher();

	// 동적 액터 업데이트
	for (auto&& [entity, actor] : _entityToDynamic)
	{
		if (actor->isSleeping())
			continue;

		// actor 정보 추출
		PxTransform pxTransform = actor->getGlobalPose();
		const PxVec3 linearVelocity = actor->getLinearVelocity();
		const PxVec3 angularVelocity = actor->getAngularVelocity();

		auto& rigidbody = registry->get<Rigidbody>(entity);
		auto& world = registry->get<WorldTransform>(entity);

		// 컴포넌트 업데이트
		rigidbody.velocity = Convert<Vector3>(linearVelocity);
		rigidbody.angularVelocity = Convert<Vector3>(angularVelocity);

		if (!static_cast<bool>(rigidbody.constraints & Rigidbody::Constraints::FreezePosition))
		{
			// 포지션 업데이트
			world.position = Convert<Vector3>(pxTransform.p);
		}
		if (!static_cast<bool>(rigidbody.constraints & Rigidbody::Constraints::FreezeRotation))
		{
			// 로테이션 업데이트
			world.rotation = Convert<Quaternion>(pxTransform.q);
		}

		dispatcher->trigger<OnUpdateTransform>({ entity, registry });
	}

	// 컨트롤러 업데이트
	auto controllerNumbers = _ctManager->getNbControllers();
	for (uint32_t i = 0; i < controllerNumbers; ++i)
	{
		auto pxController = _ctManager->getController(i);

		if (pxController->getActor()->isSleeping())
			continue;

		entt::entity entity = *static_cast<entt::entity*>(pxController->getUserData());

		auto& world = registry->get<WorldTransform>(entity);
		auto pxPosition = pxController->getPosition();

		world.position = Convert<Vector3>(pxPosition);
		dispatcher->trigger<OnUpdateTransform>({ entity, registry });
	}
}

physx::PxShape* core::PhysicsScene::createShape(Entity entity, physx::PxMaterial* material)
{
	using namespace physx;

	PxShape* shape = nullptr;
	auto physics = _resources.physics;
	auto& world = entity.Get<WorldTransform>();

	// Collider 할당
	if (entity.HasAllOf<BoxCollider>())
	{
		const auto& box = entity.Get<BoxCollider>();
		shape = physics->createShape(PxBoxGeometry(
			box.size.x * world.scale.x / 2,
			box.size.y * world.scale.y / 2,
			box.size.z * world.scale.z / 2), *material, true);
	}
	else if (entity.HasAllOf<SphereCollider>())
	{
		const auto& sphere = entity.Get<SphereCollider>();
		auto maxScale = (std::max)({ world.scale.x / 2, world.scale.y / 2, world.scale.z / 2 });
		shape = physics->createShape(PxSphereGeometry(
			sphere.radius * maxScale), *material, true);
	}
	else if (entity.HasAllOf<CapsuleCollider>())
	{
		const auto& capsule = entity.Get<CapsuleCollider>();

		shape = physics->createShape(PxCapsuleGeometry(
			capsule.radius * (std::max)(world.scale.x / 2, world.scale.z / 2),
			capsule.height * world.scale.y / 2), *material, true);

		PxTransform localPose(PxIdentity);

		// direction 값에 따라 회전 설정
		switch (capsule.direction)
		{
		case CapsuleCollider::X: // y-axis (default orientation)
			localPose.q = PxQuat(PxIdentity);
			break;
		case CapsuleCollider::Y:
			localPose.q = PxQuat(PxHalfPi, PxVec3(0, 0, 1));
			break;
		case CapsuleCollider::Z: // z-axis
			localPose.q = PxQuat(PxHalfPi, PxVec3(1, 0, 0));
			break;
		default:
			localPose.q = PxQuat(PxIdentity); // Default orientation
			break;
		}

		shape->setLocalPose(localPose);
	}
	else if (entity.HasAllOf<MeshCollider>())
	{
		if (entity.HasAnyOf<MeshRenderer>())
		{
			const auto& meshCollider = entity.Get<MeshCollider>();
			const auto& meshRenderer = entity.Get<MeshRenderer>();

			if (!meshRenderer.mesh)
				LOG_ERROR(*_scene, "{} : No mesh \"{}\" to make mesh collider",
					entity, meshRenderer.meshString);
			else if (meshCollider.convex)
				shape = createConvexMesh(meshRenderer, meshCollider, world.scale, material);
			else
				shape = createTriangleMesh(meshRenderer, meshCollider, world.scale, material);
		}
		else
		{
			LOG_ERROR(*_scene, "{} : MeshCollider must contain MeshRenderer", entity);
		}
	}
	else
	{
		LOG_ERROR(*_scene, "{} : Entity must contain one collider", entity);
	}

	return shape;
}

physx::PxActor* core::PhysicsScene::createActor(Entity entity, physx::PxShape* shape)
{
	using namespace physx;

	PxActor* actor = nullptr;
	auto physics = _resources.physics;
	auto world = entity.Get<WorldTransform>();

	if (entity.HasAllOf<Rigidbody>())
	{
		const auto& rigid = entity.Get<Rigidbody>();
		const auto* meshCollider = entity.TryGet<MeshCollider>();

		// 동적 액터 생성
		PxRigidDynamic* dynamicActor = physics->createRigidDynamic(
			PxTransform(
				Convert<PxVec3>(world.position),
				Convert<PxQuat>(world.rotation)
			)
		);

		if (meshCollider && !meshCollider->convex && !rigid.isKinematic)
		{
			LOG_ERROR(*_scene, "{} : Attaching a non-convex mesh shape is not supported for non-kinematic instances.", entity);
			dynamicActor->release();
			return nullptr;
		}

		dynamicActor->attachShape(*shape);
		_entityToDynamic[entity] = dynamicActor;

		UpdateRigidbody(entity, rigid, false);
		actor = dynamicActor;
	}
	else
	{
		// 정적 액터 생성
		PxRigidStatic* staticActor = physics->createRigidStatic(
			PxTransform(
				Convert<PxVec3>(world.position),
				Convert<PxQuat>(world.rotation)
			)
		);

		staticActor->attachShape(*shape);

		actor = staticActor;
		_entityToStatic[entity] = staticActor;
	}

	if (!actor)
	{
		LOG_ERROR(*_scene, "{} : Failed to assign Actor", entity);
	}

	return actor;
}

bool core::PhysicsScene::setFilter(Entity entity, physx::PxShape* shape, physx::PxActor* actor)
{
	// 레이어 컴포넌트를 사용하여 레이어 마스크를 확인
	if (entity.HasAllOf<Layer>())
	{
		const auto& layerComponent = entity.Get<Layer>();
		uint32_t layerMask = layerComponent.mask; // 레이어 마스크 가져오기

		uint32_t collisionGroup = layerMask;  // 충돌 그룹: 레이어 마스크를 그대로 사용
		uint32_t collisionMask = 0;

		// 모든 가능한 레이어와의 충돌 여부 확인 후 마스크 설정
		for (uint32_t i = 0; i < physics::collisionMatrix.size(); ++i)
		{
			if (layerMask & (1 << i))
			{
				collisionMask |= physics::collisionMatrix[i];
			}
		}

		physx::PxFilterData filterData;

		filterData.word0 = collisionGroup;  // 충돌 그룹 설정
		shape->setQueryFilterData(filterData);

		filterData.word1 = collisionMask;  // 충돌 마스크 설정
		shape->setSimulationFilterData(filterData);
	}
	else
	{
		// Layer 컴포넌트가 없는 경우, 기본 필터 데이터 설정
		physx::PxFilterData filterData;

		filterData.word0 = 1u;            // 기본 충돌 그룹 (layer::Default)
		filterData.word1 = 0xFFFFFFFF;    // 모든 충돌을 허용

		shape->setQueryFilterData(filterData);
		shape->setSimulationFilterData(filterData);
	}

	shape->release();

	return true;
}

void core::PhysicsScene::UpdateTransform(entt::entity entity, const WorldTransform& world)
{
	using namespace physx;

	if (_entityToStatic.contains(entity))
	{
		PxTransform transform{ Convert<PxVec3>(world.position), Convert<PxQuat>(world.rotation) };
		_entityToStatic[entity]->setGlobalPose(transform);
	}
	else if (_entityToDynamic.contains(entity))
	{
		if (_scene->GetRegistry()->get<Rigidbody>(entity).isDisabled)
			return;

		PxTransform transform{ Convert<PxVec3>(world.position), Convert<PxQuat>(world.rotation) };
		_entityToDynamic[entity]->setGlobalPose(transform);
	}
	else if (_entityToCharacterIndex.contains(entity))
	{
		auto index = _entityToCharacterIndex[entity];
		_ctManager->getController(index)->setPosition(Convert<PxExtendedVec3>(world.position));
	}
}

void core::PhysicsScene::UpdateTransform(const entt::entity entity)
{
	using namespace physx;

	auto& world = _scene->GetRegistry()->get<core::WorldTransform>(entity);

	if (_entityToStatic.contains(entity))
	{
		PxTransform transform{ Convert<PxVec3>(world.position), Convert<PxQuat>(world.rotation) };
		_entityToStatic[entity]->setGlobalPose(transform);
	}
	else if (_entityToDynamic.contains(entity))
	{
		PxTransform transform{ Convert<PxVec3>(world.position), Convert<PxQuat>(world.rotation) };
		_entityToDynamic[entity]->setGlobalPose(transform);
	}
	else if (_entityToCharacterIndex.contains(entity))
	{
		auto index = _entityToCharacterIndex[entity];
		_ctManager->getController(index)->setPosition(Convert<PxExtendedVec3>(world.position));
	}
}

void core::PhysicsScene::UpdateRigidbody(entt::entity entity, const Rigidbody& rigid, bool wakeUp)
{
	using namespace physx;

	auto actor = _entityToDynamic[entity];

	if (rigid.isDisabled)
	{
		PxU32 shapeCount = actor->getNbShapes();
		std::vector<PxShape*> shapes(shapeCount);
		actor->getShapes(shapes.data(), shapeCount);
		for (auto* shape : shapes)
		{
			shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
			shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		}
	}
	else if (actor->getActorFlags() & PxActorFlag::eDISABLE_SIMULATION)
	{
		PxU32 shapeCount = actor->getNbShapes();
		std::vector<PxShape*> shapes(shapeCount);
		actor->getShapes(shapes.data(), shapeCount);
		for (auto* shape : shapes)
		{
			shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
			shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);
		}
	}

	actor->setActorFlag(PxActorFlag::eDISABLE_SIMULATION, rigid.isDisabled);
	actor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !rigid.useGravity);
	actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, rigid.isKinematic);

	if (std::abs(actor->getLinearDamping() - rigid.drag) > DBL_EPSILON)
		actor->setLinearDamping(rigid.drag);
	if (std::abs(actor->getAngularDamping() - rigid.angularDrag) > DBL_EPSILON)
		actor->setAngularDamping(rigid.angularDrag);

	switch (rigid.interpolation)
	{
	case Rigidbody::Interpolation::None:
		if (rigid.isKinematic)
			actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, false);  // CCD 활성화 (보간 없음)
		else
			actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);  // CCD 활성화 (보간 없음)
		break;

	case Rigidbody::Interpolation::Interpolate:
		actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_POSE_INTEGRATION_PREVIEW, true);  // 자세 보간 처리
		actor->setRigidBodyFlag(PxRigidBodyFlag::eFORCE_KINE_KINE_NOTIFICATIONS, true);    // 충돌 시 보간
		break;

	case Rigidbody::Interpolation::Extrapolate:
		actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_SPECULATIVE_CCD, true);  // 예측된 충돌 처리 (외삽)
		break;
	}


	auto setLockFlag = [&](Rigidbody::Constraints flag, PxRigidDynamicLockFlag::Enum pxFlag)
		{
			if ((rigid.constraints & flag) != Rigidbody::Constraints::None)
				actor->setRigidDynamicLockFlag(pxFlag, true);
			else
				actor->setRigidDynamicLockFlag(pxFlag, false);
		};

	setLockFlag(Rigidbody::Constraints::FreezePositionX, PxRigidDynamicLockFlag::eLOCK_LINEAR_X);
	setLockFlag(Rigidbody::Constraints::FreezePositionY, PxRigidDynamicLockFlag::eLOCK_LINEAR_Y);
	setLockFlag(Rigidbody::Constraints::FreezePositionZ, PxRigidDynamicLockFlag::eLOCK_LINEAR_Z);
	setLockFlag(Rigidbody::Constraints::FreezeRotationX, PxRigidDynamicLockFlag::eLOCK_ANGULAR_X);
	setLockFlag(Rigidbody::Constraints::FreezeRotationY, PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y);
	setLockFlag(Rigidbody::Constraints::FreezeRotationZ, PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z);

	if (!rigid.isDisabled & wakeUp)
	{
		actor->wakeUp();
	}
}

void core::PhysicsScene::UpdateCollider(entt::entity entity, const ColliderCommon& collider)
{
	using namespace physx;
}

physx::PxShape* core::PhysicsScene::createConvexMesh(const MeshRenderer& meshData, const MeshCollider& collider, const Vector3& worldScale, physx::PxMaterial* material)
{
	using namespace physx;

	auto& physics = _resources.physics;

	// 물리 엔진에서 사용할 메쉬 데이터를 저장할 버퍼를 준비합니다.
	std::vector<PxVec3> vertices;

	// 메쉬 데이터를 가져옵니다.
	auto& mesh = meshData.mesh;

	// 메쉬의 정점 데이터를 변환하여 PhysX 포맷으로 저장합니다.
	for (const auto& vertex : mesh->vertices)
		vertices.emplace_back(vertex.x, vertex.y, vertex.z);

	// PhysX 메쉬 디스크립터를 생성합니다.
	PxConvexMeshDesc convexDesc;
	convexDesc.points.count = static_cast<PxU32>(vertices.size());
	convexDesc.points.stride = sizeof(PxVec3);
	convexDesc.points.data = vertices.data();
	convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX | PxConvexFlag::eQUANTIZE_INPUT;


	// PhysX 쿡킹 파라미터를 설정합니다.
	PxCookingParams params(physics->getTolerancesScale());
	params.midphaseDesc.setToDefault(PxMeshMidPhase::eBVH34);

	if (collider.cookingOptions & MeshCollider::MeshColliderCookingOptions::CookForFasterSimulation)
		params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;
	if (collider.cookingOptions & MeshCollider::MeshColliderCookingOptions::DisableMeshCleaning)
		params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;
	if (collider.cookingOptions & MeshCollider::MeshColliderCookingOptions::WeldColocatedVertices)
		params.meshPreprocessParams |= PxMeshPreprocessingFlag::eWELD_VERTICES;
	if (collider.cookingOptions & MeshCollider::MeshColliderCookingOptions::UseLegacyMidphase)
		params.midphaseDesc = PxMeshMidPhase::eBVH33;
	if (collider.cookingOptions & MeshCollider::MeshColliderCookingOptions::BuildGPUData)
	{
		params.buildGPUData = true;
		convexDesc.vertexLimit = 64;
	}


	// 요리된 데이터를 저장할 스트림
	PxDefaultMemoryOutputStream buf;
	PxConvexMeshCookingResult::Enum result;

	// 삼각형 메쉬를 요리합니다.
	if (!PxCookConvexMesh(params, convexDesc, buf, &result))
		return nullptr;

	// 메모리에서 요리된 데이터를 읽어들입니다.
	PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
	PxConvexMesh* convexMesh = physics->createConvexMesh(input);

	if (!convexMesh)
		return nullptr;

	// 스케일 적용
	PxMeshScale meshScale;
	meshScale.scale = { worldScale.x, worldScale.y, worldScale.z };

	// 메쉬 기하를 생성합니다.
	PxConvexMeshGeometry meshGeometry(convexMesh, meshScale);

	// 콜라이더 쉐이프를 생성합니다.
	return physics->createShape(meshGeometry, *material, true);
}

physx::PxShape* core::PhysicsScene::createTriangleMesh(const MeshRenderer& meshData, const MeshCollider& collider, const Vector3& worldScale, physx::PxMaterial* material)
{
	using namespace physx;

	auto& physics = _resources.physics;

	// 물리 엔진에서 사용할 메쉬 데이터를 저장할 버퍼를 준비합니다.
	std::vector<PxVec3> vertices;
	std::vector<PxU32> indices;

	// 메쉬 데이터를 가져옵니다.
	auto& mesh = meshData.mesh;

	// 메쉬의 정점 데이터를 변환하여 PhysX 포맷으로 저장합니다.
	vertices.reserve(mesh->vertices.size());
	for (const auto& vertex : mesh->vertices)
		vertices.emplace_back(vertex.x, vertex.y, vertex.z);

	// 메쉬의 인덱스 데이터를 변환하여 PhysX 포맷으로 저장합니다.
	indices.reserve(mesh->indices.size());
	for (size_t i = 0; i < mesh->indices.size(); i += 3)
	{
		indices.emplace_back(mesh->indices[i]);
		indices.emplace_back(mesh->indices[i + 1]);
		indices.emplace_back(mesh->indices[i + 2]);
	}

	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = static_cast<PxU32>(vertices.size());
	meshDesc.points.stride = sizeof(PxVec3);
	meshDesc.points.data = vertices.data();
	meshDesc.triangles.count = static_cast<PxU32>(indices.size() / 3);
	meshDesc.triangles.stride = 3 * sizeof(PxU32);
	meshDesc.triangles.data = indices.data();

	// PhysX 쿡킹 파라미터를 설정합니다.
	PxCookingParams params(physics->getTolerancesScale());

	if (collider.cookingOptions & MeshCollider::MeshColliderCookingOptions::CookForFasterSimulation)
		params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;
	if (collider.cookingOptions & MeshCollider::MeshColliderCookingOptions::DisableMeshCleaning)
		params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;
	if (collider.cookingOptions & MeshCollider::MeshColliderCookingOptions::WeldColocatedVertices)
		params.meshPreprocessParams |= PxMeshPreprocessingFlag::eWELD_VERTICES;
	if (collider.cookingOptions & MeshCollider::MeshColliderCookingOptions::UseLegacyMidphase)
		params.midphaseDesc = PxMeshMidPhase::eBVH33;

	PxDefaultMemoryOutputStream writeBuffer;
	PxTriangleMeshCookingResult::Enum result;

	if (!PxCookTriangleMesh(params, meshDesc, writeBuffer, &result))
		return nullptr;

	PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
	PxTriangleMesh* triangleMesh = physics->createTriangleMesh(readBuffer);

	if (!triangleMesh)
		return nullptr;

	// 스케일 적용
	PxMeshScale meshScale;
	meshScale.scale = { worldScale.x, worldScale.y, worldScale.z };

	// 메쉬 기하를 생성합니다.
	PxTriangleMeshGeometry meshGeometry(triangleMesh, meshScale);

	// 콜라이더 쉐이프를 생성합니다.
	return physics->createShape(meshGeometry, *material, true);
}

physx::PxController* core::PhysicsScene::createController(physx::PxMaterial* material, const CharacterController& controller, const physx::PxVec3T<double>& worldPos)
{
	using namespace physx;

	PxCapsuleControllerDesc desc;
	desc.height = controller.height;
	desc.radius = controller.radius;
	desc.position = worldPos;
	desc.material = material;
	desc.stepOffset = controller.stepOffset; // 오를 수 있는 높이
	desc.slopeLimit = cosf(controller.slopeLimit * PxPi / 180.0f); // 오를 수 있는 각도
	desc.contactOffset = controller.skinWidth;
	desc.nonWalkableMode = PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING;
	desc.upDirection = PxVec3(0, 1, 0);
	desc.reportCallback = new DefaultCctHitReport(controller);
	desc.behaviorCallback = nullptr; // 동작 콜백
	desc.volumeGrowth = 1.5f;  // 겹침 복구를 위한 볼륨 증가율
	desc.density = controller.density;      // 컨트롤러의 밀도

	return _ctManager->createController(desc);
}

void core::PhysicsScene::Clear()
{
	using namespace physx;

	// 동적 액터 제거
	for (const auto& actor : std::views::values(_entityToDynamic))
	{
		// 액터를 물리 씬에서 제거
		_pxScene->removeActor(*actor, true);

		// 액터 메모리 해제
		delete static_cast<entt::entity*>(actor->userData);
		actor->release();
	}

	// 정적 액터 제거
	for (const auto& actor : std::views::values(_entityToStatic))
	{
		// 액터를 물리 씬에서 제거
		_pxScene->removeActor(*actor, true);

		// 액터 메모리 해제
		delete static_cast<entt::entity*>(actor->userData);
		actor->release();
	}

	// 컨트롤러 제거
	_controllerCount = 0;
	_ctManager->purgeControllers();

	// 매핑 정보 클리어
	_entityToDynamic.clear();
	_entityToStatic.clear();
	_entityToCharacterIndex.clear();
}
