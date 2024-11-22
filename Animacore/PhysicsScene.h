#pragma once

#include "PxUtils.h"
#include "PxResources.h"
#include "CoreTagsAndLayers.h"
#include "CorePhysicsComponents.h"

namespace physics
{
	class ControllerFilters;
}

namespace core
{
	class Scene;
	class Entity;
	struct MeshRenderer;
	struct WorldTransform;
	struct OnUpdateTransform;

	/// \brief
	/// \n 엔티티들의 물리 시뮬레이션을 진행하는 클래스 씬마다 개별적으로 존재함
	///	\n 엔티티가 동작하기 위해 필요한 컴포넌트
	///	\n 필수: Transform, ColliderCommon, ~Collider(Box, Sphere, Capsule, Mesh)
	///	\n 옵션: Rigidbody, Tag, Layer
	///	\n
	///	\n 물리 충돌과 관련된 컴포넌트
	///	\n Tag: 충돌을 처리하는 시스템을 정의하는데 사용함
	///	\n Layer: 특정 레이어 간 충돌 판정 여부를 설정하는데 사용함
	class PhysicsScene
	{
	public:
		PhysicsScene(Scene& scene);
		~PhysicsScene();

		void Update(float tick);

		// 액터 생성
		void CreatePhysicsActor(entt::entity handle, entt::registry& registry);

		// 액터 삭제
		void DestroyPhysicsActor(entt::entity handle, entt::registry& registry);

		// 물리 씬 중력 수치 설정
		void SetGravity(Vector3 gravity) const;

		// 물리 씬 중력 반환
		Vector3 GetGravity() const;

		// 엔티티 중력 영향 여부 설정
		void UseGravity(entt::entity entity, bool useGravity);

		// 힘
		void AddForce(entt::entity entity, Vector3 force, physics::ForceMode mode = physics::ForceMode::Force);

		// 회전력
		void AddTorque(entt::entity entity, Vector3 torque, physics::ForceMode mode = physics::ForceMode::Force);

		/// \brief 캐릭터 컨트롤러 컴포넌트를 가진 엔티티의 이동
		/// @param[in] entity 이동할 엔티티
		/// @param[in] controller 캐릭터 컨트롤러 컴포넌트
		/// @param[in] disp 현재 프레임의 벡터 변위 (중력으로 인한 수직 운동과 캐릭터가 움직일때의 측면 운동의 조합)
		/// @param[in] targetLayer 충돌 가능 레이어
		/// @param[in] elapsedTime 함수를 마지막으로 호출한 이후로 경과한 시간
		/// @param[in] minDist 남은 이동 거리가 한계 이하로 떨어질때 중지할 최소 길이
		void MoveCharacter(entt::entity entity, CharacterController& controller, Vector3 disp, uint32_t targetLayer, float elapsedTime, float minDist = 0.01f);

		// 머터리얼 로드
		bool LoadMaterial(const std::filesystem::path& path);

		/// \brief 시작점으로부터 지정 방향으로 최대 거리까지 충돌 검사
		/// \param[in] origin 레이의 시작 위치
		/// \param[in] direction 검사할 방향 (Normalize 필요)
		/// \param[ref] hits 충돌 정보를 저장할 구조체
		/// \param maxHit
		/// \param[in] maxDistance 최대 검사 거리
		/// \param[in] layerMask 충돌 검사에 사용할 레이어 마스크
		/// \param[in] queryTriggerInteraction 트리거 오브젝트와의 상호작용 설정
		/// \return 충돌 성공 여부
		bool Raycast(Vector3 origin, Vector3 direction, std::vector<physics::RaycastHit>& hits, uint32_t maxHit, float maxDistance = FLT_MAX, uint32_t layerMask = UINT_MAX & ~layer::IgnoreRaycast::mask, physics::QueryTriggerInteraction queryTriggerInteraction = physics::QueryTriggerInteraction::Collide);

		// Raycast 참고
		bool Raycast(const physics::Ray& ray, std::vector<physics::RaycastHit>& hitInfo, uint32_t maxHit, float maxDistance = FLT_MAX, uint32_t layerMask = UINT_MAX & ~layer::IgnoreRaycast::mask, physics::QueryTriggerInteraction queryTriggerInteraction = physics::QueryTriggerInteraction::Collide);

		/// \brief 중심으로부터 박스 크기의 영역을 지정 방향으로 최대 거리까지 충돌 검사
		/// \param[in] center 상자의 중심 위치
		/// \param[in] halfExtents 상자의 각 차원에 대한 절반 크기 (x, y, z 방향의 반 길이)
		/// \param[in] direction 검사할 방향 (Normalize 필요)
		/// \param[ref] hits 충돌 정보를 저장할 구조체
		/// \param[in] orientation 상자의 회전을 나타내는 쿼터니언
		/// \param[in] maxDistance 최대 검사 거리
		/// \param[in] layerMask 충돌 검사에 사용할 레이어 마스크
		/// \param[in] queryTriggerInteraction 트리거 오브젝트와의 상호작용 설정
		/// \return 충돌 성공 여부
		bool Boxcast(Vector3 center, Vector3 halfExtents, Vector3 direction, std::vector<physics::RaycastHit>& hits, Quaternion orientation = Quaternion::Identity, float maxDistance = FLT_MAX, uint32_t layerMask = UINT_MAX & ~layer::IgnoreRaycast::mask, physics::QueryTriggerInteraction queryTriggerInteraction = physics::QueryTriggerInteraction::Collide);

		/// @brief 물리 씬에 속해 있는 정적 객체들의 폴리곤을 가져옴
		/// @param[out] vertices 버텍스 버퍼
		/// @param[out] indices 인덱스 버퍼
		/// @param[in] layerMask 폴리곤을 가져올 객체가 속한 레이어 (default: 모두)
		void GetStaticPoly(std::vector<float>& vertices, std::vector<int>& indices, uint32_t layerMask = UINT_MAX);

		// 속도 설정
		void SetLinearVelocity(entt::entity entity, const Vector3& velocity);

		// 위치 업데이트
		void UpdateTransform(entt::entity entity, const WorldTransform& world);

		// 위치 업데이트
		void UpdateTransform(entt::entity entity);

		// 리지드바디 업데이트
		void UpdateRigidbody(entt::entity entity, const Rigidbody& rigid, bool wakeUp = true);

		// 콜라이더 업데이트
		void UpdateCollider(entt::entity entity, const ColliderCommon& collider);

		// 물리 씬 초기화
		void Clear();

	private:
		// 씬 패치 (값 할당)
		void sceneFetch();

		// 기본 콜라이더
		physx::PxShape* createShape(Entity entity, physx::PxMaterial* material);

		// 볼록 메쉬 콜라이더 생성
		physx::PxShape* createConvexMesh(const MeshRenderer& meshData, const MeshCollider& collider, const Vector3& worldScale, physx::PxMaterial* material);

		// 삼각형 메쉬 콜라이더 생성
		physx::PxShape* createTriangleMesh(const MeshRenderer& meshData, const MeshCollider& collider, const Vector3& worldScale, physx::PxMaterial* material);

		// 캐릭터 컨트롤러 생성
		physx::PxController* createController(physx::PxMaterial* material, const CharacterController& controller, const physx::PxVec3T<double>& worldPos);

		// 액터 생성
		physx::PxActor* createActor(Entity entity, physx::PxShape* shape);

		// 필터 설정
		bool setFilter(Entity entity, physx::PxShape* shape, physx::PxActor* actor);


		Scene* _scene = nullptr;
		bool _isStart = false;
		physx::PxScene* _pxScene = nullptr;
		physx::PxControllerManager* _ctManager = nullptr;

		std::unordered_map<entt::entity, physx::PxRigidDynamic*> _entityToDynamic;
		std::unordered_map<entt::entity, physx::PxRigidStatic*> _entityToStatic;
		std::queue<physx::PxRigidStatic*> _modifiedEntities;
		std::unordered_map<entt::entity, uint32_t> _entityToCharacterIndex;
		uint32_t _controllerCount = 0;

		// 물리 씬간 공유 자원
		inline static PxResources _resources;
	};
}

